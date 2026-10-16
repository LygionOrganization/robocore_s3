# WebSocket JSON communication

**This is a standalone sketch. Uploading replaces the complete web firmware; use [firmware recovery](../firmware.md) to restore the console.**

## Hardware and wiring

Wi-Fi computer and a WebSocket client; attach the external antenna first.

## Arduino IDE and board settings

1. Install Arduino IDE 2. In **File → Preferences → Additional Boards Manager URLs**, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. Open **Tools → Board → Boards Manager**, search `esp32`, and install **3.3.0** of the package by **Espressif Systems**.
3. Select **Tools → Board → esp32 → ESP32S3 Dev Module**. Set Flash Size=8MB, PSRAM=OPI PSRAM, USB Mode=USB-OTG (TinyUSB), USB CDC On Boot=Disabled. Feedback and interactive input use Serial0 (UART0) through Type-C UART.
4. Extract the [example archive](../../../../assets/files/robocore-s3/examples.zip) and open `arduino_example/WebSocket_JSON/WebSocket_JSON.ino`. Keep each sketch in its own project.

See [Arduino IDE installation](../../../../tutorials/install-arduino-ide.md) and the [Espressif installation guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

## Libraries for this sketch

Open **Tools → Manage Libraries**, search and install each package below, choose the listed version and verify the author. Choose Install all when dependency installation is offered.

- ArduinoJson 7.4.3 (Benoit Blanchon)
- AsyncTCP 3.5.0 (ESP32Async)
- ESPAsyncWebServer 3.12.1 (ESP32Async)

[Arduino library installation guide](https://support.arduino.cc/hc/en-us/articles/5145457742236-Install-libraries-in-the-Arduino-IDE).

## Compile and upload

1. Click Verify and resolve compilation errors. If a `.h` file is missing, install the dependency listed above and compile again.
2. Type-C UART supports automatic download. For Type-C USB, hold BOOT, press/release RESET, release BOOT, select the new port and upload.
3. Press RESET to run. Keep the cable connected to Type-C UART and use the same port as uploading for Serial0 feedback; no cable change is needed. Open Serial Monitor at115200. Reset with the monitor open if you missed startup messages.

## Operation and expected results

Connect to RoboCoreDemo / 12345678 and open ws://192.168.4.1/ws. Send one text frame {"T":1,"value":42}, then {"T":2}; replies contain ok/value. A Python client is shown below.

## Troubleshooting and next steps

Use both libraries from ESP32Async, without mixing old forks. The sketch rejects binary, fragmented and over255-byte frames. An ordinary browser address bar is not a WebSocket client.

See [hardware](../hardware.md) for interfaces and voltage, and the [JSON reference](../json-api.md) for full-firmware commands.

## Complete code

```cpp
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
void setup() {
  Serial0.begin(115200);
  WiFi.softAP("RoboCoreDemo", "12345678");
  socket.onEvent(event); server.addHandler(&socket); server.begin();
  Serial0.println("Connect ws://192.168.4.1/ws; send {\"T\":1,\"value\":42}");
}
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
