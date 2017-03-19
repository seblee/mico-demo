/**
******************************************************************************
* @file    wps.c 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This file provide functions for start a WPS (Wi-Fi protected set-up) 
*          function thread.
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
#include "WPS.h"
#include "system_internal.h"

#define wps_log(M, ...) custom_log("WPS", M, ##__VA_ARGS__)
#define wps_log_trace() custom_log_trace("WPS")

static mico_Context_t *mico_context;
static app_context_t *app_context;
static mico_semaphore_t  wps_sem = NULL;

static void wps_thread(uint32_t inContext);

static bool WpsConfigFailed = false;

void WpsConfigNotify_WifiStatusHandler(WiFiEvent event, mico_Context_t * const inContext)
{
  wps_log_trace();
  IPStatusTypedef para;

  require(inContext, exit);
  switch (event) {
  case NOTIFY_STATION_UP:
    micoWlanGetIPStatus(&para, Station);
    strncpy(inContext->micoSystemConfig.localIp, para.ip, maxIpLen);
    strncpy(inContext->micoSystemConfig.netMask, para.mask, maxIpLen);
    strncpy(inContext->micoSystemConfig.gateWay, para.gate, maxIpLen);
    strncpy(inContext->micoSystemConfig.dnsServer, para.dns, maxIpLen);
    mico_rtos_set_semaphore(&wps_sem);
    break;
  default:
    break;
  }
exit:
  return;
}

void WPSNotify_WPSCompleteHandler(network_InitTypeDef_st *nwkpara, mico_Context_t * const inContext)
{
  OSStatus err;
  wps_log_trace();
  require_action(inContext, exit, err = kParamErr);
  require_action(nwkpara, exit, err = kTimeoutErr);
  
  memcpy(inContext->micoSystemConfig.ssid, nwkpara->wifi_ssid, maxSsidLen);
  inContext->micoSystemConfig.channel = 0;
  inContext->micoSystemConfig.security = SECURITY_TYPE_AUTO;
  memcpy(inContext->micoSystemConfig.user_key, nwkpara->wifi_key, maxKeyLen);
  inContext->micoSystemConfig.user_keyLength = strlen(nwkpara->wifi_key);
  memcpy(inContext->micoSystemConfig.key, nwkpara->wifi_key, maxKeyLen);
  inContext->micoSystemConfig.keyLength = strlen(nwkpara->wifi_key);

  wps_log("Get SSID: %s, Key: %s", inContext->micoSystemConfig.ssid, inContext->micoSystemConfig.user_key);
  mico_system_delegate_config_will_stop();
  WpsConfigFailed = false;
  mico_rtos_set_semaphore(&wps_sem);
  return;

/*WPS timeout*/    
exit:
  wps_log("ERROR, err: %d", err);
  mico_system_delegate_config_will_stop();
  WpsConfigFailed = true;
  
  mico_rtos_set_semaphore(&wps_sem);
  return;
}

void WPSNotify_SYSWillPowerOffHandler(app_context_t * const inContext)
{
  stopWPS(inContext);
}

OSStatus startWPS( void *app_context_c )
{
  OSStatus err = kUnknownErr;
  app_context = (app_context_t *)app_context_c;
  mico_context = mico_system_context_get();
  
  WpsConfigFailed = true;
  
  err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *)WpsConfigNotify_WifiStatusHandler, mico_context );
  require_noerr(err, exit);
  err = mico_system_notify_register( mico_notify_EASYLINK_WPS_COMPLETED, (void *)WPSNotify_WPSCompleteHandler, mico_context );
  require_noerr(err, exit);
  err = mico_system_notify_register( mico_notify_SYS_WILL_POWER_OFF, (void *)WPSNotify_SYSWillPowerOffHandler, app_context );
  require_noerr( err, exit ); 

  // Start the WPS thread
  mico_system_delegate_config_will_start();
  mico_rtos_init_semaphore(&wps_sem, 1);
  err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "WPS", wps_thread, 0x1000, (uint32_t)app_context );
  require_noerr_string( err, exit, "ERROR: Unable to start the WPS thread." );

exit:
  return err;
}


OSStatus stopWPS(  void *app_context_c )
{
  app_context = app_context_c;
  mico_context = mico_system_context_get();
  
  mico_system_notify_remove( mico_notify_WIFI_STATUS_CHANGED, (void *)WpsConfigNotify_WifiStatusHandler );
  mico_system_notify_remove( mico_notify_EASYLINK_WPS_COMPLETED, (void *)WPSNotify_WPSCompleteHandler );
  mico_system_notify_remove( mico_notify_SYS_WILL_POWER_OFF, (void *)WPSNotify_SYSWillPowerOffHandler );

  if(wps_sem){
    mico_rtos_deinit_semaphore(&wps_sem);
    wps_sem = NULL;
  }

  micoWlanStopWPS();
  
  app_context->appConfig->at_config.wifi_config = CONFIG_NONE;
  mico_system_context_update(mico_context);
  
  return kNoErr;
}

void wps_thread(uint32_t inContext)
{
  app_context = (app_context_t *)inContext;
  wps_log_trace();
  OSStatus err = kNoErr;
  uint32_t WPS_TimeOut= 0;

  require_action(wps_sem, exit, err = kNotPreparedErr);

  WPS_TimeOut = app_context->appConfig->at_config.wifi_config_timeout;

  micoWlanStartWPS(WPS_TimeOut);
  event_printf("+EVENT=WPS\r\n");
  wps_log("Start WPS configuration");
  mico_rtos_get_semaphore(&wps_sem, MICO_WAIT_FOREVER);

  mico_thread_msleep(20);
  if( WpsConfigFailed == false ){
    event_printf("+EVENT=WPS,1\r\n");
    system_connect_wifi_normal( system_context() );
  }else{
    event_printf("+EVENT=WPS,0\r\n");
    stopWPS(app_context);
    goto reboot;
  }
  
  err = mico_rtos_get_semaphore(&wps_sem, WpsConfig_ConnectWlan_Timeout);
  require_noerr(err, reboot);
  
  if( app_context->appConfig->at_config.wifi_mode == AP ) {
    app_context->appConfig->at_config.wifi_mode = STA;
    mico_system_context_update(mico_context);
  } else if( app_context->appConfig->at_config.wifi_mode == AP_STA ){
    startSoftAP( app_context );
  }
  
  app_context->appConfig->at_config.wifi_config = CONFIG_NONE;
  mico_system_context_update(mico_context);

exit:
  if(err) wps_log("WPS thread exit, err = %d", err);
  stopWPS(app_context);
  mico_rtos_delete_thread(NULL);
  return;
  
reboot:
  mico_system_power_perform( mico_context, eState_Software_Reset );
  mico_rtos_delete_thread( NULL );
}



