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
#include "bt_hci.h"
#include "bt_hci_interface.h"
#include "bt_packet_internal.h"

/******************************************************
 *                      Macros
 ******************************************************/

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

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

OSStatus bt_mfgtest_transport_driver_bus_read_handler( bt_packet_t** packet )
{
    hci_event_header_t header;
    OSStatus     result;

    /* Get the packet type */
    result = bt_mfg_bus_receive( (uint8_t*)&header, sizeof( header ), MICO_NEVER_TIMEOUT );
    require_noerr(result, exit);

    /* Allocate buffer for the incoming packet. Always use dynamic packet pool for the  */
    result = bt_packet_pool_dynamic_allocate_packet( packet, sizeof( header ), header.content_length );
    require_noerr(result, exit);

    /* Copy header to the packet */
    memcpy( ( *packet )->packet_start, &header, sizeof( header ) );
    ( *packet )->data_end  = ( *packet )->data_start + header.content_length;

    if ( header.content_length > 0 )
    {
        /* Receive the remainder of the packet */
        result = bt_mfg_bus_receive( (uint8_t*)( ( *packet )->data_start ), (uint32_t)( ( *packet )->data_end - ( *packet )->data_start ), MICO_NEVER_TIMEOUT );
        /* Failed to receive the remainder of the data. Release packet and return error */
        require_noerr_action(result, exit, bt_packet_pool_free_packet( *packet ));
    }

    if ( header.packet_type == 0xff )
    {
        /* Unknown packet type. Release packet and return error */
        bt_packet_pool_free_packet( *packet );
        return MICO_BT_UNKNOWN_PACKET;
    }

    /* Packet successfully received. Pass up to the transport thread and return success */
exit:
    return result;
}
