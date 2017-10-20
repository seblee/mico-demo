/**
 ******************************************************************************
 * @file    ota.c
 * @author  QQ ding
 * @version V1.0.0
 * @date    219-Oct-2016
 * @brief   Firmware update example
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
#include "ota_server.h"

#define ota_log(M, ...) custom_log("OTA", M, ##__VA_ARGS__)

static mico_semaphore_t wait_sem = NULL;

static void micoNotify_WifiStatusHandler( WiFiEvent status, void* const inContext )
{
    switch ( status )
    {
        case NOTIFY_STATION_UP:
            mico_rtos_set_semaphore( &wait_sem );
            break;
         default:
            break;
    }
}

static void ota_server_status_handler(OTA_STATE_E state, float progress)
{
    switch ( state )
    {
        case OTA_LOADING:
            ota_log("ota server is loading, progress %.2f%%", progress);
            break;
        case OTA_SUCCE:
            ota_log("ota server daemons success");
            break;
        case OTA_FAIL:
            ota_log("ota server daemons failed");
            break;
        default:
            break;
    }
}

static void cli_ota(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
    if( argc < 2 ) {
        snprintf( pcWriteBuffer, xWriteBufferLen, "Please specify a correct operation: [start | pause | resume | stop ] ");
        return;
    }

    if ( strcmp( argv[1], "start") == 0) {
        if ( argc < 3 ) {
            snprintf( pcWriteBuffer, xWriteBufferLen, "Please specify a correct URL");
            return;
        }
        snprintf( pcWriteBuffer, xWriteBufferLen, "OTA from server %s", argv[2]);
        ota_server_start(argv[2], (argc >= 4)? argv[3]:NULL, ota_server_status_handler);
    }
    else if ( strcmp( argv[1], "pause") == 0 ) {
        snprintf( pcWriteBuffer, xWriteBufferLen, "Pause...");
        ota_server_pause();
    }
    else if ( strcmp( argv[1], "resume") == 0 ) {
        snprintf( pcWriteBuffer, xWriteBufferLen, "Continue...");
        ota_server_continue();
    }
    else if ( strcmp( argv[1], "stop") == 0 ) {
        snprintf( pcWriteBuffer, xWriteBufferLen, "Stop...");
        ota_server_stop();
    }
}

static const struct cli_command ota_clis[] = {
    {"otad", "ota [start | pause | resume | stop ] [url] <md5>, Download OTA data from [url], check <md5> if available, and replace current firmware", cli_ota},
};

int main( void )
{
    OSStatus err = kNoErr;

    mico_rtos_init_semaphore( &wait_sem, 1 );

    /*Register user function for MiCO nitification: WiFi status changed */
    err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED,
                                       (void *) micoNotify_WifiStatusHandler,
                                       NULL );
    require_noerr( err, exit );

    /* Start MiCO system functions according to mico_config.h*/
    err = mico_system_init( mico_system_context_init( 0 ) );
    require_noerr( err, exit );

    /* Wait for wlan connection*/
    mico_rtos_get_semaphore( &wait_sem, MICO_WAIT_FOREVER );
    ota_log( "wifi connected successful, input \"help\" for useful commands." );

    cli_register_commands(ota_clis, sizeof(ota_clis)/sizeof(struct cli_command));

exit:
    if ( wait_sem ) mico_rtos_deinit_semaphore( &wait_sem );
    return err;
}

