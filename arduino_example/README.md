# RoboCore S3 Arduino 单元例程

每个目录是独立 Arduino 工程。**上传单元例程会替换出厂完整固件**，不会同时保留网页控制台。恢复完整功能请重新上传本仓库 PlatformIO 工程或使用匹配版本的出厂恢复包。

## Arduino IDE 准备

1. 安装 Arduino IDE 2，在开发板管理器安装 **esp32 by Espressif Systems 3.3.0**。
2. 开发板选择 **ESP32S3 Dev Module**；Flash Size 8MB、PSRAM OPI PSRAM。除 USB_CDC_JSON 外，USB CDC On Boot 选择 **Disabled**，数据线保持连接 **Type-C UART**，上传与串口反馈使用同一个接口。仅 USB_CDC_JSON 选择 USB Mode **USB-OTG (TinyUSB)**、USB CDC On Boot **Enabled** 并使用 Type-C USB。
3. 在库管理器安装下表依赖。`lygion_devs` 和 `RoboCoreSupport` 是本仓库 `lib` 下的文件夹，将整个目录复制到 Arduino sketchbook 的 `libraries` 目录，再重启 IDE。SDK 也可从 [LygionOrganization/lygion_devs_cpp](https://github.com/LygionOrganization/lygion_devs_cpp) 获取；例程以本仓库附带版本为准。
4. 打开目录内同名 `.ino`，编译、上传。串口监视器选择 Type-C UART 对应端口，设为 **115200**。普通例程显式使用 Serial0，不受 Serial 的 USB 映射影响，无需更换接线。USB_CDC_JSON 例程单独使用原生 USB CDC，其波特率为虚拟参数。
5. 舵机、电机例程启动时不会主动移动。按串口提示输入单个字符触发动作。确认额定供电与机械行程后再执行。

RoboCore S3 输入 5–48V，但总线动力电压等于输入电压。所有共电设备必须支持所用电压。GH1.25-3P 用于板间 TTL 通信、供电解耦。

## 例程目录与依赖

| 目录 | 功能 | 额外依赖 |
| --- | --- | --- |
| SCS_Servo | SCS 位置、反馈、失能 | FTServo 2.0.0 |
| HLS_Servo | HLS 位置、电流限制、反馈 | FTServo 2.0.0 |
| STS_Servo | STS 位置、反馈、失能 | FTServo 2.0.0 |
| SMS_Servo | SMS 位置、反馈、失能 | FTServo 2.0.0 |
| Bus_Scan | 扫描当前波特率下的 ID | lygion_devs |
| TTL_Node | PWM、RGB、S.BUS、修改 ID | lygion_devs |
| TTL_Stepper | 位置/连续/从动模式、使能、运动、设置当前位置、反馈 | lygion_devs |
| TTL_Encoder | E02 位置、速度、基准校准 | lygion_devs |
| CAN_Communication | CAN 档，CAN 2+2，RX13/TX14，500 kbps | 核心内置 TWAI |
| RS485_Communication | 485 档，GH1.25-5P，RX13/TX14，115200 8N1 | 核心内置 HardwareSerial |
| User_Interface_Module_A | 五向按键、蜂鸣器、128×32 OLED 联合演示 | Adafruit SSD1306、GFX Library、BusIO |
| OLED | 128×32 OLED 显示 | Adafruit SSD1306、Adafruit GFX Library（及其提示的 Adafruit BusIO） |
| QMI8658A | 板载六轴传感器读取 | RoboCoreSupport，Wire 随核心提供 |
| HTTP_JSON | HTTP JSON 请求/响应 | ArduinoJson 7.x，WebServer/WiFi 内置 |
| WebSocket_JSON | WebSocket JSON 请求/响应 | ArduinoJson 7.x、ESP32Async/AsyncTCP、ESP32Async/ESPAsyncWebServer |
| USB_CDC_JSON | EVENT 接收、定长组包、只保留最新命令 | ArduinoJson 7.x；USB/USBCDC 内置 |
| ESP_NOW_JSON | 两块板间 JSON 数据包 | ArduinoJson 7.x；esp_now/WiFi 内置 |
| RGB_Buzzer | 外接 GPIO26 WS2812 灯与 UI Module (A) 蜂鸣器（非板载资源） | Adafruit NeoPixel |

配套依赖版本：ArduinoJson 7.4.3、AsyncTCP 3.5.0、ESPAsyncWebServer 3.12.1、Adafruit NeoPixel 1.15.5、GFX 1.12.6、SSD1306 2.5.17。安装依赖同名库时注意作者，避免误装旧的 AsyncWebServer 分支。

## 引脚

RoboCore S3 GPIO：Serial1 RX=5 / TX=4，I²C SDA=6 / SCL=7，CAN TX=14 / RX=13；UI Module (A) 蜂鸣器=21。OLED 为 0x3C；QMI8658A 自动尝试 0x6A、0x6B。传感器通信异常时报告错误并继续运行。

## 单元通信协议与完整固件不同

`HTTP_JSON`、`WebSocket_JSON`、`USB_CDC_JSON` 仅演示一个 RAM 数值：`{"T":1,"value":42}` 写入，`{"T":2}` 读取，不控制电机。HTTP/WS 例程建立热点 `RoboCoreDemo` / `12345678`，分别使用 `http://192.168.4.1/api/cmd` 和 `ws://192.168.4.1/ws`。USB 每条 JSON 后发送换行符，并持续读取回复。

完整出厂固件使用 `T=700…740` 等指令，不要把单元例程的 `T=1/2` 当作出厂 API。完整 API 见 [RoboCore 接口](../docs/ROBOCORE_API.md)。

ESP-NOW 例程需两块板同信道，修改 `peerMac` 为对端 STA MAC。发送帧为 250 字节、末尾补零，与本仓库完整固件的 JSON 帧一致；若对接完整固件，还需在完整固件配置相应接收模式/可信 MAC（T=411、414），并保持 Wi-Fi 信道一致。

## 步进驱动说明

`0`/`1` 命令可在一次上电期间切换位置与速度模式，无需重启；只有更改相位配置才需要重启。模式更改会关闭输出，用 `e` 重新使能。`2` 设置从动模式及示例主设备 ID=2，需按产品教程连接 EDS。速度、电流、加速度是寄存器单位，先检查设备参数与电机额定电流。`s` 只用于连续转动模式的速度归零，`x` 为失能。

## 放入 PlatformIO

这些例程已包含 `#include <Arduino.h>`。将单个 `.ino` 内容复制为独立 PIO 项目的 `src/main.cpp`，按本仓库 `platformio.ini` 配置 ESP32-S3 和依赖；不要把多个例程放进同一个 `src`，否则 `setup/loop` 会重复定义。也不要把单元例程加入完整固件的 `src`。

如果直接复制完整 `platformio.ini`，也要复制 `tools/pio_python_path.py` 到新项目相同路径；该脚本修正此平台版本在 Windows 下查找 Python 工具的路径。Arduino IDE 不需要该脚本。


## 外部通信与舵机总线

CAN/RS485 开关仅切换 IO13（RX）/IO14（TX）：CAN 档连接 CAN 2+2，485 档连接 GH1.25-5P。两种外部通信不能同时占用这组引脚；切换前停止通信，按目标协议运行对应例程。RS485_Communication 使用 UART2，板上电路自动控制收发方向，无需额外 DE GPIO；它演示字节收发，不是出厂 JSON 网关。

XH2.54-4P 舵机 RS485 与 5264-3P 单线 TTL 共用 UART1（RX5/TX4），波特率和发送数据相同，不能作为两条独立串口使用。外部 UART2 RS485 与这一路不冲突。CAN 板载 120Ω 终端电阻，多节点接线需核对整条总线的终端配置。

## User Interface Module (A)

断电后将模块连接到 PH1.25-10P 扩展口。引脚：上=10、下=11、左=12、右=9、确认=2，低电平有效；蜂鸣器=21；OLED SDA=6、SCL=7，地址 0x3C，128×32。按键去抖后在串口和 OLED 显示按键名称，并短鸣一次；长按不会持续触发。OLED 缺失时按键与蜂鸣器仍可运行。此例程不发送电机指令。OLED 与板载 IMU 共用 I²C。

OLED、按键和蜂鸣器来自外接 UI Module (A)，不是 RoboCore S3 板载器件。RGB_Buzzer 的 WS2812 功能只用于另行接线的灯带；GPIO26 不在 PH1.25-10P 接口上，不能将该例程当作本产品的板载灯演示。

## 每个例程的详细教程

每篇包含接线、依赖安装、上传步骤、预期结果和完整代码：[中文教程](https://wiki.lygion.ai/reference/bus-devices/robocore-s3/arduino/) / [English tutorials](https://wiki.lygion.ai/en/reference/bus-devices/robocore-s3/arduino/)。
