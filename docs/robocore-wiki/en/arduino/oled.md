# External OLED display

**This is a standalone sketch. Uploading replaces the complete web firmware; use [firmware recovery](../firmware.md) to restore the console.**

## Hardware and wiring

User Interface Module (A) or compatible SSD1306 128×32 I²C OLED, SDA6/SCL7, address 0x3C; use correct module power and 3.3V GPIO levels.

## Arduino IDE and board settings

1. Install Arduino IDE 2. In **File → Preferences → Additional Boards Manager URLs**, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. Open **Tools → Board → Boards Manager**, search `esp32`, and install **3.3.0** of the package by **Espressif Systems**.
3. Select **Tools → Board → esp32 → ESP32S3 Dev Module**. Set Flash Size=8MB, PSRAM=OPI PSRAM, USB Mode=USB-OTG (TinyUSB), USB CDC On Boot=Disabled. Feedback and interactive input use Serial0 (UART0) through Type-C UART.
4. Extract the [example archive](../../../../assets/files/robocore-s3/examples.zip) and open `arduino_example/OLED/OLED.ino`. Keep each sketch in its own project.

See [Arduino IDE installation](../../../../tutorials/install-arduino-ide.md) and the [Espressif installation guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

## Libraries for this sketch

Open **Tools → Manage Libraries**, search and install each package below, choose the listed version and verify the author. Choose Install all when dependency installation is offered.

- Adafruit SSD1306 2.5.17
- Adafruit GFX Library 1.12.6
- Adafruit BusIO

[Arduino library installation guide](https://support.arduino.cc/hc/en-us/articles/5145457742236-Install-libraries-in-the-Arduino-IDE).

## Compile and upload

1. Click Verify and resolve compilation errors. If a `.h` file is missing, install the dependency listed above and compile again.
2. Type-C UART supports automatic download. For Type-C USB, hold BOOT, press/release RESET, release BOOT, select the new port and upload.
3. Press RESET to run. Keep the cable connected to Type-C UART and use the same port as uploading for Serial0 feedback; no cable change is needed. Open Serial Monitor at115200. Reset with the monitor open if you missed startup messages.

## Operation and expected results

After upload, the display updates RoboCore S3 and uptime every 500 ms. Change ASCII screen.println text for your own message. The default font cannot directly render Chinese characters.

## Troubleshooting and next steps

If OLED absent appears, check connection, address 0x3C, wiring and display model. The onboard IMU shares Wire; initialize the bus once when combining sketches.

See [hardware](../hardware.md) for interfaces and voltage, and the [JSON reference](../json-api.md) for full-firmware commands.

## Complete code

```cpp
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
void setup() {
  Serial0.begin(115200);
  Wire.begin(6, 7);
  Wire.setTimeOut(20);
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() != 0) { Serial0.println("OLED absent; continuing."); return; }
  ready = screen.begin(SSD1306_SWITCHCAPVCC, 0x3C, true, false);
  if (ready) { screen.setTextColor(SSD1306_WHITE); screen.setTextSize(1); }
}
void loop() {
  if (ready) {
    screen.clearDisplay(); screen.setCursor(0, 0);
    screen.println("RoboCore S3"); screen.printf("Uptime: %lu s", millis()/1000);
    screen.display();
  }
  delay(500);
}
```
