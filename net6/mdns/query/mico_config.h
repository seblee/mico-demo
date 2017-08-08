/**
******************************************************************************
* @file    mico_config.h
* @author  William Xu
* @version V1.0.0
* @date    08-Aug-2018
* @brief   This file provide application options diff to default.
******************************************************************************
*/


#ifndef __MICO_CONFIG_H
#define __MICO_CONFIG_H

/************************************************************************
 * Enable IPv4 and IPv6 dual stack apis */
#define MICO_CONFIG_IPV6             1

/************************************************************************
 * mdns options */
#define CONFIG_MDNS_QUERY            1
#define CONFIG_MDNS_DEBUG            MICO_DEBUG_OFF


#endif
