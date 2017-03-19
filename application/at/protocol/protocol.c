#include "MICO.h"
#include "MICOAppDefine.h"
#include "protocol.h"

#define protocol_log(M, ...) custom_log("PROTOCOL", M, ##__VA_ARGS__)
static socket_port_t *socket_port;
static int check_socket_fd[16];

void cmd_socket_check_init( void )
{
  int socket_num;
  for( socket_num=0; socket_num<SOCKET_FD_NUM; socket_num++)
  {
    check_socket_fd[socket_num] = -1;
  }
}

void cmd_socket_add( int socket )
{
  int socket_num;
  for( socket_num=0; socket_num<SOCKET_FD_NUM; socket_num++)
  {
    if(check_socket_fd[socket_num] == -1){
      check_socket_fd[socket_num] = socket;
      break;
    }
  }
}

void cmd_socket_remove( int socket )
{
  int socket_num;
  for( socket_num=0; socket_num<SOCKET_FD_NUM; socket_num++)
  {
    if(check_socket_fd[socket_num] == socket){
      check_socket_fd[socket_num] = -1;
      break;
    }
  }  
}

int cmd_socket_check( int socket )
{
  int socket_num;
  
  if( socket == -1)
    return -1;
  
  for( socket_num=0; socket_num<SOCKET_FD_NUM; socket_num++)
  {
    if(check_socket_fd[socket_num] == socket){
      return 0;
    }
  }
  return -1;
}

void protocol_init( void )
{
  socket_port = malloc( sizeof(socket_port_t) );
  cmd_socket_check_init();
}

socket_port_t *get_protocol_socket_port( void )
{
  return socket_port;
}


OSStatus start_tcp_ip( app_context_t * const app_context, int event )
{
  OSStatus err = kNoErr;
  
  if( event == 1 ){
    if( app_context->appStatus.connect_state.connect1_open == true )
    return kGeneralErr;
    
    switch( app_context->appConfig->at_config.conn1_config.protocol ){
    case TCP_SERVER:
      err = start_tcp_server(app_context, 1);
      require_noerr_action( err, exit, protocol_log("ERROR: Unable to start the local server1 thread.") );
      app_context->appStatus.connect_state.connect1_open = true;
      break;
    case TCP_CLIENT:
      err = start_tcp_client(app_context, 1);
      require_noerr_action( err, exit, protocol_log("ERROR: Unable to start the remote client1 thread.") );
      app_context->appStatus.connect_state.connect1_open = true;
      break;
    case UDP_BOARDCAST:
      err = start_udp_boardcast(app_context, 1);
      require_noerr_action( err, exit, protocol_log("ERROR: Unable to start the udp boardcast1 thread.") );
      app_context->appStatus.connect_state.connect1_open = true;
      break;
    case UDP_UNICAST:
      err = start_udp_uincast(app_context, 1);
      require_noerr_action( err, exit, protocol_log("ERROR: Unable to start the udp uincast1 thread.") );
      app_context->appStatus.connect_state.connect1_open = true;
      break;
    default:
      break;
    }
  }else if( event == 2){
    if(app_context->appConfig->at_config.conn2_enable == false) 
      return kGeneralErr;
      
    if( app_context->appStatus.connect_state.connect2_open == true )
      return kGeneralErr;
    
    switch( app_context->appConfig->at_config.conn2_config.protocol ){
    case TCP_SERVER:
      err = start_tcp_server(app_context, 2);
      require_noerr_action( err, exit, protocol_log("ERROR: Unable to start the local server2 thread.") );
      app_context->appStatus.connect_state.connect2_open = true;
      break;
    case TCP_CLIENT:
      err = start_tcp_client(app_context, 2);
      require_noerr_action( err, exit, protocol_log("ERROR: Unable to start the remote client2 thread.") );
      app_context->appStatus.connect_state.connect2_open = true;
      break;
    case UDP_BOARDCAST:
      err = start_udp_boardcast(app_context, 2);
      require_noerr_action( err, exit, protocol_log("ERROR: Unable to start the udp boardcast2 thread.") );
      app_context->appStatus.connect_state.connect2_open = true;
      break;
    case UDP_UNICAST:
      err = start_udp_uincast(app_context, 2);
      require_noerr_action( err, exit, protocol_log("ERROR: Unable to start the udp uincast2 thread.") );
      app_context->appStatus.connect_state.connect2_open = true;
      break;
    default:
      break;
    } 
  }
  
exit:
    return err;
}

OSStatus stop_tcp_ip( app_context_t *const app_context, int event )
{
  OSStatus err = kNoErr; 
  
  if( event == 1 ){
    if( app_context->appStatus.connect_state.connect1_open == false )
      return kGeneralErr;
    
    switch( app_context->appConfig->at_config.conn1_config.protocol ){
    case TCP_SERVER:
      err = stop_tcp_server(app_context, 1);
      require_noerr_action( err, exit, protocol_log("ERROR: Unable to stop the local server1 thread.") );
      app_context->appStatus.connect_state.connect1_open = false;
      break;
    case TCP_CLIENT:
      err = stop_tcp_client(app_context, 1);
      require_noerr_action( err, exit, protocol_log("ERROR: Unable to stop the remote client1 thread.") );
      app_context->appStatus.connect_state.connect1_open = false;
      break;
    case UDP_BOARDCAST:
      err = stop_udp_boardcast(app_context, 1);
      require_noerr_action( err, exit, protocol_log("ERROR: Unable to stop the udp boardcast1 thread.") );
      app_context->appStatus.connect_state.connect1_open = false;
      break;
    case UDP_UNICAST:
      err = stop_udp_uincast(app_context, 1);
      require_noerr_action( err, exit, protocol_log("ERROR: Unable to stop the udp uincast1 thread.") );
      app_context->appStatus.connect_state.connect1_open = false;
      break;
    default:
      break;
    }
  }else if( event == 2){
    if(app_context->appConfig->at_config.conn2_enable == false) 
      return kGeneralErr;
    
    if( app_context->appStatus.connect_state.connect2_open == false )
      return kGeneralErr;
    
      switch( app_context->appConfig->at_config.conn2_config.protocol ){
      case TCP_SERVER:
        err = stop_tcp_server(app_context, 2);
        require_noerr_action( err, exit, protocol_log("ERROR: Unable to stop the local server2 thread.") );
        app_context->appStatus.connect_state.connect2_open = false;
        break;
      case TCP_CLIENT:
        err = stop_tcp_client(app_context, 2);
        require_noerr_action( err, exit, protocol_log("ERROR: Unable to stop the remote client2 thread.") );
        app_context->appStatus.connect_state.connect2_open = false;
        break;
      case UDP_BOARDCAST:
        err = stop_udp_boardcast(app_context, 2);
        require_noerr_action( err, exit, protocol_log("ERROR: Unable to stop the udp boardcast2 thread.") );
        app_context->appStatus.connect_state.connect2_open = false;
        break;
      case UDP_UNICAST:
        err = stop_udp_uincast(app_context, 2);
        require_noerr_action( err, exit, protocol_log("ERROR: Unable to stop the udp uincast2 thread.") );
        app_context->appStatus.connect_state.connect2_open = false;
        break;
      default:
        break;
      }
    }
  
exit:
    return err;
}


void stop_all_net_work(app_context_t *const app_context)
{
  stop_tcp_ip(app_context, 1);
  mico_thread_msleep(100);
  stop_tcp_ip(app_context, 2);
  micoWlanSuspend( );
  app_context->appStatus.wifi_open.ap_open = false;
  app_context->appStatus.wifi_open.sta_open = false;
}
