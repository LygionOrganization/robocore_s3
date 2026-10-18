#include "UsbOutput.h"
#include <SPI.h>
#include <ESP32-TWAI-CAN.hpp>

// Default for ESP32
#define CAN_TX 14
#define CAN_RX 13

#define CMD_ID   0x100   // A -> B
#define RESP_ID  0x101   // B -> A

CanFrame rxFrame;

bool sendJsonAsCAN(const JsonDocument &docData) {
    // check json

    uint32_t canId = docData["id"].as<uint32_t>();
    bool isExtended = docData["ext"].as<bool>() ? docData["ext"].as<bool>() : false;

    JsonArrayConst dataArray = docData["data"];
    if (dataArray.isNull()) {
        msg("[ERROR] data error");
        return false;
    }

    // send
    twai_message_t frame = {};
    frame.identifier = canId;
    frame.extd = isExtended;
    frame.data_length_code = dataArray.size();
    if (frame.data_length_code > 8) {
        usbOutput.println("[ERROR] data over 8");
        return false;
    }

    for (uint8_t i = 0; i < frame.data_length_code; i++) {
        frame.data[i] = dataArray[i].as<uint8_t>();
    }

    // ESP-IDF API send
    esp_err_t result = twai_transmit(&frame, pdMS_TO_TICKS(100)); // overtime 100ms
    if (result != ESP_OK) {
        usbOutput.printf("[ERROR] CAN send failed, code: %d\n", result);
        return false;
    }

    usbOutput.print("[SEND] ID: 0x");
    usbOutput.print(canId, HEX);
    usbOutput.print(" Data: ");
    for (uint8_t i = 0; i < frame.data_length_code; i++) {
        usbOutput.print(frame.data[i], HEX);
        usbOutput.print(" ");
    }
    usbOutput.println();
    return true;
}

bool sendCANFrame(uint32_t canId, bool isExtended, const uint8_t *data, uint8_t len) {
    if (len > 8) {
        usbOutput.println("[ERROR] data length > 8");
        return false;
    }

    twai_message_t frame = {};
    frame.identifier = canId;
    frame.extd = isExtended;
    frame.data_length_code = len;

    for (uint8_t i = 0; i < len; i++) {
        frame.data[i] = data[i];
    }

    esp_err_t result = twai_transmit(&frame, pdMS_TO_TICKS(100));
    if (result != ESP_OK) {
        usbOutput.printf("[ERROR] CAN send failed, code: %d\n", result);
        return false;
    }

    usbOutput.print("[SEND] ID: 0x");
    usbOutput.print(canId, HEX);
    usbOutput.print(" Data: ");
    for (uint8_t i = 0; i < len; i++) {
        usbOutput.printf("%02X ", frame.data[i]);
    }
    usbOutput.println();

    return true;
}

bool receiveCANasJson() {
    twai_message_t rxFrame;
    // timeout = 0 ticks
    esp_err_t result = twai_receive(&rxFrame, 0); 
    if (result != ESP_OK) {
        return false;
    }

    // Local buffers keep CAN output independent of other producers.
    JsonDocument jsonFeedback;
    String outputString;
    jsonFeedback["T"] = CMD_CAN_SEND;
    jsonFeedback["id"] = rxFrame.identifier;
    jsonFeedback["ext"] = rxFrame.extd;

    JsonArray dataArray = jsonFeedback["data"].to<JsonArray>();
    for (uint8_t i = 0; i < rxFrame.data_length_code; i++) {
        dataArray.add(rxFrame.data[i]);
    }

    serializeJson(jsonFeedback, outputString);
    msg(outputString);
    if (outputString.length() <= 4096 && ws.availableForWriteAll()) ws.textAll(outputString);

    uint8_t resp[1] = {0xAA};
    sendCANFrame(0x101, false, resp, 1);

    return true;
}

void canTestMachine() {
  uint8_t data[1] = {0x55};
  // id, ext, data, len
  sendCANFrame(0x100, false, data, 1);
  delay(1000);
  // twai_message_t rxFrame;
  if (twai_receive(&rxFrame, pdMS_TO_TICKS(500)) == ESP_OK) {
      if (rxFrame.identifier == RESP_ID && rxFrame.data[0] == 0xAA) {
          usbOutput.println("[A] Got response 0xAA, buzzing!");
          buttonBuzzer();
      }
  }
}


void CanStart() {
  // Set pins
  ESP32Can.setPins(CAN_TX, CAN_RX);

  // You can set custom size for the queues - those are default
  ESP32Can.setRxQueueSize(5);
  ESP32Can.setTxQueueSize(5);

  // .setSpeed() and .begin() functions require to use TwaiSpeed enum,
  // but you can easily convert it from numerical value using .convertSpeed()
  ESP32Can.setSpeed(ESP32Can.convertSpeed(500));

  // You can also just use .begin()..
  if (ESP32Can.begin()) {
    msg("CAN bus started!");
  } else {
    msg("CAN bus failed!");
  }

  // or override everything in one command;
  // It is also safe to use .begin() without .end() as it calls it internally
  if (ESP32Can.begin(ESP32Can.convertSpeed(500), CAN_TX, CAN_RX, 10, 10)) {
    msg("CAN bus started!");
  } else {
    msg("CAN bus failed!");
  }
}