# ble_hello_center
-------------------------------------------------------------------------------

## 说明

该示例与 ble_hello_sensor 服务端示例程序配合，替代手机，实现了一个蓝牙客户端的功能。该示例程序需要调用 bt_smartbridge 框架。该示例程序完成的功能如下：

* 扫描环境中的低功耗蓝牙设备，并寻找符合要求的ble_hello_sensor设备。
* 与符合的设备建立绑定，并对连接加密
* 对连接的设备进行服务和特性发现，并将结果包括在本地缓冲中。下一次连接时将跳过该步骤。
* 向ble_hello_sensor设备写入数据，控制ble_hello_sensor设备的RGB LED灯颜色。

## 使用方法

* 首先准备一个已经正确运行ble_hello_sensor示例的设备。
* 此示例会自动完成扫描，连接，配对，加密，属性发现等操作，并最终可以控制ble_hello_sensor设备RGB LED灯的颜色变化
* 在ble_hello_sensor设备的串口终端中，输入notify即可向ble_hello_center发送消息。