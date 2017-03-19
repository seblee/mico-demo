/**
  ******************************************************************************
  * @file    UartRecv.c 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   This file create a UART recv thread.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, MXCHIP Inc. SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2014 MXCHIP Inc.</center></h2>
  ******************************************************************************
  */ 
 
#include "mico.h"
#include "SppProtocol.h"
#include "at_cmd.h"

#define uart_recv_log(M, ...) custom_log("UART RECV", M, ##__VA_ARGS__)
#define uart_recv_log_trace() custom_log_trace("UART RECV")

static size_t _uart_get_one_packet(uint8_t* inBuf, int inBufLen, uint32_t timeout);

/**************************timer function**********************************/
static void AtCmdWillStart( void );
static void AtCmdWillStop( void );

static mico_timer_t _at_cmd_timer;
static bool is_rec_ok;

static void _at_cmd_Timeout_handler( void* arg )
{
  (void)(arg);
  is_rec_ok = false;
  AtCmdWillStop( );
}

static void AtCmdWillStart( void )
{
    /*Led trigger*/
  mico_init_timer(&_at_cmd_timer, AT_CMD_TIME_OUT, _at_cmd_Timeout_handler, NULL);
  mico_start_timer(&_at_cmd_timer);
  return;
}

static void AtCmdWillStop( void )
{
  mico_stop_timer(&_at_cmd_timer);
  mico_deinit_timer(&_at_cmd_timer);
  
  return;
}

/**************************timer function end******************************/

void get_uart_recv_len_time(app_context_t * const app_context, uint32_t *recv_len, uint32_t *recv_time)
{
  if(app_context->appConfig->at_config.autoFormatEnable == true){
    *recv_len = app_context->appConfig->at_config.uart_auto_format.auto_format_len;
    *recv_time = app_context->appConfig->at_config.uart_auto_format.auto_format_time;
  }else{
    *recv_len = UART_ONE_PACKAGE_LENGTH;
    *recv_time = 500;
  }
}

void uartRecv_thread(mico_thread_arg_t inContext)
{
  uart_recv_log_trace();
  app_context_t *app_context = (app_context_t *)inContext;
  int recvlen;
  uint8_t *inDataBuffer;
  bool is_at_mode = false;
  uint32_t uart_one_package_length, uart_one_package_timeout;
  
  
  inDataBuffer = malloc(UART_ONE_PACKAGE_LENGTH);
  require(inDataBuffer, exit);
  
  while(1) {
    get_uart_recv_len_time(app_context, &uart_one_package_length, &uart_one_package_timeout);
    recvlen = _uart_get_one_packet(inDataBuffer, uart_one_package_length, uart_one_package_timeout);
    if (recvlen <= 0)
      continue; 
     
    if (is_rec_ok == false) {
      if(recvlen == 3) {
        if(!strncmp((char const *)inDataBuffer, "+++", 3)) {
          is_rec_ok = true;
          at_printf("a");
          AtCmdWillStart( );
          continue;
        }
      }
    }else{
      if(recvlen == 1) {
        switch( *inDataBuffer ){
          case 'a':
            app_context->appStatus.at_mode = AT_MODE;
            is_at_mode = true;
            break;
          case 'b':
            app_context->appStatus.at_mode = AT_NO_MODE;
            is_at_mode = true;
            break;
          default:
            break;
        }
        if( is_at_mode == true ){
          is_rec_ok = false;
          AtCmdWillStop( );
          at_printf("+OK\r\n");
          mico_at_mode( app_context );
          goto exit;
        }
      } else {
        is_rec_ok = false;
      }
    }
    
    sppUartCommandProcess(inDataBuffer, recvlen, app_context);
  }
  
exit:
  if(inDataBuffer) free(inDataBuffer);
  mico_rtos_delete_thread(NULL);
}

static int uart_getchar(uint8_t *inbuf, uint32_t len, uint32_t timeout)
{
  if (MicoUartRecv(UART_FOR_APP, inbuf, len, timeout) == 0)
    return 1;
  else
    return 0;
}

/* Packet format: BB 00 CMD(2B) Status(2B) datalen(2B) data(x) checksum(2B)
* copy to buf, return len = datalen+10
*/
size_t _uart_get_one_packet(uint8_t* inBuf, int inBufLen, uint32_t timeout)
{
  uart_recv_log_trace();
  int datalen = 0;
  
  while (1) {
    if (uart_getchar(&inBuf[datalen], 1, timeout) == 1) {
     
      (datalen)++;
      if (datalen >= inBufLen) {
        return datalen;
      }
    } else {
        return datalen;
    }
  }
}

OSStatus mico_data_mode(void *app_context_c)
{
  is_rec_ok = false;
  return mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "UART Recv", uartRecv_thread, STACK_SIZE_UART_RECV_THREAD, (mico_thread_arg_t)app_context_c );
}

