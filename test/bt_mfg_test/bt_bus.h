/**
 *  UNPUBLISHED PROPRIETARY SOURCE CODE
 *  Copyright (c) 2016 MXCHIP Inc.
 *
 *  The contents of this file may not be disclosed to third parties, copied or
 *  duplicated in any form, in whole or in part, without the prior written
 *  permission of MXCHIP Corporation.
 *
 */


#pragma once

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/
  
/* Macro for checking if bus is ready */
#define BT_BUS_IS_READY( ) \
do \
{ \
    if ( bt_mfg_bus_is_ready( ) == MICO_FALSE ) \
    { \
        check_string( 0!=0, "bus not ready" ); \
        return kNotPreparedErr; \
    } \
}while ( 0 )

/* Macro for waiting until bus is ready */
#define BT_BUS_WAIT_UNTIL_READY( ) \
do \
{ \
    while ( bt_mfg_bus_is_ready( ) == MICO_FALSE ) \
    { \
        mico_rtos_delay_milliseconds( 10 ); \
    } \
} while ( 0 )

/******************************************************
 *                    Constants
 ******************************************************/

/* Should be overridden by application. If undefined, set to 512 bytes. */
#ifndef BT_BUS_RX_FIFO_SIZE
#define BT_BUS_RX_FIFO_SIZE (512)
#endif

#ifndef BLUETOOTH_CHIP_STABILIZATION_DELAY
#define BLUETOOTH_CHIP_STABILIZATION_DELAY    (100)
#endif

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

OSStatus bt_mfg_bus_init( void );

OSStatus bt_mfg_bus_deinit( void );

OSStatus bt_mfg_bus_transmit( const uint8_t* data_out, uint32_t size );

OSStatus bt_mfg_bus_receive( uint8_t* data_in, uint32_t size, uint32_t timeout_ms );

mico_bool_t   bt_mfg_bus_is_ready( void );

mico_bool_t   bt_mfg_bus_is_on( void );

#ifdef __cplusplus
} /* extern "C" */
#endif
