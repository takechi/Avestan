// itemid.cpp

#include "stdafx.h"
#include "avesta.hpp"
#include "std/str.hpp"

namespace {

HRESULT HresultFromShellExecute(HINSTANCE hInstance, PCWSTR path, PCWSTR verb, HWND hwnd) {
  if (hInstance > (HINSTANCE)32) {
    return S_OK;
  }
  if (hwnd) {  // hwnd が指定された場合のみエラーを表示
    mew::string msg;
    switch ((int)hInstance) {
      case SE_ERR_FNF:
      case SE_ERR_PNF:
        msg = mew::string::format(L"\"$1\" が見つかりません", path);
        break;
      case SE_ERR_ACCESSDENIED:
      case SE_ERR_SHARE:
        msg = mew::string::format(L"\"$1\" にアクセスできません", path);
        break;
      case SE_ERR_OOM:  // Out of Memory
        msg = L"メモリが不足しているため実行できませんでした";
        break;
      case SE_ERR_DLLNOTFOUND:
        msg = mew::string::format(L"\"$1\" を実行するために必要なDLLが見つかりませんでした", path);
        break;
      case SE_ERR_ASSOCINCOMPLETE:
      case SE_ERR_NOASSOC:
        msg = mew::string::format(L"\"$1\" に対して アクション \"$2\" が関連付けられていません", path, verb);
        break;
      case SE_ERR_DDETIMEOUT:
      case SE_ERR_DDEFAIL:
      case SE_ERR_DDEBUSY:
        msg = mew::string::format(L"DDEエラーが発生したため \"$1\" を実行できませんでした", path);
        break;
      default:
        msg = mew::string::format(L"$1 を実行できませんでした", path);
        break;
    }
    if (msg) {
      ::MessageBox(hwnd, msg.str(), nullptr, MB_OK | MB_ICONERROR);
    }
  }
  return AtlHresultFromLastError();
}

HRESULT DoPathExecute(PCWSTR path, PCWSTR verb, PCWSTR args, PCWSTR dir, HWND hwnd) {
  ASSERT(!mew::str::empty(path));

  // path
  WCHAR quoted[MAX_PATH + 2];
  // exe
  PCWSTR exe = nullptr;
  WCHAR exepath[MAX_PATH];
  // dir
  WCHAR dirpath[MAX_PATH];
  // verb
  WCHAR verbbuf[MAX_PATH] = L"";
  if (!mew::str::empty(verb)) {
    ExpandEnvironmentStrings(verb, verbbuf, MAX_PATH);
    verb = verbbuf;
    if (verb[0] == _T('.')) {  // verb = 拡張子
      if FAILED (avesta::RegGetAssocExe(verb, exepath)) {
        return E_INVALIDARG;  // 関連付けされたEXEを取得できなかった
      }
      exe = exepath;
    } else if (*PathFindExtension(verb) == _T('.')) {  // verb = EXEファイルではない
      exe = verb;
    }
  }

  if (mew::str::empty(dir)) {  // ワーキングディレクトリは、起動するファイルと同じ
    mew::str::copy(dirpath, path);
    PathRemoveFileSpec(dirpath);
    dir = dirpath;
  }

  if (hwnd) {
    hwnd = ::GetAncestor(hwnd, GA_ROOTOWNER);
  }

  HINSTANCE hInstance;
  if (exe) {
    if (mew::str::find(path, L' ')) {
      size_t ln = mew::str::length(path);
      quoted[0] = _T('\"');
      mew::str::copy(quoted + 1, path);
      quoted[ln + 1] = _T('\"');
      quoted[ln + 2] = _T('\0');
      path = quoted;
    }
#if 0  // NOT IMPLEMENTED
  if(args)
    ...;
#endif
    hInstance = ::ShellExecute(hwnd, nullptr, exe, path, dir, SW_SHOWNORMAL);
  } else {
    hInstance = ::ShellExecute(hwnd, verb, path, args, dir, SW_SHOWNORMAL);
  }
  return HresultFromShellExecute(hInstance, path, verb, hwnd);
}

HRESULT DoILExecute(LPCITEMIDLIST pidl, PCWSTR path, PCWSTR verb, PCWSTR args, PCWSTR dir, HWND hwnd) {
  SHELLEXECUTEINFO info = {sizeof(SHELLEXECUTEINFO), SEE_MASK_IDLIST, ::GetAncestor(hwnd, GA_ROOTOWNER)};
  if (!hwnd) {
    info.fMask |= SEE_MASK_FLAG_NO_UI;
  }
  info.nShow = SW_SHOWNORMAL;
  info.lpIDList = (LPITEMIDLIST)pidl;
  info.lpDirectory = dir;
  info.lpVerb = verb;
  info.lpParameters = args;
  TCHAR dirpath[MAX_PATH];
  if (mew::str::empty(dir) && !mew::str::empty(path)) {  // ワーキングディレクトリは、起動するファイルと同じ
    mew::str::copy(dirpath, path, MAX_PATH);
    PathRemoveFileSpec(dirpath);
    info.lpDirectory = dirpath;
  }
  if (ShellExecuteEx(&info)) {
    return S_OK;
  }
  return HresultFromShellExecute(info.hInstApp, path, verb, hwnd);
}
}  // namespace

namespace avesta {
HRESULT ILExecute(LPCITEMIDLIST pidl, PCWSTR verb, PCWSTR args, PCWSTR dir, HWND hwnd) {
  if (!pidl) {
    return E_INVALIDARG;
  }

  bool lazy = GetOption(BoolLazyExecute);
  if (!hwnd) {
    hwnd = GetForm();
  }

  WCHAR path[MAX_PATH] = L"";

  if (SHGetPathFromIDListW(pidl, path) && !mew::str::empty(verb)) {
    HRESULT hr;
    // 失敗した場合にダイアログが出るので、lazy の場合はUIなし (hwnd=null)。
    if SUCCEEDED (hr = DoPathExecute(path, verb, args, dir, (!lazy ? hwnd : nullptr))) {
      return S_OK;
    } else if (!lazy) {
      return hr;
    } else {
      verb = nullptr;  // この動詞では実行できない。
    }
  }

  return DoILExecute(pidl, path, verb, args, dir, hwnd);
}

HRESULT PathExecute(PCWSTR path, PCWSTR verb, PCWSTR args, PCWSTR dir, HWND hwnd) {
  if (mew::str::empty(path)) {
    return E_INVALIDARG;
  }

  bool lazy = GetOption(BoolLazyExecute);
  if (!hwnd) {
    hwnd = GetForm();
  }

  if (!mew::str::empty(verb)) {
    HRESULT hr;
    // 失敗した場合にダイアログが出るので、lazy の場合はUIなし (hwnd=null)。
    if SUCCEEDED (hr = DoPathExecute(path, verb, args, dir, (!lazy ? hwnd : nullptr))) {
      return S_OK;
    } else if (!lazy) {
      return hr;
    } else {
      verb = nullptr;  // この動詞では実行できない。
    }
  }

  return DoPathExecute(path, verb, args, dir, hwnd);
}

}  // namespace avesta