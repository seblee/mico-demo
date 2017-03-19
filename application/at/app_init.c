#include "mico.h"
#include "MICOAppDefine.h"
#include "at_cmd.h"
#include "WPS.h"
#include "AutoConfig.h"
#include "protocol.h"
#include "http_server.h"
#include "system_internal.h"

#define app_init_log(M, ...) custom_log("INIT", M, ##__VA_ARGS__)

volatile ring_buffer_t rx_buffer;
volatile uint8_t rx_data[UART_BUFFER_LENGTH];

static mico_Context_t *mico_context;
static app_context_t *app_context;

//static void time_reboot_timer_handle( void *arg )
//{
////    mico_system_power_perform( mico_context, eState_Software_Reset );
//    MicoSystemReboot( );
//}
static void reboot_thread(uint32_t arg)
{

    mico_rtos_thread_sleep(app_context->appConfig->at_config.time_reboot.time_reboot_time);
    mico_system_power_perform( mico_context, eState_Software_Reset );
    mico_rtos_delete_thread(NULL);
}
static void appNotify_WifiStatusHandler(WiFiEvent event, mico_Context_t * const inContext)
{
  (void)inContext;
  switch (event) {
  case NOTIFY_STATION_UP:
    app_context->appStatus.wifi_state.wifi_state_sta = STATION_UP;
    event_printf("+EVENT=WIFI_LINK,STATION_UP\r\n");
    break;
  case NOTIFY_STATION_DOWN:
    app_context->appStatus.wifi_state.wifi_state_sta = STATION_DOWN;
    event_printf("+EVENT=WIFI_LINK,STATION_DOWN\r\n");
    break;
  case NOTIFY_AP_UP:
    app_context->appStatus.wifi_state.wifi_state_ap = UAP_UP;
    event_printf("+EVENT=WIFI_LINK,UAP_UP\r\n");
    break;
  case NOTIFY_AP_DOWN:
    app_context->appStatus.wifi_state.wifi_state_ap = UAP_DOWN;
    event_printf("+EVENT=WIFI_LINK,UAP_DOWN\r\n");
    break;
  default:
    break;
  }
  return;
}

static void appNotify_SysWillPower(mico_Context_t * const inContext)
{
    event_printf("+EVENT=REBOOT\r\n");
}

static void soft_ap_ssid_set(void)
{
  char *ap_ssid_bf, *ap_ssid_ex, *p_ap_ssid, ap_ssid[32];
  mico_system_status_wlan_t *wlan_status;
  mico_system_wlan_get_status( &wlan_status );
  
  p_ap_ssid = malloc(32);
  ap_ssid_ex = p_ap_ssid;
  strcpy(ap_ssid_ex, app_context->appConfig->at_config.ap_config.ssid);
  ap_ssid_bf = strsep(&ap_ssid_ex, "+");
  if(!strncmp(ap_ssid_ex, "MAC", 3)) {
    sprintf(ap_ssid, "%s_%c%c%c%c%c%c", ap_ssid_bf, wlan_status->mac[9],  wlan_status->mac[10],
            wlan_status->mac[12], wlan_status->mac[13],
            wlan_status->mac[15], wlan_status->mac[16]);
  } else {
    sprintf(ap_ssid, "%s", ap_ssid_bf);
  }
  
  strcpy(app_context->appConfig->at_config.ap_config.ssid, ap_ssid);
  
  mico_system_context_update(mico_context);
  
  free(p_ap_ssid);
  
}

static int event_putstr(const char *msg)
{
  if (msg[0] != 0)
    MicoUartSend( UART_FOR_APP, (const char*)msg, strlen(msg) );
  
  return 0;
}

int event_printf(const char *msg, ...)
{
  if(app_context->appConfig->at_config.event == true) {
    va_list ap; 
    char *pos, message[256]; 
    int sz; 
    int nMessageLen = 0;
    
    memset(message, 0, 256);
    pos = message;
    
    sz = 0;
    va_start(ap, msg);
    nMessageLen = vsnprintf(pos, 256 - sz, msg, ap);
    va_end(ap);
    
    if( nMessageLen<=0 ) return 0;
    
    event_putstr((const char*)message);
  }
  return 0;
}

void get_wifi_model_name(char *model)
{
  char wifi_model[13];
  char *pos;
  sprintf(wifi_model, "%s", MODEL);
  pos = wifi_model;
  pos += (strlen(pos)-4);
  sprintf(model, "EMW%s", pos);
}

void get_firmware_version(char *version)
{
  char model[10];
  get_wifi_model_name(model);
  sprintf(version, "ATV%s@%s", APP_VERSION, model);
}

extern void wlan_set_channel(int );

void startSoftAP(void *app_context_c)
{
  app_context = app_context_c;
  network_InitTypeDef_st wNetConfig;
  
  memset(&wNetConfig, 0, sizeof(network_InitTypeDef_st));
  
#ifdef EMW1062
  wlan_set_channel(app_context->appConfig->at_config.ap_config.channel);
#endif    
  
  wNetConfig.wifi_mode = Soft_AP;
  strncpy(wNetConfig.wifi_ssid, app_context->appConfig->at_config.ap_config.ssid, maxSsidLen);
  strcpy((char*)wNetConfig.wifi_key, app_context->appConfig->at_config.ap_config.key);
  strcpy((char*)wNetConfig.local_ip_addr, app_context->appConfig->at_config.ap_ip_config.localIp);
  strcpy((char*)wNetConfig.net_mask, app_context->appConfig->at_config.ap_ip_config.netMask);
  strcpy((char*)wNetConfig.gateway_ip_addr, app_context->appConfig->at_config.ap_ip_config.gateWay);
  wNetConfig.dhcpMode = DHCP_Server;
  wNetConfig.wifi_retry_interval = 100;
  micoWlanStart(&wNetConfig);
  app_init_log("Establish soft ap: %s.....", wNetConfig.wifi_ssid);
}

OSStatus app_init_for_mico_system(void *mico_context_c)
{
  OSStatus err = kNoErr;
  char ver[sysVersionLenth];
  mico_context = mico_context_c;

  err = system_notification_init( system_context() );
  require_noerr( err, exit ); 
  
  get_firmware_version(ver);
  app_init_log("firmware ver %s", ver);
  
#ifdef MICO_SYSTEM_MONITOR_ENABLE
  /* MiCO system monitor */
  err = mico_system_monitor_daemen_start( );
  require_noerr( err, exit ); 
#endif
  
#ifdef MICO_CLI_ENABLE
  /* MiCO command line interface */
  cli_init();
#endif
  
  /* Network PHY driver and tcp/ip static init */
  err = system_network_daemen_start( system_context() );
  require_noerr( err, exit ); 
exit:
  return err;
}

OSStatus uart_recv_data_mode_init(void *app_context_c)
{
  OSStatus err = kNoErr;
  app_context = app_context_c;
  mico_uart_config_t uart_config;
  
  uart_config.baud_rate = app_context->appConfig->at_config.serial_config.baud_rate;
  uart_config.data_width = app_context->appConfig->at_config.serial_config.data_width;
  uart_config.parity = app_context->appConfig->at_config.serial_config.parity;
  uart_config.stop_bits = app_context->appConfig->at_config.serial_config.stop_bits;
  uart_config.flow_control = app_context->appConfig->at_config.serial_config.flow_control;
  if ( mico_context->micoSystemConfig.mcuPowerSaveEnable == true )
    uart_config.flags = UART_WAKEUP_ENABLE;
  else
    uart_config.flags = UART_WAKEUP_DISABLE;
  
  ring_buffer_init( (ring_buffer_t *) &rx_buffer, (uint8_t *) rx_data, UART_BUFFER_LENGTH );
  MicoUartInitialize( UART_FOR_APP, &uart_config, (ring_buffer_t *) &rx_buffer );
  
  if( app_context->appStatus.at_mode == DATA_MODE ){
    mico_data_mode( app_context );
  }else{
    mico_at_mode( app_context );
  }
  
  return err;
}

void app_wlan_init(void *app_context_c)
{
  mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *)appNotify_WifiStatusHandler, NULL );
  mico_system_notify_register( mico_notify_SYS_WILL_POWER_OFF, (void *)appNotify_SysWillPower, NULL );
  
  app_context = (app_context_t*)app_context_c;
  soft_ap_ssid_set();
  if (app_context->appConfig->at_config.wifi_config == CONFIG_EASYLINK) {
    startAutoConfig( app_context );
  }else if(app_context->appConfig->at_config.wifi_config == CONFIG_WPS){
#ifdef EMW1062
#ifndef __GNUC__
    startWPS(app_context);
#endif
#endif    
  }else{
    if( app_context->appStatus.at_mode != AT_NO_MODE ){
      if (app_context->appConfig->at_config.wifi_mode == AP) {
        startSoftAP( app_context );
        app_context->appStatus.wifi_open.ap_open = true;
      } else if (app_context->appConfig->at_config.wifi_mode == STA){
        start_sta_pre_ssid_set_manage( app_context );
        system_connect_wifi_fast( system_context() );
        app_context->appStatus.wifi_open.sta_open = true;
      } else if (app_context->appConfig->at_config.wifi_mode == AP_STA){
        startSoftAP( app_context );
        system_connect_wifi_fast( system_context() );
        app_context->appStatus.wifi_open.ap_open = true;
        app_context->appStatus.wifi_open.sta_open = true;
      }
    }
  }
  if( app_context->appConfig->at_config.wifi_config !=  CONFIG_NONE){
    app_context->appConfig->at_config.wifi_config =  CONFIG_NONE;
    mico_system_context_update(mico_context);
  }
}

#ifdef EMW1062
extern int uap_dns_redirector(int );
#endif

OSStatus app_tcp_ip_init(void *app_context_c)
{
  OSStatus err = kNoErr;
  app_context = app_context_c;
  
#ifdef EMW1062
  uap_dns_redirector(0); //close dns
#endif	
  
  if( app_context->appConfig->at_config.webEnable == true ) {
    http_server_fs_start(app_context);
  }
  
  MICOStartBonjourService( Station, app_context );
  
  protocol_init();
  
  if( app_context->appStatus.at_mode != AT_NO_MODE ){  
    start_tcp_ip(app_context, 1);
    mico_thread_msleep(50);
    start_tcp_ip(app_context, 2);
  }

  if( app_context->appConfig->at_config.time_reboot.time_reboot_enable == true ){
      mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "reboot", reboot_thread, 0x300, 0 );
      app_init_log("reboot timer, time %lds", app_context->appConfig->at_config.time_reboot.time_reboot_time);
  }
  return err;
}

void show_free_memory( void )
{
  app_init_log(" free memory %d", MicoGetMemoryInfo()->free_memory);
}

