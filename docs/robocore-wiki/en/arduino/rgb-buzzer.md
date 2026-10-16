# External RGB and buzzer (optional wiring)

**This is a standalone sketch. Uploading replaces the complete web firmware; use [firmware recovery](../firmware.md) to restore the console.**

## Hardware and wiring

Requires two separately wired WS2812 LEDs (example GPIO26) and the UI Module (A) GPIO21 buzzer. RoboCore S3 has no onboard addressable RGB; GPIO26 is not exposed on PH1.25-10P. Use User_Interface_Module_A for normal module testing. To use a free expansion GPIO, edit the leds constructor and avoid pins occupied by connected keys or buses.

## Arduino IDE and board settings

1. Install Arduino IDE 2. In **File → Preferences → Additional Boards Manager URLs**, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. Open **Tools → Board → Boards Manager**, search `esp32`, and install **3.3.0** of the package by **Espressif Systems**.
3. Select **Tools → Board → esp32 → ESP32S3 Dev Module**. Set Flash Size=8MB, PSRAM=OPI PSRAM, USB Mode=USB-OTG (TinyUSB), USB CDC On Boot=Disabled. Feedback and interactive input use Serial0 (UART0) through Type-C UART.
4. Extract the [example archive](../../../../assets/files/robocore-s3/examples.zip) and open `arduino_example/RGB_Buzzer/RGB_Buzzer.ino`. Keep each sketch in its own project.

See [Arduino IDE installation](../../../../tutorials/install-arduino-ide.md) and the [Espressif installation guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

## Libraries for this sketch

Open **Tools → Manage Libraries**, search and install each package below, choose the listed version and verify the author. Choose Install all when dependency installation is offered.

- Adafruit NeoPixel 1.15.5 (Adafruit)

[Arduino library installation guide](https://support.arduino.cc/hc/en-us/articles/5145457742236-Install-libraries-in-the-Arduino-IDE).

## Compile and upload

1. Click Verify and resolve compilation errors. If a `.h` file is missing, install the dependency listed above and compile again.
2. Type-C UART supports automatic download. For Type-C USB, hold BOOT, press/release RESET, release BOOT, select the new port and upload.
3. Press RESET to run. Keep the cable connected to Type-C UART and use the same port as uploading for Serial0 feedback; no cable change is needed. Open Serial Monitor at115200. Reset with the monitor open if you missed startup messages.

## Operation and expected results

After verifying LED power, logic levels and ground, enter l for blue, x for off, b for a1500 Hz,100 ms tone. LED brightness is set to20.

## Troubleshooting and next steps

No light is expected without external LEDs. Never apply motor-supply voltage to GPIO; use only the buzzer command when testing the buzzer alone.

See [hardware](../hardware.md) for interfaces and voltage, and the [JSON reference](../json-api.md) for full-firmware commands.

## Complete code

```cpp
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
void setup() {
  Serial0.begin(115200);
  leds.begin(); leds.setBrightness(20); leds.clear(); leds.show();
  pinMode(21, OUTPUT); digitalWrite(21, HIGH);
  Serial0.println("l=blue LEDs, x=LEDs off, b=short buzzer tone.");
}
void loop() {
  switch (Serial0.read()) {
    case 'l': leds.fill(leds.Color(0,0,255)); leds.show(); break;
    case 'x': leds.clear(); leds.show(); break;
    case 'b': tone(21, 1500, 100); break;
  }
  delay(1);
}
```
