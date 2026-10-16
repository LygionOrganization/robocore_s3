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
