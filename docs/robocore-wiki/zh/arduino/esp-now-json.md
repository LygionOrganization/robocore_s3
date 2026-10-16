# ESP-NOW 无线 JSON

**这是独立单元例程。上传会替换完整网页固件；需要恢复控制台时使用[固件恢复](../firmware.md)。**

## 准备硬件与接线

两块 ESP32-S3 控制板，均连接外部天线；示例使用信道1，双方需一致。

## Arduino IDE 与开发板设置

1. 安装 Arduino IDE 2。打开 **文件 → 首选项 → 附加开发板管理器网址**，加入 `https://espressif.github.io/arduino-esp32/package_esp32_index.json`。
2. 打开 **工具 → 开发板 → 开发板管理器**，搜索 `esp32`，选择作者 **Espressif Systems** 的包，安装 **3.3.0**。
3. 在 **工具 → 开发板 → esp32** 选择 **ESP32S3 Dev Module**。设置 Flash Size=8MB、PSRAM=OPI PSRAM、USB Mode=USB-OTG (TinyUSB)、USB CDC On Boot=Disabled。反馈和交互输入使用 Serial0（UART0），连接 Type-C UART。
4. 下载并解压[例程包](../../../../assets/files/robocore-s3/examples.zip)，打开 `arduino_example/ESP_NOW_JSON/ESP_NOW_JSON.ino`，不要将多个例程合并到同一工程。

安装界面详见[安装 Arduino IDE](../../../../tutorials/install-arduino-ide.md)；开发板来源见 [Espressif 安装指南](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)。

## 本例依赖

打开 **工具 → 管理库**，逐项搜索并安装下列库（版本下拉框可选择指定版本），核对作者；如提示依赖安装，选择 Install all。

- ArduinoJson 7.4.3 (Benoit Blanchon)

[Arduino 库安装说明](https://support.arduino.cc/hc/en-us/articles/5145457742236-Install-libraries-in-the-Arduino-IDE)。

## 编译与上传

1. 点击“验证”编译，确认没有错误。缺少 `.h` 文件时按上节安装对应库，再重新编译。
2. Type-C UART 支持自动下载；若用 Type-C USB，按住 BOOT，按下并松开 RESET，再松开 BOOT，选择出现的端口后上传。
3. 上传完成按 RESET 运行。数据线保持连接 Type-C UART，选择与上传相同的串口查看 Serial0 反馈，无需更换接口。打开串口监视器，115200。若错过启动提示，保持监视器打开再按 RESET。

## 操作与预期结果

先上传并记录双方串口打印的 STA MAC，随后把各自 peerMac 数组改为对方地址并重新上传。发送端串口输入 s，接收端打印 {"T":603}。这只是展示数据帧，接收单元例程不会执行 T=603。发送返回0表示交给协议栈，不等于对端业务已执行。

## 排查与下一步

接收不到时检查 MAC、信道、天线和距离。对接完整固件时需在完整固件设置 T=410/411/414，并遵守250字节补零消息帧格式。

接口和电压详见[硬件说明](../hardware.md)，完整固件命令见[JSON 指令集](../json-api.md)。

## 完整代码

```cpp
// RoboCore S3：ESP-NOW 无线 JSON
// 上传此独立例程后，原来的完整网页固件将被替换。
// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// Library Manager: install ArduinoJson 7.4.3 by Benoit Blanchon.
// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.
// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>
#include <freertos/queue.h>
QueueHandle_t pending;
// Both peers must use the same channel. Replace with the other board's STA MAC.
uint8_t peerMac[6] = {0x24,0x6F,0x28,0x00,0x00,0x01};
bool ready = false;
void receive(const esp_now_recv_info_t*, const uint8_t* data, int len) {
  if (len <= 0 || len > 250) return;
  char message[251] = {};
  memcpy(message, data, len);
  xQueueOverwrite(pending, message); // No parsing, logging or bus I/O in callback.
}
// 上电执行一次：初始化所需通信接口和设备。
void setup() {
  Serial0.begin(115200);
  pending = xQueueCreate(1, 251); configASSERT(pending);
  WiFi.mode(WIFI_STA); esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  Serial0.println(WiFi.macAddress());
  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb(receive);
  esp_now_peer_info_t peer = {}; memcpy(peer.peer_addr, peerMac, 6); peer.channel = 1;
  ready = esp_now_add_peer(&peer) == ESP_OK;
  Serial0.println("Send s to transmit a JSON message to the configured peer.");
}
// 循环执行：按本页操作步骤触发功能并观察反馈。
void loop() {
  char message[251];
  if (xQueueReceive(pending, message, 0) == pdPASS) {
    JsonDocument doc;
    if (!deserializeJson(doc, message)) { serializeJson(doc, Serial0); Serial0.println(); }
  }
  if (ready && Serial0.read() == 's') {
    // Factory firmware accepts JSON in a zero-padded 250-byte message frame.
    char payload[250] = "{\"T\":603}";
    Serial0.printf("Send result=%d\n", esp_now_send(peerMac, reinterpret_cast<uint8_t*>(payload), sizeof(payload)));
  }
  delay(1);
}
```
