/**
 *  UNPUBLISHED PROPRIETARY SOURCE CODE
 *  Copyright (c) 2016 MXCHIP Inc.
 *
 *  The contents of this file may not be disclosed to third parties, copied or
 *  duplicated in any form, in whole or in part, without the prior written
 *  permission of MXCHIP Corporation.
 *
 */

#include "mico.h"
#include "bt_bus.h"
#include "bt_transport_driver.h"
#include "LinkListUtils.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/* Driver thread priority is set to 1 higher than BT transport thread */
#define BT_UART_THREAD_PRIORITY MICO_NETWORK_WORKER_PRIORITY - 2
#define BT_UART_THREAD_NAME     "BT UART"
#define BT_UART_STACK_SIZE      600
#define BT_UART_PACKET_TYPE     0x0A

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
 *               Static Function Declarations
 ******************************************************/

static void bt_transport_driver_uart_thread_main  ( void* arg );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static bt_transport_driver_event_handler_t    driver_event_handler    = NULL;
static bt_transport_driver_bus_read_handler_t driver_bus_read_handler = NULL;
static volatile mico_bool_t                   driver_initialised      = MICO_FALSE;
static volatile mico_bool_t                   uart_thread_running     = MICO_FALSE;
static mico_thread_t                          uart_thread;
static mico_mutex_t                           packet_list_mutex;
static linked_list_t                          uart_rx_packet_list;

/******************************************************
 *               Function Definitions
 ******************************************************/

OSStatus bt_transport_driver_init( bt_transport_driver_event_handler_t event_handler, bt_transport_driver_bus_read_handler_t bus_read_handler )
{
    OSStatus result;
    mico_bool_t   ready;

    if ( event_handler == NULL || bus_read_handler == NULL )
    {
        return MICO_BT_BADARG;
    }

    if ( driver_initialised == MICO_TRUE )
    {
        return MICO_BT_SUCCESS;
    }

    /* Check if bus is ready */
    ready = bt_mfg_bus_is_ready( );
    require_action_string( ready == MICO_TRUE, error, result = MICO_BT_BUS_UNINITIALISED, "BT bus is NOT ready" );

    driver_initialised = MICO_TRUE;

    /* Create a linked list to hold packet momentarily before being passed to
     * the transport thread.
     */
    result = linked_list_init( &uart_rx_packet_list );
    require_noerr_string(result, error, "Error creating UART RX packet linked list");

    /* Create a semaphore. Once set, this semaphore is used to notify the UART
     * thread that the packet has been read by the upper layer. The UART thread
     * can now continue polling for another packet from the UART circular buffer.
     */
    result = mico_rtos_init_mutex( &packet_list_mutex );
    require_noerr_string(result, error, "Error creating UART driver mutex");

    /* Create UART thread. WICED UART API does not support callback mechanism.
     * The API blocks in semaphore until the transmission is complete.
     * Consequently, a dedicated thread is required to recieve and dispatch
     * incoming packets to the upper layer.
     */
    uart_thread_running     = MICO_TRUE;
    driver_event_handler    = event_handler;
    driver_bus_read_handler = bus_read_handler;

    result = mico_rtos_create_thread( &uart_thread, BT_UART_THREAD_PRIORITY, BT_UART_THREAD_NAME, bt_transport_driver_uart_thread_main, BT_UART_STACK_SIZE, NULL );
    require_noerr_string(result, error, "Error creating UART driver thread");

    return result;

    error:
    bt_transport_driver_deinit();
    return result;
}

OSStatus bt_transport_driver_deinit( void )
{
    if ( driver_initialised == MICO_FALSE )
    {
        return MICO_BT_SUCCESS;
    }

    uart_thread_running = MICO_FALSE;
    mico_rtos_delete_thread( &uart_thread );
    mico_rtos_deinit_mutex( &packet_list_mutex );
    linked_list_deinit( &uart_rx_packet_list );
    driver_event_handler    = NULL;
    driver_bus_read_handler = NULL;
    driver_initialised      = MICO_FALSE;
    return MICO_BT_SUCCESS;
}

OSStatus bt_transport_driver_send_packet( bt_packet_t* packet )
{
    OSStatus result;

    result = bt_mfg_bus_transmit( packet->packet_start, (uint32_t)(packet->data_end - packet->packet_start) );
    require_noerr_string(result, exit, "Error transmitting MPAF packet");

    /* Destroy packet */
    result = bt_packet_pool_free_packet( packet );
exit:
    return result;

}

OSStatus bt_transport_driver_receive_packet( bt_packet_t** packet )
{
    uint32_t            count;
    linked_list_node_t* node;
    OSStatus            result;

    linked_list_get_count( &uart_rx_packet_list, &count );

    if ( count == 0 )
    {
        return MICO_BT_PACKET_POOL_EXHAUSTED;
    }

    mico_rtos_lock_mutex( &packet_list_mutex );

    result = linked_list_remove_node_from_front( &uart_rx_packet_list, &node );
    if ( result == MICO_BT_SUCCESS )
    {
        *packet = (bt_packet_t*)node->data;
    }

    mico_rtos_unlock_mutex( &packet_list_mutex );
    return result;
}

static void bt_transport_driver_uart_thread_main( void* arg )
{
    check_string( driver_bus_read_handler != NULL, "driver_bus_read_handler isn't set" );
    check_string( driver_event_handler    != NULL, "driver_event_handler isn't set"    );

    while ( uart_thread_running == MICO_TRUE )
    {
        bt_packet_t* packet = NULL;

        if ( driver_bus_read_handler( &packet ) != MICO_BT_SUCCESS )
        {
            continue;
        }

        /* Read successful. Notify upper layer via driver_callback that a new packet is available */
        mico_rtos_lock_mutex( &packet_list_mutex );
        linked_list_set_node_data( &packet->node, (void*)packet );
        linked_list_insert_node_at_rear( &uart_rx_packet_list, &packet->node );
        mico_rtos_unlock_mutex( &packet_list_mutex );
        driver_event_handler( TRANSPORT_DRIVER_INCOMING_PACKET_READY );
    }

    mico_rtos_delete_thread(NULL);
}
