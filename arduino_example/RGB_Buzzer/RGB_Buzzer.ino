// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// Library Manager: install Adafruit NeoPixel 1.15.5 by Adafruit.
// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.
// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.
// END EXAMPLE SETUP
// Optional external WS2812 wiring only; RoboCore S3 has no onboard addressable RGB LEDs.
// For the supported expansion module use User_Interface_Module_A instead.
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel leds(2, 26, NEO_GRB + NEO_KHZ800);
void setup() {
  Serial0.begin(115200);
  leds.begin(); leds.setBrightness(20); leds.clear(); leds.show();
  pinMode(21, OUTPUT); digitalWrite(21, HIGH);
  Serial0.println("l=blue LEDs, x=LEDs off, b=short buzzer tone.");
}
void loop() {
  switch (Serial0.read()) {
    case 'l': leds.fill(leds.Color(0,0,255)); leds.show(); break;
    case 'x': leds.clear(); leds.show(); break;
    case 'b': tone(21, 1500, 100); break;
  }
  delay(1);
}
