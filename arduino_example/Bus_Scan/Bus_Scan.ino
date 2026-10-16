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
