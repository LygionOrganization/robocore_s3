#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// All application USB writers share this adapter. Never ask USBCDC::write
// for more than its free FIFO space: some cores otherwise spin indefinitely.
// Congested output is expendable; a newline resynchronizes the next record.
class UsbOutput : public Print {
 public:
  size_t write(uint8_t byte) override { return write(&byte, 1); }
  size_t write(const uint8_t* bytes, size_t size) override {
    if (!lock_ || xSemaphoreTake(lock_, 0) != pdTRUE) return 0;
    size_t sent = 0;
    const uint32_t started = millis();
    while (sent < size && Serial) {
      int available = Serial.availableForWrite();
      if (resync_ && available > 0) {
        Serial.write(static_cast<uint8_t>('\n'));
        --available;
        resync_ = false;
      }
      if (available > 0) {
        size_t count = size - sent < static_cast<size_t>(available) ? size - sent : available;
        sent += Serial.write(bytes + sent, count);
      }
      if (sent == size || static_cast<uint32_t>(millis() - started) >= 10) break;
      delay(1);
    }
    if (sent < size) resync_ = true;
    xSemaphoreGive(lock_);
    return sent;
  }
 private:
  SemaphoreHandle_t lock_ = xSemaphoreCreateMutex();
  bool resync_ = false;
};
inline UsbOutput usbOutput;
