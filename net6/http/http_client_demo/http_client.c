/**
 ******************************************************************************
 * @file    http_client.c
 * @author  William Xu
 * @version V1.0.0
 * @date    21-May-2015
 * @brief   MiCO http client demo to read data from www.baidu.com
 ******************************************************************************
 *  The MIT License
 *  Copyright (c) 2016 MXCHIP Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 */

#include "mico.h"
#include "HTTPUtils.h"
#include "SocketUtils.h"
#include "StringUtils.h"

#define http_client_log(M, ...) custom_log("HTTP", M, ##__VA_ARGS__)

static OSStatus onReceivedData( struct _HTTPHeader_t * httpHeader,
                                uint32_t pos,
                                uint8_t *data,
                                size_t len,
                                void * userContext );
static void onClearData( struct _HTTPHeader_t * inHeader, void * inUserContext );

static mico_semaphore_t wait_sem = NULL;

typedef struct _http_context_t
{
    char *content;
    uint64_t content_length;
} http_context_t;

OSStatus simple_http_get( char* host, char* query );
OSStatus simple_https_get( char* host, char* query );

#define SIMPLE_GET_REQUEST \
    "GET / HTTP/1.1\r\n" \
    "Host: www.baidu.com\r\n" \
    "Connection: close\r\n" \
    "\r\n"

static void micoNotify_WifiStatusHandler( WiFiEvent status, void* const inContext )
{
    UNUSED_PARAMETER( inContext );
    switch ( status )
    {
        case NOTIFY_STATION_UP:
            if ( wait_sem != NULL ) mico_rtos_set_semaphore(&wait_sem);
            break;
        case NOTIFY_STATION_DOWN:
            case NOTIFY_AP_UP:
            case NOTIFY_AP_DOWN:
            break;
    }
}

/* Only return the first address of a domain */
static OSStatus dns_lookup_by_family( char* host, char* port, int ai_family, struct sockaddr_storage* sockaddr )
{
    OSStatus err = kNoErr;
    struct addrinfo hints;
    struct addrinfo *result = NULL;

    /* Resolve DNS address */
    memset( &hints, 0, sizeof(struct addrinfo) );
    hints.ai_family = ai_family; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* STREAM socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0; /* Any protocol */

    err = getaddrinfo( host, port, &hints, &result );
    require_noerr( err, exit );
    require_action( result, exit, err = kNotFoundErr );

    memcpy(sockaddr, result->ai_addr, sizeof(struct sockaddr_storage) );

exit:
    freeaddrinfo( result ); /* No longer needed */
    return err;
}

static OSStatus dns_lookup(char* host, char* port, struct sockaddr_storage* sockaddr)
{
    OSStatus err = kNoErr;
    char ipstr[INET6_ADDRSTRLEN];
    struct sockaddr_in *sockaddr_v4 = (struct sockaddr_in *)sockaddr;
    struct sockaddr_in6 *sockaddr_v6 = (struct sockaddr_in6 *)sockaddr;

    /* Lookup IPv6 address */
    if ( kNoErr == dns_lookup_by_family(host, port, AF_INET6, sockaddr) ) {
        inet_ntop(AF_INET6, sockaddr_v6->sin6_addr.s6_addr, ipstr, INET6_ADDRSTRLEN);
        http_client_log("Server %s address: %s, port: %d", host, ipstr, ntohs(sockaddr_v6->sin6_port));
    }
    else if ( kNoErr == dns_lookup_by_family(host, port, AF_INET, sockaddr) ) {
        inet_ntop(AF_INET, &sockaddr_v4->sin_addr.s_addr, ipstr, INET_ADDRSTRLEN);
        http_client_log("Server %s address: %s, port: %d", host, ipstr, ntohs(sockaddr_v4->sin_port));
    }
    else {
        http_client_log("Server not available");
        goto exit;
    }
exit:
    return err;
}


int application_start( void )
{
    OSStatus err = kNoErr;

    mico_rtos_init_semaphore( &wait_sem, 1 );

    /*Register user function for MiCO nitification: WiFi status changed */
    err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED,
                                       (void *) micoNotify_WifiStatusHandler, NULL );
    require_noerr( err, exit );

    /* Start MiCO system functions according to mico_config.h */
    err = mico_system_init( mico_system_context_init( 0 ) );
    require_noerr( err, exit );

    /* Wait for wlan connection*/
    mico_rtos_get_semaphore( &wait_sem, MICO_WAIT_FOREVER );
    http_client_log( "wifi connected successful" );

    /* Read http data from server */
    simple_http_get( "www.baidu.com", SIMPLE_GET_REQUEST );
    simple_https_get( "www.baidu.com", SIMPLE_GET_REQUEST );

exit:
    mico_system_notify_remove( mico_notify_WIFI_STATUS_CHANGED, (void *) micoNotify_WifiStatusHandler );
    if ( wait_sem != NULL )  mico_rtos_deinit_semaphore( &wait_sem );
    mico_rtos_delete_thread( NULL );
    return err;
}

OSStatus simple_http_get( char* host, char* query )
{
    OSStatus err = kNoErr;
    int client_fd = -1;
    fd_set readfds;
    struct sockaddr_storage sockaddr;

    HTTPHeader_t *httpHeader = NULL;
    http_context_t context = { NULL, 0 };

    /* Lookup server address */
    err = dns_lookup(host, "80", &sockaddr);
    require_noerr( err, exit);

    /*HTTPHeaderCreateWithCallback set some callback functions */
    httpHeader = HTTPHeaderCreateWithCallback( 1024, onReceivedData, onClearData, &context );
    require_action( httpHeader, exit, err = kNoMemoryErr );

    client_fd = socket( sockaddr.ss_family, SOCK_STREAM, IPPROTO_TCP );
    err = connect( client_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr) );
    require_noerr_string( err, exit, "connect http server failed" );

    /* Send HTTP Request */
    send( client_fd, query, strlen( query ), 0 );

    FD_ZERO( &readfds );
    FD_SET( client_fd, &readfds );

    select( client_fd + 1, &readfds, NULL, NULL, NULL );
    if ( FD_ISSET( client_fd, &readfds ) )
    {
        /*parse header*/
        err = SocketReadHTTPHeader( client_fd, httpHeader );
        switch ( err )
        {
            case kNoErr:
                PrintHTTPHeader( httpHeader );
                err = SocketReadHTTPBody( client_fd, httpHeader );/*get body data*/
                require_noerr( err, exit );
                /*get data and print*/
                http_client_log( "Content Data: %s", context.content );
                break;
            case EWOULDBLOCK:
                case kNoSpaceErr:
                case kConnectionErr:
                default:
                http_client_log("ERROR: HTTP Header parse error: %d", err);
                break;
        }
    }

exit:
    http_client_log( "Exit: Client exit with err = %d, fd: %d", err, client_fd );
    SocketClose( &client_fd );
    HTTPHeaderDestory( &httpHeader );
    return err;
}

int ssl_errno = 0;

OSStatus simple_https_get( char* host, char* query )
{
    OSStatus err = kNoErr;
    int client_fd = -1;
    fd_set readfds;
    struct sockaddr_storage sockaddr;

    mico_ssl_t client_ssl = NULL;

    HTTPHeader_t *httpHeader = NULL;
    http_context_t context = { NULL, 0 };

    /* Lookup server address */
    err = dns_lookup(host, "443", &sockaddr);
    require_noerr( err, exit);

    /*HTTPHeaderCreateWithCallback set some callback functions */
    httpHeader = HTTPHeaderCreateWithCallback( 1024, onReceivedData, onClearData, &context );
    require_action( httpHeader, exit, err = kNoMemoryErr );

    client_fd = socket( sockaddr.ss_family, SOCK_STREAM, IPPROTO_TCP );
    err = connect( client_fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr) );
    require_noerr_string( err, exit, "connect HTTPs server failed" );

    ssl_set_client_version(TLS_V1_2_MODE);
    client_ssl = ssl_connect( client_fd, 0, NULL, &ssl_errno );
    require_string( client_ssl != NULL, exit, "ERROR: ssl disconnect" );

    /* Send HTTP Request */
    ssl_send( client_ssl, query, strlen( query ) );

    FD_ZERO( &readfds );
    FD_SET( client_fd, &readfds );

    select( client_fd + 1, &readfds, NULL, NULL, NULL );
    if ( FD_ISSET( client_fd, &readfds ) )
    {
        /*parse header*/
        err = SocketReadHTTPSHeader( client_ssl, httpHeader );
        switch ( err )
        {
            case kNoErr:
                PrintHTTPHeader( httpHeader );
                err = SocketReadHTTPSBody( client_ssl, httpHeader );/*get body data*/
                require_noerr( err, exit );
                /*get data and print*/
                http_client_log( "Content Data: %s", context.content );
                break;
            case EWOULDBLOCK:
                case kNoSpaceErr:
                case kConnectionErr:
                default:
                http_client_log("ERROR: HTTP Header parse error: %d", err);
                break;
        }
    }

exit:
    http_client_log( "Exit: Client exit with err = %d, fd: %d", err, client_fd );
    if ( client_ssl ) ssl_close( client_ssl );
    SocketClose( &client_fd );
    HTTPHeaderDestory( &httpHeader );
    return err;
}

/*one request may receive multi reply*/
static OSStatus onReceivedData( struct _HTTPHeader_t * inHeader, uint32_t inPos, uint8_t * inData,
                                size_t inLen, void * inUserContext )
{
    OSStatus err = kNoErr;
    http_context_t *context = inUserContext;
    if ( inHeader->chunkedData == false )
    { //Extra data with a content length value
        if ( inPos == 0 && context->content == NULL )
        {
            context->content = calloc( inHeader->contentLength + 1, sizeof(uint8_t) );
            require_action( context->content, exit, err = kNoMemoryErr );
            context->content_length = inHeader->contentLength;

        }
        memcpy( context->content + inPos, inData, inLen );
    } else
    { //extra data use a chunked data protocol
        http_client_log("This is a chunked data, %d", inLen);
        if ( inPos == 0 )
        {
            context->content = calloc( inHeader->contentLength + 1, sizeof(uint8_t) );
            require_action( context->content, exit, err = kNoMemoryErr );
            context->content_length = inHeader->contentLength;
        } else
        {
            context->content_length += inLen;
            context->content = realloc( context->content, context->content_length + 1 );
            require_action( context->content, exit, err = kNoMemoryErr );
        }
        memcpy( context->content + inPos, inData, inLen );
    }

    exit:
    return err;
}

/* Called when HTTPHeaderClear is called */
static void onClearData( struct _HTTPHeader_t * inHeader, void * inUserContext )
{
    UNUSED_PARAMETER( inHeader );
    http_context_t *context = inUserContext;
    if ( context->content )
    {
        free( context->content );
        context->content = NULL;
    }
}

