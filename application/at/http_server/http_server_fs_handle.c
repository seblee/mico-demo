#include <httpd.h>
#include <http_parse.h>
#include <http-strings.h>

#include "MICO.h"
#include "MICOAppDefine.h"
#include "httpd_priv.h"
#include "StringUtils.h"
#include "CheckSumUtils.h"
#include "http_server/http_server.h"
#include "http_replace.h"

#include "libmicroplatform.h"
#include "microhttpd.h"
#include "internal.h"

#define http_file_handle_log(M, ...) custom_log("http file handle", M, ##__VA_ARGS__)

#define HTTPD_SERVER_HDR_DEFORT (HTTPD_HDR_ADD_SERVER|HTTPD_HDR_ADD_CONN_KEEP_ALIVE)

#define rec_ota_buf_size 1024

static mico_Context_t *mico_context;
static app_context_t *app_context;
static CRC16_Context crc16_context;
static uint16_t crc;
static uint32_t offset = 0;
static bool is_ota_success = false;
static bool is_config_success = false;
static mico_logic_partition_t* ota_partition;

static int g_sys_file_handlers_no;
static struct httpd_wsgi_call g_sys_file_handlers[];
static int socket_web;

extern void mico_write_ota_tbl(int len, uint16_t crc);
extern char *strrep(char *src, char* oldStr, char * newStr);

struct _scan_res{
  char ssid[32];
  char power;
};
struct _scan_res scan_res[10];
static int scan_count = 0;
static mico_semaphore_t  scan_semaphore;

void WEBNotify_ApListCallback(ScanResult *pApList)
{
  int i;
  scan_count = 0;
  for(i=0; i<pApList->ApNum; i++) {
		http_file_handle_log("scan %s, %d", pApList->ApList[i].ssid, pApList->ApList[i].ApPower);
    strcpy(scan_res[i].ssid, pApList->ApList[i].ssid);
    scan_res[i].power = pApList->ApList[i].ApPower;
    scan_count++;
    if( scan_count >= 10 )
      break;
  }
  mico_rtos_set_semaphore(&scan_semaphore);
}

static int value_checker (void *cls, enum MHD_ValueKind kind, const char *key, const char *filename, const char *content_type, const char *transfer_encoding, const char *data, uint64_t off, size_t size)
{
  int *want_off = cls;
  int idx = *want_off;

  //http_file_handle_log("idx=%d; key=%s; filename=%s; size=%d; off=%d", idx, key, filename, (int)size, (int)off);
  
  if( (off != 0) && (size == 0) )
    return MHD_YES;
  
//  http_file_handle_log("key:%s ;  value:%s", key, data);
  
  
  if( !strcmp("mode", key) ){        /*wifi work mode*/
    if( !strcmp(data, "ap") ){
      app_context->appConfig->at_config.wifi_mode = AP;
    }else if( !strcmp(data, "sta") ){
      app_context->appConfig->at_config.wifi_mode = STA;
    }else if( !strcmp(data, "ap_sta") ){
      app_context->appConfig->at_config.wifi_mode = AP_STA;
    }
  }else if( !strcmp("ap_ssid", key) ){   /*ap mode setting*/
    strncpy(app_context->appConfig->at_config.ap_config.ssid, data, maxSsidLen);
  }else if( !strcmp("ap_sec", key) ){
    if( !strcmp(data, "disable") ){
      strncpy(app_context->appConfig->at_config.ap_config.key, "", maxKeyLen);
    }
  }else if( !strcmp("ap_key", key) ){
    strncpy(app_context->appConfig->at_config.ap_config.key, data, maxKeyLen);
  }else if( !strcmp("ap_ip", key) ){
    strncpy(app_context->appConfig->at_config.ap_ip_config.localIp, data, maxIpLen);
  }else if( !strcmp("ap_netmask", key) ){
    strncpy(app_context->appConfig->at_config.ap_ip_config.netMask, data, maxIpLen);
  }else if( !strcmp("ap_gateway", key) ){
    strncpy(app_context->appConfig->at_config.ap_ip_config.gateWay, data, maxIpLen);
  }else if( !strcmp("sta_ssid", key) ){        /*station mode setting*/
    strncpy( mico_context->micoSystemConfig.ssid, data, maxSsidLen );
  }else if( !strcmp("sta_sec", key) ){
    if( !strcmp(data, "disable") ){
      strncpy(mico_context->micoSystemConfig.user_key, "", maxKeyLen);
    }
  }else if( !strcmp("sta_key", key) ){
    strncpy(mico_context->micoSystemConfig.key, data, maxKeyLen);
    strncpy(mico_context->micoSystemConfig.user_key, data, maxKeyLen);
    mico_context->micoSystemConfig.keyLength = strlen(mico_context->micoSystemConfig.key);
    mico_context->micoSystemConfig.user_keyLength = strlen(mico_context->micoSystemConfig.user_key);
  }else if( !strcmp("sta_dhcp", key) ){
    if( !strcmp(data, "dhcp_client" ) ){
      mico_context->micoSystemConfig.dhcpEnable = true;
    }else if( !strcmp(data, "dhcp_disable" ) ){
      mico_context->micoSystemConfig.dhcpEnable = false;
    }
  }else if( !strcmp("sta_ip", key) ){
    strncpy(mico_context->micoSystemConfig.localIp, data, maxIpLen);
  }else if( !strcmp("sta_netmask", key) ){
    strncpy(mico_context->micoSystemConfig.netMask, data, maxIpLen);
  }else if( !strcmp("sta_gateway", key) ){
    strncpy(mico_context->micoSystemConfig.gateWay, data, maxIpLen);
  }else if( !strcmp("sta_ssid1", key) ){
    strncpy( app_context->appConfig->at_config.sta_pre_ssid[0].ssid, data, maxSsidLen );
  }else if( !strcmp("sta_key1", key) ){
    strncpy( app_context->appConfig->at_config.sta_pre_ssid[0].key, data, maxKeyLen);
  }else if( !strcmp("sta_ssid2", key) ){
    strncpy( app_context->appConfig->at_config.sta_pre_ssid[1].ssid, data, maxSsidLen );
  }else if( !strcmp("sta_key2", key) ){
    strncpy( app_context->appConfig->at_config.sta_pre_ssid[1].key, data, maxKeyLen);
  }else if( !strcmp("sta_ssid3", key) ){
    strncpy( app_context->appConfig->at_config.sta_pre_ssid[2].ssid, data, maxSsidLen );
  }else if( !strcmp("sta_key3", key) ){
    strncpy( app_context->appConfig->at_config.sta_pre_ssid[2].key, data, maxKeyLen);
  }else if( !strcmp("sta_ssid4", key) ){
    strncpy( app_context->appConfig->at_config.sta_pre_ssid[3].ssid, data, maxSsidLen );
  }else if( !strcmp("sta_key4", key) ){
    strncpy( app_context->appConfig->at_config.sta_pre_ssid[3].key, data, maxKeyLen);
  }else if( !strcmp("sta_ssid5", key) ){
    strncpy( app_context->appConfig->at_config.sta_pre_ssid[4].ssid, data, maxSsidLen );
  }else if( !strcmp("sta_key5", key) ){
    strncpy( app_context->appConfig->at_config.sta_pre_ssid[4].key, data, maxKeyLen);
  }else if( !strcmp("uart_br", key) ){      /*uart setting*/
    app_context->appConfig->at_config.serial_config.baud_rate = atoi(data);
  }else if( !strcmp("uart_data", key) ){
    if(!strcmp(data, "5")) {
      app_context->appConfig->at_config.serial_config.data_width = DATA_WIDTH_5BIT;
    } else if(!strcmp(data, "6")) {
      app_context->appConfig->at_config.serial_config.data_width = DATA_WIDTH_6BIT;
    }else if(!strcmp(data, "7")) {
      app_context->appConfig->at_config.serial_config.data_width = DATA_WIDTH_7BIT;
    }else if(!strcmp(data, "8")) {
      app_context->appConfig->at_config.serial_config.data_width = DATA_WIDTH_8BIT;
    }
  }else if( !strcmp("uart_parity", key) ){
    if(!strcmp(data, "none")) {
      app_context->appConfig->at_config.serial_config.parity = NO_PARITY;
    }else if(!strcmp(data, "odd")) {
      app_context->appConfig->at_config.serial_config.parity = ODD_PARITY;
    }else if(!strcmp(data, "even")) {
      app_context->appConfig->at_config.serial_config.parity = EVEN_PARITY;
    }
  }else if( !strcmp("uart_stop", key) ){ 
    if(!strcmp(data, "1")) {
      app_context->appConfig->at_config.serial_config.stop_bits = STOP_BITS_1;
    }else if(!strcmp(data, "2")) {
      app_context->appConfig->at_config.serial_config.stop_bits = STOP_BITS_2;
    }
  }else if( !strcmp("uart_fc", key) ){
    if(!strcmp(data, "disable")){
      app_context->appConfig->at_config.serial_config.flow_control = FLOW_CONTROL_DISABLED;
    }else if(!strcmp(data, "enable")){
      app_context->appConfig->at_config.serial_config.flow_control = FLOW_CONTROL_CTS_RTS;
    }
  }else if( !strcmp("format_mode", key) ){
    if( !strcmp(data, "disable") ){
      app_context->appConfig->at_config.autoFormatEnable = false;
    }else if( !strcmp(data, "enable" )){
      app_context->appConfig->at_config.autoFormatEnable = true;
    }
  }else if( !strcmp("format_len", key) ){
    app_context->appConfig->at_config.uart_auto_format.auto_format_len = atoi(data);
  }else if( !strcmp("format_time", key) ){
    app_context->appConfig->at_config.uart_auto_format.auto_format_time = atoi(data);
  }else if( !strcmp("con1_protocol", key) ){         /*tcp/ip setting*/
    if( !strcmp(data, "server") ){
      app_context->appConfig->at_config.conn1_config.protocol = TCP_SERVER;
    }else if( !strcmp(data, "client") ){
      app_context->appConfig->at_config.conn1_config.protocol = TCP_CLIENT;
    }else if( !strcmp(data, "unicast") ){
      app_context->appConfig->at_config.conn1_config.protocol = UDP_UNICAST;
    }else if( !strcmp(data, "boardcast") ){
      app_context->appConfig->at_config.conn1_config.protocol = UDP_BOARDCAST;
    }
  }else if( !strcmp("con1_lport", key) ){
    app_context->appConfig->at_config.conn1_config.localPort = atoi(data);
  }else if( !strcmp("con1_rport", key) ){
    app_context->appConfig->at_config.conn1_config.remotePort = atoi(data); 
  }else if( !strcmp("con1_rserver", key) ){
    strncpy(app_context->appConfig->at_config.conn1_config.remoteServerDomain, data, maxRemoteServerLenth);
  }else if( !strcmp("con2_protocol", key) ){
    if( !strcmp(data, "disable") ){
      app_context->appConfig->at_config.conn2_enable = false;
    }else{
      app_context->appConfig->at_config.conn2_enable = true;
      if( !strcmp(data, "server") ){
        app_context->appConfig->at_config.conn2_config.protocol = TCP_SERVER;
      }else if( !strcmp(data, "client") ){
        app_context->appConfig->at_config.conn2_config.protocol = TCP_CLIENT;
      }else if( !strcmp(data, "unicast") ){
        app_context->appConfig->at_config.conn2_config.protocol = UDP_UNICAST;
      }else if( !strcmp(data, "boardcast") ){
        app_context->appConfig->at_config.conn2_config.protocol = UDP_BOARDCAST;
      }
    }
  }else if( !strcmp("con2_lport", key) ){
    app_context->appConfig->at_config.conn2_config.localPort = atoi(data);
  }else if( !strcmp("con2_rport", key) ){
    app_context->appConfig->at_config.conn2_config.remotePort = atoi(data); 
  }else if( !strcmp("con2_rserver", key) ){
    strncpy(app_context->appConfig->at_config.conn2_config.remoteServerDomain, data, maxRemoteServerLenth);  
  }else if( !strcmp("user_name", key) ){    /*model setting*/ 
    strncpy(app_context->appConfig->at_config.web_login.login_username, data, maxLoginUserNameLenth);
  }else if( !strcmp("user_key", key) ){
      strncpy(app_context->appConfig->at_config.web_login.login_password, data, maxLoginPassWordLenth);
  }else if( !strcmp("reboot_mode", key) ){
    if( !strcmp(data, "disable") ){
        app_context->appConfig->at_config.time_reboot.time_reboot_enable = false;
    }else if( !strcmp(data, "enable" )){
        app_context->appConfig->at_config.time_reboot.time_reboot_enable = true;
    }
   }else if( !strcmp("reboot_time", key) ){
        app_context->appConfig->at_config.time_reboot.time_reboot_time = atoi(data);
  }else if( !strcmp("ota_fw", key) ){
    if( !strstr(filename, ".bin") ){
      is_ota_success = false;
      return MHD_YES;
    }
    is_ota_success = true;
    if( off == 0){
      if( size == 0 )
        return MHD_YES;
      CRC16_Init( &crc16_context );
      MicoFlashErase( MICO_PARTITION_OTA_TEMP, 0x0, ota_partition->partition_length);
      MicoFlashWrite( MICO_PARTITION_OTA_TEMP, &offset, (uint8_t *)data, size);
      CRC16_Update( &crc16_context, data, size);
    }else{
      MicoFlashWrite( MICO_PARTITION_OTA_TEMP, &offset, (uint8_t *)data, size);
      CRC16_Update( &crc16_context, data, size);      
    }
  }else{
    http_file_handle_log("unknow");
    is_config_success = false;
    return MHD_NO;
  }
  is_config_success = true;
  *want_off = idx + 1;
  return MHD_YES;
}

int http_replace_rep_callback(char *key, char *value)
{
  char *out_buf;
  char v_buf[60];
  char fver[sysVersionLenth];
  char model[10];
  LinkStatusTypeDef link_state;
  mico_system_status_wlan_t *mico_status_wlan;

  mico_system_wlan_get_status( &mico_status_wlan );
  
 // http_file_handle_log("key:%s ; value:%s", key, value);
  out_buf = malloc(500);
  memset(v_buf, 0x00, sizeof(v_buf));
  
  if( !strcmp( key, "MODE" ) ){       /*wifi work mode*/
    if( app_context->appConfig->at_config.wifi_mode == AP ){
      strcpy(v_buf, "ap");
    }else if( app_context->appConfig->at_config.wifi_mode == STA ){
      strcpy(v_buf, "sta");
    }else if( app_context->appConfig->at_config.wifi_mode == AP_STA ){
      strcpy(v_buf, "ap_sta");
    }
  }else if( !strcmp( key, "AP_SSID" ) ){   /*ap mode setting*/
    strcpy(v_buf, app_context->appConfig->at_config.ap_config.ssid );
  }else if( !strcmp( key, "AP_SEC" ) ){
    if( !strcmp(app_context->appConfig->at_config.ap_config.key, "") ){
       strcpy(v_buf, "disable");
    }else{
       strcpy(v_buf, "enable");
    }
  }else if( !strcmp( key, "AP_KEY" ) ){
    strcpy(v_buf, app_context->appConfig->at_config.ap_config.key );
  }else if( !strcmp( key, "AP_IP" ) ){
    strcpy(v_buf, app_context->appConfig->at_config.ap_ip_config.localIp );
  }else if( !strcmp( key, "AP_NETMASK" ) ){
    strcpy(v_buf, app_context->appConfig->at_config.ap_ip_config.netMask );
  }else if( !strcmp( key, "AP_GATEWAY" ) ){
    strcpy(v_buf, app_context->appConfig->at_config.ap_ip_config.gateWay );
  }else if( !strcmp(key, "STA_SSID") ){     /*station mode setting*/
    strcpy(v_buf, mico_context->micoSystemConfig.ssid );
  }else if( !strcmp(key, "STA_SEC") ){
    if( !strcmp(mico_context->micoSystemConfig.user_key, "" ) ){
      strcpy(v_buf, "disable");
    }else{
      strcpy(v_buf, "enable");
    }
  }else if( !strcmp( key, "STA_KEY" ) ){
    strcpy(v_buf, mico_context->micoSystemConfig.user_key);
  }else if( !strcmp( key, "STA_DHCP" ) ){
    if( mico_context->micoSystemConfig.dhcpEnable == true ){
      strcpy(v_buf, "dhcp_client");
    }else{
      strcpy(v_buf, "dhcp_disable");
    }
  }else if( !strcmp(key, "STA_IP") ){
    strcpy(v_buf, mico_status_wlan->localIp);
  }else if( !strcmp(key, "STA_NETMASK") ){
    strcpy(v_buf, mico_status_wlan->netMask);
  }else if( !strcmp(key, "STA_GATEWAY") ){
    strcpy(v_buf, mico_status_wlan->gateWay);
  }else if( !strcmp(key, "STA_SSID1") ){
    strcpy(v_buf, app_context->appConfig->at_config.sta_pre_ssid[0].ssid);
  }else if( !strcmp(key, "STA_KEY1") ){
    strcpy(v_buf, app_context->appConfig->at_config.sta_pre_ssid[0].key);
  }else if( !strcmp(key, "STA_SSID2") ){
    strcpy(v_buf, app_context->appConfig->at_config.sta_pre_ssid[1].ssid);
  }else if( !strcmp(key, "STA_KEY2") ){
    strcpy(v_buf, app_context->appConfig->at_config.sta_pre_ssid[1].key);
  }else if( !strcmp(key, "STA_SSID3") ){
    strcpy(v_buf, app_context->appConfig->at_config.sta_pre_ssid[2].ssid);
  }else if( !strcmp(key, "STA_KEY3") ){
    strcpy(v_buf, app_context->appConfig->at_config.sta_pre_ssid[2].key);
  }else if( !strcmp(key, "STA_SSID4") ){
    strcpy(v_buf, app_context->appConfig->at_config.sta_pre_ssid[3].ssid);
  }else if( !strcmp(key, "STA_KEY4") ){
    strcpy(v_buf, app_context->appConfig->at_config.sta_pre_ssid[3].key);
  }else if( !strcmp(key, "STA_SSID5") ){
    strcpy(v_buf, app_context->appConfig->at_config.sta_pre_ssid[4].ssid);
  }else if( !strcmp(key, "STA_KEY5") ){
    strcpy(v_buf, app_context->appConfig->at_config.sta_pre_ssid[4].key);     
  }else if( !strcmp( key, "UART_BR" ) ){        /*uart setting*/
     sprintf(v_buf, "%ld", app_context->appConfig->at_config.serial_config.baud_rate);
  }else if( !strcmp( key, "UART_DATA" ) ){
    if(app_context->appConfig->at_config.serial_config.data_width == DATA_WIDTH_5BIT){
       strcpy(v_buf, "5");
    } else if(app_context->appConfig->at_config.serial_config.data_width == DATA_WIDTH_6BIT) {
      strcpy(v_buf, "6");
    } else if(app_context->appConfig->at_config.serial_config.data_width == DATA_WIDTH_7BIT) {
      strcpy(v_buf, "7");
    }else if(app_context->appConfig->at_config.serial_config.data_width == DATA_WIDTH_8BIT) {
      strcpy(v_buf, "8");
    }
  }else if( !strcmp( key, "UART_PARITY" ) ){
    if(app_context->appConfig->at_config.serial_config.parity == NO_PARITY) {
      strcpy(v_buf, "none");
    }else if(app_context->appConfig->at_config.serial_config.parity == ODD_PARITY) {
      strcpy(v_buf, "odd");
    }else if(app_context->appConfig->at_config.serial_config.parity == EVEN_PARITY) {
      strcpy(v_buf, "even");
    }
  }else if( !strcmp( key, "UART_STOP" ) ){
    if(app_context->appConfig->at_config.serial_config.stop_bits == STOP_BITS_1) {
      strcpy(v_buf, "1");
    }else if(app_context->appConfig->at_config.serial_config.stop_bits == STOP_BITS_2) {
      strcpy(v_buf, "2");
    }
  }else if( !strcmp( key, "UART_FC" ) ){
    if(app_context->appConfig->at_config.serial_config.flow_control == FLOW_CONTROL_DISABLED) {
      strcpy(v_buf, "disable");
    } else if(app_context->appConfig->at_config.serial_config.flow_control == FLOW_CONTROL_CTS_RTS) {
      strcpy(v_buf, "enable");
    }
  }else if( !strcmp( key, "FORMAT_MODE" ) ){
    if (app_context->appConfig->at_config.autoFormatEnable == true) {
      strcpy(v_buf, "enable");
    }else{
      strcpy(v_buf, "disable");
    }
  }else if( !strcmp( key, "FORMAT_LEN" ) ){
    sprintf(v_buf, "%d", app_context->appConfig->at_config.uart_auto_format.auto_format_len);
  }else if( !strcmp( key, "FORMAT_TIME" ) ){
    sprintf(v_buf, "%d", app_context->appConfig->at_config.uart_auto_format.auto_format_time);
  }else if( !strcmp( key, "CON1_PROTOCOL") ){            /*tcp/ip setting*/
    if (app_context->appConfig->at_config.conn1_config.protocol == TCP_SERVER) {
      strcpy(v_buf, "server");
    } else if (app_context->appConfig->at_config.conn1_config.protocol == TCP_CLIENT) {
      strcpy(v_buf, "client");
    } else if (app_context->appConfig->at_config.conn1_config.protocol == UDP_BOARDCAST) {
      strcpy(v_buf, "boardcast");
    } else if (app_context->appConfig->at_config.conn1_config.protocol == UDP_UNICAST) {
      strcpy(v_buf, "unicast");
    }
  }else if( !strcmp( key, "CON1_LPORT") ){
    sprintf(v_buf, "%ld", app_context->appConfig->at_config.conn1_config.localPort);
  }else if( !strcmp( key, "CON1_RPORT") ){
    sprintf(v_buf, "%ld", app_context->appConfig->at_config.conn1_config.remotePort);
  }else if( !strcmp( key, "CON1_RSERVER") ){
    strcpy(v_buf, app_context->appConfig->at_config.conn1_config.remoteServerDomain );
  }else if( !strcmp( key, "CON2_PROTOCOL") ){
    if ( app_context->appConfig->at_config.conn2_enable == false ){
      strcpy(v_buf, "disable");
    }else{
      if (app_context->appConfig->at_config.conn2_config.protocol == TCP_SERVER) {
        strcpy(v_buf, "server");
      } else if (app_context->appConfig->at_config.conn2_config.protocol == TCP_CLIENT) {
        strcpy(v_buf, "client");
      } else if (app_context->appConfig->at_config.conn2_config.protocol == UDP_BOARDCAST) {
        strcpy(v_buf, "boardcast");
      } else if (app_context->appConfig->at_config.conn2_config.protocol == UDP_UNICAST) {
        strcpy(v_buf, "unicast");
      }
    }
  }else if( !strcmp( key, "CON2_LPORT") ){
    sprintf(v_buf, "%ld", app_context->appConfig->at_config.conn2_config.localPort);
  }else if( !strcmp( key, "CON2_RPORT") ){
    sprintf(v_buf, "%ld", app_context->appConfig->at_config.conn2_config.remotePort);
  }else if( !strcmp( key, "CON2_RSERVER") ){
    strcpy(v_buf, app_context->appConfig->at_config.conn2_config.remoteServerDomain );
  }else if( !strcmp( key, "USER_NAME") ){     /*modle setting*/
    strcpy(v_buf, app_context->appConfig->at_config.web_login.login_username);
  }else if( !strcmp( key, "USER_KEY" ) ){
    strcpy(v_buf, app_context->appConfig->at_config.web_login.login_password);
  }else if( !strcmp( key, "MODEL" ) ){
    get_wifi_model_name(model);
    strcpy(v_buf, model);
  }else if( !strcmp( key, "MAC" ) ){
    strcpy(v_buf, mico_status_wlan->mac);
  }else if( !strcmp( key, "SDK_VER" ) ){
    strcpy(v_buf, MicoGetVer());
  }else if( !strcmp( key, "FW_VER" ) ){
    get_firmware_version(fver);
    strcpy(v_buf, fver);
  }else if( !strcmp( key, "REBOOT_MODE" ) ){
      if (app_context->appConfig->at_config.time_reboot.time_reboot_enable == true) {
        strcpy(v_buf, "enable");
      }else{
        strcpy(v_buf, "disable");
      }
  }else if( !strcmp( key, "REBOOT_TIME" ) ){
      sprintf(v_buf, "%ld", app_context->appConfig->at_config.time_reboot.time_reboot_time);
  }else if( !strcmp( key, "RUN_MODE" ) ){     /*run state*/
    if( (app_context->appStatus.wifi_open.ap_open == true) && (app_context->appStatus.wifi_open.sta_open == true) ){
       strcpy(v_buf, "ap_sta");
    }else if( app_context->appStatus.wifi_open.ap_open == true ){
      strcpy(v_buf, "ap");
    }else if( app_context->appStatus.wifi_open.sta_open == true ){
      strcpy(v_buf, "sta");
    }
  }else if( !strcmp( key, "STA_CH" ) ){         
    if( app_context->appStatus.wifi_state.wifi_state_sta == STATION_UP ){
      micoWlanGetLinkStatus(&link_state);
      sprintf(v_buf, "%d",  link_state.channel);
    }else{
      strcpy(v_buf, "0");
    }
  }else if( !strcmp( key, "STA_POWER" ) ){
    if( app_context->appStatus.wifi_state.wifi_state_sta == STATION_UP ){
      micoWlanGetLinkStatus(&link_state);
      sprintf(v_buf, "%d %%",  link_state.wifi_strength  );
    }else{
      strcpy(v_buf, "0");
    }
  }else if( !strcmp( key, "RUN_AT_FUN" ) ){
    if( (app_context->appStatus.at_mode == AT_MODE) || (app_context->appStatus.at_mode == AT_NO_MODE) ) {
       strcpy(v_buf, "1");
    }else{
      strcpy(v_buf, "0");
    }
  }else if( !strcmp( key, "RUN_CON1_FUN" ) ){
    sprintf(v_buf, "%d",  app_context->appStatus.connect_state.connect1_open);
  }else if( !strcmp( key, "RUN_T1S_NUM" ) ){
    sprintf(v_buf, "%d",  app_context->appStatus.connect_state.connect1_server_link_count);
  }else if( !strcmp( key, "RUN_T1C_NUM" ) ){
    sprintf(v_buf, "%d",  app_context->appStatus.connect_state.connect1_client_link);
  }else if( !strcmp( key, "RUN_CON2_FUN" ) ){
    sprintf(v_buf, "%d",  app_context->appStatus.connect_state.connect2_open);  
  }else if( !strcmp( key, "RUN_T2S_NUM" ) ){
    sprintf(v_buf, "%d",  app_context->appStatus.connect_state.connect2_server_link_count);
  }else if( !strcmp( key, "RUN_T2C_NUM" ) ){
    sprintf(v_buf, "%d",  app_context->appStatus.connect_state.connect2_client_link);  
  }else if( !strcmp( key, "RUN_TIME" ) ){
    sprintf(v_buf, "%ld ms",  mico_rtos_get_time());
  }else if( !strcmp( key, "STA_SCAN") ){
    
    mico_system_notify_register(mico_notify_WIFI_SCAN_COMPLETED, (void *)WEBNotify_ApListCallback, NULL);
    mico_rtos_init_semaphore(&scan_semaphore, 1);
    
    micoWlanStartScan();
	http_file_handle_log("start scan");
    
    mico_rtos_get_semaphore(&scan_semaphore, 5000);
    mico_rtos_deinit_semaphore(&scan_semaphore);
    mico_system_notify_remove(mico_notify_WIFI_SCAN_COMPLETED,(void *)WEBNotify_ApListCallback);
    for( int i=0; i<scan_count ; i++ ){
      sprintf(out_buf, "<tr><td><input type=radio name=selectedSSID onClick=\"selectedSSIDChange('%s')\"></td><td>%s</td><td>%d%%</td></tr>\r\n", scan_res[i].ssid, scan_res[i].ssid, scan_res[i].power);
      httpd_send_chunk(socket_web, out_buf, strlen(out_buf));
    }
    goto exit;
  }else{
    sprintf(out_buf, "var %s=\"0\";\r\n", value);
    httpd_send_chunk(socket_web, out_buf, strlen(out_buf));
    goto exit;
  }
  sprintf(out_buf, "var %s=\"%s\";\r\n", value, v_buf);
  httpd_send_chunk(socket_web, out_buf, strlen(out_buf));
  
exit:  
  if(out_buf)
    free(out_buf);
  return 0;
}

int http_replace_data_callback(char *data, unsigned int len)
{
  httpd_send_chunk(socket_web, data, len);
  return 0;
}

static int httpd_send_file(int conn, char *filename, bool is_replace)
{
  int err = kNoErr;
  uint32_t readlen = 0;
  int offset = 0;
#define buf_size 1024
  char buf[buf_size];
  http_replace_t h;
  char *rep_buf;
  if( is_replace == true ){
    rep_buf = malloc(buf_size);
  }
  socket_web = conn;
  
  memset(buf, '0', buf_size);
  do{
    err = read_file(buf, buf_size, &readlen, &offset);
    if( is_replace == true ){
      if( readlen != 0 ){
        http_replace_init(&h, rep_buf, buf_size);
        http_replace_process(&h, (uint8_t*)buf, readlen);
      }
   }else{
      if( readlen != 0 ){
        err = httpd_send_chunk(conn, buf, readlen);
      }
    }
  }while( readlen != 0);
  
  err = httpd_send_chunk(conn, buf, readlen);
  
  if( is_replace == true ){
    free(rep_buf);
  }
  return err;
}

static int httpd_send_header_form_str(int conn, const char *status_heaader, const char *file_type, const char *encoding)
{
   OSStatus err = kNoErr;
   
   /* Send the HTTP status header, 200/404, etc.*/
   err = httpd_send(conn, status_heaader, strlen(status_heaader));
   if (err != kNoErr) {
     return err;
   }
   
   /* Set enconding to gz if so */
   if (strncmp(encoding, http_gz, sizeof(http_gz) - 1) == 0) {
     err = httpd_send(conn, http_content_encoding_gz, sizeof(http_content_encoding_gz) - 1);
   }
   
   if (file_type == NULL) {
     err = httpd_send(conn, http_content_type_binary, sizeof(http_content_type_binary) - 1);
   } else if (strncmp(http_manifest, file_type, sizeof(http_manifest) - 1) == 0) {
     err = httpd_send(conn, http_content_type_text_cache_manifest, sizeof(http_content_type_text_cache_manifest) - 1);
   } else if (strncmp(http_html, file_type, sizeof(http_html) - 1) == 0) {
     err = httpd_send(conn, http_content_type_html, sizeof(http_content_type_html) - 1);
   } else if (strncmp(http_shtml, file_type, sizeof(http_shtml) - 1) == 0) {
     err = httpd_send(conn, http_content_type_html_nocache, sizeof(http_content_type_html_nocache) - 1);
   } else if (strncmp(http_css, file_type, sizeof(http_css) - 1) == 0) {
     err = httpd_send(conn, http_content_type_css, sizeof(http_content_type_css) - 1);
   } else if (strncmp(http_js, file_type, sizeof(http_js) - 1) == 0) {
     err = httpd_send(conn, http_content_type_js, sizeof(http_content_type_js) - 1);
   } else if (strncmp(http_png, file_type, sizeof(http_png) - 1) == 0) {
     err = httpd_send(conn, http_content_type_png, sizeof(http_content_type_png) - 1);
   } else if (strncmp(http_gif, file_type,  sizeof(http_gif) - 1) == 0) {
     err = httpd_send(conn, http_content_type_gif, sizeof(http_content_type_gif) - 1);
   } else if (strncmp(http_jpg, file_type, sizeof(http_jpg) - 1) == 0) {
     err = httpd_send(conn, http_content_type_jpg, sizeof(http_content_type_jpg) - 1);
   } else {
     err = httpd_send(conn, http_content_type_plain, sizeof(http_content_type_plain) - 1);
   }
   
   return err;
}

/**************************send msg**********************************************************/

static int web_get_config(httpd_request_t *req)
{
  OSStatus err = kNoErr;
  char buf[rec_ota_buf_size];
  
  char *archor = "/";
  char tempfile[HTTPD_MAX_URI_LENGTH + 1] = {0};
  char filename[HTTPD_MAX_URI_LENGTH + 1] = {0};
  char *ptr = NULL;
  char *file_type = NULL;
  const char *encoding = NULL;
  int pos = 0;
  char *p_auth;
  bool is_replace = false;
  char *auth_str = NULL;
  
  memset(buf, 0x00, rec_ota_buf_size);
  err = httpd_recv(req->sock, buf, rec_ota_buf_size, 0);
  if (err < 0)
    goto exit;
  
  auth_str = get_httpd_auth();
  
  if (auth_str != NULL) {
    p_auth = strstr(buf, auth_str);
    if (p_auth == NULL) { // un-authrized
      httpd_send(req->sock, (char*)httpd_authrized, strlen(httpd_authrized));
      goto exit;
    } else {
      p_auth += strlen(auth_str);
      if (*p_auth != 0x0d) {
        httpd_send(req->sock, (char*)httpd_authrized, strlen(httpd_authrized));
        goto exit;
      }
    }
  }
  
  ptr = strstr(req->filename, archor);
  if( ptr == NULL ){
    httpd_set_error("url err");
    err = httpd_send_error(req->sock, HTTP_500);
    goto exit;    
  }
  
  
  while( *ptr && (*ptr != '?') ){
    tempfile[pos] = *ptr;
    pos ++;
    ptr ++;
  }
  
  if( !strncmp(tempfile, "/", 2) ){
    strncpy(filename, "index.html", strlen("index.html"));
  }else if( !strncmp(tempfile, "/favicon.ico", strlen("/favicon.ico")+1) ){
    strncpy(filename, "index.html", strlen("index.html"));
  }else{
    strncpy(filename, &tempfile[1], strlen(&tempfile[1]));
  }
  
  file_type = strrchr(filename, ISO_period);
  if(file_type){
    http_file_handle_log("file tpye: %s", file_type);
    memset(tempfile, 0x00, sizeof(tempfile));
    strncpy(tempfile, filename, strlen(filename));
    strncat(tempfile, ".gz", 3);
    if( open_file(tempfile, 1) == 0 ){
      close_file();
      strncpy(filename, tempfile, strlen(tempfile));
      encoding = ".gz";
    }
  }
  
  http_file_handle_log("need open file: %s", filename);
  
  if(open_file(filename, 1) != 0){
    http_file_handle_log("open file false");
    httpd_set_error("no find file: %s", filename);
    err = httpd_send_error(req->sock, HTTP_404);
    goto exit;
  }else{
    httpd_send_header_form_str(req->sock, http_header_200, file_type, encoding);
    httpd_send_default_headers(req->sock, req->wsgi->hdr_fields);
    httpd_send_crlf(req->sock);
  }
  
  if( strrchr(filename, '$') ){
    is_replace = true;
  }
  
  if(httpd_send_file(req->sock, filename, is_replace) != 0){
    httpd_set_error("read file: %s", filename);
    err = httpd_send_error(req->sock, HTTP_500);
    goto exit;
  }
  if (kNoErr != close_file() ){
    http_file_handle_log("close file false");
  }
  
exit:
  return err;
}

static int web_post_config(httpd_request_t *req)
{
  OSStatus err = kNoErr;
  char buf[rec_ota_buf_size];
  int file_length, rec_length;
  int ota_length = 0;

  struct MHD_Connection connection;
  struct MHD_HTTP_Header header;
  struct MHD_PostProcessor *pp;
  unsigned int want_off = 0;
  char contnet_type_value[128];
  
  char *archor = "/";
  char tempfile[HTTPD_MAX_URI_LENGTH + 1] = {0};
  char filename[HTTPD_MAX_URI_LENGTH + 1] = {0};
  char *ptr = NULL;
  char *file_type = NULL;
  const char *encoding = NULL;
  int pos = 0;
  char *tempf = NULL;
  
  bool is_reboot_success = false;
  bool is_factory_success = false;
  
  ota_partition = MicoFlashGetInfo( MICO_PARTITION_OTA_TEMP );
  
  offset = 0x0;
  is_ota_success = false;
  is_config_success = false;
  
  ptr = strstr(req->filename, archor);
  if( ptr == NULL ){
    httpd_set_error("url err");
    err = httpd_send_error(req->sock, HTTP_500);
    goto exit;    
  }
  
  while( *ptr && (*ptr != '?') ){
    tempfile[pos] = *ptr;
    pos ++;
    ptr ++;
  }
  
  strncpy(filename, &tempfile[1], strlen(&tempfile[1]));
  
  file_type = strrchr(filename, ISO_period);
  if(file_type){
    http_file_handle_log("file tpye: %s", file_type);
    memset(tempfile, 0x00, sizeof(tempfile));
    strncpy(tempfile, filename, strlen(filename));
    strncat(tempfile, ".gz", 3);
    if( open_file(tempfile, 1) == 0 ){
      close_file();
      strncpy(filename, tempfile, strlen(tempfile));
      encoding = ".gz";
    }
  }

  
  if(open_file(filename, 1) != 0){
    httpd_set_error("no find file: %s", filename);
    err = httpd_send_error(req->sock, HTTP_404);
    err = -1;
    goto exit;
  }else{
    err = close_file();
  }
  
  if( (!strcmp( req->filename, "/reboot_success.html")) || (!strcmp( req->filename, "/reboot_success_en.html")) ){
    is_reboot_success = true;
    is_config_success = true;
  }else if( (!strcmp( req->filename, "/factory_success.html")) || (!strcmp( req->filename, "/factory_success_en.html")) ){
    is_factory_success = true;
    is_config_success = true;
  }
  
  err = httpd_parse_hdr_tags(req, req->sock, buf, rec_ota_buf_size);
  require_noerr(err, exit);
  
  file_length = req->body_nbytes;
  ota_length = req->body_nbytes;
  if( !strncmp("multipart/form-data;", req->content_type, strlen("multipart/form-data;")) ){
    sprintf(contnet_type_value, "%s, %s", MHD_HTTP_POST_ENCODING_MULTIPART_FORMDATA,  &req->content_type[strlen("multipart/form-data; ")]);
  }else if( !strncmp(MHD_HTTP_POST_ENCODING_FORM_URLENCODED, req->content_type, strlen(MHD_HTTP_POST_ENCODING_FORM_URLENCODED)) ){
    sprintf(contnet_type_value, "%s", MHD_HTTP_POST_ENCODING_FORM_URLENCODED);
  }
  
  http_file_handle_log("content-type=%s; length=%d", contnet_type_value, req->body_nbytes);
  
  memset (&connection, 0, sizeof (struct MHD_Connection));
  memset (&header, 0, sizeof (struct MHD_HTTP_Header));
  connection.headers_received = &header;
  header.header = MHD_HTTP_HEADER_CONTENT_TYPE;
  header.value = contnet_type_value;
  header.kind = MHD_HEADER_KIND;
  pp = MHD_create_post_processor (&connection, 1024, &value_checker, &want_off);
  
  while(file_length > 0) {
    memset(buf, 0x00, rec_ota_buf_size);
    rec_length = recv(req->sock, buf, (file_length > rec_ota_buf_size) ? rec_ota_buf_size : file_length, 0);
    if(rec_length < 0){
      err = -1;
      goto exit;
    }
    MHD_post_process (pp, buf, rec_length);
    file_length -= rec_length;
  }
  MHD_destroy_post_processor (pp);
  
  //fail is replace
  
  if( is_config_success == false){
    tempf = strrep(filename, "success", "fail");
    strcpy(filename, tempf);
    if( tempf )
      free(tempf);
  }
  
  http_file_handle_log("need open file: %s", filename);
  
  if(open_file(filename, 1) != 0){
    httpd_set_error("no find file: %s", filename);
    err = httpd_send_error(req->sock, HTTP_404);
    goto exit;
  }else{
    httpd_send_header_form_str(req->sock, http_header_200, file_type, encoding);
    httpd_send_default_headers(req->sock, req->wsgi->hdr_fields);
    httpd_send_crlf(req->sock);
  }
  
  if(httpd_send_file(req->sock, filename, false) != 0){
    httpd_set_error("read file: %s", filename);
    err = httpd_send_error(req->sock, HTTP_500);
  }
  err = close_file();
  
exit:
  if( is_reboot_success == true ){
   goto reset;
  }
  
  if( is_factory_success == true ){
    mico_system_context_restore(mico_context);
    goto reset;
  }
  
  if( is_ota_success == true ){
      http_file_handle_log("ota...");
    CRC16_Final( &crc16_context, &crc);
//    memset(&mico_context->bootTable, 0, sizeof(boot_table_t));
//    mico_context->bootTable.length = ota_length;
//    mico_context->bootTable.start_address = ota_partition->partition_start_addr;
//    mico_context->bootTable.type = 'A';
//    mico_context->bootTable.upgrade_type = 'U';
//    mico_context->bootTable.crc = crc;
    mico_ota_switch_to_new_fw(ota_length, crc);
    mico_system_context_update(mico_context);
    goto reset;
  }
  
  if( is_config_success == true ){
    mico_system_context_update(mico_context);
  }
  
  return err;
  
reset:
  mico_system_power_perform( mico_context, eState_Software_Reset );
  mico_thread_sleep( MICO_WAIT_FOREVER ); 
  return err;
}


/*************************************************************************************************************/

void http_sys_register_file_handlers(void *app_context_c)
{
  int rc;
  app_context = app_context_c;
  mico_context = mico_system_context_get();

  rc = httpd_register_wsgi_handlers(g_sys_file_handlers, g_sys_file_handlers_no);
  if (rc) {
    http_file_handle_log("failed to register test web handler");
  }
}

static struct httpd_wsgi_call g_sys_file_handlers[] = {
  {"/", HTTPD_DEFAULT_HDR_FLAGS, APP_HTTP_FLAGS_NO_EXACT_MATCH, web_get_config, web_post_config, NULL, NULL},
};

static int g_sys_file_handlers_no = sizeof(g_sys_file_handlers)/sizeof(struct httpd_wsgi_call);

