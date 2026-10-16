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
