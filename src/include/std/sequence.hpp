// sequence.hpp
#pragma once

#include "mew.hpp"

namespace mew {
template <bool IsArrayOfPOD = false>
struct Sequence {
  template <typename TSequence>
  static IStream& write(IStream& stream, const TSequence& v) {
    size_t size = v.size();
    stream << size;
    for (typename TSequence::const_iterator i = v.begin(); i != v.end(); ++i) {
      stream << *i;
    }
    return stream;
  }
  template <typename TSequence>
  static IStream& read(IStream& stream, TSequence& v) {
    size_t size;
    stream >> size;
    v.clear();
    v.resize(size);
    for (typename TSequence::iterator i = v.begin(); i != v.end(); ++i) {
      stream >> *i;
    }
    return stream;
  }
};

template <>
struct Sequence<true> {
  template <typename TSequence>
  static IStream& write(IStream& stream, const TSequence& v) {
    size_t size = v.size();
    stream << size;
    if (size > 0) io::StreamWrite(stream, &v[0], sizeof(typename TSequence::value_type) * size);
    return stream;
  }
  template <typename TSequence>
  static IStream& read(IStream& stream, TSequence& v) {
    size_t size;
    stream >> size;
    v.clear();
    if (size > 0) {
      v.resize(size);
      io::StreamRead(stream, &v[0], sizeof(typename TSequence::value_type) * size);
    }
    return stream;
  }
};
}  // namespace mew
