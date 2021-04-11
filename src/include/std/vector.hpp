/// @file vector.hpp
/// UNDOCUMENTED
#pragma once

#pragma warning(disable : 4702)
#include <vector>
#pragma warning(default : 4702)
#include "sequence.hpp"

//==============================================================================
// std::vector<T, Alloc>に対するBinary入出力オペレータ

template <typename T, class Alloc>
inline IStream& operator<<(IStream& stream, const std::vector<T, Alloc>& v) throw(...) {
  return mew::Sequence<mew::IsPOD<T>::value>::write(stream, v);
}

template <typename T, class Alloc>
inline IStream& operator>>(IStream& stream, std::vector<T, Alloc>& v) throw(...) {
  return mew::Sequence<mew::IsPOD<T>::value>::read(stream, v);
}
