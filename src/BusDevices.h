#pragma once
#include <ArduinoJson.h>
#include <lygion_devs.h>
#include <OptionalQMI8658.h>
#include "BusGuard.h"

// New API namespace. Legacy commands stay available.
enum DeviceCommand {
  BUS_SCAN = 700, NODE_ID = 710, NODE_PWM = 711, NODE_RGB = 712, NODE_SBUS = 713,
  STEPPER_ID = 720, STEPPER_MODE = 721, STEPPER_ORIGIN = 722, STEPPER_READ = 723,
  STEPPER_POSITION = 724, STEPPER_SPEED = 725, STEPPER_ENABLE = 726,
  ENCODER_ORIGIN = 730, ENCODER_READ = 732, IMU_READ = 740
};

class BusDevices {
 public:
  OptionalQMI8658 imu;
  void begin(HardwareSerial& port) { node_.pSerial = &port; stepper_.pSerial = &port; }
  bool handle(const JsonDocument& command, JsonDocument& response);
 private:
  NodeClass node_;
  TTLSDClass stepper_;
};
