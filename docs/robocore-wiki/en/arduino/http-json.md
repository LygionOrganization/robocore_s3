# HTTP JSON communication

**This is a standalone sketch. Uploading replaces the complete web firmware; use [firmware recovery](../firmware.md) to restore the console.**

## Hardware and wiring

A Wi-Fi computer; attach the external antenna before running. This sketch only reads/writes a RAM value and does not control motors.

## Arduino IDE and board settings

1. Install Arduino IDE 2. In **File → Preferences → Additional Boards Manager URLs**, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. Open **Tools → Board → Boards Manager**, search `esp32`, and install **3.3.0** of the package by **Espressif Systems**.
3. Select **Tools → Board → esp32 → ESP32S3 Dev Module**. Set Flash Size=8MB, PSRAM=OPI PSRAM, USB Mode=USB-OTG (TinyUSB), USB CDC On Boot=Disabled. Feedback and interactive input use Serial0 (UART0) through Type-C UART.
4. Extract the [example archive](../../../../assets/files/robocore-s3/examples.zip) and open `arduino_example/HTTP_JSON/HTTP_JSON.ino`. Keep each sketch in its own project.

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

Connect to RoboCoreDemo with password12345678. POST {"T":1,"value":42} to http://192.168.4.1/api/cmd for {"ok":true,"value":42}; send {"T":2} to read. Value resets on reboot. Use the PowerShell example below; typing the address in a browser sends GET, not POST.

## Troubleshooting and next steps

HTTP400 indicates invalid or oversized input (over255 bytes). No home page is expected: this sketch exposes only an API; restore full firmware for the console.

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
#include <WebServer.h>
#include <ArduinoJson.h>
WebServer server(80);
int value = 0;
void setup() {
  Serial0.begin(115200);
  WiFi.softAP("RoboCoreDemo", "12345678");
  server.on("/api/cmd", HTTP_POST, []() {
    String body = server.arg("plain");
    JsonDocument in, out;
    if (body.length() > 255 || deserializeJson(in, body) || !in["T"].is<int>()) {
      server.send(400, "application/json", "{\"ok\":false}"); return;
    }
    // Teaching protocol: T=1 sets a RAM value, T=2 reads it. No motors move.
    int type = in["T"];
    bool ok = type == 2 || (type == 1 && in["value"].is<int>());
    if (ok && type == 1) value = in["value"];
    out["ok"] = ok; out["value"] = value;
    String response; serializeJson(out, response);
    server.send(ok ? 200 : 400, "application/json", response);
  });
  server.begin();
  Serial0.println("POST http://192.168.4.1/api/cmd with {\"T\":1,\"value\":42}");
}
void loop() { server.handleClient(); delay(1); }
```

### PowerShell

```powershell
Invoke-RestMethod -Uri http://192.168.4.1/api/cmd -Method Post -ContentType application/json -Body '{"T":1,"value":42}'
Invoke-RestMethod -Uri http://192.168.4.1/api/cmd -Method Post -ContentType application/json -Body '{"T":2}'
```
