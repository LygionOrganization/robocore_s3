# SMS bus servo

**This is a standalone sketch. Uploading replaces the complete web firmware; use [firmware recovery](../firmware.md) to restore the console.**

## Hardware and wiring

One SMS servo with ID 1 and bus baud 1000000. With power off, use 5264-3P TTL or XH2.54-4P RS485 to match the servo interface, controller RX5/TX4. XT30 voltage must match its rating.

## Arduino IDE and board settings

1. Install Arduino IDE 2. In **File → Preferences → Additional Boards Manager URLs**, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. Open **Tools → Board → Boards Manager**, search `esp32`, and install **3.3.0** of the package by **Espressif Systems**.
3. Select **Tools → Board → esp32 → ESP32S3 Dev Module**. Set Flash Size=8MB, PSRAM=OPI PSRAM, USB Mode=USB-OTG (TinyUSB), USB CDC On Boot=Disabled. Feedback and interactive input use Serial0 (UART0) through Type-C UART.
4. Extract the [example archive](../../../../assets/files/robocore-s3/examples.zip) and open `arduino_example/SMS_Servo/SMS_Servo.ino`. Keep each sketch in its own project.

See [Arduino IDE installation](../../../../tutorials/install-arduino-ide.md) and the [Espressif installation guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

## Libraries for this sketch

Open **Tools → Manage Libraries**, search and install each package below, choose the listed version and verify the author. Choose Install all when dependency installation is offered.

- FTServo 2.0.0 (ftservo / FEETECH)

[Arduino library installation guide](https://support.arduino.cc/hc/en-us/articles/5145457742236-Install-libraries-in-the-Arduino-IDE).

## Compile and upload

1. Click Verify and resolve compilation errors. If a `.h` file is missing, install the dependency listed above and compile again.
2. Type-C UART supports automatic download. For Type-C USB, hold BOOT, press/release RESET, release BOOT, select the new port and upload.
3. Press RESET to run. Keep the cable connected to Type-C UART and use the same port as uploading for Serial0 feedback; no cable change is needed. Open Serial Monitor at115200. Reset with the monitor open if you missed startup messages.

## Operation and expected results

Send `r` to read position and speed. Check `ID` and `CENTER=2047` against mechanical travel. `m` enables torque and moves to **absolute CENTER + 50**, not 50 counts from the current position. `c` moves to CENTER; `x` disables torque. Support the load before disabling torque.

## Troubleshooting and next steps

Negative readings or no reply: check ID, baud, motor power and protocol family. Do not reuse another model’s travel limits.

See [hardware](../hardware.md) for interfaces and voltage, and the [JSON reference](../json-api.md) for full-firmware commands.

## Complete code

```cpp
// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// Library Manager: install FTServo 2.0.0 by ftservo / FEETECH (provides SCServo.h).
// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.
// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <SCServo.h>

// Supply the servo at its rated voltage. Bus power equals board input power.
// STS and SMS share the SDK class; verify your model's range before moving.
SMS_STS servo;
constexpr uint8_t ID = 1;
constexpr int CENTER = 2047;
void setup() {
  Serial0.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, 5, 4);
  servo.pSerial = &Serial1;
  Serial0.println("Send m to move 50 counts, c to center, r to read, or x to disable.");
}
void loop() {
  if (!Serial0.available()) { delay(1); return; }
  char command = Serial0.read();
  if (command == 'm' || command == 'c') {
    int target = command == 'm' ? CENTER + 50 : CENTER;
    servo.EnableTorque(ID, 1);
    servo.WritePosEx(ID, target, 100, 10);
  } else if (command == 'x') {
    servo.EnableTorque(ID, 0);
  } else if (command == 'r') {
    Serial0.printf("Position=%d, speed=%d\n", servo.ReadPos(ID), servo.ReadSpeed(ID));
  }
}
```
