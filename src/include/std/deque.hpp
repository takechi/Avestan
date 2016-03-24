/// @file deque.hpp
/// UNDOCUMENTED
#pragma once

#include <deque>
#include "sequence.hpp"

//==============================================================================
// std::deque<T, Alloc>に対するBinary入出力オペレータ

template < typename T, class Alloc >
inline IStream& operator << (IStream& stream, const std::deque<T, Alloc>& v) throw(...)
{
	return mew::Sequence< false >::write(stream, v);
}

template < typename T, class Alloc >
inline IStream& operator >> (IStream& stream, std::deque<T, Alloc>& v) throw(...)
{
	return mew::Sequence< false >::read(stream, v);
}
