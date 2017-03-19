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

#define uincast_log(M, ...) custom_log("UDP UNICAST", M, ##__VA_ARGS__)
#define uincast_log_trace() custom_log_trace("UDP UNICAST")

static app_context_t *Context;
static bool _wifiConnected = false;
static mico_semaphore_t  _wifiConnected_sem[2] = {NULL}, close_udp_unicast_sem[2] = {NULL};

static bool is_udp_unicast_1_established = false;
static bool is_udp_unicast_2_established = false;
static bool is_egisist_wifi_status_notify = false;

void unicastNotify_WifiStatusHandler(int event, mico_Context_t * const inContext)
{
  uincast_log_trace();
  (void)inContext;
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

void unicast_thread(mico_thread_arg_t arg)
{
  OSStatus err;
  struct _socket_port_t socket_port;
  socket_port = *( socket_port_t *)arg;
  int len, sent_len;
  uint8_t *inDataBuffer = NULL;
  struct sockaddr_in addr, set_addr;
  socklen_t addrLen;
  fd_set readfds;
  fd_set writeSet;
  struct timeval t;
  char ipstr[16];
  int udp_unicast_fd = -1;
  int eventFd = -1;
   int close_udp_uincast_fd = -1;
  mico_queue_t queue;
  socket_msg_t *msg;
  cmd_data_send_t *cmd_data_send;
  udp_cmd_send_t *udp_cmd_send;
  LinkStatusTypeDef wifi_link;
  struct hostent* hostent_content = NULL;
  char **pptr = NULL;
  struct in_addr in_addr;
  
  mico_rtos_init_semaphore(&_wifiConnected_sem[socket_port.source-1], 1);
  mico_rtos_init_semaphore(&close_udp_unicast_sem[socket_port.source-1], 1);
  
  inDataBuffer = malloc(wlanBufferLen);
  require_action(inDataBuffer, exit, err = kNoMemoryErr);
  
  err = micoWlanGetLinkStatus( &wifi_link );
  require_noerr( err, exit );
  
  if( wifi_link.is_connected == true )
    _wifiConnected = true;
  
  if( Context->appStatus.wifi_open.ap_open == true ){
      _wifiConnected = true;
  }

  uincast_log("UDP uincast %d established at local port:%ld, remote port: %ld", socket_port.source,
                socket_port.localPort, socket_port.remotePort);
    
  while(1) {
    if(udp_unicast_fd == -1) {
      if(_wifiConnected == false){
        require_action_quiet(mico_rtos_get_semaphore(&_wifiConnected_sem[socket_port.source-1], 200000) == kNoErr, Continue, err = kTimeoutErr);
      }

      if( socket_port.remotePort != 0 ){
          hostent_content = gethostbyname((char *)socket_port.remoteServerDomain);
          require_action_quiet( hostent_content != NULL, ReConnWithDelay, err = kNotFoundErr);
          pptr=hostent_content->h_addr_list;
          in_addr.s_addr = *(uint32_t *)(*pptr);
          strcpy( ipstr, inet_ntoa(in_addr));
      }
      udp_unicast_fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
      require_action( IsValidSocket( udp_unicast_fd ), exit, err = kNoResourcesErr );
      
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
      addr.sin_port = htons(socket_port.localPort);
    
      err = bind(udp_unicast_fd, (struct sockaddr *)&addr, sizeof(addr));
      require_noerr(err, exit);
      
      if( socket_port.remotePort != 0 ){
          addr.sin_family = AF_INET;
          addr.sin_addr.s_addr = inet_addr(ipstr);
          addr.sin_port = htons(socket_port.remotePort);
      
        err = connect(udp_unicast_fd, (struct sockaddr *)&addr, sizeof(addr));
        require_noerr_quiet(err, ReConnWithDelay);
      }
      
      cmd_socket_add(udp_unicast_fd);
      uincast_log("udp unicast %d connected at port: %ld, fd: %d",  socket_port.source, socket_port.remotePort, udp_unicast_fd);
      event_printf("+EVENT=UDP_UNICAST,CONNECT,%d\r\n", udp_unicast_fd);
      
      err = socket_queue_create(Context, &queue);
      require_noerr( err, exit );
      
      eventFd = mico_create_event_fd(queue);
      if (eventFd < 0) {
        uincast_log("create event fd error");
        goto exit;
      } 
      
      close_udp_uincast_fd = mico_create_event_fd(close_udp_unicast_sem[socket_port.source - 1]);
      if( close_udp_uincast_fd < 0 ){
        uincast_log("create close fd err");
        socket_queue_delete(Context, &close_udp_unicast_sem[socket_port.source - 1]);
        goto exit;
      }
    } else {
      
      FD_ZERO(&readfds);
      FD_SET(udp_unicast_fd, &readfds);
      FD_SET(eventFd, &readfds); 
      FD_SET(close_udp_uincast_fd, &readfds); 
      
      require_action( select(udp_unicast_fd + eventFd + close_udp_uincast_fd +1, &readfds, NULL, NULL, NULL) >= 0, exit, err = kConnectionErr );
      
      /* send UART data */
      if (FD_ISSET( eventFd, &readfds )) { // have data and can write
        FD_ZERO(&writeSet );
        FD_SET(udp_unicast_fd, &writeSet );
        t.tv_usec = 100*1000; // max wait 100ms.
        select(udp_unicast_fd + 1, NULL, &writeSet, NULL, &t);
        if((FD_ISSET( udp_unicast_fd, &writeSet )) && (kNoErr == mico_rtos_pop_from_queue( &queue, &msg, 0))) {
          
          if(Context->appStatus.at_mode == DATA_MODE){
            sendto(udp_unicast_fd, msg->data, msg->len, 0, (const struct sockaddr *)&addr, sizeof(addr));
          }else{
            cmd_data_send = (cmd_data_send_t *) msg->data;
            if( cmd_data_send->cmd_send_type == CMD_DATA ){
              if( cmd_data_send->socket !=0 ){
                if( cmd_data_send->socket == udp_unicast_fd ){
                  sent_len = sendto(udp_unicast_fd, cmd_data_send->buf, cmd_data_send->lenth, 0, (const struct sockaddr *)&addr, sizeof(addr));
                    if( sent_len > 0){
                      at_printf("+OK\r\n");
                    }else{
                      at_printf("+ERR=%d\r\n", INVALID_OPERATION);
                    }                  
                }
              }else{
                sendto(udp_unicast_fd, cmd_data_send->buf, cmd_data_send->lenth, 0, (const struct sockaddr *)&addr, sizeof(addr));
              }
            }else{
              udp_cmd_send = (udp_cmd_send_t *) msg->data;
              if( udp_cmd_send->cmd_send_type == CMD_UNICAST ){
                if( udp_cmd_send->socket !=0 ){
                  if( udp_cmd_send->socket == udp_unicast_fd ){
                    if( udp_cmd_send->port != 0 ){
                        set_addr.sin_family = AF_INET;
                        set_addr.sin_addr.s_addr = inet_addr(udp_cmd_send->ip);
                        set_addr.sin_port = htons(udp_cmd_send->port);
                      sendto(udp_unicast_fd, udp_cmd_send->buf, udp_cmd_send->lenth, 0, (const struct sockaddr *)&set_addr, sizeof(set_addr));
                    }else{ 
                      sendto(udp_unicast_fd, udp_cmd_send->buf, udp_cmd_send->lenth, 0, (const struct sockaddr *)&addr, sizeof(addr));
                    }
                  }
                }
              }              
            }
          }
          
          socket_msg_free(msg);
        }
      }
      
      if(FD_ISSET( udp_unicast_fd, &readfds ))
      {
        len = recvfrom(udp_unicast_fd, inDataBuffer, wlanBufferLen, 0, (struct sockaddr *)&addr, &addrLen);
        event_printf("+EVENT=SOCKET,%d,%d,", udp_unicast_fd, len);
        sppWlanCommandProcess(inDataBuffer, &len, udp_unicast_fd, Context);
        event_printf("\r\n");
      }
      
      /*check delete thread*/
      if( FD_ISSET(close_udp_uincast_fd, &readfds) ){
        mico_rtos_get_semaphore(&close_udp_unicast_sem[socket_port.source - 1], 0);
        goto exit;
      }      
      
    Continue:    
      continue;
      
    ReConnWithDelay:
      if (eventFd >= 0) {
        mico_delete_event_fd(eventFd);
        socket_queue_delete(Context, &queue);
        eventFd = -1;
      }
      if( close_udp_uincast_fd >= 0 ){
        mico_delete_event_fd(close_udp_uincast_fd);
        close_udp_uincast_fd = -1;
      }
      if(udp_unicast_fd != -1){
        cmd_socket_remove(udp_unicast_fd);
        SocketClose(&udp_unicast_fd);
        udp_unicast_fd = -1;
      }
      mico_thread_sleep(1);
    }
  }
  
exit:
  
  event_printf("+EVENT=UDP_UNICAST,DISCONNECT,%d\r\n", udp_unicast_fd);
  
  if(inDataBuffer) free(inDataBuffer);
  if( eventFd >= 0 ){
    mico_delete_event_fd(eventFd);
    socket_queue_delete(Context, &queue);
  }
  if( close_udp_uincast_fd >= 0 ){
    mico_delete_event_fd(close_udp_uincast_fd);
  }
  if( udp_unicast_fd != -1 ){
    cmd_socket_remove(udp_unicast_fd);
    SocketClose(&udp_unicast_fd);
    udp_unicast_fd = -1;
  }
  if( _wifiConnected_sem[socket_port.source-1] != NULL ){
      mico_rtos_deinit_semaphore( &_wifiConnected_sem[socket_port.source-1] );
      _wifiConnected_sem[socket_port.source-1] = NULL;
  }
  if( close_udp_unicast_sem[socket_port.source-1] != NULL ){
    mico_rtos_deinit_semaphore( &close_udp_unicast_sem[socket_port.source-1] );
    close_udp_unicast_sem[socket_port.source-1] = NULL;
  }

  uincast_log("Exit: udp unicast %d exit with err = %d", socket_port.source, err);
  mico_rtos_delete_thread(NULL);
}

OSStatus start_udp_uincast(app_context_t * const app_context, int event)
{
  OSStatus err = kGeneralErr;
  Context = app_context;
  
  if( is_egisist_wifi_status_notify == false ){
    is_egisist_wifi_status_notify = true;
    mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *)unicastNotify_WifiStatusHandler, NULL );
  }
  if( event == 1 ){
    if( is_udp_unicast_1_established )
      return kGeneralErr;
      
    is_udp_unicast_1_established = true;
    close_udp_unicast_sem[0] = NULL;
    get_protocol_socket_port()->source = 1;
    get_protocol_socket_port()->localPort = Context->appConfig->at_config.conn1_config.localPort;
    get_protocol_socket_port()->remotePort = Context->appConfig->at_config.conn1_config.remotePort;
    strncpy(get_protocol_socket_port()->remoteServerDomain, Context->appConfig->at_config.conn1_config.remoteServerDomain, maxRemoteServerLenth);
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "unicast1", unicast_thread, STACK_SIZE_UNICAST_THREAD, (mico_thread_arg_t)get_protocol_socket_port());
  }else{
    if( is_udp_unicast_2_established )
      return kGeneralErr;
      
    is_udp_unicast_2_established = true;
    close_udp_unicast_sem[1] = NULL;
    get_protocol_socket_port()->source = 2;
    get_protocol_socket_port()->localPort = Context->appConfig->at_config.conn2_config.localPort;
    get_protocol_socket_port()->remotePort = Context->appConfig->at_config.conn2_config.remotePort;
    strncpy(get_protocol_socket_port()->remoteServerDomain, Context->appConfig->at_config.conn2_config.remoteServerDomain, maxRemoteServerLenth);      
    err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "unicast2", unicast_thread, STACK_SIZE_UNICAST_THREAD, (mico_thread_arg_t)get_protocol_socket_port());
  }
  
  return err;
}

OSStatus stop_udp_uincast(app_context_t * const app_context, int event)
{
  OSStatus err = kGeneralErr;
  
  if( event == 1 ){
    if( !is_udp_unicast_1_established )
      return kNoErr;
      
    is_udp_unicast_1_established = false;
    err = mico_rtos_set_semaphore(&close_udp_unicast_sem[0]);
  }else{
    if( !is_udp_unicast_2_established )
      return kNoErr;
    
    is_udp_unicast_2_established = false;
    err = mico_rtos_set_semaphore(&close_udp_unicast_sem[1]);
  }
  
  return err;
}

