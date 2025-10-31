// registry.cpp

#include "stdafx.h"
#include "avesta.hpp"
#include "std/str.hpp"

// レジストリ操作関数.

namespace {

HRESULT RegGetAssocCommand(PCWSTR key, WCHAR command[MAX_PATH]) {
  HRESULT hr;
  TCHAR def[MAX_PATH], buffer[MAX_PATH];
  if (mew::str::empty(key)) {
    return AtlHresultFromWin32(ERROR_REGISTRY_IO_FAILED);
  }
  // デフォルトコマンドを取得
  wsprintf(buffer, _T("%s\\shell"), key);
  if (FAILED(avesta::RegGetString(HKEY_CLASSES_ROOT, buffer, NULL, def)) || mew::str::empty(def)) {
    lstrcpy(def, _T("open"));
  }
  // コマンドの内容を取得
  wsprintf(buffer, _T("%s\\shell\\%s\\command"), key, def);
  if FAILED (hr = avesta::RegGetString(HKEY_CLASSES_ROOT, buffer, NULL, command)) {
    return hr;
  }
  if (mew::str::empty(command)) {
    return AtlHresultFromWin32(ERROR_REGISTRY_IO_FAILED);
  }
  return S_OK;
}
}  // namespace

namespace avesta {
HRESULT RegGetDWORD(HKEY hKey, PCWSTR subkey, PCWSTR value, DWORD* outDWORD) {
  if (!outDWORD) {
    return E_POINTER;
  }
  DWORD type = REG_DWORD;
  DWORD bufSize = sizeof(DWORD);
  DWORD dwResult = SHGetValue(hKey, subkey, value, &type, &outDWORD, &bufSize);
  if (dwResult == ERROR_SUCCESS) {
    return S_OK;
  } else {
    return AtlHresultFromWin32(dwResult);
  }
}

HRESULT RegSetDWORD(HKEY hKey, PCWSTR subkey, PCWSTR value, DWORD inDWORD) {
  DWORD dwResult = SHSetValue(hKey, subkey, value, REG_DWORD, &inDWORD, sizeof(DWORD));
  if (dwResult == ERROR_SUCCESS) {
    return S_OK;
  } else {
    return AtlHresultFromWin32(dwResult);
  }
}

HRESULT RegGetString(HKEY hKey, PCWSTR subkey, PCWSTR value, WCHAR outString[], size_t bufsize) {
  if (!outString) {
    return E_POINTER;
  }
  DWORD type = REG_SZ;
  DWORD bufSize = sizeof(WCHAR) * bufsize;
  DWORD dwResult = SHGetValueW(hKey, subkey, value, &type, outString, &bufSize);
  if (dwResult == ERROR_SUCCESS) {
    return S_OK;
  } else {
    return AtlHresultFromWin32(dwResult);
  }
}

// どちらも存在するファイルでないとエラーになるので使えない！
// if(FindExecutable(s, NULL, exe) > (HINSTANCE)32)
// if SUCCEEDED(hr = AssocQueryString(ASSOCF_OPEN_BYEXENAME | ASSOCF_NOTRUNCATE, ASSOCSTR_EXECUTABLE, s, _T("open"), exe,
//&ln))
HRESULT RegGetAssocExe(PCWSTR extension, WCHAR exe[MAX_PATH]) {
  HRESULT hr;
  TCHAR key[MAX_PATH], command[MAX_PATH];

  if FAILED (RegGetAssocCommand(extension, command)) {
    if (FAILED(hr = RegGetString(HKEY_CLASSES_ROOT, extension, NULL, key)) || FAILED(hr = RegGetAssocCommand(key, command))) {
      return hr;
    }
  }

  // 大抵、'EXE "%1"' だろうので、はじめのエントリだけを取得する
  PCWSTR src = command;
  TCHAR delim = _T(' ');
  if (src[0] == _T('"')) {
    ++src;
    delim = _T('\"');
  }
  PWSTR dst = exe;
  while (*src != L'\0' && *src != delim) {
    *dst++ = *src++;
  }
  *dst = L'\0';
  return S_OK;
}

}  // namespace avesta
