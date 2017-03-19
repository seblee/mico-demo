/**
  ******************************************************************************
  * @file    UdpBoardCast.c 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   This file create a TCP listener thread, accept every TCP client
  *          connection and create thread for them.
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

#include "MICO.h"
#include "MICOAppDefine.h"
#include "SppProtocol.h"
#include "SocketUtils.h"
#include "protocol.h"

#define boardcast_log(M, ...) custom_log("UDP BOARDCAST", M, ##__VA_ARGS__)
#define boardcast_log_trace() custom_log_trace("UDP BOARDCAST")

static app_context_t *Context;
static mico_semaphore_t close_udp_boardcast_sem[2] = {NULL};

static bool is_udp_boardcast_1_established = false;
static bool is_udp_boardcast_2_established = false;

void boardcast_thread(mico_thread_arg_t arg)
{
  OSStatus err;
  struct _socket_port_t socket_port;
  socket_port = *(socket_port_t *)arg;
  int udp_fd_rx = -1, udp_fd_tx = -1;
  int len, sent_len;
  uint8_t *inDataBuffer = NULL;
  struct sockaddr_in addr;
  fd_set readfds;
  fd_set writeSet;
  struct timeval t;
  cmd_data_send_t *cmd_data_send;
  int eventFd = -1;
  int close_udp_boardcast_fd = -1;
  mico_queue_t queue;
  socket_msg_t *msg;
  
  mico_rtos_init_semaphore(&close_udp_boardcast_sem[socket_port.source - 1], 1);
  
  inDataBuffer = malloc(wlanBufferLen);
  require_action(inDataBuffer, exit, err = kNoMemoryErr);
  
  if( socket_port.localPort != 0 ){
    udp_fd_rx = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    require_action( IsValidSocket( udp_fd_rx ), exit, err = kNoResourcesErr );
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(socket_port.localPort);
    
    err = bind(udp_fd_rx, (struct sockaddr *)&addr, sizeof(addr));
    require_noerr(err, exit);
    event_printf("+EVENT=UDP_BOARDCAST,RX_UP,%d\r\n", udp_fd_rx);
  }
  
  if( socket_port.remotePort != 0 ){
    udp_fd_tx = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    require_action( IsValidSocket( udp_fd_tx ), exit, err = kNoResourcesErr );
    cmd_socket_add(udp_fd_tx);
    event_printf("+EVENT=UDP_BOARDCAST,TX_UP,%d\r\n", udp_fd_tx);
  }
  
  err = socket_queue_create(Context, &queue);
  require_noerr( err, exit );
  eventFd = mico_create_event_fd(queue);
  if (eventFd < 0) {
    boardcast_log("create event fd error");
    goto exit;
  }
  
  close_udp_boardcast_fd = mico_create_event_fd(close_udp_boardcast_sem[socket_port.source - 1]);
  if( close_udp_boardcast_fd < 0 ){
    boardcast_log("create close fd err");
    socket_queue_delete(Context, &close_udp_boardcast_sem[socket_port.source - 1]);
    goto exit;
  }

  boardcast_log("UDP boardcast %d established at local port:%ld,rx socket:%d, remote port:%ld, tx socet:%d", socket_port.source,
                socket_port.localPort, udp_fd_rx, socket_port.remotePort, udp_fd_tx);
  
  while(1)
  {
    FD_ZERO(&readfds);
    FD_SET(eventFd, &readfds); 
    FD_SET(close_udp_boardcast_fd, &readfds);
    if( socket_port.localPort != 0 )
      FD_SET(udp_fd_rx, &readfds);
    
    require_action( select(eventFd + close_udp_boardcast_fd + udp_fd_rx + 2, &readfds, NULL, NULL, NULL) >= 0, exit, err = kConnectionErr );
    
    if( socket_port.remotePort != 0 ){
      if (FD_ISSET( eventFd, &readfds )) { // have data and can write
          FD_ZERO(&writeSet );
          FD_SET(udp_fd_tx, &writeSet );
          t.tv_usec = 100*1000; // max wait 100ms.
          select(Max(udp_fd_tx, eventFd) + 1, NULL, &writeSet, NULL, &t);
          if((FD_ISSET( udp_fd_tx, &writeSet )) && (kNoErr == mico_rtos_pop_from_queue( &queue, &msg, 0))) {
              addr.sin_family = AF_INET;
              addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
              addr.sin_port = htons(socket_port.remotePort);
            if(Context->appStatus.at_mode == DATA_MODE){
              sendto(udp_fd_tx, msg->data, msg->len, 0, (const struct sockaddr *)&addr, sizeof(addr));
            }else{
              cmd_data_send = (cmd_data_send_t *) msg->data;
              if( cmd_data_send->cmd_send_type == CMD_DATA ){
                if( cmd_data_send->socket !=0 ){
                  if( cmd_data_send->socket == udp_fd_tx ){
                    sent_len = sendto(udp_fd_tx, cmd_data_send->buf, cmd_data_send->lenth, 0, (const struct sockaddr *)&addr, sizeof(addr));
                    if( sent_len > 0){
                      at_printf("+OK\r\n");
                    }else{
                      at_printf("+ERR=%d\r\n", INVALID_OPERATION);
                    }
                  }
                }else{
                  sendto(udp_fd_tx, cmd_data_send->buf, cmd_data_send->lenth, 0, (const struct sockaddr *)&addr, sizeof(addr));
                }
              }
            }
            
            socket_msg_free(msg);
          }
       }
    }
    
    if( socket_port.localPort != 0 ){
      if(FD_ISSET( udp_fd_rx, &readfds ))
      {
        len = recv(udp_fd_rx, inDataBuffer, wlanBufferLen, 0);
        event_printf("+EVENT=SOCKET,%d,%d,", udp_fd_rx, len);
        sppWlanCommandProcess(inDataBuffer, &len, udp_fd_rx, Context);
        event_printf("\r\n");
      }
    }
    
    /*check delete thread*/
    if( FD_ISSET(close_udp_boardcast_fd, &readfds) ){
      mico_rtos_get_semaphore(&close_udp_boardcast_sem[socket_port.source - 1], 0);
      goto exit;
    }
    
  }
exit:
  if(inDataBuffer) free(inDataBuffer);
  if (eventFd >= 0) {
      mico_delete_event_fd(eventFd);
      socket_queue_delete(Context, &queue);
  }
  if( close_udp_boardcast_fd >= 0 ){
    mico_delete_event_fd(close_udp_boardcast_fd);
  }
  if (udp_fd_rx != -1) {
    event_printf("+EVENT=UDP_BOARDCAST,RX_DOWN,%d\r\n", udp_fd_rx);
    SocketClose(&udp_fd_rx);
  }
  if (udp_fd_tx != -1) {
    cmd_socket_remove(udp_fd_tx);
    event_printf("+EVENT=UDP_BOARDCAST,TX_DOWN,%d\r\n", udp_fd_tx);
    SocketClose(&udp_fd_tx);
  }
  if( close_udp_boardcast_sem[socket_port.source - 1] != NULL ){
    mico_rtos_deinit_semaphore( &close_udp_boardcast_sem[socket_port.source - 1] );
    close_udp_boardcast_sem[socket_port.source - 1] = NULL;
  }
  
  boardcast_log("Exit: udp broadcast %d exit with err = %d", socket_port.source, err);
  mico_rtos_delete_thread(NULL);
}

OSStatus start_udp_boardcast(app_context_t * const app_context, int event)
{
  OSStatus err = kGeneralErr;
  Context = app_context;
  
  if( event == 1 ){
    if( is_udp_boardcast_1_established )
      return kGeneralErr;
    
    is_udp_boardcast_1_established = true;
    close_udp_boardcast_sem[0] = NULL;
    get_protocol_socket_port()->source = 1;
    get_protocol_socket_port()->localPort = Context->appConfig->at_config.conn1_config.localPort;
    get_protocol_socket_port()->remotePort = Context->appConfig->at_config.conn1_config.remotePort;
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "boardcast1", boardcast_thread, STACK_SIZE_BOARDCAST_THREAD, (mico_thread_arg_t)get_protocol_socket_port() );
  }else{
    if( is_udp_boardcast_2_established )
      return kGeneralErr;
    
    is_udp_boardcast_2_established = true;    
    close_udp_boardcast_sem[1] = NULL;
    get_protocol_socket_port()->source = 2;
    get_protocol_socket_port()->localPort = Context->appConfig->at_config.conn2_config.localPort;
    get_protocol_socket_port()->remotePort = Context->appConfig->at_config.conn2_config.remotePort;      
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "boardcast2", boardcast_thread, STACK_SIZE_BOARDCAST_THREAD, (mico_thread_arg_t)get_protocol_socket_port() );
  }
  
  return err;
}

OSStatus stop_udp_boardcast(app_context_t * const app_context, int event)
{
  OSStatus err = kGeneralErr;
  
  if( event == 1 ){
    if( !is_udp_boardcast_1_established )
      return kNoErr;
      
      is_udp_boardcast_1_established = false;
      err = mico_rtos_set_semaphore(&close_udp_boardcast_sem[0]);
  }else{
    if( !is_udp_boardcast_2_established )
      return kNoErr;
    
      is_udp_boardcast_2_established = false;
     err = mico_rtos_set_semaphore(&close_udp_boardcast_sem[1]);
  }
  
  return err;
}
