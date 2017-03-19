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
#include "bt_bus.h"
#include "bt_hci_interface.h"
#include "platform_bluetooth.h"

/******************************************************
 *                      Macros
 ******************************************************/
/* Verify if bt_bus function returns success.
 * Otherwise, returns the error code immediately.
 * Assert in DEBUG build.
 */
#define VERIFY_RETVAL( function_call ) \
do \
{ \
    err = (function_call); \
    require_noerr(err, exit);\
} while ( 0 )

/* Verify if HCI response is expected.
 * Otherwise, returns HCI unexpected response immediately.
 * Assert in DEBUG build.
 */
#define VERIFY_RESPONSE( a, b, size ) \
{ \
    if ( memcmp( (a), (b), (size) ) != 0 ) \
    { \
        check_string( 0!=0, "HCI unexpected response" ); \
        return kUnexpectedErr; \
    } \
}

/******************************************************
 *                    Constants
 ******************************************************/
#define DEFAULT_READ_TIMEOUT (100)
#define BAUDRATE_3MPBS       (3000000)
#define BAUDRATE_115KPBS     (115200)

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
 *               extern Function Declarations
 ******************************************************/
extern OSStatus bluetooth_mico_init_config_uart( const platform_uart_config_t* bt_uart_config );

extern void get_one_command(char * out, int offset);

extern uint32_t get_hcd_content_length();

/******************************************************
 *               Variable Definitions
 ******************************************************/
static const hci_command_header_t const hci_commands[] =
{
    [HCI_CMD_RESET]               = { .packet_type = 0x1, .op_code = HCI_CMD_OPCODE_RESET,               .content_length = 0x0 },
    [HCI_CMD_DOWNLOAD_MINIDRIVER] = { .packet_type = 0x1, .op_code = HCI_CMD_OPCODE_DOWNLOAD_MINIDRIVER, .content_length = 0x0 },
    [HCI_CMD_WRITE_RAM]           = { .packet_type = 0x1, .op_code = HCI_CMD_OPCODE_WRITE_RAM,           .content_length = 0x0 },
    [HCI_CMD_LAUNCH_RAM]          = { .packet_type = 0x1, .op_code = HCI_CMD_OPCODE_LAUNCH_RAM,          .content_length = 0x0 },
    [HCI_CMD_UPDATE_BAUDRATE]     = { .packet_type = 0x1, .op_code = HCI_CMD_OPCODE_UPDATE_BAUDRATE,     .content_length = 0x6 },

};

static const hci_event_extended_header_t const expected_hci_events[] =
{
    [HCI_CMD_RESET]               = { .header = {.packet_type = 0x4, .event_code = 0xE, .content_length = 0x4 }, .total_packets = 0x1, .op_code = HCI_CMD_OPCODE_RESET,               .status = 0x0 },
    [HCI_CMD_DOWNLOAD_MINIDRIVER] = { .header = {.packet_type = 0x4, .event_code = 0xE, .content_length = 0x4 }, .total_packets = 0x1, .op_code = HCI_CMD_OPCODE_DOWNLOAD_MINIDRIVER, .status = 0x0 },
    [HCI_CMD_WRITE_RAM]           = { .header = {.packet_type = 0x4, .event_code = 0xE, .content_length = 0x4 }, .total_packets = 0x1, .op_code = HCI_CMD_OPCODE_WRITE_RAM,           .status = 0x0 },
    [HCI_CMD_LAUNCH_RAM]          = { .header = {.packet_type = 0x4, .event_code = 0xE, .content_length = 0x4 }, .total_packets = 0x1, .op_code = HCI_CMD_OPCODE_LAUNCH_RAM,          .status = 0x0 },
    [HCI_CMD_UPDATE_BAUDRATE]     = { .header = {.packet_type = 0x4, .event_code = 0xE, .content_length = 0x4 }, .total_packets = 0x1, .op_code = HCI_CMD_OPCODE_UPDATE_BAUDRATE,     .status = 0x0 },

};

/******************************************************
 *               Static Function Declarations
 ******************************************************/
static OSStatus bt_host_update_baudrate( uint32_t newBaudRate );
static OSStatus bt_issue_reset ( void );

/******************************************************
 *               Variable Definitions
 ******************************************************/
static platform_uart_config_t bt_uart_config;

/******************************************************
 *               Function Definitions
 ******************************************************/
OSStatus bt_issue_reset ( void )
{
    OSStatus err = kNoErr;
    hci_event_extended_header_t hci_event;
    uint8_t hci_data[ 8 ] = { 0x00 };
    uint8_t hardware_error[ 4 ] = { 0x04, 0x10, 0x01, 0x00 };
    VERIFY_RETVAL( bt_mfg_bus_transmit( (const uint8_t* ) &hci_commands[ HCI_CMD_RESET ], sizeof(hci_command_header_t) ) );

    /* FIXME : This hardware parsing error should be handled by the firmware
     *  and will be eventually removed from this code*/

    /* if any hardware parsing error event just ignore it and read next bytes */
    VERIFY_RETVAL( bt_mfg_bus_receive( (uint8_t* ) hci_data, 4, 1000 ) );

    if ( !memcmp( hardware_error, hci_data, 4 ) )
    {
        check_string( 0!=0 , "hardware parsing error recieved" );
        VERIFY_RETVAL( bt_mfg_bus_receive( (uint8_t* ) &hci_event, sizeof( hci_event ), 1000 ) );
        VERIFY_RESPONSE( &hci_event, &expected_hci_events[ HCI_CMD_RESET ], sizeof( hci_event ) );
    }
    else
    {
        VERIFY_RETVAL( bt_mfg_bus_receive( (uint8_t* ) &hci_data[4], 3, 1000 ) ); /* First reset command requires extra delay between write and read */
        VERIFY_RESPONSE( hci_data, &expected_hci_events[ HCI_CMD_RESET ], sizeof( hci_event ) );
    }

exit:
    return err;
}

static OSStatus bt_host_update_baudrate( uint32_t newBaudRate )
{
    OSStatus err = kNoErr;
    hci_event_extended_header_t hci_event;
    char update_command[ 10 ];

    /* format of transmit bytes for update baud rate command
     * 0x01, 0x18, 0xFC, 0x06, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff
     */
    memcpy( update_command, &hci_commands[ HCI_CMD_UPDATE_BAUDRATE ], 4 );
    update_command[ 4 ] = 0x00; /* Encoded baud rate ; disable : 0 , enable : 1 */
    update_command[ 5 ] = 0x00; /* use Encoded form  ; disable : 0 , enable : 1 */

    /* issue BT hci update baudrate */
    update_command[ 6 ] = ( newBaudRate ) & 0xff;
    update_command[ 7 ] = ( newBaudRate >> 8 ) & 0xff;
    update_command[ 8 ] = ( newBaudRate >> 16 ) & 0xff;
    update_command[ 9 ] = ( newBaudRate >> 24 ) & 0xff;

    VERIFY_RETVAL( bt_mfg_bus_transmit( (const uint8_t* ) update_command, 10 ) );
    VERIFY_RETVAL( bt_mfg_bus_receive( (uint8_t* ) &hci_event, sizeof( hci_event ), 200 ) );
    VERIFY_RESPONSE( &hci_event, &expected_hci_events[ HCI_CMD_UPDATE_BAUDRATE ], sizeof( hci_event ) );
    mico_rtos_delay_milliseconds( 40 );

    /* Update host uart baudrate*/
    bt_uart_config.baud_rate = newBaudRate;

    bluetooth_mico_init_config_uart( &bt_uart_config );

exit:
    return err;
}

OSStatus bt_firmware_download( void )
{
    OSStatus err = kNoErr;
    uint32_t remaining_length = get_hcd_content_length();
    uint32_t firmware_offset = 0;
    hci_event_extended_header_t hci_event;
    uint8_t fast_download = MICO_FALSE;

    memcpy( &bt_uart_config, &mico_bt_uart_config, sizeof( mico_bt_uart_config ) );

    BT_BUS_IS_READY();

    /* Send 'Reset' command */
    VERIFY_RETVAL( bt_mfg_bus_transmit( (const uint8_t* ) &hci_commands[ HCI_CMD_RESET ], sizeof(hci_command_header_t) ) );
    VERIFY_RETVAL( bt_mfg_bus_receive( (uint8_t* ) &hci_event, sizeof( hci_event ), 1000 ) ); /* First reset command requires extra delay between write and read */
    VERIFY_RESPONSE( &hci_event, &expected_hci_events[ HCI_CMD_RESET ], sizeof( hci_event ) );

    /* TODO : Eventually we will be using the patchram_download_baudrate variable from the platform file. */
    /* update BT and host to switch to 3mbps */
    if ( bt_host_update_baudrate( BAUDRATE_3MPBS ) == kNoErr )
    {
        fast_download = MICO_TRUE;
    }

    /* Send hci_download_minidriver command */
    if ( mico_bt_config.patchram_download_mode == PATCHRAM_DOWNLOAD_MODE_MINIDRV_CMD )
    {
        VERIFY_RETVAL( bt_mfg_bus_transmit( (const uint8_t* ) &hci_commands[ HCI_CMD_DOWNLOAD_MINIDRIVER ], sizeof(hci_command_header_t) ) );
        VERIFY_RETVAL( bt_mfg_bus_receive( (uint8_t*) &hci_event, sizeof( hci_event ), DEFAULT_READ_TIMEOUT ) );
        VERIFY_RESPONSE( &hci_event, &expected_hci_events[ HCI_CMD_DOWNLOAD_MINIDRIVER ], sizeof( hci_event ) );
    }

    /* The firmware image (.hcd format) contains a collection of hci_write_ram command + a block of the image,
     * followed by a hci_write_ram image at the end. Parse and send each individual command and wait for the response.
     * This is to ensure the integrity of the firmware image sent to the bluetooth chip.
     */
    while ( remaining_length )
    {
        uint32_t data_length; /* content of data length + 2 bytes of opcode and 1 byte of data length */
        uint8_t residual_data = 0;
        hci_command_opcode_t command_opcode;
        uint8_t temp_data[ 256 + 3 ];

        memset( &hci_event, 0, sizeof( hci_event ) );
        memset( temp_data, 0, sizeof( temp_data ) );

        /* 43438 requires the packet type before each write RAM command */
        temp_data[ 0 ] = HCI_COMMAND_PACKET;
        get_one_command( (char *)&temp_data[ 1 ], firmware_offset );
        data_length = temp_data[ 3 ] + 3;
        //command_opcode = *(hci_command_opcode_t*) &temp_data[ 1 ];
        command_opcode = temp_data[1] + (temp_data[2] << 8);

        /* Send hci_write_ram command. The length of the data immediately follows the command opcode */
        VERIFY_RETVAL( bt_mfg_bus_transmit( (const uint8_t* ) temp_data, data_length + 1 ) );
        VERIFY_RETVAL( bt_mfg_bus_receive( (uint8_t*) &hci_event, sizeof(hci_event), DEFAULT_READ_TIMEOUT ) );

        switch ( command_opcode )
        {
            case HCI_CMD_OPCODE_WRITE_RAM:
                VERIFY_RESPONSE( &hci_event, &expected_hci_events[ HCI_CMD_WRITE_RAM ], sizeof( hci_event ) );

                /* Update remaining length and data pointer */
                firmware_offset += data_length;
                remaining_length -= data_length;
                break;

            case HCI_CMD_OPCODE_LAUNCH_RAM:
                VERIFY_RESPONSE( &hci_event, &expected_hci_events[ HCI_CMD_LAUNCH_RAM ], sizeof( hci_event ) )
                ;

                /* All responses have been read. Now let's flush residual data if any and reset remaining length */
                while ( bt_mfg_bus_receive( &residual_data, sizeof( residual_data ), DEFAULT_READ_TIMEOUT ) == kNoErr )
                {
                }

                remaining_length = 0;

                break;

            default:
                return kUnexpectedErr;
        }
    }

    /* Wait for bluetooth chip to pull its RTS (host's CTS) low. From observation using CRO, it takes the bluetooth chip > 170ms to pull its RTS low after CTS low */
    BT_BUS_WAIT_UNTIL_READY();
    mico_rtos_delay_milliseconds( 200 );

    /*switch back host to 115200 if downloaded the f/w with 3mbps*/
    if(fast_download == MICO_TRUE)
    {
        bt_uart_config.baud_rate = BAUDRATE_115KPBS;
        bluetooth_mico_init_config_uart( &bt_uart_config );
        mico_rtos_delay_milliseconds( 10 );
        //bt_host_update_baudrate( BAUDRATE_3MPBS );
    }

    err = bt_issue_reset( );
exit:
    return err;
}
