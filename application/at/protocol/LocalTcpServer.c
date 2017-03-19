/**
  ******************************************************************************
  * @file    LocalTcpServer.c 
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

#define server_log(M, ...) custom_log("TCP SERVER", M, ##__VA_ARGS__)
#define server_log_trace() custom_log_trace("TCP SERVER")

static void localTcpClient_thread(mico_thread_arg_t inFd);
static app_context_t *Context;

static mico_semaphore_t close_tcp_server_sem[2] = {NULL}; 
uint8_t is_close_server_client[2][MAX_TCP_CLIENT_PER_SERVER] = { 0 };;

static bool is_tcp_server_1_established = false;
static bool is_tcp_server_2_established = false;

void localTcpServer_thread(mico_thread_arg_t arg)
{
  server_log_trace();
  OSStatus err = kUnknownErr;
  struct _socket_port_t socket_port;
  socket_port = *( socket_port_t *)arg;  
  int j;
  struct sockaddr_in addr;
  int sockaddr_t_size;
  fd_set readfds;
  char ip_address[16];
  int optval = 5;
  
  int localTcpListener_fd = -1;
  int close_tcp_server_fd = -1;
  
  mico_rtos_init_semaphore( &close_tcp_server_sem[socket_port.source - 1], 1);
  close_tcp_server_fd = mico_create_event_fd( close_tcp_server_sem[socket_port.source - 1] );  

  /*Establish a TCP server fd that accept the tcp clients connections*/ 
  localTcpListener_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  require_action(IsValidSocket( localTcpListener_fd ), exit, err = kNoResourcesErr );
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(socket_port.localPort);
  err = bind(localTcpListener_fd, (struct sockaddr *)&addr, sizeof(addr));
  require_noerr( err, exit );

  err = listen(localTcpListener_fd, 0);
  require_noerr( err, exit );
  if( socket_port.source == 1 ){
    optval = Context->appConfig->at_config.conn1_config.server_num;
  }else if( socket_port.source == 2 ){
    optval = Context->appConfig->at_config.conn2_config.server_num;
  }
  setsockopt(localTcpListener_fd, IPPROTO_TCP, TCP_MAX_CONN_NUM, &optval, sizeof(optval));
  
  server_log("Server established %d at port: %ld, fd: %d", socket_port.source, socket_port.localPort, localTcpListener_fd);
  
  while(1){
    FD_ZERO(&readfds);
    FD_SET(localTcpListener_fd, &readfds);  
    FD_SET(close_tcp_server_fd, &readfds);  
    
    select(Max( localTcpListener_fd, close_tcp_server_fd )+ 1, &readfds, NULL, NULL, NULL);
    
    /* Check close requests */
    if(FD_ISSET(close_tcp_server_fd, &readfds)){
      mico_rtos_get_semaphore( &close_tcp_server_sem[socket_port.source - 1], 0 );
      goto exit;
    }

    /*Check tcp connection requests */
    if(FD_ISSET(localTcpListener_fd, &readfds)){
      sockaddr_t_size = sizeof(struct sockaddr_in);
      j = accept(localTcpListener_fd, (struct sockaddr *)&addr, (socklen_t *)&sockaddr_t_size);
      if( IsValidFD(j) ){
        strcpy( ip_address, inet_ntoa(addr.sin_addr) );
        server_log("Client %s:%d connected, fd: %d", ip_address, addr.sin_port, j);
        event_printf("+EVENT=TCP_CLIENT,CONNECT,%d\r\n", j);
        socket_port.fd = j;
        if( socket_port.source == 1 ){
          Context->appStatus.connect_state.connect1_server_link_count++;
        }else if( socket_port.source == 2 ){
          Context->appStatus.connect_state.connect2_server_link_count++;
        }else{
          server_log("source err");
        }
        if( socket_port.source == 1 ){
          if( Context->appConfig->at_config.conn1_config.server_num == 1){
            localTcpClient_thread((mico_thread_arg_t)&socket_port);
            SocketClose(&j);
          }else{
            if(kNoErr != mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Local Clients", localTcpClient_thread, STACK_SIZE_LOCAL_TCP_CLIENT_THREAD, (mico_thread_arg_t)&socket_port) )
              SocketClose(&j);
          }
        }else if( socket_port.source == 2 ){
          if( Context->appConfig->at_config.conn2_config.server_num == 1){
            localTcpClient_thread((mico_thread_arg_t)&socket_port);
            SocketClose(&j);
          }else{
            if(kNoErr != mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Local Clients", localTcpClient_thread, STACK_SIZE_LOCAL_TCP_CLIENT_THREAD, (mico_thread_arg_t)&socket_port) )
              SocketClose(&j);
          }
        }
      }
    }
  }

exit:
  if( close_tcp_server_sem[socket_port.source - 1] != NULL ){
    mico_delete_event_fd(close_tcp_server_fd);
    mico_rtos_deinit_semaphore(&close_tcp_server_sem[socket_port.source - 1]);
    close_tcp_server_sem[socket_port.source - 1] = NULL;    
  }
  SocketClose(&localTcpListener_fd);
  server_log("Exit: Local server %d, with err = %d", socket_port.source, err);
  mico_rtos_delete_thread(NULL);
  return;
}

void localTcpClient_thread(mico_thread_arg_t arg)
{
  OSStatus err = kNoErr;
  struct _socket_port_t socket_port;
  socket_port = *( socket_port_t *)arg; 
  int clientFd = socket_port.fd;
  uint8_t *inDataBuffer = NULL;
  int len;
  fd_set readfds;
  fd_set writeSet;
  struct timeval t;
  int eventFd = -1;
  mico_queue_t queue;
  socket_msg_t *msg;
  int sent_len, errno;
  cmd_data_send_t *cmd_data_send;
  int close_sem_index;


  for( close_sem_index = 0; close_sem_index < MAX_TCP_CLIENT_PER_SERVER; close_sem_index++ ){
    if( is_close_server_client[socket_port.source - 1][close_sem_index] == 0 )
      break;
  }
  
  is_close_server_client[socket_port.source - 1][close_sem_index] = 1;
  
  if( close_sem_index == MAX_TCP_CLIENT_PER_SERVER ){
    mico_rtos_delete_thread(NULL);
    return;
  }
  
  cmd_socket_add(clientFd);
  server_log("server client index %d", close_sem_index);  
  
  inDataBuffer = malloc(wlanBufferLen);
  require_action(inDataBuffer, exit, err = kNoMemoryErr);
  
  err = socket_queue_create(Context, &queue);
  require_noerr( err, exit );
  eventFd = mico_create_event_fd(queue);
  if (eventFd < 0) {
    server_log("create event fd error");
    goto exit;
  }

  t.tv_sec = 10;
  t.tv_usec = 0;
  
  if( socket_port.source == 1 ){
   if( Context->appConfig->at_config.conn1_config.server_num == 1){
      t.tv_sec =  Context->appConfig->at_config.conn1_config.remotePort;
   }
  }else if( socket_port.source == 2 ){
    if( Context->appConfig->at_config.conn1_config.server_num == 1){
      t.tv_sec =  Context->appConfig->at_config.conn2_config.remotePort;
    }
  }
  
  while(1){

    FD_ZERO(&readfds);
    FD_SET(clientFd, &readfds); 
    FD_SET(eventFd, &readfds); 
    
    if( socket_port.source == 1 ){
      if( Context->appConfig->at_config.conn1_config.server_num == 1){
        if( select(Max(clientFd, eventFd) + 1, &readfds, NULL, NULL, &t) <= 0)
          goto exit;
      }else{
        select(Max(clientFd, eventFd) + 1, &readfds, NULL, NULL, &t);
      }
    }else if( socket_port.source == 2 ){
      if( Context->appConfig->at_config.conn1_config.server_num == 1){
        if( select(Max(clientFd, eventFd) + 1, &readfds, NULL, NULL, &t) <= 0)
          goto exit;
      }else{
        select(Max(clientFd, eventFd) + 1, &readfds, NULL, NULL, &t);
      }      
    }
    
    
    
    /* send UART data */
    if (FD_ISSET( eventFd, &readfds )) { // have data and can write
      FD_ZERO(&writeSet );
      FD_SET(clientFd, &writeSet );
      t.tv_usec = 100*1000; // max wait 100ms.
      select(clientFd + 1, NULL, &writeSet, NULL, &t);
      if((FD_ISSET( clientFd, &writeSet )) && (kNoErr == mico_rtos_pop_from_queue( &queue, &msg, 0))) {
        if(Context->appStatus.at_mode == DATA_MODE){
          sent_len = write(clientFd, msg->data, msg->len);
        }else{
          cmd_data_send = (cmd_data_send_t *) msg->data;
          if( cmd_data_send->cmd_send_type == CMD_DATA ){
            if( cmd_data_send->socket !=0 ){
              if( cmd_data_send->socket == clientFd ){
                sent_len = write(clientFd, cmd_data_send->buf, cmd_data_send->lenth);
                if( sent_len > 0){
                  at_printf("+OK\r\n");
                }else{
                  at_printf("+ERR=%d\r\n", INVALID_OPERATION);
                }
              }else{
                sent_len = 1;
              }
            }else{
              sent_len = write(clientFd, cmd_data_send->buf, cmd_data_send->lenth);
            }
          }else{
            sent_len = 1;
          }
        }
        
        if (sent_len <= 0) {
          len = sizeof(errno);
          getsockopt(clientFd, SOL_SOCKET, SO_ERROR, &errno, (socklen_t *)&len);
          socket_msg_free(msg);
              server_log("write error, fd: %d, errno %d", clientFd, errno );
              if (errno != ENOMEM) {
                  goto exit;
              }
           } else {
                  socket_msg_free(msg);
              }
           }
        }

    /*Read data from tcp clients and process these data using HA protocol */ 
    if (FD_ISSET(clientFd, &readfds)) {
      len = recv(clientFd, inDataBuffer, wlanBufferLen, 0);
      require_action_quiet(len>0, exit, err = kConnectionErr);
      
      event_printf("+EVENT=SOCKET,%d,%d,", clientFd, len);
      sppWlanCommandProcess(inDataBuffer, &len, clientFd, Context);
      event_printf("\r\n");
    }
    
    /* Check close requests */
    if( is_close_server_client[socket_port.source - 1][close_sem_index] == 2 ){
      err = kConnectionErr;
      goto exit;
    }
  }

exit:
    len = sizeof(errno);
    getsockopt(clientFd, SOL_SOCKET, SO_ERROR, &errno, (socklen_t *)&len);
    cmd_socket_remove(clientFd);
    server_log("Exit: Client exit with err = %d, socket errno %d, index %d", err, errno, close_sem_index);
    event_printf("+EVENT=TCP_CLIENT,DISCONNECT,%d\r\n", clientFd);
    if( socket_port.source == 1 ){
      Context->appStatus.connect_state.connect1_server_link_count--;
    }else if( socket_port.source == 2 ){
      Context->appStatus.connect_state.connect2_server_link_count--;
    }else{
      server_log("source err");
    }
    if (eventFd >= 0) {
      mico_delete_event_fd(eventFd);
      socket_queue_delete(Context, &queue);
    }
    if( is_close_server_client[socket_port.source - 1][close_sem_index] != 0 )
      is_close_server_client[socket_port.source - 1][close_sem_index] = 0;
    
    SocketClose(&clientFd);
    if(inDataBuffer) free(inDataBuffer);
    
    if( socket_port.source == 1 ){
      if( Context->appConfig->at_config.conn1_config.server_num == 1){
          return;
      }else{
        mico_rtos_delete_thread(NULL);
      }
    }else if( socket_port.source == 2 ){
      if( Context->appConfig->at_config.conn2_config.server_num == 1){
          return;
      }else{
        mico_rtos_delete_thread(NULL);
      }  
    }
    return;
}

OSStatus start_tcp_server(app_context_t * const app_context, int event)
{
  OSStatus err = kGeneralErr;
  Context = app_context;
  int i = 0;
  
  if( event == 1 ){
    if( is_tcp_server_1_established )
      return kGeneralErr;
    
    is_tcp_server_1_established = true;
    
    close_tcp_server_sem[0] = NULL;
    for( ; i < MAX_TCP_CLIENT_PER_SERVER; i++ )
      is_close_server_client[0][i] = 0;
    
    get_protocol_socket_port()->source = 1;
    get_protocol_socket_port()->localPort = Context->appConfig->at_config.conn1_config.localPort;
    
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Local Server1", localTcpServer_thread, STACK_SIZE_LOCAL_TCP_SERVER_THREAD, (mico_thread_arg_t)get_protocol_socket_port() );
  }else{
    if( is_tcp_server_2_established )
      return kGeneralErr;
    
    is_tcp_server_2_established = true;  
    
    close_tcp_server_sem[1] = NULL;
    for( ; i < MAX_TCP_CLIENT_PER_SERVER; i++ )
      is_close_server_client[1][i] = 0;
    
    get_protocol_socket_port()->source = 2;
    get_protocol_socket_port()->localPort = Context->appConfig->at_config.conn2_config.localPort;
    
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Local Server2", localTcpServer_thread, STACK_SIZE_LOCAL_TCP_SERVER_THREAD, (mico_thread_arg_t)get_protocol_socket_port() );
  }
  
  return err;
}

OSStatus stop_tcp_server(app_context_t * const app_context, int event)
{
  OSStatus err = kNoErr;
  int i = 0;
  
  if( event == 1 ){
    if( !is_tcp_server_1_established )
      return kNoErr;
    
    is_tcp_server_1_established = false;
    
    for( ; i < MAX_TCP_CLIENT_PER_SERVER; i++ )
      is_close_server_client[0][i] = 2;
    
    mico_thread_msleep(50);
    
    if( close_tcp_server_sem[0] != NULL )
      err = mico_rtos_set_semaphore(&close_tcp_server_sem[0]);
  }else{
    if( !is_tcp_server_2_established )
      return kNoErr;
    
    is_tcp_server_2_established = false;
    for( ; i < MAX_TCP_CLIENT_PER_SERVER; i++ )
      is_close_server_client[1][i] = 2;
    
    mico_thread_msleep(50);
    
    if( close_tcp_server_sem[1] != NULL )
      err = mico_rtos_set_semaphore(&close_tcp_server_sem[1]);
  }
  
  return err;
}
