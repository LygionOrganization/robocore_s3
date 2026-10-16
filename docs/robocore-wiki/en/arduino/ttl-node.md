# TTL Node: PWM, RGB and S.BUS

**This is a standalone sketch. Uploading replaces the complete web firmware; use [firmware recovery](../firmware.md) to restore the console.**

## Hardware and wiring

TTL Node (A), ID 1 at 1000000 baud on the 5264-3P bus. Add a compatible receiver for S.BUS.

## Arduino IDE and board settings

1. Install Arduino IDE 2. In **File → Preferences → Additional Boards Manager URLs**, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. Open **Tools → Board → Boards Manager**, search `esp32`, and install **3.3.0** of the package by **Espressif Systems**.
3. Select **Tools → Board → esp32 → ESP32S3 Dev Module**. Set Flash Size=8MB, PSRAM=OPI PSRAM, USB Mode=USB-OTG (TinyUSB), USB CDC On Boot=Disabled. Feedback and interactive input use Serial0 (UART0) through Type-C UART.
4. Extract the [example archive](../../../../assets/files/robocore-s3/examples.zip) and open `arduino_example/TTL_Node/TTL_Node.ino`. Keep each sketch in its own project.

See [Arduino IDE installation](../../../../tutorials/install-arduino-ide.md) and the [Espressif installation guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

## Libraries for this sketch

This sketch needs the included **lygion_devs**, not a similarly named third-party package. Find Sketchbook location in File → Preferences, copy the entire `lib/lygion_devs` folder to `<Sketchbook>/libraries/lygion_devs`, with `library.properties` and `src` directly inside, then restart the IDE. Avoid an extra nested folder.

## Compile and upload

1. Click Verify and resolve compilation errors. If a `.h` file is missing, install the dependency listed above and compile again.
2. Type-C UART supports automatic download. For Type-C USB, hold BOOT, press/release RESET, release BOOT, select the new port and upload.
3. Press RESET to run. Keep the cable connected to Type-C UART and use the same port as uploading for Serial0 feedback; no cable change is needed. Open Serial Monitor at115200. Reset with the monitor open if you missed startup messages.

## Operation and expected results

`p` sets PWM channel 1 to 100/1000 (10%); `x` sets zero. `l` sets both indicators; `s` prints 16 channels and S.BUS status. `i` changes ID 1 to 2: connect only the target and ensure ID 2 is free. After success this session uses ID 2; also change variable id to 2 before restarting the sketch.

## Troubleshooting and next steps

A register reply does not guarantee radio reception: check status and changing channels. RGB uses SDK quantized values, not general 0–255 colors.

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
NodeClass node;
uint8_t id = 1;
void setup() {
  Serial0.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, 5, 4);
  node.pSerial = &Serial1;
  Serial0.println("p=10% PWM, x=PWM off, l=LED, s=S.BUS, i=change ID 1 to 2.");
}
void loop() {
  switch (Serial0.read()) {
    case 'p': node.pwmCtl(id, 1, 100); break;
    case 'x': node.pwmCtl(id, 1, 0); break;
    case 'l': node.ledAllCtrl(id, 2, 1, 0, 0); break;
    case 's':
      if (node.sbusFlush(id) == 33 && node.getLastError() == 0) {
        Serial0.printf("S.BUS status=%u\n", node.sbusStatus());
        for (int ch = 1; ch <= 16; ++ch) Serial0.printf("CH%d=%u\n", ch, node.sbusGetch(ch));
      } else Serial0.println("No valid S.BUS register reply.");
      break;
    case 'i':
      // Connect only the intended device. Do not reuse an occupied ID.
      if (id == 1 && node.Ping(2) != 2 && node.Ping(1) == 1) {
        node.unLockEprom(1);
        node.writeByte(1, LY_NODE_ID, 2);
        if (node.Ping(2) == 2) { node.LockEprom(2); id = 2; }
        else { node.LockEprom(1); Serial0.println("ID change not verified."); }
      }
      break;
  }
  delay(1);
}
```
