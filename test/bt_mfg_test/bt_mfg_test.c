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
#include "mico_rtos.h"
#include "mico_platform.h"
//#include "wiced_bt_platform.h"
#include "bt_hci_interface.h"
#include "bt_bus.h"
#include "bt_mfg_test.h"
#include "bt_transport_driver.h"
#include "bt_transport_thread.h"
#include "bt_firmware.h"

/******************************************************
 *                      Macros
 ******************************************************/

/* Verify if Bluetooth function returns success.
 * Otherwise, returns the error code immediately.
 * Assert in DEBUG build.
 */

/******************************************************
 *                    Constants
 ******************************************************/

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

extern OSStatus bt_mfgtest_transport_driver_bus_read_handler        ( bt_packet_t** packet );
static OSStatus bt_mfgtest_transport_driver_event_handler           ( bt_transport_driver_event_t event );
static OSStatus bt_mfgtest_transport_thread_received_packet_handler ( bt_packet_t* packet );

/******************************************************
 *               Variable Definitions
 ******************************************************/

static ring_buffer_t       pc_uart_ring_buffer;
static uint8_t             pc_uart_ring_buffer_data[BT_BUS_RX_FIFO_SIZE];
extern const char          brcm_patch_version[];
extern const uint8_t       brcm_patchram_buf[];
extern const int           brcm_patch_ram_length;

/******************************************************
 *               Function Definitions
 ******************************************************/

OSStatus bt_mfgtest_start( const mico_uart_config_t* config )
{
    OSStatus err = kNoErr;

    err = bt_mfg_bus_init();
    require_noerr_string(err, exit, "Error initialising BT bus");

    err = bt_firmware_download( );
    require_noerr_string(err, exit, "Error downloading HCI firmware");

    err = bt_transport_driver_init( bt_mfgtest_transport_driver_event_handler, bt_mfgtest_transport_driver_bus_read_handler );
    require_noerr_string(err, exit, "Error initialising BT transport driver");

    /* Initialise BT transport thread */
    err = bt_transport_thread_init( bt_mfgtest_transport_thread_received_packet_handler );
    require_noerr_string(err, exit, "Error initialising BT transport thread");

    ring_buffer_init( &pc_uart_ring_buffer, pc_uart_ring_buffer_data, BT_BUS_RX_FIFO_SIZE );

    err = MicoUartInitialize( STDIO_UART, config, &pc_uart_ring_buffer );
    require_noerr_string(err, exit, "Error initialising UART connection to PC");

    /* Grab message from PC and pass it over to the controller */
    while ( 1 )
    {
        hci_command_header_t header;
        bt_packet_t*         packet;
        uint32_t             expected_bytes = sizeof( header );

        /* Read HCI header */
        MicoUartRecv( STDIO_UART, (void*)&header, expected_bytes, MICO_NEVER_TIMEOUT );

        /* Allocate dynamic packet */
        bt_packet_pool_dynamic_allocate_packet( &packet, sizeof( header ), header.content_length );

        /* Copy header to packet */
        memcpy( packet->packet_start, &header, sizeof( header ) );

        /* Read the remaining packet */
        if ( header.content_length > 0 )
        {
            expected_bytes = header.content_length;
            MicoUartRecv( STDIO_UART, packet->data_start, expected_bytes, MICO_NEVER_TIMEOUT );

            /* Set the end of the packet */
            packet->data_end += header.content_length;
        }

        /* Send packet to the controller */
        bt_transport_driver_send_packet( packet );
    }

exit:
    return err;
}


static OSStatus bt_mfgtest_transport_driver_event_handler( bt_transport_driver_event_t event )
{
    if ( event == TRANSPORT_DRIVER_INCOMING_PACKET_READY )
    {
        return bt_transport_thread_notify_packet_received();
    }

    return kUnknownErr;
}

static OSStatus bt_mfgtest_transport_thread_received_packet_handler( bt_packet_t* packet )
{
    /* Pass HCI event packet to STDIO UART */
    MicoUartSend( STDIO_UART, (const void*)packet->packet_start, packet->data_end - packet->packet_start );

    /* Release packet */
    return bt_packet_pool_free_packet( packet );
}
