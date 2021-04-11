// FolderList.cpp

#include "stdafx.h"
#include "main.hpp"
#include "std/buffer.hpp"
#include "afx.hpp"
#include "utils.hpp"
#include "object.hpp"

namespace {
HRESULT ShellCopyHere(IShellListView* view) {
  if (!view) return E_POINTER;
  ref<IEntryList> entries;
  HRESULT hr;
  if FAILED (hr = view->GetContents(&entries, SELECTED)) return hr;
  string path = GetPathOfView(view);
  for (size_t i = 0; i < entries->Count; ++i) {
    ref<IEntry> from;
    if FAILED (entries->GetAt(&from, i)) continue;
    string src = from->Path;
    if (!src) continue;
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
static HRESULT DoMoveTo(IEntryList* entries, PCTSTR dst, bool copy) {
  ASSERT(entries);
  StringBuffer srcfiles;
  const size_t count = entries->Count;
  for (size_t i = 0; i < count; ++i) {
    ref<IEntry> entry;
    if SUCCEEDED (entries->GetAt(&entry, i)) {
      if (string path = entry->Path) {
        srcfiles.append(path.str(), path.length() + 1);
      }
    }
  }
  srcfiles.push_back(_T('\0'));
  if (copy)
    return avesta::FileDup(srcfiles, dst);
  else
    return avesta::FileMove(srcfiles, dst);
}
static HRESULT DoMoveTo(IShellListView* src, PCTSTR dst, bool copy) {
  HRESULT hr;
  ref<IEntryList> entries;
  if FAILED (hr = src->GetContents(&entries, SELECTED)) return hr;
  return DoMoveTo(entries, dst, copy);
}
enum ExposeGroup {
  ExposeGroupView,
  ExposeGroupTab,
};
static size_t AddShownToExpose(IExpose* expose, HWND hwndRoot, IWindow* current) {
  size_t shown = 0;
  size_t indexSelected = 0;
  size_t indexCurrent = INT_MAX;
  ref<ITabPanel> tabs;
  theAvesta->GetComponent(&tabs, AvestaTab);
  const size_t count = tabs->Count;
  for (size_t i = 0; i < count; ++i) {
    ref<IWindow> w;
    if (SUCCEEDED(tabs->GetAt(&w, i)) && w->Visible) {
      if (objcmp(current, w)) indexCurrent = i;
      Rect rc;
      HWND hwndFolder = w->Handle;
      ::GetWindowRect(hwndFolder, &rc);
      afx::ScreenToClient(hwndRoot, &rc);
      UINT8 mne = IndexToMnemonic(shown);
      expose->AddRect(i, ExposeGroupView, rc, mne);
      if (indexSelected == 0 && indexCurrent < i) indexSelected = shown;
      ++shown;
    }
  }
  if (shown == 1)
    return indexCurrent + shown;  // もし表示中が一つだけならば、タブの中でその表示中を指すものにカーソルを合わせる
  else
    return indexSelected;  // 複数が表示中ならば、フォーカスの次のビューにカーソルを合わせる
}
static void AddTabToExpose(IExpose* expose, HWND hwndRoot) {
  ref<ITabPanel> tabs;
  theAvesta->GetComponent(&tabs, AvestaTab);
  HWND hwndTab = tabs->Handle;
  const size_t count = tabs->Count;
  for (size_t i = 0; i < count; ++i) {
    Rect rect = tabs->GetTabRect(i);
    afx::MapWindowRect(hwndTab, hwndRoot, &rect);
    UINT8 mne = IndexToMnemonic(10 + i);
    expose->AddRect(i, ExposeGroupTab, rect, mne);
  }
}
void ShellMoveTo(IShellListView* current, bool copy) {
  if (!current || current->SelectedCount == 0) return;

  ref<IExpose> expose(__uuidof(Expose));

  if (copy)
    expose->SetTitle(L"コピー先の指定");
  else
    expose->SetTitle(L"移動先の指定");

  HWND hwndRoot = ::GetAncestor(current->Handle, GA_ROOT);

  // 1週目
  size_t indexSelected = AddShownToExpose(expose, hwndRoot, current);
  // 2週目
  AddTabToExpose(expose, hwndRoot);
  //
  expose->Select(indexSelected);

  UINT32 time = theAvesta->GetExposeTime();
  HRESULT hr = expose->Go(hwndRoot, time);
  ref<IShellListView> dst;
  if (SUCCEEDED(hr) && SUCCEEDED(theAvesta->GetComponent(&dst, AvestaFolder, hr))) {
    if (objcmp(current, dst)) {
      if (copy)
        ShellCopyHere(current);
      else
        ErrorBox(current, _T("送り手と受け手が同じです"));
    } else if (string dstpath = GetPathOfView(dst)) {
      TCHAR dstpath2[MAX_PATH] = {0};
      dstpath.copyto(dstpath2, MAX_PATH);
      DoMoveTo(current, dstpath2, copy);
    }
  }
}
void ShellMoveToOther(IShellListView* current, bool copy) {
  if (!current || current->SelectedCount == 0) return;
  size_t shown = 0;
  ref<IShellListView> dst;
  ref<ITabPanel> tabs;
  theAvesta->GetComponent(&tabs, AvestaTab);
  const size_t count = tabs->Count;
  for (size_t i = 0; i < count; ++i) {
    ref<IShellListView> w;
    if (SUCCEEDED(tabs->GetAt(&w, i)) && w->Visible) {
      if (!objcmp(current, w)) dst = w;
      if (++shown > 2) break;
    }
  }
  if (shown != 2) return ShellMoveTo(current, copy);
  // ちょうど2つ開かれていたので、コピーまたは移動
  if (string dstpath = GetPathOfView(dst)) {
    TCHAR dstpath2[MAX_PATH] = {0};
    dstpath.copyto(dstpath2, MAX_PATH);
    DoMoveTo(current, dstpath2, copy);
  }
}
}  // namespace

class ShellListSink {
 public:
  static HRESULT OnExecuteEntry(message msg) {
    if (ref<IEntry> what = msg["what"]) {
      AvestaExecute(what);
      msg["cancel"] = true;
    }
    return S_OK;
  }
  static HRESULT OnFolderChanging(message msg) {
    HRESULT hr;
    ref<IShellListView> view = msg["from"];
    ref<IEntry> where = msg["where"];
    if (!where) return S_OK;
    ref<IList> parent;
    if FAILED (hr = QueryParent(view, &parent)) return hr;
    Navigation navi = theAvesta->NavigateVerb(view, where, IsLocked(view, parent), NaviGoto);
    switch (navi) {
      case NaviGoto:
      case NaviGotoAlways:
        break;
      case NaviOpen:
      case NaviOpenAlways:
      case NaviAppend:
      case NaviReserve:
      case NaviSwitch:
      case NaviReplace:
        msg["cancel"] = true;
        theAvesta->OpenFolder(where, navi);
        break;
      default:
        msg["cancel"] = true;
        break;
    }
    return S_OK;
  }
  static HRESULT OnUnsupported(message msg) {
    ref<IShellListView> view = msg["from"];
    message what = msg["what"];
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
        SetSelfStatus(view, SELECTED);
        break;
      case AVESTA_Hide:
        SetSelfStatus(view, UNSELECTED);
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
        EntryNameToClipboard(view, SELECTED, IEntry::PATH);
        break;
      case AVESTA_CopyName:
        EntryNameToClipboard(view, SELECTED, IEntry::LEAF_OR_NAME);
        break;
      case AVESTA_CopyBase:
        EntryNameToClipboard(view, SELECTED, IEntry::BASE_OR_NAME);
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
        if (ref<IEntry> folder = GetFolderOfView(view))
          if (SUCCEEDED(avesta::ILExecute(folder->ID, L"open"))) view->Close();
        break;
      case AVESTA_Find:
        if (ref<IEntry> folder = GetFolderOfView(view)) avesta::ILExecute(folder->ID, L"find");
        break;
      case AVESTA_ShowAllFiles:
        view->ShowAllFiles = !view->ShowAllFiles;
        break;
      case AVEOBS_ShowAllFiles:
        what["state"] = (ENABLED | (view->ShowAllFiles ? CHECKED : 0));
        break;
      case AVESTA_AutoArrange:
        view->AutoArrange = !view->AutoArrange;
        break;
      case AVEOBS_AutoArrange:
        what["state"] = (ENABLED | (view->AutoArrange ? CHECKED : 0));
        break;
      case AVESTA_Grouping:
        view->Grouping = !view->Grouping;
        break;
      case AVEOBS_Grouping:
        what["state"] = (ENABLED | (view->Grouping ? CHECKED : 0));
        break;
      default:
        ASSERT(!"Invalid Command on ShellListView");
    }
    return S_OK;
  }

 private:
  static HRESULT SetSelfStatus(IShellListView* view, Status status) {
    ref<IList> parent;
    HRESULT hr = QueryParent(view, &parent);
    if FAILED (hr) return hr;
    return parent->SetStatus(view, status);
  }
  static HRESULT PasteTo(IShellListView* view) {
    if (!view) return E_UNEXPECTED;
    HRESULT hr;
    if (!::IsClipboardFormatAvailable(CF_HDROP)) return E_UNEXPECTED;
    if (view->SelectedCount != 1) {
      ErrorBox(view, string::load(IDS_ERR_NOSELECTFOLDER));
      return E_UNEXPECTED;
    }
    ref<IEntryList> entries;
    if FAILED (hr = view->GetContents(&entries, SELECTED)) return hr;
    ref<IEntry> dst;
    if FAILED (hr = entries->GetAt(&dst, 0)) return hr;
    string dstpath = dst->Path;
    if (!::PathIsDirectory(dstpath.str())) {
      ErrorBox(view, string::load(IDS_ERR_SELECTIONISNONFOLDER));
      return E_UNEXPECTED;
    }
    return avesta::FilePaste(dstpath.str());
  }
  static HRESULT MoveCheckedTo(IShellListView* view, bool copy) {
    if (!view) return E_UNEXPECTED;
    HRESULT hr;
    // dst
    if (view->SelectedCount != 1) {
      ErrorBox(view, string::load(IDS_ERR_NOSELECTFOLDER));
      return E_UNEXPECTED;
    }
    ref<IEntryList> entries;
    if FAILED (hr = view->GetContents(&entries, SELECTED)) return hr;
    ref<IEntry> dst;
    if FAILED (hr = entries->GetAt(&dst, 0)) return hr;
    string dstpath = dst->Path;
    if (!::PathIsDirectory(dstpath.str())) {
      ErrorBox(view, string::load(IDS_ERR_SELECTIONISNONFOLDER));
      return E_UNEXPECTED;
    }
    // src
    ref<IEntryList> checked;
    if FAILED (hr = view->GetContents(&checked, CHECKED)) return hr;
    DoMoveTo(checked, dstpath.str(), copy);
    return S_OK;
  }
  static HRESULT SyncDescendants(IShellListView* view) {
    ref<IEntry> folder = GetFolderOfView(view);
    if (!folder || !folder->Exists()) return E_UNEXPECTED;
    switch (QuestionBox(view, string::load(IDS_SYNCDESC, folder->Path), MB_OKCANCEL)) {
      case IDOK:
        view->Send(CommandSave);
        theAvesta->SyncDescendants(folder);
        return S_OK;
      case IDCANCEL:
      default:
        return E_ABORT;
    }
  }
};

ref<IShellListView> CreateFolderList(IList* parent, IEntry* pFolder) {
  ref<IShellListView> view(__uuidof(ShellListView), parent);
  view->Dock = DirCenter;
  view->Connect(EventUnsupported, &ShellListSink::OnUnsupported);
  view->Connect(EventFolderChanging, &ShellListSink::OnFolderChanging);
  view->Connect(EventExecuteEntry, &ShellListSink::OnExecuteEntry);
  return view;
}
