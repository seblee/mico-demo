# ble_hello_sensor
-------------------------------------------------------------------------------

## 说明

该示例程序演示了一个低功耗蓝牙服务端程序的实现。该示例程序需要调用 bt_smart_controller 框架。可以与ble_hello_center示例配合演示。详情请看ble_hello_center相关资料。

## 服务定义

### Hello服务

向客户提供控制MiCOKit RGB LED的颜色功能。如下：

| Handle | 类型 | 权限 | UUID | 注释 |
|:----:|:---:|:--:|:---:|:----:|
| 0x30 | 服务     |                         | 1b7e8251-2877-41c3-b46ecf057c562023 | Hello 服务 |
| 0x31 | 特性     |  Read, Notify, Indicate | 8ac32d3f-5cb9-4d44-bec3ee689169f626 | 发送消息 |
| 0x32 | 特性值   |                         |                                     | 消息内容 |
| 0x33 | 特性描述 | Read, Write             |  2902 | CCCD配置属性 |
| 0x34 | 特性     | Read, Write            | 5e9bf2a8-f93f-4481-a67e3b2f4a07891a | RGB LED |
| 0x35 | 特性值   |                        |                                      | RGB LED Value |

### SPP服务

| Handle | 类型 | 权限 | UUID | 注释 |
|:----:|:---:|:--:|:---:|:----:|
| 0x40 | 服务     |                         | 7163edd8-e338-3691-324d4b3f8a21675e | SPP 服务 |
| 0x41 | 特性     |      Write              | e2aebab4-3521-7032-78211d24903e3945 | RXD |
| 0x41 | 特性值   |                         |                                      | 内容 |
| 0x43 | 特性描述 |      Read               |               2901                   | 特性描述 |
| 0x44 | 特性     | Notify, Indicate       | 30f31dba-522704391-2a122e825e1a1532 | TXD |
| 0x45 | 特性值   |                         |                                    | 内容 |
| 0x46 | 特性描述 |      Read               |               2901                 | 特性描述 |