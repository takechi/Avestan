// callback.cpp

#include "stdafx.h"
#include "main.hpp"
#include "std/buffer.hpp"
#include "object.hpp"

namespace ave {
MEW_API void GetDriveLetter(PCWSTR path, PWSTR buffer) {
  size_t pathlen = str::length(path);
  PCTSTR pathstr = path;
  if (pathlen > 1 && pathstr[1] == _T(':')) {  // 通常ドライブ(C, D, etc.)
    buffer[0] = pathstr[0];
    buffer[1] = _T('\0');
  } else {  // ネットワークドライブ
    while (*pathstr == _T('\\')) {
      ++pathstr;
    }
    PCTSTR end = str::find(pathstr, _T('\\'));
    if (end) {
      size_t length = end - pathstr;
      lstrcpyn(buffer, pathstr, length + 1);
      buffer[length + 1] = _T('\0');
    } else {  // 多分ありえないが
      lstrcpy(buffer, pathstr);
    }
  }
}

MEW_API UINT64 GetTotalBytes(IShellListView* view) {
  ASSERT(view);
  if (view->Count == 0) return 0;
  UINT64 total = 0;
  HRESULT hr;
  ref<IEntryList> entries;
  if FAILED (hr = view->GetContents(&entries, StatusNone)) return hr;
  ref<IShellFolder> folder;
  if FAILED (hr = view->GetFolder(&folder)) return hr;
  size_t cnt = entries->Count;
  for (size_t i = 0; i < cnt; ++i) {
    WIN32_FIND_DATA find;
    if SUCCEEDED (SHGetDataFromIDList(folder, entries->Leaf[i], SHGDFIL_FINDDATA, &find, sizeof(WIN32_FIND_DATA))) {
      LARGE_INTEGER sz;
      sz.HighPart = find.nFileSizeHigh;
      sz.LowPart = find.nFileSizeLow;
      total += sz.QuadPart;
    }
  }
  return total;
}

MEW_API UINT64 GetSelectedBytes(IShellListView* view) {
  ASSERT(view);
  if (view->SelectedCount == 0) return 0;
  UINT64 selected = 0;
  HRESULT hr;
  ref<IEntryList> entries;
  if FAILED (hr = view->GetContents(&entries, SELECTED)) return hr;
  ref<IShellFolder> folder;
  if FAILED (hr = view->GetFolder(&folder)) return hr;
  size_t cnt = entries->Count;
  for (size_t i = 0; i < cnt; ++i) {
    WIN32_FIND_DATA find;
    if SUCCEEDED (SHGetDataFromIDList(folder, entries->Leaf[i], SHGDFIL_FINDDATA, &find, sizeof(WIN32_FIND_DATA))) {
      LARGE_INTEGER sz;
      sz.HighPart = find.nFileSizeHigh;
      sz.LowPart = find.nFileSizeLow;
      selected += sz.QuadPart;
    }
  }
  return selected;
}
}  // namespace ave

//==============================================================================

using namespace ave;

namespace {
const PCTSTR MODS_NAME[] = {
    _T("Default"), _T("Alt"), _T("Control"), _T("ControlAlt"), _T("Shift"), _T("ShiftAlt"), _T("ControlShift"), _T("ControlShiftAlt"),
};

inline UINT ModifierToVerbIndex(UINT mods) {
  UINT index = 0;
  if (mods & ModifierShift) index |= 4;
  if (mods & ModifierControl) index |= 2;
  if (mods & ModifierAlt) index |= 1;
  ASSERT(index < 8);
  return index;
}
}  // namespace

class DefaultCallback : public Root<implements<ICallback, IDisposable> > {
 private:
  string m_ExecuteVerbs[8];

 public:
  string Caption(const string& name, const string& path) {
    if (path)
      return string::load(IDS_TITLE_2, name, path);
    else if (name)
      return string::load(IDS_TITLE_1, name);
    else
      return string::load(IDS_TITLE_0);
  }
  string StatusText(const string& text, IShellListView* view) {
    if (text) return text;
    int selectedCount = view->SelectedCount;
    int totalCount = view->Count;

    if (selectedCount == 0) {  // 選択なし
      ref<IEntry> folder;
      view->GetFolder(&folder);
      string path;
      if (folder) path = folder->Path;
      if (!path) {
        return string::format(_T("$1 個のオブジェクト"), totalCount);
      } else {
        UINT64 uTotalBytes = 0, uFreeBytes = 0;
        TCHAR sDrive[MAX_PATH] = _T("");
        TCHAR sTotalSize[32], sFreeBytes[32];
        if (path) {
          GetDiskFreeSpaceEx(path.str(), (ULARGE_INTEGER*)&uFreeBytes, null, null);
          GetDriveLetter(path.str(), sDrive);
        }
        if (totalCount > 0) {  // totalCount == 0 の場合、まだフォルダを計算中の可能性があり、
          // このときに列挙するとしばらく応答しなくなる場合がある。
          uTotalBytes = GetTotalBytes(view);
        }
        StrFormatByteSize64(uTotalBytes, sTotalSize, 32);
        StrFormatByteSize64(uFreeBytes, sFreeBytes, 32);
        return string::format(_T("$1 個のオブジェクト ( $2 ) / 空きディスク領域 [$3] $4"), totalCount, sTotalSize, sDrive, sFreeBytes);
      }
    } else {  // 複数選択
      TCHAR sSelectBytes[32];
      StrFormatByteSize64(GetSelectedBytes(view), sSelectBytes, 32);
      return string::format(_T("$1 / $2 個のオブジェクトを選択 ( $3 )"), selectedCount, totalCount, sSelectBytes);
    }
  }
  string GestureText(const Gesture gesture[], size_t length, const string& description) {
    static const PCTSTR GESTURE_CHAR[] = {
        _T("左"), _T("右"), _T("中"), _T("４"), _T("５"), _T("△"), _T("▽"), _T("←"), _T("→"), _T("↑"), _T("↓"),
    };
    StringBuffer text;
    text.reserve(64);
    for (size_t i = 0; i < length; i++) {
      text.append(GESTURE_CHAR[gesture[i]]);
    }
    text.push_back(_T('\0'));
    if (description)
      return string::format(_T("ジェスチャ ： $1（$2）"), text.str(), description);
    else
      return string::format(_T("ジェスチャ ： $1"), text.str());
  }
  Navigation NavigateVerb(IEntry* current, IEntry* where, UINT mods, bool locked, Navigation defaultVerb) {
    const Navigation NormalGoto[8] = {
        NaviGoto, NaviAppend, NaviReserve, NaviReplace, NaviGoto, NaviOpenAlways, NaviSwitch, NaviGoto,
    };
    const Navigation LockedGoto[8] = {
        NaviAppend, NaviGoto, NaviReserve, NaviAppend, NaviAppend, NaviAppend, NaviSwitch, NaviAppend,
    };
    const Navigation NormalOpen[8] = {
        NaviOpen, NaviAppend, NaviReserve, NaviReplace, NaviOpen, NaviGotoAlways, NaviSwitch, NaviOpen,
    };
    const Navigation LockedOpen[8] = {
        NaviAppend, NaviAppend, NaviReserve, NaviAppend, NaviAppend, NaviGotoAlways, NaviSwitch, NaviAppend,
    };

    int m = 0;
    if (mods & ModifierControl) m += 1;
    if (mods & ModifierShift) m += 2;
    if (mods & ModifierAlt) m += 4;

    switch (defaultVerb) {
      case NaviGoto:
        if (locked)
          return LockedGoto[m];
        else
          return NormalGoto[m];
      case NaviOpen:
        if (locked)
          return LockedOpen[m];
        else
          return NormalOpen[m];
      default:
        return defaultVerb;
    }
  }
  string ExecuteVerb(IEntry* current, IEntry* what, UINT mods) {
    if (!m_ExecuteVerbs[0]) {
      for (size_t i = 0; i < 8; ++i) m_ExecuteVerbs[i] = theAvesta->GetProfileString(_T("Execute"), MODS_NAME[i], _T(""));
    }
    UINT index = ModifierToVerbIndex(mods);
    return m_ExecuteVerbs[index];
  }
  string WallPaper(const string& filename, const string& name, const string& path) { return filename; }
};

AVESTA_EXPORT(DefaultCallback)
