// BEGIN EXAMPLE SETUP
// Board package: esp32 by Espressif Systems 3.3.0 (Boards Manager).
// Console: UART0 / Serial0 at 115200 baud through the Type-C UART port.
// Keep the upload cable connected to Type-C UART for feedback and commands.
// Recommended Arduino setting: USB CDC On Boot = Disabled.
// Serial0 explicitly selects UART0 even if USB CDC On Boot is enabled.
// Library Manager: install ArduinoJson 7.4.3 by Benoit Blanchon.
// Library Manager: install AsyncTCP 3.5.0 and ESPAsyncWebServer 3.12.1 by ESP32Async.
// Other ESP32 headers (Arduino, Wire, WiFi, USB, drivers, FreeRTOS) come with the board package.
// PlatformIO: put bundled libraries in lib/ and install external libraries via lib_deps.
// END EXAMPLE SETUP
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
AsyncWebServer server(80);
AsyncWebSocket socket("/ws");
int value = 0;
void event(AsyncWebSocket*, AsyncWebSocketClient* client, AwsEventType type,
           void* arg, uint8_t* data, size_t len) {
  if (type != WS_EVT_DATA) return;
  auto* frame = static_cast<AwsFrameInfo*>(arg);
  if (!frame->final || frame->index || frame->len != len || frame->opcode != WS_TEXT || len > 255) {
    client->text("{\"ok\":false,\"error\":\"one short text frame required\"}"); return;
  }
  JsonDocument in, out;
  if (deserializeJson(in, data, len) || !in["T"].is<int>()) {
    client->text("{\"ok\":false}"); return;
  }
  // Teaching protocol only; the complete firmware has a larger command table.
  int typeId = in["T"];
  bool ok = typeId == 2 || (typeId == 1 && in["value"].is<int>());
  if (ok && typeId == 1) value = in["value"];
  out["ok"] = ok; out["value"] = value;
  String reply; serializeJson(out, reply); client->text(reply);
}
void setup() {
  Serial0.begin(115200);
  WiFi.softAP("RoboCoreDemo", "12345678");
  socket.onEvent(event); server.addHandler(&socket); server.begin();
  Serial0.println("Connect ws://192.168.4.1/ws; send {\"T\":1,\"value\":42}");
}
void loop() { socket.cleanupClients(); delay(10); }
