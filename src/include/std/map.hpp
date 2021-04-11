/// @file map.hpp
/// UNDOCUMENTED
#pragma once

#include "mew.hpp"
#pragma warning(disable : 4702)
#include <map>
#pragma warning(default : 4702)

//==============================================================================
// std::map<Key, Mapped, Pred, Alloc>に対するBinary入出力オペレータ

template <typename Key, typename Mapped, class Pred, class Alloc>
inline IStream& operator<<(IStream& stream, const std::map<Key, Mapped, Pred, Alloc>& v) throw(...) {
  stream << v.size();
  for (std::map<Key, Mapped, Pred, Alloc>::const_iterator i = v.begin(); i != v.end(); ++i) {
    stream << i->first << i->second;
  }
  return stream;
}

template <typename Key, typename Mapped, class Pred, class Alloc>
inline IStream& operator>>(IStream& stream, std::map<Key, Mapped, Pred, Alloc>& v) throw(...) {
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
