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

#include "mico_rtos.h"
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

/* Many of the callbacks of higher layer Bluetooth protocols run on WICED_NETWORK_WORKER_PRIORITY
 * context. Bluetooth transport thread priority is to 1 higher than that of
 * WICED_NETWORK_WORKER_PRIORITY to let it preempt WICED_NETWORK_WORKER_PRIORITY.
 */
#define BT_TRANSPORT_THREAD_PRIORITY MICO_NETWORK_WORKER_PRIORITY - 1

/* ~4K of stack space is for printf and stack check.
 */
#define BT_TRANSPORT_STACK_SIZE      4096


#define BT_TRANSPORT_QUEUE_SIZE      10

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef OSStatus (*bt_transport_thread_received_packet_handler_t)( bt_packet_t* packet );
typedef OSStatus (*bt_transport_thread_callback_handler_t)( void* arg );

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

OSStatus bt_transport_thread_init( bt_transport_thread_received_packet_handler_t handler );

OSStatus bt_transport_thread_deinit( void );

OSStatus bt_transport_thread_send_packet( bt_packet_t* packet );

OSStatus bt_transport_thread_notify_packet_received( void );

OSStatus bt_transport_thread_execute_callback( bt_transport_thread_callback_handler_t callback_handler, void* arg );

OSStatus bt_transport_thread_enable_packet_dump( void );

OSStatus bt_transport_thread_disable_packet_dump( void );

#ifdef __cplusplus
} /* extern "C" */
#endif
