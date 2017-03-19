#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "Common.h"
#include "at_cmd.h"

#define SOCKET_FD_NUM 16

typedef enum  _cmd_send_type_e {
  CMD_DATA,
  CMD_UNICAST,
} cmd_send_type_e;

typedef struct _cmd_data_send_t {
  cmd_send_type_e cmd_send_type;
  int socket;
  int lenth;
  uint8_t buf[1];
} cmd_data_send_t;

typedef struct _udp_cmd_send_t {
  cmd_send_type_e cmd_send_type;
  int socket;
  int port;
  char ip[maxIpLen];
  int lenth;
  uint8_t buf[1];
} udp_cmd_send_t;

void protocol_init( void );
socket_port_t *get_protocol_socket_port( void );
void cmd_socket_add( int socket );
void cmd_socket_remove( int socket );
int cmd_socket_check( int socket );

void start_sta_pre_ssid_set_manage(void *app_context_c);

OSStatus start_tcp_ip( app_context_t * const app_context, int event );
OSStatus stop_tcp_ip( app_context_t *const app_context, int event );

void stop_all_net_work(app_context_t *const app_context);

OSStatus MICOStartBonjourService( WiFi_Interface interface, app_context_t * const inContext );

OSStatus start_tcp_server(app_context_t * const app_context, int event);
OSStatus stop_tcp_server(app_context_t * const app_context, int event);

OSStatus start_tcp_client(app_context_t * const app_context, int event);
OSStatus stop_tcp_client(app_context_t * const app_context, int event);

OSStatus start_udp_boardcast(app_context_t * const app_context, int event);
OSStatus stop_udp_boardcast(app_context_t * const app_context, int event);

OSStatus start_udp_uincast(app_context_t * const app_context, int event);
OSStatus stop_udp_uincast(app_context_t * const app_context, int event);
#endif