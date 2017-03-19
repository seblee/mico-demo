/**
 ******************************************************************************
 * @file    MICOAppDefine.h
 * @author  William Xu
 * @version V1.0.0
 * @date    05-May-2014
 * @brief   This file create a TCP listener thread, accept every TCP client
 *          connection and create thread for them.
 ******************************************************************************
 *  The MIT License
 *  Copyright (c) 2014 MXCHIP Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is furnished
 *  to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 *  IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 */

#pragma once

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Demo C function call C++ function and C++ function call C function */
//#define MICO_C_CPP_MIXING_DEMO
    
/*User provided configurations*/
#define CONFIGURATION_VERSION               0x00000012 // if default configuration is changed, update this number
#define FACTORYCONFIGURATION_VERSION        0xAB000123
#define MAX_QUEUE_NUM                       6  // 1 remote client, 5 local server
#define MAX_QUEUE_LENGTH                    8  // each queue max 8 msg
#define MAX_SOCKET_NUM                      6
#define SOCKET_TYPE                         15
#define MAX_TCP_CLIENT_PER_SERVER           5
  
#define wlanBufferLen                       (1024*4)
#define UART_BUFFER_LENGTH                  (1024*4)
#define AT_CMD_TIME_OUT                     (4000)
#define UART_ONE_PACKAGE_LENGTH             (1024*2)
#define MAX_RECV_BUFFER_SIZE                (1024*2)
  
#define BONJOUR_SERVICE                     "_easylink._tcp.local."

#define HTTP_USE_GZIP

/* Define thread stack size */

#define STACK_SIZE_UART_RECV_THREAD           0x500
#define STACK_SIZE_LOCAL_APP_HTTPD_THREAD    0x800
#define STACK_SIZE_LOCAL_TCP_SERVER_THREAD    0x800
#define STACK_SIZE_LOCAL_TCP_CLIENT_THREAD    0x800
#define STACK_SIZE_REMOTE_TCP_CLIENT_THREAD   0x800
#define STACK_SIZE_BOARDCAST_THREAD           0x800
#define STACK_SIZE_UNICAST_THREAD             0x800
#define STACK_SIZE_AT_CMD_THREAD              0x1000


/*suport uart band rate*/
#define UART_BANDRATE_9600              9600
#define UART_BANDRATE_19200             19200
#define UART_BANDRATE_38400             38400
#define UART_BANDRATE_57600             57600
#define UART_BANDRATE_115200            115200
#define UART_BANDRATE_230400            230400
#define UART_BANDRATE_460800            460800
#define UART_BANDRATE_921600            921600
#define UART_BANDRATE_1843200           1843200
#define UART_BANDRATE_3686400           3686400
  
#define maxRemoteServerLenth         64
#define maxDroductIDLenth            8 
#define maxDroductSecretLenth        72
#define maxLoginUserNameLenth        32
#define maxLoginPassWordLenth        32
#define sysVersionLenth              19
#define maxStaSsidlist               5

/*The operation mode of the WIFI module*/
typedef enum
{
  NO_WMODE,
  AP,
  STA,
  AP_STA,
} wifi_mode_e; 

/*protocol type of TCP/UDP connect*/
typedef enum
{
  NO_PROTOCAL,
  TCP_SERVER,
  TCP_CLIENT,
  UDP_UNICAST,
  UDP_BOARDCAST,
  CLOUD_FOG,
} conn_protocol_e;

/*wifi state*/
typedef enum
{
  NO_STATE,
  STATION_UP,
  STATION_DOWN,
  UAP_UP,
  UAP_DOWN,
} wifi_state_e;

typedef enum  _at_mode_e {
  NO_MODE,
  DATA_MODE,   //data mode
  AT_MODE,     //at mode : wifi/tcp/udp function continue
  AT_NO_MODE,  //at mode : wifi/tcp/udp function is killed.
} at_mode_e;

typedef enum _wifi_config_e {
  CONFIG_NONE,
  CONFIG_EASYLINK,
  CONFIG_WPS,
} wifi_config_e;

typedef struct _ap_config_t {
  char              ssid[maxSsidLen];
  char              key[maxKeyLen];
  int               channel;
} ap_config_t;

typedef struct _ap_ip_config_t {
  char              localIp[maxIpLen];
  char              netMask[maxIpLen];
  char              gateWay[maxIpLen];
} ap_ip_config_t;

typedef struct _sta_pre_ssid_t {
  char              ssid[maxSsidLen];
  char              key[maxKeyLen];
} sta_pre_ssid_t;

typedef struct _connect_config_t {
  conn_protocol_e   protocol;
  uint32_t          localPort;
  uint32_t          remotePort;
  char              remoteServerDomain[maxRemoteServerLenth];
  uint8_t           server_num;
} connect_config_t;

typedef platform_uart_config_t serial_config_t;

typedef struct _atuo_fomat_t {
  bool               fomatEnable;
  uint16_t           fomat_time;
  uint32_t           fomat_lenth;
} atuo_fomat_t;

typedef struct _web_login_t {
  char              login_username[maxLoginUserNameLenth];
  char              login_password[maxLoginPassWordLenth];
} web_login_t;

typedef struct _uart_auto_format_t {
  uint16_t          auto_format_time;
  uint16_t          auto_format_len;
} uart_auto_format_t;

typedef struct _wifi_state_t {
  wifi_state_e      wifi_state_sta;
  wifi_state_e      wifi_state_ap;
} wifi_state_t;

typedef struct _wifi_open_t {
  bool              ap_open;
  bool              sta_open;
} wifi_open_t;

typedef struct _time_reboot_t {
    bool            time_reboot_enable;
    uint32_t        time_reboot_time;
} time_reboot_t;


typedef struct _at_config_t
{
  /*Device identification*/
  char                   name[maxNameLen];
  
  at_mode_e              at_mode;
  bool                   at_echo;

  /*Event*/
  bool                    event;
  
  /*The operation mode of the WIFI module*/
  wifi_mode_e            wifi_mode;
  
  /*Ap mode configuration parameters*/
  ap_config_t            ap_config;
  
  /*IP configuration of Ap mode*/
  ap_ip_config_t         ap_ip_config;
  
  /*Station ssid key*/
  sta_pre_ssid_t          sta_pre_ssid[maxStaSsidlist];
  
  /*Easylink timeout*/
  wifi_config_e          wifi_config;
  uint32_t               wifi_config_timeout;
  
  /*Configuration parameters of TCP/UDP connect 1*/
  connect_config_t       conn1_config;
  
  /*Configuration parameters of TCP/UDP connect 2*/
  bool                   conn2_enable;
  connect_config_t       conn2_config;
  
  /*Configuration parameters of SOCKET*/
  bool                   socket_recv;
  
  /*Web log in user name and password*/
  bool                   webEnable;
  web_login_t            web_login;

  /*Services in MICO system*/
  bool            bonjourEnable;
  
  /*Configuration parameters of serial*/
  serial_config_t        serial_config;
  
  /*Configuration parameters of uart auto format*/
  bool                   autoFormatEnable;
  uart_auto_format_t     uart_auto_format;
  
  /*time reboot*/
  time_reboot_t          time_reboot;

} at_config_t;

/*Factory's configuration stores in flash*/
typedef struct _factory_config_t {
  
  uint32_t              factoryConfigDataVer;
  
  at_mode_e              at_mode;

  /*Event*/
  bool                   event;
  
  /*Services in MICO system*/
  bool                   bonjourEnable;
  
  /*The operation mode of the WIFI module*/
  wifi_mode_e            wifi_mode;
  
  /*Ap mode configuration parameters*/
  ap_config_t            ap_config;
  
  /*IP configuration of Ap mode*/
  ap_ip_config_t         ap_ip_config;
  
  /*dhcp enable*/
  bool                   dhcpEable;
  
   /*Easylink timeout*/
  uint32_t               wifi_config_timeout;
  
  /*Configuration parameters of TCP/UDP connect 1*/
  connect_config_t       conn1_config;
  
  /*Configuration parameters of TCP/UDP connect 2*/
  bool                   conn2_enable;
  connect_config_t       conn2_config;
  
  /*Configuration parameters of serial*/
  serial_config_t        serial_config;
  
  /*Web log in user name and password*/
  bool                   webEnable;
  web_login_t            web_login;
  
} factory_config_t;

/*Application's configuration stores in flash*/
typedef struct
{
  factory_config_t  factory_config;
  at_config_t      at_config;
} application_config_t;

typedef struct _socket_port_t {
  int               source;
  int               fd;  
  uint32_t          localPort;
  uint32_t          remotePort;
  char              remoteServerDomain[maxRemoteServerLenth];
} socket_port_t;

typedef struct _socket_msg {
  int ref;
  int len;
  uint8_t data[1];
} socket_msg_t;

typedef struct _connect_state_t {
  uint8_t                connect1_server_link_count;
  bool                   connect1_client_link;
  bool                   connect1_open;
  uint8_t                connect2_server_link_count;
  bool                   connect2_client_link;
  bool                   connect2_open;
} connect_state_t;

/*Running status*/
typedef struct  {
  /*Local clients port list*/
  mico_queue_t*   socket_out_queue[MAX_QUEUE_NUM];
  mico_mutex_t    queue_mtx;
  
  int                    socket_num;
  
  wifi_state_t           wifi_state;
  connect_state_t        connect_state;
  
  wifi_open_t            wifi_open;
  at_mode_e              at_mode;
  bool                   at_echo;
  
} current_app_status_t;


typedef struct _app_context_t
{
  /*Flash content*/
  application_config_t*     appConfig;

  /*Running status*/
  current_app_status_t      appStatus;
} app_context_t;

int event_printf(const char *msg, ...);

void get_wifi_model_name(char *model);
void get_firmware_version(char *version);

void para_restore_default(void);
void app_state_config(void *app_context_t);

OSStatus app_init_for_mico_system(void *mico_context_c);
void app_wlan_init(void *app_context_c);
OSStatus uart_recv_data_mode_init(void *app_context_c);
void get_uart_recv_len_time(app_context_t * const app_context, uint32_t *recv_len, uint32_t *recv_time);

OSStatus mico_data_mode(void *app_context_c);

void startSoftAP(void *app_context_c);
OSStatus app_tcp_ip_init(void *app_context_c);


#ifdef __cplusplus
} /*extern "C" */
#endif


