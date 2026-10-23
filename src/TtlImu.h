#pragma once
#include <cmath>
#include <cstdint>

// Register layout and encoding: ftservo/FTServo_Python scservo_sdk/imu.py,
// blob 2028869faa4bb9e52b688cc8e1089a73bd0c91ef.
namespace TtlImu {
constexpr uint8_t Id = 5, Lock = 55, Feedback = 56, FeedbackSize = 23;
inline uint16_t decodeUint16LE(const uint8_t* p) {
  return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}
inline int signedMagnitude(const uint8_t* p) {
  const uint16_t v = decodeUint16LE(p);
  return (v & 0x8000) ? -int(v & 0x7fff) : int(v);
}
inline float halfFloat(const uint8_t* p) {
  const uint16_t v = decodeUint16LE(p);
  const int exponent = (v >> 10) & 31, fraction = v & 1023;
  const float sign = (v & 0x8000) ? -1.f : 1.f;
  if (exponent == 31) return NAN;
  return sign * std::ldexp(float(exponent ? 1024 + fraction : fraction),
                          exponent ? exponent - 25 : -24);
}
}
