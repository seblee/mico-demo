#include "mico.h"
#include "MICOAppDefine.h"
#include "protocol.h"
#include "at_cmd.h"
#include "stdarg.h"
#include "SppProtocol.h"
#include "system_internal.h"

#define AT_HEADER		        "AT"
#define AT_HEADER_CMD			"AT+"
#define MAX_SINGLE_PARAMETER 	        (50)
#define MAX_RECV_BUFFER_SIZE            (1024*2)

static mico_Context_t *mico_context;
static app_context_t *app_context;
static bool should_del_at;

static void responseHELP(int argc ,char *argv[]);
static void responseFHELP(int argc ,char *argv[]);

/******************************************************************************/
/***********           Base AT command process interface           ************/
/******************************************************************************/

static void responseFMVER(int argc ,char *argv[])
{
  char ver[sysVersionLenth];
  
  if(argc == 1){
    memset(ver, 0, sizeof(ver));
    get_firmware_version(ver);
    at_printf("+OK=%s\r\n", ver);
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseSYSTIME(int argc ,char *argv[])
{
  if(argc == 1){
    at_printf("+OK=%d\r\n", mico_rtos_get_time());
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseSAVE(int argc ,char *argv[])
{
  if(argc == 1){
    if (mico_system_context_update(mico_context) != kNoErr) {
      at_printf("+ERR=%d\r\n", INVALID_OPERATION);
      return;
    }
    at_printf("+OK\r\n");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseFACTORY(int argc ,char *argv[])
{
  if(argc == 1){
    if (mico_system_context_restore(mico_context) != kNoErr) {
      at_printf("+ERR=%d\r\n", INVALID_OPERATION);
      return;
    }
    at_printf("+OK\r\n");
    mico_system_power_perform( mico_context, eState_Software_Reset );
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseREBOOT(int argc ,char *argv[])
{
  if(argc == 1){
    at_printf("+OK\r\n");
    mico_system_power_perform( mico_context, eState_Software_Reset );
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseEVENT(int argc ,char *argv[])
{
  if(argc == 1){
    if(app_context->appConfig->at_config.event == true){
      at_printf("+OK=ON\r\n");
    }else{
      at_printf("+OK=OFF\r\n");
    }
  }else if(argc == 2){
    if(!strcmp(argv[1], "ON")){
      app_context->appConfig->at_config.event = true;
    }else if(!strcmp(argv[1], "OFF")){
      app_context->appConfig->at_config.event = false;
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    mico_system_context_update(mico_context);
    at_printf("+OK\r\n");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseECHO(int argc ,char *argv[])
{
  if(argc == 1){
    if(app_context->appConfig->at_config.at_echo == true){
      at_printf("+OK=ON\r\n");
    }else{
      at_printf("+OK=OFF\r\n");
    }
  }else if(argc == 2){
    if(!strcmp(argv[1], "ON")){
      app_context->appConfig->at_config.at_echo = true;
      app_context->appStatus.at_echo = true;
    }else if(!strcmp(argv[1], "OFF")){
      app_context->appConfig->at_config.at_echo = false;
      app_context->appStatus.at_echo = false;
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    mico_system_context_update(mico_context);
    at_printf("+OK\r\n");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}


static void responseUART(int argc ,char *argv[])
{
  uint32_t baudrate;
  
  if(argc == 1) {
    at_printf("+OK=%d", app_context->appConfig->at_config.serial_config.baud_rate);
    if(app_context->appConfig->at_config.serial_config.data_width == DATA_WIDTH_5BIT){
      at_printf(",5");
    } else if(app_context->appConfig->at_config.serial_config.data_width == DATA_WIDTH_6BIT) {
      at_printf(",6");
    } else if(app_context->appConfig->at_config.serial_config.data_width == DATA_WIDTH_7BIT) {
      at_printf(",7");
    }else if(app_context->appConfig->at_config.serial_config.data_width == DATA_WIDTH_8BIT) {
      at_printf(",8");
    }else if(app_context->appConfig->at_config.serial_config.data_width == DATA_WIDTH_9BIT) {
      at_printf(",9");
    }
    
    if(app_context->appConfig->at_config.serial_config.parity == NO_PARITY) {
      at_printf(",NONE");
    }else if(app_context->appConfig->at_config.serial_config.parity == ODD_PARITY) {
      at_printf(",ODD");
    }else if(app_context->appConfig->at_config.serial_config.parity == EVEN_PARITY) {
      at_printf(",EVEN");
    }
    
    if(app_context->appConfig->at_config.serial_config.stop_bits == STOP_BITS_1) {
      at_printf(",1");
    }else if(app_context->appConfig->at_config.serial_config.stop_bits == STOP_BITS_2) {
      at_printf(",2");
    }
    
    if(app_context->appConfig->at_config.serial_config.flow_control == FLOW_CONTROL_DISABLED) {
      at_printf(",NONE");
    } else if(app_context->appConfig->at_config.serial_config.flow_control == FLOW_CONTROL_CTS) {
      at_printf(",CTS");
    } else if(app_context->appConfig->at_config.serial_config.flow_control == FLOW_CONTROL_RTS) {
      at_printf(",RTS");
    } else if(app_context->appConfig->at_config.serial_config.flow_control == FLOW_CONTROL_CTS_RTS) {
      at_printf(",CTSRTS");
    }
    at_printf("\r\n");
  }else if (argc == 6){
    baudrate = atoi(argv[1]);
    if((baudrate == UART_BANDRATE_9600) || (baudrate == UART_BANDRATE_19200) || (baudrate == UART_BANDRATE_38400)
       || (baudrate == UART_BANDRATE_57600) || (baudrate == UART_BANDRATE_115200) || (baudrate == UART_BANDRATE_230400)
         || (baudrate == UART_BANDRATE_460800) || (baudrate == UART_BANDRATE_921600) || (baudrate == UART_BANDRATE_1843200)
           || (baudrate == UART_BANDRATE_3686400)){
             app_context->appConfig->at_config.serial_config.baud_rate = baudrate;
           }else {
             at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
             return;
           }
    
    if(!strcmp(argv[2], "5")) {
      app_context->appConfig->at_config.serial_config.data_width = DATA_WIDTH_5BIT;
    } else if(!strcmp(argv[2], "6")) {
      app_context->appConfig->at_config.serial_config.data_width = DATA_WIDTH_6BIT;
    }else if(!strcmp(argv[2], "7")) {
      app_context->appConfig->at_config.serial_config.data_width = DATA_WIDTH_7BIT;
    }else if(!strcmp(argv[2], "8")) {
      app_context->appConfig->at_config.serial_config.data_width = DATA_WIDTH_8BIT;
    }else if(!strcmp(argv[2], "9")) {
      app_context->appConfig->at_config.serial_config.data_width = DATA_WIDTH_9BIT;
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    
    if(!strcmp(argv[3], "NONE")) {
      app_context->appConfig->at_config.serial_config.parity = NO_PARITY;
    }else if(!strcmp(argv[3], "ODD")) {
      app_context->appConfig->at_config.serial_config.parity = ODD_PARITY;
    }else if(!strcmp(argv[3], "EVEN")) {
      app_context->appConfig->at_config.serial_config.parity = EVEN_PARITY;
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    
    if(!strcmp(argv[4], "1")) {
      app_context->appConfig->at_config.serial_config.stop_bits = STOP_BITS_1;
    }else if(!strcmp(argv[4], "2")) {
      app_context->appConfig->at_config.serial_config.stop_bits = STOP_BITS_2;
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    
    if(!strcmp(argv[5], "NONE")){
      app_context->appConfig->at_config.serial_config.flow_control = FLOW_CONTROL_DISABLED;
    }else if(!strcmp(argv[5], "CTS")){
      app_context->appConfig->at_config.serial_config.flow_control = FLOW_CONTROL_CTS;
    }else if(!strcmp(argv[5], "RTS")){
      app_context->appConfig->at_config.serial_config.flow_control = FLOW_CONTROL_RTS;
    }else if(!strcmp(argv[5], "CTSRTS")){
      app_context->appConfig->at_config.serial_config.flow_control = FLOW_CONTROL_CTS_RTS;
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK\r\n");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseUARTF(int argc ,char *argv[])
{
  if(argc == 1){
    if(app_context->appConfig->at_config.autoFormatEnable == true){
      at_printf("+OK=ON\r\n");
    }else{
      at_printf("+OK=OFF\r\n");
    }
  }else if(argc == 2){
    if(!strcmp(argv[1], "ON")) {
      app_context->appConfig->at_config.autoFormatEnable= true;
    }else if(!strcmp(argv[1], "OFF")) {
      app_context->appConfig->at_config.autoFormatEnable = false;
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
    }
    at_printf("+OK\r\n");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseUARTFL(int argc ,char *argv[])
{
  
  if(argc == 1){
    at_printf("+OK=%d\r\n", app_context->appConfig->at_config.uart_auto_format.auto_format_len);
  }else if(argc == 2){
    if(str_is_digit(argv[1], 1024, 10) == 0){
      app_context->appConfig->at_config.uart_auto_format.auto_format_len = atoi(argv[1]);
      mico_system_context_update(mico_context);;
      at_printf("+OK\r\n");
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
    }
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseUARTFT(int argc ,char *argv[])
{

  if(argc == 1){
    at_printf("+OK=%d\r\n", app_context->appConfig->at_config.uart_auto_format.auto_format_time);
  }else if(argc == 2){
    if(str_is_digit(argv[1], 1000, 10) == 0){
      app_context->appConfig->at_config.uart_auto_format.auto_format_time = atoi(argv[1]);
      mico_system_context_update(mico_context);;
      at_printf("+OK\r\n");
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
    }
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responsePMSLP(int argc ,char *argv[])
{
  if(argc == 1){
    if( mico_context->micoSystemConfig.mcuPowerSaveEnable == true )
      at_printf("+OK=ON\r\n");
    else{
      at_printf("+OK=OFF\r\n");
    }
  }else if(argc == 2){
    if(!strcmp(argv[1], "ON")){
      if(mico_context->micoSystemConfig.mcuPowerSaveEnable == false){
        mico_context->micoSystemConfig.mcuPowerSaveEnable = true;
        mico_system_context_update(mico_context);;
        MicoMcuPowerSaveConfig(true);
      }
    }else if(!strcmp(argv[1], "OFF")){
      if(mico_context->micoSystemConfig.mcuPowerSaveEnable == true){
        mico_context->micoSystemConfig.mcuPowerSaveEnable = false;
        mico_system_context_update(mico_context);;
        MicoMcuPowerSaveConfig(false);
      }
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responsePRSLP(int argc ,char *argv[])
{
  if(argc == 1){
    if(mico_context->micoSystemConfig.rfPowerSaveEnable== true)
      at_printf("+OK=ON\r\n");
    else{
      at_printf("+OK=OFF\r\n");
    }
  }else if(argc == 2){
    if(!strcmp(argv[1], "ON")){
      if(mico_context->micoSystemConfig.rfPowerSaveEnable == false){
        mico_context->micoSystemConfig.rfPowerSaveEnable = true;
        mico_system_context_update(mico_context);;
        micoWlanEnablePowerSave();
      }
    }else if(!strcmp(argv[1], "OFF")){
      if(mico_context->micoSystemConfig.rfPowerSaveEnable == true){
        mico_context->micoSystemConfig.rfPowerSaveEnable = false;
        mico_system_context_update(mico_context);;
        micoWlanDisablePowerSave();
      }
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
  
}

static void responseWFVER(int argc ,char *argv[])
{
  char ver[64];
  int ret;
  
  if(argc == 1){
    memset(ver, 0, sizeof(ver));
    ret = MicoGetRfVer(ver, sizeof(ver));
    if (ret == 0){
      at_printf("+OK=%s\r\n", ver);
    }else{
      at_printf("+ERR=%d\r\n", INVALID_OPERATION);
      return;
    }
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseWMAC(int argc ,char *argv[])
{
    mico_system_status_wlan_t *wlan_status;
    mico_system_wlan_get_status( &wlan_status );

  if(argc == 1){
    at_printf("+OK=%s\r\n", wlan_status->mac);
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseWGHBN(int argc ,char *argv[])
{
    struct hostent* hostent_content = NULL;
    char **pptr = NULL;
    struct in_addr in_addr;
  char ipstr[16];

  if( argc == 2 ){
      hostent_content = gethostbyname((char *)argv[1]);
      if( hostent_content != NULL ){
      pptr=hostent_content->h_addr_list;
      in_addr.s_addr = *(uint32_t *)(*pptr);
      strcpy( ipstr, inet_ntoa(in_addr));
      at_printf("+OK=%s\r\n", ipstr);
      }else{
          at_printf("+ERR=%d\r\n", INVALID_OPERATION);
      }
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}
 
static mico_semaphore_t  scan_semaphore;

void ATNotify_ApListCallback(ScanResult *pApList)
{
  int i;
  at_printf("+OK=\r\n");
  for(i=0; i<pApList->ApNum; i++) {
    at_printf("    %s,%d\r\n", pApList->ApList[i].ssid, pApList->ApList[i].ApPower);
  }
  mico_rtos_set_semaphore(&scan_semaphore);
}


static void responseWSCAN(int argc ,char *argv[])
{
  OSStatus err = kNoErr;
  if(argc == 1){
    mico_rtos_init_semaphore(&scan_semaphore, 1);
    mico_system_notify_register(mico_notify_WIFI_SCAN_COMPLETED, (void *)ATNotify_ApListCallback, NULL);
    micoWlanStartScan();
    err = mico_rtos_get_semaphore(&scan_semaphore, 5000);
    if(err != kNoErr){
      at_printf("+ERR=%d\r\n", INVALID_OPERATION);
    }
    mico_rtos_deinit_semaphore(&scan_semaphore);
    mico_system_notify_remove(mico_notify_WIFI_SCAN_COMPLETED,(void *)ATNotify_ApListCallback);
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseWMODE(int argc ,char *argv[])
{
  if(argc == 1){
    if(app_context->appConfig->at_config.wifi_mode == AP){
      at_printf("+OK=AP\r\n");
    }else if(app_context->appConfig->at_config.wifi_mode == STA){
      at_printf("+OK=STA\r\n");
    }else{
      at_printf("+OK=AP_STA\r\n");
    }
  }else if(argc == 2){
    if(!strcmp(argv[1], "AP")){
      app_context->appConfig->at_config.wifi_mode = AP;
    }else if(!strcmp(argv[1], "STA")){
      app_context->appConfig->at_config.wifi_mode = STA;
    }else if(!strcmp(argv[1], "AP_STA")){
      app_context->appConfig->at_config.wifi_mode = AP_STA;
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK\r\n");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseWAP(int argc ,char *argv[])
{
  if(argc == 1){
    at_printf("+OK=%s,%s\r\n", app_context->appConfig->at_config.ap_config.ssid, 
              app_context->appConfig->at_config.ap_config.key);
  }else if(argc == 3){
    if(strlen(argv[1]) <= maxSsidLen){
      strncpy(app_context->appConfig->at_config.ap_config.ssid, argv[1], maxSsidLen);
      if(strlen(argv[2]) < 8){
        strncpy(app_context->appConfig->at_config.ap_config.key, "", maxKeyLen);
      }else if( (strlen(argv[2]) >= 8) && (strlen(argv[2]) <= maxKeyLen) ){
        strncpy(app_context->appConfig->at_config.ap_config.key, argv[2], maxKeyLen);
      }else{
        at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
        return;        
      }
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK\r\n");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseWAPCH(int argc ,char *argv[])
{
  LinkStatusTypeDef link_state;
  if(argc == 1){
    if(micoWlanGetLinkStatus(&link_state) != kNoErr){
      at_printf("+ERR=%d\r\n", INVALID_OPERATION);
      return;
    }
    at_printf("+OK=%d\r\n", link_state.channel);
  }else if(argc == 2){
    if(str_is_digit(argv[1], 13, 0) == 0) {
      app_context->appConfig->at_config.ap_config.channel = atoi(argv[1]);
      at_printf("+OK\r\n");
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
    }
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseWSTA(int argc ,char *argv[])
{
  if(argc == 1){
    at_printf("+OK=%s,%s\r\n", mico_context->micoSystemConfig.ssid,
              mico_context->micoSystemConfig.user_key);
  }else if(argc == 3){
    strncpy(mico_context->micoSystemConfig.ssid, argv[1], maxSsidLen);
    strncpy(mico_context->micoSystemConfig.key, argv[2], maxKeyLen);
    strncpy(mico_context->micoSystemConfig.user_key, argv[2], maxKeyLen);
    mico_context->micoSystemConfig.keyLength = strlen(mico_context->micoSystemConfig.key);
    mico_context->micoSystemConfig.user_keyLength = strlen(mico_context->micoSystemConfig.user_key);
    at_printf("+OK\r\n");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}
static void responseWPRESTA(int argc ,char *argv[])
{
  int num = 0;
  num = atoi(argv[1]);
  if(argc == 2){
    if( num <= 0 || num > maxStaSsidlist ){
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    num = num - 1;
    at_printf("+OK=%s,%s\r\n", app_context->appConfig->at_config.sta_pre_ssid[num].ssid,
              app_context->appConfig->at_config.sta_pre_ssid[num].key);
  }else if(argc == 4){
    if(  num <= 0 || num > maxStaSsidlist ){
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    num = num - 1;
    strncpy(app_context->appConfig->at_config.sta_pre_ssid[num].ssid, argv[2], maxSsidLen);
    strncpy(app_context->appConfig->at_config.sta_pre_ssid[num].key, argv[3], maxKeyLen);
    at_printf("+OK\r\n");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseWLANF(int argc ,char *argv[])
{
  if(argc == 1){
    at_printf("+OK=");
    if(app_context->appStatus.wifi_open.ap_open == true){
      at_printf("AP,ON");
    }else{
      at_printf("AP,OFF");
    }
    if(app_context->appStatus.wifi_open.sta_open == true){
      at_printf(",STA,ON\r\n");
    }else{
      at_printf(",STA,OFF\r\n");
    }
  }else if(argc == 3){
    if(!strcmp(argv[1], "AP")){
      if(!strcmp(argv[2], "ON")){
        if(app_context->appStatus.wifi_open.ap_open == false ){
          startSoftAP(app_context);
          app_context->appStatus.wifi_open.ap_open = true;
        }else{
          at_printf("+ERR=%d\r\n", INVALID_OPERATION);
          return;
        }
      }else if(!strcmp(argv[2], "OFF")){
        if(app_context->appStatus.wifi_open.ap_open == true ){
          micoWlanSuspendSoftAP();
          app_context->appStatus.wifi_open.ap_open = false;
        }else{
          at_printf("+ERR=%d\r\n", INVALID_OPERATION);
          return;
        }
      }else{
        at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
        return;
      }
      at_printf("+OK\r\n");
    }else if(!strcmp(argv[1], "STA")){
      if(!strcmp(argv[2], "ON")){
        if(app_context->appStatus.wifi_open.sta_open == false ){
          system_connect_wifi_fast(system_context());
          app_context->appStatus.wifi_open.sta_open = true;
        }else{
          at_printf("+ERR=%d\r\n", INVALID_OPERATION);
          return;
        }
      }else if(!strcmp(argv[2], "OFF")){
        if(app_context->appStatus.wifi_open.sta_open == true ){
          micoWlanSuspendStation();
          app_context->appStatus.wifi_open.sta_open = false;
        }else{
          at_printf("+ERR=%d\r\n", INVALID_OPERATION);
          return;
        }
      }else{
        at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
        return;
      }
      at_printf("+OK\r\n");
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
    }
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseWSTATUS(int argc ,char *argv[])
{
  if(argc == 1){
    if(app_context->appConfig->at_config.wifi_config != CONFIG_NONE){
      at_printf("+OK=CONFIG\r\n");
    }else{
      if(app_context->appStatus.wifi_state.wifi_state_ap == UAP_UP){
        at_printf("+OK=UAP_UP,");
      }else if(app_context->appStatus.wifi_state.wifi_state_ap == UAP_DOWN){
        at_printf("+OK=UAP_DOWN,");
      }
      if(app_context->appStatus.wifi_state.wifi_state_sta == STATION_UP){
        at_printf("STATION_UP\r\n");
      }else if(app_context->appStatus.wifi_state.wifi_state_sta == STATION_DOWN){
        at_printf("STATION_DOWN\r\n");
      }
    }
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseWLINK(int argc ,char *argv[])
{
  LinkStatusTypeDef link_state;
  if(argc == 1){
    if(micoWlanGetLinkStatus(&link_state) != kNoErr){
      at_printf("+ERR=%d\r\n", INVALID_OPERATION);
      return;
    }
    at_printf("+OK=%d,%d,%d\r\n", link_state.is_connected, link_state.wifi_strength, link_state.channel);
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseIPCONFIG(int argc ,char *argv[])
{
    mico_system_status_wlan_t *wlan_status;
    mico_system_wlan_get_status( &wlan_status );

  if(argc == 1){
    at_printf("+OK=%s,%s,%s,%s,%s,%s\r\n", app_context->appConfig->at_config.ap_ip_config.localIp,
                app_context->appConfig->at_config.ap_ip_config.netMask,
                app_context->appConfig->at_config.ap_ip_config.gateWay,
                wlan_status->localIp, wlan_status->netMask, wlan_status->gateWay);
  }else if(argc == 5){
    if(!strcmp(argv[1], "AP")){
      if( (is_valid_ip(argv[2]) == 0) && (is_valid_ip(argv[3]) == 0) && (is_valid_ip(argv[4]) == 0) ){
        strcpy(app_context->appConfig->at_config.ap_ip_config.localIp, argv[2]);
        strcpy(app_context->appConfig->at_config.ap_ip_config.netMask, argv[3]);
        strcpy(app_context->appConfig->at_config.ap_ip_config.gateWay, argv[4]);
      }else{
        at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
        return;
      }
    }else if(!strcmp(argv[1], "STA")){
      if( (is_valid_ip(argv[2]) == 0) && (is_valid_ip(argv[3]) == 0) && (is_valid_ip(argv[4]) == 0) ){
        strcpy(mico_context->micoSystemConfig.localIp, argv[2]);
        strcpy(mico_context->micoSystemConfig.netMask, argv[3]);
        strcpy(mico_context->micoSystemConfig.gateWay, argv[4]);
      }else{
        at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
        return;
      }
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK\r\n");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseDHCP(int argc ,char *argv[])
{
  if(argc == 1){
    if(mico_context->micoSystemConfig.dhcpEnable == true){
      at_printf("+OK=ON\r\n");  
    }else{
      at_printf("+OK=OFF\r\n");
    }
  }else if(argc == 2){
    if(!strcmp(argv[1], "ON")){
      mico_context->micoSystemConfig.dhcpEnable = true;
    }else if(!strcmp(argv[1], "OFF")){
      mico_context->micoSystemConfig.dhcpEnable = false;
    } else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK\r\n");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseOCFG(int argc ,char *argv[])
{
  if(argc == 2){
     if(!strcmp(argv[1], "EASYLINK")){
       app_context->appConfig->at_config.wifi_config = CONFIG_EASYLINK;
     }else if(!strcmp(argv[1], "WPS")){
       app_context->appConfig->at_config.wifi_config = CONFIG_WPS;
     }else{
       at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
       return;
     }
    at_printf("+OK\r\n"); 
    mico_system_context_update(mico_context);
    mico_system_power_perform( mico_context, eState_Software_Reset );
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseOCFGT(int argc ,char *argv[])
{
  if(argc == 1){
    at_printf("+OK=%d\r\n", app_context->appConfig->at_config.wifi_config_timeout);
  }else if(argc == 2){
    if(str_is_digit(argv[1], 4294967295, 0) == 0) {
      app_context->appConfig->at_config.wifi_config_timeout = atoi(argv[1]);
      mico_system_context_update(mico_context);;
      at_printf("+OK\r\n");
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
    }
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

//static void responseOCFGS(int argc ,char *argv[])
//{
//  if(argc == 1){
//    if(app_context->appConfig->at_config.wifi_config == CONFIG_NONE) {
//      at_printf("+ERR=%d\r\n", INVALID_OPERATION);
//    }else{
//      app_context->appConfig->at_config.wifi_config = CONFIG_NONE;
//      at_printf("+OK\r\n");
//      mico_system_context_update(mico_context);;
//      app_context->micoStatus.sys_state = eState_Software_Reset;
//      mico_rtos_set_semaphore(&app_context->micoStatus.sys_state_change_sem);
//    }
//  }else{
//    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
//  }
//}

static void responseCON1(int argc ,char *argv[])
{
  if (argc == 1) {
    at_printf("+OK=");
    if(app_context->appConfig->at_config.conn1_config.protocol == TCP_SERVER) {
      at_printf("SERVER");
    }else if(app_context->appConfig->at_config.conn1_config.protocol == TCP_CLIENT){
      at_printf("CLIENT");
    }else if(app_context->appConfig->at_config.conn1_config.protocol == UDP_UNICAST){
      at_printf("UNICAST");
    }else if(app_context->appConfig->at_config.conn1_config.protocol == UDP_BOARDCAST){
      at_printf("BOARDCAST");
    }else if(app_context->appConfig->at_config.conn1_config.protocol == CLOUD_FOG){
      at_printf("FOG");
    }
    at_printf(",%d,%d",app_context->appConfig->at_config.conn1_config.localPort,
              app_context->appConfig->at_config.conn1_config.remotePort);
    at_printf(",%s\r\n", app_context->appConfig->at_config.conn1_config.remoteServerDomain);
  } else if (argc == 5){ 
    if (!strcmp(argv[1], "SERVER")) {
      app_context->appConfig->at_config.conn1_config.protocol = TCP_SERVER;
    } else if (!strcmp(argv[1], "CLIENT")) {
      app_context->appConfig->at_config.conn1_config.protocol = TCP_CLIENT;
    } else if (!strcmp(argv[1], "UNICAST")) {
      app_context->appConfig->at_config.conn1_config.protocol = UDP_UNICAST;
    } else if (!strcmp(argv[1], "BOARDCAST")) {
      app_context->appConfig->at_config.conn1_config.protocol = UDP_BOARDCAST;
    } else if (!strcmp(argv[1], "FOG")) {
      app_context->appConfig->at_config.conn1_config.protocol = CLOUD_FOG;
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    if (str_is_digit(argv[2], 65535, 0) == 0) {
       app_context->appConfig->at_config.conn1_config.localPort = atoi(argv[2]);
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    if (str_is_digit(argv[3], 65535, 0) == 0) {
      app_context->appConfig->at_config.conn1_config.remotePort = atoi(argv[3]);
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    strncpy(app_context->appConfig->at_config.conn1_config.remoteServerDomain, argv[4], maxRemoteServerLenth);
    at_printf("+OK\r\n");
  } else {
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseCON2F(int argc ,char *argv[])
{
  if(argc == 1){
    if(app_context->appConfig->at_config.conn2_enable == true){
      at_printf("+OK=ON\r\n");
    }else{
      at_printf("+OK=OFF\r\n");
    }
  }else if(argc == 2){
    if(!strcmp(argv[1], "ON")){
      app_context->appConfig->at_config.conn2_enable = true;
    }else if(!strcmp(argv[1], "OFF")){
      app_context->appConfig->at_config.conn2_enable = false;
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK\r\n");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseCON2(int argc ,char *argv[])
{
  if (argc == 1) {
    at_printf("+OK=");
    if(app_context->appConfig->at_config.conn2_config.protocol == TCP_SERVER) {
      at_printf("SERVER");
    }else if(app_context->appConfig->at_config.conn2_config.protocol == TCP_CLIENT){
      at_printf("CLIENT");
    }else if(app_context->appConfig->at_config.conn2_config.protocol == UDP_UNICAST){
      at_printf("UNICAST");
    }else if(app_context->appConfig->at_config.conn2_config.protocol == UDP_BOARDCAST){
      at_printf("BOARDCAST");
    }
    at_printf(",%d,%d", app_context->appConfig->at_config.conn2_config.localPort,
              app_context->appConfig->at_config.conn2_config.remotePort);
    at_printf(",%s\r\n", app_context->appConfig->at_config.conn2_config.remoteServerDomain);  
  } else if ((argc == 5)){ //&& str_is_digit(argv[2] && str_is_digit(argv[3])) {
    if (!strcmp(argv[1], "SERVER")) {
      app_context->appConfig->at_config.conn2_config.protocol = TCP_SERVER;
    } else if (!strcmp(argv[1], "CLIENT")) {
      app_context->appConfig->at_config.conn2_config.protocol = TCP_CLIENT;
    } else if (!strcmp(argv[1], "UNICAST")) {
      app_context->appConfig->at_config.conn2_config.protocol = UDP_UNICAST;
    } else if (!strcmp(argv[1], "BOARDCAST")) {
      app_context->appConfig->at_config.conn2_config.protocol = UDP_BOARDCAST;
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    if (str_is_digit(argv[2], 65535, 0) == 0) {
      app_context->appConfig->at_config.conn2_config.localPort = atoi(argv[2]);
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    if (str_is_digit(argv[3], 65535, 0) == 0) {
      app_context->appConfig->at_config.conn2_config.remotePort = atoi(argv[3]);
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    strncpy(app_context->appConfig->at_config.conn2_config.remoteServerDomain, argv[4], maxRemoteServerLenth);
    at_printf("+OK\r\n");
  } else {
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseCONSN(int argc ,char *argv[])
{
  if( argc == 2 ){
    if( !strcmp(argv[1], "1") ){
      at_printf("+OK=1,%d\r\n", app_context->appConfig->at_config.conn1_config.server_num);
    }else if( !strcmp(argv[1], "2") ){
      at_printf("+OK=2,%d\r\n", app_context->appConfig->at_config.conn2_config.server_num);
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
    }
  }else if( argc == 3 ){
    if( !strcmp(argv[1], "1") ){
      if( str_is_digit(argv[2], 5, 0) == 0 ){
        app_context->appConfig->at_config.conn1_config.server_num = atoi(argv[2]);
      }else{
        at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
        return;
      }
    }else if( !strcmp(argv[1], "2") ){
      if( str_is_digit(argv[2], 5, 0) == 0 ){
        app_context->appConfig->at_config.conn2_config.server_num = atoi(argv[2]);
      }else{
        at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
        return;
      }      
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK\r\n");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseCONF(int argc ,char *argv[])
{
  if( argc == 2 ){
    if( !strcmp(argv[1], "1") ){
      if( app_context->appStatus.connect_state.connect1_open == true ){
        at_printf("+OK=1,ON\r\n");
      }else{
        at_printf("+OK=1,OFF\r\n");
      }
    }else if( !strcmp(argv[1], "2") ){
      if( app_context->appStatus.connect_state.connect2_open == true ){
        at_printf("+OK=2,ON\r\n");
      }else{
        at_printf("+OK=2,OFF\r\n");
      }
    }
  }else if( argc == 3 ){
    if( !strcmp(argv[1], "1") ){
      if( !strcmp(argv[2], "ON") ){
        if( start_tcp_ip(app_context, 1) != kNoErr ){
          at_printf("+ERR=%d\r\n", INVALID_OPERATION);
          return;
        }
      }else if( !strcmp(argv[2], "OFF") ){
        if( stop_tcp_ip(app_context, 1) != kNoErr ){
          at_printf("+ERR=%d\r\n", INVALID_OPERATION);
          return;
        }
      }else{
        at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
        return;
      }
      at_printf("+OK\r\n");
    }else if( !strcmp(argv[1], "2") ){
      if( !strcmp(argv[2], "ON") ){
        if( start_tcp_ip(app_context, 2) != kNoErr ){
          at_printf("+ERR=%d\r\n", INVALID_OPERATION);
          return;
        }
      }else if( !strcmp(argv[2], "OFF") ){
        if( stop_tcp_ip(app_context, 2) != kNoErr ){
          at_printf("+ERR=%d\r\n", INVALID_OPERATION);
          return;
        }
      }else{
        at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
        return;
      }
      at_printf("+OK\r\n");
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
    }
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseCONS(int argc ,char *argv[])
{
  if(argc == 1){
    at_printf("+OK=%d,%d,%d,%d\r\n", app_context->appStatus.connect_state.connect1_server_link_count,
              app_context->appStatus.connect_state.connect1_client_link,
              app_context->appStatus.connect_state.connect2_server_link_count,
              app_context->appStatus.connect_state.connect2_client_link);
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseSSEND(int argc ,char *argv[])
{
  at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  return;
}


static void responseSSSEND(int argc ,char *argv[])
{
  at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  return;
}


static void responseSUNSEND(int argc ,char *argv[])
{
  at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  return;
}

static void socket_cmd_send(int socket, int lenth, char *buf)
{
  cmd_data_send_t *cmd_data_rec;
  uint8_t *tempbuf = NULL;
  
  if( socket !=0 ){
    if(cmd_socket_check(socket) != 0){
      at_printf("+ERR=%d\r\n", INVALID_SOCKET);
      return;
    }
  }
  
  tempbuf = malloc(lenth);
  memcpy(tempbuf, buf, lenth);
  memset(buf, 0x00, MAX_RECV_BUFFER_SIZE);
  cmd_data_rec = (cmd_data_send_t *) buf;
  cmd_data_rec->cmd_send_type = CMD_DATA;
  cmd_data_rec->socket = socket;
  cmd_data_rec->lenth = lenth;
  memcpy(cmd_data_rec->buf, tempbuf, lenth);
  lenth = sizeof(cmd_data_send_t) - 4 + lenth;
  sppUartCommandProcess((uint8_t *)cmd_data_rec, lenth, app_context);
  free(tempbuf);
  return;
}

static void socket_unicast_send(int socket, int port, char *ip, int lenth, char *buf)
{
  udp_cmd_send_t *udp_cmd_rec;
  uint8_t *tempbuf = NULL;
  
  if( socket !=0 ){
    if(cmd_socket_check(socket) != 0){
      at_printf("+ERR=%d\r\n", INVALID_SOCKET);
      return;
    }
  }
  tempbuf = malloc(lenth);
  memcpy(tempbuf, buf, lenth);
  memset(buf, 0x00, MAX_RECV_BUFFER_SIZE);
  udp_cmd_rec = (udp_cmd_send_t *) buf;
  udp_cmd_rec->cmd_send_type = CMD_UNICAST;
  udp_cmd_rec->socket = socket;
  udp_cmd_rec->port = port;
  strcpy(udp_cmd_rec->ip, ip);
  udp_cmd_rec->lenth = lenth;
  memcpy(udp_cmd_rec->buf, tempbuf, lenth);
  lenth = sizeof(udp_cmd_send_t) - 4 + lenth;
  sppUartCommandProcess((uint8_t *)udp_cmd_rec, lenth, app_context);
  free(tempbuf);

  return;
}

static void responseWEBF(int argc ,char *argv[])
{
  if(argc == 1){
    if(app_context->appConfig->at_config.webEnable == false){
      at_printf("+OK=OFF\r\n");
    }else{
      at_printf("+OK=ON\r\n");
    }
  }else if(argc == 2){
    if(!strcmp(argv[1], "ON")){
      app_context->appConfig->at_config.webEnable = true;
    }else if(!strcmp(argv[1], "OFF")){
      app_context->appConfig->at_config.webEnable = false;
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK\r\n");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseWEBU(int argc ,char *argv[])
{
  if(argc == 1){
    at_printf("+OK=%s,%s\r\n", app_context->appConfig->at_config.web_login.login_username,
              app_context->appConfig->at_config.web_login.login_password);
  }else if(argc == 3){
    if((strlen(argv[1]) >= 1) && strlen(argv[2]) >= 1) {
      strncpy(app_context->appConfig->at_config.web_login.login_username, argv[1], maxLoginUserNameLenth);
      strncpy(app_context->appConfig->at_config.web_login.login_password, argv[2], maxLoginPassWordLenth);
      at_printf("+OK\r\n");
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
    }
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseQUIT(int argc ,char *argv[])
{
  if(argc == 1){  
    app_context->appStatus.at_mode = DATA_MODE;
    at_printf("+OK\r\n");
    should_del_at = true;
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseFAT(int argc, char *argv[])
{
  if (argc == 1) {
    if (app_context->appConfig->factory_config.at_mode == AT_MODE) {
      at_printf("+OK=ON\r\n");
    } else if (app_context->appConfig->factory_config.at_mode == DATA_MODE) {
      at_printf("+OK=OFF\r\n");
    } 
  } else if (argc == 2) {
    if (!strcmp(argv[1], "ON")) {
      app_context->appConfig->factory_config.at_mode = AT_MODE;
    } else if (!strcmp(argv[1], "OFF")) {
      app_context->appConfig->factory_config.at_mode = DATA_MODE;
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK\r\n");
  } else {
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseFMODE(int argc, char *argv[])
{
  if (argc == 1) {
    if (app_context->appConfig->factory_config.at_mode == AT_MODE) {
      at_printf("+OK=AT\r\n");
    } else if (app_context->appConfig->factory_config.at_mode == AT_NO_MODE){
      at_printf("+OK=AT_NONE\r\n");
    } else if (app_context->appConfig->factory_config.at_mode == DATA_MODE) {
      at_printf("+OK=DATA\r\n");
    } 
  } else if (argc == 2) {
    if (!strcmp(argv[1], "AT")) {
      app_context->appConfig->factory_config.at_mode = AT_MODE;
    } else if (!strcmp(argv[1], "AT_NONE")) {
      app_context->appConfig->factory_config.at_mode = AT_NO_MODE;
    }else if (!strcmp(argv[1], "DATA")) {
      app_context->appConfig->factory_config.at_mode = DATA_MODE;
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK\r\n");
  } else {
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseFEVENT(int argc,char * argv [ ])
{
  if(argc == 1){
    if(app_context->appConfig->factory_config.event == true){
      at_printf("+OK=ON\r\n");
    }else{
      at_printf("+OK=OFF\r\n");
    }
  }else if(argc == 2){
    if(!strcmp(argv[1], "ON")){
      app_context->appConfig->factory_config.event = true;
    }else if(!strcmp(argv[1], "OFF")){
      app_context->appConfig->factory_config.event = false;
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK\r\n");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseFBONJOUR(int argc, char *argv[])
{
  if (argc == 1) {
    if (app_context->appConfig->factory_config.bonjourEnable == true) {
      at_printf("+OK=ON\r\n");
    } else {
      at_printf("+OK=OFF\r\n");
    }
  } else if (argc == 2) {
    if (!strcmp(argv[1], "ON")) {
      app_context->appConfig->factory_config.bonjourEnable = true;
    } else if (!strcmp(argv[1], "OFF")) {
      app_context->appConfig->factory_config.bonjourEnable = false;
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK\r\n");
  } else {
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseFWMODE(int argc, char *argv[])
{
  if (argc == 1) {
    if (app_context->appConfig->factory_config.wifi_mode == AP) {
      at_printf("+OK=AP\r\n");
    } else if (app_context->appConfig->factory_config.wifi_mode == STA) {
      at_printf("+OK=STA\r\n");
    } else if (app_context->appConfig->factory_config.wifi_mode == AP_STA) {
      at_printf("+OK=AP_STA\r\n");
    }
  } else if (argc == 2) {
    if (!strcmp(argv[1], "AP")) {
      app_context->appConfig->factory_config.wifi_mode = AP;
    } else if (!strcmp(argv[1], "STA")) {
      app_context->appConfig->factory_config.wifi_mode = STA;
    } else if (!strcmp(argv[1], "AP_STA")) {
      app_context->appConfig->factory_config.wifi_mode = AP_STA;
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK\r\n");
  } else {
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
  
}

static void responseFWAP(int argc, char *argv[])
{
  if (argc == 1) {
    at_printf("+OK=%s,%s,%d\r\n", app_context->appConfig->factory_config.ap_config.ssid,
              app_context->appConfig->factory_config.ap_config.key, 
              app_context->appConfig->factory_config.ap_config.channel);
  } else if ((argc == 4)) {
    if(str_is_digit(argv[3], 13, 0) == 0) {
      if(strlen(argv[2]) <= maxSsidLen){
        strncpy(app_context->appConfig->factory_config.ap_config.ssid, argv[1], maxSsidLen);
        if(strlen(argv[2]) < 8){
          strncpy(app_context->appConfig->factory_config.ap_config.key, "", maxKeyLen);
        }else if( (strlen(argv[2]) >= 8) && (strlen(argv[2]) <= maxKeyLen) ){
          strncpy(app_context->appConfig->factory_config.ap_config.key, argv[2], maxKeyLen);
        }else{
          at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
          return;
        }
        app_context->appConfig->factory_config.ap_config.channel = atoi(argv[3]);
      }else{
        at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
        return;
      }
      at_printf("+OK\r\n");
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
    }
  } else {
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseFWAPIP(int argc, char *argv[])
{
  if (argc == 1) {
    at_printf("+OK=%s,%s,%s\r\n", app_context->appConfig->factory_config.ap_ip_config.localIp,
              app_context->appConfig->factory_config.ap_ip_config.netMask,
              app_context->appConfig->factory_config.ap_ip_config.gateWay);
  } else if ((argc == 4)){
    if( (is_valid_ip(argv[1]) == 0) && (is_valid_ip(argv[2]) == 0) && (is_valid_ip(argv[3]) == 0) ) {
      strncpy(app_context->appConfig->factory_config.ap_ip_config.localIp, argv[1], maxIpLen);
      strncpy(app_context->appConfig->factory_config.ap_ip_config.netMask, argv[2], maxIpLen);
      strncpy(app_context->appConfig->factory_config.ap_ip_config.gateWay, argv[3], maxIpLen);
      at_printf("+OK\r\n");
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
    }
  } else {
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseFOCFGT(int argc, char *argv[])
{
  if (argc == 1) {
    at_printf("+OK=%d\r\n", app_context->appConfig->factory_config.wifi_config_timeout);
  } else if (argc == 2) { 
    if( str_is_digit(argv[1], 4294967295, 0) == 0 ) {
      app_context->appConfig->factory_config.wifi_config_timeout = atoi(argv[1]);
      at_printf("+OK\r\n");
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
    }
  } else {
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseFCON1(int argc, char *argv[])
{
  if (argc == 1) {
    at_printf("+OK=");
    if(app_context->appConfig->factory_config.conn1_config.protocol == TCP_SERVER) {
      at_printf("SERVER");
    }else if(app_context->appConfig->factory_config.conn1_config.protocol == TCP_CLIENT){
      at_printf("CLIENT");
    }else if(app_context->appConfig->factory_config.conn1_config.protocol == UDP_UNICAST){
      at_printf("UNICAST");
    }else if(app_context->appConfig->factory_config.conn1_config.protocol == UDP_BOARDCAST){
      at_printf("BOARDCAST");
    }else if(app_context->appConfig->factory_config.conn1_config.protocol == CLOUD_FOG){
      at_printf("FOG");
    }
    at_printf(",%d,%d",app_context->appConfig->factory_config.conn1_config.localPort,
              app_context->appConfig->factory_config.conn1_config.remotePort);

    at_printf(",%s\r\n", app_context->appConfig->at_config.conn1_config.remoteServerDomain);

  } else if ((argc == 5)){
    if (!strcmp(argv[1], "SERVER")) {
      app_context->appConfig->factory_config.conn1_config.protocol = TCP_SERVER;
    } else if (!strcmp(argv[1], "CLIENT")) {
      app_context->appConfig->factory_config.conn1_config.protocol = TCP_CLIENT;
    } else if (!strcmp(argv[1], "UNICAST")) {
      app_context->appConfig->factory_config.conn1_config.protocol = UDP_UNICAST;
    } else if (!strcmp(argv[1], "BOARDCAST")) {
      app_context->appConfig->factory_config.conn1_config.protocol = UDP_BOARDCAST;
    } else if (!strcmp(argv[1], "FOG")) {
      app_context->appConfig->factory_config.conn1_config.protocol = CLOUD_FOG;
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    if (str_is_digit(argv[2], 65535, 0) == 0) {
       app_context->appConfig->factory_config.conn1_config.localPort = atoi(argv[2]);
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    if (str_is_digit(argv[3], 65535, 0) == 0) {
      app_context->appConfig->factory_config.conn1_config.remotePort = atoi(argv[3]);
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    strncpy(app_context->appConfig->factory_config.conn1_config.remoteServerDomain, argv[4], maxRemoteServerLenth);
    at_printf("+OK\r\n");

  } else {
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}
#if 0
static void responseFCLD(int argc, char *argv[])
{
  if (argc == 1) {
    at_printf("+OK=%s,%s\r\n", app_context->appConfig->factory_config.cloud_fog.cloud_productID,
              app_context->
                flashContentInRam.factoryConfig.cloud_fog.cloud_productSecret);
  } else if (argc == 3) {
    strncpy(app_context->appConfig->factory_config.cloud_fog.cloud_productID, argv[1], maxDroductIDLenth);
    strncpy(app_context->appConfig->factory_config.cloud_fog.cloud_productSecret, argv[2], maxDroductSecretLenth);
    at_printf("+OK\r\n");
  } else {
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}
#endif
static void responseFCON2F(int argc, char *argv[])
{
  if (argc == 1) {
    if (app_context->appConfig->factory_config.conn2_enable == true) {
      at_printf("+OK=ON\r\n");
    } else {
      at_printf("+OK=OFF\r\n");
    }
  } else if (argc == 2) {
    if(!strcmp(argv[1], "ON")) {
      app_context->appConfig->factory_config.conn2_enable = true;
    } else if (!strcmp(argv[2], "OFF")) {
      app_context->appConfig->factory_config.conn2_enable = false;
    }
    at_printf("+OK\r\n");
  } else {
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }  
}

static void responseFCON2(int argc, char *argv[])
{
  if (argc == 1) {
    at_printf("+OK=");
    if(app_context->appConfig->factory_config.conn2_config.protocol == TCP_SERVER) {
      at_printf("SERVER");
    }else if(app_context->appConfig->factory_config.conn2_config.protocol == TCP_CLIENT){
      at_printf("CLIENT");
    }else if(app_context->appConfig->factory_config.conn2_config.protocol == UDP_UNICAST){
      at_printf("UNICAST");
    }else if(app_context->appConfig->factory_config.conn2_config.protocol == UDP_BOARDCAST){
      at_printf("BOARDCAST");
    }
    at_printf(",%d,%d", app_context->appConfig->factory_config.conn2_config.localPort,
              app_context->appConfig->factory_config.conn2_config.remotePort);
    at_printf(",%s\r\n", app_context->appConfig->factory_config.conn2_config.remoteServerDomain);  
  } else if ((argc == 5)){
    if (!strcmp(argv[1], "SERVER")) {
      app_context->appConfig->factory_config.conn2_config.protocol = TCP_SERVER;
    } else if (!strcmp(argv[1], "CLIENT")) {
      app_context->appConfig->factory_config.conn2_config.protocol = TCP_CLIENT;
    } else if (!strcmp(argv[1], "UNICAST")) {
      app_context->appConfig->factory_config.conn2_config.protocol = UDP_UNICAST;
    } else if (!strcmp(argv[1], "BOARDCAST")) {
      app_context->appConfig->factory_config.conn2_config.protocol = UDP_BOARDCAST;
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    if (str_is_digit(argv[2], 65535, 0) == 0) {
      app_context->appConfig->factory_config.conn2_config.localPort = atoi(argv[2]);
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    if (str_is_digit(argv[3], 65535, 0) == 0) {
      app_context->appConfig->factory_config.conn2_config.remotePort = atoi(argv[3]);
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    strncpy(app_context->appConfig->factory_config.conn2_config.remoteServerDomain, argv[4], maxRemoteServerLenth);
    at_printf("+OK\r\n");

  } else {
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseFUART(int argc, char *argv[])
{
  uint32_t baudrate;
  if(argc == 1) {
    at_printf("+OK=%d", app_context->appConfig->factory_config.serial_config.baud_rate);
    if(app_context->appConfig->factory_config.serial_config.data_width == DATA_WIDTH_5BIT){
      at_printf(",5");
    } else if(app_context->appConfig->factory_config.serial_config.data_width == DATA_WIDTH_6BIT) {
      at_printf(",6");
    } else if(app_context->appConfig->factory_config.serial_config.data_width == DATA_WIDTH_7BIT) {
      at_printf(",7");
    }else if(app_context->appConfig->factory_config.serial_config.data_width == DATA_WIDTH_8BIT) {
      at_printf(",8");
    }else if(app_context->appConfig->factory_config.serial_config.data_width == DATA_WIDTH_9BIT) {
      at_printf(",9");
    }
    
    if(app_context->appConfig->factory_config.serial_config.parity == NO_PARITY) {
      at_printf(",NONE");
    }else if(app_context->appConfig->factory_config.serial_config.parity == ODD_PARITY) {
      at_printf(",ODD");
    }else if(app_context->appConfig->factory_config.serial_config.parity == EVEN_PARITY) {
      at_printf(",EVEN");
    }
    
    if(app_context->appConfig->factory_config.serial_config.stop_bits == STOP_BITS_1) {
      at_printf(",1");
    }else if(app_context->appConfig->factory_config.serial_config.stop_bits == STOP_BITS_2) {
      at_printf(",2");
    }
    
    if(app_context->appConfig->factory_config.serial_config.flow_control == FLOW_CONTROL_DISABLED) {
      at_printf(",NONE");
    } else if(app_context->appConfig->factory_config.serial_config.flow_control == FLOW_CONTROL_CTS) {
      at_printf(",CTS");
    } else if(app_context->appConfig->factory_config.serial_config.flow_control == FLOW_CONTROL_RTS) {
      at_printf(",RTS");
    } else if(app_context->appConfig->factory_config.serial_config.flow_control == FLOW_CONTROL_CTS_RTS) {
      at_printf(",CTSRTS");
    }
    at_printf("\r\n");
  }else if (argc == 6){
    baudrate = atoi(argv[1]);
    if((baudrate == UART_BANDRATE_9600) || (baudrate == UART_BANDRATE_19200) || (baudrate == UART_BANDRATE_38400)
       || (baudrate == UART_BANDRATE_57600) || (baudrate == UART_BANDRATE_115200) || (baudrate == UART_BANDRATE_230400)
         || (baudrate == UART_BANDRATE_460800) || (baudrate == UART_BANDRATE_921600) || (baudrate == UART_BANDRATE_1843200)
           || (baudrate == UART_BANDRATE_3686400)){
             app_context->appConfig->factory_config.serial_config.baud_rate = baudrate;
           }else {
             at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
             return;
           }
    
    if(!strcmp(argv[2], "5")) {
      app_context->appConfig->factory_config.serial_config.data_width = DATA_WIDTH_5BIT;
    } else if(!strcmp(argv[2], "6")) {
      app_context->appConfig->factory_config.serial_config.data_width = DATA_WIDTH_6BIT;
    }else if(!strcmp(argv[2], "7")) {
      app_context->appConfig->factory_config.serial_config.data_width = DATA_WIDTH_7BIT;
    }else if(!strcmp(argv[2], "8")) {
      app_context->appConfig->factory_config.serial_config.data_width = DATA_WIDTH_8BIT;
    }else if(!strcmp(argv[2], "9")) {
      app_context->appConfig->factory_config.serial_config.data_width = DATA_WIDTH_9BIT;
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    
    if(!strcmp(argv[3], "NONE")) {
      app_context->appConfig->factory_config.serial_config.parity = NO_PARITY;
    }else if(!strcmp(argv[3], "ODD")) {
      app_context->appConfig->factory_config.serial_config.parity = ODD_PARITY;
    }else if(!strcmp(argv[3], "EVEN")) {
      app_context->appConfig->factory_config.serial_config.parity = EVEN_PARITY;
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    
    if(!strcmp(argv[4], "1")) {
      app_context->appConfig->factory_config.serial_config.stop_bits = STOP_BITS_1;
    }else if(!strcmp(argv[4], "2")) {
      app_context->appConfig->factory_config.serial_config.stop_bits = STOP_BITS_2;
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    
    if(!strcmp(argv[5], "NONE")){
      app_context->appConfig->factory_config.serial_config.flow_control = FLOW_CONTROL_DISABLED;
    }else if(!strcmp(argv[5], "CTS")){
      app_context->appConfig->factory_config.serial_config.flow_control = FLOW_CONTROL_CTS;
    }else if(!strcmp(argv[5], "RTS")){
      app_context->appConfig->factory_config.serial_config.flow_control = FLOW_CONTROL_RTS;
    }else if(!strcmp(argv[5], "CTSRTS")){
      app_context->appConfig->factory_config.serial_config.flow_control = FLOW_CONTROL_CTS_RTS;
    }else{
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK\r\n");
  }else {
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
  
}

static void responseFWEBF(int argc, char *argv[])
{
  if(argc == 1) {
    if(app_context->appConfig->factory_config.webEnable == true) {
      at_printf("+OK=ON\r\n");
    }else{
      at_printf("+OK=OFF\r\n");
    }
  }else if (argc == 2) {
    if(!strcmp(argv[1], "ON")) {
      app_context->appConfig->factory_config.webEnable = true;
    } else if(!strcmp(argv[1], "OFF")) {
      app_context->appConfig->factory_config.webEnable = false;
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
      return;
    }
    at_printf("+OK\r\n");
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseFWEBU(int argc, char *argv[])
{
  if(argc == 1) {
    at_printf("+OK=%s,%s\r\n", app_context->appConfig->factory_config.web_login.login_username,
              app_context->appConfig->factory_config.web_login.login_password);
  } else if(argc == 3){
    if((strlen(argv[1]) >= 1) && strlen(argv[2]) >= 1) {
      strncpy(app_context->appConfig->factory_config.web_login.login_username, argv[1], maxLoginUserNameLenth);
      strncpy(app_context->appConfig->factory_config.web_login.login_password, argv[2], maxLoginPassWordLenth);
      at_printf("+OK\r\n");
    } else {
      at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
    }
  }else{
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseFCLR(int argc, char *argv[])
{
  if(argc == 1){
    app_context->appConfig->factory_config.factoryConfigDataVer = 0x0;
    if(mico_system_context_restore(mico_context) != kNoErr){
      at_printf("+ERR=%d\r\n", INVALID_OPERATION);
      return;
    }
    at_printf("+OK\r\n");
    mico_system_power_perform( mico_context, eState_Software_Reset );   
  }else {
    at_printf("+ERR=%d\r\n", INVALID_PARAMETER);
  }
}

/***************************************************************/

static _S_AT_CMD s_at_commands[] = {
  
  /*manage command*/
  #define BASIC_MANAGE_COMMAND  (8)
  {"HELP",              NULL,  	                responseHELP},
  {"FMVER",             NULL,     	        responseFMVER},
  {"SYSTIME",           NULL,     	        responseSYSTIME},
  {"SAVE",              NULL,     	        responseSAVE},
  {"FACTORY",           NULL,     	        responseFACTORY},
  {"REBOOT",            NULL,     	        responseREBOOT},
  {"EVENT",             NULL,     	        responseEVENT},
  {"ECHO",              NULL,     	        responseECHO},
  
  /*uart command*/
  #define UART_COMMAND (4)
  {"UART",              NULL,     	        responseUART},
  {"UARTF",             NULL,     	        responseUARTF},
  {"UARTFT",            NULL,     	        responseUARTFT},
  {"UARTFL",            NULL,     	        responseUARTFL},
  
  /*power management command*/
  #define POWER_MANAGEMENT_COMMAND (2)
  {"PMSLP",              NULL,     	        responsePMSLP},
  {"PRSLP",              NULL,     	        responsePRSLP},
  
  /*wifi management command*/
  #define WIFI_MANAGEMENT_COMMAND (7)
  {"WFVER",             NULL,              responseWFVER},
  {"WMAC",               NULL,     	        responseWMAC},
  {"WGHBN",               NULL,     	        responseWGHBN},
  {"WSCAN",              NULL,               responseWSCAN},
  {"WMODE",             NULL,     	        responseWMODE},
  {"WAP",               NULL,     	        responseWAP},
  {"WAPCH",             NULL,     	        responseWAPCH},
  {"WSTA",              NULL,     	        responseWSTA},
  {"WPRESTA",              NULL,     	        responseWPRESTA},
  {"WLANF",             NULL,               responseWLANF},
  {"WSTATUS",           NULL,               responseWSTATUS},
  {"WLINK",             NULL,               responseWLINK},

  /*wifi basic command*/
  #define IP_MANAGEMENT_COMMAND (2)
  {"IPCONFIG",          NULL,     	        responseIPCONFIG},
  {"DHCP",              NULL,     	        responseDHCP},
  
  /*wifi cfg command*/
  #define ONE_CONFIG_COMMAND (2)  
  {"OCFG",              NULL,     	        responseOCFG},
  {"OCFGT",             NULL,     	        responseOCFGT},
  
  /*TCP/UDP cfg command*/
  #define TCP_CONFIG_COMMAND (5)
  {"CON1",             NULL,     	        responseCON1},
  {"CON2F",            NULL,     	        responseCON2F},
  {"CON2",             NULL,     	        responseCON2},
  {"CONSN",            NULL,     	        responseCONSN}, //server number
  {"CONF",             NULL,     	        responseCONF},  
  {"CONS",             NULL,     	        responseCONS},
  
  /*socket command*/
  #define SOCKET_COMMAND (2)
  {"SSEND",             NULL,     	        responseSSEND},
  {"SSSEND",            NULL,     	        responseSSSEND},
  {"SUNSEND",           NULL,     	        responseSUNSEND}, 
  
  /*cloud command*/
//  #define CLOUD_COMMAND (4)
//  {"WCLDS",             NULL,     	        responseWCLDS},
//  {"WCLD",              NULL,     	        responseWCLD},
//  {"WCVER",             NULL,     	        responseWCVER},
//  {"WOTA",              NULL,     	        responseWOTA},
  
  /*web command*/
  #define WEB_COMMAND (2)
  {"WEBF",              NULL,     	        responseWEBF},
  {"WEBU",              NULL,     	        responseWEBU},
  
  {"QUIT", 	        NULL,	                responseQUIT},
  
  /*factory command*/
  #define FACTORY_COMMAND (15)
  {"FHELP", 	        NULL,	                responseFHELP},
  {"FAT", 	        NULL,	                responseFAT},
  {"FMODE", 	        NULL,	                responseFMODE},
  {"FEVENT", 	        NULL,	                responseFEVENT},
  {"FBONJOUR", 	        NULL,	                responseFBONJOUR},
  {"FWMODE", 	        NULL,	                responseFWMODE},
  {"FWAP", 	        NULL,	                responseFWAP},
  {"FWAPIP", 	        NULL,	                responseFWAPIP},
  {"FOCFGT", 	        NULL,	                responseFOCFGT},
  {"FCON1", 	        NULL,	                responseFCON1},
//  {"FCLD", 	        NULL,	                responseFCLD},
  {"FCON2F", 	        NULL,	                responseFCON2F},
  {"FCON2", 	        NULL,	                responseFCON2},
  {"FUART", 	        NULL,	                responseFUART},
  {"FWEBF", 	        NULL,	                responseFWEBF},
  {"FWEBU", 	        NULL,	                responseFWEBU},
  {"FCLR", 	        NULL,	                responseFCLR},
	
};

#define AT_COMMAND_NUM (sizeof(s_at_commands) / sizeof(s_at_commands[0]))


/********************************************************************/

static void responseHELP(int argc ,char *argv[])
{
  uint8_t i;
  
  if(argc == 1){
    at_printf("+OK=\r\n");
    for(i = 0; i < (AT_COMMAND_NUM - FACTORY_COMMAND); i++) {
      at_printf("    AT+%s%s\r\n", s_at_commands[i].requestName,
                s_at_commands[i].help?s_at_commands[i].help:"");
    }
  } else {
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void responseFHELP(int argc ,char *argv[])
{
  uint8_t i;
  
  if(argc == 1) {
    at_printf("+OK=\r\n");
    for(i = (uint8_t)(AT_COMMAND_NUM - FACTORY_COMMAND); i < AT_COMMAND_NUM; i++) {
      at_printf("    AT+%s%s\r\n", s_at_commands[i].requestName,
                s_at_commands[i].help?s_at_commands[i].help:"");
    }
  } else {
    at_printf("+ERR=%d\r\n", INVALID_FORMAT);
  }
}

static void processCommand(int argc, char *argv[])
{
  int i;
  if (argc >= 1) {
    
    for (i = 0; i < strlen(argv[0]); i++)
      argv[0][i] = toupper(argv[0][i]);
    
    for(i = 0; i < AT_COMMAND_NUM; i++){
      if( 0 == strcmp(argv[0], s_at_commands[i].requestName)){
        (&(s_at_commands[i]))->responseFunction(argc, argv);
        break;
      }
    }
    
    if (i >= AT_COMMAND_NUM) {
      at_printf("+ERR=%d\r\n", INVALID_COMMAND);
    }
    
  } else {
    at_printf("+ERR=%d\r\n", INVALID_COMMAND);
  }
}

static void parseParameter(char *args)
{
  int argc = 1;
  char *p = args;
  char *argv[MAX_SINGLE_PARAMETER];
  
  argv[0] = strsep(&p, "=");
  while (p) {
    while ((argv[argc] = strsep(&p, ",")) != NULL) {
      argc++;
    }
  }
  processCommand(argc, argv);
}

static int at_putstr(const char *msg)
{
  if (msg[0] != 0)
    MicoUartSend( UART_FOR_APP, (const char*)msg, strlen(msg) );
  
  return 0;
}

static int at_getchar(char *inbuf, uint32_t len, uint32_t timeout)
{
  if (MicoUartRecv(UART_FOR_APP, inbuf, len, timeout) == 0)
    return 1;
  else
    return 0;
}

int at_printf(const char *msg, ...)
{
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
  
  at_putstr((const char*)message);
  return 0;
}

/*AT+SSEND=[socket,length]<CR>*/
static int command_ssend(char *inbuf, uint32_t timeout)
{
  unsigned int bp = 0;
  int socket=0,length=0;
  int count = 0;
  while (1) {
    if (at_getchar(&inbuf[bp], 1, timeout) == 1) {
      if (count == 1) {
        if (inbuf[bp] == '\r'){
          inbuf[bp] = 0x0;
          bp = 0;
          if(str_is_digit(inbuf, MAX_RECV_BUFFER_SIZE, 0) == 0) {
            at_printf(">");
            length=atoi(inbuf);
            memset(inbuf, 0x00, MAX_RECV_BUFFER_SIZE);
            if (at_getchar(&inbuf[bp], 1, 3000) == 1) {
              bp++;
              if(bp == length) {
                socket_cmd_send(socket, bp, inbuf);
                return 1;
              }
              while(1){ 
                if (at_getchar(&inbuf[bp], 1, 10) == 1) {
                  bp++;
                  if (bp == length) {
                    socket_cmd_send(socket, bp, inbuf);
                    return 1;
                  }
                }else{
                  socket_cmd_send(socket, bp, inbuf);
                  return 1;
                }
              }
            } else {
              return 1;
            }
          }
        }
        bp++;
        continue;
      }
      
      if (inbuf[bp] == ',') {
        inbuf[bp] = 0x0;
        bp = 0;
        count = 1;
        if(str_is_digit(inbuf, 10, 0) == 0) {
          socket=atoi(inbuf);
          memset(inbuf, 0x00, MAX_RECV_BUFFER_SIZE);
          continue;
        }
      }
      bp++;
    } else {
      return 0;
    }
  }
}

/*AT+SSSEND=[socket,length,data]<CR>*/
static int command_sssend(char *inbuf, uint32_t timeout)
{
  unsigned int bp = 0;
  int socket = 0,length = 0;
  int count = 0;
  while (1) {
    if (at_getchar(&inbuf[bp], 1, timeout) == 1) {
      if (count == 1) {
        if (inbuf[bp] == ','){
          inbuf[bp] = 0x0;
          bp = 0;
          if(str_is_digit(inbuf, MAX_RECV_BUFFER_SIZE, 0) == 0) {
            length=atoi(inbuf);
            memset(inbuf, 0x00, MAX_RECV_BUFFER_SIZE);
            while(1){
              if (at_getchar(&inbuf[bp], 1, timeout) == 1) {
                bp++;
                if (bp == length) {
                  if (at_getchar(&inbuf[bp], 1, timeout) == 1){
                    if (inbuf[bp] == '\r'){
                      socket_cmd_send(socket, bp, inbuf);
                      return 1;
                    }else{
                      return 0;
                    }
                  }else{
                    return 0;
                  }
                }
              }else{
                return 0;
              }
            }
          }
        }
        bp++;
        continue;
      }
      
      if (inbuf[bp] == ',') {
        inbuf[bp] = 0x0;
        bp = 0;
        count = 1;
        if(str_is_digit(inbuf, 10, 0) == 0) {
          socket=atoi(inbuf);
          memset(inbuf, 0x00, MAX_RECV_BUFFER_SIZE);
          continue;
        }
      }
      bp++;
    } else {
      return 0;
    }
  }
}

/*AT+SUNSEND=[port,ip,length,data]<CR>*/
static int command_sunsend(char *inbuf, uint32_t timeout)
{
  unsigned int bp = 0;
  int port = 0, con  = -1, length;
  char ip[maxIpLen];
  int count = 0;

  while (1) {
    if (at_getchar(&inbuf[bp], 1, timeout) == 1) {
      
      if (count == 3) {
        if (inbuf[bp] == ','){
          inbuf[bp] = 0x0;
          bp = 0;
          if(str_is_digit(inbuf, MAX_RECV_BUFFER_SIZE, 0) == 0) {
            length=atoi(inbuf);
            memset(inbuf, 0x00, MAX_RECV_BUFFER_SIZE);
            while(1){
              if (at_getchar(&inbuf[bp], 1, timeout) == 1) {
                bp++;
                if (bp == length) {
                  if (at_getchar(&inbuf[bp], 1, timeout) == 1){
                    if (inbuf[bp] == '\r'){
                      socket_unicast_send(con, port, ip, bp, inbuf);
                      return 1;
                    }else{
                      return 0;
                    }
                  }else{
                    return 0;
                  }
                }
              }else{
                return 0;
              }
            }
          }
        }
        bp++;
        continue;
      }
      
      if (count == 2){
        if (inbuf[bp] == ',') {
          inbuf[bp] = 0x0;
          bp = 0;
          count = 3;
          if(is_valid_ip(inbuf) == 0) {
            strcpy(ip, inbuf);
            memset(inbuf, 0x00, MAX_RECV_BUFFER_SIZE);
            continue;
          }
        }
      }
      
      if (count == 1){
        if (inbuf[bp] == ',') {
          inbuf[bp] = 0x0;
          bp = 0;
          count = 2;
          if(str_is_digit(inbuf, 65535, 0) == 0) {
            port=atoi(inbuf);
            memset(inbuf, 0x00, MAX_RECV_BUFFER_SIZE);
            continue;
          }
        }
      }
      
      if (inbuf[bp] == ',') {
        inbuf[bp] = 0x0;
        bp = 0;
        count = 1;
        if(str_is_digit(inbuf, 3, 0) == 0) {
          con = atoi(inbuf);
          memset(inbuf, 0x00, MAX_RECV_BUFFER_SIZE);
          continue;
        }
      }
      
      bp++;
    } else {
      return 0;
    }
  }
}

/* Get an input line.
*
* Returns: 1 if there is input, 0 if the line should be ignored. */
static int get_input(char *inbuf, uint32_t timeout)
{
  unsigned int bp = 0;
  if (inbuf == NULL) {
    return 0;
  }
  while (1) {
    if (at_getchar(&inbuf[bp], 1, timeout) == 1) {
      if (app_context->appStatus.at_echo == true) {
        at_printf("%c", inbuf[bp]);
      }
      
      if (inbuf[bp] == '\r') {
        return bp+1;
      }
      
      if (!strncmp(inbuf, "AT+SSEND=", 9)) {
        bp=0;
        memset(inbuf, 0x00, MAX_RECV_BUFFER_SIZE);
        if (command_ssend(inbuf, timeout) == 0) {
          at_printf("+ERR=%d\r\n", INVALID_FORMAT);
        }
        return 0;
      } else if (!strncmp(inbuf, "AT+SSSEND=", 10)) {
        bp=0;
        memset(inbuf, 0x00, MAX_RECV_BUFFER_SIZE);
        if (command_sssend(inbuf, timeout) == 0) {
          at_printf("+ERR=%d\r\n", INVALID_FORMAT);
        }
        return 0;
      } else if ( !strncmp(inbuf, "AT+SUNSEND=", 11) ){
         if (command_sunsend(inbuf, timeout) == 0) {
          at_printf("+ERR=%d\r\n", INVALID_FORMAT);
        }
        return 0;       
      }
     
      (bp)++;
      if (bp >= 1024) {
        return bp;
      }
    } else {
      return bp;
    }
  }
}

static void at_cmd_Thread(mico_thread_arg_t inapp_context)
{
  int i;
  uint32_t timeout = 500;
  uint32_t len = 1024;
  int reclen = 0;
  app_context = (app_context_t *)inapp_context;
  char uart_rec_data[MAX_RECV_BUFFER_SIZE];
  
  if( app_context->appStatus.at_mode == AT_NO_MODE ){
    stop_all_net_work( app_context );
  }
  
  while (1) {
    get_uart_recv_len_time(app_context, &len, &timeout);
    memset(uart_rec_data,0x00, MAX_RECV_BUFFER_SIZE);
    reclen = get_input(uart_rec_data, timeout);
    if (reclen == 0)
      continue;
    
    if (replaceCRCF(uart_rec_data, uart_rec_data) == 1) {
      
      /* upper the header of AT command */
      for (i = 0; i < 2; i++)
        uart_rec_data[i] = toupper(uart_rec_data[i]);
      
      if (strStartsWith(uart_rec_data, AT_HEADER) == 1) {	
        if (0 == strcmp(uart_rec_data, AT_HEADER)) {	/* AT test command */
          at_printf("+OK\r\n");
          
        } else {	 /* AT command with parameters */
          if (strStartsWith(uart_rec_data, AT_HEADER_CMD) == 1) {
            parseParameter(&uart_rec_data[3]);
            
          } else {
            at_printf("+ERR=%d\r\n", INVALID_COMMAND);
          }
        }
        
      } else {
        at_printf("+ERR=%d\r\n", INVALID_COMMAND);
      }
    } else {
      at_printf("+ERR=%d\r\n", INVALID_COMMAND);
    }
    
    if(should_del_at == true){
      mico_data_mode( app_context );
      goto exit;
    }
  }
exit:
  mico_rtos_delete_thread(NULL);
}

OSStatus mico_at_mode ( void *app_context_c )
{
  app_context = app_context_c;
  mico_context = mico_system_context_get();
  should_del_at = false;
  return mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "AT Cmd", at_cmd_Thread, STACK_SIZE_AT_CMD_THREAD, (mico_thread_arg_t)app_context );
}

