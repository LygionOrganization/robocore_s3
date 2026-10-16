# User Interface Module (A): keys, buzzer and display

**This is a standalone sketch. Uploading replaces the complete web firmware; use [firmware recovery](../firmware.md) to restore the console.**

## Hardware and wiring

Connect UI Module (A) to PH1.25-10P with power off. Up/Down/Left/Right/OK use GPIO10/11/12/9/2, active low; buzzer21; OLED SDA6/SCL7, 0x3C, 128×32. All these resources are on the external module.

## Arduino IDE and board settings

1. Install Arduino IDE 2. In **File → Preferences → Additional Boards Manager URLs**, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. Open **Tools → Board → Boards Manager**, search `esp32`, and install **3.3.0** of the package by **Espressif Systems**.
3. Select **Tools → Board → esp32 → ESP32S3 Dev Module**. Set Flash Size=8MB, PSRAM=OPI PSRAM, USB Mode=USB-OTG (TinyUSB), USB CDC On Boot=Disabled. Feedback and interactive input use Serial0 (UART0) through Type-C UART.
4. Extract the [example archive](../../../../assets/files/robocore-s3/examples.zip) and open `arduino_example/User_Interface_Module_A/User_Interface_Module_A.ino`. Keep each sketch in its own project.

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

Press each key: serial prints Button: UP/DOWN/LEFT/RIGHT/OK, the OLED updates Last, and the buzzer sounds for 80 ms. Debounce is 30 ms; holding a key emits once until released and pressed again. This sketch sends no motor commands.

## Troubleshooting and next steps

Buttons and buzzer continue if OLED is unavailable. Check module orientation, ground and GPIO if keys fail; RESET/BOOT are not the module’s five keys.

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

// Connect User Interface Module (A) to the PH1.25-10P expansion port.
// OLED and onboard QMI8658A share SDA=6 / SCL=7; initialize Wire only once.
constexpr uint8_t buttonPins[] = {10, 11, 12, 9, 2};
const char *buttonNames[] = {"UP", "DOWN", "LEFT", "RIGHT", "OK"};
constexpr uint8_t buzzerPin = 21;
bool rawState[5], stableState[5];
uint32_t changedAt[5] = {};
uint32_t beepAt = 0;
bool beeping = false, screenReady = false;
Adafruit_SSD1306 screen(128, 32, &Wire, -1);

void showButton(const char *name) {
  Serial0.printf("Button: %s\n", name);
  if (!screenReady) return;
  screen.clearDisplay();
  screen.setCursor(0, 0);
  screen.println("RoboCore S3 / UI (A)");
  screen.println("Press a button");
  screen.print("Last: "); screen.println(name);
  screen.display();
}

void setup() {
  Serial0.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, HIGH);
  for (size_t i = 0; i < 5; ++i) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    rawState[i] = stableState[i] = digitalRead(buttonPins[i]);
  }
  Wire.begin(6, 7);
  Wire.setTimeOut(20);
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() == 0)
    screenReady = screen.begin(SSD1306_SWITCHCAPVCC, 0x3C, true, false);
  if (screenReady) {
    screen.setTextColor(SSD1306_WHITE);
    screen.setTextSize(1);
  } else Serial0.println("OLED unavailable; buttons and buzzer remain active.");
  showButton("None");
}

void loop() {
  const uint32_t now = millis();
  for (size_t i = 0; i < 5; ++i) {
    const bool state = digitalRead(buttonPins[i]);
    if (state != rawState[i]) { rawState[i] = state; changedAt[i] = now; }
    // Debounce both edges; emit one event per press, without blocking other keys.
    if (state != stableState[i] && uint32_t(now - changedAt[i]) >= 30) {
      stableState[i] = state;
      if (state == LOW) {
        showButton(buttonNames[i]);
        tone(buzzerPin, 1500);
        beepAt = now; beeping = true;
      }
    }
  }
  if (beeping && uint32_t(now - beepAt) >= 80) {
    noTone(buzzerPin); digitalWrite(buzzerPin, HIGH); beeping = false;
  }
  delay(1);
}
```
