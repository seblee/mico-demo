/**
  ******************************************************************************
  * @file    RemoteTcpClient.c 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   Create a TCP client thread, and connect to a remote server.
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

#define client_log(M, ...) custom_log("TCP client", M, ##__VA_ARGS__)
#define client_log_trace() custom_log_trace("TCP client")

static app_context_t *Context;
static bool _wifiConnected = false;
static mico_semaphore_t  _wifiConnected_sem[2] = {NULL}, close_tcp_client_sem[2] = {NULL};

static bool is_tcp_client_1_established = false;
static bool is_tcp_client_2_established = false;
static bool is_egisist_wifi_status_notify = false;

void clientNotify_WifiStatusHandler(int event, void *arg)
{
  client_log_trace();
  (void)arg;
  switch (event) {
  case NOTIFY_STATION_UP:
    _wifiConnected = true;
    if( _wifiConnected_sem[0] != NULL ){
      mico_rtos_set_semaphore(&_wifiConnected_sem[0]);
    }
    if( _wifiConnected_sem[1] != NULL ){
     mico_rtos_set_semaphore(&_wifiConnected_sem[1]);
    }
    break;
  case NOTIFY_STATION_DOWN:
    _wifiConnected = false;
    break;
  case NOTIFY_AP_UP:
    _wifiConnected = true;
    if( _wifiConnected_sem[0] != NULL ){
      mico_rtos_set_semaphore(&_wifiConnected_sem[0]);
    }
    if( _wifiConnected_sem[1] != NULL ){
      mico_rtos_set_semaphore(&_wifiConnected_sem[1]);
    }
    break;
  case NOTIFY_AP_DOWN:
    _wifiConnected = false;
    break;
  default:
    break;
  }
  return;
}

void remoteTcpClient_thread(mico_thread_arg_t arg)
{
  client_log_trace();
  OSStatus err = kUnknownErr;
  struct _socket_port_t socket_port;
  socket_port = *( socket_port_t *)arg;
  int len;
  struct sockaddr_in addr;
  fd_set readfds;
  fd_set writeSet;
  char ipstr[16];
  struct timeval t;
  int remoteTcpClient_fd = -1;
  uint8_t *inDataBuffer = NULL;
  int eventFd = -1;
  int close_tcp_client_fd = -1;
  mico_queue_t queue;
  socket_msg_t *msg;
  LinkStatusTypeDef wifi_link;
  int sent_len, errno;
  cmd_data_send_t *cmd_data_send;
  int opt = 0;
  struct hostent* hostent_content = NULL;
  char **pptr = NULL;
  struct in_addr in_addr;
  
  mico_rtos_init_semaphore(&_wifiConnected_sem[socket_port.source-1], 1);
  mico_rtos_init_semaphore(&close_tcp_client_sem[socket_port.source-1], 1);
  
  inDataBuffer = malloc(wlanBufferLen);
  require_action(inDataBuffer, exit, err = kNoMemoryErr);
  
  err = micoWlanGetLinkStatus( &wifi_link );
  require_noerr( err, exit );
  
  if( wifi_link.is_connected == true )
    _wifiConnected = true;
  
  if( Context->appStatus.wifi_open.ap_open == true ){
      _wifiConnected = true;
  }

  client_log("tcp client %d, connect to %s, at port %ld", socket_port.source, socket_port.remoteServerDomain, socket_port.remotePort);
  
  while(1) {
    if(remoteTcpClient_fd == -1 ) {
      if(_wifiConnected == false){
        require_action_quiet(mico_rtos_get_semaphore(&_wifiConnected_sem[socket_port.source-1], 200000) == kNoErr, Continue, err = kTimeoutErr);
      }
      hostent_content = gethostbyname( (char *)socket_port.remoteServerDomain );
      require_action_quiet( hostent_content != NULL, ReConnWithDelay, err = kNotFoundErr);
      pptr=hostent_content->h_addr_list;
      in_addr.s_addr = *(uint32_t *)(*pptr);
      strcpy( ipstr, inet_ntoa(in_addr));
      remoteTcpClient_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = inet_addr(ipstr);
      addr.sin_port = htons(socket_port.remotePort);
      
      // set keepalive
      opt = 1;
      setsockopt(remoteTcpClient_fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&opt, sizeof(opt)); // 开启socket的Keepalive功能
      opt = 10;
      setsockopt(remoteTcpClient_fd, IPPROTO_TCP, TCP_KEEPIDLE, (void *)&opt, sizeof(opt)); // TCP IDLE 10秒以后开始发送第一个Keepalive包
      opt = 5;
      setsockopt(remoteTcpClient_fd, IPPROTO_TCP, TCP_KEEPINTVL, (void *)&opt, sizeof(opt)); // TCP后面的Keepalive的间隔时间是10秒。
      opt = 3;
      setsockopt(remoteTcpClient_fd, IPPROTO_TCP, TCP_KEEPCNT, (void *)&opt, sizeof(opt)); // Keepalive 数量为3次
      //client_log("set tcp keepalive: idle=%d, interval=%d, cnt=%d.", 10, 10, 3);
      
      err = connect(remoteTcpClient_fd, (struct sockaddr *)&addr, sizeof(addr));
      require_noerr_quiet(err, ReConnWithDelay);
      cmd_socket_add(remoteTcpClient_fd);
      client_log("Remote server %d connected at port: %ld, fd: %d",  socket_port.source, socket_port.remotePort, remoteTcpClient_fd);
      event_printf("+EVENT=TCP_SERVER,CONNECT,%d\r\n", remoteTcpClient_fd);
      if( socket_port.source == 1 ){
        Context->appStatus.connect_state.connect1_client_link = 1;
      }else if( socket_port.source == 2 ){
        Context->appStatus.connect_state.connect2_client_link = 1;
      }else {
        client_log("source err");
      }
      err = socket_queue_create(Context, &queue);
      require_noerr( err, exit );
      eventFd = mico_create_event_fd(queue);
      if (eventFd < 0) {
        client_log("create event fd error");
        socket_queue_delete(Context, &queue);
        goto ReConnWithDelay;
      }
      close_tcp_client_fd = mico_create_event_fd(close_tcp_client_sem[socket_port.source-1]);
      if( close_tcp_client_fd < 0 ){
        client_log("create close fd err");
        socket_queue_delete(Context, &close_tcp_client_sem[socket_port.source-1]);
        goto ReConnWithDelay;
      }
    }else{
      FD_ZERO(&readfds);
      FD_SET(remoteTcpClient_fd, &readfds);
      FD_SET(eventFd, &readfds); 
      FD_SET(close_tcp_client_fd, &readfds);
      t.tv_sec = 4;
      t.tv_usec = 0;
      select(remoteTcpClient_fd + eventFd + close_tcp_client_fd + 1, &readfds, NULL, NULL, &t);
      /* send UART data */
      if (FD_ISSET( eventFd, &readfds )) {// have data 
        FD_ZERO(&writeSet );
        FD_SET(remoteTcpClient_fd, &writeSet );
        t.tv_usec = 100*1000; // max wait 100ms.
        select(remoteTcpClient_fd + 1, NULL, &writeSet, NULL, &t);
        if ((FD_ISSET(remoteTcpClient_fd, &writeSet )) && (kNoErr == mico_rtos_pop_from_queue( &queue, &msg, 0))) {
          if(Context->appStatus.at_mode == DATA_MODE){
            sent_len = write(remoteTcpClient_fd, msg->data, msg->len);
          }else{
            cmd_data_send = (cmd_data_send_t *) msg->data;
            if( cmd_data_send->cmd_send_type == CMD_DATA ){
              if( cmd_data_send->socket !=0 ){
                if( cmd_data_send->socket == remoteTcpClient_fd ){
                  sent_len = write(remoteTcpClient_fd, cmd_data_send->buf, cmd_data_send->lenth);
                  if( sent_len > 0){
                    at_printf("+OK\r\n");
                  }else{
                    at_printf("+ERR=%d\r\n", INVALID_OPERATION);
                  }
                }else{
                  sent_len = 1;
                }
              }else{
                sent_len = write(remoteTcpClient_fd, cmd_data_send->buf, cmd_data_send->lenth);
              }
            }else{
              sent_len = 1;
            }
          }
          
          if (sent_len <= 0) {
            len = sizeof(errno);
            getsockopt(remoteTcpClient_fd, SOL_SOCKET, SO_ERROR, &errno, (socklen_t *)&len);
            
            socket_msg_free(msg);
            if (errno != ENOMEM) {
              client_log("write error, fd: %d, errno %d", remoteTcpClient_fd,errno );
              goto ReConnWithDelay;
            }
          } else {
            socket_msg_free(msg);
          }
        }
      }
      /*recv wlan data using remote client fd*/
      if (FD_ISSET(remoteTcpClient_fd, &readfds)) {
        len = recv(remoteTcpClient_fd, inDataBuffer, wlanBufferLen, 0);
        if(len <= 0) {
          cmd_socket_remove(remoteTcpClient_fd);
          client_log("Remote client %d closed, fd: %d", socket_port.source, remoteTcpClient_fd);
          event_printf("+EVENT=TCP_SERVER,DISCONNECT,%d\r\n", remoteTcpClient_fd);
          if( socket_port.source == 1 ){
            Context->appStatus.connect_state.connect1_client_link = 0;
          }else if( socket_port.source == 2 ){
            Context->appStatus.connect_state.connect2_client_link = 0;
          }else {
            client_log("source err");
          }
          goto ReConnWithDelay;
        }
        event_printf("+EVENT=SOCKET,%d,%d,", remoteTcpClient_fd, len);
        sppWlanCommandProcess(inDataBuffer, &len, remoteTcpClient_fd, Context);
        event_printf("\r\n");
      }
      /*check delete thread*/
      if( FD_ISSET(close_tcp_client_fd, &readfds) ){
        mico_rtos_get_semaphore(&close_tcp_client_sem[socket_port.source-1], 0);
        if( remoteTcpClient_fd != -1 ){
          cmd_socket_remove(remoteTcpClient_fd);
          event_printf("+EVENT=TCP_SERVER,DISCONNECT,%d\r\n", remoteTcpClient_fd);
        }
        if( socket_port.source == 1 ){
          Context->appStatus.connect_state.connect1_client_link = 0;
        }else if( socket_port.source == 2 ){
          Context->appStatus.connect_state.connect2_client_link = 0;
        }else {
          client_log("source err");
        }        
        goto exit;
      }

    Continue:    
      continue;
      
    ReConnWithDelay:
        if( eventFd >= 0 ){
          mico_delete_event_fd(eventFd);
          socket_queue_delete(Context, &queue);
          eventFd = -1;
        }
        if( close_tcp_client_fd >= 0 ){
          mico_delete_event_fd(close_tcp_client_fd);
          close_tcp_client_fd = -1;
        }
        if( remoteTcpClient_fd != -1 ){
          SocketClose(&remoteTcpClient_fd);
          remoteTcpClient_fd = -1;
        }
        mico_thread_sleep(1);
    }
  }
    
exit:
  if(inDataBuffer) free(inDataBuffer);
  if( eventFd >= 0 ){
    mico_delete_event_fd(eventFd);
    socket_queue_delete(Context, &queue);
  }
  if( close_tcp_client_fd >= 0 ){
    mico_delete_event_fd(close_tcp_client_fd);
  }
  if( remoteTcpClient_fd != -1 ){
    cmd_socket_remove(remoteTcpClient_fd);
    SocketClose(&remoteTcpClient_fd);
  }
  if( _wifiConnected_sem[socket_port.source-1] != NULL ){
      mico_rtos_deinit_semaphore( &_wifiConnected_sem[socket_port.source-1] );
      _wifiConnected_sem[socket_port.source-1] = NULL;
  }
  if( close_tcp_client_sem[socket_port.source-1] != NULL ){
    mico_rtos_deinit_semaphore( &close_tcp_client_sem[socket_port.source-1] );
    close_tcp_client_sem[socket_port.source-1] = NULL;
  }
  client_log("Exit: Remote TCP client %d exit with err = %d", socket_port.source, err);
  mico_rtos_delete_thread(NULL);
  return;
}

OSStatus start_tcp_client(app_context_t * const app_context, int event)
{
  OSStatus err = kGeneralErr;
  Context = app_context;
  
  if( is_egisist_wifi_status_notify == false ){
    is_egisist_wifi_status_notify = true;
    mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *)clientNotify_WifiStatusHandler, NULL );
  }
  if( event == 1 ){
    if( is_tcp_client_1_established )
      return kGeneralErr;
    
    is_tcp_client_1_established = true;
    close_tcp_client_sem[0] = NULL;
    get_protocol_socket_port()->source = 1;
    get_protocol_socket_port()->remotePort = Context->appConfig->at_config.conn1_config.remotePort;
    strncpy(get_protocol_socket_port()->remoteServerDomain, Context->appConfig->at_config.conn1_config.remoteServerDomain, maxRemoteServerLenth);
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Remote Client1", remoteTcpClient_thread, STACK_SIZE_REMOTE_TCP_CLIENT_THREAD, (mico_thread_arg_t)get_protocol_socket_port());
  }else{
    if( is_tcp_client_2_established )
      return kGeneralErr;
    
    is_tcp_client_2_established = true;
    close_tcp_client_sem[1] = NULL;
    get_protocol_socket_port()->source = 2;
    get_protocol_socket_port()->remotePort = Context->appConfig->at_config.conn2_config.remotePort;
    strncpy(get_protocol_socket_port()->remoteServerDomain, Context->appConfig->at_config.conn2_config.remoteServerDomain, maxRemoteServerLenth);      
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Remote Client2", remoteTcpClient_thread, STACK_SIZE_REMOTE_TCP_CLIENT_THREAD, (mico_thread_arg_t)get_protocol_socket_port());
  }
  
  return err;
}

OSStatus stop_tcp_client(app_context_t * const app_context, int event)
{
  OSStatus err = kNoErr;
  
  if( event == 1 ){
    if( !is_tcp_client_1_established )
      return kNoErr;
    
    is_tcp_client_1_established = false;
    err = mico_rtos_set_semaphore(&close_tcp_client_sem[0]);
  }else{
    if( !is_tcp_client_2_established )
      return kNoErr;
    
    is_tcp_client_2_established = false;
    err = mico_rtos_set_semaphore(&close_tcp_client_sem[1]);
  }
  
  return err;
}

