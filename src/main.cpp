#include "UsbOutput.h"
// for PlatformIO. This is the main file of the project.
// This file is the entry point of the program.
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include "sbus.h"
#include "USB.h"
#include "USBCDC.h"
#include <atomic>
#include "CommandLineBuffer.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "Config.h"
#include "jointsCtrl.h"
#include "FilesCtrl.h"
#include "ScreenCtrl.h"
#include "Wireless.h"
#include "BusDevices.h"
#include "MissionStorage.h"
#include "SyncCommandValidation.h"
#ifdef DEV_TYPE_LINKARM_LT
#include "Arm_page.h"
#else
#include "Web_page.h"
#endif

QueueHandle_t cmdQueue;
JsonDocument jsonFeedback;
/*
Declare USBSerial as an instance of USBCDC
Serial is used for USB CDC communication
Serial0 is used for UART communication
*/
// USBCDC USBSerial;
String outputString;
JointsCtrl jointsCtrl;
FilesCtrl filesCtrl;
ScreenCtrl screenCtrl;
Wireless wireless;
BusDevices busDevices;
SemaphoreHandle_t resultMutex;
String latestResult = "{}";

#define CMD_MAX_LEN 256
#define CMD_QUEUE_LEN 1 // xQueueOverwrite requires exactly one pending item.

std::atomic<bool> breakloop{false};
std::atomic<bool> missionRunning{false};
std::atomic_flag missionGate = ATOMIC_FLAG_INIT;
MissionStorage missionStorage;
struct MissionGuard {
  bool acquired = !missionGate.test_and_set();
  ~MissionGuard() { if (acquired) missionGate.clear(); }
};
struct MissionRunState {
  MissionRunState() { missionRunning.store(true); }
  ~MissionRunState() { missionRunning.store(false); }
};
unsigned long tuneStartTime;
int* jointFeedback;

int espNowPacket[4] = {0, 0, 0, 0};
int jointsZeroPos[JOINTS_NUM] = {511, 511, 511, 511};
int jointsCurrentPos[JOINTS_NUM];
int jointsRelativesSteps[JOINTS_NUM];
double jointFBRads[JOINTS_NUM];
int jointFBTorques[JOINTS_NUM];
double jointsGoalBuffer[JOINTS_NUM];
double xyzgIKFeedback[JOINTS_NUM + 1];
double rbzgIKFeedback[JOINTS_NUM + 1];
double xyzgIK[JOINTS_NUM + 1];
double rbzgIK[JOINTS_NUM + 1];

void jsonCmdReceiveHandler(const JsonDocument& jsonCmdInput);
void addJsonCmd2Queue(const char* jsonString);
void usbCdcEvent(void*, esp_event_base_t, int32_t, void*);
void relativeStepsCtrl(int j0, int j1, int j2, int j3);
void runMission(String missionName, int intervalTime, int loopTimes);

unsigned long startTime;
unsigned long endTime;

#ifdef UART0_AS_SBUS
bfs::SbusRx sbus(&Serial0, 44, 43, true);
bfs::SbusData sbusData;
#endif

void msg(String msgStr, bool newLine = true) {
#ifndef UART0_AS_SBUS
  if (uartMsgFlag) {
    if (newLine) {
      Serial0.println(msgStr);
    } else {
      Serial0.print(msgStr);
    }
  }
#endif
  if (usbMsgFlag) {
    if (newLine) {
      usbOutput.println(msgStr);
    } else {
      usbOutput.print(msgStr);
    }
  }
}

void getMsgStatus() {
  JsonDocument jsonFeedback;
  String outputString;
  jsonFeedback.clear();
  jsonFeedback["T"] = CMD_SET_MSG_OUTPUT;
  jsonFeedback["echo"] = echoMsgFlag;
  jsonFeedback["uart"] = uartMsgFlag;
  jsonFeedback["usb"]  = usbMsgFlag;
  serializeJson(jsonFeedback, outputString);
  msg(outputString);
}

void buttonBuzzer() {
  for (int f = 1200; f <= 2000; f += 200) {
    tone(BUZZER_PIN, f);
    delay(5);
  }
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, HIGH);
}

#include "ButtonUI.h"

// websocket & http
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

#include "Can.h"

void handleWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                   AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    if (ws.count() > 4) { client->close(); return; }
    JsonDocument jsonFeedback;
    String outputString;
    // feedback after connection
    jsonFeedback.clear();
    jsonFeedback["type"] = "hello";
    jsonFeedback["id"]   = client->id();
    jsonFeedback["msg"]  = "connected";
    serializeJson(jsonFeedback, outputString);
    client->text(outputString);
  }
  else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      if (len == 0 || len >= CMD_MAX_LEN || memchr(data, '\0', len)) return;
      char cmdBuffer[CMD_MAX_LEN] = {};
      memcpy(cmdBuffer, data, len);
      addJsonCmd2Queue(cmdBuffer);
    }
  }
}

// Only the latest executed result is retained, just like the command mailbox.
void publishResult(const JsonDocument& result) {
  String payload;
  serializeJson(result, payload);
  xSemaphoreTake(resultMutex, portMAX_DELAY);
  latestResult = payload;
  xSemaphoreGive(resultMutex);
  if (payload.length() <= 4096 && ws.availableForWriteAll()) ws.textAll(payload);
  msg(payload);
}

// auto push telemetry
unsigned int baudrate;
String sta_ip;
String ap_ip;
String mac_addr;
uint32_t lastTick = 0;
void pushTelemetry() {
  if (millis() - lastTick >= FB_INTERVAL_MS) {
    lastTick = millis();
    if (ws.count() > 0) {
      jsonFeedback.clear();
      jsonFeedback["T"]   = 50;
      jsonFeedback["baud"]   = baudrate;
      jsonFeedback["sta"]   = wireless.getSTAIP();
      jsonFeedback["ap"]   = ap_ip;
      jsonFeedback["mac"]   = mac_addr;

      if (WiFi.status() == WL_CONNECTED) {
        jsonFeedback["ssid"] = WiFi.SSID();
      } else {
        jsonFeedback["ssid"] = "Disconnected";
      }
      jsonFeedback["uptime"] = (uint32_t)(millis()/1000);
      serializeJson(jsonFeedback, outputString);
      if (outputString.length() <= 4096 && ws.availableForWriteAll()) ws.textAll(outputString);
    }
  }
}

void setupHttpRoutes() {
#ifndef DEV_TYPE_LINKARM_LT
  server.on("/legacy", HTTP_GET, [](AsyncWebServerRequest *req){
    req->redirect("/#json");
  });
#endif
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req){
    req->send(200, "text/html; charset=utf-8", INDEX_HTML);
  });

  // CORS
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");

  // device info：GET /api/info
  server.on("/api/info", HTTP_GET, [](AsyncWebServerRequest *req){
    JsonDocument jsonFeedback;
    String outputString;
    jsonFeedback.clear();
    
    jsonFeedback["chip"]   = "ESP32-S3";
    jsonFeedback["uptime_ms"] = millis();
    jsonFeedback["free_heap"] = ESP.getFreeHeap();
    jsonFeedback["min_free_heap"] = ESP.getMinFreeHeap();
    jsonFeedback["reset_reason"] = int(esp_reset_reason());
    jsonFeedback["sdk"]    = ESP.getSdkVersion();
    jsonFeedback["fw_ver"] = "robocore-1.3.8";
    jsonFeedback["product"] = "RoboCore S3 / RD Lite";
    jsonFeedback["imu_present"] = busDevices.imuPresent();
    jsonFeedback["mission_running"] = missionRunning.load();
    jsonFeedback["sta_state"] = wireless.getSTAState();
    jsonFeedback["sta_ip"] = wireless.getSTAIP();
    jsonFeedback["ap_ip"] = wireless.getAPIP();
    jsonFeedback["sta_ssid"] = WiFi.SSID();
    jsonFeedback["baud"] = jointsCtrl.baudrate;
    uint8_t m[6]; WiFi.macAddress(m);
    jsonFeedback["mac"]    = wireless.macToString(m);
    serializeJson(jsonFeedback, outputString);
    req->send(200, "application/json", outputString);
  });

  // Read-only telemetry: no command queue, serial output or WS broadcast.
  server.on("/api/imu", HTTP_GET, [](AsyncWebServerRequest *req){
    JsonDocument command, result;
    command["T"] = 740;
    busDevices.handle(command, result);
    String payload;
    serializeJson(result, payload);
    auto* response = req->beginResponse(200, "application/json", payload);
    response->addHeader("Cache-Control", "no-store");
    req->send(response);
  });
  server.on("/api/result", HTTP_GET, [](AsyncWebServerRequest *req){
    xSemaphoreTake(resultMutex, portMAX_DELAY);
    String result = latestResult;
    xSemaphoreGive(resultMutex);
    req->send(200, "application/json", result);
  });

  // cmd：POST /api/cmd  ->
  server.on("/api/cmd", HTTP_POST, [](AsyncWebServerRequest *req){},
    NULL,
    [](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total){
      // Fragmented or oversized bodies are not commands; never copy beyond the buffer.
      if (index != 0) return;
      if (len != total || len == 0 || len >= CMD_MAX_LEN || memchr(data, '\0', len)) {
        req->send(413, "application/json", "{\"error\":\"command too large or fragmented\"}");
        return;
      }
      char cmdBuffer[CMD_MAX_LEN] = {};
      memcpy(cmdBuffer, data, len);
      JsonDocument command;
      if (deserializeJson(command, cmdBuffer) || !command["T"].is<int>()) {
        req->send(400, "application/json", "{\"accepted\":false,\"error\":\"integer T required\"}");
        return;
      }
      addJsonCmd2Queue(cmdBuffer);
      req->send(202, "application/json", "{\"accepted\":true,\"policy\":\"latest-wins\"}");
    }
  );

  // WebSocket
  ws.onEvent(handleWsEvent);
  server.addHandler(&ws);
}

void runMission(String missionName, int intervalTime, int loopTimes);
void runMissionTask(void *parameter) {
    runMission("boot_user", 0, -1);
    vTaskDelete(NULL);
}

/**
 * @brief Setup function to initialize the device and its peripherals.
 * 
 * - Waits for the internal capacitors of the joints to charge.
 * - Initializes communication interfaces (S.BUS or Serial0).
 * - Configures I2C communication with specified pins and clock speed.
 * - Sets up the buzzer and triggers a button buzzer action.
 * - Initializes USB CDC communication for the ESP32-S3.
 * - Configures and initializes joint control with specific parameters.
 * - Optionally initializes the screen controller for display (commented out).
 * - If file system usage is enabled, initializes the file system and checks/creates a boot mission.
 * - If ESP-NOW is enabled, initializes wireless communication and sets up a JSON command callback.
 */
void setup() {
  /*
  Waits for the internal capacitors of the joints to charge.
  */
  delay(1000);

#ifdef UART0_AS_SBUS
    sbus.Begin();
    msg("S.BUS mode initialized.");
#else
    Serial0.begin(ESP32S3_BAUD_RATE);
    msg("Serial0 initialized for normal communication.");
    // Hardware UART does not require a connected terminal.
#endif
  msg("Device starting...");

  Wire.begin(IIC_SDA, IIC_SCL);
  Wire.setClock(400000);
  Wire.setTimeOut(20);
  // IMU initialization and continuous acquisition run after other peripherals start.
  resultMutex = xSemaphoreCreateMutex();
  configASSERT(resultMutex);

  cmdQueue = xQueueCreate(CMD_QUEUE_LEN, CMD_MAX_LEN);
  if (cmdQueue == NULL) {
    msg("Failed to create command queue!");
    abort();
  }

  // buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);
  buttonBuzzer();

  // CAN-2.0
  CanStart();

  // fake args, it will be ignored by the USB stack, default baudrate is 12Mbps
  USBSerial.setRxBufferSize(2048);
  USBSerial.onEvent(usbCdcEvent);
  USBSerial.begin(ESP32S3_BAUD_RATE);
  USBSerial.setTxTimeoutMs(0);
  USB.begin();
  msg("ESP32-S3 USB CDC DONE!");

  // init serial for bus devs
  jointsCtrl.init(BUS_SERVO_BAUD_RATE);
  busDevices.begin(Serial1);

#ifdef USE_UI_CTRL
  screenCtrl.init();
  screenCtrl.displayText("LYgion", 0, 0, 2);
  screenCtrl.displayText("Robotics", 0, 16, 2);
  msg("ScreenCtrl initialized.");
#else
  msg("ScreenCtrl NOT initialized.");
#endif

#ifdef USE_FILE_SYSTEM
  filesCtrl.init();
  if(!filesCtrl.checkMission("boot")) {
    filesCtrl.createMission("boot", "this is the boot mission.");
    filesCtrl.appendStep("boot", "{\"T\":400,\"mode\":1,\"ap_ssid\":\"Robot\",\"ap_password\":\"12345678\",\"channel\":1,\"sta_ssid\":\"\",\"sta_password\":\"\"}");
    msg("creat New mission: boot");
  } else {
    msg("boot mission already exists.");
    if (filesCtrl.checkStepByType("boot", CMD_SET_WIFI_MODE)) {
      msg("Already set wifi mode.");
    } else {
      filesCtrl.appendStep("boot", "{\"T\":400,\"mode\":1,\"ap_ssid\":\"Robot\",\"ap_password\":\"12345678\",\"channel\":1,\"sta_ssid\":\"\",\"sta_password\":\"\"}");
      msg("Haven't set wifi mode yet. Appending to boot mission.");
    }
  }
  runMission("boot", 0, 1);
  delay(200);
  msg("File system initialized.");
#else
  msg("File system NOT initialized.");
#endif

#ifdef USE_ESP_NOW
  wireless.espnowInit(false);
  // wireless.espnowInit(true); // longRange Mode
#ifndef ESP_NOW_SYNC
  wireless.setJsonCommandCallback(addJsonCmd2Queue);
#else
  wireless.setRelativeStepsCtrlCallback(relativeStepsCtrl);
#endif
  msg("ESP-NOW initialized.");
#else
  msg("ESP-NOW NOT initialized.");
#endif

#ifndef USE_HUB_MOTORS
  msg("Hub motors NOT initialized.");
#endif

  // buttonUI
  /*
  Assign button events to menu actions
  must be after wifi function
  */
  menu_init_RD();

#ifdef USE_ROBOTIC_ARM
  jointsCtrl.setJointType(JOINT_TYPE_SC);
  jointsCtrl.setEncoderStepRange(1024, 220);
  // jointsCtrl.setJointsZeroPosArray(jointsZeroPos);
  jointsCtrl.allLedCtrl(40, 160, 0, 255);
  wireless.addMacToPeer(broadcastAddress);
  if (!filesCtrl.checkStepByType("boot", CMD_SET_JOINTS_ZERO)) {
    msg("No CMD_SET_JOINTS_ZERO found, torqueLock <OFF>");
    jointsCtrl.setJointsZeroPosArray(jointsZeroPos);
    jointsCtrl.torqueLockMode = false;
    jointsCtrl.fineTuningMode = true;
    jointsCtrl.allLedCtrl(40, 255, 128, 0);

    double maxSpeedBuffer = jointsCtrl.getMaxJointsSpeed();
    jointsCtrl.setMaxJointsSpeed(50);
    double anglesBuffer[4] = {0, 0, 0, -90};
    jointsCtrl.linkArmSCJointsCtrlAngle(anglesBuffer);
    delay(3500);
    jointsCtrl.allLedCtrl(40, 255, 32, 0);
    jointsCtrl.setMaxJointsSpeed(maxSpeedBuffer);

    jointsCtrl.torqueLock(254, 0);
    screenCtrl.changeSingleLine(1, "-----o---o     Manual", 0);
    screenCtrl.changeSingleLine(2, "     |   | Joint Pose", 0);
    screenCtrl.changeSingleLine(3, "     oo--o Status<OK>", 0);
    screenCtrl.changeSingleLine(4, "then longPress <Down>", 1);
  } else {
    double maxSpeedBuffer = jointsCtrl.getMaxJointsSpeed();
    jointsCtrl.setMaxJointsSpeed(0.1);
    jointsCtrl.linkArmFPVIK(LINK_AB + LINK_BF_1 + LINK_EF/2, 
                            0, 
                            LINK_BF_2,
                            GRIPPER_OPEN);
    msg("JointsCtrl initialized.");
    jointsCtrl.setMaxJointsSpeed(maxSpeedBuffer);

    String line_1 = "STA:" + wireless.getSTAIP();
    String line_2 = "MAC:" + wireless.getMac();
    String line_3;
    if (espnowMode == 0) {
      jointsCtrl.allLedCtrl(40, 0, 0, 0);
      line_3 = "ESP-NOW <OFF>";
    } else if (espnowMode == 1) {
      jointsCtrl.allLedCtrl(40, 64, 0, 255);
      line_3 = "ESP-NOW <ON> KnownMAC";
    } else if (espnowMode == 2) {
      jointsCtrl.allLedCtrl(40, 64, 0, 255);
      line_3 = "ESP-NOW <ON> BRD MAC";
    }
    String line_4;
    if (jointsCtrl.espnowLeader) {
      jointsCtrl.allLedCtrl(40, 0, 32, 255);
      line_4 = "BRD Leader <ON>";
    } else {
      jointsCtrl.allLedCtrl(40, 0, 0, 0);
      line_4 = "BRD Leader <OFF>";
    }
    screenCtrl.changeSingleLine(1, line_1, 0);
    screenCtrl.changeSingleLine(2, line_2, 0);
    screenCtrl.changeSingleLine(3, line_3, 0);
    screenCtrl.changeSingleLine(4, line_4, 1);

    jointsCtrl.allLedCtrl(40, 0, 0, 0);
  }
#else
  screenCtrl.clearDisplay();
  String line_1 = filesCtrl.getValueByMissionNameAndKey("boot", CMD_SET_WIFI_MODE, "ap_ssid");
  String line_2 = "AP:" + wireless.getAPIP();
  String line_3 = "STA:" + wireless.getSTAIP();
  String line_4 = "MAC:" + wireless.getMac();
  screenCtrl.changeSingleLine(1, line_1, 0);
  screenCtrl.changeSingleLine(2, line_2, 0);
  screenCtrl.changeSingleLine(3, line_3, 0);
  screenCtrl.changeSingleLine(4, line_4, 1);
#endif

  setupHttpRoutes();
  server.begin();

  baudrate = jointsCtrl.baudrate;
  sta_ip = wireless.getSTAIP();
  ap_ip = wireless.getAPIP();
  mac_addr = wireless.getMac();
  
  // runMission("boot_user", 0, -1);
  xTaskCreatePinnedToCore(
      runMissionTask,
      "MissionTask", 
      8192,            
      NULL,            
      1,               
      NULL,            
      1                
  );

  if (jointsCtrl.sbusCtrl) {
    jointsCtrl.allLedCtrl(40, 255, 255, 255);
  }

#ifdef CAN_BUS_MACHINE
  screenCtrl.init();
  screenCtrl.displayText("CAN-TEST", 0, 0, 4);
#endif

#ifdef TTL_ADAPTER_MACHINE
  screenCtrl.init();
  screenCtrl.displayText("TTLA IO", 0, 0, 4);
#endif
  // Same continuous sampling model as QMI-FINAL-1; HTTP/JSON only reads snapshots.
  const BaseType_t imuTaskCreated = xTaskCreate([](void*) {
    busDevices.startImuSampling();
    for (;;) { busDevices.pollImu(); vTaskDelay(pdMS_TO_TICKS(2) > 0 ? pdMS_TO_TICKS(2) : 1); }
  }, "ImuSampler", 4096, nullptr, 1, nullptr);
  configASSERT(imuTaskCreated == pdPASS);
}

void delayInterruptible(int ms) {
    int elapsed = 0;
    int step = 10;
    while (elapsed < ms) {
        if (breakloop) {
            // breakloop = false;
            return;
        }
        int delayTime = (ms - elapsed > step) ? step : (ms - elapsed);
        delay(delayTime);
        elapsed += delayTime;
    }
}

void breakLoop() {
  breakloop = true;
  if (cmdQueue) xQueueReset(cmdQueue);
}

bool runStep(String missionName, int step) {
  String outputString;
  JsonDocument jsonCmdReceive;
  DeserializationError err;
  outputString = filesCtrl.readStep(missionName, step);
  err = deserializeJson(jsonCmdReceive, outputString);
  if (err == DeserializationError::Ok) {
    jsonCmdReceiveHandler(jsonCmdReceive);
    return true;
  } else {
    msg("JSON parsing error (this is a normal output when booting or running Mission): ");
    msg(err.c_str());
    return false;
  }
}

void executeMission(String missionName, int intervalTime, int loopTimes) {
  if (!filesCtrl.checkMission(missionName)) {
    return;
  }
  if (intervalTime < 0) {intervalTime = 0;}
  int j = 1;
  while (true) {
    msg("Running loop: ", false);
    msg(String(j));
    int i = 1;
    while (true) {
      if (breakloop) {
        breakloop = false;
        msg("breakloop");
        return;
      }
      if (runStep(missionName, i)) {
        msg("Step: ", false);
        msg(String(i));
        i++;
        delayInterruptible(intervalTime);
        delay(1); // Even zero-interval scripts must yield between commands.
      } else {
        msg("Mission Completed.");
        break;
      }
    }
    delay(1); // Yield even when an empty legacy mission loops forever.
    j++;
    if (j > loopTimes && loopTimes != -1) {
      return;
    }
  }
}

void runMission(String missionName, int intervalTime, int loopTimes) {
  MissionGuard guard;
  if (!guard.acquired) return;
  MissionRunState state;
  breakloop.store(false);
  executeMission(missionName, intervalTime, loopTimes);
}

void addJsonCmd2Queue(const char* jsonString) {
  if (!cmdQueue || !jsonString) return;
  const size_t len = strnlen(jsonString, CMD_MAX_LEN);
  if (!len || len >= CMD_MAX_LEN) return;

  // Parse privately: callbacks must not touch the executing command's document.
  JsonDocument command;
  if (deserializeJson(command, jsonString, len) || !command["T"].is<int>()) return;
  if (command["T"].as<int>() == CMD_BREAK_LOOP) {
    // Out-of-band stop remains effective while runMission occupies loop().
    // Do not enqueue it: executing it later could erase a newer command.
    xQueueReset(cmdQueue);
    breakloop.store(true);
    return;
  }

  // FreeRTOS copies CMD_MAX_LEN bytes, including for short string literals.
  char cmdBuffer[CMD_MAX_LEN] = {};
  memcpy(cmdBuffer, jsonString, len);
  xQueueOverwrite(cmdQueue, cmdBuffer);
}


void relativeStepsCtrl(int j0, int j1, int j2, int j3) {
  espNowPacket[0] = j0;
  espNowPacket[1] = j1;
  espNowPacket[2] = j2;
  espNowPacket[3] = j3;
  // jointsCtrl.relativeStepsCtrl(j0, j1, j2, j3);
}

void jsonCmdReceiveHandler(const JsonDocument& jsonCmdInput) {
  const int idType = jsonCmdInput["T"] | -1;
  if (idType == 13 || idType == 23 || idType == 33) {
    JsonDocument result; result["T"] = idType;
    if (jsonCmdInput["rid"].is<uint32_t>()) result["rid"] = jsonCmdInput["rid"];
    int oldId = jsonCmdInput["old_id"] | -1, newId = jsonCmdInput["new_id"] | -1;
    bool valid = jsonCmdInput["old_id"].is<int>() && jsonCmdInput["new_id"].is<int>() && oldId >= 0 && oldId <= 253 && newId >= 0 && newId <= 253 && oldId != newId;
    bool ok = valid && (idType == 13 ? jointsCtrl.changeIDSTSM(oldId,newId) : idType == 23 ? jointsCtrl.changeIDHL(oldId,newId) : jointsCtrl.changeIDSC(oldId,newId));
    result["ok"] = ok;
    if (ok) result["new_id"] = newId;
    else result["error"] = valid ? "ID change not verified; check connection, duplicate IDs and scan again" : "distinct old_id and new_id in 0..253 required";
    publishResult(result); return;
  }
  if (!validSyncCommand(jsonCmdInput)) {
    JsonDocument result;
    result["T"] = jsonCmdInput["T"];
    if (jsonCmdInput["rid"].is<uint32_t>()) result["rid"] = jsonCmdInput["rid"];
    result["ok"] = false;
    result["error"] = "invalid_sync_parameters";
    publishResult(result);
    return;
  }
  const int missionType = jsonCmdInput["T"] | -1;
  if (missionType >= 300 && missionType <= 310) {
    JsonDocument result;
    MissionGuard guard;
    MissionApi::handle(missionStorage, jsonCmdInput, result, !guard.acquired);
    const bool run = result["ok"].as<bool>() && (missionType == 307 || missionType == 308);
    if (run) {
      MissionRunState state;
      breakloop.store(false);
      publishResult(result);
      if (missionType == 307) runStep(jsonCmdInput["name"], jsonCmdInput["step"]);
      else executeMission(jsonCmdInput["name"], jsonCmdInput["interval"], jsonCmdInput["loop"]);
      result["status"] = "finished";
      publishResult(result);
    } else publishResult(result);
    return;
  }
  JsonDocument deviceResult;
  if (busDevices.handle(jsonCmdInput, deviceResult)) {
    publishResult(deviceResult);
    return;
  }
  JsonDocument jsonFeedback;
  String outputString;
  int cmdType;
  cmdType = jsonCmdInput["T"].as<int>();
  switch(cmdType){
  // for web ui ctrl
  case CMD_BREAK_LOOP:
                        breakLoop();
                        break;
  case CMD_WEB_SET_JOINTS_BAUD:
                        jointsCtrl.setBaudRate(jsonCmdInput["baud"]);
                        baudrate = jointsCtrl.baudrate;
                        break;
  
  case CMD_STSM_CTRL:
                        jointsCtrl.stepsCtrlSMST(jsonCmdInput["id"],
                                                 jsonCmdInput["pos"],
                                                 jsonCmdInput["spd"],
                                                 jsonCmdInput["acc"],
                                                 true);
                        break;
  case CMD_STSM_SET_MIDDLE:
                        jointsCtrl.setMiddleSTSM(jsonCmdInput["id"]);
                        break;
  case CMD_STSM_CHANGE_ID:
                        jointsCtrl.changeIDSTSM(jsonCmdInput["old_id"], 
                                                jsonCmdInput["new_id"]);
                        break;
  case CMD_STSM_TORQUE_LOCK:
                        jointsCtrl.torqueLockSTSM(jsonCmdInput["id"],
                                                  jsonCmdInput["state"]);
                        break;
  case CMD_STSM_FEEDBACK:
                        jointFeedback = jointsCtrl.feedbackSTSM(jsonCmdInput["id"]);
                        jsonFeedback.clear();
                        jsonFeedback["T"] = -CMD_STSM_FEEDBACK;
                        if (jointFeedback[0] == -1) {
                          jsonFeedback["ps"] = -1;
                          serializeJson(jsonFeedback, outputString);
                          msg(outputString);
                        } else {
                          jsonFeedback["ps"] = jointFeedback[0];
                          jsonFeedback["pos"] = jointFeedback[1];
                          jsonFeedback["spd"] = jointFeedback[2];
                          jsonFeedback["load"] = jointFeedback[3];
                          jsonFeedback["vol"] = jointFeedback[4];
                          jsonFeedback["temp"] = jointFeedback[5];
                          jsonFeedback["mov"] = jointFeedback[6];
                          jsonFeedback["curt"] = jointFeedback[7];
                          serializeJson(jsonFeedback, outputString);
                          msg(outputString);
                        }
                        break;
  case CMD_STSM_SYNC_CTRL:{
                        JsonArrayConst id  = jsonCmdInput["id"].as<JsonArrayConst>();
                        JsonArrayConst pos = jsonCmdInput["pos"].as<JsonArrayConst>();
                        JsonArrayConst spd = jsonCmdInput["spd"].as<JsonArrayConst>();
                        JsonArrayConst acc = jsonCmdInput["acc"].as<JsonArrayConst>();

                        int idn = jsonCmdInput["idn"].as<int>();

                        jointsCtrl.jsonSyncCtrlSMST(id, idn, pos, spd, acc);
                        break;
  }
  case CMD_STSM_SYNC_SIMPLE_CTRL:{
                        JsonArrayConst id  = jsonCmdInput["id"].as<JsonArrayConst>();
                        JsonArrayConst pos = jsonCmdInput["pos"].as<JsonArrayConst>();

                        int idn = jsonCmdInput["idn"].as<int>();

                        jointsCtrl.jsonSyncSimpleCtrlSMST(id, idn, pos);
                        break;
  }



  case CMD_HL_CTRL:
                        jointsCtrl.stepsCtrlHL(jsonCmdInput["id"],
                                               jsonCmdInput["pos"],
                                               jsonCmdInput["spd"],
                                               jsonCmdInput["acc"],
                                               jsonCmdInput["cl"],
                                               true);
                        break;
  case CMD_HL_SET_MIDDLE:
                        jointsCtrl.setMiddleHL(jsonCmdInput["id"]);
                        break;
  case CMD_HL_CHANGE_ID:
                        jointsCtrl.changeIDHL(jsonCmdInput["old_id"],
                                              jsonCmdInput["new_id"]);
                        break;
  case CMD_HL_TORQUE_LOCK:
                        jointsCtrl.torqueLockHL(jsonCmdInput["id"],
                                                jsonCmdInput["state"]);
                        break;
  case CMD_HL_FEEDBACK:
                        jointFeedback = jointsCtrl.feedbackHL(jsonCmdInput["id"]);
                        jsonFeedback.clear();
                        jsonFeedback["T"] = -CMD_HL_FEEDBACK;
                        if (jointFeedback[0] == -1) {
                          jsonFeedback["ps"] = -1;
                          serializeJson(jsonFeedback, outputString);
                          msg(outputString);
                        } else {
                          jsonFeedback["ps"] = jointFeedback[0];
                          jsonFeedback["pos"] = jointFeedback[1];
                          jsonFeedback["spd"] = jointFeedback[2];
                          jsonFeedback["load"] = jointFeedback[3];
                          jsonFeedback["vol"] = jointFeedback[4];
                          jsonFeedback["temp"] = jointFeedback[5];
                          jsonFeedback["mov"] = jointFeedback[6];
                          jsonFeedback["curt"] = jointFeedback[7];
                          serializeJson(jsonFeedback, outputString);
                          msg(outputString);
                        }
                        break;
  case CMD_HL_SYNC_CTRL:{
                        JsonArrayConst id  = jsonCmdInput["id"].as<JsonArrayConst>();
                        JsonArrayConst pos = jsonCmdInput["pos"].as<JsonArrayConst>();
                        JsonArrayConst spd = jsonCmdInput["spd"].as<JsonArrayConst>();
                        JsonArrayConst acc = jsonCmdInput["acc"].as<JsonArrayConst>();
                        JsonArrayConst tor = jsonCmdInput["tor"].as<JsonArrayConst>();

                        int idn = jsonCmdInput["idn"].as<int>();

                        jointsCtrl.jsonSyncCtrlHL(id, idn, pos, spd, acc, tor);
                        break;
  }
  case CMD_HL_SYNC_SIMPLE_CTRL:{
                        JsonArrayConst id  = jsonCmdInput["id"].as<JsonArrayConst>();
                        JsonArrayConst pos = jsonCmdInput["pos"].as<JsonArrayConst>();
                        JsonArrayConst tor = jsonCmdInput["tor"].as<JsonArrayConst>();

                        int idn = jsonCmdInput["idn"].as<int>();

                        jointsCtrl.jsonSyncSimpleCtrlHL(id, idn, pos, tor);
                        break;
  }




  case CMD_SC_CTRL:
                        jointsCtrl.stepsCtrlSC(jsonCmdInput["id"],
                                               jsonCmdInput["pos"],
                                               jsonCmdInput["time"],
                                               jsonCmdInput["spd"],
                                               true);
                        break;
  case CMD_SC_CHANGE_ID:
                        jointsCtrl.changeIDSC(jsonCmdInput["old_id"],
                                              jsonCmdInput["new_id"]);
                        break;
  case CMD_SC_TORQUE_LOCK:
                        jointsCtrl.torqueLockSC(jsonCmdInput["id"],
                                                jsonCmdInput["state"]);
                        break;
  case CMD_SC_FEEDBACK:
                        jointFeedback = jointsCtrl.feedbackSC(jsonCmdInput["id"]);
                        jsonFeedback.clear();
                        jsonFeedback["T"] = -CMD_SC_FEEDBACK;
                        if (jointFeedback[0] == -1) {
                          jsonFeedback["ps"] = -1;
                          serializeJson(jsonFeedback, outputString);
                          msg(outputString);
                        } else {
                          jsonFeedback["ps"] = jointFeedback[0];
                          jsonFeedback["pos"] = jointFeedback[1];
                          jsonFeedback["spd"] = jointFeedback[2];
                          jsonFeedback["load"] = jointFeedback[3];
                          jsonFeedback["vol"] = jointFeedback[4];
                          jsonFeedback["temp"] = jointFeedback[5];
                          jsonFeedback["mov"] = jointFeedback[6];
                          jsonFeedback["curt"] = jointFeedback[7];
                          serializeJson(jsonFeedback, outputString);
                          msg(outputString);
                        }
                        break;
  case CMD_SC_SYNC_CTRL:{
                        JsonArrayConst id  = jsonCmdInput["id"].as<JsonArrayConst>();
                        JsonArrayConst pos = jsonCmdInput["pos"].as<JsonArrayConst>();
                        JsonArrayConst spd = jsonCmdInput["spd"].as<JsonArrayConst>();

                        int idn = jsonCmdInput["idn"].as<int>();

                        jointsCtrl.jsonSyncCtrlSC(id, idn, pos, spd);
                        break;
  }
  case CMD_SC_SYNC_SIMPLE_CTRL:{
                        JsonArrayConst id  = jsonCmdInput["id"].as<JsonArrayConst>();
                        JsonArrayConst pos = jsonCmdInput["pos"].as<JsonArrayConst>();

                        int idn = jsonCmdInput["idn"].as<int>();

                        jointsCtrl.jsonSyncSimpleCtrlSC(id, idn, pos);
                        break;
  }



  case CMD_DELAY: 
                        delayInterruptible(jsonCmdInput["delay"]);
                        break;



  case CMD_SET_JOINTS_BAUD:
                        jointsCtrl.setBaudRate(jsonCmdInput["baud"]);
                        break;
  case CMD_SET_JOINTS_TYPE:
                        if (jointsCtrl.setJointType(jsonCmdInput["type"])) {
                          msg("Joint type set successfully.");
                        } else {
                          msg("Failed to set joint type.");
                        }
                        break;
  case CMD_SET_ENCODER:
                        if (jointsCtrl.setEncoderStepRange(jsonCmdInput["steps"], jsonCmdInput["angle"])) {
                          msg("Encoder step range set successfully.");
                        } else {
                          msg("Failed to set encoder step range.");
                        }
                        break;
  case CMD_SINGLE_FEEDBACK:
                        jointFeedback = jointsCtrl.singleFeedBack(jsonCmdInput["id"]);
                        jsonFeedback.clear();
                        jsonFeedback["T"] = -CMD_SINGLE_FEEDBACK;
                        if (jointFeedback[0] == -1) {
                          jsonFeedback["ps"] = -1;
                          serializeJson(jsonFeedback, outputString);
                          msg(outputString);
                        } else {
                          jsonFeedback["ps"] = jointFeedback[0];
                          jsonFeedback["pos"] = jointFeedback[1];
                          jsonFeedback["spd"] = jointFeedback[2];
                          jsonFeedback["load"] = jointFeedback[3];
                          jsonFeedback["vol"] = jointFeedback[4];
                          jsonFeedback["temp"] = jointFeedback[5];
                          jsonFeedback["mov"] = jointFeedback[6];
                          jsonFeedback["curt"] = jointFeedback[7];
                          serializeJson(jsonFeedback, outputString);
                          msg(outputString);
                        }
                        break;
  case CMD_PING:
                        jsonFeedback.clear();
                        jsonFeedback["T"] = -CMD_PING;
                        if (jointsCtrl.ping(jsonCmdInput["id"])) {
                          jsonFeedback["ps"] = 1;
                        } else {
                          jsonFeedback["ps"] = -1;
                        }
                        serializeJson(jsonFeedback, outputString);
                        msg(outputString);
                        break;
  case CMD_CHANGE_ID:
                        jointsCtrl.changeID(jsonCmdInput["old_id"], 
                                            jsonCmdInput["new_id"]);
                        break;
  case CMD_SET_MIDDLE:
                        if (jointsCtrl.setMiddle(jsonCmdInput["id"])) {
                          msg("Joint middle set successfully.");
                        } else {
                          msg("Failed to set joint middle.");
                        }
                        break;
  case CMD_MOVE_MIDDLE:
                        jointsCtrl.moveMiddle(jsonCmdInput["id"]);
                        break;
  case CMD_TORQUE_LOCK:
                        jointsCtrl.torqueLock(jsonCmdInput["id"], 
                                              jsonCmdInput["state"]);
                        break;
  case CMD_STEPS_CTRL_SC:
                        jointsCtrl.stepsCtrlSC(jsonCmdInput["id"], 
                                               jsonCmdInput["pos"], 
                                               jsonCmdInput["time"], 
                                               jsonCmdInput["spd"], 
                                               jsonCmdInput["move"]);
                        break;
  case CMD_STEPS_CTRL_SMST:
                        jsonFeedback["ok"] = false;
                        jsonFeedback["error"] = "unsupported_command";
                        break;
  case CMD_STEPS_CTRL_HL:
                        jsonFeedback["ok"] = false;
                        jsonFeedback["error"] = "unsupported_command";
                        break;
  case CMD_ANGLE_CTRL_SC:
                        jointsCtrl.angleCtrlSC(jsonCmdInput["id"], 
                                               jsonCmdInput["mid"], 
                                               jsonCmdInput["ang"], 
                                               jsonCmdInput["spd"], 
                                               jsonCmdInput["move"]);
                        break;
  case CMD_RAD_CTRL_SC:
                        jointsCtrl.radCtrlSC(jsonCmdInput["id"],
                                             jsonCmdInput["mid"],
                                             jsonCmdInput["rad"],
                                             jsonCmdInput["spd"],
                                             jsonCmdInput["move"]);
                        break;
  case CMD_MOVE_TRIGGER:
                        jointsCtrl.moveTrigger();
                        break;
  case CMD_TORQUE_LIMIT:
                        jointsCtrl.setTorqueLimitSC(jsonCmdInput["id"],
                                                    jsonCmdInput["limit"]);
                        break;


  // --- --- --- for applications: LyLinkArm --- --- ---
  case CMD_GET_JOINTS_ZERO:
                        memcpy(jointsZeroPos, jointsCtrl.getJointsZeroPosArray(), sizeof(jointsZeroPos));
                        jsonFeedback.clear();
                        jsonFeedback["T"] = -CMD_GET_JOINTS_ZERO;
                        for (int i = 0; i < JOINTS_NUM; i++) {  
                            jsonFeedback["pos"][i] = jointsZeroPos[i];
                        }
                        serializeJson(jsonFeedback, outputString);
                        msg(outputString);
                        break;
  case CMD_SET_JOINTS_ZERO:
                        for (int i = 0; i < JOINTS_NUM; i++) {
                            jointsZeroPos[i] = jsonCmdInput["pos"][i];
                            
                        }
                        jointsCtrl.setJointsZeroPosArray(jointsZeroPos);
                        break;
  case CMD_GET_LINK_ARM_POS:
                        memcpy(jointsCurrentPos, jointsCtrl.getLinkArmPosSC(), sizeof(jointsCurrentPos));
                        jsonFeedback.clear();
                        jsonFeedback["T"] = -CMD_GET_LINK_ARM_POS;
                        for (int i = 0; i < JOINTS_NUM; i++) {
                            jsonFeedback["pos"][i] = jointsCurrentPos[i];
                        }
                        serializeJson(jsonFeedback, outputString);
                        msg(outputString);
                        break;
  case CMD_SET_LINK_ARM_ZERO:
                        jointsCtrl.setCurrentSCPosMiddle();
                        memcpy(jointsCurrentPos, jointsCtrl.getLinkArmPosSC(), sizeof(jointsCurrentPos));
                        jsonFeedback.clear();
                        jsonFeedback["T"] = CMD_SET_JOINTS_ZERO;
                        for (int i = 0; i < JOINTS_NUM; i++) {
                            jsonFeedback["pos"][i] = jointsCurrentPos[i];
                        }
                        serializeJson(jsonFeedback, outputString);
                        msg(outputString);
                        break;
  case CMD_LINK_ARM_SC_JOINTS_CTRL_ANGLE:
                        for (int i = 0; i < JOINTS_NUM; i++) {
                            jointsGoalBuffer[i] = jsonCmdInput["ang"][i];
                        }
                        jointsCtrl.linkArmSCJointsCtrlAngle(jointsGoalBuffer);
                        break;
  case CMD_LINK_ARM_SC_JOINTS_CTRL_RAD:
                        for (int i = 0; i < JOINTS_NUM; i++) {
                            jointsGoalBuffer[i] = jsonCmdInput["rad"][i];
                        }
                        jointsCtrl.linkArmSCJointsCtrlRad(jointsGoalBuffer);
                        break;
  case CMD_XYZG_CTRL:
                        memcpy(xyzgIK,
                        jointsCtrl.linkArmSpaceIK(jsonCmdInput["xyzg"][0],
                                                  jsonCmdInput["xyzg"][1],
                                                  jsonCmdInput["xyzg"][2],
                                                  jsonCmdInput["xyzg"][3]),
                        sizeof(xyzgIK));
                        if (xyzgIK[0] == -1) {
                          jsonFeedback.clear();
                          jsonFeedback["T"] = -CMD_XYZG_CTRL;
                          jsonFeedback["ik"] = xyzgIK[0];
                          for (int i = 0; i < JOINTS_NUM; i++) {
                            jsonFeedback["xyzg"][i] = xyzgIK[i + 1];
                          }
                          serializeJson(jsonFeedback, outputString);
                          msg(outputString);
                        }
                        break;
  case CMD_FPV_ABS_CTRL:
                        memcpy(rbzgIKFeedback, 
                               jointsCtrl.linkArmFPVIK(jsonCmdInput["rbzg"][0],
                                                      jsonCmdInput["rbzg"][1],
                                                      jsonCmdInput["rbzg"][2],
                                                      jsonCmdInput["rbzg"][3]), 
                               sizeof(rbzgIKFeedback));
                        if (rbzgIKFeedback[0] == -1) {
                          jsonFeedback.clear();
                          jsonFeedback["T"] = -CMD_FPV_ABS_CTRL;
                          jsonFeedback["ik"] = rbzgIKFeedback[0];
                          for (int i = 0; i < JOINTS_NUM; i++) {
                            jsonFeedback["rbzg"][i] = rbzgIKFeedback[i + 1];
                          }
                          serializeJson(jsonFeedback, outputString);
                          msg(outputString);
                        }
                        break;
  case CMD_SMOOTH_XYZG_CTRL:
                        memcpy(xyzgIKFeedback, 
                          jointsCtrl.smoothXYZGCtrl(jsonCmdInput["xyzg"][0],
                                                    jsonCmdInput["xyzg"][1],
                                                    jsonCmdInput["xyzg"][2],
                                                    jsonCmdInput["xyzg"][3],
                                                    jsonCmdInput["spd"]), 
                          sizeof(xyzgIKFeedback));
                        if (xyzgIKFeedback[0] == -1) {
                          jsonFeedback.clear();
                          jsonFeedback["T"] = -CMD_SMOOTH_XYZG_CTRL;
                          jsonFeedback["ik"] = xyzgIKFeedback[0];
                          for (int i = 0; i < JOINTS_NUM; i++) {
                            jsonFeedback["xyzg"][i] = xyzgIKFeedback[i + 1];
                          }
                          serializeJson(jsonFeedback, outputString);
                          msg(outputString);
                        }
                        break;
  case CMD_SMOOTH_FPV_ABS_CTRL:
                        memcpy(rbzgIKFeedback, 
                               jointsCtrl.smoothFPVAbsCtrl(jsonCmdInput["rbzg"][0],
                                                           jsonCmdInput["rbzg"][1],
                                                           jsonCmdInput["rbzg"][2],
                                                           jsonCmdInput["rbzg"][3],
                                                           jsonCmdInput["spd"],
                                                           jsonCmdInput["br"]), 
                               sizeof(rbzgIKFeedback));
                        if (rbzgIKFeedback[0] == -1) {
                          jsonFeedback.clear();
                          jsonFeedback["T"] = -CMD_FPV_ABS_CTRL;
                          jsonFeedback["ik"] = rbzgIKFeedback[0];
                          for (int i = 0; i < JOINTS_NUM; i++) {
                            jsonFeedback["rbzg"][i] = rbzgIKFeedback[i + 1];
                          }
                          serializeJson(jsonFeedback, outputString);
                          msg(outputString);
                        }
                        break;
  case CMD_SET_MAX_JOINTS_SPEED:
                        jointsCtrl.setMaxJointsSpeed(jsonCmdInput["spd"]);
                        break;
  case CMD_GET_JOINT_FB_RADS:
                        memcpy(jointFBRads, jointsCtrl.getJointFBRads(), sizeof(jointFBRads));
                        jsonFeedback.clear();
                        jsonFeedback["T"] = -CMD_GET_JOINT_FB_RADS;
                        for (int i = 0; i < JOINTS_NUM; i++) {
                          jsonFeedback["fb-rads"][i] = jointFBRads[i];
                        }
                        serializeJson(jsonFeedback, outputString);
                        msg(outputString);
                        break;
  case CMD_GET_JOINT_GOAL_RADS:
                        memcpy(jointsGoalBuffer, jointsCtrl.getJointGoalRads(), sizeof(jointsGoalBuffer));
                        jsonFeedback.clear();
                        jsonFeedback["T"] = -CMD_GET_JOINT_GOAL_RADS;
                        for (int i = 0; i < JOINTS_NUM; i++) {
                          jsonFeedback["goal-rads"][i] = jointsGoalBuffer[i];
                        }
                        serializeJson(jsonFeedback, outputString);
                        msg(outputString);
                        break;
  case CMD_GET_XYZG_IK:
                        memcpy(xyzgIK, jointsCtrl.getXYZGIK(), sizeof(xyzgIK));
                        jsonFeedback.clear();
                        jsonFeedback["T"] = -CMD_GET_XYZG_IK;
                        for (int i = 0; i < JOINTS_NUM + 1; i++) {
                          jsonFeedback["xyzg"][i] = xyzgIK[i];
                        }
                        serializeJson(jsonFeedback, outputString);
                        msg(outputString);
                        break;
  case CMD_GET_RBZG_IK:
                        memcpy(rbzgIK, jointsCtrl.getRBZGIK(), sizeof(rbzgIK));
                        jsonFeedback.clear();
                        jsonFeedback["T"] = -CMD_GET_RBZG_IK;
                        for (int i = 0; i < JOINTS_NUM + 1; i++) {
                          jsonFeedback["rbzg"][i] = rbzgIK[i];
                        }
                        serializeJson(jsonFeedback, outputString);
                        msg(outputString);
                        break;
  case CMD_SET_LINK_ARM_FEEDBACK_FLAG:
                        if (jsonCmdInput["flag"] == 0) {
                          jointsCtrl.setLinkArmFeedbackFlag(false, jsonCmdInput["hz"]);
                        } else if (jsonCmdInput["flag"] == 1) {
                          jointsCtrl.setLinkArmFeedbackFlag(true, jsonCmdInput["hz"]);
                        }
                        break;
  case CMD_CONST_CTRL:
                        if (jsonCmdInput["flag"] == 0) {
                          jointsCtrl.constantCtrl(false, -1, 0);
                        } else if (jsonCmdInput["flag"] == 1) {
                          jointsCtrl.constantCtrl(true, jsonCmdInput["axis"], jsonCmdInput["delta"]);
                        }
                        break;
  case CMD_JOINT_CALIB:
                        jointsCtrl.setJointsZeroPosArray(jointsZeroPos);
                        jointsCtrl.torqueLockMode = false;
                        jointsCtrl.fineTuningMode = true;
                        jointsCtrl.allLedCtrl(40, 255, 32, 0);
                        jointsCtrl.torqueLock(254, 0);
                        screenCtrl.changeSingleLine(1, "-----o---o     Manual", 0);
                        screenCtrl.changeSingleLine(2, "     |   | Joint Pose", 0);
                        screenCtrl.changeSingleLine(3, "     oo--o Status<OK>", 0);
                        screenCtrl.changeSingleLine(4, "then longPress <Down>", 1);
                        break;






  case CMD_HUB_MOTOR_CTRL:
                        jointsCtrl.hubMotorCtrl(jsonCmdInput["A"], 
                                                jsonCmdInput["B"], 
                                                jsonCmdInput["C"], 
                                                jsonCmdInput["D"]);
                        break;





  case CMD_SET_SINGLE_COLOR:
                        jointsCtrl.singleLedCtrl(jsonCmdInput["id"],
                                                 jsonCmdInput["set"][0],
                                                 jsonCmdInput["set"][1],
                                                 jsonCmdInput["set"][2],
                                                 jsonCmdInput["set"][3]);
                        break;
  case CMD_SET_ALL_COLOR:
                        jointsCtrl.allLedCtrl(jsonCmdInput["id"],
                                              jsonCmdInput["set"][0],
                                              jsonCmdInput["set"][1],
                                              jsonCmdInput["set"][2]);
                        break;
  case CMD_DISPLAY_SINGLE:
                        screenCtrl.changeSingleLine(jsonCmdInput["line"], 
                                                    jsonCmdInput["text"], 
                                                    jsonCmdInput["update"]);
                        break;
  case CMD_DISPLAY_UPDATE:
                        screenCtrl.updateFrame();
                        break;
                        
  case CMD_DISPLAY_FRAME:
                        screenCtrl.changeSingleLine(1, jsonCmdInput["l1"], false);
                        screenCtrl.changeSingleLine(2, jsonCmdInput["l2"], false);
                        screenCtrl.changeSingleLine(3, jsonCmdInput["l3"], false);
                        screenCtrl.changeSingleLine(4, jsonCmdInput["l4"], true);
                        break;
  case CMD_DISPLAY_CLEAR:
                        screenCtrl.clearDisplay();
                        break;
  case CMD_BUZZER_CTRL:
                        tone(BUZZER_PIN, jsonCmdInput["freq"]);
                        tuneStartTime = millis();
                        while (millis() - tuneStartTime < jsonCmdInput["duration"]) {
                          if (breakloop) {
                            noTone(BUZZER_PIN);
                            digitalWrite(BUZZER_PIN, HIGH);
                            break;
                          }
                          delay(1); // Keep scheduler/idle tasks running during long tones.
                        }
                        noTone(BUZZER_PIN);
                        digitalWrite(BUZZER_PIN, HIGH);
                        break;
  // case CMD_BUTTONS:
  //                       buttonInteractionCtrl(jsonCmdInput["L"],
  //                                             jsonCmdInput["I0"],
  //                                             jsonCmdInput["I1"],
  //                                             jsonCmdInput["I2"]);
  //                       break;
  case CMD_NODE_CHANG_ID:
                        jointsCtrl.ttlnChangeID(jsonCmdInput["old"],
                                                jsonCmdInput["new"]);
                        break;
  case CMD_NODE_CTRL_PWM:
                        jointsCtrl.ttlnCtrlPWM(jsonCmdInput["id"],
                                               jsonCmdInput["ch"],
                                               jsonCmdInput["pwm"]);
                        break;
  case CMD_NODE_SINGLE_LED:
                        jointsCtrl.ttlnCtrlSingleLed(jsonCmdInput["id"],
                                                     jsonCmdInput["ch"],
                                                     jsonCmdInput["r"],
                                                     jsonCmdInput["g"],
                                                     jsonCmdInput["b"]);
                        break;
  case CMD_NODE_FLUSH:
                        jointsCtrl.ttlnLedFlush(jsonCmdInput["id"],
                                                jsonCmdInput["num"]);
                        break;
  case CMD_NODE_CTRL_ALL_LED:
                        jointsCtrl.ttlnCtrlAllLed(jsonCmdInput["id"],
                                                  jsonCmdInput["num"],
                                                  jsonCmdInput["r"],
                                                  jsonCmdInput["g"],
                                                  jsonCmdInput["b"]);
                        break;


  case CMD_FORMAT_FLASH:
    filesCtrl.flash();
    break;

  case CMD_SET_WIFI_MODE: {
    const int mode = jsonCmdInput["mode"] | -1;
    String ap = jsonCmdInput["ap_ssid"] | "", pass = jsonCmdInput["ap_password"] | "";
    String ssid = jsonCmdInput["sta_ssid"] | "", password = jsonCmdInput["sta_password"] | "";
    int channel = jsonCmdInput["channel"] | 1;
    if ((mode != 0 && mode != 1) || (mode == 1 && (ap.length() < 1 || ap.length() > 32 || pass.length() < 8 || pass.length() > 63 || ssid.length() > 32 || password.length() > 63 || channel < 1 || channel > 13))) {
      jsonFeedback["ok"] = false; jsonFeedback["error"] = "invalid_wifi_settings"; break;
    }
    // Save the user's configuration independently of router availability.
    JsonDocument saved; saved.set(jsonCmdInput); saved.remove("rid");
    String config; serializeJson(saved, config);
    if (!filesCtrl.checkReplaceStep("boot", config)) {
      jsonFeedback["ok"] = false; jsonFeedback["error"] = "wifi_config_save_failed"; break;
    }
    bool applied = wireless.setWifiMode(mode, ap, pass, channel, ssid, password);
    jsonFeedback["ok"] = mode == 0 || applied;
    if (mode == 1 && !applied) jsonFeedback["error"] = "ap_start_failed";
    jsonFeedback["sta_state"] = wireless.getSTAState();
    jsonFeedback["sta_ip"] = wireless.getSTAIP();
    break;
  }
  case CMD_WIFI_INFO:
                        outputString = filesCtrl.findCmdByType("boot", CMD_SET_WIFI_MODE);
                        deserializeJson(jsonFeedback, outputString);
                        jsonFeedback.remove("ap_password");
                        jsonFeedback.remove("sta_password");
                        outputString = ""; serializeJson(jsonFeedback, outputString);
                        msg(outputString);
                        break;
  case CMD_GET_AP_IP:
                        outputString = wireless.getAPIP();
                        jsonFeedback["ap_ip"] = outputString;
                        msg(outputString);
                        break;
  case CMD_GET_STA_IP:
                        outputString = wireless.getSTAIP();
                        jsonFeedback["sta_ip"] = outputString;
                        msg(outputString);
                        break;



  case CMD_INIT_ESP_NOW:
                        wireless.espnowInit(jsonCmdInput["longrange"]);
                        break;
  case CMD_SET_ESP_NOW_MODE:
                        wireless.setEspNowMode(jsonCmdInput["mode"]);
                        break;
  case CMD_GET_MAC:
                        outputString = wireless.getMac();
                        jsonFeedback["mac"] = outputString;
                        msg(outputString);
                        break;
  case CMD_ESP_NOW_SEND:
                        wireless.sendEspNow(jsonCmdInput["mac"], jsonCmdInput["data"]);
                        break;
  case CMD_ADD_MAC:
                        wireless.addMacToPeerString(jsonCmdInput["mac"]);
                        break;
  case CMD_FAKE_MAC:    wireless.fakeMac = jsonCmdInput["fake"];
                        break;
  


  case CMD_CAN_SEND:
                        sendJsonAsCAN(jsonCmdInput);
                        break;



  case CMD_ESP32_REBOOT:
                        ESP.restart();
                        break;
  case CMD_CLEAR_NVS:
                        nvs_flash_erase();
                        delay(1000);
                        nvs_flash_init();
                        break;
  case CMD_RESET:
                        filesCtrl.flash();
                        break;
  case CMD_GET_MSG_OUTPUT:
                        jsonFeedback["echo"] = echoMsgFlag;
                        jsonFeedback["uart"] = uartMsgFlag;
                        jsonFeedback["usb"] = usbMsgFlag;
                        getMsgStatus();
                        break;
  case CMD_SET_MSG_OUTPUT:
                        echoMsgFlag = jsonCmdInput["echo"];
                        uartMsgFlag = jsonCmdInput["uart"];
                        usbMsgFlag  = jsonCmdInput["usb"];
                        break;
  case CMD_SERIAL_FORWARDING:
                        if (jsonCmdInput["sf"] == 1){
                          serialForwarding = true;
                        } else if (jsonCmdInput["sf"] == 0) {
                          serialForwarding = false;
                        }
                        break;
  
  case CMD_TEST:
                        jointsCtrl.setBaudRate(1000000);
                        // jointsCtrl.stepsCtrlSC(254, 200, 0, 0, true);
                        jointsCtrl.stepsCtrlSMST(254, 0, 0, 0, true);
                        msg("test - 1");
                        delay(2000);
                        // jointsCtrl.stepsCtrlSC(254, 800, 0, 0, true);
                        jointsCtrl.stepsCtrlSMST(254, 4000, 0, 0, true);
                        msg("test - 2");
                        delay(2000);
                        // jointsCtrl.stepsCtrlSC(254, 200, 0, 0, true);
                        jointsCtrl.stepsCtrlSMST(254, 0, 0, 0, true);
                        msg("test - 3");
                        delay(2000);
                        // jointsCtrl.stepsCtrlSC(254, 800, 0, 0, true);
                        jointsCtrl.stepsCtrlSMST(254, 4000, 0, 0, true);
                        msg("test - 4");
                        break;
  
  default:
    jsonFeedback["ok"] = false;
    jsonFeedback["error"] = "unsupported_command";
    break;
  }
  if (jsonCmdInput["rid"].is<uint32_t>()) {
    // Shared HTTP/WS results must not expose Wi-Fi credentials.
    jsonFeedback.remove("ap_password");
    jsonFeedback.remove("sta_password");
    jsonFeedback["rid"] = jsonCmdInput["rid"];
    jsonFeedback["command_T"] = jsonCmdInput["T"];
    jsonFeedback["dispatched"] = true;
    publishResult(jsonFeedback);
  }
}


// Arduino owns tud_cdc_rx_cb and transfers bytes into USBSerial's RX queue.
// This runs in the USB event task, independently of loop() and runMission().
void usbCdcEvent(void*, esp_event_base_t base, int32_t eventId, void*) {
  static CommandLineBuffer<CMD_MAX_LEN> line;
  if (base != ARDUINO_USB_CDC_EVENTS) return;
  if (eventId == ARDUINO_USB_CDC_DISCONNECTED_EVENT) {
    line.reset();
    return;
  }
  if (eventId == ARDUINO_USB_CDC_RX_OVERFLOW_EVENT) {
    // Lost bytes must never turn two partial commands into one command.
    line.reset(true);
  } else if (eventId != ARDUINO_USB_CDC_RX_EVENT) {
    return;
  }

  // Snapshot the byte count so a continuous sender cannot monopolize this task.
  // Subsequent data has its own RX event. No writes/logs/bus I/O in this callback.
  int remaining = USBSerial.available();
  while (remaining-- > 0) {
    int value = USBSerial.read();
    if (value < 0) break;
    line.push(static_cast<char>(value), addJsonCmd2Queue);
  }
}

void serialCtrl() {
  if (serialForwarding) {
    BusGuard busGuard;
    // Serial0 -> Serial1
    int budget = 128;
    while (budget-- > 0 && Serial0.available() && Serial1.availableForWrite() > 0) {
      Serial1.write(uint8_t(Serial0.read()));
    }
    // Serial1 -> Serial0
    budget = 128;
    while (budget-- > 0 && Serial1.available() && Serial0.availableForWrite() > 0) {
      Serial0.write(uint8_t(Serial1.read()));
    }
  } else {
    static CommandLineBuffer<CMD_MAX_LEN> line;
    int remaining = Serial0.available();
    while (remaining-- > 0) {
      int value = Serial0.read();
      if (value < 0) break;
      line.push(static_cast<char>(value), addJsonCmd2Queue);
    }
  } 
}

static unsigned long executeCount = 0;
static unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 5000;
void printExecuteFrequency() {
  executeCount++;
  unsigned long currentTime;
  currentTime = millis();
  if (currentTime - lastPrintTime >= PRINT_INTERVAL) {
    float frequency = (float)executeCount / (PRINT_INTERVAL / 1000.0f);
    usbOutput.println(frequency);
    lastPrintTime = currentTime;
    executeCount = 0;
  }
}


void loop() {
  static unsigned long networkRefresh = 0;
  static String lastNetwork;
  if (millis() - networkRefresh >= 1000) {
    networkRefresh = millis();
    ws.cleanupClients(4);
    String state = wireless.getSTAState();
    String network = state + wireless.getSTAIP() + wireless.getAPIP();
    if (network != lastNetwork) {
      lastNetwork = network;
      screenCtrl.changeSingleLine(2, "AP:" + wireless.getAPIP(), false);
      screenCtrl.changeSingleLine(3, "STA:" + (state == "connected" ? wireless.getSTAIP() : state), true);
    }
  }

  // unsigned long startMicros = micros();
#ifndef TTL_ADAPTER_MACHINE
  serialCtrl();
#endif

  receiveCANasJson();
  // unsigned long endMicros = micros();
  // unsigned long elapsedTime = endMicros - startMicros;
  // usbOutput.println(elapsedTime);

#ifdef UART0_AS_SBUS
  if (sbus.Read()) {
    /* Grab the received data */
    sbusData = sbus.data();
    /* Display the received data */
    for (int8_t i = 0; i < sbusData.NUM_CH; i++) {
      usbOutput.print(sbusData.ch[i]);
      usbOutput.print("\t");
    }
    /* Display lost frames and failsafe data */
    usbOutput.print(sbusData.lost_frame);
    usbOutput.print("\t");
    usbOutput.println(sbusData.failsafe);

    float speed_limit = 1.0;
    if (sbusData.ch[4] < 300) {
      speed_limit = 0.33;
    } else if (sbusData.ch[4] == 1002) {
      speed_limit = 0.66;
    } else if (sbusData.ch[4] > 1700) {
      speed_limit = 1.0;
    }

    float speed_input = constrain(float(sbusData.ch[2] - SBUS_MID)/SBUS_RAN, -1.0, 1.0);
    float turn_input = - constrain(float(sbusData.ch[3] - SBUS_MID)/SBUS_RAN, -1.0, 1.0);

    float left_speed = (speed_input) * speed_limit * 6000.0 - (turn_input * 0.33 * 6000.0);
    float right_speed = (speed_input) * speed_limit * 6000.0 + (turn_input * 0.33 * 6000.0);

    jointsCtrl.hubMotorCtrl(left_speed, -(right_speed), right_speed, left_speed);
  }
#endif

#ifdef USE_ROBOTIC_ARM
  static unsigned long lastFeedbackTime = 0;
  if (jointsCtrl.linkArmFeedbackFlag && (millis() - lastFeedbackTime >= (1000 / jointsCtrl.linkArmFeedbackHz)) && !jointsCtrl.espnowLeader && wireless.getEspNowMode() == 0) {
    lastFeedbackTime = millis();
    memcpy(jointFBRads, jointsCtrl.getJointFBRads(), sizeof(jointFBRads));
    memcpy(jointFBTorques, jointsCtrl.getLinkArmTorqueSC(), sizeof(jointFBTorques));
    jsonFeedback.clear();
    jsonFeedback["T"] = -CMD_GET_JOINT_FB_RADS;
    jsonFeedback["fpv"][0] = jointsCtrl.rbzgIK[0];
    for (int i = 0; i < JOINTS_NUM; i++) {
      jsonFeedback["rads"][i] = String(jointFBRads[i], 3).toDouble();
      jsonFeedback["tors"][i] = jointFBTorques[i];
      jsonFeedback["fpv"][i+1] = jointsCtrl.rbzgIK[i+1];
    }
    serializeJson(jsonFeedback, outputString);
    // ws.textAll(outputString);
    msg(outputString);
  }

  if (jointsCtrl.sbusCtrl && !jointsCtrl.espnowLeader && wireless.getEspNowMode() == 0) {
    jointsCtrl.readSBUS();
    if (jointsCtrl.sbus[8] == SBUS_MAX) {
      jointsCtrl.fpv_r = 260.5;
      jointsCtrl.fpv_b = 0;
      jointsCtrl.fpv_z = 122.38;
      jointsCtrl.fpv_g = 0;
      jointsCtrl.linkArmFPVIK(260.5, 0, 122.38, 0);
    } else if (jointsCtrl.sbus[7] == SBUS_MAX) {
      jointsCtrl.torqueLock(254, 0);
    } else {
      if (jointsCtrl.sbus[0] == 0) {
        jointsCtrl.fpv_z = jointsCtrl.rbzgIK[3] + (SBUS_MID - jointsCtrl.sbus[2])*0.0036;
        jointsCtrl.fpv_b = jointsCtrl.rbzgIK[2] + (jointsCtrl.sbus[1] - SBUS_MID)*0.000018;
        jointsCtrl.fpv_r = jointsCtrl.mapDouble(jointsCtrl.sbus[10], SBUS_MAX, SBUS_MIN, 0, 320);
        if (jointsCtrl.sbus[6] == SBUS_MIN) {
          jointsCtrl.torqueLock(SERVO_ARRAY_3, 0);
        } else if (jointsCtrl.sbus[6] == SBUS_MID) {
          jointsCtrl.fpv_g = GRIPPER_OPEN;
        } else if (jointsCtrl.sbus[6] == SBUS_MAX) {
          jointsCtrl.fpv_g = GRIPPER_CLOSE;
        }

        jointsCtrl.linkArmFPVIK(jointsCtrl.fpv_r, jointsCtrl.fpv_b, jointsCtrl.fpv_z, jointsCtrl.fpv_g);

        float speed_limit = 1.0;
        if (jointsCtrl.sbus[5] == SBUS_MIN) {
          speed_limit = 0.33;
        } else if (jointsCtrl.sbus[5] == SBUS_MID) {
          speed_limit = 0.66;
        } else if (jointsCtrl.sbus[5] == SBUS_MAX) {
          speed_limit = 1.0;
        }

        double speed_input = constrain(float(jointsCtrl.sbus[3] - SBUS_MID)/SBUS_RAN, -1.0, 1.0);
        double turn_input = - constrain(float(jointsCtrl.sbus[4] - SBUS_MID)/SBUS_RAN, -1.0, 1.0);

        int left_speed = (speed_input) * speed_limit * 6000.0 - (turn_input * 0.33 * 6000.0);
        int right_speed = (speed_input) * speed_limit * 6000.0 + (turn_input * 0.33 * 6000.0);

        delay(2);
        jointsCtrl.hubMotorCtrl(left_speed, right_speed, right_speed, left_speed);
        delay(2);
      } else {
        jointsCtrl.hubMotorCtrl(0, 0, 0, 0);
        jointsCtrl.torqueLock(254, 0);
      }
    }
  }

  if (jointsCtrl.espnowLeader && !jointsCtrl.sbusCtrl) {
    // jsonFeedback.clear();
    // jsonFeedback["T"] = CMD_LINK_ARM_SC_JOINTS_CTRL_RAD;
    // jsonFeedback["rad"][0] = jointFBRads[0];
    // jsonFeedback["rad"][1] = jointFBRads[1];
    // jsonFeedback["rad"][2] = jointFBRads[2];
    // jsonFeedback["rad"][3] = jointFBRads[3];
    // wireless.sendEspNowJson(broadcastAddress, jsonFeedback);
    memcpy(jointsRelativesSteps, jointsCtrl.getRelativeSteps(), sizeof(jointsRelativesSteps));
    wireless.sendEspNowSteps(broadcastAddress, jointsRelativesSteps[0], jointsRelativesSteps[1], jointsRelativesSteps[2], jointsRelativesSteps[3]);
  }

  if (!jointsCtrl.sbusCtrl && wireless.getEspNowMode() == 0 && !jointsCtrl.espnowLeader) {
    jointsCtrl.constantCtrlLoop();
  }
#endif


  char cmdBuffer[CMD_MAX_LEN];
  if (xQueueReceive(cmdQueue, cmdBuffer, 0) == pdPASS) {
    JsonDocument jsonCmdReceive;
    DeserializationError err = deserializeJson(jsonCmdReceive, cmdBuffer);
    if (err == DeserializationError::Ok) {
      jsonCmdReceiveHandler(jsonCmdReceive);
      jsonCmdReceive.clear();
    } else {
      msg("JSON parse error");
    }
  }
  
  if(!jointsCtrl.espnowLeader) {
    pushTelemetry();
  }

  // printExecuteFrequency();

  if(wireless.getEspNowMode() != 0) {
    jointsCtrl.relativeStepsCtrl(espNowPacket[0], espNowPacket[1], espNowPacket[2], espNowPacket[3]);
  }

#ifdef CAN_BUS_MACHINE
  canTestMachine();
#endif
#ifdef TTL_ADAPTER_MACHINE
  msg("TTL Adapter Test");
  jointsCtrl.ttlTestMachine();
#endif
  delay(1); // Continuous command/serial traffic must not starve idle tasks.
}

