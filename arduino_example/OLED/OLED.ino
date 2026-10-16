// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// Library Manager: install Adafruit SSD1306 2.5.17, Adafruit GFX Library 1.12.6, and Adafruit BusIO.
// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.
// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 screen(128, 32, &Wire, -1);
bool ready = false;
void setup() {
  Serial0.begin(115200);
  Wire.begin(6, 7);
  Wire.setTimeOut(20);
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() != 0) { Serial0.println("OLED absent; continuing."); return; }
  ready = screen.begin(SSD1306_SWITCHCAPVCC, 0x3C, true, false);
  if (ready) { screen.setTextColor(SSD1306_WHITE); screen.setTextSize(1); }
}
void loop() {
  if (ready) {
    screen.clearDisplay(); screen.setCursor(0, 0);
    screen.println("RoboCore S3"); screen.printf("Uptime: %lu s", millis()/1000);
    screen.display();
  }
  delay(500);
}
