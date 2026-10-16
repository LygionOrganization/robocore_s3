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
#include <WebServer.h>
#include <ArduinoJson.h>
WebServer server(80);
int value = 0;
void setup() {
  Serial0.begin(115200);
  WiFi.softAP("RoboCoreDemo", "12345678");
  server.on("/api/cmd", HTTP_POST, []() {
    String body = server.arg("plain");
    JsonDocument in, out;
    if (body.length() > 255 || deserializeJson(in, body) || !in["T"].is<int>()) {
      server.send(400, "application/json", "{\"ok\":false}"); return;
    }
    // Teaching protocol: T=1 sets a RAM value, T=2 reads it. No motors move.
    int type = in["T"];
    bool ok = type == 2 || (type == 1 && in["value"].is<int>());
    if (ok && type == 1) value = in["value"];
    out["ok"] = ok; out["value"] = value;
    String response; serializeJson(out, response);
    server.send(ok ? 200 : 400, "application/json", response);
  });
  server.begin();
  Serial0.println("POST http://192.168.4.1/api/cmd with {\"T\":1,\"value\":42}");
}
void loop() { server.handleClient(); delay(1); }
