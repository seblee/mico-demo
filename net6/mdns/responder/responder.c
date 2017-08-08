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

struct mdns_service demo_service;
char   keyvals[]="txtvers=2/.0/.1.path=/mico_demo";

int application_start(void)
{
    OSStatus err = kNoErr;

    /* Initialize system core data */
    mico_Context_t *context = mico_system_context_init( 0 );

    /* Initialize tcpip, Wi-Fi stacks and system functions */
    err = mico_system_init( context );

    /* Initialize mdns protocol and add a service*/
    memset(&demo_service, 0x0, sizeof(demo_service));
    demo_service.servname = "demo_web",
    demo_service.servtype = "http",
    demo_service.domain = ".local",
    demo_service.port = 80,
    demo_service.proto = MDNS_PROTO_TCP,
    mdns_set_txt_rec(&demo_service, keyvals, '.');

    mdns_start(NULL, DEFAULT_NAME);
    mdns_announce_service(&demo_service, INTERFACE_STA);

    return err;
}

