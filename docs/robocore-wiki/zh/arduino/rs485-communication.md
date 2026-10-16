# 外部 RS485 通信

**这是独立单元例程。上传会替换完整网页固件；需要恢复控制台时使用[固件恢复](../firmware.md)。**

## 准备硬件与接线

开关拨至 485，通过 GH1.25-5P 连接 RS485 对端的 A/B/GND，115200、8N1；使用 UART2 RX13/TX14。对端自行供电时不并联两个电源输出。

## Arduino IDE 与开发板设置

1. 安装 Arduino IDE 2。打开 **文件 → 首选项 → 附加开发板管理器网址**，加入 `https://espressif.github.io/arduino-esp32/package_esp32_index.json`。
2. 打开 **工具 → 开发板 → 开发板管理器**，搜索 `esp32`，选择作者 **Espressif Systems** 的包，安装 **3.3.0**。
3. 在 **工具 → 开发板 → esp32** 选择 **ESP32S3 Dev Module**。设置 Flash Size=8MB、PSRAM=OPI PSRAM、USB Mode=USB-OTG (TinyUSB)、USB CDC On Boot=Disabled。反馈和交互输入使用 Serial0（UART0），连接 Type-C UART。
4. 下载并解压[例程包](../../../../assets/files/robocore-s3/examples.zip)，打开 `arduino_example/RS485_Communication/RS485_Communication.ino`，不要将多个例程合并到同一工程。

安装界面详见[安装 Arduino IDE](../../../../tutorials/install-arduino-ide.md)；开发板来源见 [Espressif 安装指南](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)。

## 本例依赖

无需安装额外库。`HardwareSerial` 随上面的 esp32 开发板包提供。找不到头文件时，请检查是否安装并选择了 ESP32S3，而不是安装其它同名库。

## 编译与上传

1. 点击“验证”编译，确认没有错误。缺少 `.h` 文件时按上节安装对应库，再重新编译。
2. Type-C UART 支持自动下载；若用 Type-C USB，按住 BOOT，按下并松开 RESET，再松开 BOOT，选择出现的端口后上传。
3. 上传完成按 RESET 运行。数据线保持连接 Type-C UART，选择与上传相同的串口查看 Serial0 反馈，无需更换接口。打开串口监视器，115200。若错过启动提示，保持监视器打开再按 RESET。

## 操作与预期结果

发送 `t`，主板发送一行 `RoboCore S3 RS485` 加换行；对端应收到对应文本。对端发送字节后本板以十六进制打印。双方轮流发送以避免半双工冲突，板上自动切换收发方向，无需 DE 引脚。

## 排查与下一步

这是字节收发例程，不是出厂 JSON 网关，也不是 IO4/IO5 的舵机 RS485。乱码先检查波特率、8N1、A/B 线序及共地。

接口和电压详见[硬件说明](../hardware.md)，完整固件命令见[JSON 指令集](../json-api.md)。

## 完整代码

```cpp
// RoboCore S3：外部 RS485 通信
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

// Set the physical selector to 485 before running this sketch.
// GH1.25-5P: external RS485, RX=13 / TX=14, with onboard automatic direction.
// Match A/B/GND and 115200 8N1 on the peer; do not connect 5V to a powered peer.
// Serial1 RX=5 / TX=4 belongs to the separate servo/TTL bus and is not used here.
HardwareSerial externalBus(2);

// 上电执行一次：初始化所需通信接口和设备。
void setup() {
  Serial0.begin(115200);
  externalBus.begin(115200, SERIAL_8N1, 13, 14);
  Serial0.println("Selector: 485. External RS485: RX13/TX14, 115200 8N1.");
  Serial0.println("Send t to transmit a line. Received bytes are printed as hex.");
}

// 循环执行：按本页操作步骤触发功能并观察反馈。
void loop() {
  // Transmit only on request. Coordinate half-duplex access with the other node.
  if (Serial0.read() == 't') {
    externalBus.print("RoboCore S3 RS485\n");
    externalBus.flush();
    Serial0.println("Local transmission complete; peer receipt is not confirmed.");
  }
  for (unsigned n = 0; n < 64 && externalBus.available(); ++n)
    Serial0.printf("%02X ", externalBus.read());
  delay(1);
}
```
