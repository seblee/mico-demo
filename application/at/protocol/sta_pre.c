#include "mico.h"
#include "MICOAppDefine.h"
#include "system_internal.h"

#define sta_pre_log(M, ...) custom_log("STA", M, ##__VA_ARGS__)

static mico_Context_t *mico_context;
static app_context_t *app_context;
static mico_semaphore_t  scan_semaphore;
static bool should_connect = false;

static void staNotify_ApListCallback(ScanResult *pApList)
{
  int i;
  int j;
  for(i=0; i<pApList->ApNum; i++) {
    sta_pre_log("ssid %s, power %d", pApList->ApList[i].ssid, pApList->ApList[i].ApPower);
    
    for(j=0; j<maxStaSsidlist; j++  ){
      if( strcmp(pApList->ApList[i].ssid, app_context->appConfig->at_config.sta_pre_ssid[j].ssid) == 0 ){
        if( strcmp(mico_context->micoSystemConfig.ssid, app_context->appConfig->at_config.sta_pre_ssid[j].ssid) !=0 ){
          strncpy(mico_context->micoSystemConfig.ssid, app_context->appConfig->at_config.sta_pre_ssid[j].ssid, maxSsidLen);
          strncpy(mico_context->micoSystemConfig.user_key, app_context->appConfig->at_config.sta_pre_ssid[j].key, maxKeyLen);
          mico_context->micoSystemConfig.user_keyLength = strlen(mico_context->micoSystemConfig.user_key);
          should_connect = true;
        }
        mico_rtos_set_semaphore(&scan_semaphore);
        return;
      }
    }
    
  }
}

void sta_pre_ssid_set_manage_thread(mico_thread_arg_t arg)
{
  OSStatus err = kNoErr;
  int i=0;
  bool is_scan = false;
  LinkStatusTypeDef link_status;
  
  should_connect = false;
  
  mico_rtos_init_semaphore(&scan_semaphore, 1);
  
  for(i=0; i<maxStaSsidlist; i++){
    if( strlen(app_context->appConfig->at_config.sta_pre_ssid[i].ssid) >0 ){
      is_scan = true;
      break;
    }
  }
  
  if( is_scan == false ) goto exit;
  
  while(1){
    if( app_context->appStatus.wifi_state.wifi_state_sta != STATION_UP ){
    scan:
      sta_pre_log("sta pre scan");
      mico_system_notify_register(mico_notify_WIFI_SCAN_COMPLETED, (void *)staNotify_ApListCallback, NULL);
      micoWlanStartScan();
      err = mico_rtos_get_semaphore(&scan_semaphore, 5000);
      mico_system_notify_remove(mico_notify_WIFI_SCAN_COMPLETED,(void *)staNotify_ApListCallback);
      if(err != kNoErr){
        sta_pre_log("scan timeout");
        goto scan;
      }else{
        sta_pre_log("scan success");
        micoWlanGetLinkStatus(&link_status);
        if( strcmp((char const*)link_status.ssid, mico_context->micoSystemConfig.ssid) != 0 ){
          if( should_connect == true ){
            system_connect_wifi_normal( system_context() );
          }
        }
        mico_thread_sleep(20);
      }
    }else{
      mico_thread_sleep(1);
    }
  }
exit:
  mico_rtos_deinit_semaphore(&scan_semaphore);
  mico_rtos_delete_thread(NULL);
}

void start_sta_pre_ssid_set_manage(void *app_context_c)
{
  app_context = app_context_c;
  mico_context = mico_system_context_get();
  mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "Sta Pre", sta_pre_ssid_set_manage_thread, 0x500, 0);
}
