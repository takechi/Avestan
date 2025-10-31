// callback.cpp

#include "stdafx.h"
#include "main.hpp"
#include "std/buffer.hpp"
#include "object.hpp"

namespace ave {

MEW_API void GetDriveLetter(PCWSTR path, PWSTR buffer) {
  size_t pathlen = mew::str::length(path);
  PCTSTR pathstr = path;
  if (pathlen > 1 && pathstr[1] == _T(':')) {  // 通常ドライブ(C, D, etc.)
    buffer[0] = pathstr[0];
    buffer[1] = _T('\0');
  } else {  // ネットワークドライブ
    while (*pathstr == _T('\\')) {
      ++pathstr;
    }
    PCTSTR end = mew::str::find(pathstr, _T('\\'));
    if (end) {
      size_t length = end - pathstr;
      lstrcpyn(buffer, pathstr, length + 1);
      buffer[length + 1] = _T('\0');
    } else {  // 多分ありえないが
      lstrcpy(buffer, pathstr);
    }
  }
}

MEW_API UINT64 GetTotalBytes(mew::ui::IShellListView* view) {
  ASSERT(view);
  if (view->Count == 0) {
    return 0;
  }
  UINT64 total = 0;
  HRESULT hr;
  mew::ref<mew::io::IEntryList> entries;
  if FAILED (hr = view->GetContents(&entries, mew::StatusNone)) {
    return hr;
  }
  mew::ref<IShellFolder> folder;
  if FAILED (hr = view->GetFolder(&folder)) {
    return hr;
  }
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

MEW_API UINT64 GetSelectedBytes(mew::ui::IShellListView* view) {
  ASSERT(view);
  if (view->SelectedCount == 0) {
    return 0;
  }
  UINT64 selected = 0;
  HRESULT hr;
  mew::ref<mew::io::IEntryList> entries;
  if FAILED (hr = view->GetContents(&entries, mew::SELECTED)) {
    return hr;
  }
  mew::ref<IShellFolder> folder;
  if FAILED (hr = view->GetFolder(&folder)) {
    return hr;
  }
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

namespace {
const PCTSTR MODS_NAME[] = {
    _T("Default"), _T("Alt"),      _T("Control"),      _T("ControlAlt"),
    _T("Shift"),   _T("ShiftAlt"), _T("ControlShift"), _T("ControlShiftAlt"),
};

inline UINT ModifierToVerbIndex(UINT mods) {
  UINT index = 0;
  if (mods & mew::ui::ModifierShift) {
    index |= 4;
  }
  if (mods & mew::ui::ModifierControl) {
    index |= 2;
  }
  if (mods & mew::ui::ModifierAlt) {
    index |= 1;
  }
  ASSERT(index < 8);
  return index;
}
}  // namespace

class DefaultCallback : public mew::Root<mew::implements<ICallback, mew::IDisposable> > {
 private:
  mew::string m_ExecuteVerbs[8];

 public:
  mew::string Caption(const mew::string& name, const mew::string& path) {
    if (path) {
      return mew::string::load(IDS_TITLE_2, name, path);
    } else if (name) {
      return mew::string::load(IDS_TITLE_1, name);
    } else {
      return mew::string::load(IDS_TITLE_0);
    }
  }
  mew::string StatusText(const mew::string& text, mew::ui::IShellListView* view) {
    if (text) {
      return text;
    }
    int selectedCount = view->SelectedCount;
    int totalCount = view->Count;

    if (selectedCount == 0) {  // 選択なし
      mew::ref<mew::io::IEntry> folder;
      view->GetFolder(&folder);
      mew::string path;
      if (folder) {
        path = folder->Path;
      }
      if (!path) {
        return mew::string::format(_T("$1 個のオブジェクト"), totalCount);
      } else {
        UINT64 uTotalBytes = 0, uFreeBytes = 0;
        TCHAR sDrive[MAX_PATH] = _T("");
        TCHAR sTotalSize[32], sFreeBytes[32];
        if (path) {
          GetDiskFreeSpaceEx(path.str(), (ULARGE_INTEGER*)&uFreeBytes, nullptr, nullptr);
          ave::GetDriveLetter(path.str(), sDrive);
        }
        if (totalCount > 0) {  // totalCount == 0 の場合、まだフォルダを計算中の可能性があり、
          // このときに列挙するとしばらく応答しなくなる場合がある。
          uTotalBytes = ave::GetTotalBytes(view);
        }
        StrFormatByteSize64(uTotalBytes, sTotalSize, 32);
        StrFormatByteSize64(uFreeBytes, sFreeBytes, 32);
        return mew::string::format(_T("$1 個のオブジェクト ( $2 ) / 空きディスク領域 [$3] $4"), totalCount, sTotalSize, sDrive,
                                   sFreeBytes);
      }
    } else {  // 複数選択
      TCHAR sSelectBytes[32];
      StrFormatByteSize64(ave::GetSelectedBytes(view), sSelectBytes, 32);
      return mew::string::format(_T("$1 / $2 個のオブジェクトを選択 ( $3 )"), selectedCount, totalCount, sSelectBytes);
    }
  }
  mew::string GestureText(const mew::ui::Gesture gesture[], size_t length, const mew::string& description) {
    static const PCTSTR GESTURE_CHAR[] = {
        _T("左"), _T("右"), _T("中"), _T("４"), _T("５"), _T("△"), _T("▽"), _T("←"), _T("→"), _T("↑"), _T("↓"),
    };
    mew::StringBuffer text;
    text.reserve(64);
    for (size_t i = 0; i < length; i++) {
      text.append(GESTURE_CHAR[gesture[i]]);
    }
    text.push_back(_T('\0'));
    if (description) {
      return mew::string::format(_T("ジェスチャ ： $1（$2）"), text.str(), description);
    } else {
      return mew::string::format(_T("ジェスチャ ： $1"), text.str());
    }
  }
  avesta::Navigation NavigateVerb(mew::io::IEntry* current, mew::io::IEntry* where, UINT mods, bool locked,
                                  avesta::Navigation defaultVerb) {
    const avesta::Navigation NormalGoto[8] = {
        avesta::NaviGoto, avesta::NaviAppend,     avesta::NaviReserve, avesta::NaviReplace,
        avesta::NaviGoto, avesta::NaviOpenAlways, avesta::NaviSwitch,  avesta::NaviGoto,
    };
    const avesta::Navigation LockedGoto[8] = {
        avesta::NaviAppend, avesta::NaviGoto,   avesta::NaviReserve, avesta::NaviAppend,
        avesta::NaviAppend, avesta::NaviAppend, avesta::NaviSwitch,  avesta::NaviAppend,
    };
    const avesta::Navigation NormalOpen[8] = {
        avesta::NaviOpen, avesta::NaviAppend,     avesta::NaviReserve, avesta::NaviReplace,
        avesta::NaviOpen, avesta::NaviGotoAlways, avesta::NaviSwitch,  avesta::NaviOpen,
    };
    const avesta::Navigation LockedOpen[8] = {
        avesta::NaviAppend, avesta::NaviAppend,     avesta::NaviReserve, avesta::NaviAppend,
        avesta::NaviAppend, avesta::NaviGotoAlways, avesta::NaviSwitch,  avesta::NaviAppend,
    };

    int m = 0;
    if (mods & mew::ui::ModifierControl) {
      m += 1;
    }
    if (mods & mew::ui::ModifierShift) {
      m += 2;
    }
    if (mods & mew::ui::ModifierAlt) {
      m += 4;
    }

    switch (defaultVerb) {
      case avesta::NaviGoto:
        if (locked) {
          return LockedGoto[m];
        } else {
          return NormalGoto[m];
        }
      case avesta::NaviOpen:
        if (locked) {
          return LockedOpen[m];
        } else {
          return NormalOpen[m];
        }
      default:
        return defaultVerb;
    }
  }
  mew::string ExecuteVerb(mew::io::IEntry* current, mew::io::IEntry* what, UINT mods) {
    if (!m_ExecuteVerbs[0]) {
      for (size_t i = 0; i < 8; ++i) {
        m_ExecuteVerbs[i] = theAvesta->GetProfileString(_T("Execute"), MODS_NAME[i], _T(""));
      }
    }
    UINT index = ModifierToVerbIndex(mods);
    return m_ExecuteVerbs[index];
  }
  mew::string WallPaper(const mew::string& filename, const mew::string& name, const mew::string& path) { return filename; }
};

AVESTA_EXPORT(DefaultCallback)
