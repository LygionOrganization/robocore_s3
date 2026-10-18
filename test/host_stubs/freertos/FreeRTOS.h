#pragma once
#include <cassert>
using SemaphoreHandle_t = int*;
constexpr unsigned portMAX_DELAY = ~0u;
#define configASSERT(x) assert(x)

#define pdMS_TO_TICKS(ms) (ms)
