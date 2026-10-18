# 外接 RGB 与蜂鸣器（可选接线）

**这是独立单元例程。上传会替换完整网页固件；需要恢复控制台时使用[固件恢复](../firmware.md)。**

## 准备硬件与接线

本例需要另行接线的两颗 WS2812 灯（示例 GPIO26）及 UI Module (A) 的 GPIO21 蜂鸣器。RoboCore S3 无板载可寻址 RGB，GPIO26 未引出到 PH1.25-10P；使用 UI 模块的按键、屏幕和蜂鸣器时，请选择 User_Interface_Module_A。若改用扩展口空闲 GPIO，请先修改 leds 构造函数，且不能与已连接按键或总线共用。

## Arduino IDE 与开发板设置

1. 安装 Arduino IDE 2。打开 **文件 → 首选项 → 附加开发板管理器网址**，加入 `https://espressif.github.io/arduino-esp32/package_esp32_index.json`。
2. 打开 **工具 → 开发板 → 开发板管理器**，搜索 `esp32`，选择作者 **Espressif Systems** 的包，安装 **3.3.0**。
3. 在 **工具 → 开发板 → esp32** 选择 **ESP32S3 Dev Module**。设置 Flash Size=8MB、PSRAM=OPI PSRAM、USB Mode=USB-OTG (TinyUSB)、USB CDC On Boot=Disabled。反馈和交互输入使用 Serial0（UART0），连接 Type-C UART。
4. 下载并解压[例程包](../../../../assets/files/robocore-s3/examples.zip)，打开 `arduino_example/RGB_Buzzer/RGB_Buzzer.ino`，不要将多个例程合并到同一工程。

安装界面详见[安装 Arduino IDE](../../../../tutorials/install-arduino-ide.md)；开发板来源见 [Espressif 安装指南](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)。

## 本例依赖

打开 **工具 → 管理库**，逐项搜索并安装下列库（版本下拉框可选择指定版本），核对作者；如提示依赖安装，选择 Install all。

- Adafruit NeoPixel 1.15.5 (Adafruit)

[Arduino 库安装说明](https://support.arduino.cc/hc/en-us/articles/5145457742236-Install-libraries-in-the-Arduino-IDE)。

## 编译与上传

1. 点击“验证”编译，确认没有错误。缺少 `.h` 文件时按上节安装对应库，再重新编译。
2. Type-C UART 支持自动下载；若用 Type-C USB，按住 BOOT，按下并松开 RESET，再松开 BOOT，选择出现的端口后上传。
3. 上传完成按 RESET 运行。数据线保持连接 Type-C UART，选择与上传相同的串口查看 Serial0 反馈，无需更换接口。打开串口监视器，115200。若错过启动提示，保持监视器打开再按 RESET。

## 操作与预期结果

确认灯带供电、逻辑电平和共地后，输入 l 显示蓝色，x 关闭灯；b 发出1500Hz、100ms短鸣。灯的亮度在代码中设为20。

## 排查与下一步

没有另外接灯时不会出现灯光，这是预期行为。不要向 GPIO 接入动力电压；只测试蜂鸣器时无需运行灯带操作。

接口和电压详见[硬件说明](../hardware.md)，完整固件命令见[JSON 指令集](../json-api.md)。

## 完整代码

```cpp
// RoboCore S3：外接 RGB 与蜂鸣器（可选接线）
// 上传此独立例程后，原来的完整网页固件将被替换。
// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// Library Manager: install Adafruit NeoPixel 1.15.5 by Adafruit.
// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.
// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.
// END EXAMPLE SETUP
// Optional external WS2812 wiring only; RoboCore S3 has no onboard addressable RGB LEDs.
// For the supported expansion module use User_Interface_Module_A instead.
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel leds(2, 26, NEO_GRB + NEO_KHZ800);
// 上电执行一次：初始化所需通信接口和设备。
void setup() {
  Serial0.begin(115200);
  leds.begin(); leds.setBrightness(20); leds.clear(); leds.show();
  pinMode(21, OUTPUT); digitalWrite(21, HIGH);
  Serial0.println("l=blue LEDs, x=LEDs off, b=short buzzer tone.");
}
// 循环执行：按本页操作步骤触发功能并观察反馈。
void loop() {
  switch (Serial0.read()) {
    case 'l': leds.fill(leds.Color(0,0,255)); leds.show(); break;
    case 'x': leds.clear(); leds.show(); break;
    case 'b': tone(21, 1500, 100); break;
  }
  delay(1);
}
```
