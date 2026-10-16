#pragma once
#include <ArduinoJson.h>

// Bound variable-sized servo sync packets before allocating arrays or touching a bus.
inline bool validSyncCommand(const JsonDocument& command) {
  const int t = command["T"] | -1;
  if (t != 16 && t != 17 && t != 26 && t != 27 && t != 36 && t != 37) return true;
  if (!command["idn"].is<int>()) return false;
  const int count = command["idn"];
  if (count < 1 || count > 16) return false;
  auto validArray = [&](const char* key, int low, int high) {
    if (!command[key].is<JsonArrayConst>() || command[key].size() != size_t(count)) return false;
    for (JsonVariantConst item : command[key].as<JsonArrayConst>())
      if (!item.is<int>() || item.as<int>() < low || item.as<int>() > high) return false;
    return true;
  };
  if (!validArray("id", 0, 253) || !validArray("pos", 0, t >= 36 ? 1023 : 4095)) return false;
  for (int i = 0; i < count; ++i)
    for (int j = 0; j < i; ++j)
      if (command["id"][i].as<int>() == command["id"][j].as<int>()) return false;
  if ((t == 16 || t == 26 || t == 36) && !validArray("spd", t == 26 ? 1 : 0, 32767)) return false;
  if ((t == 16 || t == 26) && !validArray("acc", 0, 254)) return false;
  if ((t == 26 || t == 27) && !validArray("tor", 0, 1000)) return false;
  return true;
}
