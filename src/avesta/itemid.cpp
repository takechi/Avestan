// itemid.cpp

#include "stdafx.h"
#include "avesta.hpp"
#include "std/str.hpp"

namespace {

HRESULT HresultFromShellExecute(HINSTANCE hInstance, PCWSTR path, PCWSTR verb, HWND hwnd) {
  if (hInstance > (HINSTANCE)32) {
    return S_OK;
  }
  if (hwnd) {  // hwnd ���w�肳�ꂽ�ꍇ�̂݃G���[��\��
    mew::string msg;
    switch ((int)hInstance) {
      case SE_ERR_FNF:
      case SE_ERR_PNF:
        msg = mew::string::format(L"\"$1\" ��������܂���", path);
        break;
      case SE_ERR_ACCESSDENIED:
      case SE_ERR_SHARE:
        msg = mew::string::format(L"\"$1\" �ɃA�N�Z�X�ł��܂���", path);
        break;
      case SE_ERR_OOM:  // Out of Memory
        msg = L"���������s�����Ă��邽�ߎ��s�ł��܂���ł���";
        break;
      case SE_ERR_DLLNOTFOUND:
        msg = mew::string::format(L"\"$1\" �����s���邽�߂ɕK�v��DLL��������܂���ł���", path);
        break;
      case SE_ERR_ASSOCINCOMPLETE:
      case SE_ERR_NOASSOC:
        msg = mew::string::format(L"\"$1\" �ɑ΂��� �A�N�V���� \"$2\" ���֘A�t�����Ă��܂���", path, verb);
        break;
      case SE_ERR_DDETIMEOUT:
      case SE_ERR_DDEFAIL:
      case SE_ERR_DDEBUSY:
        msg = mew::string::format(L"DDE�G���[�������������� \"$1\" �����s�ł��܂���ł���", path);
        break;
      default:
        msg = mew::string::format(L"$1 �����s�ł��܂���ł���", path);
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
    if (verb[0] == _T('.')) {  // verb = �g���q
      if FAILED (avesta::RegGetAssocExe(verb, exepath)) {
        return E_INVALIDARG;  // �֘A�t�����ꂽEXE���擾�ł��Ȃ�����
      }
      exe = exepath;
    } else if (*PathFindExtension(verb) == _T('.')) {  // verb = EXE�t�@�C���ł͂Ȃ�
      exe = verb;
    }
  }

  if (mew::str::empty(dir)) {  // ���[�L���O�f�B���N�g���́A�N������t�@�C���Ɠ���
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
  if (mew::str::empty(dir) && !mew::str::empty(path)) {  // ���[�L���O�f�B���N�g���́A�N������t�@�C���Ɠ���
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
    // ���s�����ꍇ�Ƀ_�C�A���O���o��̂ŁAlazy �̏ꍇ��UI�Ȃ� (hwnd=null)�B
    if SUCCEEDED (hr = DoPathExecute(path, verb, args, dir, (!lazy ? hwnd : nullptr))) {
      return S_OK;
    } else if (!lazy) {
      return hr;
    } else {
      verb = nullptr;  // ���̓����ł͎��s�ł��Ȃ��B
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
    // ���s�����ꍇ�Ƀ_�C�A���O���o��̂ŁAlazy �̏ꍇ��UI�Ȃ� (hwnd=null)�B
    if SUCCEEDED (hr = DoPathExecute(path, verb, args, dir, (!lazy ? hwnd : nullptr))) {
      return S_OK;
    } else if (!lazy) {
      return hr;
    } else {
      verb = nullptr;  // ���̓����ł͎��s�ł��Ȃ��B
    }
  }

  return DoPathExecute(path, verb, args, dir, hwnd);
}

}  // namespace avesta