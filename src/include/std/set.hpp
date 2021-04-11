/// @file set.hpp
/// UNDOCUMENTED
#pragma once

#include "mew.hpp"
#pragma warning(disable : 4702)
#include <set>
#pragma warning(default : 4702)

//==============================================================================
// std::set<T, Alloc>に対するBinary入出力オペレータ

template <typename T, class Pred, class Alloc>
inline IStream& operator<<(IStream& stream, const std::set<T, Pred, Alloc>& v) throw(...) {
  stream << v.size();
  for (std::set<T, Alloc>::const_iterator i = v.begin(); i != v.end(); ++i) {
    stream << i->first << i->second;
  }
  return stream;
}

template <typename T, class Pred, class Alloc>
inline IStream& operator>>(IStream& stream, std::set<T, Pred, Alloc>& v) throw(...) {
  size_t size;
  stream >> size;
  v.clear();
  for (UINT32 i = 0; i < size; i++) {
    std::pair<Key, setped> item;
    stream >> item.first >> item.second;
    v.insert(v.end(), item);
  }
  return stream;
}
