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
