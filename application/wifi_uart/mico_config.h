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

#define APP_INFO   "SPP(wlan<>uart) Demo based on MICO OS"

#define FIRMWARE_REVISION   "MICO_SPP_2_6"
#define MANUFACTURER        "MXCHIP Inc."
#define SERIAL_NUMBER       "20140606"
#define PROTOCOL            "com.mxchip.spp"

/************************************************************************
 * Application thread stack size */
#define MICO_DEFAULT_APPLICATION_STACK_SIZE         (4096)

/************************************************************************
 * MiCO TCP server used for configuration and ota. */
#define MICO_CONFIG_SERVER_ENABLE                   1

#endif
