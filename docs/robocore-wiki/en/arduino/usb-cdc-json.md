# Native USB CDC JSON communication

**This is a standalone sketch. Uploading replaces the complete web firmware; use [firmware recovery](../firmware.md) to restore the console.**

## Hardware and wiring

Connect a data cable to Type-C USB (GPIO19/20); select USB-OTG (TinyUSB) and USB CDC On Boot Enabled.

## Arduino IDE and board settings

1. Install Arduino IDE 2. In **File → Preferences → Additional Boards Manager URLs**, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. Open **Tools → Board → Boards Manager**, search `esp32`, and install **3.3.0** of the package by **Espressif Systems**.
3. Select **Tools → Board → esp32 → ESP32S3 Dev Module**. Set Flash Size=8MB, PSRAM=OPI PSRAM, USB Mode=USB-OTG (TinyUSB), USB CDC On Boot=Enabled. Serial output uses native USB.
4. Extract the [example archive](../../../../assets/files/robocore-s3/examples.zip) and open `arduino_example/USB_CDC_JSON/USB_CDC_JSON.ino`. Keep each sketch in its own project.

See [Arduino IDE installation](../../../../tutorials/install-arduino-ide.md) and the [Espressif installation guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

## Libraries for this sketch

Open **Tools → Manage Libraries**, search and install each package below, choose the listed version and verify the author. Choose Install all when dependency installation is offered.

- ArduinoJson 7.4.3 (Benoit Blanchon)

[Arduino library installation guide](https://support.arduino.cc/hc/en-us/articles/5145457742236-Install-libraries-in-the-Arduino-IDE).

## Compile and upload

1. Click Verify and resolve compilation errors. If a `.h` file is missing, install the dependency listed above and compile again.
2. Type-C UART supports automatic download. For Type-C USB, hold BOOT, press/release RESET, release BOOT, select the new port and upload.
3. Press RESET to run. If you uploaded over Type-C UART, move the data cable to Type-C USB for this sketch’s Serial output and select the re-enumerated port. Open Serial Monitor at115200. Reset with the monitor open if you missed startup messages.

## Operation and expected results

Select the native USB port and newline in Serial Monitor. Send {"T":1,"value":42}, then {"T":2}; replies contain ok=true,value=42. Keep reading replies to drain the output buffer. USB EVENT receives up to255 bytes per line; only the latest pending command is retained.

## Troubleshooting and next steps

No reply: check Type-C USB, newline and CDC settings. Do not open the same port in Serial Monitor and Python simultaneously. This teaching protocol does not implement factory bus-control commands.

See [hardware](../hardware.md) for interfaces and voltage, and the [JSON reference](../json-api.md) for full-firmware commands.

## Complete code

```cpp
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
void setup() {
  pending = xQueueCreate(1, 256); configASSERT(pending);
  USBSerial.setRxBufferSize(2048); USBSerial.onEvent(event);
  USBSerial.begin(); USB.begin();
}
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
