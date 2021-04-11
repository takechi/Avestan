// version.cpp

#include "stdafx.h"
#include "io.hpp"
#include "shell.hpp"
#include "private.h"

#pragma comment(lib, "version.lib")

using namespace mew::io;

//======================================================================
// Version

Version::Version() : m_VersionInfo(null) {}
Version::~Version() { Close(); }
string Version::GetPath() const { return m_Path; }
bool Version::Open(string filename) {
  m_Path = filename;
  Close();
  DWORD dummy;
  DWORD versionInfoSize = ::GetFileVersionInfoSize((PTSTR)filename.str(), &dummy);
  m_VersionInfo = new BYTE[versionInfoSize];
  struct LangCodePage {
    WORD Language;
    WORD CodePage;
  };
  UINT size;
  VS_FIXEDFILEINFO* info;
  if (::GetFileVersionInfo((PTSTR)filename.str(), 0, versionInfoSize, m_VersionInfo) &&
      ::VerQueryValue(m_VersionInfo, _T("\\"), (void**)&info, &size)) {
    LangCodePage* lang;
    ::VerQueryValue(m_VersionInfo, _T("\\VarFileInfo\\Translation"), (void**)&lang, &size);
    m_Language = lang->Language;
    m_CodePage = lang->CodePage;
  } else {
    delete[] m_VersionInfo;
    return false;
  }
  return true;
}
void Version::Close() {
  delete[] m_VersionInfo;
  m_VersionInfo = NULL;
}
PCTSTR Version::QueryValue(PCTSTR what) {
  PCTSTR value = NULL;
  UINT size;
  TCHAR query[256];
  wsprintf(query, _T("\\StringFileInfo\\%04x%04x\\%s"), m_Language, m_CodePage, what);
  BOOL res = ::VerQueryValue(m_VersionInfo, query, (void**)&value, &size);
  if (!res) return null;
  return value;
}
