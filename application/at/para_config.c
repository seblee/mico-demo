#include "mico.h"
#include "MICOAppDefine.h"

static app_context_t *app_context;
static mico_Context_t * mico_context;

void para_restore_mxchip_default(mico_Context_t * mico_context)
{
  application_config_t* application_config = mico_system_context_get_user_data(mico_context);
  
  memset(&application_config->factory_config, 0x0, sizeof(factory_config_t));
  
  application_config->factory_config.factoryConfigDataVer = FACTORYCONFIGURATION_VERSION;
  application_config->factory_config.at_mode = DATA_MODE;
  application_config->factory_config.event = false;
  application_config->factory_config.bonjourEnable = true;
  application_config->factory_config.wifi_mode = AP;
  
  sprintf(application_config->factory_config.ap_config.ssid,"MXCHIP+MAC"); 
  strcpy(application_config->factory_config.ap_config.key,"");
  application_config->factory_config.ap_config.channel = 6;  //auto
  
  sprintf(application_config->factory_config.ap_ip_config.localIp, "10.10.10.1");
  sprintf(application_config->factory_config.ap_ip_config.netMask, "255.255.255.0");
  sprintf(application_config->factory_config.ap_ip_config.gateWay, "10.10.10.1");
  
  application_config->factory_config.dhcpEable = true;
  
  application_config->factory_config.wifi_config_timeout = 60; //s
  
  application_config->factory_config.conn1_config.protocol = TCP_SERVER;
  application_config->factory_config.conn1_config.localPort = 8080;
  
  application_config->factory_config.conn2_enable = false;
  
  application_config->factory_config.serial_config.baud_rate = UART_BANDRATE_115200;
  application_config->factory_config.serial_config.data_width = DATA_WIDTH_8BIT;
  application_config->factory_config.serial_config.stop_bits = STOP_BITS_1;
  application_config->factory_config.serial_config.parity = NO_PARITY;
  application_config->factory_config.serial_config.flow_control = FLOW_CONTROL_DISABLED;
  
  application_config->factory_config.webEnable = true;
  sprintf(application_config->factory_config.web_login.login_username, "admin");
  sprintf(application_config->factory_config.web_login.login_password, "admin");
}

void para_restore_default(void)
{
  char model[10];
  mico_context = mico_system_context_get();
  application_config_t* application_config = mico_system_context_get_user_data(mico_context);
  
  memset( &application_config->at_config, 0x0, sizeof(at_config_t) );
  
  if( application_config->factory_config.factoryConfigDataVer != FACTORYCONFIGURATION_VERSION ){
    para_restore_mxchip_default(mico_context);
  }
  
  get_wifi_model_name(model);
  sprintf(mico_context->micoSystemConfig.name, model);
  application_config->at_config.at_mode = application_config->factory_config.at_mode;
  application_config->at_config.at_echo = false;
  application_config->at_config.event = application_config->factory_config.event;
  application_config->at_config.bonjourEnable = application_config->factory_config.bonjourEnable;
  application_config->at_config.wifi_mode = application_config->factory_config.wifi_mode;
  
  strcpy(application_config->at_config.ap_config.ssid, application_config->factory_config.ap_config.ssid);
  strcpy(application_config->at_config.ap_config.key, application_config->factory_config.ap_config.key);
  application_config->at_config.ap_config.channel = application_config->factory_config.ap_config.channel;
  
  strcpy(application_config->at_config.ap_ip_config.localIp, application_config->factory_config.ap_ip_config.localIp);
  strcpy(application_config->at_config.ap_ip_config.netMask, application_config->factory_config.ap_ip_config.netMask);
  strcpy(application_config->at_config.ap_ip_config.gateWay, application_config->factory_config.ap_ip_config.gateWay);
  
  mico_context->micoSystemConfig.dhcpEnable = application_config->factory_config.dhcpEable;
  
  application_config->at_config.wifi_config_timeout = application_config->factory_config.wifi_config_timeout;
  
  application_config->at_config.conn1_config.protocol = application_config->factory_config.conn1_config.protocol;
  application_config->at_config.conn1_config.localPort = application_config->factory_config.conn1_config.localPort;
  application_config->at_config.conn1_config.remotePort = application_config->factory_config.conn1_config.remotePort;
  strcpy(application_config->at_config.conn1_config.remoteServerDomain, application_config->factory_config.conn1_config.remoteServerDomain);
  application_config->at_config.conn1_config.server_num = MAX_TCP_CLIENT_PER_SERVER;
  
  application_config->at_config.conn2_enable = application_config->factory_config.conn2_enable;
  application_config->at_config.conn2_config.protocol = application_config->factory_config.conn2_config.protocol;
  application_config->at_config.conn2_config.localPort = application_config->factory_config.conn2_config.localPort;
  application_config->at_config.conn2_config.remotePort = application_config->factory_config.conn2_config.remotePort;
  strcpy(application_config->at_config.conn2_config.remoteServerDomain, application_config->factory_config.conn2_config.remoteServerDomain);
  application_config->at_config.conn2_config.server_num = MAX_TCP_CLIENT_PER_SERVER;
  
  application_config->at_config.serial_config.baud_rate = application_config->factory_config.serial_config.baud_rate;
  application_config->at_config.serial_config.data_width = application_config->factory_config.serial_config.data_width;
  application_config->at_config.serial_config.stop_bits = application_config->factory_config.serial_config.stop_bits;
  application_config->at_config.serial_config.parity = application_config->factory_config.serial_config.parity;
  application_config->at_config.serial_config.flow_control = application_config->factory_config.serial_config.flow_control;
  
  application_config->at_config.webEnable = application_config->factory_config.webEnable;
  strcpy(application_config->at_config.web_login.login_username, application_config->factory_config.web_login.login_username);
  strcpy(application_config->at_config.web_login.login_password, application_config->factory_config.web_login.login_password);
  
  application_config->at_config.bonjourEnable = application_config->factory_config.bonjourEnable;
  
  application_config->at_config.autoFormatEnable = false;
  application_config->at_config.uart_auto_format.auto_format_len = 1024;
  application_config->at_config.uart_auto_format.auto_format_time = 500;
  
  application_config->at_config.time_reboot.time_reboot_enable = false;
  application_config->at_config.time_reboot.time_reboot_time = 60;

   mico_context->micoSystemConfig.easyLinkByPass = 1;
}

void app_state_config(void *app_context_t)
{
  app_context = app_context_t;
  mico_Context_t *mico_context = mico_system_context_get();
  
  if (app_context->appConfig->at_config.at_mode == AT_MODE) {
    app_context->appStatus.at_mode = AT_MODE;
  } else if( app_context->appConfig->at_config.at_mode == AT_NO_MODE ){
    app_context->appStatus.at_mode = AT_NO_MODE;
  } else {
    app_context->appStatus.at_mode = DATA_MODE;
  }
  
  if( mico_context->micoSystemConfig.easyLinkByPass == 0 ){
    mico_context->micoSystemConfig.easyLinkByPass = 1;
    app_context->appConfig->at_config.wifi_config = CONFIG_EASYLINK;
    mico_system_context_update(mico_context);
  }

  if (app_context->appConfig->at_config.at_echo == true) {
    app_context->appStatus.at_echo = true;
  } else {
    app_context->appStatus.at_echo = false;
  }
  
  app_context->appStatus.wifi_state.wifi_state_ap = UAP_DOWN;
  app_context->appStatus.wifi_state.wifi_state_sta = STATION_DOWN;
  
}
