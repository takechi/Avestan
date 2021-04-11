/// @file list.hpp
/// UNDOCUMENTED
#pragma once

#pragma warning(disable : 4702)
#include <list>
#pragma warning(default : 4702)
#include "sequence.hpp"

//==============================================================================
// std::list<T, Alloc>に対するBinary入出力オペレータ

template <typename T, class Alloc>
inline IStream& operator<<(IStream& stream, const std::list<T, Alloc>& v) throw(...) {
  return mew::Sequence<false>::write(stream, v);
}

template <typename T, class Alloc>
inline IStream& operator>>(IStream& stream, std::list<T, Alloc>& v) throw(...) {
  return mew::Sequence<false>::read(stream, v);
}
