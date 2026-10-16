# 外接 OLED 显示

**这是独立单元例程。上传会替换完整网页固件；需要恢复控制台时使用[固件恢复](../firmware.md)。**

## 准备硬件与接线

User Interface Module (A) 或兼容 SSD1306 128×32 I²C OLED。SDA6、SCL7，地址 0x3C；使用适合模块的供电和 3.3V GPIO 电平。

## Arduino IDE 与开发板设置

1. 安装 Arduino IDE 2。打开 **文件 → 首选项 → 附加开发板管理器网址**，加入 `https://espressif.github.io/arduino-esp32/package_esp32_index.json`。
2. 打开 **工具 → 开发板 → 开发板管理器**，搜索 `esp32`，选择作者 **Espressif Systems** 的包，安装 **3.3.0**。
3. 在 **工具 → 开发板 → esp32** 选择 **ESP32S3 Dev Module**。设置 Flash Size=8MB、PSRAM=OPI PSRAM、USB Mode=USB-OTG (TinyUSB)、USB CDC On Boot=Disabled。反馈和交互输入使用 Serial0（UART0），连接 Type-C UART。
4. 下载并解压[例程包](../../../../assets/files/robocore-s3/examples.zip)，打开 `arduino_example/OLED/OLED.ino`，不要将多个例程合并到同一工程。

安装界面详见[安装 Arduino IDE](../../../../tutorials/install-arduino-ide.md)；开发板来源见 [Espressif 安装指南](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)。

## 本例依赖

打开 **工具 → 管理库**，逐项搜索并安装下列库（版本下拉框可选择指定版本），核对作者；如提示依赖安装，选择 Install all。

- Adafruit SSD1306 2.5.17
- Adafruit GFX Library 1.12.6
- Adafruit BusIO

[Arduino 库安装说明](https://support.arduino.cc/hc/en-us/articles/5145457742236-Install-libraries-in-the-Arduino-IDE)。

## 编译与上传

1. 点击“验证”编译，确认没有错误。缺少 `.h` 文件时按上节安装对应库，再重新编译。
2. Type-C UART 支持自动下载；若用 Type-C USB，按住 BOOT，按下并松开 RESET，再松开 BOOT，选择出现的端口后上传。
3. 上传完成按 RESET 运行。数据线保持连接 Type-C UART，选择与上传相同的串口查看 Serial0 反馈，无需更换接口。打开串口监视器，115200。若错过启动提示，保持监视器打开再按 RESET。

## 操作与预期结果

上传后屏幕每 500ms 更新一次 RoboCore S3 和运行秒数。修改 screen.println 的 ASCII 文本可显示自己的消息。默认字体不支持直接显示中文汉字。

## 排查与下一步

出现 OLED absent 时检查模块是否接入、0x3C 地址、线序和屏幕型号。板载惯性传感器也使用 Wire；合并程序时只初始化一次总线。

接口和电压详见[硬件说明](../hardware.md)，完整固件命令见[JSON 指令集](../json-api.md)。

## 完整代码

```cpp
// RoboCore S3：外接 OLED 显示
// 上传此独立例程后，原来的完整网页固件将被替换。
// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// Library Manager: install Adafruit SSD1306 2.5.17, Adafruit GFX Library 1.12.6, and Adafruit BusIO.
// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.
// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 screen(128, 32, &Wire, -1);
bool ready = false;
// 上电执行一次：初始化所需通信接口和设备。
void setup() {
  Serial0.begin(115200);
  Wire.begin(6, 7);
  Wire.setTimeOut(20);
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() != 0) { Serial0.println("OLED absent; continuing."); return; }
  ready = screen.begin(SSD1306_SWITCHCAPVCC, 0x3C, true, false);
  if (ready) { screen.setTextColor(SSD1306_WHITE); screen.setTextSize(1); }
}
// 循环执行：按本页操作步骤触发功能并观察反馈。
void loop() {
  if (ready) {
    screen.clearDisplay(); screen.setCursor(0, 0);
    screen.println("RoboCore S3"); screen.printf("Uptime: %lu s", millis()/1000);
    screen.display();
  }
  delay(500);
}
```
