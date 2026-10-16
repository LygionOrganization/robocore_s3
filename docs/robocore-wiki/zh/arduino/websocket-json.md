# WebSocket JSON 通信

**这是独立单元例程。上传会替换完整网页固件；需要恢复控制台时使用[固件恢复](../firmware.md)。**

## 准备硬件与接线

带 Wi-Fi 的电脑及 WebSocket 客户端，先连接外部天线。

## Arduino IDE 与开发板设置

1. 安装 Arduino IDE 2。打开 **文件 → 首选项 → 附加开发板管理器网址**，加入 `https://espressif.github.io/arduino-esp32/package_esp32_index.json`。
2. 打开 **工具 → 开发板 → 开发板管理器**，搜索 `esp32`，选择作者 **Espressif Systems** 的包，安装 **3.3.0**。
3. 在 **工具 → 开发板 → esp32** 选择 **ESP32S3 Dev Module**。设置 Flash Size=8MB、PSRAM=OPI PSRAM、USB Mode=USB-OTG (TinyUSB)、USB CDC On Boot=Disabled。反馈和交互输入使用 Serial0（UART0），连接 Type-C UART。
4. 下载并解压[例程包](../../../../assets/files/robocore-s3/examples.zip)，打开 `arduino_example/WebSocket_JSON/WebSocket_JSON.ino`，不要将多个例程合并到同一工程。

安装界面详见[安装 Arduino IDE](../../../../tutorials/install-arduino-ide.md)；开发板来源见 [Espressif 安装指南](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)。

## 本例依赖

打开 **工具 → 管理库**，逐项搜索并安装下列库（版本下拉框可选择指定版本），核对作者；如提示依赖安装，选择 Install all。

- ArduinoJson 7.4.3 (Benoit Blanchon)
- AsyncTCP 3.5.0 (ESP32Async)
- ESPAsyncWebServer 3.12.1 (ESP32Async)

[Arduino 库安装说明](https://support.arduino.cc/hc/en-us/articles/5145457742236-Install-libraries-in-the-Arduino-IDE)。

## 编译与上传

1. 点击“验证”编译，确认没有错误。缺少 `.h` 文件时按上节安装对应库，再重新编译。
2. Type-C UART 支持自动下载；若用 Type-C USB，按住 BOOT，按下并松开 RESET，再松开 BOOT，选择出现的端口后上传。
3. 上传完成按 RESET 运行。数据线保持连接 Type-C UART，选择与上传相同的串口查看 Serial0 反馈，无需更换接口。打开串口监视器，115200。若错过启动提示，保持监视器打开再按 RESET。

## 操作与预期结果

连接热点 RoboCoreDemo / 12345678，客户端打开 ws://192.168.4.1/ws。发送单个文本帧 {"T":1,"value":42}，再发送 {"T":2}，均收到 ok/value。可用下方 Python 客户端。

## 排查与下一步

必须使用 ESP32Async 的两个库；不要混入旧分支。例程拒绝二进制帧、分片帧、超过255字节的文本。浏览器普通地址栏不能代替 WebSocket 客户端。

接口和电压详见[硬件说明](../hardware.md)，完整固件命令见[JSON 指令集](../json-api.md)。

## 完整代码

```cpp
// RoboCore S3：WebSocket JSON 通信
// 上传此独立例程后，原来的完整网页固件将被替换。
// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// Library Manager: install ArduinoJson 7.4.3 by Benoit Blanchon.
// Library Manager: install AsyncTCP 3.5.0 and ESPAsyncWebServer 3.12.1 by ESP32Async.
// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.
// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
AsyncWebServer server(80);
AsyncWebSocket socket("/ws");
int value = 0;
void event(AsyncWebSocket*, AsyncWebSocketClient* client, AwsEventType type,
           void* arg, uint8_t* data, size_t len) {
  if (type != WS_EVT_DATA) return;
  auto* frame = static_cast<AwsFrameInfo*>(arg);
  if (!frame->final || frame->index || frame->len != len || frame->opcode != WS_TEXT || len > 255) {
    client->text("{\"ok\":false,\"error\":\"one short text frame required\"}"); return;
  }
  JsonDocument in, out;
  if (deserializeJson(in, data, len) || !in["T"].is<int>()) {
    client->text("{\"ok\":false}"); return;
  }
  // Teaching protocol only; the complete firmware has a larger command table.
  int typeId = in["T"];
  bool ok = typeId == 2 || (typeId == 1 && in["value"].is<int>());
  if (ok && typeId == 1) value = in["value"];
  out["ok"] = ok; out["value"] = value;
  String reply; serializeJson(out, reply); client->text(reply);
}
// 上电执行一次：初始化所需通信接口和设备。
void setup() {
  Serial0.begin(115200);
  WiFi.softAP("RoboCoreDemo", "12345678");
  socket.onEvent(event); server.addHandler(&socket); server.begin();
  Serial0.println("Connect ws://192.168.4.1/ws; send {\"T\":1,\"value\":42}");
}
// 循环执行：按本页操作步骤触发功能并观察反馈。
void loop() { socket.cleanupClients(); delay(10); }
```

### Python

```shell
python -m pip install websocket-client
```

```python
import websocket
ws = websocket.create_connection("ws://192.168.4.1/ws", timeout=5)
try:
    ws.send('{"T":1,"value":42}')
    print(ws.recv())
    ws.send('{"T":2}')
    print(ws.recv())
finally:
    ws.close()
```
