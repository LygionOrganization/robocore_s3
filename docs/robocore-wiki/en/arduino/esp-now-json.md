# ESP-NOW wireless JSON

**This is a standalone sketch. Uploading replaces the complete web firmware; use [firmware recovery](../firmware.md) to restore the console.**

## Hardware and wiring

Two ESP32-S3 controllers with external antennas connected; both use channel1.

## Arduino IDE and board settings

1. Install Arduino IDE 2. In **File → Preferences → Additional Boards Manager URLs**, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. Open **Tools → Board → Boards Manager**, search `esp32`, and install **3.3.0** of the package by **Espressif Systems**.
3. Select **Tools → Board → esp32 → ESP32S3 Dev Module**. Set Flash Size=8MB, PSRAM=OPI PSRAM, USB Mode=USB-OTG (TinyUSB), USB CDC On Boot=Disabled. Feedback and interactive input use Serial0 (UART0) through Type-C UART.
4. Extract the [example archive](../../../../assets/files/robocore-s3/examples.zip) and open `arduino_example/ESP_NOW_JSON/ESP_NOW_JSON.ino`. Keep each sketch in its own project.

See [Arduino IDE installation](../../../../tutorials/install-arduino-ide.md) and the [Espressif installation guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

## Libraries for this sketch

Open **Tools → Manage Libraries**, search and install each package below, choose the listed version and verify the author. Choose Install all when dependency installation is offered.

- ArduinoJson 7.4.3 (Benoit Blanchon)

[Arduino library installation guide](https://support.arduino.cc/hc/en-us/articles/5145457742236-Install-libraries-in-the-Arduino-IDE).

## Compile and upload

1. Click Verify and resolve compilation errors. If a `.h` file is missing, install the dependency listed above and compile again.
2. Type-C UART supports automatic download. For Type-C USB, hold BOOT, press/release RESET, release BOOT, select the new port and upload.
3. Press RESET to run. Keep the cable connected to Type-C UART and use the same port as uploading for Serial0 feedback; no cable change is needed. Open Serial Monitor at115200. Reset with the monitor open if you missed startup messages.

## Operation and expected results

Upload first to read each STA MAC, then set each peerMac array to the other board’s address and upload again. Enter s on the sender; the receiving sketch prints {"T":603}. This demonstrates a packet, not execution of T=603. Send result0 means stack submission, not confirmed application execution.

## Troubleshooting and next steps

Check MAC, channel, antennas and range if nothing arrives. To communicate with full firmware, configure T=410/411/414 there and use its250-byte zero-padded JSON frame.

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
