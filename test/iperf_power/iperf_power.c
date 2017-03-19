#include "MICO.h"
#include "command_console/mico_cli.h"
#include "mico_api.h"

#define wifi_station_log(M, ...) custom_log("WIFI", M, ##__VA_ARGS__)

static int uap_up = -1;
static void micoNotify_ConnectFailedHandler(OSStatus err, void* inContext)
{
  wifi_station_log("join Wlan failed Err: %d", err);
}

static void micoNotify_WifiStatusHandler(WiFiEvent event,  void* inContext)
{
  switch (event) 
  {
  case NOTIFY_STATION_UP:
    wifi_station_log("Station up");
    break;
  case NOTIFY_STATION_DOWN:
    wifi_station_log("Station down");
    break;
  case NOTIFY_AP_UP:
  	uap_up = 1;
  	wifi_station_log("AP up");
	break;
  case NOTIFY_AP_DOWN:
  	uap_up = 0;
  	wifi_station_log("AP down");
  	break;
  default:
    break;
  }
}

static void connect_ap(char *ssid, char *key)
{
	
  network_InitTypeDef_adv_st  wNetConfigAdv={0};
  /* Initialize wlan parameters */
  strcpy((char*)wNetConfigAdv.ap_info.ssid, ssid);   /* wlan ssid string */
  if (key) {
  	strcpy((char*)wNetConfigAdv.key, key);                /* wlan key string or hex data in WEP mode */
  	wNetConfigAdv.key_len = strlen(key);                  /* wlan key length */
  } else {
	  wNetConfigAdv.key_len = 0;
  }
  wNetConfigAdv.ap_info.security = SECURITY_TYPE_AUTO;          /* wlan security mode */
  wNetConfigAdv.ap_info.channel = 0;                            /* Select channel automatically */
  wNetConfigAdv.dhcpMode = DHCP_Client;                         /* Fetch Ip address from DHCP server */
  wNetConfigAdv.wifi_retry_interval = 100;                      /* Retry interval after a failure connection */
  
  /* Connect Now! */
  wifi_station_log("connecting to %s...", wNetConfigAdv.ap_info.ssid);
  micoWlanStartAdv(&wNetConfigAdv);
}

static void ap_start(char *ssid, char *key)
{
	network_InitTypeDef_st wNetConfig;
  
  memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_st));
  
  strcpy((char*)wNetConfig.wifi_ssid, ssid);
  if (key)
  	strcpy((char*)wNetConfig.wifi_key, key);
  
  wNetConfig.wifi_mode = Soft_AP;
  wNetConfig.dhcpMode = DHCP_Server;
  wNetConfig.wifi_retry_interval = 100;
  strcpy((char*)wNetConfig.local_ip_addr, "192.168.0.1");
  strcpy((char*)wNetConfig.net_mask, "255.255.255.0");
  strcpy((char*)wNetConfig.dnsServer_ip_addr, "192.168.0.1");
  
  wifi_station_log("ssid:%s  key:%s", wNetConfig.wifi_ssid, wNetConfig.wifi_key);
  micoWlanStart(&wNetConfig);
}

static void standby_Command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	int sleeptime = 0;
	if (argc == 2)
		sleeptime = argv[1][0] - '0';
	
    printf("goto standby mode, sleep %d seconds\r\n", sleeptime);
	msleep(10);
	MicoSystemStandBy(sleeptime);
}

static void ps_cmd_handler(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
  if (argc == 1) {
    cmd_printf("Usage: ps on/off. \r\n");
    return;
  }
  
  if (!strcasecmp(argv[1], "on")) {
    cmd_printf("Enable IEEE Power save\r\n");
	ps_enable();
  } else if (!strcasecmp(argv[1], "off")) {
    cmd_printf("Disable IEEE Power save\r\n");
	ps_disable();
  } else {
	cmd_printf("Usage: ps on/off. \r\n");
  }
}

static void ap_command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	if (uap_up == 1)
		return;
	
	if (argc == 2) {
		cmd_printf("start AP ssid=%s\r\n", argv[1]);
		ap_start(argv[1], NULL);
	} else if (argc == 3) {
		cmd_printf("start AP ssid=%s, key=%s\r\n", argv[1], argv[2]);
		ap_start(argv[1], argv[2]);
	} else {
		cmd_printf("Usage: ap <ssid> [<key>]");
	}
}

static void conn_command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	if (argc == 2) {
		cmd_printf("Connect to AP ssid=%s\r\n", argv[1]);
		connect_ap(argv[1], NULL);
	} else if (argc == 3) {
		cmd_printf("Connect to AP ssid=%s, key=%s\r\n", argv[1], argv[2]);
		
		connect_ap(argv[1], argv[2]);
	} else {
		cmd_printf("Usage: connect <ssid> [<key>]");
	}
}
static void timer_command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	mico_rtc_time_t t;

	MicoRtcGetTime(&t);
		cmd_printf("%d-%d-%d %d:%d:%d\r\n",
			t.year, t.month, t.date, t.hr, t.min, t.sec);
	memset(&pcWriteBuffer[xWriteBufferLen], 0, 1000);
}

static void ntp_command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	sntp_client_start();
}

static void scan_test_command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	micoWlanStartScan( );
}

static void aon_test_command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	uint8_t val[100];
	int i;
	uint8_t t;

	if (argc == 1) {
		aon_read(0, val, sizeof(val));
		for(i=0;i<100;i++) {
			cmd_printf("%02x ", val[i]);
		}
		cmd_printf("\r\n");
		return;
	} else {
		t = strtoul(argv[1], NULL, 0);
		for(i=0;i<100;i++) {
			val[i] = t;
		}
		cmd_printf("Set to %02x\r\n", t);
		aon_write(0, val, sizeof(val));
	}

	
}

static void pwm_test_command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	uint8_t val[100];
	int i;
	uint8_t t;
	int port, freq, d;
	float duty = 50.0;

	if (argc != 4) {
		cmd_printf("Usage: pwm <id> <freq> <duty>. \r\n");
		return;
	}
	port = strtoul(argv[1], NULL, 0);
	freq = strtoul(argv[2], NULL, 0);
	d = strtoul(argv[3], NULL, 0);
	duty = (float)d;
	printf("d %d, duty %f\r\n", d, duty);
	MicoPwmInitialize(port, freq, duty);
	MicoPwmStart(port);	
}

static void gpio_test_command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	int port, on;

	port = strtoul(argv[1], NULL, 0);
	on = strtoul(argv[2], NULL, 0);
	cmd_printf("put port %d, %d\r\n", port, on);
	MicoGpioInitialize(port, OUTPUT_PUSH_PULL);
	if (on)
		MicoGpioOutputHigh(port);
	else
		MicoGpioOutputLow(port);
}

static void udp_test_command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	int fd;
	struct sockaddr_t addr;
	
	fd = socket( AF_INET, SOCK_DGRM, IPPROTO_UDP );
	addr.s_ip = INADDR_ANY;
    addr.s_port = 0;
    bind( fd, &addr, sizeof(addr) );
	cmd_printf("create udp fd %d\r\n", fd);
}
static void tcp_test_command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	int fd;
	struct sockaddr_t addr;
	
	fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

	cmd_printf("create tcp fd %d\r\n", fd);
}
typedef  struct _wifi_mgmt_frame_tx {
	/** Packet Length */
	uint16_t frm_len;
	/** Frame Control */
	uint16_t frm_ctl;
	/** Duration ID */
	uint16_t duration_id;
	/** Address1 */
	uint8_t addr1[6];
	/** Address2 */
	uint8_t addr2[6];
	/** Address3 */
	uint8_t addr3[6];
	/** Sequence Control */
	uint16_t seq_ctl;
	/** Address4 */
	uint8_t addr4[6];
	/** Frame payload */
	uint8_t payload[1];
}  wifi_mgmt_frame_tx;
#define BUFFER_LENGTH 2048

static void inject_fram_command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	/** Modify below values to inject different type and subtypes of
	 * frames */
	uint16_t type = 0x00, subtype = 0x04;
	uint16_t seq_num = 0, frag_num = 0, from_ds = 0, to_ds = 0;
	uint8_t dest[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	uint8_t bssid[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	uint8_t data[] = {0x01, 0x01, 0x00, 0x0c, 0x00, 0x58, 0x02, 0x40};
	uint8_t probe_req[] = {0x00,0x00,0x01,0x08,0x02,0x04,0x0B,0x0C,0x12,0x16,
		0x18,0x24,0x03,0x01,0x04,0x2D,0x1A,0x00,0x00,0x03,0xFF,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x32,0x04,0x30,0x48,0x60,0x6C,};
	/*
		type=0x00
			# MGMT frames	       0
			# CONTROL frames       1
			# DATA frames          2


		subtype=0x05
		    for MGMT frames(type: 0x00)
			# Assoc Request        0
			# Assoc Response       1
			# Re-Assoc Request     2
			# Re-Assoc Response    3
			# Probe Request        4
			# Probe Response       5
			# Beacon               8
			# Atim                 9
			# Dis-assoc            10
			# Auth                 11
			# Deauth               12
			# Action Frame         13
		    for CONTROL frames(type: 0x01)
			# Block ACK	       10
			# RTS		       11
			# ACK		       13
		    for DATA frames(type: 0x02)
			# Data		       0
			# Null (no data)       4
			# QoS Data	       8
			# QoS Null (no data)   12

	 * seq_num means Sequence number
	 * frag_num means Fragmentation number
	 * to_ds = 1 means To the Distribution system
	 * from_ds = 1 means Exit from the Distribution system
	 * dest Destination MAC address
	 * bssid BSSID
	 * data Data in payload.
	 */

	/* Do not modify below code as it prepares WLAN 802.11
	 * frame from values specified above.
	 */
	uint16_t data_len = sizeof(wifi_mgmt_frame_tx);
	unsigned char mac[6] = { 0 };
	uint8_t *buffer = NULL;
	wifi_mgmt_frame_tx *pmgmt_frame = NULL;

	buffer = (uint8_t *)malloc(BUFFER_LENGTH);
	if (!buffer) {
		printf("ERR:Cannot allocate memory!\r\n");
		return;
	}

	memset(buffer, 0, BUFFER_LENGTH);

	wlan_get_mac_address(mac);

	pmgmt_frame = (wifi_mgmt_frame_tx *)(buffer);

	pmgmt_frame->frm_ctl = (type & 0x3) << 2;

	pmgmt_frame->frm_ctl |= subtype << 4;

	memcpy(pmgmt_frame->addr1, dest, 6);
	memcpy(pmgmt_frame->addr2, mac, 6);
	memcpy(pmgmt_frame->addr3, bssid, 6);

	memcpy(pmgmt_frame->payload, probe_req, sizeof(probe_req));
	data_len += sizeof(probe_req);

	pmgmt_frame->seq_ctl = seq_num << 4;
	pmgmt_frame->seq_ctl |= frag_num;
	pmgmt_frame->frm_ctl |= (from_ds & 0x1) << 9;
	pmgmt_frame->frm_ctl |= (to_ds & 0x1) << 8;

	pmgmt_frame->frm_len = data_len - sizeof(pmgmt_frame->frm_len);

	wlan_inject_frame(buffer, data_len);

	if (buffer)
		free(buffer);
}
static const uint8_t fix_para[] = {
	0x42,0x21,0x5B,0x02,0x00,0x00,0x00,0x00,0xC8,0x00,0x21,0x04};
	
static const uint8_t beacon_tlv[] = {
	0x01,0x08,0x82,0x84,0x8B,0x96,0x0C,0x12,0x18,0x24,
	0x05,0x05,0x05,0x0A,0x00,0x02,0x00,
	0x2A,0x01,0x00,
	0x32,0x04,0x30,0x48,0x60,0x6C};
	
static void beacon_tx(uint8_t *ssid, uint8_t ssid_len, uint8_t *vendor, uint8_t vendor_len)
{
	/** Modify below values to inject different type and subtypes of
	 * frames */
	uint16_t type = 0x00, subtype = 0x08;
	uint8_t dest[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	unsigned char mac[6] = { 0 };

	/* Do not modify below code as it prepares WLAN 802.11
	 * frame from values specified above.
	 */
	uint16_t data_len = 0;//sizeof(wifi_mgmt_frame_tx);
	
	uint8_t *buffer = NULL;
	wifi_mgmt_frame_tx *pmgmt_frame = NULL;

	buffer = (uint8_t *)malloc(BUFFER_LENGTH);
	if (!buffer) {
		printf("ERR:Cannot allocate memory!\r\n");
		return;
	}

	memset(buffer, 0, BUFFER_LENGTH);

	wlan_get_mac_address(mac);

	pmgmt_frame = (wifi_mgmt_frame_tx *)(buffer);

	pmgmt_frame->frm_ctl = (type & 0x3) << 2;

	pmgmt_frame->frm_ctl |= subtype << 4;

	memcpy(pmgmt_frame->addr1, dest, 6);
	memcpy(pmgmt_frame->addr2, mac, 6);
	memcpy(pmgmt_frame->addr3, mac, 6);

	memcpy(pmgmt_frame->payload, fix_para, sizeof(fix_para));
	data_len += sizeof(fix_para);
	
	pmgmt_frame->payload[data_len++]= 0; // ssid tlv
	pmgmt_frame->payload[data_len++]= ssid_len;
	memcpy(&pmgmt_frame->payload[data_len], ssid, ssid_len);
	data_len += ssid_len;
	
	memcpy(&pmgmt_frame->payload[data_len], beacon_tlv, sizeof(beacon_tlv));
	data_len += sizeof(beacon_tlv);
	
	pmgmt_frame->payload[data_len++]= 0xDD; // custom IE
	pmgmt_frame->payload[data_len++]= vendor_len;
	memcpy(&pmgmt_frame->payload[data_len], vendor, vendor_len);
	data_len += vendor_len;

	data_len += sizeof(wifi_mgmt_frame_tx);
	data_len -= 2;
	
	pmgmt_frame->frm_len = data_len - 2;
	wlan_inject_frame(buffer, data_len);
	
	if (buffer)
		free(buffer);
}
static void data_tx(uint8_t *rawdata, uint8_t rawdata_len)
{
	/** Modify below values to inject different type and subtypes of
	 * frames */
	uint16_t type = 0x2, subtype = 0x00;
	uint8_t dest[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	unsigned char mac[6] = { 0 };

	/* Do not modify below code as it prepares WLAN 802.11
	 * frame from values specified above.
	 */
	uint16_t data_len = sizeof(wifi_mgmt_frame_tx) - 2;
	
	uint8_t *buffer = NULL;
	wifi_mgmt_frame_tx *pmgmt_frame = NULL;

	buffer = (uint8_t *)malloc(BUFFER_LENGTH);
	if (!buffer) {
		printf("ERR:Cannot allocate memory!\r\n");
		return;
	}

	memset(buffer, 0, BUFFER_LENGTH);

	wlan_get_mac_address(mac);

	pmgmt_frame = (wifi_mgmt_frame_tx *)(buffer);

	pmgmt_frame->frm_ctl = (type & 0x3) << 2;

	pmgmt_frame->frm_ctl |= subtype << 4;

	memcpy(pmgmt_frame->addr1, dest, 6);
	memcpy(pmgmt_frame->addr2, mac, 6);
	memcpy(pmgmt_frame->addr3, mac, 6);

	memcpy(pmgmt_frame->payload, rawdata, rawdata_len);
	data_len += rawdata_len;
	
	pmgmt_frame->frm_len = data_len - 2;
	wlan_inject_frame(buffer, data_len);

	if (buffer)
		free(buffer);
}

static void beacon_command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	beacon_tx("yhb", 3, "this is a custom IE", strlen("this is a custom IE"));
}
static void data_command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	data_tx("yhb", 3);
}

static void easylink_command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	
	cmd_printf("easylink...\r\n");
	micoWlanStartEasyLinkPlus(60);
	ssl_connect(1, 100, NULL, NULL);
}

void print_mac(const char *mac)
{
	printf("%02X:%02X:%02X:%02X:%02X:%02X ", mac[0], mac[1], mac[2],
		       mac[3], mac[4], mac[5]);
}

static void process_frame(const enum wlan_bss_type bss_type,
			const wlan_mgmt_frame_t *frame, const uint16_t len)
{
	if (frame->frame_type == PROBE_REQ_FRAME) {
		printf("%s: Received probe req from ", bss_type == 1 ? "uAP" : "sta");
		print_mac((const char *)frame->addr2);
		printf("with payload \r\n");
		printf("\r\n");
	} else if (frame->frame_type == ASSOC_REQ_FRAME) {
		printf("%s: Received association req from ", bss_type == 1 ? "uAP" : "sta");
		print_mac((const char *)frame->addr2);
		printf("\r\n");
	} else if (frame->frame_type == PROBE_RESP_FRAME) {
		printf("%s: Received probe resp from ", bss_type == 1 ? "uAP" : "sta");
		print_mac((const char *)frame->addr2);
		printf("\r\n");
	} else if (frame->frame_type == ASSOC_RESP_FRAME) {
		printf("%s: Received association resp from ", bss_type == 1 ? "uAP" : "sta");
		printf("\r\n");
	}
}

/** This RX Management frame callback is called from a thread with small stack size,
 * So do minimal memory allocations for correct behaviour.
 */
void rx_mgmt_cb(const enum wlan_bss_type bss_type,
		const wlan_mgmt_frame_t *frame, const uint16_t len)
{
	if (frame)
		process_frame(bss_type, frame, len);
}

static void rxmgmt_command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	uint32_t type = 0xFF;

	cmd_printf("register rxmgmt %x\r\n", type);
	wlan_rx_mgmt_indication(WLAN_BSS_TYPE_STA, 0x20, rx_mgmt_cb);
}

static void channel_command(char *pcWriteBuffer, int xWriteBufferLen,int argc, char **argv)
{
	uint8_t channel = argv[1][0] - '0';

	wlan_remain_on_channel(false, channel, 1000);
	cmd_printf("set channel %d\r\n", channel);
	wlan_remain_on_channel(true, channel, 1000000);

}



static const struct cli_command user_cmds[] = {
  {"ap", NULL, ap_command},
  {"connect", NULL, conn_command},
  {"iperf",    "iperf",            iperf_Command},
  {"standby",    "standby",            standby_Command},
  {"ps",    "wifi power save",            ps_cmd_handler},
  {"timer",    "time",            timer_command},
  {"ntp",    "ntp get time",            ntp_command},
  {"t",    "",            scan_test_command},
  {"aon",    "",            aon_test_command},
  {"pwm",    "",            pwm_test_command},
  {"gpio",    "",            gpio_test_command},
  {"udp",    "",            udp_test_command},
  {"tcp",    "",            tcp_test_command},
  {"inject",    "",            inject_fram_command},
  {"beacon",    "",            beacon_command},
  {"data",    "",            data_command},
  {"easylink", "", easylink_command},
  {"mgmt", "", rxmgmt_command},
  {"channel", "", channel_command},
};


static void printf_thread(void*arg)
{
	int i=0;
	while(1) {
		msleep(500);
		printf("%s %d\r\n", __FUNCTION__, i++);
	}
}

static void micoNotify_ApListCallback(ScanResult *pApList)
{
	if (pApList->ApNum < 2) {
		printf("\r\n!!!!!!!!!!!!!!\r\n\r\n");
	}
  printf("got %d AP!!\r\n", pApList->ApNum);

}


int application_start( void )
{
  OSStatus err = kNoErr;
  int i = 0;
  int val;
  mico_rtc_time_t t;
  mico_system_config_t cfg;
  float test = 1.02;
  
  cfg.country_code = 9;
  cfg.enable_healthmon = 1;
  system_config_set(&cfg);
  MicoInit( );

  msleep(100);

  printf("test is %f\r\n", test);
  MicoRtcInitialize();
  cli_init();
  cli_printf("\r\n# ");
  cli_register_commands(user_cmds, sizeof(user_cmds)/sizeof(struct cli_command));
  /* Register user function when wlan connection status is changed */
  err = mico_system_notify_register( mico_notify_WIFI_STATUS_CHANGED, (void *)micoNotify_WifiStatusHandler, NULL );
  require_noerr( err, exit ); 

  mico_system_notify_register( mico_notify_WIFI_SCAN_COMPLETED, (void *)micoNotify_ApListCallback, NULL );
  
  /* Register user function when wlan connection is faile in one attempt */
  err = mico_system_notify_register( mico_notify_WIFI_CONNECT_FAILED, (void *)micoNotify_ConnectFailedHandler, NULL );
  require_noerr( err, exit );
  

	
  //mico_rtos_create_thread(NULL, 4, "", printf_thread, 0x200, NULL);
  while(1) {
  	msleep(200);
	MicoGpioOutputTrigger((mico_gpio_t)MICO_SYS_LED);
	//MicoRtcGetTime(&t);
	//printf("%02dƒÍ%02d‘¬%02d»’ %02d:%02d:%02d\r\n",
	//		t.year, t.month, t.date, t.hr, t.min, t.sec);
  }
  
exit:
  mico_rtos_delete_thread(NULL);
  return err;
}

