// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// Library Manager: install ArduinoJson 7.4.3 by Benoit Blanchon.
// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.
// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>
#include <freertos/queue.h>
QueueHandle_t pending;
// Both peers must use the same channel. Replace with the other board's STA MAC.
uint8_t peerMac[6] = {0x24,0x6F,0x28,0x00,0x00,0x01};
bool ready = false;
void receive(const esp_now_recv_info_t*, const uint8_t* data, int len) {
  if (len <= 0 || len > 250) return;
  char message[251] = {};
  memcpy(message, data, len);
  xQueueOverwrite(pending, message); // No parsing, logging or bus I/O in callback.
}
void setup() {
  Serial0.begin(115200);
  pending = xQueueCreate(1, 251); configASSERT(pending);
  WiFi.mode(WIFI_STA); esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  Serial0.println(WiFi.macAddress());
  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb(receive);
  esp_now_peer_info_t peer = {}; memcpy(peer.peer_addr, peerMac, 6); peer.channel = 1;
  ready = esp_now_add_peer(&peer) == ESP_OK;
  Serial0.println("Send s to transmit a JSON message to the configured peer.");
}
void loop() {
  char message[251];
  if (xQueueReceive(pending, message, 0) == pdPASS) {
    JsonDocument doc;
    if (!deserializeJson(doc, message)) { serializeJson(doc, Serial0); Serial0.println(); }
  }
  if (ready && Serial0.read() == 's') {
    // Factory firmware accepts JSON in a zero-padded 250-byte message frame.
    char payload[250] = "{\"T\":603}";
    Serial0.printf("Send result=%d\n", esp_now_send(peerMac, reinterpret_cast<uint8_t*>(payload), sizeof(payload)));
  }
  delay(1);
}
