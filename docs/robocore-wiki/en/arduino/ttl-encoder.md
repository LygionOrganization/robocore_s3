# TTL Encoder E02

**This is a standalone sketch. Uploading replaces the complete web firmware; use [firmware recovery](../firmware.md) to restore the console.**

## Hardware and wiring

TTL Encoder E02, ID 1 at 1000000 baud on IO4/IO5 TTL; rotate its shaft by hand to inspect feedback.

## Arduino IDE and board settings

1. Install Arduino IDE 2. In **File → Preferences → Additional Boards Manager URLs**, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. Open **Tools → Board → Boards Manager**, search `esp32`, and install **3.3.0** of the package by **Espressif Systems**.
3. Select **Tools → Board → esp32 → ESP32S3 Dev Module**. Set Flash Size=8MB, PSRAM=OPI PSRAM, USB Mode=USB-OTG (TinyUSB), USB CDC On Boot=Disabled. Feedback and interactive input use Serial0 (UART0) through Type-C UART.
4. Extract the [example archive](../../../../assets/files/robocore-s3/examples.zip) and open `arduino_example/TTL_Encoder/TTL_Encoder.ino`. Keep each sketch in its own project.

See [Arduino IDE installation](../../../../tutorials/install-arduino-ide.md) and the [Espressif installation guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

## Libraries for this sketch

This sketch needs the included **lygion_devs**, not a similarly named third-party package. Find Sketchbook location in File → Preferences, copy the entire `lib/lygion_devs` folder to `<Sketchbook>/libraries/lygion_devs`, with `library.properties` and `src` directly inside, then restart the IDE. Avoid an extra nested folder.

## Compile and upload

1. Click Verify and resolve compilation errors. If a `.h` file is missing, install the dependency listed above and compile again.
2. Type-C UART supports automatic download. For Type-C USB, hold BOOT, press/release RESET, release BOOT, select the new port and upload.
3. Press RESET to run. Keep the cable connected to Type-C UART and use the same port as uploading for Serial0 feedback; no cable change is needed. Open Serial Monitor at115200. Reset with the monitor open if you missed startup messages.

## Operation and expected results

Send `r` for position, angle and speed. There are 4096 counts per turn; displayed angle = position × 360/4096 and includes accumulated turns. Send `z` only when calibration is needed to assign zero to the current position; calibration is persistent, so do not call it repeatedly.

## Troubleshooting and next steps

Position range is 0–65534. The calibration reference persists, but accumulated turns do not survive power loss. Check ID, baud and power on read failure; never use errors as angles.

See [hardware](../hardware.md) for interfaces and voltage, and the [JSON reference](../json-api.md) for full-firmware commands.

## Complete code

```cpp
// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// Bundled library: copy the entire lib/lygion_devs folder from this project
// to your Arduino sketchbook libraries/lygion_devs folder, then restart the IDE.
// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.
// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <lygion_devs.h>
// E02 uses the same position/speed register protocol as TTLSDClass.
TTLSDClass encoder;
constexpr uint8_t ID = 1;
void setup() {
  Serial0.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, 5, 4);
  encoder.pSerial = &Serial1;
  Serial0.println("r=read, z=save current position as 0 (persistent calibration).");
}
void loop() {
  char c = Serial0.read();
  if (c == 'z') {
    encoder.CalibrationOfs(ID, 0);
    Serial0.printf("Calibration bus error=%u\n", encoder.getLastError());
  } else if (c == 'r') {
    int pos = encoder.ReadPos(ID);
    if (encoder.getLastError()) { Serial0.println("Position read failed"); return; }
    int speed = encoder.ReadSpeed(ID);
    if (encoder.getLastError()) { Serial0.println("Speed read failed"); return; }
    Serial0.printf("Position=%d, angle=%.2f deg, speed=%d counts/s\n", pos, pos * 360.0 / 4096.0, speed);
  }
  delay(1);
}
```
