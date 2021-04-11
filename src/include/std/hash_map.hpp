/// @file hash_map.hpp
/// UNDOCUMENTED
#pragma once

#include "mew.hpp"
#pragma warning(disable : 4702)
#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
#include <functional>
#include <hash_map>
#pragma warning(default : 4702)

template <>
inline size_t stdext::hash_value(REFGUID key) throw() {
  const size_t* var = reinterpret_cast<const size_t*>(&key);
#ifdef _WIN64
  STATIC_ASSERT(sizeof(size_t) * 2 == sizeof(GUID));
  return var[0] ^ var[1];
#else
  STATIC_ASSERT(sizeof(size_t) * 4 == sizeof(GUID));
  return var[0] ^ var[1] ^ var[2] ^ var[3];
#endif
}

//==============================================================================
// stdext::hash_map<Key, Mapped, Pred, Alloc>に対するBinary入出力オペレータ

template <typename Key, typename Mapped, class Pred, class Alloc>
inline IStream& operator<<(IStream& stream, const stdext::hash_map<Key, Mapped, Pred, Alloc>& v) throw(...) {
  stream << v.size();
  for (stdext::hash_map<Key, Mapped, Pred, Alloc>::const_iterator i = v.begin(); i != v.end(); ++i) {
    stream << i->first << i->second;
  }
  return stream;
}

template <typename Key, typename Mapped, class Pred, class Alloc>
inline IStream& operator>>(IStream& stream, stdext::hash_map<Key, Mapped, Pred, Alloc>& v) throw(...) {
  size_t size;
  stream >> size;
  v.clear();
  for (UINT32 i = 0; i < size; i++) {
    std::pair<Key, Mapped> item;
    stream >> item.first >> item.second;
    v.insert(v.end(), item);
  }
  return stream;
}
