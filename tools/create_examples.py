"""Generate the small, self-contained Arduino teaching sketches."""
from pathlib import Path
from example_setup import prepare
root = Path(__file__).resolve().parents[1] / 'arduino_example'

def save(name, source):
    # Preserve maintained examples; this bootstrap tool only creates missing sketches.
    if (root / name / (name + '.ino')).exists():
        return
    folder = root / name
    folder.mkdir(parents=True, exist_ok=True)
    (folder / (name + '.ino')).write_text(prepare(name, source.strip() + '\n'), encoding='utf-8')

for family, cls, middle, write in [
    ('SCS','SCSCL',511,'servo.WritePos(ID, target, 1000, 100);'),
    ('STS','SMS_STS',2047,'servo.WritePosEx(ID, target, 100, 10);'),
    ('SMS','SMS_STS',2047,'servo.WritePosEx(ID, target, 100, 10);'),
    ('HLS','HLSCL',2047,'servo.WritePosEx(ID, target, 100, 10, 150);'),
]:
    save(f'{family}_Servo',f'''
#include <Arduino.h>
#include <SCServo.h>

// Supply the servo at its rated voltage. Bus power equals board input power.
// STS and SMS share the SDK class; verify your model's range before moving.
{cls} servo;
constexpr uint8_t ID = 1;
constexpr int CENTER = {middle};
void setup() {{
  Serial.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, 5, 4);
  servo.pSerial = &Serial1;
  Serial.println("Send m to move 50 counts, c to center, r to read, or x to disable.");
}}
void loop() {{
  if (!Serial.available()) {{ delay(1); return; }}
  char command = Serial.read();
  if (command == 'm' || command == 'c') {{
    int target = command == 'm' ? CENTER + 50 : CENTER;
    servo.EnableTorque(ID, 1);
    {write}
  }} else if (command == 'x') {{
    servo.EnableTorque(ID, 0);
  }} else if (command == 'r') {{
    Serial.printf("Position=%d, speed=%d\\n", servo.ReadPos(ID), servo.ReadSpeed(ID));
  }}
}}
''')

save('Bus_Scan',r'''
#include <Arduino.h>
#include <lygion_devs.h>
TTLSDClass bus;
void setup() {
  Serial.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, 5, 4);
  bus.pSerial = &Serial1;
  Serial.println("Send s to scan IDs 0..253 at 1 Mbps.");
}
void loop() {
  if (Serial.read() == 's') {
    for (int id = 0; id <= 253; ++id) {
      if (bus.Ping(id) == id) Serial.printf("ID %d\n", id);
      delay(1);
    }
    Serial.println("Scan finished. No replies do not prove a device is unpowered.");
  }
  delay(1);
}
''')

save('TTL_Node',r'''
#include <Arduino.h>
#include <lygion_devs.h>
NodeClass node;
uint8_t id = 1;
void setup() {
  Serial.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, 5, 4);
  node.pSerial = &Serial1;
  Serial.println("p=10% PWM, x=PWM off, l=LED, s=S.BUS, i=change ID 1 to 2.");
}
void loop() {
  switch (Serial.read()) {
    case 'p': node.pwmCtl(id, 1, 100); break;
    case 'x': node.pwmCtl(id, 1, 0); break;
    case 'l': node.ledAllCtrl(id, 2, 1, 0, 0); break;
    case 's':
      if (node.sbusFlush(id) == 33 && node.getLastError() == 0) {
        Serial.printf("S.BUS status=%u\n", node.sbusStatus());
        for (int ch = 1; ch <= 16; ++ch) Serial.printf("CH%d=%u\n", ch, node.sbusGetch(ch));
      } else Serial.println("No valid S.BUS register reply.");
      break;
    case 'i':
      // Connect only the intended device. Do not reuse an occupied ID.
      if (id == 1 && node.Ping(2) != 2 && node.Ping(1) == 1) {
        node.unLockEprom(1);
        node.writeByte(1, LY_NODE_ID, 2);
        if (node.Ping(2) == 2) { node.LockEprom(2); id = 2; }
        else { node.LockEprom(1); Serial.println("ID change not verified."); }
      }
      break;
  }
  delay(1);
}
''')

save('TTL_Stepper',r'''
#include <Arduino.h>
#include <lygion_devs.h>
TTLSDClass stepper;
constexpr uint8_t ID = 1;
// These are raw register values, not amperes or steps per second.
constexpr uint16_t CURRENT = 150;
void setMode(uint8_t mode) {
  stepper.unLockEprom(ID); // Also disables output.
  stepper.SetMode(ID, mode);
  if (mode == 2) stepper.writeByte(ID, 7, 2); // Leader primary ID = 2.
  stepper.LockEprom(ID);
  // Modes 0 and 1 can switch without restarting. Phase changes require restart.
  Serial.println("Mode written. Use e to enable output when ready.");
}
void setup() {
  Serial.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, 5, 4);
  stepper.pSerial = &Serial1;
  Serial.println("0=position, 1=speed, 2=follower, e=enable, x=disable");
  Serial.println("p=move to 3200, v=rotate, s=stop, z=set current position to 0, r=read");
}
void loop() {
  switch (Serial.read()) {
    case '0': setMode(0); break;
    case '1': setMode(1); break;
    case '2': setMode(2); break; // Connect EDS according to the product guide.
    case 'e': stepper.EnableTorque(ID, 1); break;
    case 'x': stepper.EnableTorque(ID, 0); break;
    case 'p': stepper.WritePosEx(ID, 3200, 100, 10, CURRENT); break;
    case 'v': stepper.WriteSpe(ID, 100, 10, CURRENT); break;
    case 's': stepper.WriteSpe(ID, 0, 10, CURRENT); break;
    case 'z': stepper.CalibrationOfs(ID, 0); break;
    case 'r': {
      int position = stepper.ReadPos(ID);
      if (stepper.getLastError()) { Serial.println("Position read failed"); break; }
      int speed = stepper.ReadSpeed(ID);
      if (stepper.getLastError()) { Serial.println("Speed read failed"); break; }
      Serial.printf("Position=%d, speed=%d\n", position, speed);
      break;
    }
  }
  delay(1);
}
''')

save('TTL_Encoder',r'''
#include <Arduino.h>
#include <lygion_devs.h>
// E02 uses the same position/speed register protocol as TTLSDClass.
TTLSDClass encoder;
constexpr uint8_t ID = 1;
void setup() {
  Serial.begin(115200);
  Serial1.begin(1000000, SERIAL_8N1, 5, 4);
  encoder.pSerial = &Serial1;
  Serial.println("r=read, z=save current position as 0 (persistent calibration).");
}
void loop() {
  char c = Serial.read();
  if (c == 'z') {
    encoder.CalibrationOfs(ID, 0);
    Serial.printf("Calibration bus error=%u\n", encoder.getLastError());
  } else if (c == 'r') {
    int pos = encoder.ReadPos(ID);
    if (encoder.getLastError()) { Serial.println("Position read failed"); return; }
    int speed = encoder.ReadSpeed(ID);
    if (encoder.getLastError()) { Serial.println("Speed read failed"); return; }
    Serial.printf("Position=%d, angle=%.2f deg, speed=%d counts/s\n", pos, pos * 360.0 / 4096.0, speed);
  }
  delay(1);
}
''')

save('OLED',r'''
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 screen(128, 32, &Wire, -1);
bool ready = false;
void setup() {
  Serial.begin(115200);
  Wire.begin(6, 7);
  Wire.setTimeOut(20);
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() != 0) { Serial.println("OLED absent; continuing."); return; }
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
''')

save('QMI8658A',r'''
#include <Arduino.h>
#include <Wire.h>
#include <OptionalQMI8658.h>
OptionalQMI8658 imu;
void setup() {
  Serial.begin(115200);
  Wire.begin(6, 7);
  Wire.setClock(400000);
  Wire.setTimeOut(20);
  if (!imu.begin(Wire)) Serial.println("QMI8658A absent; normal on RD Lite.");
}
void loop() {
  OptionalQMI8658::Sample s;
  if (imu.sample(s)) {
    Serial.printf("a[g]=%.3f,%.3f,%.3f g[dps]=%.3f,%.3f,%.3f T[C]=%.2f\n",
                  s.ax,s.ay,s.az,s.gx,s.gy,s.gz,s.temperature);
  }
  delay(100);
}
''')

save('CAN_Communication',r'''
#include <Arduino.h>
#include <driver/twai.h>
bool ready = false;
void setup() {
  Serial.begin(115200);
  twai_general_config_t general = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_14, GPIO_NUM_13, TWAI_MODE_NORMAL);
  twai_timing_config_t timing = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t filter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  ready = twai_driver_install(&general, &timing, &filter) == ESP_OK;
  if (ready) ready = twai_start() == ESP_OK;
  Serial.println("Connect another 500 kbps CAN node, common ground and proper termination.");
  Serial.println("Send t to transmit ID 0x100 with two bytes.");
}
void loop() {
  if (!ready) { delay(100); return; }
  if (Serial.read() == 't') {
    twai_message_t tx = {};
    tx.identifier = 0x100; tx.data_length_code = 2; tx.data[0] = 0x12; tx.data[1] = 0x34;
    Serial.printf("Transmit result=%d\n", twai_transmit(&tx, pdMS_TO_TICKS(100)));
  }
  twai_message_t rx = {};
  if (twai_receive(&rx, 0) == ESP_OK) {
    Serial.printf("CAN ID=%lx DLC=%u", rx.identifier, rx.data_length_code);
    for (int i = 0; i < rx.data_length_code && i < 8; ++i) Serial.printf(" %02x", rx.data[i]);
    Serial.println();
  }
  delay(1);
}
''')

save('HTTP_JSON',r'''
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
WebServer server(80);
int value = 0;
void setup() {
  Serial.begin(115200);
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
  Serial.println("POST http://192.168.4.1/api/cmd with {\"T\":1,\"value\":42}");
}
void loop() { server.handleClient(); delay(1); }
''')

save('WebSocket_JSON',r'''
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
  Serial.begin(115200);
  WiFi.softAP("RoboCoreDemo", "12345678");
  socket.onEvent(event); server.addHandler(&socket); server.begin();
  Serial.println("Connect ws://192.168.4.1/ws; send {\"T\":1,\"value\":42}");
}
void loop() { socket.cleanupClients(); delay(10); }
''')

save('USB_CDC_JSON',r'''
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
''')

save('ESP_NOW_JSON',r'''
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
  Serial.begin(115200);
  pending = xQueueCreate(1, 251); configASSERT(pending);
  WiFi.mode(WIFI_STA); esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  Serial.println(WiFi.macAddress());
  if (esp_now_init() != ESP_OK) return;
  esp_now_register_recv_cb(receive);
  esp_now_peer_info_t peer = {}; memcpy(peer.peer_addr, peerMac, 6); peer.channel = 1;
  ready = esp_now_add_peer(&peer) == ESP_OK;
  Serial.println("Send s to transmit a JSON message to the configured peer.");
}
void loop() {
  char message[251];
  if (xQueueReceive(pending, message, 0) == pdPASS) {
    JsonDocument doc;
    if (!deserializeJson(doc, message)) { serializeJson(doc, Serial); Serial.println(); }
  }
  if (ready && Serial.read() == 's') {
    // Factory firmware accepts JSON in a zero-padded 250-byte message frame.
    char payload[250] = "{\"T\":603}";
    Serial.printf("Send result=%d\n", esp_now_send(peerMac, reinterpret_cast<uint8_t*>(payload), sizeof(payload)));
  }
  delay(1);
}
''')

save('RGB_Buzzer',r'''
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel leds(2, 26, NEO_GRB + NEO_KHZ800);
void setup() {
  Serial.begin(115200);
  leds.begin(); leds.setBrightness(20); leds.clear(); leds.show();
  pinMode(21, OUTPUT); digitalWrite(21, HIGH);
  Serial.println("l=blue LEDs, x=LEDs off, b=short buzzer tone.");
}
void loop() {
  switch (Serial.read()) {
    case 'l': leds.fill(leds.Color(0,0,255)); leds.show(); break;
    case 'x': leds.clear(); leds.show(); break;
    case 'b': tone(21, 1500, 100); break;
  }
  delay(1);
}
''')

print('Generated', len(list(root.glob('*/*.ino'))), 'sketches')
