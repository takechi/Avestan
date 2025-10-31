/// @storage io.hpp
/// 二層ロックストレージ.

#pragma once

#include "std/map.hpp"
#include "string.hpp"

namespace mew {
namespace io {
template <class TBase>
class __declspec(novtable) StreamImpl : public TBase {
 public:
  HRESULT __stdcall Read(void* data, ULONG size, ULONG* done) { return E_NOTIMPL; }
  HRESULT __stdcall Write(const void* data, ULONG size, ULONG* done) { return E_NOTIMPL; }

  HRESULT __stdcall Seek(LARGE_INTEGER where, DWORD whence, ULARGE_INTEGER* pos) {
    UINT64 p;
    switch (whence) {
      case STREAM_SEEK_SET:
        p = where.QuadPart;
        break;
      case STREAM_SEEK_CUR:
        p = GetPosition() + where.QuadPart;
        break;
      case STREAM_SEEK_END:
        p = GetSize() + where.QuadPart;
        break;
      default:
        return E_INVALIDARG;
    }
    HRESULT hr = SeekTo(p);
    if (SUCCEEDED(hr) && pos) pos->QuadPart = p;
    return hr;
  }
  HRESULT __stdcall Stat(STATSTG* stat, DWORD grfStatFlag) {
    if (!stat) return E_INVALIDARG;
    switch (grfStatFlag) {
      case STATFLAG_DEFAULT:
        str::clear(stat->pwcsName);
        break;
      default:
        stat->pwcsName = NULL;
        break;
    }
    FILETIME time;
    CoFileTimeNow(&time);
    stat->type = STGTY_STREAM;
    stat->cbSize.QuadPart = GetSize();
    stat->grfMode = GetAccessMode();
    stat->grfLocksSupported = 0;
    stat->clsid = GUID_NULL;
    stat->mtime = stat->ctime = stat->atime = time;
    return S_OK;
  }

  HRESULT __stdcall CopyTo(IStream* pStream, ULARGE_INTEGER size, ULARGE_INTEGER* read, ULARGE_INTEGER* written) {
    return E_NOTIMPL;
  }
  HRESULT __stdcall Commit(DWORD grfCommitFlags) { return E_NOTIMPL; }
  HRESULT __stdcall Revert() { return E_NOTIMPL; }
  HRESULT __stdcall Clone(IStream** ppClone) { return E_NOTIMPL; }
  HRESULT __stdcall SetSize(ULARGE_INTEGER sz) { return E_NOTIMPL; }
  HRESULT __stdcall LockRegion(ULARGE_INTEGER where, ULARGE_INTEGER size, DWORD dwLockType) { return E_NOTIMPL; }
  HRESULT __stdcall UnlockRegion(ULARGE_INTEGER where, ULARGE_INTEGER size, DWORD dwLockType) { return E_NOTIMPL; }

 protected:
  virtual DWORD GetAccessMode() const = 0;
  virtual UINT64 GetSize() const = 0;
  virtual UINT64 GetPosition() const = 0;
  virtual HRESULT SeekTo(UINT64 where) = 0;
};

template <typename K, typename T>
class StorageT {
 private:
  struct Mapped {
    UINT locks;
    T value;

    Mapped() : locks(0), value() {}

    friend inline IStream& operator>>(IStream& stream, Mapped& v) throw(...) {
      v.locks = 0;
      return stream >> v.value;
    }

    friend inline IStream& operator<<(IStream& stream, const Mapped& v) throw(...) { return stream << v.value; }
  };
  using Map = std::map<K, Mapped>;

 private:
  Map m_map;

 public:
  HRESULT Lock(const K& key, bool exclusive, T** value) {
    ASSERT(value);
    *value = NULL;
    Mapped* mapped;
    if (exclusive) {
      mapped = &m_map[key];
      if (mapped->locks != 0) return STG_E_SHAREVIOLATION;
      mapped->locks = UINT_MAX;
    } else {
      Map::iterator iter = m_map.find(key);
      if (iter == m_map.end()) return STG_E_FILENOTFOUND;
      mapped = &iter->second;
      if (mapped->locks == UINT_MAX) return STG_E_SHAREVIOLATION;
      ++mapped->locks;
    }
    *value = &mapped->value;
    return S_OK;
  }
  HRESULT Unlock(const K& key) {
    Mapped& mapped = m_map[key];
    ASSERT(mapped.locks != 0);
    if (mapped.locks == 0) return E_UNEXPECTED;
    if (mapped.locks == UINT_MAX)
      mapped.locks = 0;
    else
      --mapped.locks;
    return S_OK;
  }
  void Dispose() throw() { m_map.clear(); }
  template <typename Pred>
  void DeleteIf(Pred pred) {
    for (Map::iterator i = m_map.begin(); i != m_map.end();) {
      if (i->second.locks == 0 && pred(i->first, i->second.value))
        i = m_map.erase(i);
      else
        ++i;
    }
  }

  template <typename K, typename T>
  friend inline IStream& operator>>(IStream& stream, StorageT<K, T>& v) throw(...) {
    return stream >> v.m_map;
  }

  template <typename K, typename T>
  friend inline IStream& operator<<(IStream& stream, const StorageT<K, T>& v) throw(...) {
    return stream << v.m_map;
  }
};
}  // namespace io
}  // namespace mew
