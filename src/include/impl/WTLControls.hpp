// WTLControls.hpp
#pragma once

#ifndef __ATLCTRLS_H__
#error WTLControls.hpp requires atlctrls.h to be included first
#endif

#include "widgets.client.hpp"

namespace WTLEX {
//==============================================================================

template <class TParam, class TItem>
struct CTypedItem : TItem {
  TParam get_Param() { return (TParam)lParam; }
  void set_Param(TParam p) { lParam = (LPARAM)p; }
  __declspec(property(get = get_Param, put = set_Param)) TParam Param;

  CTypedItem() {}
  CTypedItem(UINT mask) { this->mask = mask; }
};

//==============================================================================

template <class TParam = LPARAM, class TBase = CListViewCtrl>
class CTypedList : public TBase {
 public:
  using ParamType = TParam;
  using Item = CTypedItem<TParam, LVITEM>;

 private:
  INT32 m_SelectedColumn;

 public:
  CTypedList() : m_SelectedColumn(-1) {}
  ParamType GetItemData(int nItem) const { return (ParamType) __super::GetItemData(nItem); }
  INT32 GetColumnCount() const { return GetHeader().GetItemCount(); }
  void InsertColumn(int index, PCTSTR text, int width, int align) {
    LVCOLUMN column = {
        LVCF_FMT | LVCF_WIDTH | LVCF_TEXT,
        align,
        width,
        const_cast<PTSTR>(text),
    };
    VERIFY(__super::InsertColumn(index, &column) >= 0);
  }
  void InsertRow(UINT32 row, UINT32 column, ParamType param, UINT mask = LVIF_TEXT | LVIF_PARAM) {
    Item item(mask);
    item.iItem = row;
    item.iSubItem = 0;
    item.pszText = LPSTR_TEXTCALLBACK;
    item.iImage = I_IMAGECALLBACK;
    item.Param = param;
    InsertItem(&item);
    //
    item.mask &= ~(LVIF_PARAM | LVIF_IMAGE);
    for (UINT32 c = 1; c < column; ++c) {
      item.iSubItem = c;
      SetItem(&item);
    }
  }
  void DeleteAllColumn() {
    int nColumnCount = GetColumnCount();
    for (int i = 0; i < nColumnCount; i++) {
      DeleteColumn(0);
    }
  }
  void MakeEmpty() {
    DeleteAllItems();
    DeleteAllColumn();
  }
  // SelectedColumn は、XP以上を要求.
  INT32 GetSelectedColumn() const noexcept { return m_SelectedColumn; }
  void SetSelectedColumn(INT32 column) noexcept {
    if (column < 0 || column >= GetColumnCount()) {
      column = -1;
    }
    m_SelectedColumn = column;
    __super::SetSelectedColumn(m_SelectedColumn);
  }
};

//==============================================================================

template <class TFinal, class TParam, class TClient = CListViewCtrl, class TWinTraits = ATL::CControlWinTraits>
class __declspec(novtable) CTypedListImpl : public mew::ui::CWindowImplEx<TFinal, CTypedList<TParam, TClient>, TWinTraits> {
 public:
  using ParamType = TParam;

  bool m_SortedAscending;

  enum SortFlags {
    SortDefault,
    SortAscending,
    SortDescending,
  };

 public:  // overridable
  void OnInsertItem(int index) {}
  void OnDeleteItem(int index) {}
  void OnExecuteItem() {}
  bool OnContextMenu(mew::Point ptScreen) { return false; }
  void OnGetDispInfo(LVITEMW& item, ParamType param) { TRESPASS(); }
  int CompareElement(ParamType lhs, ParamType rhs, int column) { return lhs < rhs; }

 public:  // operation
  bool IsSortedAscending() const noexcept { return m_SortedAscending; }
  void SortElements() {
    SortParam param(final, GetSelectedColumn(), IsSortedAscending());
    SortItems((PFNLVCOMPARE)CompareItemThunk, (LPARAM)&param);
  }
  void SortElements(INT32 column, SortFlags flags) {
    if (column < 0 || column >= GetColumnCount()) {  // 不正なので、デフォルト (0, ascending) に設定する
      column = 0;
      if (flags == SortDefault) flags = SortAscending;
    }
    bool ascending;
    switch (flags) {
      case SortDefault:
        if (GetSelectedColumn() == column) {
          ascending = !m_SortedAscending;
        } else {
          ascending = true;
        }
        break;
      case SortAscending:
        ascending = true;
        break;
      case SortDescending:
        ascending = false;
        break;
      default:
        ASSERT(0);
        ascending = true;
    }
    SetSelectedColumn(column);
    m_SortedAscending = ascending;
    SortParam param(final, column, ascending);
    SortItems((PFNLVCOMPARE)CompareItemThunk, (LPARAM)&param);
  }

 public:
  BEGIN_MSG_MAP(_)
  // デフォルトハンドラで処理させたいため。フレームワークにまわすとREFLECT_NOTIFICATIONS()してしまう
  MSG_LAMBDA(WM_NOTIFY, { lResult = DefWindowProc(uMsg, wParam, lParam); })
  MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
  MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
  // WindowsXPスタイルを使うとUnicode版が呼ばれるぽい
  REFLECTED_NOTIFY_CODE_HANDLER(LVN_GETDISPINFOW, OnGetDispInfo)
  REFLECTED_NOTIFY_CODE_HANDLER(LVN_INSERTITEM, OnInsertItem)
  REFLECTED_NOTIFY_CODE_HANDLER(LVN_DELETEITEM, OnDeleteItem)
  REFLECTED_NOTIFY_CODE_HANDLER(LVN_COLUMNCLICK, OnColumnClick)
  REFLECTED_NOTIFY_CODE_HANDLER(NM_RETURN, OnExecuteItem)
  REFLECTED_NOTIFY_CODE_HANDLER(NM_DBLCLK, OnExecuteItem)
  CHAIN_MSG_MAP(__super)
  DEFAULT_REFLECTION_HANDLER()
  END_MSG_MAP()

  LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled) {
    MakeEmpty();  // 破棄時には自動的に LVM_DELETEALLITEMS は呼ばれないもよう
    bHandled = false;
    return 0;
  }
  LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    Point ptScreen(GET_XY_LPARAM(lParam));
    if (ptScreen.x < 0 || ptScreen.y < 0) {
      int index = GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
      if (index >= 0) {
        RECT rc;
        GetItemRect(index, &rc, LVIR_BOUNDS);
        ptScreen.x = rc.left;
        ptScreen.y = rc.bottom;
      } else {
        ptScreen.x = ptScreen.y = 0;
      }
      ClientToScreen(&ptScreen);
    }
    bHandled = final.OnContextMenu(ptScreen);
    return 0;
  }
  LRESULT OnInsertItem(int, LPNMHDR pnmh, BOOL&) {
    final.OnInsertItem(((NMLISTVIEW*)pnmh)->iItem);
    return 0;
  }
  LRESULT OnDeleteItem(int, LPNMHDR pnmh, BOOL&) {
    final.OnDeleteItem(((NMLISTVIEW*)pnmh)->iItem);
    return 0;
  }
  struct SortParam {
    TFinal& final;
    int column;
    bool ascending;

    SortParam(TFinal& f, int c, bool a) : final(f), column(c), ascending(a) {}
  };
  LRESULT OnColumnClick(int, LPNMHDR pnmh, BOOL&) {
    NMLISTVIEW* nm = (NMLISTVIEW*)pnmh;
    SortElements(nm->iSubItem, SortDefault);
    return 0;
  }
  LRESULT OnExecuteItem(int, LPNMHDR, BOOL&) {
    if (GetSelectedCount() > 0) {
      final.OnExecuteItem();
    }
    return 0;
  }
  template <typename DISPINFO>
  LRESULT OnGetDispInfo(int, LPNMHDR pnmh, BOOL&) {
    DISPINFO& disp = *(DISPINFO*)pnmh;
    ParamType p = (disp.item.mask & LVIF_PARAM) ? (ParamType)disp.item.lParam : GetItemData(disp.item.iItem);
    final.OnGetDispInfo(disp.item, p);
    return 0;
  }

 private:
  static int __stdcall CompareItemThunk(ParamType lhs, ParamType rhs, SortParam* param) {
    int result = param->final.CompareElement(lhs, rhs, param->column);
    return param->ascending ? result : -result;
  }
};

//==============================================================================

template <class TParam = LPARAM, class TBase = CTreeViewCtrl>
class CTypedTree : public TBase {
 public:
  using ParamType = TParam;
  using Item = CTypedItem<TParam, TVITEM>;

  struct Notify {
    NMHDR hdr;
    UINT action;
    Item itemOld;
    Item itemNew;
    POINT ptDrag;
  };

  STATIC_ASSERT(sizeof(Notify) == sizeof(NMTREEVIEW));

 public:
  ParamType GetItemData(HTREEITEM hItem) const { return hItem ? (ParamType) __super::GetItemData(hItem) : ParamType(); }
  HTREEITEM HitTest(TVHITTESTINFO* info) const { return const_cast<CTypedTree*>(this)->__super::HitTest(info); }
  HTREEITEM InsertItem(HTREEITEM hParent, const Item& item) {
    TVINSERTSTRUCT tvis = {hParent};
    tvis.item = item;
    return __super::InsertItem(&tvis);
  }
  void SetItem(const Item& item) { __super::SetItem(const_cast<Item*>(&item)); }
  void DeleteChildren(HTREEITEM hParent) {
    if (!hParent) {
      MakeEmpty();
    } else {
      Expand(hParent, TVE_COLLAPSE | TVE_COLLAPSERESET);
    }
  }
  void MakeEmpty() { DeleteAllItems(); }
  template <class Op>
  bool ForEach(HTREEITEM hParent, Op op) {
    for (HTREEITEM hChild = GetChildItem(hParent); hChild; hChild = GetNextSiblingItem(hChild)) {
      if (!op(hChild)) {
        return false;
      }
    }
    return true;
  }
  WTL::CImageList SetImageList(IImageList* pImageList, int what = TVSIL_NORMAL) {
    if (mew::ref<mew::drawing::IImageList2> image2 = mew::cast(pImageList)) {
      return __super::SetImageList(image2->Normal, what);
    } else {
      return __super::SetImageList((HIMAGELIST)pImageList, what);
    }
  }
  WTL::CImageList SetImageList(HIMAGELIST hImageList, int what = TVSIL_NORMAL) {
    return __super::SetImageList(hImageList, what);
  }
};

//==============================================================================

template <class TFinal, class TParam, class TClient = CTreeViewCtrl, class TWinTraits = ATL::CControlWinTraits>
class __declspec(novtable) CTypedTreeImpl : public mew::ui::CWindowImplEx<TFinal, CTypedTree<TParam, TClient>, TWinTraits> {
 public:  // overridable
  using ParamType = typename CTypedTree<TParam, TClient>::ParamType;
  using Item = typename CTypedTree<TParam, TClient>::Item;
  using Notify = typename CTypedTree<TParam, TClient>::Notify;

  void OnInsertItem(HTREEITEM hItem, ParamType param) {}
  void OnDeleteItem(HTREEITEM hItem, ParamType param) {}
  void OnResetItem(HTREEITEM hItem, ParamType prev, ParamType next) {}
  void OnExpanding(HTREEITEM hItem, ParamType param, bool expand) {}
  void OnExpanded(UINT action, const Item& item) {}
  void OnDrag(const Notify& notify) {}
  void OnSelChanged(HTREEITEM hItem, ParamType param) {}

  void OnQueryHasChildren(HTREEITEM hItem, ParamType param, bool& hasChildren) { TRESPASS(); }
  void OnQueryName(HTREEITEM hItem, ParamType param, PCWSTR& name) { TRESPASS(); }
  void OnQueryImage(HTREEITEM hItem, ParamType param, bool selected, int& image) { image = I_IMAGENONE; }

  void OnExecuteItem(HTREEITEM hItem) {}
  bool OnContextMenu(HTREEITEM hItem, POINT ptScreen) { return false; }

 public:  // operation
  HTREEITEM InsertItem(HTREEITEM hParent, ParamType param) {
    Item item(TVIF_PARAM | TVIF_CHILDREN | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE);
    item.Param = param;
    item.cChildren = I_CHILDRENCALLBACK;
    item.pszText = LPSTR_TEXTCALLBACK;
    item.iImage = item.iSelectedImage = I_IMAGECALLBACK;
    HTREEITEM hItem = __super::InsertItem(hParent, item);
    if (hItem) {
      final.OnInsertItem(hItem, param);
    }
    return hItem;
  }
  void SetItem(const Item& item) { __super::SetItem(item); }
  void SetItem(HTREEITEM hItem, ParamType param) {
    //
    Item item(TVIF_HANDLE | TVIF_PARAM);
    item.hItem = hItem;
    if (GetItem(&item)) {
      final.OnResetItem(hItem, item.Param, param);
    }
    item.mask |= TVIF_CHILDREN | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    item.Param = param;
    item.cChildren = I_CHILDRENCALLBACK;
    item.pszText = LPSTR_TEXTCALLBACK;
    item.iImage = item.iSelectedImage = I_IMAGECALLBACK;
    __super::SetItem(item);
  }

 public:
  BEGIN_MSG_MAP(_)
  // デフォルトハンドラで処理させたいため。フレームワークにまわすとREFLECT_NOTIFICATIONS()してしまう
  MSG_LAMBDA(WM_NOTIFY, { lResult = DefWindowProc(uMsg, wParam, lParam); })
  MSG_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClk)
  MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
  MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
  MESSAGE_HANDLER(WM_RBUTTONUP, OnRButtonUp)
  MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
  REFLECTED_NOTIFY_CODE_HANDLER(TVN_DELETEITEM, OnDeleteItem)
  REFLECTED_NOTIFY_CODE_HANDLER(TVN_ITEMEXPANDING, OnExpanding)
  REFLECTED_NOTIFY_CODE_HANDLER(TVN_ITEMEXPANDED, OnExpanded)
  REFLECTED_NOTIFY_CODE_HANDLER(TVN_GETDISPINFOW, OnGetDispInfo)
  REFLECTED_NOTIFY_CODE_HANDLER(TVN_BEGINDRAG, OnBeginDrag)
  REFLECTED_NOTIFY_CODE_HANDLER(TVN_BEGINRDRAG, OnBeginDrag)
  REFLECTED_NOTIFY_CODE_HANDLER(NM_RETURN, OnItemReturn)
  REFLECTED_NOTIFY_CODE_HANDLER(TVN_KEYDOWN, OnItemKeyDown)
  REFLECTED_NOTIFY_CODE_HANDLER(TVN_SELCHANGED, OnSelChanged)
  CHAIN_MSG_MAP(__super)
  DEFAULT_REFLECTION_HANDLER()
  END_MSG_MAP()

  LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled) {
    MakeEmpty();  // 破棄時には自動的に LVM_DELETEALLITEMS は呼ばれないもよう
    bHandled = false;
    return 0;
  }
  LRESULT OnContextMenu(UINT, WPARAM, LPARAM lParam, BOOL&) {
    mew::Point ptScreen(GET_XY_LPARAM(lParam));
    HTREEITEM hItem;
    if (ptScreen.x == -1 || ptScreen.y == -1) {
      hItem = GetSelectedItem();
      RECT rc;
      GetItemRect(hItem, &rc, true);
      mew::Point pt(rc.left, rc.bottom);
      ClientToScreen(&pt);
      ptScreen = pt;
    } else {
      TVHITTESTINFO hit = {ptScreen.x, ptScreen.y};
      ScreenToClient(&hit.pt);
      hItem = HitTest(&hit);
    }
    final.OnContextMenu(hItem, ptScreen);
    return 0;
  }
  bool OnLButtonDblClk(UINT, WPARAM, LPARAM lParam, LRESULT&) {
    TVHITTESTINFO hit = {GET_XY_LPARAM(lParam)};
    if (HTREEITEM hItem = HitTest(&hit)) {
      switch (hit.flags) {
        case TVHT_ONITEM:
        case TVHT_ONITEMICON:
        case TVHT_ONITEMLABEL:
        case TVHT_ONITEMSTATEICON:
          final.OnExecuteItem(hItem);
          return true;
      }
    }
    return false;
  }
  LRESULT OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) { return 0; }
  LRESULT OnRButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam,
                      BOOL& bHandled) {  // ツリービューではなく、デフォルト（WM_CONTEXTMENUを発生させる）
    ::DefWindowProc(m_hWnd, uMsg, wParam, lParam);
    return 0;
  }
  LRESULT OnItemReturn(int, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {  // デフォルト動作は「ビープを鳴らす」ので、キャンセルする。
    return 1;
  }
  LRESULT OnItemKeyDown(int, LPNMHDR pnmh,
                        BOOL& bHandled) {  // NM_RETURN では、Ctrl+Return を取得できないため、TVN_KEYDOWN をハンドルする。
    NMTVKEYDOWN* key = (NMTVKEYDOWN*)pnmh;
    switch (key->wVKey) {
      case VK_RETURN:
        final.OnExecuteItem(GetSelectedItem());
        return 1;  // non-zero インクリメンタルサーチに含まない
      case VK_SPACE:
        Expand(GetSelectedItem(), TVE_TOGGLE);
        return 1;
      default:
        bHandled = false;
        return 0;
    }
  }
  LRESULT OnDeleteItem(int, LPNMHDR pnmh, BOOL&) {
    Notify* nm = (Notify*)pnmh;
    final.OnDeleteItem(nm->itemOld.hItem, (ParamType)nm->itemOld.lParam);
    return 0;
  }
  LRESULT OnSelChanged(int, LPNMHDR pnmh, BOOL&) {
    Notify* nm = (Notify*)pnmh;
    final.OnSelChanged(nm->itemNew.hItem, (ParamType)nm->itemNew.lParam);
    return 0;
  }
  LRESULT OnExpanding(int, LPNMHDR pnmh, BOOL&) {
    Notify* nm = (Notify*)pnmh;
    HTREEITEM hItem = nm->itemNew.hItem;
    final.OnExpanding(hItem, (ParamType)nm->itemNew.lParam, nm->action == TVE_EXPAND);
    if (!GetNextItem(hItem, TVGN_CHILD)) {  // 開いてみたら、実は子供を持っていなかった
      Item item(TVIF_HANDLE | TVIF_CHILDREN);
      item.hItem = hItem;
      item.cChildren = 0;
      SetItem(item);
    }
    return 0;
  }
  LRESULT OnExpanded(int, LPNMHDR pnmh, BOOL&) {
    Notify* nm = (Notify*)pnmh;
    final.OnExpanded(nm->action, nm->itemNew);
    return 0;
  }
  LRESULT OnGetDispInfo(int, LPNMHDR pnmh, BOOL&) {
    NMTVDISPINFOW& disp = *(NMTVDISPINFOW*)pnmh;
    if (disp.item.mask & TVIF_CHILDREN) {
      bool hasChildren = false;
      final.OnQueryHasChildren(disp.item.hItem, (ParamType)disp.item.lParam, hasChildren);
      disp.item.cChildren = hasChildren;
    }
    if (disp.item.mask & TVIF_TEXT) {
      PCWSTR name = NULL;
      final.OnQueryName(disp.item.hItem, (ParamType)disp.item.lParam, name);
      disp.item.pszText = const_cast<PWSTR>(name);
    }
    if (disp.item.mask & TVIF_IMAGE) {
      final.OnQueryImage(disp.item.hItem, (ParamType)disp.item.lParam, false, disp.item.iImage);
    }
    if (disp.item.mask & TVIF_SELECTEDIMAGE) {
      final.OnQueryImage(disp.item.hItem, (ParamType)disp.item.lParam, true, disp.item.iSelectedImage);
    }
    disp.item.mask |= TVIF_DI_SETITEM;
    return 0;
  }
  LRESULT OnBeginDrag(int, LPNMHDR pnmh, BOOL&) {
    Notify* notify = (Notify*)pnmh;
    final.OnDrag(*notify);
    return 0;
  }
};

//==============================================================================

template <class TParam = LPARAM, class TBase = CTabCtrl>
class CTypedTab : public TBase {
 public:
  using ParamType = TParam;
  using Item = CTypedItem<TParam, TCITEM>;

 public:
  int SetCurSel(int index, bool notify) {
    int result = __super::SetCurSel(index);
    if (result >= 0 && notify) {
      NMHDR nm = {m_hWnd, GetDlgCtrlID(), TCN_SELCHANGE};
      GetParent().SendMessage(WM_NOTIFY, nm.idFrom, (LPARAM)&nm);
    }
    return result;
  }
  ParamType GetItemData(int nItem) const {
    Item item(TCIF_PARAM);
    if (__super::GetItem(nItem, &item)) {
      return item.Param;
    } else {
      return ParamType(0);
    }
  }
  int InsertItem(const Item& item, int index = -1) {
    if (index < 0) {
      index = GetItemCount();
    }
    int result = __super::InsertItem(index, const_cast<Item*>(&item));
    ASSERT(result >= 0);
    return result;
  }
  int InsertItem(PCTSTR text, ParamType param, int index = -1) {
    Item item(TCIF_TEXT | TCIF_PARAM);
    item.pszText = const_cast<PTSTR>(text);
    item.Param = param;
    return InsertItem(item, index);
  }
  void MakeEmpty() {
    DeleteAllItems();
    DeleteAllColumn();
  }
};

//==============================================================================

template <class TFinal, class TParam, class TClient = CTabCtrl, class TWinTraits = ATL::CControlWinTraits>
class __declspec(novtable) CTypedTabImpl : public mew::ui::CWindowImplEx<TFinal, CTypedTab<TParam, TClient>, TWinTraits> {
 public:
  using ParamType = TParam;

 public:  // overridable
  void OnSelChange(int index);
  void OnLeftClick(int tab) {}
  void OnRightClick(int tab) {}
  void OnMiddleClick(int tab) {}
  // void OnInsertItem(int index) {}
  // void OnDeleteItem(int index) {}
  // void OnQueryText(int index, int column, PTSTR pszText, int cchTextMax) {}

 public:  // operation
  int IndexFromParam(ParamType param) noexcept {
    int count = GetItemCount();
    for (int i = 0; i < count; ++i) {
      Item item(TCIF_PARAM);
      if (GetItem(i, &item) && item.Param == param) {
        return i;
      }
    }
    return -1;
  }
  bool GetTabsRect(LPRECT rc) const {
    RECT rcTab;
    GetItemRect(0, &rcTab);  // タブが無くても高さは取れるが、falseが返るもよう
    GetClientRect(rc);
    rc->bottom = rcTab.bottom;
    return true;
  }

 public:
  BEGIN_MSG_MAP(_)
  // デフォルトハンドラで処理させたいため。フレームワークにまわすとREFLECT_NOTIFICATIONS()してしまう
  MSG_LAMBDA(WM_NOTIFY, { lResult = DefWindowProc(uMsg, wParam, lParam); })
  REFLECTED_NOTIFY_CODE_LAMBDA(TCN_SELCHANGE, { final.OnSelChange(GetCurSel()); })
  REFLECTED_NOTIFY_CODE_LAMBDA(NM_CLICK, { final.OnLeftClick(GetCurSel()); })
  REFLECTED_NOTIFY_CODE_HANDLER(NM_RCLICK, OnRClick)
  MESSAGE_HANDLER(WM_MBUTTONUP, OnMButtonUp)
  // REFLECTED_NOTIFY_CODE_HANDLER(LVN_INSERTITEM, OnInsertItem)
  // REFLECTED_NOTIFY_CODE_HANDLER(LVN_DELETEITEM, OnDeleteItem)
  // REFLECTED_NOTIFY_CODE_HANDLER(LVN_COLUMNCLICK, OnColumnClick)
  CHAIN_MSG_MAP(__super)
  DEFAULT_REFLECTION_HANDLER()
  END_MSG_MAP()

  LRESULT OnRClick(int, LPNMHDR pnmh, BOOL&) {
    TCHITTESTINFO info;
    ::GetCursorPos(&info.pt);
    ScreenToClient(&info.pt);
    final.OnRightClick(HitTest(&info));
    return 0;
  }
  LRESULT OnMButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    TCHITTESTINFO info = {GET_XY_LPARAM(lParam)};
    final.OnMiddleClick(HitTest(&info));
    return 0;
  }
  /*
                  LRESULT OnInsertItem(int, LPNMHDR pnmh, BOOL&)
                  {
                          final.OnInsertItem(((NMLISTVIEW*)pnmh)->iItem);
                          return 0;
                  }
                  LRESULT OnDeleteItem(int, LPNMHDR pnmh, BOOL&)
                  {
                          final.OnDeleteItem(((NMLISTVIEW*)pnmh)->iItem);
                          return 0;
                  }
  */
};

//==============================================================================

template <class TBase = CStatusBarCtrl>
class CStatusBar : public TBase {};

//==============================================================================

template <typename TParam = LPARAM, class TBase = CToolBarCtrl>
class CTypedToolBar : public TBase {
 public:
  struct Item : TBBUTTON {
    TParam get_Param() { return (TParam)dwData; }
    void set_Param(TParam p) { dwData = (DWORD_PTR)p; }
    __declspec(property(get = get_Param, put = set_Param)) TParam Param;
  };

 public:
  int GetItemCount() const { return __super::GetButtonCount(); }
  TParam GetItemData(int index) const {
    ASSERT(index >= 0);
    ASSERT(index < GetItemCount());
    TBBUTTONINFO info = {sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_LPARAM};
    if (!GetButtonInfo(index, &info)) return 0;
    return (TParam)info.lParam;
  }
  void SetItemData(int index, TParam param) {
    TBBUTTONINFO info = {sizeof(TBBUTTONINFO)};
    info.dwMask = TBIF_LPARAM | TBIF_BYINDEX;
    info.lParam = (DWORD_PTR)param;
    VERIFY(SetButtonInfo(index, &info));
  }
  BOOL GetItemText_ByCommand(int command, PTSTR buffer, int cch) const {
    TBBUTTONINFO tbbi;
    tbbi.cbSize = sizeof(TBBUTTONINFO);
    tbbi.dwMask = TBIF_TEXT;
    tbbi.pszText = buffer;
    tbbi.cchText = cch;
    return GetButtonInfo(command, &tbbi);
  }
  void SetItemText(int index, PCTSTR text) {
    TBBUTTONINFO info = {sizeof(TBBUTTONINFO)};
    info.dwMask = TBIF_TEXT | TBIF_BYINDEX;
    info.pszText = (PTSTR)text;
    VERIFY(SetButtonInfo(index, &info));
  }
  int IndexFromParam(TParam param) {
    int count = GetButtonCount();
    for (int i = 0; i < count; ++i) {
      if (GetItemData(i) == param) {
        return i;
      }
    }
    return -1;
  }
  bool GetStateOf(int index, UINT state) const {
    TBBUTTONINFO info = {sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_STATE};
    GetButtonInfo(index, &info);
    return (info.fsState & state) != 0;
  }
  bool SetStateOf(int index, UINT state, bool value) {
    TBBUTTONINFO info = {sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_STATE};
    GetButtonInfo(index, &info);
    if (value) {
      info.fsState |= state;
    } else {
      info.fsState &= ~state;
    }
    SetButtonInfo(index, &info);
    return true;
  }
  bool ReverseStateOf(int index, UINT state) {
    TBBUTTONINFO info = {sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_STATE};
    GetButtonInfo(index, &info);
    info.fsState ^= state;
    SetButtonInfo(index, &info);
    return (info.fsState & state) != 0;
  }
  bool GetChecked(int index) const { return GetStateOf(index, TBSTATE_CHECKED); }
  bool SetChecked(int index, bool check) {
    TBBUTTONINFO info = {sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_STATE};
    GetButtonInfo(index, &info);
    if ((info.fsState & TBSTATE_ENABLED) == 0) {
      return false;
    }
    if (check) {
      info.fsState |= TBSTATE_CHECKED;
    } else {
      info.fsState &= ~TBSTATE_CHECKED;
    }
    SetButtonInfo(index, &info);
    return true;
  }
  bool ReverseCheck(int index) {
    TBBUTTONINFO info = {sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_STATE};
    GetButtonInfo(index, &info);
    if ((info.fsState & TBSTATE_ENABLED) == 0) {
      return false;
    }
    info.fsState ^= TBSTATE_CHECKED;
    SetButtonInfo(index, &info);
    return (info.fsState & TBSTATE_CHECKED) != 0;
  }
  bool GetEnabled(int index) const { return GetStateOf(index, TBSTATE_ENABLED); }
  bool SetEnabled(int index, bool value) { return SetStateOf(index, TBSTATE_ENABLED, value); }
  bool ReverseEnable(int index) { return ReverseStateOf(index, TBSTATE_ENABLED); }
  bool IsDropDown(int index) const {
    TBBUTTONINFO info = {sizeof(TBBUTTONINFO), TBIF_STYLE | TBIF_BYINDEX};
    GetButtonInfo(index, &info);
    return (info.fsStyle & (BTNS_DROPDOWN | BTNS_WHOLEDROPDOWN)) != 0;
  }
  bool IsWholeDropDown(int index) const {
    TBBUTTONINFO info = {sizeof(TBBUTTONINFO), TBIF_STYLE | TBIF_BYINDEX};
    GetButtonInfo(index, &info);
    return (info.fsStyle & BTNS_WHOLEDROPDOWN) != 0;
  }
  void MakeEmpty() {
    int nCount = GetButtonCount();
    if (nCount == 0) {
      return;
    }
    mew::ui::SuppressRedraw redraw(m_hWnd);
    for (int i = 0; i < nCount; i++) {
      VERIFY(DeleteButton(0));
    }
  }
  WTL::CImageList SetImageList(IImageList* pImageList, int index = 0) {
    if (mew::ref<mew::drawing::IImageList2> image2 = mew::cast(pImageList)) {
      return SetImageList(image2->Normal, index);
    } else {
      return SetImageList((HIMAGELIST)pImageList, index);
    }
  }
  WTL::CImageList SetImageList(HIMAGELIST hImageList, int index = 0) {
    return (HIMAGELIST)SendMessage(TB_SETIMAGELIST, (WPARAM)index, (LPARAM)hImageList);
  }
};

//==============================================================================

template <class TBase = CReBarCtrl>
class CReBar : public TBase {
 public:
  int FindBand(HWND hWnd) const {
    REBARBANDINFO band = {sizeof(REBARBANDINFO), RBBIM_CHILD};
    int count = GetBandCount();
    for (int i = 0; i < count; ++i) {
      if (GetBandInfo(i, &band) && band.hwndChild == hWnd) {
        return i;
      }
    }
    return -1;
  }
};
}  // namespace WTLEX
