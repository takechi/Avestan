// file.cpp

#include "stdafx.h"
#include "avesta.hpp"
#include "std/str.hpp"
#include "std/buffer.hpp"
#include "io.hpp"

namespace {
bool ChoiceBetter(LPTSTR bestfile, PCWSTR candidate) {
  bool better = false;
  if (mew::str::empty(bestfile)) {
    better = true;
  } else {
    int i;
    for (i = 0; bestfile[i] != _T('\0') && candidate[i] != _T('\0') &&
                mew::str::tolower(bestfile[i]) == mew::str::tolower(candidate[i]);
         ++i) {
    }
    better = (mew::str::atoi(bestfile + i) < mew::str::atoi(candidate + i));
  }
  //
  if (better) {
    mew::str::copy(bestfile, candidate);
    return true;
  }
  return false;
}

HRESULT FileNewShell(PCWSTR path, PCWSTR ext, PCWSTR templates) {
  TCHAR reg[MAX_PATH], srcfile[MAX_PATH] = _T("");
  lstrcpy(reg, ext);
  lstrcat(reg, _T("\\ShellNew"));
  // レジストリの単純ShellNewエントリを探す
  if SUCCEEDED (avesta::RegGetString(HKEY_CLASSES_ROOT, reg, _T("FileName"), srcfile)) {
    if (PathIsRelative(srcfile)) {
      mew::str::prepend(srcfile, templates);
    }
    if (PathFileExists(srcfile)) {
      return avesta::FileDup(srcfile, path);
    }
  }
  return E_FAIL;
}

HRESULT FileNewTemplate(PCWSTR path, PCWSTR ext, PCWSTR templates) {
  TCHAR wildcard[MAX_PATH], srcfile[MAX_PATH] = _T("");
  WIN32_FIND_DATA find;
  lstrcpy(wildcard, templates);
  lstrcat(wildcard, _T("*"));
  lstrcat(wildcard, ext);
  HANDLE hFind = ::FindFirstFile(wildcard, &find);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      ChoiceBetter(srcfile, find.cFileName);
    } while (::FindNextFile(hFind, &find));
    ::FindClose(hFind);
    if (!mew::str::empty(srcfile)) {
      mew::str::prepend(srcfile, templates);
      return avesta::FileDup(srcfile, path);
    }
  }
  return E_FAIL;
}

HRESULT FileNewNull(PCWSTR path) {
  HANDLE hFile = ::CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, 0, 0);
  if (hFile == INVALID_HANDLE_VALUE) {
    return AtlHresultFromLastError();
  }
  ::SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, path, NULL);
  ::CloseHandle(hFile);
  return S_OK;
}

HRESULT DoFileOperation(PCWSTR src, PCWSTR dst, WORD fo, FILEOP_FLAGS flags, HWND hwnd = avesta::GetForm()) {
  SHFILEOPSTRUCT op = {hwnd};
  op.wFunc = fo;
  op.pFrom = src;
  op.pTo = dst;
  op.fFlags = flags;
  if (SHFileOperation(&op) != 0) {
    return E_FAIL;
  }
  return op.fAnyOperationsAborted ? S_FALSE : S_OK;
}

static bool IsCopy() {
  static const UINT CF_PREFERREDDROPEFFECT = RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
  HGLOBAL hDropEffect = ::GetClipboardData(CF_PREFERREDDROPEFFECT);
  DWORD* pdw = reinterpret_cast<DWORD*>(GlobalLock(hDropEffect));
  ASSERT(pdw);
  DWORD dwEffect = (pdw ? *pdw : DROPEFFECT_COPY);
  GlobalUnlock(hDropEffect);
  return (dwEffect & DROPEFFECT_COPY) != 0;
}

HRESULT FileCutOrCopy(PCTSTR srcSingle, DWORD dwDropEffect) {
  if (!srcSingle) {
    return E_INVALIDARG;
  }
  size_t len = lstrlen(srcSingle);
  if (len == 0) {
    return E_INVALIDARG;
  }

  // CF_HDROPを作成
  DROPFILES dropfiles = {sizeof(DROPFILES), 0, 0, true, IS_UNICODE_CHARSET};
  mew::Stream stream;
  HGLOBAL hDrop = mew::io::StreamCreateOnHGlobal(&stream, sizeof(DROPFILES) + ((len + 1) * 1 + 1) * sizeof(TCHAR), false);
  stream.write(&dropfiles, sizeof(DROPFILES));
  stream.write(srcSingle, (len + 1) * sizeof(TCHAR));
  stream.write(_T("\0"), sizeof(TCHAR));
  stream.clear();

  // Preferred DropEffectを作成
  HGLOBAL hDropEffect = GlobalAlloc(GMEM_MOVEABLE, sizeof(DWORD));
  DWORD* pdw = static_cast<DWORD*>(GlobalLock(hDropEffect));
  *pdw = dwDropEffect;
  GlobalUnlock(hDropEffect);

  // クリップボードにデーターをセット
  HWND hwnd = avesta::GetForm();
  UINT CF_PREFERREDDROPEFFECT = RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
  if (!::OpenClipboard(hwnd)) {
    ::GlobalFree(hDrop);
    ::GlobalFree(hDropEffect);
    return AtlHresultFromLastError();
  }
  ::EmptyClipboard();
  ::SetClipboardData(CF_HDROP, hDrop);
  ::SetClipboardData(CF_PREFERREDDROPEFFECT, hDropEffect);
  ::CloseClipboard();
  return S_OK;
}
}  // namespace

namespace avesta {

HRESULT FileNew(PCWSTR path) {
  PCWSTR ext = PathFindExtension(path);
  if (*ext == _T('.')) {
    TCHAR templates[MAX_PATH];
    SHGetSpecialFolderPath(NULL, templates, CSIDL_TEMPLATES, FALSE);
    PathAddBackslash(templates);
    // まずは単純ShellNewエントリを参照する。
    if SUCCEEDED (FileNewShell(path, ext, templates)) {
      return S_OK;
    }
    // 次にCSIDL_TEMPLATESフォルダの同じ拡張子を捜す
    if SUCCEEDED (FileNewTemplate(path, ext, templates)) {
      return S_OK;
    }
  }
  // テンプレートファイルが無い
  return FileNewNull(path);
}

HRESULT FileDup(PCWSTR src, PCWSTR dst) { return DoFileOperation(src, dst, FO_COPY, FOF_ALLOWUNDO); }

HRESULT FileMove(PCWSTR src, PCWSTR dst) { return DoFileOperation(src, dst, FO_MOVE, FOF_ALLOWUNDO); }

HRESULT FileDelete(PCWSTR src) { return DoFileOperation(src, nullptr, FO_DELETE, FOF_WANTNUKEWARNING | FOF_ALLOWUNDO); }

HRESULT FileBury(PCWSTR src) { return DoFileOperation(src, nullptr, FO_DELETE, FOF_WANTNUKEWARNING); }

HRESULT FileRename(PCWSTR src, PCWSTR dst) {
  if (lstrcmp(src, dst) == 0) {  // 同じ名前なので変更する必要が無い
    return true;
  }
  return DoFileOperation(src, dst, FO_RENAME, FOF_FILESONLY | FOF_ALLOWUNDO);
}

HRESULT FilePaste(PCTSTR dst) {
  HWND hwnd = GetForm();
  if (::IsClipboardFormatAvailable(CF_HDROP) && ::PathIsDirectory(dst) && ::OpenClipboard(hwnd)) {
    if (HDROP hDrop = (HDROP)::GetClipboardData(CF_HDROP)) {
      bool copy = IsCopy();

      mew::StringBuffer srcfiles;
      const UINT count = ::DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
      for (UINT i = 0; i < count; ++i) {
        TCHAR path[MAX_PATH];
        ::DragQueryFile(hDrop, i, path, MAX_PATH);
        srcfiles.append(path, mew::str::length(path) + 1);
      }

      if (!copy) {
        ::EmptyClipboard();
      }
      ::CloseClipboard();

      if (!srcfiles.empty()) {
        srcfiles.push_back(_T('\0'));
        return DoFileOperation(srcfiles, dst, (copy ? FO_COPY : FO_MOVE), FOF_ALLOWUNDO, hwnd);
      }
      return S_OK;
    }
    ::CloseClipboard();
  }
  return AtlHresultFromLastError();
}

HRESULT FileCut(PCWSTR src) { return FileCutOrCopy(src, DROPEFFECT_MOVE); }

HRESULT FileCopy(PCWSTR src) { return FileCutOrCopy(src, DROPEFFECT_COPY | DROPEFFECT_LINK); }

}  // namespace avesta

#ifdef AVESTA_PYTHON_INTERFACE

namespace {
HRESULT DoFileOperation(Object* src, PCWSTR dst, WORD fo, FILEOP_FLAGS flags) {
  size_t len = DoubleNullTextLength(src);
  if (len == 0) return true;
  std::vector<WCHAR> srcpath;
  srcpath.resize(len);
  DoubleNullTextFill(src, &srcpath[0]);
  return DoFileOperation(&srcpath[0], dst, fo, flags);
}

HRESULT FileCopyOrMove(Tuple* args, WORD fo) {
  Object* src;
  Unicode* dst;
  extract(args, "OO!", &src, &PyUnicode_Type, &dst);
  return DoFileOperation(src, cast<PCWSTR>(dst), fo, FOF_ALLOWUNDO);
}

HRESULT FileDeleteOrBury(Tuple* args, FILEOP_FLAGS flags) {
  if (len(args) == 0) return true;
  Object* src = (len(args) == 1 ? getitem(args, 0) : args);
  return DoFileOperation(src, null, FO_DELETE, flags);
}
}  // namespace

//================================================================================

HRESULT avesta::FileCopy(Tuple* args) { return FileCopyOrMove(args, FO_COPY); }

HRESULT avesta::FileMove(Tuple* args) { return FileCopyOrMove(args, FO_MOVE); }

HRESULT avesta::FileDelete(Tuple* args) { return FileDeleteOrBury(args, FOF_WANTNUKEWARNING | FOF_ALLOWUNDO); }

HRESULT avesta::FileBury(Tuple* args) { return FileDeleteOrBury(args, FOF_WANTNUKEWARNING); }

//================================================================================

#endif  // AVESTA_PYTHON_INTERFACE
