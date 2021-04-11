/// @file math.hpp
/// êîäw.
#pragma once

#include <cmath>

#include "mew.hpp"

//==============================================================================

#undef min
#undef max

namespace mew {
/// êîäwä÷êî.
namespace math {
//==============================================================================
//

const REAL32 E = 2.7182818284590452354f;    ///< é©ëRëŒêîÇÃíÍ(REAL32).
const REAL32 PI = 3.14159265358979323846f;  ///< â~é¸ó¶(REAL32).
                                            // const REAL64 E = 2.7182818284590452354; ///< é©ëRëŒêîÇÃíÍ(REAL64).
                                            // const REAL64 PI = 3.14159265358979323846; ///< â~é¸ó¶(REAL64).

template <typename T>
T abs(T x) {
  return x < T(0) ? -x : x;
}
template <typename T>
T max(T a, T b) {
  return a > b ? a : b;
}
template <typename T>
T max(T a, T b, T c) {
  return max(max(a, b), c);
}
template <typename T>
T min(T a, T b) {
  return a < b ? a : b;
}
template <typename T>
T min(T a, T b, T c) {
  return min(min(a, b), c);
}
template <typename T>
T clamp(T v, T min_, T max_) {
  return min(max(v, min_), max_);
}

template <typename T>
T mod(T x, T y) {
  return x % y;
}
template <typename T>
T sqr(T x) {
  return x * x;
}
template <typename T>
T hypot2(T x, T y) {
  return x * x + y * y;
}

//==============================================================================

template <>
inline int abs<int>(int x) {
  return std::abs(x);
}
template <>
inline long abs<long>(long x) {
  return std::abs(x);
}
template <>
inline REAL32 abs<REAL32>(REAL32 x) {
  return std::fabsf(x);
}
template <>
inline REAL64 abs<REAL64>(REAL64 x) {
  return std::fabs(x);
}
template <>
inline __int64 abs<__int64>(__int64 x) {
  return _abs64(x);
}

template <>
inline REAL32 mod<REAL32>(REAL32 x, REAL32 y) {
  return std::fmodf(x, y);
}
template <>
inline REAL64 mod<REAL64>(REAL64 x, REAL64 y) {
  return std::fmod(x, y);
}

inline REAL32 ceil(REAL32 x) { return std::ceilf(x); }
inline REAL64 ceil(REAL64 x) { return std::ceil(x); }
inline REAL32 floor(REAL32 x) { return std::floorf(x); }
inline REAL64 floor(REAL64 x) { return std::floor(x); }
inline REAL32 sqrt(REAL32 x) { return std::sqrtf(x); }
inline REAL64 sqrt(REAL64 x) { return std::sqrt(x); }
inline REAL32 exp(REAL32 x) { return std::expf(x); }
inline REAL64 exp(REAL64 x) { return std::exp(x); }
inline REAL32 log(REAL32 x) { return std::logf(x); }
inline REAL64 log(REAL64 x) { return std::log(x); }
inline REAL32 log10(REAL32 x) { return std::log10f(x); }
inline REAL64 log10(REAL64 x) { return std::log10(x); }
inline REAL32 pow(REAL32 x, REAL32 y) { return std::powf(x, y); }
inline REAL64 pow(REAL64 x, REAL64 y) { return std::pow(x, y); }
inline REAL32 hypot(REAL32 x, REAL32 y) { return (REAL32)_hypot(x, y); }
inline REAL64 hypot(REAL64 x, REAL64 y) { return _hypot(x, y); }

inline REAL32 sin(REAL32 x) { return std::sinf(x); }
inline REAL64 sin(REAL64 x) { return std::sin(x); }
inline REAL32 cos(REAL32 x) { return std::cosf(x); }
inline REAL64 cos(REAL64 x) { return std::cos(x); }
inline REAL32 tan(REAL32 x) { return std::tanf(x); }
inline REAL64 tan(REAL64 x) { return std::tan(x); }

inline REAL32 asin(REAL32 x) { return std::asinf(x); }
inline REAL64 asin(REAL64 x) { return std::asin(x); }
inline REAL32 acos(REAL32 x) { return std::acosf(x); }
inline REAL64 acos(REAL64 x) { return std::acos(x); }
inline REAL32 atan(REAL32 x) { return std::atanf(x); }
inline REAL64 atan(REAL64 x) { return std::atan(x); }
inline REAL32 atan2(REAL32 y, REAL32 x) { return std::atan2f(y, x); }
inline REAL64 atan2(REAL64 y, REAL64 x) { return std::atan2(y, x); }

inline REAL32 toDegree(REAL32 radian) { return radian * 180.0f / PI; }
inline REAL64 toDegree(REAL64 radian) { return radian * 180.0 / PI; }
inline REAL32 toRadian(REAL32 degree) { return degree / 180.0f * PI; }
inline REAL64 toRadian(REAL64 degree) { return degree / 180.0 * PI; }
}  // namespace math
}  // namespace mew
