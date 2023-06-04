// utils.hpp
#pragma once

#include "path.hpp"

namespace ave {
HRESULT EntryNameToClipboard(mew::io::IEntryList* entries, mew::io::IEntry::NameType what);
HRESULT EntryNameToClipboard(mew::ui::IShellListView* view, mew::Status status, mew::io::IEntry::NameType what);

inline mew::ref<mew::io::IEntry> GetFolderOfView(mew::ui::IShellListView* view) {
  if (!view) {
    return mew::null;
  }
  mew::ref<mew::io::IEntry> folder;
  if FAILED (view->GetFolder(&folder)) {
    return mew::null;
  }
  return folder;
}
inline mew::string GetNameOfView(mew::ui::IShellListView* view, mew::io::IEntry::NameType what) {
  if (mew::ref<mew::io::IEntry> folder = GetFolderOfView(view)) {
    return folder->GetName(what);
  } else {
    return mew::null;
  }
}
inline mew::string GetPathOfView(mew::ui::IShellListView* view) { return GetNameOfView(view, mew::io::IEntry::PATH); }

struct WindowRef {
  HWND hWnd;
  WindowRef(const mew::Null&) : hWnd(0) {}
  WindowRef(mew::ui::IWindow* window) : hWnd(window ? window->Handle : 0) {}
  template <class T>
  WindowRef(const mew::ref<T>& window) : hWnd(window ? window->Handle : 0) {}
  WindowRef(HWND hwnd) : hWnd(hwnd) {}
  operator HWND() const { return hWnd; }
};

struct StringRef {
  PCWSTR str;
  StringRef(const mew::string& s) : str(s.str()) {}
  StringRef(PCWSTR s) : str(s) {}
  operator PCWSTR() const { return str; }
};

inline int MessageBox(HWND hWnd, PCTSTR text, UINT type = MB_OK) {
  TCHAR caption[MAX_PATH] = _T("");
  HWND hParent = ::GetAncestor(hWnd, GA_ROOT);
  if (hParent) {
    ::GetWindowText(hParent, caption, MAX_PATH);
  }
  return ::MessageBox(hParent, text, caption, type);
}
inline int InfoBox(WindowRef window, StringRef text, UINT type = MB_OK) {
  return MessageBox(window, text, type | MB_ICONINFORMATION);
}
inline int QuestionBox(WindowRef window, StringRef text, UINT type = MB_OK) {
  return MessageBox(window, text, type | MB_ICONQUESTION);
}
inline int WarningBox(WindowRef window, StringRef text, UINT type = MB_OK) {
  return MessageBox(window, text, type | MB_ICONWARNING);
}
inline int ErrorBox(WindowRef window, StringRef text, UINT type = MB_OK) {
  return MessageBox(window, text, type | MB_ICONERROR);
}
}  // namespace ave
