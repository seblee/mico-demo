mico-demos
====
包含：一系列基于MiCO的示例程序。

###目录
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
 
* application：应用程序示例程序,包括：
 * [alink](application/alink/ReadMe.md) 
 * [at](application/at/ReadMe.md)
 * [homekit](application/homekit/ReadMe.md) 
 * [wifi_uart](application/wifi_uart/ReadMe.md)
 
* bluetooth：蓝牙功能相关的示例程序，如：蓝牙center，蓝牙sensor和蓝牙设备扫描等。
* filesystem：文件系统读写功能示例程序。
* hardware：硬件外设控制实现示例程序。
* helloworld：MiCO最简单的入门级应用程序。
* net：网络通信功能示例程序，如：dns域名解析，http通信，tcp客户端创建及通信等。
* os：操作系统功能示例程序，如：多线程，信号量，互斥锁，定时器和队列。
* parser：解析器功能示例程序，如:JSON解析，url解析。
* power_measure: MiCO设备低功耗功能实现示例程序。
* test：功能测试示例程序，如：iperf等。
* wifi：无线wifi功能实现示例程序，如：wifi扫描，softap建立，station建立等。


### 使用前准备
1. 首先您需要安装 [mico-cube](https://code.aliyun.com/mico/mico-cube).
2. 安装MiCoder IDE集成开发环境，下载地址：[MiCoder IDE](http://developer.mico.io/downloads/2)；
3. 准备一个Jlink下载调试工具(针对ST开发板，可使用Stlink)，并在PC上安装Jlink驱动软件；
4. 连接Jlink工具到PC端，并更新驱动程序，具体方法参考：[MiCO SDK 使用](http://developer.mico.io/docs/10)页面中步骤 1；
5. 使用USB线连接PC和MiCOKit，实现一个虚拟串口用于供电和输出调试信息, 驱动下载地址：[VCP driver](http://www.ftdichip.com/Drivers/VCP.htm)；

###工程导入，编译及下载，调试

具体操作方法可参考：[第一个MiCO应用程序：Helloworld](https://code.aliyun.com/mico/helloworld)