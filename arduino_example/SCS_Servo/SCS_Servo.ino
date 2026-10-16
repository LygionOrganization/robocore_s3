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
SCSCL servo;
constexpr uint8_t ID = 1;
constexpr int CENTER = 511;
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
    servo.WritePos(ID, target, 1000, 100);
  } else if (command == 'x') {
    servo.EnableTorque(ID, 0);
  } else if (command == 'r') {
    Serial0.printf("Position=%d, speed=%d\n", servo.ReadPos(ID), servo.ReadSpeed(ID));
  }
}
