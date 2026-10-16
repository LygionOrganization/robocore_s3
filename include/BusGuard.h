#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// One physical half-duplex bus, shared by old servo code and new peripherals.
// Recursive because some JointsCtrl operations call other JointsCtrl methods.
inline SemaphoreHandle_t busMutex() {
  static SemaphoreHandle_t mutex = xSemaphoreCreateRecursiveMutex();
  configASSERT(mutex);
  return mutex;
}
class BusGuard {
 public:
  BusGuard() { xSemaphoreTakeRecursive(busMutex(), portMAX_DELAY); }
  ~BusGuard() { xSemaphoreGiveRecursive(busMutex()); }
  BusGuard(const BusGuard&) = delete;
  BusGuard& operator=(const BusGuard&) = delete;
};
