/// @file struct.hpp
/// 基本構造体.

#pragma once

#pragma warning(push)
#pragma warning(disable : 4458)
#include <gdiplus.h>
#pragma warning(pop)
#include "io.hpp"
#include "math.hpp"
#include "string.hpp"

namespace mew {
//==============================================================================

struct Size;
struct Point;
struct Rect;

//==============================================================================

/// タプル実装用のベースクラス.
/// 自動的に +, -, *, / オペレータが定義される.
template <class TFinal, size_t N>
struct Tuple {
  enum { Dimension = N };
  using out_t = TFinal;
  using in_t = Tuple;

  Tuple() { STATIC_ASSERT(sizeof(TFinal) == sizeof(INT32) * N); }

  operator const INT32*() const { return (const INT32*)static_cast<const TFinal*>(this); }
  operator INT32*() { return (INT32*)static_cast<TFinal*>(this); }

  const INT32& at(size_t index) const { return ((const INT32*)static_cast<const TFinal*>(this))[index]; }
  INT32& at(size_t index) { return ((INT32*)static_cast<TFinal*>(this))[index]; }

  const INT32& operator[](size_t index) const { return at(index); }
  INT32& operator[](size_t index) { return at(index); }

  out_t operator-() const {
    out_t obj;
    for (size_t i = 0; i < N; ++i) obj[i] = -(*this)[i];
    return obj;
  }

#define MEW_TUPLE(rhs, i) rhs[i]
#define MEW_SCALAR(rhs, i) rhs
#define MEW_TUPLE_ARG const in_t&
#define MEW_SCALAR_ARG INT32

#define MEW_OPERATOR(op, fn)                                      \
  out_t& operator op##=(fn##_ARG rhs) {                           \
    for (size_t i = 0; i < N; ++i) (*this)[i] op## = fn(rhs, i);  \
    return *static_cast<out_t*>(this);                            \
  }                                                               \
  friend out_t operator op(const in_t& lhs, fn##_ARG rhs) {       \
    out_t obj;                                                    \
    for (size_t i = 0; i < N; ++i) obj[i] = lhs[i] op fn(rhs, i); \
    return obj;                                                   \
  }

  MEW_OPERATOR(+, MEW_TUPLE)
  MEW_OPERATOR(-, MEW_TUPLE)
  // MEW_OPERATOR(*, MEW_TUPLE)
  // MEW_OPERATOR(/, MEW_TUPLE)
  MEW_OPERATOR(+, MEW_SCALAR)
  MEW_OPERATOR(-, MEW_SCALAR)
  MEW_OPERATOR(*, MEW_SCALAR)
  MEW_OPERATOR(/, MEW_SCALAR)

#undef MEW_OPERATOR

#undef MEW_TUPLE
#undef MEW_SCALAR
#undef MEW_TUPLE_ARG

  friend bool operator==(const in_t& lhs, const in_t& rhs) {
    for (size_t i = 0; i < N; ++i) {
      if (lhs[i] != rhs[i]) return false;
    }
    return true;
  }
  friend bool operator!=(const in_t& lhs, const in_t& rhs) {
    for (size_t i = 0; i < N; ++i) {
      if (lhs[i] != rhs[i]) return true;
    }
    return false;
  }
};

//==============================================================================
/// Size

struct Size : SIZE, Tuple<Size, 2> {
  Size() {}
  Size(INT32 ww, INT32 hh) { assign(ww, hh); }
  void assign(INT32 ww, INT32 hh) {
    cx = ww;
    cy = hh;
  }
  void assign(const Size& sz) { *this = sz; }

  bool empty() const { return cx <= 0 || cy <= 0; }

#ifndef DOXYGEN
  const INT32 get_w() const { return cx; }
  const INT32 get_h() const { return cy; }
  const INT32 set_w(INT32 value) { return cx = value; }
  const INT32 set_h(INT32 value) { return cy = value; }
#endif  // DOXYGEN

  __declspec(property(get = get_w, put = set_w)) INT32 w;  ///< 幅
  __declspec(property(get = get_h, put = set_h)) INT32 h;  ///< 高さ

  static const Size Zero;
};

//==============================================================================
/// Point

struct Point : POINT, Tuple<Point, 2> {
  Point() {}
  Point(INT32 xx, INT32 yy) { assign(xx, yy); }
  void assign(INT32 xx, INT32 yy) {
    x = xx;
    y = yy;
  }
  void assign(const Point& rhs) {
    x = rhs.x;
    y = rhs.y;
  }

  const Point operator*(const Size& rhs) const { return Point(x * rhs.w, y * rhs.h); }
  const Point operator/(const Size& rhs) const { return Point(x / rhs.w, y / rhs.h); }
  friend Point operator+(const Point& lhs, const Size& rhs) { return Point(lhs.x + rhs.w, lhs.y + rhs.h); }
  friend Point operator-(const Point& lhs, const Size& rhs) { return Point(lhs.x - rhs.w, lhs.y - rhs.h); }

  static const Point Zero;  ///< (0, 0).
};

//==============================================================================
/// Rect

struct Rect : RECT {
  //==============================================================================
  /// @name 作成・代入
  //@{
  Rect() {}
  Rect(INT32 l, INT32 t, INT32 r, INT32 b) { assign(l, t, r, b); }
  Rect(const Point& pt, const Size& sz) { assign(pt, sz); }
  void assign(INT32 l, INT32 t, INT32 r, INT32 b) {
    left = l;
    top = t;
    right = r;
    bottom = b;
  }
  void assign(const Point& pt, const Size& sz) {
    location = pt;
    size = sz;
  }
  void assign(const Rect& rhs) { assign(rhs.left, rhs.top, rhs.right, rhs.bottom); }
  explicit Rect(const RECT& rc) { assign(rc.left, rc.top, rc.right, rc.bottom); }
  //@}

  //==============================================================================
  /// @name メソッド
  //@{
  bool empty() const { return w <= 0 || h <= 0; }
  bool contains(INT32 xx, INT32 yy) const { return left <= xx && top <= yy && xx < right && yy < bottom; }
  bool contains(const Point& pt) const { return contains(pt.x, pt.y); }
  bool contains(const Rect& rc) const { return contains(rc.left, rc.top) && contains(rc.right, rc.bottom); }
  Rect& scale(INT32 sx, INT32 sy) {
    left *= sx;
    top *= sy;
    right *= sx;
    bottom *= sy;
    return *this;
  }
  //@}

  //==============================================================================
  /// @name 演算子
  //@{
  /* Rect& operator &= (const Rect& rhs)
          {
                  return *this;
          }
          Rect& operator |= (const Rect& rhs)
          {
                  return *this;
          }
          friend Rect operator & (const Rect& lhs, const Rect& rhs)
          {
                  return Rect();
          }
          friend Rect operator | (const Rect& lhs, const Rect& rhs)
          {
                  return Rect();
          }
  */
  friend bool operator==(const Rect& lhs, const Rect& rhs) { return memcmp(&lhs, &rhs, sizeof(Rect)) == 0; }
  //@}

#ifndef DOXYGEN
  const INT32 get_x() const { return left; }
  const INT32 get_y() const { return top; }
  const INT32 get_w() const { return right - left; }
  const INT32 get_h() const { return bottom - top; }
  const INT32 set_x(INT32 value) {
    right += value - left;
    left = value;
    return value;
  }
  const INT32 set_y(INT32 value) {
    bottom += value - top;
    top = value;
    return value;
  }
  const INT32 set_w(INT32 value) {
    right = left + value;
    return value;
  }
  const INT32 set_h(INT32 value) {
    bottom = top + value;
    return value;
  }

  const Point get_location() const { return Point(left, top); }
  const Size get_size() const { return Size(w, h); }
  const Point get_center() const { return Point((left + right) / 2, (top + bottom) / 2); }
  void set_location(const Point& pt) {
    x = pt.x;
    y = pt.y;
  }
  void set_size(const Size& sz) {
    w = sz.w;
    h = sz.h;
  }
  void set_center(const Point& pt) {
    x = pt.x - w / 2;
    y = pt.y - h / 2;
  }

#endif  // DOXYGEN

  //==============================================================================
  /// @name プロパティ
  //@{
  __declspec(property(get = get_x, put = set_x)) INT32 x;
  __declspec(property(get = get_y, put = set_y)) INT32 y;
  __declspec(property(get = get_w, put = set_w)) INT32 w;
  __declspec(property(get = get_h, put = set_h)) INT32 h;
  __declspec(property(get = get_location, put = set_location)) Point location;
  __declspec(property(get = get_size, put = set_size)) Size size;
  __declspec(property(get = get_center, put = set_center)) Point center;
  //@}

  static const Rect Zero;
};

using Gdiplus::Color;
}  // namespace mew

namespace mew {
__declspec(selectany) const Size Size::Zero(0, 0);
__declspec(selectany) const Point Point::Zero(0, 0);
__declspec(selectany) const Rect Rect::Zero(0, 0, 0, 0);

//==============================================================================

// 文字列への変換(Size).
template <>
class ToString<Size> {
 private:
  WCHAR m_str[80];

 public:
  ToString(const Size& value) throw() { swprintf(m_str, L"%d %d", value.w, value.h); }
  operator PCWSTR() const throw() { return m_str; }
};

// 文字列への変換(Point).
template <>
class ToString<Point> {
 private:
  WCHAR m_str[80];

 public:
  ToString(const Point& value) throw() { swprintf(m_str, L"%d %d", value.x, value.y); }
  operator PCWSTR() const throw() { return m_str; }
};

// 文字列への変換(Rect).
template <>
class ToString<Rect> {
 private:
  WCHAR m_str[160];

 public:
  ToString(const Rect& value) throw() { swprintf(m_str, L"%d %d %d %d", value.left, value.top, value.right, value.bottom); }
  operator PCWSTR() const throw() { return m_str; }
};

// 文字列への変換(Color).
template <>
class ToString<Color> {
 private:
  WCHAR m_str[10];  // "#RRGGBBAA" : 9 chars
 public:
  ToString(const Color& value) throw() {
    swprintf(m_str, L"#%02X%02X%02X%02X", value.GetR(), value.GetG(), value.GetB(), value.GetA());
  }
  operator PCWSTR() const throw() { return m_str; }
};
}  // namespace mew

//==============================================================================

AVESTA_POD(mew::Size)
AVESTA_POD(mew::Point)
AVESTA_POD(mew::Rect)
AVESTA_POD(mew::Color)
