/**
 ******************************************************************************
 * @file    dns.c
 * @author  William Xu
 * @version V1.0.0
 * @date    21-May-2015
 * @brief   Get the IP address from a host name.(DNS)
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

#define dns_log(M, ...) custom_log("DNS", M, ##__VA_ARGS__)

static char *domain = "www.baidu.com";

int main( void )
{
    OSStatus err = kNoErr;
    char **pptr = NULL;
    struct hostent* host = NULL;
    struct in_addr in_addr;

    /* Start MiCO system functions according to mico_config.h*/
    err = mico_system_init( mico_system_context_init( 0 ) );
    require_noerr( err, exit );

    while ( 1 ) {
        /* Resolve DNS address */
        dns_log( "Requesting server address..." );
        host = gethostbyname( domain );
        require_action_quiet( host != NULL, exit, err = kNotFoundErr );

        pptr = host->h_addr_list;
        for ( ; *pptr != NULL; pptr++ ) {
            in_addr.s_addr = *(uint32_t *) (*pptr);
            dns_log( "%s ip address: %s", domain, inet_ntoa(in_addr));
        }
        mico_rtos_delay_milliseconds( 2000 );
    }

exit:
    return err;
}

