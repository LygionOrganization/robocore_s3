# Bus ID scan

**This is a standalone sketch. Uploading replaces the complete web firmware; use [firmware recovery](../firmware.md) to restore the console.**

## Hardware and wiring

One compatible TTL/RS485 device on the IO4 TX / IO5 RX bus, with rated external power.

## Arduino IDE and board settings

1. Install Arduino IDE 2. In **File → Preferences → Additional Boards Manager URLs**, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. Open **Tools → Board → Boards Manager**, search `esp32`, and install **3.3.0** of the package by **Espressif Systems**.
3. Select **Tools → Board → esp32 → ESP32S3 Dev Module**. Set Flash Size=8MB, PSRAM=OPI PSRAM, USB Mode=USB-OTG (TinyUSB), USB CDC On Boot=Disabled. Feedback and interactive input use Serial0 (UART0) through Type-C UART.
4. Extract the [example archive](../../../../assets/files/robocore-s3/examples.zip) and open `arduino_example/Bus_Scan/Bus_Scan.ino`. Keep each sketch in its own project.

See [Arduino IDE installation](../../../../tutorials/install-arduino-ide.md) and the [Espressif installation guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

## Libraries for this sketch

This sketch needs the included **lygion_devs**, not a similarly named third-party package. Find Sketchbook location in File → Preferences, copy the entire `lib/lygion_devs` folder to `<Sketchbook>/libraries/lygion_devs`, with `library.properties` and `src` directly inside, then restart the IDE. Avoid an extra nested folder.

## Compile and upload

1. Click Verify and resolve compilation errors. If a `.h` file is missing, install the dependency listed above and compile again.
2. Type-C UART supports automatic download. For Type-C USB, hold BOOT, press/release RESET, release BOOT, select the new port and upload.
3. Press RESET to run. Keep the cable connected to Type-C UART and use the same port as uploading for Serial0 feedback; no cable change is needed. Open Serial Monitor at115200. Reset with the monitor open if you missed startup messages.

## Operation and expected results

Send `s` to ping IDs 0–253. Each response prints `ID n`, followed by `Scan finished`. This finds responding IDs, not product types. Default baud is 1000000; edit the first Serial1.begin argument to match other devices.

## Troubleshooting and next steps

If empty, check power, ground, wiring and baud. Duplicate IDs can collide; connect devices individually to change IDs.

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
TTLSDClass bus;
void setup() {
  Serial0.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, 5, 4);
  bus.pSerial = &Serial1;
  Serial0.println("Send s to scan IDs 0..253 at 1 Mbps.");
}
void loop() {
  if (Serial0.read() == 's') {
    for (int id = 0; id <= 253; ++id) {
      if (bus.Ping(id) == id) Serial0.printf("ID %d\n", id);
      delay(1);
    }
    Serial0.println("Scan finished. No replies do not prove a device is unpowered.");
  }
  delay(1);
}
```
