// FolderList.cpp

#include "stdafx.h"
#include "main.hpp"
#include "std/buffer.hpp"
#include "afx.hpp"
#include "utils.hpp"
#include "object.hpp"

namespace {
HRESULT ShellCopyHere(mew::ui::IShellListView* view) {
  if (!view) {
    return E_POINTER;
  }
  mew::ref<mew::io::IEntryList> entries;
  HRESULT hr;
  if FAILED (hr = view->GetContents(&entries, mew::SELECTED)) {
    return hr;
  }
  mew::string path = ave::GetPathOfView(view);
  for (size_t i = 0; i < entries->Count; ++i) {
    mew::ref<mew::io::IEntry> from;
    if FAILED (entries->GetAt(&from, i)) {
      continue;
    }
    mew::string src = from->Path;
    if (!src) {
      continue;
    }
    WCHAR dst[MAX_PATH];
    if (PathMakeUniqueName(dst, MAX_PATH, NULL, PathFindFileNameW(src.str()), path.str())) {
      WCHAR src2null[MAX_PATH] = {0};  // 末端はダブルNULLが必要なため。
      src.copyto(src2null, MAX_PATH);
      avesta::FileDup(src2null, dst);
    }
  }
  return S_OK;
}

inline UINT8 IndexToMnemonic(size_t index) {
  if (index < 9)
    return (UINT8)(_T('1') + index);
  else if (index == 9)
    return _T('0');
  else if (index < 10 + 26)
    return (UINT8)(_T('A') + index - 10);
  else
    return 0;
}
static HRESULT DoMoveTo(mew::io::IEntryList* entries, PCTSTR dst, bool copy) {
  ASSERT(entries);
  mew::StringBuffer srcfiles;
  const size_t count = entries->Count;
  for (size_t i = 0; i < count; ++i) {
    mew::ref<mew::io::IEntry> entry;
    if SUCCEEDED (entries->GetAt(&entry, i)) {
      if (mew::string path = entry->Path) {
        srcfiles.append(path.str(), path.length() + 1);
      }
    }
  }
  srcfiles.push_back(_T('\0'));
  if (copy) {
    return avesta::FileDup(srcfiles, dst);
  } else {
    return avesta::FileMove(srcfiles, dst);
  }
}
static HRESULT DoMoveTo(mew::ui::IShellListView* src, PCTSTR dst, bool copy) {
  HRESULT hr;
  mew::ref<mew::io::IEntryList> entries;
  if FAILED (hr = src->GetContents(&entries, mew::SELECTED)) {
    return hr;
  }
  return DoMoveTo(entries, dst, copy);
}
enum ExposeGroup {
  ExposeGroupView,
  ExposeGroupTab,
};
static size_t AddShownToExpose(mew::ui::IExpose* expose, HWND hwndRoot, mew::ui::IWindow* current) {
  size_t shown = 0;
  size_t indexSelected = 0;
  size_t indexCurrent = INT_MAX;
  mew::ref<mew::ui::ITabPanel> tabs;
  theAvesta->GetComponent(&tabs, avesta::AvestaTab);
  const size_t count = tabs->Count;
  for (size_t i = 0; i < count; ++i) {
    mew::ref<mew::ui::IWindow> w;
    if (SUCCEEDED(tabs->GetAt(&w, i)) && w->Visible) {
      if (objcmp(current, w)) {
        indexCurrent = i;
      }
      mew::Rect rc;
      HWND hwndFolder = w->Handle;
      ::GetWindowRect(hwndFolder, &rc);
      afx::ScreenToClient(hwndRoot, &rc);
      UINT8 mne = IndexToMnemonic(shown);
      expose->AddRect(i, ExposeGroupView, rc, mne);
      if (indexSelected == 0 && indexCurrent < i) {
        indexSelected = shown;
      }
      ++shown;
    }
  }
  if (shown == 1) {
    return indexCurrent + shown;  // もし表示中が一つだけならば、タブの中でその表示中を指すものにカーソルを合わせる
  } else {
    return indexSelected;  // 複数が表示中ならば、フォーカスの次のビューにカーソルを合わせる
  }
}
static void AddTabToExpose(mew::ui::IExpose* expose, HWND hwndRoot) {
  mew::ref<mew::ui::ITabPanel> tabs;
  theAvesta->GetComponent(&tabs, avesta::AvestaTab);
  HWND hwndTab = tabs->Handle;
  const size_t count = tabs->Count;
  for (size_t i = 0; i < count; ++i) {
    mew::Rect rect = tabs->GetTabRect(i);
    afx::MapWindowRect(hwndTab, hwndRoot, &rect);
    UINT8 mne = IndexToMnemonic(10 + i);
    expose->AddRect(i, ExposeGroupTab, rect, mne);
  }
}
void ShellMoveTo(mew::ui::IShellListView* current, bool copy) {
  if (!current || current->SelectedCount == 0) {
    return;
  }

  mew::ref<mew::ui::IExpose> expose(__uuidof(mew::ui::Expose));

  if (copy) {
    expose->SetTitle(L"コピー先の指定");
  } else {
    expose->SetTitle(L"移動先の指定");
  }

  HWND hwndRoot = ::GetAncestor(current->Handle, GA_ROOT);

  // 1週目
  size_t indexSelected = AddShownToExpose(expose, hwndRoot, current);
  // 2週目
  AddTabToExpose(expose, hwndRoot);
  //
  expose->Select(indexSelected);

  UINT32 time = theAvesta->GetExposeTime();
  HRESULT hr = expose->Go(hwndRoot, time);
  mew::ref<mew::ui::IShellListView> dst;
  if (SUCCEEDED(hr) && SUCCEEDED(theAvesta->GetComponent(&dst, avesta::AvestaFolder, hr))) {
    if (objcmp(current, dst)) {
      if (copy) {
        ShellCopyHere(current);
      } else {
        ave::ErrorBox(current, _T("送り手と受け手が同じです"));
      }
    } else if (mew::string dstpath = ave::GetPathOfView(dst)) {
      TCHAR dstpath2[MAX_PATH] = {0};
      dstpath.copyto(dstpath2, MAX_PATH);
      DoMoveTo(current, dstpath2, copy);
    }
  }
}
void ShellMoveToOther(mew::ui::IShellListView* current, bool copy) {
  if (!current || current->SelectedCount == 0) {
    return;
  }
  size_t shown = 0;
  mew::ref<mew::ui::IShellListView> dst;
  mew::ref<mew::ui::ITabPanel> tabs;
  theAvesta->GetComponent(&tabs, avesta::AvestaTab);
  const size_t count = tabs->Count;
  for (size_t i = 0; i < count; ++i) {
    mew::ref<mew::ui::IShellListView> w;
    if (SUCCEEDED(tabs->GetAt(&w, i)) && w->Visible) {
      if (!objcmp(current, w)) {
        dst = w;
      }
      if (++shown > 2) {
        break;
      }
    }
  }
  if (shown != 2) {
    return ShellMoveTo(current, copy);
  }
  // ちょうど2つ開かれていたので、コピーまたは移動
  if (mew::string dstpath = ave::GetPathOfView(dst)) {
    TCHAR dstpath2[MAX_PATH] = {0};
    dstpath.copyto(dstpath2, MAX_PATH);
    DoMoveTo(current, dstpath2, copy);
  }
}
}  // namespace

class ShellListSink {
 public:
  static HRESULT OnExecuteEntry(mew::message msg) {
    if (mew::ref<mew::io::IEntry> what = msg["what"]) {
      avesta::AvestaExecute(what);
      msg["cancel"] = true;
    }
    return S_OK;
  }
  static HRESULT OnFolderChanging(mew::message msg) {
    HRESULT hr;
    mew::ref<mew::ui::IShellListView> view = msg["from"];
    mew::ref<mew::io::IEntry> where = msg["where"];
    if (!where) {
      return S_OK;
    }
    mew::ref<mew::ui::IList> parent;
    if FAILED (hr = QueryParent(view, &parent)) {
      return hr;
    }
    avesta::Navigation navi = theAvesta->NavigateVerb(view, where, IsLocked(view, parent), avesta::NaviGoto);
    switch (navi) {
      case avesta::NaviGoto:
      case avesta::NaviGotoAlways:
        break;
      case avesta::NaviOpen:
      case avesta::NaviOpenAlways:
      case avesta::NaviAppend:
      case avesta::NaviReserve:
      case avesta::NaviSwitch:
      case avesta::NaviReplace:
        msg["cancel"] = true;
        theAvesta->OpenFolder(where, navi);
        break;
      default:
        msg["cancel"] = true;
        break;
    }
    return S_OK;
  }
  static HRESULT OnUnsupported(mew::message msg) {
    mew::ref<mew::ui::IShellListView> view = msg["from"];
    mew::message what = msg["what"];
    switch (what.code) {
      case AVESTA_New:
        DlgNew(view);
        break;
      case AVESTA_NewFolder:
        NewFolder(view);
        break;
      case AVESTA_SelectPattern:
        DlgSelect(view);
        break;
      case AVESTA_CopyHere:
        ShellCopyHere(view);
        break;
      case AVESTA_Show:
        SetSelfStatus(view, mew::SELECTED);
        break;
      case AVESTA_Hide:
        SetSelfStatus(view, mew::UNSELECTED);
        break;
      case AVESTA_CopyTo:
        ShellMoveTo(view, true);
        break;
      case AVESTA_MoveTo:
        ShellMoveTo(view, false);
        break;
      case AVESTA_CopyToOther:
        ShellMoveToOther(view, true);
        break;
      case AVESTA_MoveToOther:
        ShellMoveToOther(view, false);
        break;
      case AVESTA_PasteTo:
        PasteTo(view);
        break;
      case AVESTA_CopyCheckedTo:
        MoveCheckedTo(view, true);
        break;
      case AVESTA_MoveCheckedTo:
        MoveCheckedTo(view, false);
        break;
      case AVESTA_SyncDesc:
        SyncDescendants(view);
        break;
      case AVESTA_CopyPath:
        ave::EntryNameToClipboard(view, mew::SELECTED, mew::io::IEntry::PATH);
        break;
      case AVESTA_CopyName:
        ave::EntryNameToClipboard(view, mew::SELECTED, mew::io::IEntry::LEAF_OR_NAME);
        break;
      case AVESTA_CopyBase:
        ave::EntryNameToClipboard(view, mew::SELECTED, mew::io::IEntry::BASE_OR_NAME);
        break;
      case AVESTA_RenamePaste:
        DlgRename(view, true);
        break;
      case AVESTA_PatternMask:
        DlgPattern(view);
        break;
      case AVESTA_RenameDialog:
        DlgRename(view, false);
        break;
      case AVESTA_Export:
        if (mew::ref<mew::io::IEntry> folder = ave::GetFolderOfView(view)) {
          if (SUCCEEDED(avesta::ILExecute(folder->ID, L"open"))) {
            view->Close();
          }
        }
        break;
      case AVESTA_Find:
        if (mew::ref<mew::io::IEntry> folder = ave::GetFolderOfView(view)) {
          avesta::ILExecute(folder->ID, L"find");
        }
        break;
      case AVESTA_ShowAllFiles:
        view->ShowAllFiles = !view->ShowAllFiles;
        break;
      case AVEOBS_ShowAllFiles:
        what["state"] = (mew::ENABLED | (view->ShowAllFiles ? mew::CHECKED : 0));
        break;
      case AVESTA_AutoArrange:
        view->AutoArrange = !view->AutoArrange;
        break;
      case AVEOBS_AutoArrange:
        what["state"] = (mew::ENABLED | (view->AutoArrange ? mew::CHECKED : 0));
        break;
      case AVESTA_Grouping:
        view->Grouping = !view->Grouping;
        break;
      case AVEOBS_Grouping:
        what["state"] = (mew::ENABLED | (view->Grouping ? mew::CHECKED : 0));
        break;
      default:
        ASSERT(!"Invalid Command on ShellListView");
    }
    return S_OK;
  }

 private:
  static HRESULT SetSelfStatus(mew::ui::IShellListView* view, mew::Status status) {
    mew::ref<mew::ui::IList> parent;
    HRESULT hr = QueryParent(view, &parent);
    if FAILED (hr) {
      return hr;
    }
    return parent->SetStatus(view, status);
  }
  static HRESULT PasteTo(mew::ui::IShellListView* view) {
    if (!view) {
      return E_UNEXPECTED;
    }
    HRESULT hr;
    if (!::IsClipboardFormatAvailable(CF_HDROP)) {
      return E_UNEXPECTED;
    }
    if (view->SelectedCount != 1) {
      ave::ErrorBox(view, mew::string::load(IDS_ERR_NOSELECTFOLDER));
      return E_UNEXPECTED;
    }
    mew::ref<mew::io::IEntryList> entries;
    if FAILED (hr = view->GetContents(&entries, mew::SELECTED)) {
      return hr;
    }
    mew::ref<mew::io::IEntry> dst;
    if FAILED (hr = entries->GetAt(&dst, 0)) {
      return hr;
    }
    mew::string dstpath = dst->Path;
    if (!::PathIsDirectory(dstpath.str())) {
      ave::ErrorBox(view, mew::string::load(IDS_ERR_SELECTIONISNONFOLDER));
      return E_UNEXPECTED;
    }
    return avesta::FilePaste(dstpath.str());
  }
  static HRESULT MoveCheckedTo(mew::ui::IShellListView* view, bool copy) {
    if (!view) {
      return E_UNEXPECTED;
    }
    HRESULT hr;
    // dst
    if (view->SelectedCount != 1) {
      ave::ErrorBox(view, mew::string::load(IDS_ERR_NOSELECTFOLDER));
      return E_UNEXPECTED;
    }
    mew::ref<mew::io::IEntryList> entries;
    if FAILED (hr = view->GetContents(&entries, mew::SELECTED)) {
      return hr;
    }
    mew::ref<mew::io::IEntry> dst;
    if FAILED (hr = entries->GetAt(&dst, 0)) {
      return hr;
    }
    mew::string dstpath = dst->Path;
    if (!::PathIsDirectory(dstpath.str())) {
      ave::ErrorBox(view, mew::string::load(IDS_ERR_SELECTIONISNONFOLDER));
      return E_UNEXPECTED;
    }
    // src
    mew::ref<mew::io::IEntryList> checked;
    if FAILED (hr = view->GetContents(&checked, mew::CHECKED)) {
      return hr;
    }
    DoMoveTo(checked, dstpath.str(), copy);
    return S_OK;
  }
  static HRESULT SyncDescendants(mew::ui::IShellListView* view) {
    mew::ref<mew::io::IEntry> folder = ave::GetFolderOfView(view);
    if (!folder || !folder->Exists()) {
      return E_UNEXPECTED;
    }
    switch (ave::QuestionBox(view, mew::string::load(IDS_SYNCDESC, folder->Path), MB_OKCANCEL)) {
      case IDOK:
        view->Send(mew::ui::CommandSave);
        theAvesta->SyncDescendants(folder);
        return S_OK;
      case IDCANCEL:
      default:
        return E_ABORT;
    }
  }
};

mew::ref<mew::ui::IShellListView> CreateFolderList(mew::ui::IList* parent, mew::io::IEntry* pFolder) {
  mew::ref<mew::ui::IShellListView> view(__uuidof(mew::ui::ShellListView), parent);
  view->Dock = mew::ui::DirCenter;
  view->Connect(mew::ui::EventUnsupported, &ShellListSink::OnUnsupported);
  view->Connect(mew::ui::EventFolderChanging, &ShellListSink::OnFolderChanging);
  view->Connect(mew::ui::EventExecuteEntry, &ShellListSink::OnExecuteEntry);
  return view;
}
