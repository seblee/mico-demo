/**
 ******************************************************************************
 * @file    wifi_station.c
 * @author  William Xu
 * @version V1.0.0
 * @date    21-May-2015
 * @brief   Connect to access point using core MiCO wlan APIs
 ******************************************************************************
 *
 *  The MIT License
 *  Copyright (c) 2014 MXCHIP Inc.
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
#include "mdns.h"

#define QUERY_DEBUG MICO_DEBUG_ON
#define qurey_printf(M, ...) MICO_PRINT(QUERY_DEBUG, M, ##__VA_ARGS__)

char service_type[]="_airplay._tcp.local";

static int _mdns_query_cb(void *data, const struct mdns_service *s, int status)
{
    struct in_addr addr;

    qurey_printf("\r\n==================Got callback for %s======================\r\n", s->servname);

    if ( status == MDNS_DISCOVERED )
        qurey_printf("DISCOVERED:\r\n");
    else if ( status == MDNS_CACHE_FULL )
        qurey_printf("NOT_CACHED:\r\n");
    else if ( status == MDNS_DISAPPEARED )
        qurey_printf("DISAPPEARED:\r\n");
    else if ( status == MDNS_UPDATED )
        qurey_printf("UPDATED:\r\n");
    else {
        qurey_printf("Warning: unknown status %d\r\n", status);
        return kNoErr;
    }
    qurey_printf("%s._%s._%s.%s. port:%d\r\n", s->servname, s->servtype,
                 s->proto == MDNS_PROTO_UDP ? "udp" : "tcp",
                 s->domain, s->port);

    addr.s_addr = s->ipaddr;
    qurey_printf("at %s\r\n", inet_ntoa(addr));

    qurey_printf("TXT : %s\r\n", s->keyvals ? s->keyvals : "no key vals");

    return kNoErr;
}

int main(void)
{
    OSStatus err = kNoErr;

    /* Initialize system core data */
    mico_Context_t *context = mico_system_context_init( 0 );

    /* Initialize tcpip, Wi-Fi stacks and system functions */
    err = mico_system_init( context );

    /* Initialize mdns protocol and start query */
    mdns_start(NULL, DEFAULT_NAME);
    mdns_query_monitor(service_type, _mdns_query_cb, NULL, INTERFACE_STA);

    return err;
}


