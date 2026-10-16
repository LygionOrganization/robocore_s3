# RoboCore S3

RoboCore S3 是面向机器人下位机的 ESP32-S3 主控板。出厂安装完整控制固件，无需编程，即可用手机或电脑网页扫描总线 ID、控制舵机与步进电机、设置 TTL Node 和读取编码器。

## 产品参数

| 项目 | RoboCore S3 |
| --- | --- |
| 尺寸与安装 | 47 × 52 mm；4 个直径 3 mm 装配孔，孔中心间距 40 × 45 mm |
| 主控 | ESP32-S3，8 MB Flash、8 MB PSRAM |
| 输入电源 | XT30 输入 5–48V；带电源总线口电压跟随输入 |
| 设备总线 | 单线 TTL 与总线设备 RS485，共用 IO4 TX / IO5 RX |
| 外部通信 | 开关选择 CAN 或 RS485，共用 IO13 RX / IO14 TX |
| 感知与扩展 | 板载 QMI8658A，支持外接 User Interface Module (A) |
| 开发方式 | 开源例程，支持 PlatformIO 与 Arduino IDE |

接线前请阅读[供电与接口](hardware.md)。

!!! danger "48V 输入不代表所有设备都能用 48V"
    动力电源直接传到总线供电接口。接入低压舵机前必须选择匹配的供电。

## 按经验选择路线

1. **不写代码**：[快速开始](quickstart.md) → [网页控制台](web-console.md) → [总线设备控制](devices.md)。
2. **电脑/上位机控制**：[JSON 接口](json-api.md) → [电脑/上位机控制](python.md)。
3. **自己写固件**：[Arduino 单元例程](arduino.md) → [AI 辅助开发](ai-development.md)。
4. **恢复出厂软件**：[固件恢复](firmware.md)。

[开始使用](quickstart.md){ .md-button .md-button--primary }
[下载例程包](../../../assets/files/robocore-s3/examples.zip){ .md-button }

源码：[完整固件](https://github.com/LygionOrganization/robocore_s3) · [TTL SDK](https://github.com/LygionOrganization/lygion_devs_cpp) · [Wiki](https://github.com/LygionOrganization/lygion-wiki)
