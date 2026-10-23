# RoboCore S3

**用于机器人控制的开源 ESP32-S3 固件与例程。**

**简体中文** | [English](README.md)

[中文 Wiki](https://wiki.lygion.ai/reference/bus-devices/robocore-s3/) · [English Wiki](https://wiki.lygion.ai/en/reference/bus-devices/robocore-s3/) · [JSON 指令表](docs/ROBOCORE_API.md) · [Arduino 例程](arduino_example/)

## 这个项目可以做什么？

RoboCore S3 将机器人的总线设备连接到浏览器或上位机。本仓库提供完整控制固件、中英双语网页控制台、独立 Arduino 例程和 Python 通信客户端。

它可以独立完成设备调试与任务执行，也可以作为机器人的“小脑板”，与 PC 或单板计算机（SBC）组成大小脑架构：上位机负责视觉、规划和应用逻辑，RoboCore S3 负责发送设备控制指令、读取反馈，双方通过人类可读的 JSON 指令通信。

[`robocore_s3`](https://github.com/LygionOrganization/robocore_s3) 固件项目**同时适用于 RoboCore S3 和 Robot Driver with ESP32S3 Lite**。本指南以 RoboCore S3 为主要使用对象；Lite 用户也可使用同一套固件及适用于其硬件的例程。

Lite 的供电、接口和外设请以 [Lite 中文 Wiki](https://wiki.lygion.ai/reference/bus-devices/robot-driver-with-esp32s3-lite/) 为准。下文硬件参数属于 RoboCore S3，不能将其 5–48V 输入范围套用于 Lite。Lite 没有板载 QMI8658A，固件在未检测到 IMU 时仍会继续运行。

## 主要功能

| 功能 | 用途 |
| --- | --- |
| 网页控制 | 无需编程即可扫描总线 ID、设置设备参数和读取反馈 |
| 总线设备控制 | 支持 SCS、HLS、STS、SMS 舵机，以及 TTL Node、TTL Stepper Driver、TTL Encoder E02 |
| 上位机通信 | 通过 HTTP、WebSocket 或原生 USB CDC 发送 JSON 指令 |
| 运动感知 | 读取板载 QMI8658A 加速度计与陀螺仪 |
| 本地交互 | 连接 User Interface Module (A)，实现按键输入、蜂鸣器提醒与 OLED 显示 |
| 自定义开发 | 使用 PlatformIO 构建完整固件，或从独立 Arduino 例程开始 |

主网页控制台无需联网，默认显示英文，点击右上角 **中文** 即可切换语言。任务脚本、多轴操作等高级功能通过 JSON 调试使用；`/legacy` 会跳转到该页面。

## 先用网页控制设备

已安装完整固件时：

1. 连接一个总线设备，选择符合设备额定电压的电源。带供电的总线接口输出电压与主板输入电压相同。
2. 连接主板默认 Wi-Fi 热点：**`Robot`**，密码 **`12345678`**。如果此前修改过网络配置，请使用当前设置。
3. 在浏览器中打开 **[http://192.168.4.1](http://192.168.4.1)**，无需连接互联网。
4. 点击右上角 **中文**，进入“总线设备”，确认波特率后扫描 ID。选择对应设备组件，先读取状态，再执行运动。

详细步骤见[快速开始](https://wiki.lygion.ai/reference/bus-devices/robocore-s3/quickstart/)。需要恢复完整程序时，请查看[固件恢复](https://wiki.lygion.ai/reference/bus-devices/robocore-s3/firmware/)。

## 使用电脑或 SBC 控制

同一条 JSON 指令可以通过 HTTP、WebSocket 或 USB CDC 发送。例如，以下指令只扫描 ID 0–20：

```json
{"T":700,"first":0,"last":20}
```

仓库中的 Python 客户端会处理请求 ID，并等待匹配的执行结果。在仓库根目录安装依赖：

```sh
python -m pip install requests websocket-client pyserial
```

```python
from python_example.robocore.client import request

result = request(
    'http',
    'http://192.168.4.1',
    {'T': 700, 'first': 0, 'last': 20},
)
print(result)
```

使用 `ws` 与 `ws://192.168.4.1/ws` 可切换为 WebSocket；使用 `usb` 与实际串口名称可切换为 USB CDC。USB CDC 应连接主板原生 **Type-C USB** 接口。

Python 只是其中一种选择。用户可以根据 [JSON 指令表](docs/ROBOCORE_API.md)，借助 AI 工具使用 C++、C#、JavaScript 等语言开发通信程序。

**指令执行规则：**固件只保留最新一条待执行指令。HTTP `202` 代表已接收，不代表已执行。修改参数时应等待匹配的结果；写入超时后先读取状态，再决定是否重试。**“停止循环”（`T=0`）用于退出任务循环，不等于电机急停。**

## Windows 固件下载工具

[下载工具目录](download_tool/RoboCore_S3_Download_Tool/)包含配套固件和中英文说明，适用于 RoboCore S3 与 Lite。完整解压后运行 `Start_Download_Tool.bat`，选择主板对应的 COM 端口再开始烧录。维护者可运行 `python tools/package_download_tool.py` 重新构建工具包。

## 开发自己的固件

### 完整固件：PlatformIO

使用安装了 PlatformIO 的 VS Code 打开本仓库。保留配套的 `platformio.ini`、`src/`、`include/`、`lib/` 和 `tools/`，项目已经固定平台与依赖版本。

```sh
pio run
```

修改网页时，编辑 `web/index.html`，重新生成嵌入页面后再编译：

```sh
python tools/embed_web.py
pio run
```

编译产物位于 `.pio/build/esp32-s3-devkitc-1-n8r8/`。工具链配置和验证范围见[验证说明](docs/ROBOCORE_VALIDATION.md)。

### 单项功能：Arduino IDE

[`arduino_example/`](arduino_example/) 提供 18 个独立工程，覆盖总线设备、CAN、外部 RS485、IMU、User Interface Module (A) 和 JSON 通信。

从 [Arduino 使用指南](https://wiki.lygion.ai/reference/bus-devices/robocore-s3/arduino/)或[本地依赖与操作说明](arduino_example/README.md)开始。已验证的配置为 **ESP32S3 Dev Module**、**8 MB Flash**、**OPI PSRAM**、**USB-OTG (TinyUSB)**、**USB CDC On Boot 按各例程顶部说明设置**。

**上传独立例程会替换完整固件，网页控制台不会同时保留。** HTTP/WebSocket/USB 教学例程使用简化协议，不包含完整控制固件的全部 JSON 指令。

## 硬件要点

| 资源 | RoboCore S3 |
| --- | --- |
| 主控模组 | ESP32-S3-WROOM-1U-MCR8N8；8 MB Flash、8 MB PSRAM |
| 电源输入 | CAN 2+2 接口的 XT30 电源端输入 5–48V |
| 总线设备接口 | 5264-3P 单线 TTL 与 XH2.54-4P RS485，共用 IO4 TX / IO5 RX |
| 外部通信 | 开关选择 CAN 2+2 或 GH1.25-5P RS485，共用 IO13 RX / IO14 TX |
| 板间接口 | GH1.25-3P，用于板间 TTL 通信、供电解耦 |
| USB | Type-C UART 用于串口下载与调试；Type-C USB 用于原生 USB 通信 |
| IMU | 板载 QMI8658A，使用 IO6 SDA / IO7 SCL |
| 扩展接口 | PH1.25-10P，连接 User Interface Module (A)；OLED、按键和蜂鸣器位于外接模块上 |
| 尺寸与安装 | 47 × 52 mm；4 个直径 3 mm 装配孔，孔中心间距 40 × 45 mm |

**供电电压必须符合所有共电总线设备的额定要求。** 主控支持 48V，不代表低压舵机可以接入 48V；仅连接 USB 也不代表执行器已经获得动力供电。

IO13/IO14 的外部 RS485 与 IO4/IO5 的总线设备接口独立；总线设备 RS485 与 TTL 共用一个 UART。CAN/RS485 开关切换硬件通路，程序也需要采用对应协议。接线前请阅读[硬件说明](https://wiki.lygion.ai/reference/bus-devices/robocore-s3/hardware/)。

## 仓库目录

| 路径 | 内容 |
| --- | --- |
| [`src/`](src/) | 完整固件及生成的网页文件 |
| [`web/`](web/) | 可编辑的双语控制台 |
| [`arduino_example/`](arduino_example/) | 独立 Arduino 例程 |
| [`python_example/robocore/`](python_example/robocore/) | HTTP、WebSocket、USB CDC 客户端 |
| [`lib/`](lib/) | 本地设备库与支持代码 |
| [`docs/`](docs/) | JSON 文档、开发指南与 Wiki 源文件 |
| [`tools/`](tools/) | 网页嵌入、例程检查与打包工具 |
| [`test/`](test/) | 主机回归测试与网页检查 |

借助 AI 开发时，请提供[开发指南](AGENTS.md)、JSON 指令表、设备型号、ID、波特率和目标行为，先完成只读反馈测试，再添加运动控制。

## 开源协议

本项目采用 [GNU AGPL v3](LICENSE)。附带的第三方组件遵循各自的开源协议。

## 发布与 AI 二次开发

[AGENTS.md](AGENTS.md) 提供架构、功能入口和开发约束，AI 工具可从根目录直接读取。[发布说明](docs/RELEASING.md) 介绍干净源码包与固件工具包的制作方法；`.pio/` 中的本地缓存与内部归档不属于对外交付内容。
