# 原生 USB CDC JSON 通信

**这是独立单元例程。上传会替换完整网页固件；需要恢复控制台时使用[固件恢复](../firmware.md)。**

## 准备硬件与接线

USB 数据线连接 Type-C USB（IO19/20），选择 USB-OTG (TinyUSB) 和 USB CDC On Boot Enabled。

## Arduino IDE 与开发板设置

1. 安装 Arduino IDE 2。打开 **文件 → 首选项 → 附加开发板管理器网址**，加入 `https://espressif.github.io/arduino-esp32/package_esp32_index.json`。
2. 打开 **工具 → 开发板 → 开发板管理器**，搜索 `esp32`，选择作者 **Espressif Systems** 的包，安装 **3.3.0**。
3. 在 **工具 → 开发板 → esp32** 选择 **ESP32S3 Dev Module**。设置 Flash Size=8MB、PSRAM=OPI PSRAM、USB Mode=USB-OTG (TinyUSB)、USB CDC On Boot=Enabled。串口输出使用原生 USB。
4. 下载并解压[例程包](../../../../assets/files/robocore-s3/examples.zip)，打开 `arduino_example/USB_CDC_JSON/USB_CDC_JSON.ino`，不要将多个例程合并到同一工程。

安装界面详见[安装 Arduino IDE](../../../../tutorials/install-arduino-ide.md)；开发板来源见 [Espressif 安装指南](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)。

## 本例依赖

打开 **工具 → 管理库**，逐项搜索并安装下列库（版本下拉框可选择指定版本），核对作者；如提示依赖安装，选择 Install all。

- ArduinoJson 7.4.3 (Benoit Blanchon)

[Arduino 库安装说明](https://support.arduino.cc/hc/en-us/articles/5145457742236-Install-libraries-in-the-Arduino-IDE)。

## 编译与上传

1. 点击“验证”编译，确认没有错误。缺少 `.h` 文件时按上节安装对应库，再重新编译。
2. Type-C UART 支持自动下载；若用 Type-C USB，按住 BOOT，按下并松开 RESET，再松开 BOOT，选择出现的端口后上传。
3. 上传完成按 RESET 运行。使用 Type-C UART 下载时，将数据线切到 Type-C USB 观察本例的 Serial 输出；选择重新枚举的端口。打开串口监视器，115200。若错过启动提示，保持监视器打开再按 RESET。

## 操作与预期结果

串口监视器选择原生 USB 端口并启用换行，发送 {"T":1,"value":42}，随后 {"T":2}；回复 ok=true、value=42。连续读取回复，避免输出缓冲塞满。程序通过 USB EVENT 收包，每行最多255字节，等待队列只保留最新一条。

## 排查与下一步

无回复先确认接的是 Type-C USB、已发送换行且 CDC 设置正确；不要同时用串口监视器和 Python 占用同一端口。该教学协议不支持完整固件的总线控制命令。

接口和电压详见[硬件说明](../hardware.md)，完整固件命令见[JSON 指令集](../json-api.md)。

## 完整代码

```cpp
// RoboCore S3：原生 USB CDC JSON 通信
// 上传此独立例程后，原来的完整网页固件将被替换。
// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Connect Type-C USB; select USB-OTG (TinyUSB), USB CDC On Boot = Enabled.
// Library Manager: install ArduinoJson 7.4.3 by Benoit Blanchon.
// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.
// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <ArduinoJson.h>
#include <USB.h>
#include <USBCDC.h>
#include <freertos/queue.h>
// Select USB-OTG (TinyUSB) and USB CDC On Boot = Enabled in Arduino Tools.
QueueHandle_t pending;
void event(void*, esp_event_base_t base, int32_t type, void*) {
  static char line[256]; static size_t used = 0; static bool drop = false;
  if (base != ARDUINO_USB_CDC_EVENTS) return;
  if (type == ARDUINO_USB_CDC_DISCONNECTED_EVENT) { used = 0; drop = false; return; }
  if (type == ARDUINO_USB_CDC_RX_OVERFLOW_EVENT) { used = 0; drop = true; }
  else if (type != ARDUINO_USB_CDC_RX_EVENT) return;
  int remaining = USBSerial.available();
  while (remaining-- > 0) {
    int c = USBSerial.read(); if (c < 0) break;
    if (c == '\n') {
      if (!drop && used) { line[used] = 0; xQueueOverwrite(pending, line); }
      used = 0; drop = false;
    } else if (c == 0) { used = 0; drop = true; }
    else if (c != '\r' && !drop) {
      if (used < sizeof(line) - 1) line[used++] = c;
      else { used = 0; drop = true; }
    }
  }
}
// 上电执行一次：初始化所需通信接口和设备。
void setup() {
  pending = xQueueCreate(1, 256); configASSERT(pending);
  USBSerial.setRxBufferSize(2048); USBSerial.onEvent(event);
  USBSerial.begin(); USB.begin();
}
// 循环执行：按本页操作步骤触发功能并观察反馈。
void loop() {
  static int value = 0;
  char line[256];
  if (xQueueReceive(pending, line, 0) == pdPASS) {
    JsonDocument in, out;
    bool valid = !deserializeJson(in, line) && in["T"].is<int>();
    int type = valid ? in["T"].as<int>() : -1;
    bool ok = valid && (type == 2 || (type == 1 && in["value"].is<int>()));
    if (ok && type == 1) value = in["value"];
    out["ok"] = ok; out["value"] = value;
    // Drop a complete response if the host is not draining USB.
    // This single-writer example never writes beyond the free TX FIFO.
    char reply[96];
    size_t n = serializeJson(out, reply, sizeof(reply) - 1);
    reply[n++] = '\n';
    if (USBSerial.availableForWrite() >= static_cast<int>(n))
      USBSerial.write(reinterpret_cast<const uint8_t*>(reply), n);
  }
  delay(1);
}
```
