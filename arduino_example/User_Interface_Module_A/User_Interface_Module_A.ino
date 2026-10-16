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

// Connect User Interface Module (A) to the PH1.25-10P expansion port.
// OLED and onboard QMI8658A share SDA=6 / SCL=7; initialize Wire only once.
constexpr uint8_t buttonPins[] = {10, 11, 12, 9, 2};
const char *buttonNames[] = {"UP", "DOWN", "LEFT", "RIGHT", "OK"};
constexpr uint8_t buzzerPin = 21;
bool rawState[5], stableState[5];
uint32_t changedAt[5] = {};
uint32_t beepAt = 0;
bool beeping = false, screenReady = false;
Adafruit_SSD1306 screen(128, 32, &Wire, -1);

void showButton(const char *name) {
  Serial0.printf("Button: %s\n", name);
  if (!screenReady) return;
  screen.clearDisplay();
  screen.setCursor(0, 0);
  screen.println("RoboCore S3 / UI (A)");
  screen.println("Press a button");
  screen.print("Last: "); screen.println(name);
  screen.display();
}

void setup() {
  Serial0.begin(115200);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, HIGH);
  for (size_t i = 0; i < 5; ++i) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    rawState[i] = stableState[i] = digitalRead(buttonPins[i]);
  }
  Wire.begin(6, 7);
  Wire.setTimeOut(20);
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() == 0)
    screenReady = screen.begin(SSD1306_SWITCHCAPVCC, 0x3C, true, false);
  if (screenReady) {
    screen.setTextColor(SSD1306_WHITE);
    screen.setTextSize(1);
  } else Serial0.println("OLED unavailable; buttons and buzzer remain active.");
  showButton("None");
}

void loop() {
  const uint32_t now = millis();
  for (size_t i = 0; i < 5; ++i) {
    const bool state = digitalRead(buttonPins[i]);
    if (state != rawState[i]) { rawState[i] = state; changedAt[i] = now; }
    // Debounce both edges; emit one event per press, without blocking other keys.
    if (state != stableState[i] && uint32_t(now - changedAt[i]) >= 30) {
      stableState[i] = state;
      if (state == LOW) {
        showButton(buttonNames[i]);
        tone(buzzerPin, 1500);
        beepAt = now; beeping = true;
      }
    }
  }
  if (beeping && uint32_t(now - beepAt) >= 80) {
    noTone(buzzerPin); digitalWrite(buzzerPin, HIGH); beeping = false;
  }
  delay(1);
}
