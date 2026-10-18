#pragma once
#include <cstdint>
#include <cstddef>
#include <initializer_list>
#include <string>
#include <cassert>
using uint8_t = std::uint8_t;
class HardwareSerial {};
inline uint32_t fakeMillis = 0;
inline uint32_t millis() { return fakeMillis; }
inline void delay(unsigned long ms) { fakeMillis += ms; }
class Print {
 public:
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t*, size_t) = 0;
};
class FakeUsb {
 public:
  int space = 64;
  bool connected = true, drain = false;
  std::string output;
  explicit operator bool() const { return connected; }
  int availableForWrite() { if (drain) space = 64; return space; }
  size_t write(const uint8_t* data, size_t count) {
    assert(count <= static_cast<size_t>(space));
    space -= static_cast<int>(count); output.append(reinterpret_cast<const char*>(data), count); return count;
  }
  size_t write(uint8_t byte) { return write(&byte, 1); }
};
inline FakeUsb Serial;

inline void delayMicroseconds(unsigned) {}

// Match the real ESP32 Arduino core: this macro can silently replace local calls.
inline uint16_t makeWord(uint16_t value) { return value; }
inline uint16_t makeWord(uint8_t high, uint8_t low) { return (uint16_t(high)<<8)|low; }
#define word(...) makeWord(__VA_ARGS__)
