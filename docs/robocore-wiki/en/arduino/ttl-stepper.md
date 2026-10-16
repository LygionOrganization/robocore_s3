# TTL stepper driver

**This is a standalone sketch. Uploading replaces the complete web firmware; use [firmware recovery](../firmware.md) to restore the console.**

## Hardware and wiring

TTL Stepper Driver (A), matching motor and rated supply. Default ID 1, 1000000 baud on IO4/IO5 TTL. Check phase/current configuration and mechanical clearance.

## Arduino IDE and board settings

1. Install Arduino IDE 2. In **File → Preferences → Additional Boards Manager URLs**, add `https://espressif.github.io/arduino-esp32/package_esp32_index.json`.
2. Open **Tools → Board → Boards Manager**, search `esp32`, and install **3.3.0** of the package by **Espressif Systems**.
3. Select **Tools → Board → esp32 → ESP32S3 Dev Module**. Set Flash Size=8MB, PSRAM=OPI PSRAM, USB Mode=USB-OTG (TinyUSB), USB CDC On Boot=Disabled. Feedback and interactive input use Serial0 (UART0) through Type-C UART.
4. Extract the [example archive](../../../../assets/files/robocore-s3/examples.zip) and open `arduino_example/TTL_Stepper/TTL_Stepper.ino`. Keep each sketch in its own project.

See [Arduino IDE installation](../../../../tutorials/install-arduino-ide.md) and the [Espressif installation guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

## Libraries for this sketch

This sketch needs the included **lygion_devs**, not a similarly named third-party package. Find Sketchbook location in File → Preferences, copy the entire `lib/lygion_devs` folder to `<Sketchbook>/libraries/lygion_devs`, with `library.properties` and `src` directly inside, then restart the IDE. Avoid an extra nested folder.

## Compile and upload

1. Click Verify and resolve compilation errors. If a `.h` file is missing, install the dependency listed above and compile again.
2. Type-C UART supports automatic download. For Type-C USB, hold BOOT, press/release RESET, release BOOT, select the new port and upload.
3. Press RESET to run. Keep the cable connected to Type-C UART and use the same port as uploading for Serial0 feedback; no cable change is needed. Open Serial Monitor at115200. Reset with the monitor open if you missed startup messages.

## Operation and expected results

Read with `r` first. Position mode: `0` → `e` → `p`, targeting absolute position 3200. Speed mode: `1` → `e` → `v`; `s` sets speed to zero, `x` disables output. Mode writes disable output; switching 0/1 needs no restart. `z` assigns zero to the current position without moving. `2` selects follower mode with leader ID 2; wire EDS as documented and keep primary IDs unique.

## Troubleshooting and next steps

CURRENT=150, speed 100 and acceleration 10 are register values, not amperes or ordinary steps/s. Phase changes require restart. Match each movement command to its operating mode.

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
TTLSDClass stepper;
constexpr uint8_t ID = 1;
// These are raw register values, not amperes or steps per second.
constexpr uint16_t CURRENT = 150;
void setMode(uint8_t mode) {
  stepper.unLockEprom(ID); // Also disables output.
  stepper.SetMode(ID, mode);
  if (mode == 2) stepper.writeByte(ID, 7, 2); // Leader primary ID = 2.
  stepper.LockEprom(ID);
  // Modes 0 and 1 can switch without restarting. Phase changes require restart.
  Serial0.println("Mode written. Use e to enable output when ready.");
}
void setup() {
  Serial0.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, 5, 4);
  stepper.pSerial = &Serial1;
  Serial0.println("0=position, 1=speed, 2=follower, e=enable, x=disable");
  Serial0.println("p=move to 3200, v=rotate, s=stop, z=set current position to 0, r=read");
}
void loop() {
  switch (Serial0.read()) {
    case '0': setMode(0); break;
    case '1': setMode(1); break;
    case '2': setMode(2); break; // Connect EDS according to the product guide.
    case 'e': stepper.EnableTorque(ID, 1); break;
    case 'x': stepper.EnableTorque(ID, 0); break;
    case 'p': stepper.WritePosEx(ID, 3200, 100, 10, CURRENT); break;
    case 'v': stepper.WriteSpe(ID, 100, 10, CURRENT); break;
    case 's': stepper.WriteSpe(ID, 0, 10, CURRENT); break;
    case 'z': stepper.CalibrationOfs(ID, 0); break;
    case 'r': {
      int position = stepper.ReadPos(ID);
      if (stepper.getLastError()) { Serial0.println("Position read failed"); break; }
      int speed = stepper.ReadSpeed(ID);
      if (stepper.getLastError()) { Serial0.println("Speed read failed"); break; }
      Serial0.printf("Position=%d, speed=%d\n", position, speed);
      break;
    }
  }
  delay(1);
}
```
