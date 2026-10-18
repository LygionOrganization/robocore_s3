#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
// Serialize whole OLED frames and IMU snapshots, including clock changes.
inline SemaphoreHandle_t i2cMutex() {
  static SemaphoreHandle_t mutex = xSemaphoreCreateRecursiveMutex();
  configASSERT(mutex);
  return mutex;
}
class I2cGuard {
 public:
  bool acquired;
  I2cGuard() : acquired(xSemaphoreTakeRecursive(i2cMutex(), pdMS_TO_TICKS(20)) == pdTRUE) {}
  ~I2cGuard() { if (acquired) xSemaphoreGiveRecursive(i2cMutex()); }
  I2cGuard(const I2cGuard&) = delete;
  I2cGuard& operator=(const I2cGuard&) = delete;
};
