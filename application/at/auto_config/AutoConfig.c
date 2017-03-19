/**
******************************************************************************
* @file    AutoConfig.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide the AutoConfig function and FTC server for quick 
*          provisioning and first time configuration.
******************************************************************************
*
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

#include "MICO.h"
#include "MICOAppDefine.h"
#include "StringUtils.h"
#include "HTTPUtils.h"
#include "SocketUtils.h"
#include "system_internal.h"
#include "AutoConfig.h"
  
// AutoConfig HTTP messages
#define kEasyLinkURLAuth          "/auth-setup"

#define autoconfig_log(M, ...) custom_log("AutoConfig", M, ##__VA_ARGS__)
#define autoconfig_log_trace() custom_log_trace("AutoConfig")

static mico_semaphore_t      AutoConfig_sem;
static int                   AutoConfigClient_fd;
static uint8_t airkiss_data;
static mico_config_source_t source = CONFIG_BY_NONE;
static bool AutoConfigFailed = false;

static mico_Context_t *mico_context;
static app_context_t *app_context;

static void AutoConfig_thread(mico_thread_arg_t inContext);

static OSStatus mico_easylink_bonjour_start( WiFi_Interface interface, mico_Context_t * const inContext );
static void remove_bonjour_for_easylink(void);
static OSStatus mico_easylink_bonjour_update( WiFi_Interface interface, mico_Context_t * const inContext );


void AutoConfigNotify_WifiStatusHandler(WiFiEvent event, mico_Context_t * const inContext)
{
  autoconfig_log_trace();
  IPStatusTypedef para;

  require(inContext, exit);
  switch (event) {
  case NOTIFY_STATION_UP:
    mico_easylink_bonjour_update(Station, inContext);
    micoWlanGetIPStatus(&para, Station);
    strncpy(inContext->micoSystemConfig.localIp, para.ip, maxIpLen);
    strncpy(inContext->micoSystemConfig.netMask, para.mask, maxIpLen);
    strncpy(inContext->micoSystemConfig.gateWay, para.gate, maxIpLen);
    strncpy(inContext->micoSystemConfig.dnsServer, para.dns, maxIpLen);
    mico_rtos_set_semaphore(&AutoConfig_sem);
    break;
  default:
    break;
  }
exit:
  return;
}


void AutoConfigNotify_AutoConfigCompleteHandler(network_InitTypeDef_st *nwkpara, mico_Context_t * const inContext)
{
  OSStatus err;
  autoconfig_log_trace();
  autoconfig_log("AutoConfig return");
  require_action(inContext, exit, err = kParamErr);
  require_action(nwkpara, exit, err = kTimeoutErr);
  

  memcpy(inContext->micoSystemConfig.ssid, nwkpara->wifi_ssid, maxSsidLen);
  memset(inContext->micoSystemConfig.bssid, 0x0, 6);
  memcpy(inContext->micoSystemConfig.user_key, nwkpara->wifi_key, maxKeyLen);
  inContext->micoSystemConfig.user_keyLength = strlen(nwkpara->wifi_key);
  inContext->micoSystemConfig.dhcpEnable = true;

  autoconfig_log("Get SSID: %s, Key: %s", nwkpara->wifi_ssid, nwkpara->wifi_key);
  source = (mico_config_source_t)nwkpara->wifi_retry_interval;
  return;

/*AutoConfig timeout or error*/    
exit:
  autoconfig_log("ERROR, err: %d", err);
  AutoConfigFailed = true;
  mico_system_delegate_config_will_stop();
  mico_rtos_set_semaphore(&AutoConfig_sem);
  return;
}

// Data = AuthData#FTCServer(localIp/netMask/gateWay/dnsServer)
void AutoConfigNotify_AutoConfigGetExtraDataHandler(int datalen, char* data, mico_Context_t * const inContext)
{
  OSStatus err;
  int index;
//  char address[16];
  autoconfig_log_trace();
  uint32_t *ipInfo, ipInfoCount;
  require_action(inContext, exit, err = kParamErr);
  char *debugString;
  struct in_addr ipv4_addr;

  debugString = DataToHexStringWithSpaces( (const uint8_t *)data, datalen );
  autoconfig_log("Get user info: %s", debugString);
  free(debugString);
  
  autoconfig_log("extra data len %d", datalen);
  
  if(source == CONFIG_BY_AIRKISS)
  {
    airkiss_data = data[0];
  }
  else
  {
    for(index = datalen - 1; index>=0; index-- ){
      if(data[index] == '#' &&( (datalen - index) == 5 || (datalen - index) == 25 ) )
        break;
    }
    require_action(index >= 0, exit, err = kParamErr);

    data[index++] = 0x0;
    ipInfo = (uint32_t *)&data[index];
    ipInfoCount = (datalen - index)/sizeof(uint32_t);
    require_action(ipInfoCount >= 1, exit, err = kParamErr);

    if(ipInfoCount == 1){
      inContext->micoSystemConfig.dhcpEnable = true;
      //autoconfig_log("Get auth info: %s, AutoConfig server ip address: %s", data, address);
    }else{
      inContext->micoSystemConfig.dhcpEnable = false;
      ipInfo = (uint32_t *)&data[index];

      ipv4_addr.s_addr = *(ipInfo+1);
      strcpy( (char *) inContext->micoSystemConfig.localIp, inet_ntoa( ipv4_addr ) );

      ipv4_addr.s_addr = *(ipInfo+2);
      strcpy( (char *) inContext->micoSystemConfig.netMask, inet_ntoa( ipv4_addr ) );

      ipv4_addr.s_addr = *(ipInfo+3);
      strcpy( (char *) inContext->micoSystemConfig.gateWay, inet_ntoa( ipv4_addr ) );

      ipv4_addr.s_addr = *(ipInfo+4);
      strcpy( (char *) inContext->micoSystemConfig.dnsServer, inet_ntoa( ipv4_addr ) );

    }

  }
  
  AutoConfigFailed = false;
  
  mico_rtos_set_semaphore(&AutoConfig_sem);
  mico_system_delegate_config_recv_ssid(NULL, NULL);
  return;

exit:
  autoconfig_log("ERROR, err: %d", err);
  mico_system_delegate_config_will_stop( );
  mico_system_power_perform( mico_context, eState_Software_Reset );
}

OSStatus startAutoConfig(  void *app_context_c )
{
  OSStatus err = kUnknownErr;
  app_context = app_context_c;
  mico_context = mico_system_context_get();
  
  AutoConfigClient_fd = -1;
  source = CONFIG_BY_NONE;
  
  err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *)AutoConfigNotify_WifiStatusHandler, mico_context );
  require_noerr(err, exit);
  err = mico_system_notify_register( mico_notify_EASYLINK_WPS_COMPLETED, (void *)AutoConfigNotify_AutoConfigCompleteHandler, mico_context );
  require_noerr(err, exit);
  err = mico_system_notify_register( mico_notify_EASYLINK_GET_EXTRA_DATA, (void *)AutoConfigNotify_AutoConfigGetExtraDataHandler, mico_context );
  require_noerr(err, exit);
  
  // Start the AutoConfig thread
  mico_system_delegate_config_will_start();
  mico_easylink_bonjour_start(Station, mico_context);
  mico_rtos_init_semaphore(&AutoConfig_sem, 1);
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "AutoConfig", AutoConfig_thread, 0x1000, (mico_thread_arg_t)app_context );
  require_noerr_action( err, exit, autoconfig_log("ERROR: Unable to start the AutoConfig thread.") );

exit:
  return err;
}

void _cleanAutoConfigResource( void )
{
  if(AutoConfigClient_fd != -1)
    SocketClose(&AutoConfigClient_fd);

  /*module should power down under default setting*/ 
  mico_system_notify_remove( mico_notify_WIFI_STATUS_CHANGED, (void *)AutoConfigNotify_WifiStatusHandler );
  mico_system_notify_remove( mico_notify_EASYLINK_WPS_COMPLETED, (void *)AutoConfigNotify_AutoConfigCompleteHandler );
  mico_system_notify_remove( mico_notify_EASYLINK_GET_EXTRA_DATA, (void *)AutoConfigNotify_AutoConfigGetExtraDataHandler );
  
  mico_rtos_deinit_semaphore(&AutoConfig_sem);
  AutoConfig_sem = NULL;
  app_context->appConfig->at_config.wifi_config = CONFIG_NONE;
  mico_system_context_update(mico_context);
}

OSStatus stopAutoConfig(  void *app_context_c )
{
  app_context = app_context_c;
  mico_context = mico_system_context_get();
  
  if(app_context->appConfig->at_config.wifi_config == true){
    micoWlanStopEasyLinkPlus();
    if(AutoConfig_sem != NULL){
      _cleanAutoConfigResource( );
    }
    if(AutoConfigClient_fd != -1){
      SocketClose(&AutoConfigClient_fd);
    }
  }
  return kNoErr;
}

void AutoConfig_thread(mico_thread_arg_t inContext)
{
  OSStatus err = kNoErr;
  
  int fd;
  struct sockaddr_in addr;
  int i = 0;
  uint32_t AutoConfig_TimeOut = 0;

  autoconfig_log_trace();
  require_action(AutoConfig_sem, threadexit, err = kNotPreparedErr);

  event_printf("+EVENT=EASYLINK\r\n");
  autoconfig_log("Start AutoConfig mode");
  AutoConfig_TimeOut = app_context->appConfig->at_config.wifi_config_timeout;
  micoWlanStartEasyLinkPlus(AutoConfig_TimeOut);

  mico_rtos_get_semaphore(&AutoConfig_sem, MICO_WAIT_FOREVER);
  
  if(AutoConfigFailed == false){
    event_printf("+EVENT=EASYLINK,1\r\n");
    system_connect_wifi_normal( system_context() );
  }else{
    mico_thread_msleep(20);
    event_printf("+EVENT=EASYLINK,0\r\n");
    goto reboot;
  }
  
  err = mico_rtos_get_semaphore(&AutoConfig_sem, AutoConfig_ConnectWlan_Timeout);
  require_noerr(err, reboot);
  
  mico_thread_sleep(1);
  
  if(source == CONFIG_BY_AIRKISS)
  {
    fd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if (fd < 0)
      goto threadexit;
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_BROADCAST;
    addr.sin_port = htons(10000);

    autoconfig_log("Send UDP to WECHAT");
    while(1){
      sendto(fd, &airkiss_data, 1, 0, (struct sockaddr *)&addr, sizeof(addr));
      
      mico_thread_msleep(10);
      i++;
      if (i > 20)
        break;
    }
  }
  
  if( app_context->appConfig->at_config.wifi_mode == AP ) {
    app_context->appConfig->at_config.wifi_mode = STA;
    mico_system_context_update(mico_context);
  } else if( app_context->appConfig->at_config.wifi_mode == AP_STA ){
    startSoftAP( app_context );
  }
  
  app_context->appStatus.wifi_open.sta_open = true;
  
/*Module is ignored by FTC server, */
threadexit:
  _cleanAutoConfigResource( );
  mico_system_delegate_config_will_stop( );
  SetTimer( 60*1000 , remove_bonjour_for_easylink );
  mico_rtos_delete_thread( NULL );
  return;
  
/*SSID or Password is not correct, module cannot connect to wlan, so reboot and enter AutoConfig again*/
reboot:
  mico_system_power_perform( mico_context, eState_Software_Reset );
  mico_rtos_delete_thread( NULL );
  return;
}


static void _bonjour_generate_txt_record( char *txt_record, WiFi_Interface interface, mico_Context_t * const inContext )
{
  char *temp_txt2 = NULL;
  mico_system_status_wlan_t *wlan_status;
  mico_system_wlan_get_status( &wlan_status );

  temp_txt2 = __strdup_trans_dot(FIRMWARE_REVISION);
  sprintf(txt_record, "FW=%s.", temp_txt2);
  free(temp_txt2);
  
  temp_txt2 = __strdup_trans_dot(HARDWARE_REVISION);
  sprintf(txt_record, "%sHD=%s.", txt_record, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(PROTOCOL);
  sprintf(txt_record, "%sPO=%s.", txt_record, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(wlan_status->rf_version);
  sprintf(txt_record, "%sRF=%s.", txt_record, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(wlan_status->mac);
  sprintf(txt_record, "%sMAC=%s.", txt_record, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(MicoGetVer());
  sprintf(txt_record, "%sOS=%s.", txt_record, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(MODEL);
  sprintf(txt_record, "%sMD=%s.", txt_record, temp_txt2);
  free(temp_txt2);

  temp_txt2 = __strdup_trans_dot(MANUFACTURER);
  sprintf(txt_record, "%sMF=%s.", txt_record, temp_txt2);
  free(temp_txt2);

  if(interface == Soft_AP){
    sprintf(txt_record, "%swlan unconfigured=T.", txt_record);
    sprintf(txt_record, "%sFTC=T.", txt_record);
  }
  else{
    sprintf(txt_record, "%swlan unconfigured=F.", txt_record);
#ifdef MICO_CONFIG_SERVER_ENABLE
    sprintf(txt_record, "%sFTC=T.", txt_record);
#else
    sprintf(txt_record, "%sFTC=F.", txt_record);
#endif
  }
}

static OSStatus mico_easylink_bonjour_start( WiFi_Interface interface, mico_Context_t * const inContext )
{
  char *temp_txt= NULL;
  OSStatus err = kNoErr;
  net_para_st para;
  mdns_init_t init;
  mico_system_status_wlan_t *wlan_status;
  mico_system_wlan_get_status( &wlan_status );

  temp_txt = malloc(500);
  require_action(temp_txt, exit, err = kNoMemoryErr);

  memset(&init, 0x0, sizeof(mdns_init_t));

  micoWlanGetIPStatus(&para, interface);

  init.service_name = "_easylink_config._tcp.local.";

  /*   name#xxxxxx.local.  */
  snprintf( temp_txt, 100, "%s#%c%c%c%c%c%c.local.", MODEL,
            wlan_status->mac[9],  wlan_status->mac[10], wlan_status->mac[12], wlan_status->mac[13], \
            wlan_status->mac[15], wlan_status->mac[16] );

  init.host_name = (char*)__strdup(temp_txt);

  /*   name#xxxxxx.   */
  snprintf( temp_txt, 100, "%s#%c%c%c%c%c%c", MODEL,
            wlan_status->mac[9],  wlan_status->mac[10], wlan_status->mac[12], wlan_status->mac[13], \
            wlan_status->mac[15], wlan_status->mac[16] );

  init.instance_name = (char*)__strdup(temp_txt);

  init.service_port = MICO_CONFIG_SERVER_PORT;

  _bonjour_generate_txt_record( temp_txt, interface, inContext );

  init.txt_record = (char*)__strdup(temp_txt);

  if(interface == Soft_AP)
    mdns_add_record(init, interface, 1500);
  else
    mdns_add_record(init, interface, 10);

  free(init.host_name);
  free(init.instance_name);
  free(init.txt_record);

exit:
  if(temp_txt) free(temp_txt);
  return err;
}

static OSStatus mico_easylink_bonjour_update( WiFi_Interface interface, mico_Context_t * const inContext )
{
  char *temp_txt= NULL;
  OSStatus err = kNoErr;

  temp_txt = malloc(500);
  require_action(temp_txt, exit, err = kNoMemoryErr);

  _bonjour_generate_txt_record( temp_txt, interface, inContext );

  mdns_update_txt_record( "_easylink_config._tcp.local.", interface, temp_txt );

exit:
  if(temp_txt) free(temp_txt);
  return err;
}

static void remove_bonjour_for_easylink(void)
{
  mdns_suspend_record( "_easylink_config._tcp.local.", Station, true );
}


