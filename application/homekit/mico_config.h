/**
 ******************************************************************************
 * @file    mico_config.h
 * @author  William Xu
 * @version V1.0.0
 * @date    12-Aug-2015
 * @brief   This file provide constant definition and type declaration for MICO
 *          system running.
 ******************************************************************************
 *  UNPUBLISHED PROPRIETARY SOURCE CODE
 *  Copyright (c) 2016 MXCHIP Inc.
 *
 *  The contents of this file may not be disclosed to third parties, copied or
 *  duplicated in any form, in whole or in part, without the prior written
 *  permission of MXCHIP Corporation.
 ******************************************************************************
 */

#pragma once

#define APP_INFO   "Apple HomeKit Demo based on MICO OS"

#define FIRMWARE_REVISION   "MICO_HOMEKIT_1"
#define MANUFACTURER        "MXCHIP Inc."
#define SERIAL_NUMBER       "20140606"
#define PROTOCOL            "com.apple.homekit"

/************************************************************************
 * Application thread stack size */
#define MICO_DEFAULT_APPLICATION_STACK_SIZE         (2048)

/************************************************************************
 * Enable wlan connection, start easylink configuration if no wlan settings are existed */
#define MICO_WLAN_CONNECTION_ENABLE

#define MICO_WLAN_CONFIG_MODE CONFIG_MODE_WAC   //CONFIG_MODE_WAC needs a MFi CP on i2c bus

#define EasyLink_TimeOut                60000 /**< EasyLink timeout 60 seconds. */

#define EasyLink_ConnectWlan_Timeout    20000 /**< Connect to wlan after configured by easylink.
                                                   Restart easylink after timeout: 20 seconds. */

/************************************************************************
 * Device enter MFG mode if MICO settings are erased. */
//#define MFG_MODE_AUTO 

/************************************************************************
 * Command line interface */
#define MICO_CLI_ENABLE  

/************************************************************************
 * Add service _easylink._tcp._local. for discovery */
#define MICO_SYSTEM_DISCOVERY_ENABLE   

/************************************************************************
 * Start a system monitor daemon, application can register some monitor  
 * points, If one of these points is not executed in a predefined period, 
 * a watchdog reset will occur. */
//#define MICO_SYSTEM_MONITOR_ENABLE

/************************************************************************
 * MiCO TCP server used for configuration and ota. */
#define MICO_CONFIG_SERVER_ENABLE 
#define MICO_CONFIG_SERVER_PORT    8000

