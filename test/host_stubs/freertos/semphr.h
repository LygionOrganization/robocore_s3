#pragma once
#include "FreeRTOS.h"
#define pdTRUE 1
inline SemaphoreHandle_t xSemaphoreCreateMutex() { static int value; return &value; }
inline int xSemaphoreTake(SemaphoreHandle_t, unsigned) { return pdTRUE; }
inline void xSemaphoreGive(SemaphoreHandle_t) {}
inline SemaphoreHandle_t xSemaphoreCreateRecursiveMutex() { static int value; return &value; }
inline void xSemaphoreTakeRecursive(SemaphoreHandle_t, unsigned) {}
inline void xSemaphoreGiveRecursive(SemaphoreHandle_t) {}
