// sequence.hpp
#pragma once

#include "mew.hpp"
#include "reference.hpp"

// 前方宣言
IStream& operator<<(IStream& stream, const size_t& size);
IStream& operator>>(IStream& stream, size_t& size);

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
}  // namespace mew
