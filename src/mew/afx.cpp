// afx.cpp

#include "stdafx.h"
#include "afx.hpp"

#include "math.hpp"
#include "string.hpp"
#include "widgets.hpp"
#include "std/buffer.hpp"
#include <atlpath.h>

namespace {

enum {
  SHELL32_SHAllocShared = 520,
  SHELL32_SHLockShared = 521,
  SHELL32_SHUnlockShared = 522,
  SHELL32_SHFreeShared = 523,
  SHELL32_SHGetImageList = 727,
  USER32_PrivateExtractIconsW = 521,
};

#define INVALID_FUNCTION_POINTER ((void*)(UINT_PTR)-1)

// ���̊֐��͔r��������s��Ȃ��ėǂ��B
// ���ʂɂ͂Ȃ邪�A�ǂ��������|�C���^������̂ŁB
static bool DynamicLoadVoidPtr(void** fn, PCWSTR module, PCSTR name, int64_t index) throw() {
  ASSERT(fn);
  if (*fn == INVALID_FUNCTION_POINTER) {
    return false;
  }
  if (*fn) {
    return true;
  }
  HINSTANCE hDLL = ::LoadLibrary(module);
  *fn = ::GetProcAddress(hDLL, name);
  if (!*fn && index > 0) {
    *fn = ::GetProcAddress(hDLL, (PCSTR)index);
  }
  ::FreeLibrary(hDLL);
  if (*fn) {
    return true;
  }
  *fn = INVALID_FUNCTION_POINTER;
  return false;
}

template <typename T>
inline bool DynamicLoad(T** fn, PCWSTR module, PCSTR name, int64_t index) throw() {
  return DynamicLoadVoidPtr((void**)fn, module, name, index);
}
}  // namespace

#if 0
HRESULT avesta::UrlDownload(PCWSTR url) {
  // 128 DoFileDownload   : goes straight to the Save As dialog.
  // 129 DoFileDownloadEx : opens one additional dialog that asks you if you want to open the file "in place" or save it to disk
  static BOOL(__stdcall * fn)(PCWSTR file) = NULL;
  if (DynamicLoad(&fn, L"shdocvw.dll", "DoFileDownload", 128))
    return fn(url) ? S_OK : E_FAIL;
  else
    return E_NOTIMPL;
}
#endif

HANDLE afx::SHAllocShared(LPVOID data, ULONG size, DWORD pid) {
  static LPVOID(__stdcall * fn)(LPVOID data, ULONG size, DWORD pid) = NULL;
  if (DynamicLoad(&fn, L"shell32.dll", "SHAllocShared", SHELL32_SHAllocShared)) {
    return fn(data, size, pid);
  } else {
    return NULL;
  }
}

LPVOID afx::SHLockShared(HANDLE hData, DWORD dwOtherProcId) {
  static LPVOID(__stdcall * fn)(HANDLE hData, DWORD dwOtherProcId) = NULL;
  if (DynamicLoad(&fn, L"shell32.dll", "SHLockShared", SHELL32_SHLockShared)) {
    return fn(hData, dwOtherProcId);
  } else {
    return NULL;
  }
}
BOOL afx::SHUnlockShared(LPVOID lpvData) {
  static BOOL(__stdcall * fn)(LPVOID lpvData) = NULL;
  if (DynamicLoad(&fn, L"shell32.dll", "SHUnlockShared", SHELL32_SHUnlockShared)) {
    return fn(lpvData);
  } else {
    return FALSE;
  }
}
BOOL afx::SHFreeShared(HANDLE hData, DWORD dwSourceProcId) {
  static BOOL(__stdcall * fn)(HANDLE hData, DWORD dwSourceProcId) = NULL;
  if (DynamicLoad(&fn, L"shell32.dll", "SHFreeShared", SHELL32_SHFreeShared)) {
    return fn(hData, dwSourceProcId);
  } else {
    return FALSE;
  }
}

namespace {
UINT __stdcall PrivateExtractIconsStab(PCWSTR lpszFile, int nIconIndex, int cxIcon, int cyIcon, HICON* phicon, UINT* piconid,
                                       UINT nIcons, UINT flags) {
  for (UINT i = 0; i < nIcons; ++i) piconid[i] = 0;
  if (cxIcon > 16 || cyIcon > 16) {
    return ExtractIconEx(lpszFile, nIconIndex, phicon, NULL, nIcons);
  } else {
    return ExtractIconEx(lpszFile, nIconIndex, NULL, phicon, nIcons);
  }
}
}  // namespace

HICON afx::ExtractIcon(PCWSTR filename, int index, int w, int h) {
  static UINT(__stdcall * fn)(PCWSTR lpszFile, int nIconIndex, int cxIcon, int cyIcon, HICON* phicon, UINT* piconid,
                              UINT nIcons, UINT flags) = NULL;
  if (!fn) {
    if (!DynamicLoad(&fn, L"user32.dll", "PrivateExtractIconsW", USER32_PrivateExtractIconsW)) {
      fn = PrivateExtractIconsStab;
    }
  }
  HICON hIcon;
  UINT id;
  if (fn(filename, index, w, h, &hIcon, &id, 1, 0) == 1) {
    return hIcon;
  } else {
    return NULL;
  }
}

//==============================================================================
// �V�F�����j���[.

namespace {
class ShellMenuProvider : public ATL::CWindowImpl<ShellMenuProvider> {
 private:
  CComPtr<IContextMenu> m_context1;
  CComQIPtr<IContextMenu2> m_context2;
  CComQIPtr<IContextMenu3> m_context3;

 public:
  ShellMenuProvider(IContextMenu* menu) : m_context1(menu), m_context2(menu), m_context3(menu) { ASSERT(m_context1); }

 public:  // msg map
  BEGIN_MSG_MAP(ShellMenuProvider)
  MESSAGE_HANDLER(WM_INITMENUPOPUP, OnContextMenu2)
  MESSAGE_HANDLER(WM_DRAWITEM, OnContextMenu2)
  MESSAGE_HANDLER(WM_MEASUREITEM, OnContextMenu2)
  MESSAGE_HANDLER(WM_MENUCHAR, OnContextMenu3)
  END_MSG_MAP()

  LRESULT OnContextMenu2(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (m_context2) {
      try {
        if SUCCEEDED (m_context2->HandleMenuMsg(uMsg, wParam, lParam)) {
          return 0;
        }
      } catch (...) {
      }
    }
    bHandled = false;
    return 0;
  }

  LRESULT OnContextMenu3(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (m_context3) {
      try {
        LRESULT lResult = 0;
        if SUCCEEDED (m_context3->HandleMenuMsg2(uMsg, wParam, lParam, &lResult)) {
          return lResult;
        }
      } catch (...) {
      }
    }
    bHandled = false;
    return 0;
  }
};

static const UINT ID_SHELL_MENU_FIRST = 0x7FFF;
static const UINT ID_SHELL_MENU_LAST = 0x8FFF;
}  // namespace

HMENU afx::SHBeginContextMenu(IContextMenu* pMenu) {
  if (!pMenu) {
    return NULL;
  }
  WTL::CMenu popup;
  popup.CreatePopupMenu();
  if FAILED (pMenu->QueryContextMenu(popup, 0, ID_SHELL_MENU_FIRST, ID_SHELL_MENU_LAST, CMF_CANRENAME | CMF_EXPLORE)) {
    ASSERT(!"error: IContextMenu.QueryContextMenu()");
    return NULL;
  }
  return popup.Detach();
}

HMENU afx::SHBeginContextMenu(IShellView* pView, UINT svgio, IContextMenu** ppMenu) {
  if (!ppMenu) {
    return nullptr;
  }
  *ppMenu = nullptr;
  if (!pView || FAILED(pView->GetItemObject(svgio, IID_IContextMenu, (void**)ppMenu))) {
    return nullptr;
  }
  return afx::SHBeginContextMenu(*ppMenu);
}

UINT afx::SHPopupContextMenu(IContextMenu* pMenu, HMENU hMenu, POINT ptScreen) {
  // �Z�p���[�^���A������ȂǁA�s�v�c�ȃ��j���[���擾����Ă��܂����Ƃ�����̂ŁA��������B
  bool prevIsSeparator = true;
  for (int i = GetMenuItemCount(hMenu) - 1; i >= 0; --i) {
    MENUITEMINFO info = {sizeof(MENUITEMINFO), MIIM_TYPE};
    if (!GetMenuItemInfo(hMenu, i, true, &info)) {
      prevIsSeparator = false;
      continue;
    }
    bool isSep = !!(info.fType & MF_SEPARATOR);
    if (isSep && prevIsSeparator) {
      DeleteMenu(hMenu, i, MF_BYPOSITION);
    }
    prevIsSeparator = isSep;
  }
  //
  ShellMenuProvider provider(pMenu);
  if (!provider.Create(HWND_MESSAGE)) {
    return 0;
  }
  UINT cmd = ::TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, ptScreen.x, ptScreen.y, 0, provider, NULL);
  provider.DestroyWindow();
  return cmd;
}

HRESULT afx::SHEndContextMenu(IContextMenu* pMenu, int command, HWND hwnd, PWSTR verb) {
  HRESULT hr = S_FALSE;
  if (pMenu && ID_SHELL_MENU_FIRST <= command && command <= ID_SHELL_MENU_LAST) {
    CMINVOKECOMMANDINFO cmi = {sizeof(CMINVOKECOMMANDINFO)};
    cmi.hwnd = hwnd;
    cmi.lpVerb = MAKEINTRESOURCEA(command - ID_SHELL_MENU_FIRST);
    cmi.nShow = SW_SHOWNORMAL;
    if (SUCCEEDED(hr = pMenu->InvokeCommand(&cmi)) && verb) {
      if FAILED (pMenu->GetCommandString(command, GCS_VERBW, NULL, (LPSTR)verb, MAX_PATH)) {
        mew::str::clear(verb);
      }
    }
  }
  return hr;
}

//==============================================================================
// �N���b�v�{�[�h.

namespace {
template <class T>
static void WritePath(const CIDA* pCIDA, mew::Stream& stream, IShellFolder* pParentFolder, const T* parentPath,
                      size_t parentPathLength, const T* sep, size_t seplen) {
  const size_t count = afx::CIDAGetCount(pCIDA);
  for (size_t i = 0; i < count; i++) {
    WIN32_FIND_DATA desc;
    SHGetDataFromIDList(pParentFolder, afx::CIDAGetAt(pCIDA, i), SHGDFIL_FINDDATA, &desc, sizeof(desc));
    stream.write(parentPath, parentPathLength * sizeof(T));
    stream.write(desc.cFileName, (lstrlen(desc.cFileName)) * sizeof(T));
    stream.write(sep, seplen * sizeof(T));
  }
  stream.write(_T("\0"), sizeof(T));  // �Ō��NULL
}

template <class T>
HGLOBAL CreateTextT(const CIDA* pCIDA) {
  CComPtr<IShellFolder> pParentFolder;
  if FAILED (afx::ILGetSelfFolder(afx::CIDAGetParent(pCIDA), &pParentFolder)) {
    return NULL;
  }
  T parentPath[MAX_PATH];
  afx::ILGetPath(afx::CIDAGetParent(pCIDA), parentPath);
  ATLPath::AddBackslash(parentPath);
  UINT parentPathLength = mew::str::length(parentPath);
  // ��̂���Ȃ��񂠂�Ώ\�����낤�B
  UINT allocateBytes = (parentPathLength + 64) * afx::CIDAGetCount(pCIDA) * sizeof(T);
  mew::Stream stream;
  HGLOBAL hGlobal = mew::io::StreamCreateOnHGlobal(&stream, allocateBytes, false);
  const T sep[2] = {'\r', '\n'};
  WritePath<T>(pCIDA, stream, pParentFolder, parentPath, parentPathLength, sep, 2);
  return hGlobal;
}
}  // namespace

HGLOBAL afx::CIDAToHDROP(const CIDA* pCIDA) {
  // HDROP �́ADROPFILES �\���̂̃w�b�_�̌�ɁA�_�u��NULL��؂�t�@�C�����z�񂪑����`���B
  // �c�c�Ƃ������Ƃ́ADragQueryFile() �͎w�肵���C���f�b�N�X�̃t�@�C�������擾����̂�
  // ���������_�u��NULL��؂蕶������X�L�������Ă���̂��c�c�B
  CComPtr<IShellFolder> pParentFolder;
  if FAILED (ILGetSelfFolder(afx::CIDAGetParent(pCIDA), &pParentFolder)) {
    return NULL;
  }
  TCHAR parentPath[MAX_PATH];
  ILGetPath(afx::CIDAGetParent(pCIDA), parentPath);
  PathAddBackslash(parentPath);
  UINT parentPathLength = lstrlen(parentPath);
  // ��̂���Ȃ��񂠂�Ώ\�����낤�B
  UINT allocateBytes = sizeof(DROPFILES) + (parentPathLength + 64) * CIDAGetCount(pCIDA) * sizeof(TCHAR);
  mew::Stream stream;
  HGLOBAL hGlobal = mew::io::StreamCreateOnHGlobal(&stream, allocateBytes, false);
  DROPFILES dropfiles = {sizeof(DROPFILES), 0, 0, true, IS_UNICODE_CHARSET};
  GetCursorPos(&dropfiles.pt);
  stream.write(&dropfiles, sizeof(DROPFILES));
  WritePath<TCHAR>(pCIDA, stream, pParentFolder, parentPath, parentPathLength, _T(""), 1);
  return hGlobal;
}

HGLOBAL afx::CIDAToTextA(const CIDA* pCIDA) { return CreateTextT<CHAR>(pCIDA); }
HGLOBAL afx::CIDAToTextW(const CIDA* pCIDA) { return CreateTextT<WCHAR>(pCIDA); }

mew::string afx::GetClipboardText(HWND hwnd) {
  if (!::OpenClipboard(hwnd)) {
    return mew::null;
  }
  mew::string result;
  if (HGLOBAL hText = ::GetClipboardData(CF_UNICODETEXT)) {
    PCTSTR data = (PCTSTR)::GlobalLock(hText);
    result = mew::string(data);
    ::GlobalUnlock(hText);
  }
  ::CloseClipboard();
  return result;
}

bool afx::SetClipboard(HGLOBAL hMem, int format, HWND hWnd) {
  if (!::OpenClipboard(hWnd)) {
    return false;
  }
  ::EmptyClipboard();
  ::SetClipboardData(format, hMem);
  ::CloseClipboard();
  return true;
}

//==============================================================================

namespace {
static void PathNormaizeSeparator(PWSTR dst, PCWSTR src) {
  PCTSTR src2 = src;
  WCHAR srcbuf[MAX_PATH];
  size_t len = wcslen(src);
  if (len >= 2) {
    for (size_t i = 0; i < len; ++i) {
      srcbuf[i] = (src[i] != L'/' ? src[i] : L'\\');
    }
    srcbuf[len] = L'\0';
    src2 = srcbuf;
    bool prev_is_sep = false;
    for (size_t i = 1; i < len;) {
      bool is_sep = (srcbuf[i] == L'\\');
      if (is_sep && prev_is_sep) {
        size_t seplen = 1;
        while (srcbuf[i + seplen] == L'\\') {
          ++seplen;
        }
        len -= seplen;
        memmove(srcbuf + i, srcbuf + i + seplen, len * sizeof(WCHAR));
      } else {
        prev_is_sep = is_sep;
        ++i;
      }
    }
  }
  if (!PathCanonicalizeW(dst, src2)) {
    wcscpy(dst, src2);
  }
}
}  // namespace

HRESULT afx::PathNormalize(WCHAR dst[MAX_PATH], PCWSTR src) {
  if (mew::str::compare(src, L"::{", 3) == 0) {  //
    mew::str::copy(dst, src);
  } else if (UrlIsFileUrlW(src)) {
    DWORD dwLength = MAX_PATH;
    PathCreateFromUrlW(src, dst, &dwLength, 0);
  } else if (PathIsRegistory(src)) {  // Registory\HKEY_...
    mew::str::copy(dst, src);
  } else {
    PathNormaizeSeparator(dst, src);
    if (wcslen(dst) == 1 && iswalpha(src[0])) {  // "C" �Ƃ��B
      wcscat(dst, L":\\");
    }
  }
  return S_OK;
}

bool afx::PathIsRegistory(PCWSTR name) {
  static const WCHAR REGISTORY[] = L"Registry\\";
  static const WCHAR HKEY_XXX[] = L"HKEY_";
  return mew::str::compare_nocase(name, REGISTORY, 9) == 0 || mew::str::compare_nocase(name, HKEY_XXX, 5) == 0;
}

bool afx::PathIsFolder(PCWSTR path) {
  if (PathIsDirectory(path)) {
    return true;  // �f�B���N�g��
  }
  PCTSTR ext = PathFindExtension(path);
  TCHAR key[MAX_PATH], def[MAX_PATH], command[MAX_PATH], buffer[MAX_PATH];
  if FAILED (avesta::RegGetString(HKEY_CLASSES_ROOT, ext, NULL, key)) {
    return true;  // ���o�^����
  }
  wsprintf(buffer, _T("%s\\shell"), key);
  if FAILED (avesta::RegGetString(HKEY_CLASSES_ROOT, buffer, NULL, def)) {
    return true;  // �G���[
  }
  if (mew::str::equals_nocase(def, _T("explore"))) {
    return true;  // �G�N�X�v���[���Ɋ֘A�t��
  }
  if (mew::str::empty(def)) {
    lstrcpy(def, _T("open"));
  }
  wsprintf(buffer, _T("%s\\shell\\%s\\command"), key, def);
  if FAILED (avesta::RegGetString(HKEY_CLASSES_ROOT, buffer, NULL, command)) {
    return true;
  }
  return false;  // �����A�v���P�[�V�����Ɋ֘A�t�����Ă���
}

//==============================================================================

HRESULT afx::SHResolveLink(PCWSTR shortcut, PWSTR resolved) {
  HRESULT hr;
  if (!resolved) {
    return E_POINTER;
  }
  mew::str::clear(resolved);

  // �I�u�W�F�N�g�̍쐬
  CComPtr<IShellLink> link;
  hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&link);
  if FAILED (hr) {
    return hr;
  }
  CComPtr<IPersistFile> persist;
  hr = link->QueryInterface(&persist);
  if FAILED (hr) {
    return hr;
  }

  // �V���[�g�J�b�g��ǂݍ���
  hr = persist->Load(CT2CW(shortcut), STGM_READ);
  if FAILED (hr) {
    return hr;
  }

  // �����N��̃p�X�𓾂�
  hr = link->GetPath(resolved, MAX_PATH, NULL, SLGP_UNCPRIORITY);
  if FAILED (hr) {
    return hr;
  }
  return S_OK;
}

//==============================================================================
// ITEMIDLIST �֐�.

LPITEMIDLIST afx::ILFromPath(PCWSTR path, DWORD* pdwAttr) {
  LPITEMIDLIST pidl = 0;
  DWORD dwReserved = (pdwAttr ? *pdwAttr : 0);
  SHILCreateFromPath(path, &pidl, pdwAttr);
  if (pidl) {
    return pidl;
  }
  if (pdwAttr) {
    *pdwAttr = dwReserved;  // �[���N���A����Ă��܂��̂ŁA�����ł�����x�Z�b�g����
  }                         //
  size_t len = mew::str::length(path);
  if (len < 4 || path[0] != L':') {
    return NULL;
  }
  // format is ":mem:pid"
  PCWSTR nextColon = mew::str::find(path + 1, L':');
  if (!nextColon) {
    return NULL;
  }
  HANDLE hMem = (HANDLE)mew::str::atoi64(path + 1);
  DWORD pid = (DWORD)mew::str::atoi(nextColon + 1);
  pidl = ILFromShared(hMem, pid);
  if (pidl) {
    if (pdwAttr && *pdwAttr != 0) {
      if (ILIsRoot(pidl)) {
        *pdwAttr &= SFGAO_FOLDER;
      } else {
        LPCITEMIDLIST leaf = NULL;
        CComPtr<IShellFolder> parent;
        if SUCCEEDED (afx::ILGetParentFolder(pidl, &parent, &leaf)) {
          VERIFY_HRESULT(parent->GetAttributesOf(1, &leaf, pdwAttr));
        } else {
          ASSERT(!"�e���擾�ł��Ȃ����Ăǂ��������Ƃ�H");
          *pdwAttr &= SFGAO_FOLDER;
        }
      }
    }
  }
  return pidl;
}
LPITEMIDLIST afx::ILFromShared(HANDLE hMem, DWORD pid) {
  LPITEMIDLIST pidl = NULL;
  if (LPITEMIDLIST pidlShared = (LPITEMIDLIST)SHLockShared(hMem, pid)) {
    if (!IsBadReadPtr(pidlShared, 1)) {
      pidl = ILClone(pidlShared);
    }
    afx::SHUnlockShared(pidlShared);
    SHFreeShared(hMem, pid);
  }
  return pidl;
}
LPITEMIDLIST afx::ILFromExplorer(HWND hwnd) {
  LPITEMIDLIST pIDL = nullptr;
  IShellWindows* psw;
  if (!SUCCEEDED(CoCreateInstance(CLSID_ShellWindows, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&psw)))) {
    return nullptr;
  }
  long swCount = 0;
  psw->get_Count(&swCount);
  for (long i = 0; i < swCount; ++i) {
    IDispatch* pdisp;
    psw->Item(CComVariant(i), &pdisp);
    if (pdisp == NULL) {
      continue;
    }
    IShellBrowser* psb;
    if (SUCCEEDED(IUnknown_QueryService(pdisp, SID_SShellBrowser, IID_PPV_ARGS(&psb)))) {
      CComQIPtr<IWebBrowser2> pwb(pdisp);
      if (pwb == NULL) {
        continue;
      }
      HWND wbhwnd;
      pwb->get_HWND((SHANDLE_PTR*)&wbhwnd);
      if (wbhwnd == hwnd) {
        BSTR path;
        pwb->get_LocationURL(&path);
        if (SysStringByteLen(path) != 0) {  // ����t�H���_�ł͂Ȃ�
          LPSHELLFOLDER pDesktopFolder;
          if (SUCCEEDED(::SHGetDesktopFolder(&pDesktopFolder))) {
            ULONG chEaten;
            ULONG dwAttributes;
            pDesktopFolder->ParseDisplayName(NULL, NULL, path, &chEaten, &pIDL, &dwAttributes);
            pDesktopFolder->Release();
          }
        }
      }
      pwb.Release();
    }
    pdisp->Release();
    if (pIDL != nullptr) {
      break;
    }
  }
  psw->Release();

  return pIDL;
}
HRESULT afx::ILGetSelfFolder(LPCITEMIDLIST pidl, IShellFolder** ppFolder) {
  HRESULT hr;
  if (ILIsRoot(pidl)) return SHGetDesktopFolder(ppFolder);
  LPCITEMIDLIST leaf;
  CComPtr<IShellFolder> pParent;
  if FAILED (hr = afx::ILGetParentFolder(pidl, &pParent, &leaf)) return hr;
  return pParent->BindToObject(leaf, NULL, IID_IShellFolder, (void**)ppFolder);
}

//==============================================================================
// CIDA �֐�.

HGLOBAL afx::CIDAFromSingleIDList(LPCITEMIDLIST pidl) {
  LPCITEMIDLIST leaf = ILFindLastID(pidl);
  if (!leaf) {
    return NULL;
  }
  size_t pidlSize = ILGetSize(pidl);
  size_t headerSize = sizeof(UINT) * 3;
  size_t parentSize = (BYTE*)leaf - (BYTE*)pidl;
  size_t leafSize = pidlSize - parentSize - 2;
  size_t totalSize = headerSize + parentSize + leafSize + 4;

  ASSERT(ILGetSize(leaf) == leafSize + 2);
  ASSERT(pidlSize + 2 + headerSize == totalSize);

  HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, totalSize);
  CIDA* p = (CIDA*)::GlobalLock(hGlobal);
  memset(p, 0, totalSize);
  p->cidl = 1;
  p->aoffset[0] = headerSize;
  p->aoffset[1] = headerSize + parentSize + 2;
  // ���[�� (WORD)0 �Ȃ̂ŁA�R�s�[���Ȃ��imemset()�ł��łɃ[�������߂��Ă���j
  memcpy(((BYTE*)p) + p->aoffset[0], pidl, parentSize);
  memcpy(((BYTE*)p) + p->aoffset[1], leaf, leafSize);
  ::GlobalUnlock(hGlobal);
  return hGlobal;
}

//==============================================================================

namespace {
int ExpGetImageIndexForDrive(UINT type) {
  DWORD drives = GetLogicalDrives();
  // FDD(A:)���㏑�����Ă��܂��̂ŁA2(C:)����n�߂�
  for (int i = 2; i < 26; ++i) {
    if (drives & (1 << i)) {
      TCHAR path[] = _T("*:\\");
      path[0] = (TCHAR)('A' + i);
      if (GetDriveType(path) == type) {
        return afx::ExpGetImageIndex(path);
      }
    }
  }
  return 0;
}

int ExpGetDummyFile(PCWSTR relpath) {
  TCHAR path[MAX_PATH];
  ::GetModuleFileName(NULL, path, MAX_PATH);
  ::PathRemoveFileSpec(path);
  ::PathAppend(path, relpath);
  if (!::PathFileExists(path)) {
    avesta::FileNew(path);
  }
  return afx::ExpGetImageIndex(path);
}

int IconID_Index(int ID) {
  switch (ID) {
    case 0:  // ����`�̃t�@�C��
    case 1:  // �W���̕���
      return ExpGetDummyFile(L"..\\var\\unknown");
    case 2:  // �W���� EXE, COM �t�@�C��
      return ExpGetDummyFile(L"..\\var\\unknown.exe");
    case 3:  // ������Ԃ̃t�H���_
      return afx::ExpGetImageIndexForFolder();
    case 4:  // �J���Ă�t�H���_
      break;
    case 5:  // Floppy Drive (5.25)
    case 6:  // Floppy Drive (3.5)
      return afx::ExpGetImageIndex(_T("A:\\"));
    case 7:  // �����[�o�u���h���C�u
      return ExpGetImageIndexForDrive(DRIVE_REMOVABLE);
    case 8:  // �n�[�h�h���C�u
      return ExpGetImageIndexForDrive(DRIVE_FIXED);
    case 9:  // �l�b�g���[�N�h���C�u
      return ExpGetImageIndexForDrive(DRIVE_REMOTE);
    case 10:  // �l�b�g���[�N�h���C�u�i�I�t���C���j
      break;
    case 11:  // CD Drive
      return ExpGetImageIndexForDrive(DRIVE_CDROM);
    case 12:  // ���z RAM �h���C�u
      return ExpGetImageIndexForDrive(DRIVE_RAMDISK);
    case 13:  // �l�b�g���[�N�S��
      return afx::ExpGetImageIndex(CSIDL_NETWORK);
    case 17:  // �l�b�g���[�N�R���s���[�^
      break;
    case 18:  // ���[�N�O���[�v
      break;
    case 19:  // �v���O�����i�X�^�[�g���j���[�j
      return afx::ExpGetImageIndex(CSIDL_STARTMENU);
    case 20:  // �ŋߎg�����t�@�C���i�X�^�[�g���j���[�j
      return afx::ExpGetImageIndex(CSIDL_RECENT);
    case 21:  // �ݒ�i�X�^�[�g���j���[�j
    case 22:  // �����i�X�^�[�g���j���[�j
    case 23:  // �w���v�i�X�^�[�g���j���[�j
    case 24:  // �t�@�C�������w�肵�Ď��s�i�X�^�[�g���j���[�j
    case 25:  // �T�X�y���h�i�X�^�[�g���j���[�j
    case 26:  // �h�b�L���O�X�e�[�V�����i�X�^�[�g���j���[�j
    case 27:  // Windows �̏I���i�X�^�[�g���j���[�j
    case 28:  // �V�F�A�t�H���_�p�A�C�R��
    case 29:  // �V���[�g�J�b�g�̏��������
      break;
    case 34:  // �f�X�N�g�b�v
      return afx::ExpGetImageIndex(CSIDL_DESKTOP);
    case 36:  // �v���O�����O���[�v
    case 40:  // ���y CD
    case 44:  // ���O�I�t
    default:
      break;
  }
  TRACE(L"TODO: IconID_Index($1) ����肷����@��������܂���", ID);
  return -1;
}
void OverrideIcons(IImageList* imagelist[], size_t count) {
  HUSKEY hKey;
  if (ERROR_SUCCESS ==
      SHRegOpenUSKey(_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Icons"), KEY_READ, NULL, &hKey, TRUE)) {
    TCHAR cchName[512], cchValue[512];
    DWORD nName, nValue, nType;
    for (DWORD index = 0; nName = lengthof(cchName), nValue = sizeof(cchValue),
               ERROR_SUCCESS == SHRegEnumUSValue(hKey, index, cchName, &nName, &nType, cchValue, &nValue, SHREGENUM_HKLM);
         ++index) {
      // 'Shell Icons' format:
      // name: <icon id>
      // value: "<resource path>,<icon index>"
      if (LPTSTR comma = mew::str::find_reverse(cchValue, _T(','))) {
        int iconID = mew::str::atoi(cchName);
        int resID = mew::str::atoi(comma + 1);
        *comma = _T('\0');
        int iconIndex = IconID_Index(iconID);
        if (iconIndex >= 0) {
          for (size_t i = 0; i < count; ++i) {
            if (!imagelist[i]) {
              continue;
            }
            int w, h;
            if FAILED (imagelist[i]->GetIconSize(&w, &h)) {
              continue;
            }
            if (HICON hIcon = afx::ExtractIcon(cchValue, resID, w, h)) {
              int ret;
              VERIFY_HRESULT(imagelist[i]->ReplaceIcon(iconIndex, hIcon, &ret));
              ::DestroyIcon(hIcon);
            }
          }
        }
      }
    }
    SHRegCloseUSKey(hKey);
  }
}

void SHImageListEnsureInitialize() {
  static BOOL(__stdcall * fn)(int, REFIID, void**) = NULL;
  if (fn) {
    return;
  }
  if (DynamicLoad(&fn, L"shell32.dll", "SHGetImageList", SHELL32_SHGetImageList)) {
    IImageList* img[3] = {};
    fn(SHIL_SMALL, __uuidof(IImageList), (void**)&img[0]);
    fn(SHIL_LARGE, __uuidof(IImageList), (void**)&img[1]);
    fn(SHIL_EXTRALARGE, __uuidof(IImageList), (void**)&img[2]);
    OverrideIcons(img, lengthof(img));
    for (size_t i = 0; i < lengthof(img); ++i) {
      mew::objdec(img[i]);
    }
  }
}
}  // namespace

HRESULT afx::ExpGetImageList(int size, IImageList** ppImageList) {
  SHImageListEnsureInitialize();
  static BOOL(__stdcall * fn)(int, REFIID, void**) = NULL;
  if (DynamicLoad(&fn, L"shell32.dll", "SHGetImageList", SHELL32_SHGetImageList)) {
    int what;
    if (size < 32) {
      what = SHIL_SMALL;
    } else if (size < 48) {
      what = SHIL_LARGE;
    } else {
      what = SHIL_EXTRALARGE;
    }
    return fn(what, __uuidof(IImageList), (void**)ppImageList);
  } else {
    return E_UNEXPECTED;
  }
}

HIMAGELIST afx::ExpGetImageList(int size) {
  SHImageListEnsureInitialize();
  SHFILEINFO file;
  DWORD which = (size <= 16 ? SHGFI_SMALLICON : SHGFI_LARGEICON);
  return (HIMAGELIST)SHGetFileInfo(_T(""), 0, &file, sizeof(file), SHGFI_SYSICONINDEX | which);
}

int afx::ExpGetImageIndex(PCWSTR path) {
  if (!path) {
    return 0;
  }
  SHFILEINFO file;
  if (!SHGetFileInfo(path, 0, &file, sizeof(file), SHGFI_SYSICONINDEX | SHGFI_SMALLICON)) {
    return 0;
  }
  return file.iIcon;
}

int afx::ExpGetImageIndex(LPCITEMIDLIST pidl) {
  SHFILEINFO file;
  if (!afx::ILGetFileInfo(pidl, &file, SHGFI_SYSICONINDEX | SHGFI_SMALLICON)) {
    return 0;
  }
  return file.iIcon;
}

int afx::ExpGetImageIndex(int csidl) {
  LPITEMIDLIST pidl = nullptr;
  if FAILED (SHGetSpecialFolderLocation(nullptr, csidl, &pidl)) {
    return 0;
  }
  int ret = ExpGetImageIndex(pidl);
  ::ILFree(pidl);
  return ret;
}

int afx::ExpGetImageIndexForFolder() {
  // XXX: exe���u����Ă���p�X�́A���ʂȃA�C�R�����w�肳��Ă��Ȃ��Ɖ��肷��
  TCHAR path[MAX_PATH];
  ::GetModuleFileName(NULL, path, MAX_PATH);
  ::PathRemoveFileSpec(path);
  return ExpGetImageIndex(path);
}

namespace {
static bool IsExplorer(HWND hwnd) {
  TCHAR wndcls[MAX_PATH];
  ::GetClassName(hwnd, wndcls, MAX_PATH);
  if (lstrcmp(wndcls, _T("CabinetWClass")) != 0 && lstrcmp(wndcls, _T("ExploreWClass")) != 0) return false;
  return true;
}
struct SHEnumExplorersParam {
  HRESULT (*fnEnum)(HWND hExplorer, LPCITEMIDLIST pidl, LPARAM lParam);
  LPARAM lParam;
  HRESULT hr;
};
static BOOL CALLBACK EnumExplorersProc(HWND hwnd, LPARAM lParam) {
  SHEnumExplorersParam* param = (SHEnumExplorersParam*)lParam;
  ASSERT(param);
  if (!::IsWindowVisible(hwnd)) {
    return true;
  }
  if (!IsExplorer(hwnd)) {
    return true;
  }
  LPITEMIDLIST pidl = afx::ILFromExplorer(hwnd);
  if (!pidl) {
    return true;
  }
  param->hr = param->fnEnum(hwnd, pidl, param->lParam);
  ILFree(pidl);
  return (param->hr == S_OK);
}
}  // namespace

HRESULT afx::ExpEnumExplorers(HRESULT (*fnEnum)(HWND hExplorer, LPCITEMIDLIST pidl, LPARAM lParam), LPARAM lParam) {
  ASSERT(fnEnum);
  SHEnumExplorersParam param = {fnEnum, lParam, S_OK};
  ::EnumWindows(EnumExplorersProc, (LPARAM)&param);
  return param.hr;
}

namespace {
static UINT FindGroupItem(HMENU hMenu, HMENU* phMenuParent) {
  const int count = ::GetMenuItemCount(hMenu);
  for (int i = 0; i < count; ++i) {
    TCHAR text[MAX_PATH];
    MENUITEMINFO info = {sizeof(MENUITEMINFO), MIIM_ID | MIIM_SUBMENU | MIIM_STRING};
    info.dwTypeData = text;
    info.cch = MAX_PATH;
    if (::GetMenuItemInfo(hMenu, i, TRUE, &info)) {
      if (mew::str::equals(text, _T("�O���[�v�ŕ\��(&G)"))) {
        if (phMenuParent) {
          *phMenuParent = hMenu;
        }
        return info.wID;
      } else if (info.hSubMenu) {
        if (UINT cmd = FindGroupItem(info.hSubMenu, phMenuParent)) {
          return cmd;
        }
      }
    }
  }
  return 0;
}
static HRESULT GetGroupMenuItem(IShellView* pShellView, IContextMenu** ppMenu = NULL, UINT* uCommand = NULL) {
  if (ppMenu) {
    *ppMenu = NULL;
  }
  if (uCommand) {
    *uCommand = 0;
  }
  if (!pShellView) {
    return E_UNEXPECTED;
  }
  HRESULT hr;
  CComPtr<IContextMenu> pContextMenu1;
  if FAILED (hr = pShellView->GetItemObject(SVGIO_BACKGROUND, IID_IContextMenu, (void**)&pContextMenu1)) {
    return hr;
  }
  CMenu menu;
  menu.CreatePopupMenu();
  if FAILED (hr = pContextMenu1->QueryContextMenu(menu, 0, ID_SHELL_MENU_FIRST, ID_SHELL_MENU_LAST,
                                                  CMF_CANRENAME | CMF_EXPLORE)) {
    return hr;
  }
  CMenuHandle menuParent;
  UINT cmd = FindGroupItem(menu, &menuParent.m_hMenu);
  if (cmd == 0) {
    return E_FAIL;
  }
  if (uCommand) {
    *uCommand = cmd;
  }
  pContextMenu1.CopyTo(ppMenu);
  return (menuParent.GetMenuState(cmd, MF_BYCOMMAND) & MF_CHECKED) ? S_OK : S_FALSE;
}
}  // namespace

bool afx::ExpEnableGroup(IShellView* pShellView, bool enable) {
  UINT cmd = 0;
  CComPtr<IContextMenu> menu;
  HRESULT hr = GetGroupMenuItem(pShellView, &menu, &cmd);
  if FAILED (hr) {
    ASSERT(!"�O���[�v�؂�ւ����j���[��������Ȃ�");
    return false;
  }
  if ((hr == S_OK && enable) || (hr == S_FALSE && !enable)) {
    return enable;  // no needs
  }
  CMINVOKECOMMANDINFO cmi = {sizeof(CMINVOKECOMMANDINFO)};
  cmi.lpVerb = MAKEINTRESOURCEA(cmd - ID_SHELL_MENU_FIRST);
  cmi.nShow = SW_SHOWNORMAL;
  if SUCCEEDED (menu->InvokeCommand(&cmi)) {
    return enable;
  }
  ASSERT(!"�O���[�v�؂�ւ��Ɏ��s");
  return false;
}

static const PCTSTR ThumbnailSizeSubKey = _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer");
static const PCTSTR ThumbnailSizeValue = _T("ThumbnailSize");

DWORD afx::ExpGetThumbnailSize() {
  DWORD currentSize = 0;
  if SUCCEEDED (avesta::RegGetDWORD(HKEY_CURRENT_USER, ThumbnailSizeSubKey, ThumbnailSizeValue, &currentSize)) {
    return currentSize;
  } else {
    return 0xFFFFFFFF;
  }
}

HRESULT afx::ExpSetThumbnailSize(DWORD dwSize) {
  DWORD currentSize = ExpGetThumbnailSize();
  if (currentSize == dwSize) {
    return S_FALSE;  // no needs
  }
  return avesta::RegSetDWORD(HKEY_CURRENT_USER, ThumbnailSizeSubKey, ThumbnailSizeValue, dwSize);
}

HRESULT afx::ExpGetNavigateSound(PWSTR path) {
  return avesta::RegGetString(HKEY_CURRENT_USER, _T("AppEvents\\Schemes\\Apps\\Explorer\\Navigating\\.Current"), NULL, path);
}

//==============================================================================
// �E�B���h�E����֌W

namespace {
inline void RestoreKeyState(BYTE* keys, UINT vkey) { keys[vkey] = (BYTE)(((UINT)GetAsyncKeyState(vkey) & 0x8000U) >> 8); }
}  // namespace

void afx::RestoreModifierState(UINT vkey) {
  BYTE keys[256];
  GetKeyboardState(keys);
  RestoreKeyState(keys, VK_CONTROL);
  RestoreKeyState(keys, VK_LCONTROL);
  RestoreKeyState(keys, VK_RCONTROL);
  RestoreKeyState(keys, VK_SHIFT);
  RestoreKeyState(keys, VK_LSHIFT);
  RestoreKeyState(keys, VK_RSHIFT);
  RestoreKeyState(keys, VK_MENU);
  RestoreKeyState(keys, VK_LMENU);
  RestoreKeyState(keys, VK_RMENU);
  if (vkey) {
    RestoreKeyState(keys, vkey);
  }
  SetKeyboardState(keys);
}

void afx::SetModifierState(UINT vkey, DWORD mods) {
  BYTE keys[256];
  GetKeyboardState(keys);
  BYTE vkeyControl = ((mods & mew::ui::ModifierControl) ? 0x80 : 0x00);
  BYTE vkeyShift = ((mods & mew::ui::ModifierShift) ? 0x80 : 0x00);
  BYTE vkeyAlt = ((mods & mew::ui::ModifierAlt) ? 0x80 : 0x00);
  keys[VK_CONTROL] = vkeyControl;
  keys[VK_LCONTROL] = vkeyControl;
  keys[VK_RCONTROL] = vkeyControl;
  keys[VK_SHIFT] = vkeyShift;
  keys[VK_LSHIFT] = vkeyShift;
  keys[VK_RSHIFT] = vkeyShift;
  keys[VK_MENU] = vkeyAlt;
  keys[VK_LMENU] = vkeyAlt;
  keys[VK_RMENU] = vkeyAlt;
  if (vkey) {
    keys[vkey] = 0x80;
  }
  SetKeyboardState(keys);
}

bool afx::PumpMessage() {
  MSG msg;
  while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
    switch (msg.message) {
      case WM_NULL:
        // WH_GETMESSAGE �ŏ�������A�������ꂽ���b�Z�[�W�� ID �� WM_NULL �ɏ����������Ă���B
        continue;
      case WM_QUIT:
        return false;
    }
    // dispatch
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
  return true;
}

namespace {
static BOOL __stdcall PostMessageToTopMost(HWND hWnd, LPARAM lParam) {
  if (!IsWindow(hWnd)) {
    return true;
  }
  if (GetParent(hWnd)) {
    return true;
  }
  ::PostMessage(hWnd, lParam, 0, 0);
  return true;
}
}  // namespace

void afx::BroadcastMessage(UINT uMsg, DWORD dwThreadID) { EnumThreadWindows(dwThreadID, PostMessageToTopMost, uMsg); }

HRESULT afx::PrintWindow(HWND hwnd, HDC hdcBlt, UINT nFlags) { return ::PrintWindow(hwnd, hdcBlt, nFlags) ? S_OK : E_FAIL; }

HRESULT afx::MimicDoubleClick(int x, int y) {
  int w = ::GetSystemMetrics(SM_CXSCREEN);
  int h = ::GetSystemMetrics(SM_CYSCREEN);
  POINT pt;
  ::GetCursorPos(&pt);
  INPUT in[6] = {0};
  for (int i = 0; i < 6; ++i) {
    in[i].type = INPUT_MOUSE;
  }
  in[0].mi.dx = 65535 * x / w;
  in[0].mi.dy = 65535 * y / h;
  in[0].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTUP;
  in[1].mi.dwFlags = in[3].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
  in[2].mi.dwFlags = in[4].mi.dwFlags = MOUSEEVENTF_LEFTUP;
  in[5].mi.dx = 65535 * pt.x / w;
  in[5].mi.dy = 65535 * pt.y / h;
  in[5].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
  if (::SendInput(6, in, sizeof(INPUT)) > 0) {
    return S_OK;
  } else {
    return AtlHresultFromLastError();
  }
}

HRESULT afx::TipRelayEvent(HWND hTip, HWND hOwner, int xScreen, int yScreen) {
  if (!hTip || !hOwner) {
    return E_INVALIDARG;
  }
  POINT ptOwner = {xScreen, yScreen};
  ::ScreenToClient(hOwner, &ptOwner);
  MSG msg = {hOwner, WM_MOUSEMOVE, 0, MAKELPARAM(ptOwner.x, ptOwner.y), GetTickCount(), xScreen, yScreen};
  BYTE keys[256], keys2[256];
  GetKeyboardState(keys);
  memcpy(keys, keys, sizeof(256));
  keys2[VK_LBUTTON] = 0;
  keys2[VK_RBUTTON] = 0;
  keys2[VK_MBUTTON] = 0;
  keys2[VK_XBUTTON1] = 0;
  keys2[VK_XBUTTON2] = 0;
  SetKeyboardState(keys2);
  ::SendMessage(hTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
  SetKeyboardState(keys);
  return S_OK;
}

//==============================================================================
// �G�f�B�b�g.

namespace {
// respects "���E�у��l�[��" : http://www.digidigiday.com/
class OshinobiEdit : public ATL::CWindowImpl<OshinobiEdit, WTL::CEdit> {
 private:
  const bool m_isDirectory;
  const DWORD m_options;

 public:
  int EndOfBaseName() const {
    TCHAR text[MAX_PATH];
    GetWindowText(text, MAX_PATH);
    return EndOfBaseName(text);
  }
  int EndOfBaseName(PCTSTR text) const {
    if (m_isDirectory) {
      return lstrlen(text);
    }
    PCTSTR ext = ::PathFindExtension(text);
    return ext - text;
  }
  int StartOfExtension() const {
    if (m_isDirectory) {
      return -1;
    }
    TCHAR text[MAX_PATH];
    GetWindowText(text, MAX_PATH);
    return StartOfExtension(text);
  }
  int StartOfExtension(PCTSTR text) const {
    int len = lstrlen(text);
    if (m_isDirectory) {
      return len;
    }
    PCTSTR ext = ::PathFindExtension(text);
    int beg = ext - text + 1;  // +1 �� . �̂Ԃ�B
    return (beg > len) ? -1 : beg;
  }
  void SelectBaseName() {
    int end = EndOfBaseName();
    if (end > 0) {
      PostMessage(EM_SETSEL, 0, end);
    }
  }
  void SelectExtension() {
    int beg = StartOfExtension();
    if (beg > 0) {
      PostMessage(EM_SETSEL, beg, -1);
    }
  }
  bool IsBaseNameSelected() {
    int beg, end;
    GetSel(beg, end);
    return (beg == 0 && end == EndOfBaseName());
  }
  bool IsExtensionSelected() {
    TCHAR text[MAX_PATH];
    GetWindowText(text, MAX_PATH);
    int beg, end;
    GetSel(beg, end);
    return (end == lstrlen(text) && beg == StartOfExtension(text));
  }
  static bool Contains(UINT lcmap, PCTSTR str, size_t length) {
    int category;
    WORD flags;
    switch (lcmap) {
      case LCMAP_LOWERCASE:
        category = CT_CTYPE1;
        flags = C1_LOWER;
        break;
      case LCMAP_UPPERCASE:
        category = CT_CTYPE1;
        flags = C1_UPPER;
        break;
      case LCMAP_HALFWIDTH:
        category = CT_CTYPE3;
        flags = C3_HALFWIDTH;
        break;
      case LCMAP_FULLWIDTH:
        category = CT_CTYPE3;
        flags = C3_FULLWIDTH;
        break;
      case LCMAP_HIRAGANA:
        category = CT_CTYPE3;
        flags = C3_HIRAGANA;
        break;
      case LCMAP_KATAKANA:
        category = CT_CTYPE3;
        flags = C3_KATAKANA;
        break;
      default:
        TRESPASS();
    }
    WORD check[MAX_PATH];
    if (!GetStringTypeEx(LOCALE_USER_DEFAULT, category, str, length, check)) {
      return true;
    }
    for (size_t i = 0; i < length; ++i) {
      if (check[i] & flags) {
        return true;
      }
    }
    return false;
  }
  void ReplaceSelection(UINT first, UINT second) {
    TCHAR src[MAX_PATH];
    GetWindowText(src, MAX_PATH);
    int beg, end;
    GetSel(beg, end);
    if (end - beg <= 0) {
      beg = 0;
      end = EndOfBaseName(src);
      if (end < 0) {
        return;
      }
      SetSel(beg, end);
    }
    UINT lcmap = (!Contains(second, src + beg, end - beg) ? second : first);
    TCHAR dst[MAX_PATH];
    int len = LCMapString(GetUserDefaultLCID(), lcmap, src + beg, end - beg, dst, MAX_PATH);
    if (len >= 0) {
      dst[len] = _T('\0');
      ReplaceSel(dst, TRUE);
      SetSel(beg, beg + len);
    }
  }

 public:  // msg map
  BEGIN_MSG_MAP(OshinobiEdit)
  MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
  MESSAGE_HANDLER(WM_CHAR, OnChar)
  END_MSG_MAP()

  OshinobiEdit(HWND hwndEdit, bool isDirectory, DWORD options) : m_isDirectory(isDirectory), m_options(options) {
    SubclassWindow(hwndEdit);
  }

  /*override*/ void OnFinalMessage(HWND) { delete this; }

  int StartOfSel() const {
    int beg, end;
    GetSel(beg, end);
    return mew::math::min(beg, end);
  }

  int EndOfSel() const {
    int beg, end;
    GetSel(beg, end);
    return mew::math::max(beg, end);
  }

  static UINT GetShiftModifier() {
    if (::GetKeyState(VK_SHIFT) & 0x8000) {
      return MK_SHIFT;
    } else {
      return 0;
    }
  }
  void PressKey(UINT vkey, UINT mods) {
    afx::SetModifierState(vkey, mods);
    SendMessage(WM_KEYDOWN, vkey, 0);
    afx::RestoreModifierState(vkey);
  }
  void SelectAll() { SetSel(0, -1); }
  // void DeleteCharBefore() {}
  void DeleteCharAfter() { PressKey(VK_DELETE, 0); }
  void DeleteLineBefore() {
    SetSel(0, EndOfSel());
    ReplaceSel(_T(""), TRUE);
  }
  void DeleteLineAfter() {
    SetSel(StartOfSel(), -1);
    ReplaceSel(_T(""), TRUE);
  }
  void MoveCursor(UINT vk) { PressKey(vk, GetShiftModifier()); }

  LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
#define CTRL(k) ((k) - 'A' + 1)  // �� �炵���B�ڍוs���B�V�t�g��������Ă��ς��Ȃ��B
    switch (m_options & afx::KeybindMask) {
      case afx::KeybindAtok:
        if (::GetKeyState(VK_CONTROL) & 0x8000) {
          switch (wParam) {
            case CTRL('A'):
              MoveCursor(VK_HOME);
              return 0;
            case CTRL('S'):
              MoveCursor(VK_LEFT);
              return 0;
            case CTRL('D'):
              MoveCursor(VK_RIGHT);
              return 0;
            case CTRL('F'):
              MoveCursor(VK_END);
              return 0;
            case CTRL('G'):
              DeleteCharAfter();
              return 0;
            case CTRL('K'):
              DeleteLineAfter();
              return 0;
            case CTRL('U'):
              DeleteLineBefore();
              return 0;
          }
        }
        break;
      case afx::KeybindEmacs:
        if (::GetKeyState(VK_CONTROL) & 0x8000) {
          switch (wParam) {
            case CTRL('A'):
              MoveCursor(VK_HOME);
              return 0;
            case CTRL('B'):
              MoveCursor(VK_LEFT);
              return 0;
            case CTRL('D'):
              DeleteCharAfter();
              return 0;
            case CTRL('E'):
              MoveCursor(VK_END);
              return 0;
            case CTRL('F'):
              MoveCursor(VK_RIGHT);
              return 0;
            case CTRL('K'):
              DeleteLineAfter();
              return 0;
            case CTRL('N'):
              MoveCursor(VK_DOWN);
              return 0;
            case CTRL('P'):
              MoveCursor(VK_UP);
              return 0;
            case CTRL('U'):
              DeleteLineBefore();
              return 0;
          }
        }
        break;
      default:
        if (::GetKeyState(VK_CONTROL) & 0x8000) {
          switch (wParam) {
            case CTRL('A'):
              SelectAll();
              return 0;
            case CTRL('D'):
              DeleteCharAfter();
              return 0;
          }
        }
        break;
    }
#undef CTRL
    bHandled = false;
    return 0;
  }

  LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (!(::GetKeyState(VK_CONTROL) & 0x8000)) {
      switch (wParam) {
        case VK_F1:
          SelectAll();
          return 0;
        case VK_F6:
          ReplaceSelection(LCMAP_LOWERCASE, LCMAP_UPPERCASE);
          return 0;
        case VK_F7:
          ReplaceSelection(LCMAP_HALFWIDTH, LCMAP_FULLWIDTH);
          return 0;
        case VK_F8:
          ReplaceSelection(LCMAP_HIRAGANA, LCMAP_KATAKANA);
          return 0;
        default:
          if (m_options & afx::EditTypeFileName) {
            switch (wParam) {
              case VK_F2:
                if (IsBaseNameSelected())
                  SelectExtension();
                else
                  SelectBaseName();
                return 0;
              case VK_F3:
                SelectBaseName();
                return 0;
              case VK_F4:
                SelectExtension();
                return 0;
            }
          }
          break;
      }
    }
    bHandled = FALSE;
    return 0;
  }
};
}  // namespace

HRESULT afx::Edit_SubclassSingleLineTextBox(HWND hwndEdit, LPCWSTR fullpath, DWORD options) {
  BOOL isDirectory = fullpath && ::PathIsDirectory(fullpath);
  OshinobiEdit* edit = new OshinobiEdit(hwndEdit, isDirectory != 0, options);
  if (!(options & afx::RenameExtension)) {
    edit->SelectBaseName();
  }
  return S_OK;
}

//==============================================================================
// ���X�g�r���[.

namespace {
int ListView_GetMaxColumnWidth(HWND hwndListView, HWND hwndHeader, int index) {
  int width = 0;
  TCHAR text[MAX_PATH];
  // hwndHeader
  HDITEM hdItem = {HDI_TEXT};
  hdItem.pszText = text;
  hdItem.cchTextMax = lengthof(text);
  if (Header_GetItem(hwndHeader, index, &hdItem)) {
    CClientDC dc(hwndHeader);
    SIZE size;
    dc.GetTextExtent(text, lstrlen(text), &size);
    width = size.cx;
  }
  // listview
  const int rows = ListView_GetItemCount(hwndListView);
  for (int i = 0; i < rows; ++i) {
    ListView_GetItemText(hwndListView, i, index, text, lengthof(text));
    if (!mew::str::empty(text)) {
      CClientDC dc(hwndListView);
      SIZE size;
      dc.GetTextExtent(text, lstrlen(text), &size);
      width = mew::math::max<int>(width, size.cx);
    }
  }
  // special
  const int ListView_LabelMargin = GetSystemMetrics(SM_CXEDGE);
  if (index == 0) {
    width += 16;  // icon
    width += 4 * ListView_LabelMargin;
    if (ListView_GetExtendedListViewStyle(hwndListView) & LVS_EX_CHECKBOXES) {
      width += GetSystemMetrics(SM_CXMENUCHECK);  // ���j���[�ƃ��X�g�r���[�͈Ⴄ�悤�ȋC�����邪�c�c�B
    }
  } else {
    width += 8 * ListView_LabelMargin;
  }
  return width;
}
}  // namespace

HRESULT afx::ListView_AdjustToWindow(HWND hwndListView) {
  HWND hwndHeader = ListView_GetHeader(hwndListView);
  if (!hwndHeader || !IsWindowVisible(hwndHeader)) {
    return E_UNEXPECTED;
  }
  const int columns = Header_GetItemCount(hwndHeader);
  if (columns == 0) {
    return E_UNEXPECTED;
  }
  std::vector<int> width(columns);
  int wTotal = 0, wMin = INT_MAX;
  for (int i = 0; i < columns; ++i) {
    int w = ListView_GetMaxColumnWidth(hwndListView, hwndHeader, i);
    width[i] = w;
    wTotal += w;
    if (w < wMin) {
      wMin = w;
    }
  }
  // client
  RECT rcClient;
  ::GetClientRect(hwndListView, &rcClient);
  const int wClient = rcClient.right - rcClient.left;
  if (wClient < wTotal) {            // �N���C�A���g��������Ȃ�
    if (wMin * columns > wClient) {  // ��ԋ����J�������������B���߂ہB
      for (int i = 0; i < columns; ++i) {
        width[i] = wClient / columns;
      }
    } else {
      int remainClient = wClient - wMin * columns;
      int remainTotal = wTotal - wMin * columns;
      for (int i = 0; i < columns; ++i) {
        width[i] = wMin + ((width[i] - wMin) * remainClient / remainTotal);
      }
    }
  } else {                               // �N���C�A���g���͏\��
    for (int i = 0; i < columns; ++i) {  // ����܂�L���Ȃ肷���Ă�����̂ŁA1.5�{�łƂ߂Ă���
      width[i] = mew::math::min(width[i] * 3 / 2, width[i] * wClient / wTotal);
    }
  }
  for (int i = 0; i < columns; ++i) {
    HDITEM item = {HDI_WIDTH, width[i]};
    Header_SetItem(hwndHeader, i, &item);
  }
  return S_OK;
}

HRESULT afx::ListView_AdjustToItems(HWND hwndListView) {
  HWND hwndHeader = ListView_GetHeader(hwndListView);
  if (!hwndHeader || !IsWindowVisible(hwndHeader)) {
    return E_UNEXPECTED;
  }
  const int columns = Header_GetItemCount(hwndHeader);
  for (int i = 0; i < columns; ++i) {
    NMHEADER notify = {hwndHeader, static_cast<UINT_PTR>(::GetDlgCtrlID(hwndHeader)), HDN_DIVIDERDBLCLICK, i, 0};
    ::SendMessage(hwndListView, WM_NOTIFY, notify.hdr.idFrom, (LPARAM)&notify);
  }
  return S_OK;
}

HRESULT afx::ListView_GetSortKey(HWND hwndListView, int* column, bool* ascending) {
  ASSERT(column);
  ASSERT(ascending);
  *column = -1;
  HWND hwndHeader = ListView_GetHeader(hwndListView);
  if (!::IsWindow(hwndHeader)) {
    return E_INVALIDARG;
  }
  const int headerCount = Header_GetItemCount(hwndHeader);
  *column = ListView_GetSelectedColumn(hwndListView);
  *ascending = false;
  // SelectedColumn �� Header �̊֌W�����������Ȃ��Ă���ꍇ������̂�
  for (int i = 0; i < headerCount; ++i) {
    HDITEM info = {HDI_FORMAT};
    Header_GetItem(hwndHeader, i, &info);
    if (i == *column) {
      if ((info.fmt & (HDF_SORTUP | HDF_SORTDOWN)) == 0) {  // SelectedColumn �Ȃ̂ɂɃ\�[�g�}�[�N���t���Ă��Ȃ��I
        *column = -1;                                       // invalid
        return E_UNEXPECTED;
      }
      *ascending = ((info.fmt & HDF_SORTDOWN) == HDF_SORTDOWN);
    } else if (info.fmt & (HDF_SORTUP | HDF_SORTDOWN)) {  // SelectedColumn �ȊO�Ƀ\�[�g�}�[�N���t���Ă���I
      *column = -1;                                       // invalid
      return E_UNEXPECTED;
    }
  }
  return S_OK;
}

HRESULT afx::ListView_SetSortKey(HWND hwndListView, int column, bool ascending) {
  if (column < 0) {
    return E_INVALIDARG;
  }

  HWND hwndOwner = ::GetParent(hwndListView);
  HWND hwndHeader = ListView_GetHeader(hwndListView);
  if (!::IsWindow(hwndHeader)) {
    return E_INVALIDARG;
  }
  if (column >= Header_GetItemCount(hwndHeader)) {
    return E_INVALIDARG;
  }
  for (int i = 0; i < 10; ++i) {  // �ςȂ��Ƃ��N�����Ė������[�v�ɂȂ�̂�h�����߁B
    NMLISTVIEW notify = {hwndListView, static_cast<UINT_PTR>(::GetDlgCtrlID(hwndListView)), LVN_COLUMNCLICK, -1, column};
    ::SendMessage(hwndOwner, WM_NOTIFY, notify.hdr.idFrom, (LPARAM)&notify);
    int c;
    bool a;
    if (SUCCEEDED(ListView_GetSortKey(hwndListView, &c, &a)) && c == column && a == ascending) {
      break;
    }
  }
  return S_OK;
}

HRESULT afx::ListView_SelectReverse(HWND hwndListView) {
  if (!::IsWindow(hwndListView)) {
    return E_POINTER;
  }
  int count = ListView_GetItemCount(hwndListView);
  int focus = ListView_GetNextItem(hwndListView, -1, LVNI_FOCUSED);
  int focusNext = INT_MAX;
  for (int i = 0; i < count; i++) {
    if (ListView_GetItemState(hwndListView, i, LVIS_SELECTED) == LVIS_SELECTED) {
      ListView_SetItemState(hwndListView, i, 0, LVIS_SELECTED);
    } else {
      ListView_SetItemState(hwndListView, i, LVIS_SELECTED, LVIS_SELECTED);
      if (mew::math::abs(focusNext - focus) > mew::math::abs(i - focus)) {
        focusNext = i;
      }
    }
  }
  // �ȑO�̃t�H�[�J�X�ɍł��߂��A�I�𒆂̍��ڂ��t�H�[�J�X����B
  if (focusNext < count) {
    ListView_SetItemState(hwndListView, focusNext, LVIS_FOCUSED, LVIS_FOCUSED);
    ListView_EnsureVisible(hwndListView, focusNext, false);
  }
  return S_OK;
}

//==============================================================================
// �R���{�{�b�N�X.

bool afx::ComboBoxEx_IsAutoCompleting(HWND hwndComboBoxEx) {
  HWND hwndEdit = ComboBoxEx_GetEdit(hwndComboBoxEx);
  HWND hwndCombo = ComboBoxEx_GetCombo(hwndComboBoxEx);
  RECT rc;
  ::GetWindowRect(hwndEdit, &rc);
  // TODO: �E�B���h�E�����̂ق��ɂ���ƁA�ۊǃE�B���h�E���㑤�ɏo��B
  // ���ꂾ���ł͔���ł��Ȃ��ہB
  POINT pt = {(rc.left + rc.right) / 2, rc.bottom + 1};
  HWND hwnd = WindowFromPoint(pt);
  TCHAR classname[64];
  return hwnd != hwndCombo && hwnd != ::GetParent(hwndComboBoxEx) &&
         ::GetWindowThreadProcessId(hwnd, NULL) == ::GetCurrentThreadId() && ::GetClassName(hwnd, classname, 64) &&
         mew::str::equals(classname, _T("SysListView32"));
}

HRESULT afx::ILGetDisplayName(IShellFolder* folder, LPCITEMIDLIST leaf, DWORD shgdn, WCHAR name[], size_t bufsize) {
  STRRET strret = {0};
  HRESULT hr = folder->GetDisplayNameOf(leaf, shgdn, &strret);
  if FAILED (hr) {
    return hr;
  }
  return StrRetToBuf(&strret, leaf, name, bufsize);
}

namespace {
inline bool eq(WCHAR lhs, WCHAR rhs) { return ChrCmpI(lhs, rhs) == 0; }
}  // namespace

// from http://www.codeproject.com/string/wildcmp.asp
/*
bool afx::PatternEquals(PCWSTR wild, PCWSTR text)
{
        PCWSTR cp = 0;
        PCWSTR mp = 0;

        while((*text) && (*wild != '*'))
        {
                if(!eq(*wild, *text) && (*wild != '?'))
                        return false;
                wild = str::inc(wild);
                text = str::inc(text);
        }
        while(*text)
        {
                if(*wild == '*')
                {
                        wild = str::inc(wild);
                        if(!*wild)
                                return true;
                        mp = wild;
                        cp = str::inc(text);
                }
                else if(eq(*wild, *text) || (*wild == '?'))
                {
                        wild = str::inc(wild);
                        text = str::inc(text);
                }
                else
                {
                        wild = mp;
                        text = cp;
                        cp = str::inc(cp);
                }
        }
        while(*wild == '*')
        {
                wild = str::inc(wild);
        }
        return !*wild;
}
*/

bool afx::PatternEquals(PCWSTR wild, PCWSTR text_orig) {
  PCWSTR cp = 0;
  PCWSTR mp = 0;
  PCWSTR text = text_orig;

  while (*text && *wild != ';' && *wild != '*') {
    if (!eq(*wild, *text) && (*wild != '?')) {
      return false;
    }
    wild = mew::str::inc(wild);
    text = mew::str::inc(text);
  }
  if (*wild != ';') {
    while (*text) {
      if (*wild == '*') {
        wild = mew::str::inc(wild);
        if (!*wild || *wild == ';') {
          return true;
        }
        mp = wild;
        cp = mew::str::inc(text);
      } else if (eq(*wild, *text) || (*wild == '?')) {
        wild = mew::str::inc(wild);
        text = mew::str::inc(text);
      } else {
        wild = mp;
        text = cp;
        cp = mew::str::inc(cp);
      }
    }
    while (*wild == '*') {
      wild = mew::str::inc(wild);
    }
  }
  if (*wild == ';') {
    return true;
  }
  if (*wild) {
    mp = wild;
    while ((*mp) && (*mp != ';')) {
      mp = mew::str::inc(mp);
    }
    if (*mp == ';') {
      wild = mew::str::inc(mp);
      return PatternEquals(wild, text_orig);
    }
  }

  return !*wild;
}
