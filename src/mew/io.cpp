// io.cpp

#include "stdafx.h"
#include "private.h"
#include "io.hpp"
#include "path.hpp"

using namespace mew;
using namespace mew::io;
using namespace mew::io;

void mew::io::StreamReadExact(IStream* stream, void* buffer, size_t size) {
  size_t done = StreamReadSome(stream, buffer, size);
  if (done != size) {
    throw mew::exceptions::IOError(_T(__FUNCTION__), STG_E_READFAULT);
  }
  // throw IOError(string::load(IDS_ERR_READ_LACK), STG_E_READFAULT);
}

size_t mew::io::StreamReadSome(IStream* stream, void* buffer, size_t size) {
  ASSERT(stream);
  ULONG done = 0;
  HRESULT hr = stream->Read(buffer, size, &done);
  if FAILED (hr) {
    throw mew::exceptions::IOError(_T(__FUNCTION__), hr);
  }
  return done;
}

void mew::io::StreamWriteExact(IStream* stream, const void* buffer, size_t size) {
  size_t done = StreamWriteSome(stream, buffer, size);
  if (done != size) {
    throw mew::exceptions::IOError(_T(__FUNCTION__), STG_E_MEDIUMFULL);
  }
  // throw IOError(string::load(IDS_ERR_WRITE_LACK), STG_E_MEDIUMFULL);
}

size_t mew::io::StreamWriteSome(IStream* stream, const void* buffer, size_t size) {
  ASSERT(stream);
  ULONG done = 0;
  HRESULT hr = stream->Write(buffer, size, &done);
  if FAILED (hr) {
    throw mew::exceptions::IOError(_T(__FUNCTION__), hr);
  }
  return done;
}

inline void StreamSeek(IStream* stream, STREAM_SEEK origin, INT64 param, UINT64* newpos) {
  ASSERT(stream);
  HRESULT hr = stream->Seek((LARGE_INTEGER&)param, origin, (ULARGE_INTEGER*)newpos);
  if FAILED (hr) {
    throw mew::exceptions::IOError(_T(__FUNCTION__), hr);
  }
}

void mew::io::StreamSeekAbs(IStream* stream, UINT64 pos, UINT64* newpos) {
  StreamSeek(stream, STREAM_SEEK_SET, (INT64)pos, newpos);
}

void mew::io::StreamSeekRel(IStream* stream, INT64 mov, UINT64* newpos) { StreamSeek(stream, STREAM_SEEK_CUR, mov, newpos); }

void mew::io::StreamReadObject(IStream* stream, REFINTF obj) {
  ASSERT(stream);
  CLSID clsid;
  StreamReadExact(stream, &clsid, sizeof(CLSID));
  if (clsid == GUID_NULL) {
    *obj.pp = null;
  } else {
    mew::CreateInstance(clsid, obj, stream);
  }
}

void mew::io::StreamWriteObject(IStream* stream, IUnknown* obj) {
  ASSERT(stream);
  if (!obj) {
    StreamWriteExact(stream, &GUID_NULL, sizeof(CLSID));
  } else if (ref<ISerializable> serial = cast(obj)) {
    CLSID clsid = serial->Class;
    StreamWriteExact(stream, &clsid, sizeof(CLSID));
    serial->Serialize(*stream);
  } else {
    throw mew::exceptions::CastError(string::load(IDS_ERR_FLATTENABLE, obj));
  }
}

HGLOBAL mew::io::StreamCreateOnHGlobal(IStream** pp, size_t size, bool bDeleteOnRelease) {
  ASSERT(pp);
  HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, size);
  if (!hGlobal) return null;
  if FAILED (::CreateStreamOnHGlobal(hGlobal, bDeleteOnRelease, pp)) {
    ::GlobalFree(hGlobal);
    return null;
  }
  return hGlobal;
}

namespace {
#define CSIDL_ENTRY(name) \
  { L#name, CSIDL_##name }
#define CSIDL_PLURA(name) \
  {L#name, CSIDL_##name##S}, { L#name L"S", CSIDL_##name##S }
#define CSIDL_ALIAS(name, csidl) \
  { L#name, CSIDL_##csidl }
static const struct CSIDL_Map {
  PCWSTR name;
  int csidl;
} CSIDL_MAP[] = {
    CSIDL_PLURA(ADMINTOOL),
    CSIDL_ENTRY(ALTSTARTUP),
    CSIDL_ENTRY(APPDATA),
    CSIDL_ENTRY(BITBUCKET),
    CSIDL_ENTRY(CDBURN_AREA),
    CSIDL_PLURA(COMMON_ADMINTOOL),
    CSIDL_ENTRY(COMMON_ALTSTARTUP),
    CSIDL_ENTRY(COMMON_APPDATA),
    CSIDL_ENTRY(COMMON_DESKTOPDIRECTORY),
    CSIDL_PLURA(COMMON_DOCUMENT),
    CSIDL_PLURA(COMMON_FAVORITE),
    CSIDL_ENTRY(COMMON_MUSIC),
    CSIDL_PLURA(COMMON_PICTURE),
    CSIDL_PLURA(COMMON_PROGRAM),
    CSIDL_ENTRY(COMMON_STARTMENU),
    CSIDL_ENTRY(COMMON_STARTUP),
    CSIDL_PLURA(COMMON_TEMPLATE),
    CSIDL_ENTRY(COMMON_VIDEO),
    CSIDL_ALIAS(CONTROL, CONTROLS),
    CSIDL_ALIAS(CONTROLPANEL, CONTROLS),
    CSIDL_ENTRY(CONTROLS),
    CSIDL_PLURA(COOKIE),
    CSIDL_ENTRY(DESKTOP),
    CSIDL_ENTRY(DESKTOPDIRECTORY),
    CSIDL_PLURA(DRIVE),
    CSIDL_PLURA(FAVORITE),
    CSIDL_PLURA(FONT),
    CSIDL_ENTRY(HISTORY),
    CSIDL_ALIAS(HOME, MYDOCUMENTS),
    CSIDL_ENTRY(INTERNET),
    CSIDL_ENTRY(INTERNET_CACHE),
    CSIDL_ENTRY(LOCAL_APPDATA),
    CSIDL_ALIAS(MYCOMPUTER, DRIVES),
    CSIDL_PLURA(MYDOCUMENT),
    CSIDL_ENTRY(MYMUSIC),
    CSIDL_PLURA(MYPICTURE),
    CSIDL_ENTRY(MYVIDEO),
    CSIDL_ENTRY(NETHOOD),
    CSIDL_ENTRY(NETWORK),
    CSIDL_ENTRY(PERSONAL),
    CSIDL_PLURA(PRINTER),
    CSIDL_ENTRY(PRINTHOOD),
    CSIDL_ENTRY(PROFILE),
    // CSIDL_ENTRY( PROFILES ), // MSDN にはあるものの、ヘッダには書いてない？
    CSIDL_ALIAS(PROGRAM, PROGRAMS),
    CSIDL_PLURA(PROGRAM_FILE),
    CSIDL_ENTRY(PROGRAM_FILES_COMMON),
    CSIDL_ENTRY(PROGRAMS),
    CSIDL_ENTRY(RECENT),
    CSIDL_ALIAS(RECYCLEBIN, BITBUCKET),
    CSIDL_ENTRY(SENDTO),
    CSIDL_ENTRY(STARTMENU),
    CSIDL_ENTRY(STARTUP),
    CSIDL_ENTRY(SYSTEM),
    CSIDL_PLURA(TEMPLATE),
    CSIDL_ALIAS(TRASH, BITBUCKET),
    CSIDL_ENTRY(WINDOWS),
    CSIDL_ALIAS(~, MYDOCUMENTS),
};
#undef CSIDL_ENTRY
#undef CSIDL_ALIAS
static bool operator<(const CSIDL_Map& lhs, PCWSTR rhs) { return _wcsicmp(lhs.name, rhs) < 0; }
static bool operator==(const CSIDL_Map& lhs, PCWSTR rhs) { return _wcsicmp(lhs.name, rhs) == 0; }
#ifdef _DEBUG
struct CSIDLVerifier {
  CSIDLVerifier() {
    const int NUM = sizeof(CSIDL_MAP) / sizeof(CSIDL_MAP[0]);
    for (int i = 0; i < NUM - 1; i++) {
      if (!(CSIDL_MAP[i] < CSIDL_MAP[i + 1].name)) {
        PCWSTR lhs = CSIDL_MAP[i].name;
        PCWSTR rhs = CSIDL_MAP[i + 1].name;
        ASSERT(!"CSIDL_MAP はソート済みでなければならない");
      }
    }
  }
} _verify;
#endif
}  // namespace

std::pair<int, PCWSTR> mew::io::PathResolveCSIDL(PCWSTR src, PCWSTR approot, int appcsidl) {
  WCHAR name[MAX_PATH];
  size_t eaten;
  for (size_t i = 0;; ++i) {
    if (src[i] == L'\0' || src[i] == L'\\' || src[i] == L'/') {
      eaten = i;
      name[i] = L'\0';
      break;
    }
    name[i] = src[i];
  }
  const CSIDL_Map* begin = CSIDL_MAP;
  const CSIDL_Map* end = begin + lengthof(CSIDL_MAP);
  const CSIDL_Map* found = algorithm::binary_search(begin, end, name);
  int csidl;
  if (found != end)
    csidl = found->csidl;
  else if (_wcsicmp(name, approot) == 0)
    csidl = appcsidl;
  else
    return std::pair<int, PCWSTR>(0, 0);
  if (src[eaten] != L'\0') ++eaten;
  return std::pair<int, PCWSTR>(csidl, src + eaten);
}

string mew::io::PathResolvePath(PCWSTR src, PCWSTR approot, int appcsidl) {
  std::pair<int, PCWSTR> result = io::PathResolveCSIDL(src, approot, appcsidl);
  int csidl = result.first;
  PCWSTR next = result.second;
  if (!next) return src;
  io::Path file;
  if (csidl == appcsidl) {
    ::GetModuleFileName(null, file, MAX_PATH);
    file.RemoveLeaf().RemoveLeaf();
  } else {
    SHGetFolderPath(null, csidl, null, SHGFP_TYPE_CURRENT, file);
  }
  if (*next) file.Append(next);
  for (int i = 0; file[i] != L'\0'; ++i) {
    switch (file[i]) {
      case L'/':
        file[i] = L'\\';
        break;
    }
  }
  return string(file);
}

//==============================================================================
// INIファイル.

string mew::io::IniGetString(PCTSTR filename, PCTSTR group, PCTSTR key, PCTSTR defaultValue) {
  TCHAR buffer[MAX_PATH];
  // DefaultNewName
  ::GetPrivateProfileString(group, key, _T(""), buffer, MAX_PATH, filename);
  if (str::empty(buffer))
    return defaultValue;
  else
    return buffer;
}

bool mew::io::IniGetBool(PCTSTR filename, PCTSTR group, PCTSTR key, bool defaultValue) {
  TCHAR buffer[MAX_PATH];
  int len = ::GetPrivateProfileString(group, key, NULL, buffer, MAX_PATH, filename);
  if (len == 0) return defaultValue;
  if (str::equals_nocase(buffer, _T("true")) || str::equals_nocase(buffer, _T("yes"))) return true;
  int value = str::atoi(buffer);
  return value != 0;
}

//==============================================================================

namespace {
static string PathFromArg(IUnknown* arg) {
  if (string s = cast(arg))
    return s;
  else if (ref<IEntry> entry = cast(arg))
    return entry->Path;
  else
    return null;
}
static void CreateFileStream(REFINTF pp, IUnknown* arg, DWORD stgm, UINT nErrorID) throw(...) {
  if (string path = PathFromArg(arg)) {
    ref<IStream> stream;
    HRESULT hr;
    if FAILED (hr = SHCreateStreamOnFile(path.str(), stgm, &stream)) {
      throw mew::exceptions::IOError(string::load(IDS_ERR_OPENFILE, path), hr);
    }
    if FAILED (hr = stream.copyto(pp)) {
      throw mew::exceptions::CastError(string::load(IDS_ERR_NOINTERFACE, stream, pp.iid), hr);
    }
  } else {
    throw mew::exceptions::ArgumentError(string::load(nErrorID));
  }
}
}  // namespace

//==============================================================================

void CreateFileReader(REFINTF pp, IUnknown* arg) throw(...) {
  CreateFileStream(pp, arg, STGM_DEFAULT_READ, IDS_ERR_ARG_FILEINPUTSTREAM);
}

AVESTA_EXPORT_FUNC(FileReader)

//==============================================================================

void CreateFileWriter(REFINTF pp, IUnknown* arg) throw(...) {
  CreateFileStream(pp, arg, STGM_DEFAULT_WRITE, IDS_ERR_ARG_FILEOUTPUTSTREAM);
}

AVESTA_EXPORT_FUNC(FileWriter)

//==============================================================================

void CreateReader(REFINTF pp, IUnknown* arg) throw(...) {
  HRESULT hr;
  if (ref<IStream> stream = cast(arg)) {
    if FAILED (hr = stream.copyto(pp)) {
      throw mew::exceptions::CastError(string::load(IDS_ERR_NOINTERFACE, stream, pp.iid), hr);
    }
  }
  // else if(ref<IBuffer> buffer = cast(arg)) {
  //   CreateMemoryReader(pp, arg);
  // }
  else {
    CreateFileReader(pp, arg);
  }
  ASSERT(*pp.pp);
}

AVESTA_EXPORT_FUNC(Reader)

//==============================================================================

void CreateWriter(REFINTF pp, IUnknown* arg) throw(...) {
  HRESULT hr;
  if (ref<IStream> stream = cast(arg)) {
    if FAILED (hr = stream.copyto(pp)) {
      throw mew::exceptions::CastError(string::load(IDS_ERR_NOINTERFACE, stream, pp.iid), hr);
    }
  }
  // else if(ref<IBuffer> buffer = cast(arg)) {
  //   CreateMemoryWriter(pp, arg);
  // }
  else {
    CreateFileWriter(pp, arg);
  }
  ASSERT(*pp.pp);
}

AVESTA_EXPORT_FUNC(Writer)

//==============================================================================
