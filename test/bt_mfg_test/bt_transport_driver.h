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

#include "mico.h"
#include "bt_packet_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    TRANSPORT_DRIVER_INCOMING_PACKET_READY,
} bt_transport_driver_event_t;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef OSStatus (*bt_transport_driver_event_handler_t)   ( bt_transport_driver_event_t event );
typedef OSStatus (*bt_transport_driver_bus_read_handler_t)( bt_packet_t** packet );

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

OSStatus bt_transport_driver_init( bt_transport_driver_event_handler_t event_handler,  bt_transport_driver_bus_read_handler_t bus_read_handler );

OSStatus bt_transport_driver_deinit( void );

OSStatus bt_transport_driver_send_packet( bt_packet_t* packet );

OSStatus bt_transport_driver_receive_packet( bt_packet_t** packet );

#ifdef __cplusplus
} /* extern "C" */
#endif
