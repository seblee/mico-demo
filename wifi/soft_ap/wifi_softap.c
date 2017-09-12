/**
 ******************************************************************************
 * @file    wifi_softap.c
 * @author  William Xu
 * @version V1.0.0
 * @date    21-May-2015
 * @brief   Create an access point!
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
 *
 ******************************************************************************
 */

#include "mico.h"

#define app_log(M, ...) MICO_LOG(CONFIG_APP_DEBUG, "APP", M, ##__VA_ARGS__)

#define SOFTAP_SSID  "mxchip_zfw"
#define SOFTAP_KEY   "12345678"

int main( void )
{
    network_InitTypeDef_st wNetConfig;

    /* Setup WI-Fi and tcpip stack*/
    mico_Context_t* context = mico_system_context_init( 0 );
    mico_system_init( context );

    /* Setup Soft AP*/
    memset( &wNetConfig, 0x0, sizeof(network_InitTypeDef_st) );
    strcpy( (char*) wNetConfig.wifi_ssid, SOFTAP_SSID );
    strcpy( (char*) wNetConfig.wifi_key, SOFTAP_KEY );
    wNetConfig.wifi_mode = Soft_AP;
    wNetConfig.dhcpMode = DHCP_Server;
    strcpy( (char*) wNetConfig.local_ip_addr, "192.168.0.1" );
    strcpy( (char*) wNetConfig.net_mask, "255.255.255.0" );
    strcpy( (char*) wNetConfig.dnsServer_ip_addr, "192.168.0.1" );

    app_log("Establish Sodt AP, SSID:%s and KEY:%s", wNetConfig.wifi_ssid, wNetConfig.wifi_key);

    micoWlanStart( &wNetConfig );

    return 0;
}

