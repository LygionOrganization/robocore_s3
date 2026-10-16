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
