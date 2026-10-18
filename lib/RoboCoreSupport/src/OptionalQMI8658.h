#pragma once
#include <Arduino.h>
#include <Wire.h>

// Register definitions follow the QST QMI8658A datasheet (Rev A).
// No hardware is required: begin() returns false after a bounded probe.
class OptionalQMI8658 {
 public:
  struct Sample { float ax, ay, az, gx, gy, gz, temperature; uint32_t sampleCount; };
  bool begin(TwoWire& wire = Wire) {
    wire_ = &wire;
    present_ = false;
    haveSample_ = false;
    error_ = "imu_not_present";
    // The caller owns Wire.begin() so the OLED keeps its pins and bus state.
    for (uint8_t address : {uint8_t(0x6A), uint8_t(0x6B)}) {
      address_ = address;
      uint8_t identity = 0;
      if (!read(0x00, &identity, 1) || identity != 0x05) continue;
      // The IMU can remain powered across ESP32 resets. Clear retained modes
      // and wait past the datasheet's 15 ms reset interval before configuring.
      error_ = "imu_reset_failed";
      if (!write(0x60, 0xB0)) return false;
      delay(30);
      uint8_t resetState = 0;
      if (!read(0x4D, &resetState, 1) || resetState != 0x80) return false;
      // Preserve tested ranges/ODR; select STATUSINT handshake for locked reads.
      error_ = "imu_config_failed";
      const uint8_t config[][2] = {
        {0x08, 0x00}, {0x02, 0x40}, {0x14, 0x00}, {0x09, 0x80},
        {0x03, 0x26}, {0x04, 0x56}, {0x06, 0x00}
      };
      for (const auto& setting : config) {
        uint8_t actual = 0;
        if (!write(setting[0], setting[1]) ||
            !read(setting[0], &actual, 1) || actual != setting[1]) return false;
      }
      // Datasheet Rev A section 13: I2C locking requires AHB clock gating off.
      error_ = "imu_clock_gating_failed";
      if (!write(0x0B, 0x01) || !write(0x0A, 0x12) || !waitStatus(0x80, true, 100) ||
          !write(0x0A, 0x00) || !waitStatus(0x80, false, 100)) return false;
      error_ = "imu_sync_config_failed";
      uint8_t ctrl7 = 0;
      if (!write(0x08, 0x83) || !read(0x08, &ctrl7, 1) || ctrl7 != 0x83) return false;
      delay(250); // Gyroscope startup; only during initialization.
      present_ = true;
      error_ = "";
      return true;
    }
    return false;
  }
  bool present() const { return present_; }
  uint8_t address() const { return address_; }
  const char* error() const { return error_; }
  uint32_t countBefore = 0, countAfter = 0;
  const char* readState = "not_sampled";
  bool sample(Sample& value) {
    if (!present_) return false;
    // SyncSample locks the sensor output until GZ_H (0x40) is read.
    // Unlike timestamp-before/after rejection, this tolerates Wi-Fi scheduling.
    uint8_t data[14], status = 0;
    readState = "i2c_error";
    if (!read(0x2D, &status, 1)) { error_ = "imu_read_failed"; return false; }
    if (!(status & 1)) {
      readState = "no_sample_available"; error_ = "imu_data_not_ready"; return false;
    }
    if (!(status & 2)) {
      delayMicroseconds(12); // ODR code 6: maximum Data_Lock_Delay, section 13.3.
      if (!waitStatus(2, true, 5)) {
        releaseSample(); readState = "lock_timeout"; error_ = "imu_lock_timeout"; return false;
      }
    }
    uint32_t count;
    // Read the counter while locked, then read all data through GZ_H to unlock.
    if (!readCount(count) || !read(0x33, data, sizeof(data))) {
      releaseSample(); error_ = "imu_read_failed"; return false;
    }
    countBefore = countAfter = count;
    lastSample_ = count;
    haveSample_ = true;
    // Avoid Arduino word(...) macro: decode actual signed little-endian bytes.
    auto decodeInt16LE = [&](size_t offset) { return int16_t(uint16_t(data[offset]) | (uint16_t(data[offset + 1]) << 8)); };
    value.temperature = decodeInt16LE(0) / 256.0f;
    value.ax = decodeInt16LE(2) / 4096.0f; value.ay = decodeInt16LE(4) / 4096.0f; value.az = decodeInt16LE(6) / 4096.0f;
    value.gx = decodeInt16LE(8) / 64.0f; value.gy = decodeInt16LE(10) / 64.0f; value.gz = decodeInt16LE(12) / 64.0f;
    value.sampleCount = lastSample_;
    readState = "fresh";
    error_ = "";
    return true;
  }
 private:
  void releaseSample() { uint8_t ignored; read(0x40, &ignored, 1); }
  bool waitStatus(uint8_t mask, bool set, uint32_t timeoutMs) {
    const uint32_t started = millis();
    do {
      uint8_t status;
      if (!read(0x2D, &status, 1)) return false;
      if (bool(status & mask) == set) return true;
      delay(1);
    } while (uint32_t(millis() - started) < timeoutMs);
    return false;
  }
  bool readCount(uint32_t& count) {
    uint8_t bytes[3];
    for (uint8_t i = 0; i < 3; ++i)
      if (!read(0x30 + i, &bytes[i], 1)) return false;
    count = uint32_t(bytes[0]) | (uint32_t(bytes[1]) << 8) | (uint32_t(bytes[2]) << 16);
    return true;
  }
  bool write(uint8_t reg, uint8_t value) {
    wire_->beginTransmission(address_); wire_->write(reg); wire_->write(value);
    return wire_->endTransmission() == 0;
  }
  bool read(uint8_t reg, uint8_t* data, size_t size) {
    wire_->beginTransmission(address_); wire_->write(reg);
    if (wire_->endTransmission(false) != 0) return false;
    if (wire_->requestFrom(address_, size, true) != size) {
      while (wire_->available()) wire_->read();
      return false;
    }
    for (size_t i = 0; i < size; ++i) data[i] = static_cast<uint8_t>(wire_->read());
    return true;
  }
  TwoWire* wire_ = nullptr;
  uint8_t address_ = 0;
  bool present_ = false;
  bool haveSample_ = false;
  uint32_t lastSample_ = 0;
  const char* error_ = "imu_not_present";
};
