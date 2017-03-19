/**
 ******************************************************************************
 * @file    MICOAppEntrance.c
 * @author  William Xu
 * @version V1.0.0
 * @date    05-May-2014
 * @brief   Mico application entrance, addd user application functons and threads.
 ******************************************************************************
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

#include "StringUtils.h"
#include "SppProtocol.h"
#include "cfunctions.h"
#include "cppfunctions.h"
#include "MICOAppDefine.h"

#define app_log(M, ...) custom_log("APP", M, ##__VA_ARGS__)
#define app_log_trace() custom_log_trace("APP")

static mico_Context_t* mico_context;

/* MICO system callback: Restore default configuration provided by application */
void appRestoreDefault_callback( void * const user_config_data, uint32_t size )
{
    UNUSED_PARAMETER( size );
    para_restore_default( );
}

/* EasyLink callback: Notify wlan configuration type */
USED void mico_system_delegate_config_success( mico_config_source_t source )
{
    app_log( "Configured by %d", source );
}

/* Config server callback: Current Device configuration sent to config client */
USED void config_server_delegate_report( json_object *app_menu, mico_Context_t *in_context )
{
    return;
}

/* Config server callback: New Device configuration received from config client */
USED void config_server_delegate_recv( const char *key, json_object *value, bool *need_reboot,
                                       mico_Context_t *in_context )
{
    return;
}
 
int application_start( void )
{
    app_log_trace();
    OSStatus err = kNoErr;
    app_context_t* app_context;

    /* Create application context */
    app_context = (app_context_t *) calloc( 1, sizeof(app_context_t) );
    require_action( app_context, exit, err = kNoMemoryErr );

    /* Create mico system context and read application's config data from flash */
    mico_context = mico_system_context_init( sizeof(application_config_t) );
    app_context->appConfig = mico_system_context_get_user_data( mico_context );
    
    app_state_config(app_context);
      
    /* mico system initialize */
    app_init_for_mico_system(mico_context);
      
     /* Protocol initialize */
    sppProtocolInit( app_context );

    /*UART receive initialize*/
    uart_recv_data_mode_init( app_context );
    
    /*wlan initialize*/
    app_wlan_init( app_context );
    
    /*tcp ip intialize*/
    app_tcp_ip_init( app_context );
    
exit:
    mico_rtos_delete_thread( NULL );
    return err;
}
