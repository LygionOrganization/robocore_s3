# CAN 外部通信

**这是独立单元例程。上传会替换完整网页固件；需要恢复控制台时使用[固件恢复](../firmware.md)。**

## 准备硬件与接线

开关拨至 CAN，通过 CAN 2+2 连接另一个 500 kbps CAN 节点，CAN H/L 对应且共地；RX13/TX14。板载 120Ω 终端，按总线两端的实际终端配置组网。

## Arduino IDE 与开发板设置

1. 安装 Arduino IDE 2。打开 **文件 → 首选项 → 附加开发板管理器网址**，加入 `https://espressif.github.io/arduino-esp32/package_esp32_index.json`。
2. 打开 **工具 → 开发板 → 开发板管理器**，搜索 `esp32`，选择作者 **Espressif Systems** 的包，安装 **3.3.0**。
3. 在 **工具 → 开发板 → esp32** 选择 **ESP32S3 Dev Module**。设置 Flash Size=8MB、PSRAM=OPI PSRAM、USB Mode=USB-OTG (TinyUSB)、USB CDC On Boot=Disabled。反馈和交互输入使用 Serial0（UART0），连接 Type-C UART。
4. 下载并解压[例程包](../../../../assets/files/robocore-s3/examples.zip)，打开 `arduino_example/CAN_Communication/CAN_Communication.ino`，不要将多个例程合并到同一工程。

安装界面详见[安装 Arduino IDE](../../../../tutorials/install-arduino-ide.md)；开发板来源见 [Espressif 安装指南](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)。

## 本例依赖

无需安装额外库。`driver/twai.h` 随上面的 esp32 开发板包提供。找不到头文件时，请检查是否安装并选择了 ESP32S3，而不是安装其它同名库。

## 编译与上传

1. 点击“验证”编译，确认没有错误。缺少 `.h` 文件时按上节安装对应库，再重新编译。
2. Type-C UART 支持自动下载；若用 Type-C USB，按住 BOOT，按下并松开 RESET，再松开 BOOT，选择出现的端口后上传。
3. 上传完成按 RESET 运行。数据线保持连接 Type-C UART，选择与上传相同的串口查看 Serial0 反馈，无需更换接口。打开串口监视器，115200。若错过启动提示，保持监视器打开再按 RESET。

## 操作与预期结果

发送 `t`，发出标准帧 ID=0x100，数据 0x12 0x34；查看 Transmit result。对端向本板发送帧时，串口打印 CAN ID、DLC 和每个数据字节。发送接口返回成功只表示帧进入发送流程，最终接收需对端确认。

## 排查与下一步

无法发送时检查开关、双方波特率、H/L、共地、终端及有效应答节点；不要把 IO13/14 直接当 CAN H/L 接线。

接口和电压详见[硬件说明](../hardware.md)，完整固件命令见[JSON 指令集](../json-api.md)。

## 完整代码

```cpp
// RoboCore S3：CAN 外部通信
// 上传此独立例程后，原来的完整网页固件将被替换。
// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// No external libraries required; all headers are included with the ESP32 board package.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <driver/twai.h>
bool ready = false;
// 上电执行一次：初始化所需通信接口和设备。
void setup() {
  Serial0.begin(115200);
  // Set the physical selector to CAN. CAN and external RS485 share RX13/TX14.
  // GPIO38 drives the CAN transceiver slope-control input; LOW selects normal speed.
  pinMode(38, OUTPUT);
  digitalWrite(38, LOW);
  twai_general_config_t general = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_14, GPIO_NUM_13, TWAI_MODE_NORMAL);
  twai_timing_config_t timing = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  ready = twai_driver_install(&general, &timing, &filter) == ESP_OK;
  if (ready) ready = twai_start() == ESP_OK;
  Serial0.println("Selector: CAN. Use CAN 2+2, another 500 kbps node and common ground.");
  Serial0.println("This board has 120 ohm termination. Check termination across the full bus.");
  Serial0.println("Send t to transmit ID 0x100 with two bytes.");
}
// 循环执行：按本页操作步骤触发功能并观察反馈。
void loop() {
  if (!ready) { delay(100); return; }
  if (Serial0.read() == 't') {
    twai_message_t tx = {};
    tx.identifier = 0x100; tx.data_length_code = 2; tx.data[0] = 0x12; tx.data[1] = 0x34;
    Serial0.printf("Transmit result=%d\n", twai_transmit(&tx, pdMS_TO_TICKS(100)));
  }
  twai_message_t rx = {};
  if (twai_receive(&rx, 0) == ESP_OK) {
    Serial0.printf("CAN ID=%lx DLC=%u", rx.identifier, rx.data_length_code);
    for (int i = 0; i < rx.data_length_code && i < 8; ++i) Serial0.printf(" %02x", rx.data[i]);
    Serial0.println();
  }
  delay(1);
}
```
