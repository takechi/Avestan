// TreeView.cpp

#include "stdafx.h"
#include "../private.h"
#include "WindowImpl.h"
#include "std/buffer.hpp"
#include "impl/WTLControls.hpp"
#include "drawing.hpp"
#include "path.hpp"

#include "../server/main.hpp"  // もうぐちゃぐちゃ……

namespace {

const DWORD MEW_WS_TREE =
    WS_CONTROL | WS_VSCROLL | WS_HSCROLL | TVS_HASBUTTONS | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_EDITLABELS;
const DWORD MEW_WS_EX_TREE = WS_EX_CLIENTEDGE;

template <class TFinal, class TParam, class TList>
class __declspec(novtable) TreeViewBase
    : public mew::ui::WindowImpl<WTLEX::CTypedTreeImpl<TFinal, TParam*, CTreeViewCtrlT<mew::ui::WindowImplBase> >, TList> {
 protected:
  using ParamType = TParam*;
  mew::ref<TParam> m_root;
  mew::ref<IImageList> m_image;

 public:  // override
  HTREEITEM UpdateTree(HTREEITEM hNode, ParamType pNode) {
    if (!pNode) {
      return nullptr;
    }
    DeleteChildren(hNode);
    pNode->OnUpdate();
    if (hNode) {
      SetItem(hNode, pNode);
      return hNode;
    } else {
      return InsertItem(NULL, pNode);
    }
  }
  void AddChildren(HTREEITEM hParant, ParamType pParent) {}

 public:
  void HandleDestroy() {
    MakeEmpty();
    m_root.clear();
    m_image.clear();
  }

 public:
  void OnInsertItem(HTREEITEM hItem, ParamType param) {
    if (param) {
      param->AddRef();
    }
  }
  void OnDeleteItem(HTREEITEM hItem, ParamType param) {
    if (param) {
      param->Release();
    }
  }
  void OnResetItem(HTREEITEM hItem, ParamType prev, ParamType next) {
    if (next) {
      next->AddRef();
    }
    if (prev) {
      prev->Release();
    }
  }
  void OnExpanding(HTREEITEM hItem, ParamType param, bool expand) {
    if (expand && GetChildItem(hItem) == NULL) {
      final.AddChildren(hItem, param);
    }
  }
  void OnQueryHasChildren(HTREEITEM hItem, ParamType param, bool& hasChildren) { hasChildren = param->HasChildren(); }
  void OnQueryName(HTREEITEM hItem, ParamType param, PCWSTR& name) { name = param->Name.str(); }
  void OnQueryImage(HTREEITEM hItem, ParamType param, bool selected, int& image) { image = param->Image; }
  void OnSelChanged(HTREEITEM hItem, ParamType param) { InvokeEvent<mew::ui::EventItemFocus>(this, param); }

 public:  // ITree
  mew::ui::ITreeItem* get_Root() { return m_root; }
  void set_Root(mew::ui::ITreeItem* value) {
    mew::ui::SuppressRedraw redraw(m_hWnd);
    m_root = mew::cast(value);
    ASSERT(m_root);
    HTREEITEM hItem = final.UpdateTree(NULL, m_root);
    Expand(hItem);
    SelectItem(hItem);
  }
  IImageList* get_ImageList() { return m_image; }
  void set_ImageList(IImageList* value) {
    m_image = value;
    SetImageList(value);
  }

 public:
  bool SupportsEvent(mew::EventCode code) const throw() {
    if (__super::SupportsEvent(code)) {
      return true;
    }
    switch (code) {
      case mew::ui::EventItemFocus:
        return true;
      default:
        return false;
    }
  }

 public:  // ITreeView
  HRESULT GetContents(mew::REFINTF ppInterface, mew::Status status) {
    switch (status) {
      case mew::FOCUSED:
        return objcpy(GetItemData(GetSelectedItem()), ppInterface);
      default:
        TRESPASS_DBG(return E_NOTIMPL);
    }
  }
  HRESULT GetStatus(IUnknown* item, DWORD* status) { TRESPASS_DBG(return E_NOTIMPL); }
  HRESULT SetStatus(IUnknown* item, mew::Status status) { TRESPASS_DBG(return E_NOTIMPL); }

 public:
  BEGIN_MSG_MAP_(HandleWindowMessage)
  MESSAGE_HANDLER(WM_FORWARDMSG, OnForwardMsg)
  MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMButtonDown)
  MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
  REFLECTED_NOTIFY_CODE_HANDLER(TVN_BEGINLABELEDIT, OnBeginLabelEdit)
  REFLECTED_NOTIFY_CODE_HANDLER(TVN_ENDLABELEDIT, OnEndLabelEdit)
  CHAIN_MSG_MAP_TO(__super::HandleWindowMessage)
  END_MSG_MAP()

  LRESULT OnForwardMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    MSG* msg = (MSG*)lParam;
    if (IsChild(msg->hwnd)) {  // リストビューの子供＝ファイルリネーム中のエディットなので処理を任せる
      return 0;
    }
    bHandled = false;
    return 0;
  }
  LRESULT OnMButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    UINT32 m = theAvesta->MiddleClick;
    if (m != 0) {
      TVHITTESTINFO hit = {GET_XY_LPARAM(lParam)};
      if (HTREEITEM hItem = HitTest(&hit)) {
        SelectDropTarget(hItem);
        // TODO: ユーザがカスタマイズできるように
        UINT32 mods = mew::ui::GetCurrentModifiers();
        if (mods == 0) {
          mods = m;
        }
        afx::SetModifierState(0, mods);
        final.OnExecuteItem(hItem);
        afx::RestoreModifierState(0);
        SelectDropTarget(NULL);
        return true;
      }
    }
    return 0;
  }
  LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    HWND hwndFocus = (HWND)wParam;
    if (!IsChild(hwndFocus)) {
      EndEditLabelNow(FALSE);
    }
    bHandled = false;
    return 0;
  }
  bool OnBeginLabelEdit(HTREEITEM hItem, ParamType param, PCTSTR text) { return true; }
  LRESULT OnBeginLabelEdit(int, LPNMHDR pnmh, BOOL& bHandled) {
    NMTVDISPINFO* disp = (NMTVDISPINFO*)pnmh;
    return !final.OnBeginLabelEdit(disp->item.hItem, (ParamType)disp->item.lParam, disp->item.pszText);
  }
  bool OnEndLabelEdit(HTREEITEM hItem, ParamType param, PCTSTR text) { return false; }
  LRESULT OnEndLabelEdit(int, LPNMHDR pnmh, BOOL& bHandled) {
    NMTVDISPINFO* disp = (NMTVDISPINFO*)pnmh;
    return final.OnEndLabelEdit(disp->item.hItem, (ParamType)disp->item.lParam, disp->item.pszText);
  }
};

}  // namespace

//==============================================================================

namespace mew {
namespace ui {

class TreeView : public TreeViewBase<TreeView, ITreeItem, implements<ITreeView, ITree, IWindow, ISignal, IDisposable> > {
 public:  // override
  void AddChildren(HTREEITEM hParant, ITreeItem* pParent) {
    SuppressRedraw redraw(m_hWnd);
    pParent->OnUpdate();
    size_t count = pParent->GetChildCount();
    for (size_t i = 0; i < count; ++i) {
      ref<ITreeItem> child;
      if SUCCEEDED (pParent->GetChild(&child, i)) {
        InsertItem(hParant, child);
      }
    }
  }

 public:
  void DoCreate(CWindowEx parent) {
    __super::DoCreate(parent, NULL, DirNone, MEW_WS_TREE, MEW_WS_EX_TREE);
    ImmAssociateContext(m_hWnd, null);
  }
  void OnExecuteItem(HTREEITEM hItem) {
    if (ITreeItem* item = GetItemData(hItem)) {
      if (ICommand* command = item->Command) {
        command->Invoke();
      }
    }
  }
  bool OnContextMenu(HTREEITEM hItem, POINT ptScreen) {
    enum IDs {
      ID_CLOSE = 1,
      ID_EXECUTE,
    };
    if (!hItem) {
      return true;
    }
    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, ID_EXECUTE, _T("開く(&O)"));
    int cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RETURNCMD, ptScreen.x, ptScreen.y, m_hWnd, NULL);
    switch (cmd) {
      case ID_EXECUTE:
        OnExecuteItem(hItem);
        break;
    }
    return true;
  }
  void OnDrag(const Notify& notify) {
    // DoDragDrop(&data, DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK | DROPEFFECT_SCROLL, notify->hdr.hwndFrom,
    // notify->ptDrag);
  }

 public:  // msg map
  DECLARE_WND_SUPERCLASS(_T("mew.ui.TreeView"), __super::GetWndClassName());

  // BEGIN_MSG_MAP_(HandleWindowMessage)
  //  CHAIN_MSG_MAP_TO(__super::HandleWindowMessage)
  // END_MSG_MAP()

  // HRESULT Send(message msg) {
  //   if(!m_hWnd) return E_FAIL;
  //   return __super::Send(msg);
  // }
};

}  // namespace ui
}  // namespace mew

//==============================================================================

#include "io.hpp"
#include "ShellFolder.h"

namespace mew {
namespace ui {

class ShellTreeView : public TreeViewBase<ShellTreeView, io::IFolder,
                                          implements<ITreeView, ITree, IWindow, ISignal, IDisposable, IDropTarget> > {
 public:  // override
  void AddChildren(HTREEITEM hParant, io::IFolder* pParent) {
    SuppressRedraw redraw(m_hWnd);
    pParent->OnUpdate();
    size_t count = pParent->GetChildCount();
    for (size_t i = 0; i < count; ++i) {
      ref<io::IFolder> child;
      if SUCCEEDED (pParent->GetChild(&child, i)) {
        InsertItem(hParant, child);
      }
    }
  }
  HRESULT ExpandAndFocus(LPCITEMIDLIST pidl) {
    SuppressRedraw redraw(m_hWnd);
    HRESULT hr = E_FAIL;
    HTREEITEM hTreeItem = GetNextItem(NULL, TVGN_CHILD);
    if (io::IFolder* folder = GetItemData(hTreeItem)) {
      if (ILIsEqual(folder->Entry->ID, pidl)) {
        SelectItem(hTreeItem);
        EnsureVisible(hTreeItem);
        hr = S_OK;
      } else {
        hr = ExpandAndFocus(pidl, hTreeItem);
      }
    }
    return hr;
  }
  HRESULT ExpandAndFocus(LPCITEMIDLIST pidl, HTREEITEM hTreeItem) {
    ASSERT(pidl);
    Expand(hTreeItem);
    for (HTREEITEM hItem = GetNextItem(hTreeItem, TVGN_CHILD); hItem; hItem = GetNextItem(hItem, TVGN_NEXT)) {
      if (io::IFolder* folder = GetItemData(hItem)) {
        LPCITEMIDLIST itemIDL = folder->Entry->ID;
        if (ILIsEqual(itemIDL, pidl)) {
          SelectItem(hItem);
          EnsureVisible(hItem);
          return S_OK;
        } else if (ILIsParent(itemIDL, pidl, false)) {
          if SUCCEEDED (ExpandAndFocus(pidl, hItem)) return S_OK;
          EnsureVisible(hItem);
          return E_FAIL;
        }
      }
    }
    return E_FAIL;
  }
  HRESULT SetStatusByEntry(io::IEntry* entry, Status status) {
    ASSERT(entry);
    switch (status) {
      case FOCUSED:
        return ExpandAndFocus(entry->ID);
      default:
        TRESPASS_DBG(return E_NOTIMPL);
    }
  }
  HRESULT SetStatus(IUnknown* item, Status status) {
    try {
      if (ref<io::IFolder> folder = cast(item)) {
        return SetStatusByEntry(folder->Entry, status);
      } else if (ref<io::IEntry> entry = cast(item)) {
        return SetStatusByEntry(entry, status);
      } else if (string path = cast(item)) {
        return SetStatusByEntry(ref<io::IEntry>(__uuidof(io::Entry), path), status);
      } else {
        ASSERT(0);
        return E_NOTIMPL;
      }
    } catch (mew::exceptions::Error& e) {
      TRACE(e.Message);
      return e.Code;
    }
  }

 public:
  void DoCreate(CWindowEx parent) {
    __super::DoCreate(parent, NULL, DirNone, MEW_WS_TREE, MEW_WS_EX_TREE);
    ImmAssociateContext(m_hWnd, null);
    this->set_Root(ref<ITreeItem>(__uuidof(io::FolderMenu)));
  }
  void Update(bool sync = false) {
    __super::Update(sync);
    if (!m_root) {
      return;
    }
    SuppressRedraw redraw(m_hWnd);
    ref<io::IFolder> selected = GetItemData(GetSelectedItem());
    m_root->Reset();
    Expand(UpdateTree(null, m_root));
    if (selected) {
      ExpandAndFocus(selected->Entry->ID);
    }
  }
  void OnExecuteItem(HTREEITEM hItem) {
    if (ITreeItem* item = GetItemData(hItem)) {
      if (ICommand* command = item->Command) {
        command->Invoke();
      }
    }
  }
  bool OnContextMenu(HTREEITEM hItem, POINT ptScreen) {
    if (!hItem) {
      return true;
    }
    io::IFolder* folder = GetItemData(hItem);
    ref<IContextMenu> menu;
    if SUCCEEDED (folder->Entry->QueryObject(&menu)) {  // 選択アイテムがある場合
      if (CMenu popup = afx::SHBeginContextMenu(menu)) {
        UINT cmd = OnContextMenu_Selection(menu, popup.m_hMenu, ptScreen, hItem);
        // XP 名前の変更(&M) BUG ...
        // じゃないかも。名前の変更は、ビューによって操作が異なるので、デフォルトでは実装されていないのか？
        TCHAR text[MAX_PATH];
        if (popup.GetMenuString(cmd, text, MAX_PATH, MF_BYCOMMAND) > 0 && str::equals(text, SHSTR_MENU_RENAME)) {
          afx::SHEndContextMenu(menu, cmd, m_hWnd);
          Rename(hItem);
        } else {
          afx::SHEndContextMenu(menu, cmd, m_hWnd);
        }
      }
    }
    return 0;
  }
  UINT OnContextMenu_Selection(IContextMenu* menu, CMenuHandle popup, POINT ptScreen, HTREEITEM hItem) {
    enum {
      ID_FOLDER_GO = 100,
    };
    popup.InsertMenu(0, MF_BYPOSITION, ID_FOLDER_GO, SHSTR_MENU_GO);
    popup.InsertMenu(1, MF_BYPOSITION | MF_SEPARATOR);
    popup.SetMenuDefaultItem(0, true);
    SelectDropTarget(hItem);
    UINT cmd = afx::SHPopupContextMenu(menu, popup, ptScreen);
    SelectDropTarget(null);
    switch (cmd) {
      case ID_FOLDER_GO:
        afx::SetModifierState(VK_APPS, GetCurrentModifiers());
        OnExecuteItem(hItem);
        afx::RestoreModifierState(VK_APPS);
        break;
    }
    return cmd;
  }
  void OnDrag(const Notify& notify) {
    // DoDragDrop(&data, DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK | DROPEFFECT_SCROLL, notify->hdr.hwndFrom,
    // notify->ptDrag);
  }

 public:  // msg map
  DECLARE_WND_SUPERCLASS(_T("mew.ui.ShellTreeView"), __super::GetWndClassName());

  // BEGIN_MSG_MAP_(HandleWindowMessage)
  //  CHAIN_MSG_MAP_TO(__super::HandleWindowMessage)
  // END_MSG_MAP()

  HRESULT Send(message msg) {
    if (!m_hWnd) {
      return E_FAIL;
    }
    switch (msg.code) {
      case CommandCut:
        Cut();
        break;
      case CommandCopy:
        Copy();
        break;
      case CommandPaste:
        Paste();
        break;
      case CommandDelete:
        Delete(true);
        break;
      case CommandBury:
        Delete(false);
        break;
      case CommandRename:
        Rename();
        break;
      case CommandProperty:
        Property();
        break;
      default:
        return __super::Send(msg);
    }
    return S_OK;
  }

 public:  // file operations
  HRESULT Cut() {
    HRESULT hr;
    HTREEITEM hItem = GetSelectedItem();
    io::IFolder* folder = GetItemData(hItem);
    if (!folder) {
      return E_UNEXPECTED;
    }
    if FAILED (hr = avesta::FileCut(folder->Entry->Path.str())) {
      return hr;
    }
    SetItemState(hItem, TVIS_CUT, TVIS_CUT);
    return S_OK;
  }
  HRESULT Copy() {
    io::IFolder* folder = GetItemData(GetSelectedItem());
    if (!folder) {
      return E_UNEXPECTED;
    }
    return avesta::FileCopy(folder->Entry->Path.str());
  }
  HRESULT Paste() {
    io::IFolder* folder = GetItemData(GetSelectedItem());
    if (!folder) {
      return E_UNEXPECTED;
    }
    return avesta::FilePaste(folder->Entry->Path.str());
  }
  HRESULT Delete(bool undo) {
    HRESULT hr;
    HTREEITEM hItem = GetSelectedItem();
    io::IFolder* folder = GetItemData(hItem);
    if (!folder) {
      return E_UNEXPECTED;
    }
    ref<io::IEntry> entry = folder->Entry;
    ASSERT(entry);
    TCHAR buffer[MAX_PATH + 1] = {0};
    entry->Path.copyto(buffer, MAX_PATH);
    if FAILED (hr = (undo ? avesta::FileDelete(buffer) : avesta::FileBury(buffer))) {
      return hr;
    }
    if (!entry->Exists()) {  // キャンセルされた時にも S_OK が帰るので、ファイルの存在チェックを行う.
      // TODO: IFolder との同期。
      VERIFY(DeleteItem(hItem));
      return S_OK;
    }
    return S_FALSE;
  }
  HRESULT Property() {
    io::IFolder* folder = GetItemData(GetSelectedItem());
    if (!folder) {
      return E_UNEXPECTED;
    }
    return avesta::ILExecute(folder->Entry->ID, L"properties", null, null, m_hWnd);
  }
  HRESULT Rename(HTREEITEM hItem = null) {
    if (!hItem) {
      hItem = GetSelectedItem();
    }
    if (!hItem) {
      return E_UNEXPECTED;
    }
    SetFocus();
    return EditLabel(hItem) ? S_OK : E_FAIL;
  }
  bool OnBeginLabelEdit(HTREEITEM hItem, ParamType param, PCTSTR text) {
    TRACE(L"OnBeginLabelEdit($1, $2, $3)", (DWORD)hItem, param->Entry->Name, text);
    if (param) {
      WTL::CEdit edit = GetEditControl();
      ref<IShellFolder> psf;
      if SUCCEEDED (param->Entry->QueryObject(&psf)) SHLimitInputEdit(edit, psf);
    }
    return true;
  }
  bool OnEndLabelEdit(HTREEITEM hItem, ParamType param, PCTSTR text) {
    TRACE(L"OnEndLabelEdit($1, $2, $3)", (DWORD)hItem, param->Entry->Name, text);
    if (!text) {
      return true;
    }
    return SUCCEEDED(param->Entry->SetName(text, m_hWnd));
  }

 public:  // IDropTarget
  BOOL ForceSelectDropTarget(HTREEITEM hItem) {
    BOOL bDragFullWindows = FALSE;
    SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &bDragFullWindows, 0);
    if (!bDragFullWindows) {
      SetRedraw(TRUE);
    }
    BOOL result = SelectDropTarget(hItem);
    if (!bDragFullWindows) {
      UpdateWindow();
    }
    return result;
  }
  bool HandleQueryDrop(IDropTarget** ppDropTarget, LPARAM lParam) {
    if (!__super::HandleQueryDrop(ppDropTarget, lParam)) {
      QueryInterface(ppDropTarget);
    }
    return true;
  }
  HTREEITEM ItemFromScreenPoint(int x, int y) const {
    TVHITTESTINFO hit = {x, y};
    ScreenToClient(&hit.pt);
    return HitTest(&hit);
  }
  ref<io::IEntry> EntryFromScreenPoint(int x, int y) const {
    if (HTREEITEM hItem = ItemFromScreenPoint(x, y)) {
      return GetItemData(hItem)->Entry;
    }
    return null;
  }
  STDMETHODIMP DragEnter(IDataObject* src, DWORD key, POINTL pt, DWORD* effect) {
    return ProcessDragEnter(src, EntryFromScreenPoint(pt.x, pt.y), key, effect);
  }
  STDMETHODIMP DragOver(DWORD key, POINTL pt, DWORD* effect) {
    HTREEITEM hItemNext = ItemFromScreenPoint(pt.x, pt.y);
    HTREEITEM hItemPrev = GetNextItem(NULL, TVGN_DROPHILITE);
    if (hItemNext != hItemPrev) {
      ForceSelectDropTarget(hItemNext);
    }
    if (!hItemNext) {
      *effect = 0;
    }
    return ProcessDragOver(EntryFromScreenPoint(pt.x, pt.y), key, effect);
  }
  STDMETHODIMP DragLeave() {
    ForceSelectDropTarget(NULL);
    return ProcessDragLeave();
  }
  STDMETHODIMP Drop(IDataObject* src, DWORD key, POINTL pt, DWORD* effect) {
    HRESULT hr = ProcessDrop(src, EntryFromScreenPoint(pt.x, pt.y), pt, key, effect);
    ForceSelectDropTarget(NULL);
    return hr;
  }
};

AVESTA_EXPORT(TreeView)
AVESTA_EXPORT(ShellTreeView)

}  // namespace ui
}  // namespace mew

/*
void CIEFolderTreeCtrl::SetAttributes(HTREEITEM hItem,LPSHELLFOLDER pFolder,ITEMIDLIST* pidl)
{
        DWORD dwAttributes = SFGAO_DISPLAYATTRMASK | SFGAO_REMOVABLE;
        HRESULT hr = pFolder->GetAttributesOf(1,(const ITEMIDLIST**)&pidl,&dwAttributes);
        if (FAILED(hr))
                return;
        if ((dwAttributes & SFGAO_COMPRESSED) && GetShellSettings().ShowCompColor())
                SetTextColor(hItem,RGB(0,0,255));
        else
                SetDefaultTextColor(hItem);
        if (dwAttributes & SFGAO_GHOSTED)
                SetItemState(hItem,TVIS_CUT,TVIS_CUT);
        else
                SetItemState(hItem,TVIS_CUT,0);
        if (dwAttributes & SFGAO_LINK)
                SetItemState(hItem,INDEXTOOVERLAYMASK(2),TVIS_OVERLAYMASK);
        else
                SetItemState(hItem,0,TVIS_OVERLAYMASK);
        if (dwAttributes & SFGAO_SHARE)
                SetItemState(hItem,INDEXTOOVERLAYMASK(1),TVIS_OVERLAYMASK);
        else
                SetItemState(hItem,0,TVIS_OVERLAYMASK);
}
*/
