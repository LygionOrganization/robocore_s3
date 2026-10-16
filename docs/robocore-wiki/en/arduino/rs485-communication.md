# External RS485 communication

**This is a standalone sketch. Uploading replaces the complete web firmware; use [firmware recovery](../firmware.md) to restore the console.**

## Hardware and wiring

Set selector to 485; connect GH1.25-5P A/B/GND to an RS485 peer at 115200, 8N1. Uses UART2 RX13/TX14. Do not parallel power outputs when the peer is separately powered.

## Arduino IDE and board settings

1. Install Arduino IDE 2. In **File → Preferences → Additional Boards Manager URLs**, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. Open **Tools → Board → Boards Manager**, search `esp32`, and install **3.3.0** of the package by **Espressif Systems**.
3. Select **Tools → Board → esp32 → ESP32S3 Dev Module**. Set Flash Size=8MB, PSRAM=OPI PSRAM, USB Mode=USB-OTG (TinyUSB), USB CDC On Boot=Disabled. Feedback and interactive input use Serial0 (UART0) through Type-C UART.
4. Extract the [example archive](../../../../assets/files/robocore-s3/examples.zip) and open `arduino_example/RS485_Communication/RS485_Communication.ino`. Keep each sketch in its own project.

See [Arduino IDE installation](../../../../tutorials/install-arduino-ide.md) and the [Espressif installation guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

## Libraries for this sketch

No additional library is needed. `HardwareSerial` comes with the esp32 board package. If a header is missing, verify the selected ESP32S3 board and installed core instead of adding an unrelated library.

## Compile and upload

1. Click Verify and resolve compilation errors. If a `.h` file is missing, install the dependency listed above and compile again.
2. Type-C UART supports automatic download. For Type-C USB, hold BOOT, press/release RESET, release BOOT, select the new port and upload.
3. Press RESET to run. Keep the cable connected to Type-C UART and use the same port as uploading for Serial0 feedback; no cable change is needed. Open Serial Monitor at115200. Reset with the monitor open if you missed startup messages.

## Operation and expected results

Send `t` to transmit `RoboCore S3 RS485` plus newline; verify that text at the peer. Received bytes print in hexadecimal. Take turns transmitting to avoid half-duplex collisions. Onboard direction control requires no DE pin.

## Troubleshooting and next steps

This is a byte transport demo, not the factory JSON gateway or the IO4/IO5 servo RS485 bus. For corrupt data check baud, 8N1, A/B and ground.

See [hardware](../hardware.md) for interfaces and voltage, and the [JSON reference](../json-api.md) for full-firmware commands.

## Complete code

```cpp
// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// No external libraries required; all headers are included with the ESP32 board package.
// END EXAMPLE SETUP
#include <Arduino.h>

// Set the physical selector to 485 before running this sketch.
// GH1.25-5P: external RS485, RX=13 / TX=14, with onboard automatic direction.
// Match A/B/GND and 115200 8N1 on the peer; do not connect 5V to a powered peer.
// Serial1 RX=5 / TX=4 belongs to the separate servo/TTL bus and is not used here.
HardwareSerial externalBus(2);

void setup() {
  Serial0.begin(115200);
  externalBus.begin(115200, SERIAL_8N1, 13, 14);
  Serial0.println("Selector: 485. External RS485: RX13/TX14, 115200 8N1.");
  Serial0.println("Send t to transmit a line. Received bytes are printed as hex.");
}

void loop() {
  // Transmit only on request. Coordinate half-duplex access with the other node.
  if (Serial0.read() == 't') {
    externalBus.print("RoboCore S3 RS485\n");
    externalBus.flush();
    Serial0.println("Local transmission complete; peer receipt is not confirmed.");
  }
  for (unsigned n = 0; n < 64 && externalBus.available(); ++n)
    Serial0.printf("%02X ", externalBus.read());
  delay(1);
}
```
