#pragma once

#include "mew.hpp"
#include "path.hpp"

//#define AVESTA_PYTHON_INTERFACE

namespace avesta {
//==============================================================================
// .

HMODULE GetDLL();
HWND GetForm();
HWND GetForm(DWORD dwThreadId);

//==============================================================================
// �I�v�V����.

enum BoolOption {
  BoolCheckBox,
  BoolDnDCopyInterDrv,
  BoolDistinguishTab,
  BoolFullRowSelect,
  BoolGestureOnName,
  BoolGridLine,
  BoolLazyExecute,
  BoolLockClose,
  BoolLoopCursor,
  BoolMiddleSingle,
  BoolOpenDups,
  BoolOpenNotify,
  BoolRestoreCond,
  BoolRenameExtension,
  BoolPasteInFolder,
  BoolPython,
  BoolQuietProgress,
  BoolTreeAutoSync,
  BoolTreeAutoReflect,
  NumOfBooleans,
};

bool GetOption(BoolOption what);

//==============================================================================
// �t�@�C������.

HRESULT FileNew(PCWSTR path);
HRESULT FileDup(PCWSTR src, PCWSTR dst);
HRESULT FileMove(PCWSTR src, PCWSTR dst);
HRESULT FileDelete(PCWSTR src);
HRESULT FileBury(PCWSTR src);
HRESULT FileRename(PCWSTR src, PCWSTR dst);

HRESULT FileCut(PCWSTR src);
HRESULT FileCopy(PCWSTR src);
HRESULT FilePaste(PCWSTR dst);

HRESULT FileOperationHack(HWND hwndProgress, HWND hwndOwner);
HRESULT FileDialogSetPath(PCWSTR path);

//==============================================================================
// ���W�X�g��.

HRESULT RegGetDWORD(HKEY hKey, PCWSTR subkey, PCWSTR value, DWORD* outDWORD);
HRESULT RegSetDWORD(HKEY hKey, PCWSTR subkey, PCWSTR value, DWORD inDWORD);
HRESULT RegGetString(HKEY hKey, PCWSTR subkey, PCWSTR value, WCHAR outString[], size_t bufsize = MAX_PATH);
HRESULT RegGetAssocExe(PCWSTR extension, WCHAR exe[MAX_PATH]);

//==============================================================================
// Path and ITEMIDLIST.

HRESULT ILExecute(LPCITEMIDLIST pidl, PCWSTR verb = nullptr, PCWSTR args = nullptr, PCWSTR dir = nullptr, HWND hwnd = nullptr);
HRESULT PathExecute(PCWSTR path, PCWSTR verb = nullptr, PCWSTR args = nullptr, PCWSTR dir = nullptr, HWND hwnd = nullptr);
//HRESULT UrlDownload(PCWSTR url);

//==============================================================================
// �R�}���h���C��.

__interface __declspec(uuid("7756E3FD-567D-4011-950F-4FC5F3AE75D4")) ICommandLine : IUnknown {
  bool Next(PWSTR * option, PWSTR * value);
  void Reset();
};

mew::ref<ICommandLine> ParseCommandLine(PCWSTR args);

//==============================================================================
// �E�B���h�E.

HRESULT WindowClose(HWND hwnd);
HRESULT WindowSetFocus(HWND hwnd);

//==============================================================================
// �_�C�A���O.

mew::ref<mew::io::IEntry> PathDialog(mew::string path = mew::null);
HRESULT NameDialog(mew::IString** pp, mew::string path, mew::string name, UINT resID);

//==============================================================================
// UI.

class Thread {
 public:
  using Routine = unsigned int(__stdcall*)(void*);
  static HANDLE New(Routine fn, void* args = nullptr);
  static int Run(Routine fn, void* args = nullptr);
  static int Loop(HWND hwnd);
  static bool IsLocalLoop();
};

//==============================================================================

class Window {
 protected:
  HWND m_hwnd;
  DWORD m_flags;

 public:
  Window();
  virtual ~Window();

  operator HWND() const { return m_hwnd; }

  bool Attach(HWND hwnd);
  HWND Detach();

  HWND Create(WNDCLASSEX wc, HWND parent, DWORD dwStyle, DWORD dwStyleEx, INT x = CW_USEDEFAULT, INT y = CW_USEDEFAULT,
              INT w = CW_USEDEFAULT, INT h = CW_USEDEFAULT);
  void Dispose();
  void Close();
  bool Show(INT sw);
  void Move(INT x, INT y, INT w, INT h);
  void MoveToCenter(HWND hwnd = nullptr);
  void Resize(INT w, INT h);

 public:
  static Window* FromHandle(HWND hwnd);
  static Window* FromHandleParent(HWND hwnd);
  static int Broadcast(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0);
  static bool Filter(MSG& msg);

 protected:
  virtual LRESULT OnMessage(UINT msg, WPARAM wParam, LPARAM lParam);
  virtual bool OnFilter(MSG& msg) { return false; }
  virtual bool OnCreate() { return true; }
  virtual bool OnClose() { return true; }
  virtual void OnCommand(UINT what, HWND ctrl) {}
  virtual void OnDispose() {}
  virtual void OnLayout() {}

 private:
  static LRESULT CALLBACK MainProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data);
  static LRESULT CALLBACK StartupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

//==============================================================================

class Dialog : public Window {
 private:
  HWND m_hwndToolTip;

 public:
  Dialog();
  INT_PTR Go(UINT nID, HWND parent = GetForm(), HINSTANCE module = GetDLL());
  BOOL End(INT_PTR result);

  HWND GetItem(UINT nID) const;
  int GetText(UINT nID, PTSTR text, int nMaxCount) const;
  void SetText(UINT nID, PCTSTR text);
  bool GetEnabled(UINT nID) const;
  void SetEnabled(UINT nID, bool enable);
  bool GetChecked(UINT nID) const;
  void SetChecked(UINT nID, bool check);
  void SetTip(UINT nID, PCTSTR text);

 protected:
  bool OnCreate();
  void OnDispose();

 private:
  static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
}  // namespace avesta
