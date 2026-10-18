#pragma once
#include "FreeRTOS.h"
#define pdTRUE 1
inline SemaphoreHandle_t xSemaphoreCreateMutex() { static int value; return &value; }
inline int xSemaphoreTake(SemaphoreHandle_t, unsigned) { return pdTRUE; }
inline void xSemaphoreGive(SemaphoreHandle_t) {}
inline SemaphoreHandle_t xSemaphoreCreateRecursiveMutex() { static int value; return &value; }
inline bool fakeI2cBusy=false;
inline int xSemaphoreTakeRecursive(SemaphoreHandle_t, unsigned) { return fakeI2cBusy?0:pdTRUE; }
inline void xSemaphoreGiveRecursive(SemaphoreHandle_t) {}
