// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Connect Type-C USB; select USB-OTG (TinyUSB), USB CDC On Boot = Enabled.
// Library Manager: install ArduinoJson 7.4.3 by Benoit Blanchon.
// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.
// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <ArduinoJson.h>
#include <USB.h>
#include <USBCDC.h>
#include <freertos/queue.h>
// Select USB-OTG (TinyUSB) and USB CDC On Boot = Enabled in Arduino Tools.
QueueHandle_t pending;
void event(void*, esp_event_base_t base, int32_t type, void*) {
  static char line[256]; static size_t used = 0; static bool drop = false;
  if (base != ARDUINO_USB_CDC_EVENTS) return;
  if (type == ARDUINO_USB_CDC_DISCONNECTED_EVENT) { used = 0; drop = false; return; }
  if (type == ARDUINO_USB_CDC_RX_OVERFLOW_EVENT) { used = 0; drop = true; }
  else if (type != ARDUINO_USB_CDC_RX_EVENT) return;
  int remaining = USBSerial.available();
  while (remaining-- > 0) {
    int c = USBSerial.read(); if (c < 0) break;
    if (c == '\n') {
      if (!drop && used) { line[used] = 0; xQueueOverwrite(pending, line); }
      used = 0; drop = false;
    } else if (c == 0) { used = 0; drop = true; }
    else if (c != '\r' && !drop) {
      if (used < sizeof(line) - 1) line[used++] = c;
      else { used = 0; drop = true; }
    }
  }
}
void setup() {
  pending = xQueueCreate(1, 256); configASSERT(pending);
  USBSerial.setRxBufferSize(2048); USBSerial.onEvent(event);
  USBSerial.begin(); USB.begin();
}
void loop() {
  static int value = 0;
  char line[256];
  if (xQueueReceive(pending, line, 0) == pdPASS) {
    JsonDocument in, out;
    bool valid = !deserializeJson(in, line) && in["T"].is<int>();
    int type = valid ? in["T"].as<int>() : -1;
    bool ok = valid && (type == 2 || (type == 1 && in["value"].is<int>()));
    if (ok && type == 1) value = in["value"];
    out["ok"] = ok; out["value"] = value;
    // Drop a complete response if the host is not draining USB.
    // This single-writer example never writes beyond the free TX FIFO.
    char reply[96];
    size_t n = serializeJson(out, reply, sizeof(reply) - 1);
    reply[n++] = '\n';
    if (USBSerial.availableForWrite() >= static_cast<int>(n))
      USBSerial.write(reinterpret_cast<const uint8_t*>(reply), n);
  }
  delay(1);
}
