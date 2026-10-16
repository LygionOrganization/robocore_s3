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
      delay(20);
      uint8_t resetState = 0;
      if (!read(0x4D, &resetState, 1) || resetState != 0x80) return false;
      // Auto-increment; +/-8 g, +/-512 dps, approximately 112 Hz in 6-DoF.
      error_ = "imu_config_failed";
      // Explicitly exit FIFO mode, including after an MCU-only restart.
      if (!write(0x08, 0x00) || !write(0x02, 0x40) || !write(0x14, 0x00) ||
          !write(0x03, 0x26) || !write(0x04, 0x56) || !write(0x08, 0x03)) return false;
      for (const auto reg : {uint8_t(2), uint8_t(3), uint8_t(4), uint8_t(8), uint8_t(20)}) {
        uint8_t actual = 0;
        const uint8_t expected = reg == 2 ? 0x40 : reg == 3 ? 0x26 : reg == 4 ? 0x56 : reg == 8 ? 3 : 0;
        if (!read(reg, &actual, 1) || actual != expected) return false;
      }
      present_ = true;
      error_ = "";
      return true;
    }
    return false;
  }
  bool present() const { return present_; }
  uint8_t address() const { return address_; }
  const char* error() const { return error_; }
  bool sample(Sample& value) {
    if (!present_) return false;
    // TIMESTAMP is the sensor's 24-bit sample counter, not the host clock.
    // Reject stale samples and snapshots straddling an output update.
    uint8_t frame[17], after[3];
    const auto count = [](const uint8_t* p) { return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16); };
    const unsigned long started = millis();
    bool fresh = false;
    do {
      uint8_t ready = 0;
      if (!read(0x2E, &ready, 1)) { error_ = "imu_read_failed"; return false; }
      if ((ready & 3) == 3) {
        if (!read(0x30, frame, sizeof(frame)) || !read(0x30, after, sizeof(after))) {
          error_ = "imu_read_failed"; return false;
        }
        const uint32_t current = count(frame);
        if (current == count(after) && (!haveSample_ || current != lastSample_)) {
          lastSample_ = current; haveSample_ = true; fresh = true; break;
        }
      }
      delay(2);
    } while (millis() - started < 300);
    if (!fresh) { error_ = "imu_data_timeout"; return false; }
    const uint8_t* data = frame + 3;
    auto word = [&](size_t offset) { return int16_t(uint16_t(data[offset]) | (uint16_t(data[offset + 1]) << 8)); };
    value.temperature = word(0) / 256.0f;
    value.ax = word(2) / 4096.0f; value.ay = word(4) / 4096.0f; value.az = word(6) / 4096.0f;
    value.gx = word(8) / 64.0f; value.gy = word(10) / 64.0f; value.gz = word(12) / 64.0f;
    value.sampleCount = lastSample_;
    error_ = "";
    return true;
  }
 private:
  bool write(uint8_t reg, uint8_t value) {
    wire_->beginTransmission(address_); wire_->write(reg); wire_->write(value);
    return wire_->endTransmission() == 0;
  }
  bool read(uint8_t reg, uint8_t* data, size_t size) {
    wire_->beginTransmission(address_); wire_->write(reg);
    if (wire_->endTransmission(false) != 0) return false;
    if (wire_->requestFrom(address_, size) != size) {
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
