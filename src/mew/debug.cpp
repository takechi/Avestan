// Debug.cpp

#include "stdafx.h"
#include "private.h"

// #define DEBUG_LOG
// #define DEBUG_MEMLEAK

#ifdef _DEBUG
#include <set>
#ifdef DEBUG_LOG
#include <fstream>
#include <sstream>
#define LOG_DIRECTORY "log"
#define TRACE_LOG "trace.log"
#define MEMLEAK_LOG "memleak.log"
#endif
#ifdef DEBUG_MEMLEAK
#include "std/map.hpp"
#endif
#endif

#ifdef _DEBUG

namespace {

// Assert ignore list
struct SourceInfo {
  PCWSTR file;
  INT line;
  PCWSTR fn;
};
static bool operator<(const SourceInfo& lhs, const SourceInfo& rhs) {
  if (lhs.file < rhs.file) {
    return true;
  } else if (lhs.file > rhs.file) {
    return false;
  } else {
    return lhs.line < rhs.line;
  }
}
static std::set<SourceInfo> ignorelist;

#ifdef DEBUG_MEMLEAK
//==============================================================================
// Memory leak check
class MemoryLeadCheck {
 private:
  CriticalSection m_cs;
  size_t index;
  struct info {
    size_t index;
    char name[256];
  };
  using Objects = std::map<const void*, info>;
  Objects m_objects;

 public:
  MemoryLeadCheck() : index(0) {}
  ~MemoryLeadCheck() {
    m_cs.Lock();
    if (!m_objects.empty()) {
      const char header[] = "\n========= MEMORY LEAK : BEGIN DUMP =========\n";
      const char footer[] = "========= MEMORY LEAK : END DUMP =========\n\n";
      OutputDebugStringA(header);
#ifdef DEBUG_LOG
      ::CreateDirectoryA(LOG_DIRECTORY, null);
      static std::ofstream ofs(LOG_DIRECTORY "\\" MEMLEAK_LOG);
      ofs << header;
      for (Objects::iterator i = m_objects.begin(); i != m_objects.end(); ++i) {
        CHAR buffer[1024];
        _snprintf(buffer, 1024, "0x%08X : [%5d] %s\n", i->first, i->second.index, i->second.name);
        OutputDebugStringA(buffer);
        ofs << buffer;
      }
      ofs << footer;
#else
      for (Objects::iterator i = m_objects.begin(); i != m_objects.end(); ++i) {
        CHAR buffer[1024];
        _snprintf(buffer, 1024, "0x%08X : [%5d] %s\n", i->first, i->second.index, i->second.name);
        OutputDebugStringA(buffer);
      }
#endif
      OutputDebugStringA(footer);
    }
    m_cs.Unlock();
  }
  void add(const void* obj, const char* name) {
    info i;
    i.index = index++;
    str::copy(i.name, name, 256);
    m_cs.Lock();
    m_objects[obj] = i;
    m_cs.Unlock();
  }
  void remove(const void* obj) {
    m_cs.Lock();
    m_objects.erase(obj);
    m_cs.Unlock();
  }
};
static MemoryLeadCheck memchk;

#endif  // DEBUG_MEMLEAK

}  // namespace

namespace mew {
MEW_API void Trace(const string& msg) {
  OutputDebugString(msg.str());
  OutputDebugString(_T("\n"));
#ifdef DEBUG_LOG
  static std::ofstream ofs;
  if (!ofs.is_open()) {
    ::CreateDirectoryA(LOG_DIRECTORY, null);
    ofs.open(LOG_DIRECTORY "\\" TRACE_LOG);
  }
  ofs << CW2A(msg.str()) << std::endl;
#endif
}
MEW_API bool Assert(PCWSTR msg, PCWSTR file, int line, PCWSTR fn) {
  SourceInfo info = {file, line, fn};
  if (ignorelist.find(info) != ignorelist.end()) {
    return true;
  }
  WCHAR buffer[1024];
  _snwprintf(buffer, 1024, L"%s (%d)\n%s()\n%s", file, line, fn, msg);
  switch (::MessageBoxW(null, buffer, L"ASSERTION FAILED", MB_ABORTRETRYIGNORE | MB_ICONERROR)) {
    case IDABORT: {
      Trace(_T("========== ASSERTION FAILED =========="));
      Trace(buffer);
      if (IsDebuggerPresent()) {
        return false;
      } else {
        FatalExit(3);
      }
      break;
    }
    case IDRETRY:
      break;
    case IDIGNORE:
      ignorelist.insert(info);
      break;
  }
  return true;
}
MEW_API void RegisterInstance(IUnknown* obj, const char* name) {
#ifdef DEBUG_MEMLEAK
  memchk.add(obj, name);
#endif
}
MEW_API void UnregisterInstance(IUnknown* obj) {
#ifdef DEBUG_MEMLEAK
  memchk.remove(obj);
#endif
}
}  // namespace mew

#endif  // _DEBUG
