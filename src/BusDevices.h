#pragma once
#include <ArduinoJson.h>
#include <mutex>
#include <atomic>
#include <lygion_devs.h>
#include <OptionalQMI8658.h>
#include "BusGuard.h"

// New API namespace. Legacy commands stay available.
enum DeviceCommand {
  BUS_SCAN = 700, NODE_ID = 710, NODE_PWM = 711, NODE_RGB = 712, NODE_SBUS = 713,
  STEPPER_ID = 720, STEPPER_MODE = 721, STEPPER_ORIGIN = 722, STEPPER_READ = 723,
  STEPPER_POSITION = 724, STEPPER_SPEED = 725, STEPPER_ENABLE = 726,
  ENCODER_ORIGIN = 730, ENCODER_READ = 732, IMU_READ = 740,
  TTL_IMU_ID = 750, TTL_IMU_READ = 751
};

class BusDevices {
 public:
  OptionalQMI8658 imu;
  void begin(HardwareSerial& port) { node_.pSerial = &port; stepper_.pSerial = &port; ttlImu_.pSerial = &port; }
  bool imuPresent();
  void startImuSampling();
  void pollImu();
  bool handle(const JsonDocument& command, JsonDocument& response);
 private:
  struct ImuSnapshot {
    OptionalQMI8658::Sample sample = {};
    bool present = false, valid = false;
    uint8_t address = 0;
    uint32_t updated = 0;
    const char* error = "imu_not_present";
  };
  ImuSnapshot snapshot_;
  std::mutex snapshotMutex_;
  ImuSnapshot snapshot();
  void publishSnapshot();
  std::atomic<bool> backgroundImu_{false};
  bool cachedImu_ = false;
  OptionalQMI8658::Sample imuCache_ = {};
  uint32_t imuUpdated_ = 0, imuRestarted_ = 0;
  NodeClass node_;
  TTLSDClass stepper_;
  LYSerial ttlImu_{0};
};
