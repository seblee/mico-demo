mico-demos
====
包含：一系列基于MiCO的示例程序。

### 目录

* os：操作系统功能示例程序，包括：
 * [os_thread](os/os_thread/ReadMe.md)
 * [os_sem](os/os_sem/ReadMe.md)
 * [os_mutex](os/os_mutex/ReadMe.md)
 * [os_queue](os/os_queue/ReadMe.md)
 * [os_timer](os/os_timer/ReadMe.md)
 

* wifi：无线wifi功能实现示例程序，包括：
 * [scan](wifi/scan/ReadMe.md)
 * [soft_ap](wifi/soft_ap/ReadMe.md)
 * [station](wifi/station/ReadMe.md)


* net：网络通信功能示例程序，包括：
 * [dns](net/dns/ReadMe.md) 
 * [gagent](net/gagent/ReadMe.md)
 * [http_client](net/http/http_client_demo/ReadMe.md)
 * [http_server](net/http/http_server_demo/ReadMe.md)
 * [mDNS](net/mDNS/ReadMe.md)
 * [mqtt_client](net/mqtt_client/ReadMe.md)
 * [ota](net/ota/ReadMe.md)
 * [sntp_client](net/sntp_client/ReadMe.md)
 * [tcp_client](net/tcp_client/ReamMe.md)
 * [tcp_server](net/tcp_server/ReadMe.md)
 * [tls_server](net/tls_server/ReadMe.md)
 * [udp_broadcast](net/udp_broadcast/ReadMe.md)
 * [udp_unicast](net/udp_unicast/ReadMe.md)
 * [websocket](net/websocket/ReadMe.md)

* [helloworld](hardware/helloworld/ReadMe.md)：最简单的MiCO入门级应用程序。


* application：应用程序示例,包括：
 * [alink](application/alink/ReadMe.md) 
 * [at](application/at/ReadMe.md)
 * [homekit](application/homekit/ReadMe.md) 
 * [wifi_uart](application/wifi_uart/ReadMe.md)
 
* bluetooth：蓝牙功能相关示例程序，包括：
 * [ble_ access_center](bluetooth/ble_access_center/ReadMe.md) 
 * [ble_advertisements](bluetooth/ble_advertisements/ReadMe.md) 
 * [ble_ hello_center](bluetooth/ble_hello_center/ReadMe.md)
 * [ble_ hello_sensor](bluetooth/ble_hello_sensor/ReadMe.md)
 * [ble_scan](bluetooth/ble_scan/ReadMe.md)
 * [ble_ whitelist_connect](bluetooth/ble_whitelist_connect/ReadMe.md)
 * [ble_ wlan_ config](bluetooth/ble_wlan_config/ReadMe.md)
 * [bt_ rfcomm_server](bluetooth/bt_rfcomm_server/ReadMe.md)
 
* filesystem：文件系统读写功能示例程序,包括：
 * [fatfs](filesystem/fatfs/ReadMe.md) 


* parser：解析器功能示例程序，包括：
 * [json](parser/json/ReadMe.md)
 * [url](parser/url/ReadMe.md)

* algorithm：安全和校验算法示例程序，包括：
 * [AES_CBC](algorithm/AES/AES_CBC/ReadMe.md)
 * [AES_ECB](algorithm/AES/AES_ECB/ReadMe.md)
 * [AES_PKCS5](algorithm/AES/AES_PKCS5/ReadMe.md)
 * [ARC4](algorithm/ARC4/ReadMe.md)
 * [CRC8](algorithm/CRC8/ReadMe.md)
 * [CRC16](algorithm/CRC16/ReadMe.md)
 * [DES](algorithm/DES/ReadMe.md)
 * [DES3](algorithm/DES3/ReadMe.md)
 * [MD5](algorithm/MD5/ReadMe.md)
 * [SHA1](algorithm/SHA/ReadMe.md)
 * [SHA256](algorithm/SHA/ReadMe.md)
 * [SHA384](algorithm/SHA/ReadMe.md)
 * [SHA512](algorithm/SHA/ReadMe.md)
 * [HMAC_MD5](algorithm/HMAC/ReadMe.md)
 * [HMAC_SHA1](algorithm/HMAC/ReadMe.md)
 * [HMAC_SHA256](algorithm/HMAC/ReadMe.md)
 * [HMAC_SHA384](algorithm/HMAC/ReadMe.md)
 * [HMAC_SHA512](algorithm/HMAC/ReadMe.md)
 * [Rabbit](algorithm/Rabbit/ReadMe.md)
 * [RSA](algorithm/RSA/ReadMe.md)


* [power_measure](power_measure/ReadMe.md): MiCO设备低功耗功能实现示例程序。

* hardware：硬件外设控制实现示例程序，包括：
 * [dc_motor](hardware/micokit_ext/dc_motor/ReadMe.md) 
 * [ambient_ light_sensor](hardware/micokit_ext/ambient_light_sensor/ReadMe.md)
 * [environmental_sensor](hardware/micokit_ext/environmental_sensor/ReadMe.md)
 * [infrared_reflective](hardware/micokit_ext/infrared_reflective/ReadMe.md)
 * [light_sensor](hardware/micokit_ext/light_sensor/ReadMe.md)
 * [motion_sensor](hardware/micokit_ext/motion_sensor/ReadMe.md)
 * [oled](hardware/micokit_ext/oled/ReadMe.md)
 * [rgb_led](hardware/micokit_ext/rgb_led/ReadMe.md)
 * [temp_ hum_sensor](hardware/micokit_ext/temp_hum_sensor/ReadMe.md)
 * [micokit_stmems](hardware/micokit_stmems/ReadMe.md)
  
  
* test：功能测试示例程序，包括：
 * [auto_ self_test](test/auto_self_test/ReadMe.md)
 * [bt_ mfg_test](test/bt_mfg_test/ReadMe.md)
 * [hardware_test_3080](test/hardware_test_3080/ReadMe.md)
 * [iperf](test/iperf/ReadMe.md)
 * [iperf_power](test/iperf_power/ReadMe.md)
 * [spi_flash_test](test/spi_flash_test/ReadMe.md)




### 使用前准备
1. 首先您需要安装 [mico-cube](https://code.aliyun.com/mico/mico-cube).
2. 安装MiCoder IDE集成开发环境，下载地址：[MiCoder IDE](http://developer.mico.io/downloads/2).
3. 准备一个Jlink下载调试工具(针对ST开发板，可使用Stlink)，并在PC上安装Jlink驱动软件.
4. 连接Jlink工具到PC端，并更新驱动程序，具体方法参考：[MiCO SDK 使用](http://developer.mico.io/docs/10)页面中步骤 1.
5. 使用USB线连接PC和MiCOKit，实现一个虚拟串口用于供电和输出调试信息, 驱动下载地址：[VCP driver](http://www.ftdichip.com/Drivers/VCP.htm).

### 工程导入，编译及下载，调试

具体操作方法可参考：[第一个MiCO应用程序：Helloworld](https://code.aliyun.com/mico/helloworld)