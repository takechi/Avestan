// buffer.hpp
#pragma once

#include "math.hpp"

namespace mew {
/// BufferT.
template <class T, class TAlloc = ATL::CCRTAllocator>
class BufferT : public CHeapPtr<T, TAlloc> {
  using super = CHeapPtr<T, TAlloc>;

 private:
  size_t m_length;
  size_t m_capacity;

 public:
  BufferT() noexcept : m_length(0), m_capacity(0) {}
  BufferT(const BufferT& rhs) noexcept : m_length(0), m_capacity(0) { append(rhs, rhs.size()); }
  BufferT& operator=(const BufferT& rhs) noexcept {
    clear();
    append(rhs, rhs.size());
    return *this;
  }

 public:
  T* data() noexcept { return super::m_pData; }
  const T* data() const noexcept { return super::m_pData; }
  void push_back(T c) noexcept {
    reserve(m_length + 1);
    super::m_pData[m_length] = c;
    ++m_length;
  }
  void append(T c) noexcept { push_back(c); }
  void append(const T* data, size_t len) noexcept { write(m_length, data, len); }
  size_t read(size_t pos, T* data, size_t len) {
    if (pos >= m_length) return 0;
    if (pos + len >= m_length) len = m_length - pos;
    memcpy(data, super::m_pData + pos, len * sizeof(T));
    return len;
  }
  void write(size_t pos, const T* data, size_t len) {
    if (len == 0) return;
    if (m_length < pos + len) {
      size_t length = m_length;
      resize(pos + len);
      if (pos > length) {  // 末尾を越えた書き込みの場合、「穴」をゼロで埋める。
        memset(super::m_pData + length, 0, (pos - length) * sizeof(T));
      }
    }
    memcpy(super::m_pData + pos, data, len * sizeof(T));
  }
  bool empty() const noexcept { return m_length == 0; }
  size_t size() const noexcept { return m_length; }
  void resize(size_t sz) noexcept {
    reserve(sz);
    m_length = sz;
  }
  void reserve(size_t capacity) noexcept {
    if (m_capacity >= capacity) return;
    m_capacity = math::max(static_cast<size_t>(16), capacity, m_capacity * 2);
    super::Reallocate(m_capacity);
  }
  void clear() noexcept { m_length = 0; }

  friend inline IStream& operator>>(IStream& stream, BufferT& v) throw(...) {
    size_t size;
    stream >> size;
    v.resize(size);
    io::StreamReadExact(&stream, v, size * sizeof(T));
    return stream;
  }

  friend inline IStream& operator<<(IStream& stream, const BufferT& v) throw(...) {
    stream << v.size();
    io::StreamWriteExact(&stream, v, v.size() * sizeof(T));
    return stream;
  }
};

/// StringBufferT.
template <class T, class TAlloc = ATL::CCRTAllocator>
class StringBufferT : public BufferT<T, TAlloc> {
  using super = BufferT<T, TAlloc>;

 public:
  using super::append;
  using super::data;
  using super::empty;
  using super::resize;
  using super::size;
  const T* str() const { return empty() ? 0 : data(); }
  /// 末端のNULL文字は追加しない.
  void append(const T* str) {
    size_t length = str::length(str);
    size_t sz = size();
    resize(sz + length);
    for (size_t i = 0; i < length; ++i) data()[sz + i] = str[i];
  }
  void append_path(const T* path) {
    if (str::find(path, T(' '))) {  // スペースを含む場合
      append(T('"'));
      append(path);
      append(T('"'));
    } else {  // スペースを含まない場合
      append(path);
    }
  }
  void terminate() { push_back(T('\0')); }
  StringBufferT& operator<<(const T* str) {
    append(str);
    return *this;
  }
  StringBufferT& operator<<(T c) {
    append(c);
    return *this;
  }
};
using StringBuffer = StringBufferT<TCHAR>;
}  // namespace mew
