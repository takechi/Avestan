// ShellListView.cpp
#pragma once

#include "stdafx.h"
#include "../private.h"
#include "ShellFolder.h"
#include "WindowImpl.h"
#include <Dbt.h>
#include "ShellNotify.h"
#include "../server/main.hpp"

// #define ENABLE_DEFAULT_GESTURE

namespace mew {
template <>
struct Event<mew::ui::EventFolderChanging> {
  static void event(message& msg, mew::ui::IWindow* from, io::IEntry* where) {
    msg["from"] = from;
    msg["where"] = where;
  }
};
template <>
struct Event<mew::ui::EventFolderChange> {
  static void event(message& msg, mew::ui::IWindow* from, io::IEntry* where) {
    msg["from"] = from;
    msg["where"] = where;
  }
};
template <>
struct Event<mew::ui::EventExecuteEntry> {
  static void event(message& msg, mew::ui::IWindow* from, io::IEntry* what) {
    msg["from"] = from;
    msg["what"] = what;
  }
};
template <>
struct Event<mew::ui::EventStatusText> {
  static void event(message& msg, mew::ui::IWindow* from, string text) {
    msg["from"] = from;
    msg["text"] = text;
  }
};
}

//==============================================================================

namespace mew {
namespace ui {

class ShellListView
    : public WindowImpl<CWindowImplEx<ShellListView, Shell>, implements<IShellListView, IListView, IList, IWindow, ISignal,
                                                                        IDisposable, IGesture, IWallPaper, IDropTarget> >,
      public SHNotifyBase {
 public:
  void QueryMouseGesture(IGesture** pp, Point ptScreen, size_t length, const Gesture gesture[]) {
    // アイテム上でなければジェスチャを受け入れる
    if (!__super::HandleQueryGesture(pp, ptScreen, length, gesture)) {
      QueryInterface(pp);
    }
  }
  bool HandleQueryGesture(IGesture** pp, Point ptScreen, size_t length, const Gesture gesture[]) {
    ListCtrl list = this->ListView;
    if (!list) {
      return true;  // no list => no gesture
    }
    switch (gesture[0]) {
      case GestureButtonLeft:
      case GestureButtonRight:
        break;
      case GestureButtonMiddle:
        if (theAvesta->MiddleClick == 0) {
          QueryMouseGesture(pp, ptScreen, length, gesture);
          return true;
        }
        break;
      default:  // 4, 5 button はアイテムを考慮しない
        QueryMouseGesture(pp, ptScreen, length, gesture);
        return true;
    }
    // アイテム上なら無効
    LVHITTESTINFO hit = {ptScreen, 0, -1, 0};
    list.ScreenToClient(&hit.pt);
    if (list.HitTest(&hit) >= 0 && (hit.flags & (LVHT_ONITEMICON | LVHT_ONITEMLABEL))) {
      return true;  // on item => no gesture
    }
    if (gesture[0] == GestureButtonLeft) {
      HeaderCtrl header = list.GetHeader();
      if (header && header.IsWindowVisible()) {
        if (afx::GetClientArea(header).contains(hit.pt.x, hit.pt.y)) {
          return true;  // on header
        }
        if (!theAvesta->GestureOnName) {
          RECT rcName, rcHeader, rcView;
          header.GetItemRect(0, &rcName);
          header.GetWindowRect(&rcHeader);
          ShellView.GetWindowRect(&rcView);
          hit.pt.x += rcView.left - rcHeader.left;
          if (rcName.left < hit.pt.x && hit.pt.x < rcName.right) {
            return true;  // on name column
          }
        }
      }
    }
    QueryMouseGesture(pp, ptScreen, length, gesture);
    return true;
  }
  HRESULT OnGestureAccept(HWND hWnd, Point ptScreen, size_t length, const Gesture gesture[]) {  // default gesture
#ifdef ENABLE_DEFAULT_GESTURE
    if (length == 1 && gesture[0] == GestureButtonRight) return S_OK;
#endif
    return E_FAIL;
  }
  HRESULT OnGestureUpdate(UINT16 modifiers, size_t length, const Gesture gesture[]) { return S_OK; }
  HRESULT OnGestureFinish(UINT16 modifiers, size_t length, const Gesture gesture[]) {
#ifdef ENABLE_DEFAULT_GESTURE
    if (length == 2) {
      switch (gesture[1]) {
        case GestureEast:
          Go(DirEast);
          return S_OK;
        case GestureWest:
          Go(DirWest);
          return S_OK;
        case GestureNorth:
          Go(DirNorth);
          return S_OK;
        case GestureSouth:
          Close();
          return S_OK;
      }
#endif
      return E_INVALIDARG;
    }

   private:
    using AddressBar = CComboBoxExT<CWindowEx>;
    using EditBox = CContainedWindowT<CEditT<CWindowEx> >;
    using ComboBox = CContainedWindowT<CComboBoxT<CWindowEx> >;

    AddressBar m_Address;
    ComboBox m_ComboBox;
    EditBox m_EditBox;
    bool m_EditBoxAutoComplete;
    ref<IShellStorage> m_pStorage;
    string m_StatusText;
    WTL::CFontHandle m_ListFont;

    enum {
      WM_ASYNC_GOABSOLUTE = WM_USER + 2000,
      TIMER_STATUSTEXT = 0x1234,
      DELAY_STATUSTEXT = 100,  // ms
    };

   public:  // IWindow
    ShellListView() : m_ComboBox(NULL, this, 1), m_EditBox(NULL, this, 2), m_EditBoxAutoComplete(false) {}
    void DoCreate(CWindowEx parent) { __super::DoCreate(parent); }
    bool SupportsEvent(EventCode code) const noexcept {
      if (__super::SupportsEvent(code)) {
        return true;
      }
      switch (code) {
        // case EventListViewExecute:
        // case EventListViewContext:
        case EventFolderChanging:
        case EventFolderChange:
        case EventExecuteEntry:
        case EventStatusText:
          return true;
        default:
          return false;
      }
    }

   public:  // msg map
    DECLARE_WND_CLASS_EX(L"mew.ui.ShellListView", CS_BYTEALIGNWINDOW, -1);

    BEGIN_MSG_MAP_(HandleWindowMessage)
    CHAIN_MSG_MAP_TO(ProcessShellMessage)
    MESSAGE_HANDLER(WM_ASYNC_GOABSOLUTE, OnAsyncGoAbsolute)
    MESSAGE_HANDLER(MEW_NM_SHELL, OnSHChangeNotify)
    MESSAGE_HANDLER(WM_FORWARDMSG, OnForwardMsg)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER(WM_DEVICECHANGE, OnDeviceChange)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    MESSAGE_HANDLER(WM_GETFONT, OnGetFont)
    MESSAGE_HANDLER(WM_SETFONT, OnSetFont)
    MESSAGE_HANDLER(SB_SETTEXTW, OnStatusBarText)
    COMMAND_CODE_HANDLER(CBN_DROPDOWN, OnComboDropDown)
    COMMAND_CODE_HANDLER(CBN_SELENDOK, OnComboSelEndOk)
    COMMAND_CODE_HANDLER(CBN_SELENDCANCEL, OnComboSelEndCancel)
    NOTIFY_CODE_HANDLER(CBEN_ENDEDIT, OnComboEndEdit)
    CHAIN_MSG_MAP_TO(__super::HandleWindowMessage)
    DEFAULT_REFLECTION_HANDLER()
    ALT_MSG_MAP(1)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnComboLRButtonDown)
    MESSAGE_HANDLER(WM_RBUTTONDOWN, OnComboLRButtonDown)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnComboContextMenu)
    MESSAGE_HANDLER(WM_MOUSEWHEEL, OnComboMouseWheel)
    DEFAULT_REFLECTION_HANDLER()
    ALT_MSG_MAP(2)
    MESSAGE_HANDLER(WM_KEYDOWN, OnComboEditKeyDown)
    MESSAGE_HANDLER(WM_CHAR, OnComboEditChar)
    MESSAGE_HANDLER(WM_MOUSEWHEEL, OnComboMouseWheel)
    DEFAULT_REFLECTION_HANDLER()
    END_MSG_MAP()

    void Update(bool sync = false) {
      Shell::Refresh();
      __super::Update(sync);
    }
    HRESULT Send(message msg) {
      if (!m_hWnd) {
        return E_FAIL;
      }
      switch (msg.code) {
        case CommandKeyUp:
          MimicKeyDown(VK_UP, 0);
          break;
        case CommandKeyDown:
          MimicKeyDown(VK_DOWN, 0);
          break;
        case CommandKeyLeft:
          MimicKeyDown(VK_LEFT, 0);
          break;
        case CommandKeyRight:
          MimicKeyDown(VK_RIGHT, 0);
          break;
        case CommandKeyHome:
          MimicKeyDown(VK_HOME, 0);
          break;
        case CommandKeyEnd:
          MimicKeyDown(VK_END, 0);
          break;
        case CommandKeyPageUp:
          MimicKeyDown(VK_PRIOR, 0);
          break;
        case CommandKeyPageDown:
          MimicKeyDown(VK_NEXT, 0);
          break;
        case CommandKeySpace:
          MimicKeyDown(VK_SPACE, 0);
          break;
        case CommandKeyEnter:
          MimicKeyDown(VK_RETURN, 0);
          break;
        case CommandCut:
          Cut();
          break;
        case CommandCopy:
          Copy();
          break;
        case CommandPaste:
          PasteEx();
          break;
        case CommandDelete:
          Delete();
          break;
        case CommandBury:
          Bury();
          break;
        case CommandUndo:
          Undo();
          break;
        case CommandRename:
          Rename();
          break;
        case CommandSelectChecked:
          SelectChecked();
          break;
        case CommandSelectAll:
          SelectAll();
          break;
        case CommandSelectNone:
          SelectNone();
          break;
        case CommandSelectReverse:
          afx::ListView_SelectReverse(this->ListView);
          break;
        case CommandSelectToFirst:
          SelectToFirst();
          break;
        case CommandSelectToLast:
          SelectToLast();
          break;
        case CommandCheckAll:
          CheckAll();
          break;
        case CommandCheckNone:
          CheckNone();
          break;
        case CommandProperty:
          Property();
          break;
        case CommandGoBack:
          Go(DirWest);
          break;
        case CommandGoForward:
          Go(DirEast);
          break;
        case CommandGoUp:
          Go(DirNorth);
          break;
        case CommandSave:
          Shell::SaveViewState();
          break;
        case CommandSetStyle:
          Shell::set_Style((ListStyle)(INT32)msg["style"]);
          break;
        case CommandFocusAddress:
          m_Address.SetFocus();
          break;
        case CommandFocusHeader:
          if (WTL::CHeaderCtrl header = this->ListView.GetHeader()) {
            if (header.IsWindowVisible()) {
              WTL::CListViewCtrl list = this->ListView;
              ref<IExpose> expose(__uuidof(Expose));
              expose->SetTitle(_T("ソート項目の選択"));
              HWND hwndRoot = ::GetAncestor(m_hWnd, GA_ROOT);
              const int count = header.GetItemCount();
              std::vector<int> orders(count);
              for (int i = 0; i < count; ++i) {
                orders[header.OrderToIndex(i)] = i;
              }
              for (int i = 0; i < count; ++i) {
                int index = orders[i];
                RECT rc;
                if (header.GetItemRect(index, &rc)) {
                  afx::MapWindowRect(header, hwndRoot, &rc);
                  UINT8 mne = (i <= 10 ? (UINT8)("1234567890"[i]) : 0);
                  expose->AddRect(index, 0, Rect(rc), mne);
                }
              }
              //
              RECT rcClient, rcHeader;
              list.GetClientRect(&rcClient);
              header.GetWindowRect(&rcHeader);
              rcClient.top += (rcHeader.bottom - rcHeader.top);
              afx::MapWindowRect(list, hwndRoot, &rcClient);
              expose->Select(list.GetSelectedColumn());

              UINT32 time = msg["time"];
              HRESULT hr = expose->Go(hwndRoot, time);
              if SUCCEEDED (hr) {
                if (IsKeyPressed(VK_CONTROL)) {
                  NMHEADER notify = {header, static_cast<UINT_PTR>(header.GetDlgCtrlID()), HDN_DIVIDERDBLCLICK, hr, 0};
                  list.SendMessage(WM_NOTIFY, notify.hdr.idFrom, (LPARAM)&notify);
                } else {
                  SetSortKey(hr, !IsKeyPressed(VK_SHIFT));
                }
                SetFocus();
              }
            }
          }
          break;
        case CommandAdjustToWindow:
          afx::ListView_AdjustToWindow(this->ListView);
          break;
        case CommandAdjustToItem:
          afx::ListView_AdjustToItems(this->ListView);
          break;
        case CommandMenu: {
          BOOL bHandled = false;
          OnContextMenu(WM_CONTEXTMENU, 0, 0xFFFFFFFF, bHandled);
          break;
        }
        default:
          return __super::Send(msg);
      }
      return S_OK;
    }
    void PasteEx() {
      ref<io::IEntryList> entries;
      ref<io::IEntry> item;
      if (theAvesta->PasteInFolder && get_SelectedCount() == 1 && SUCCEEDED(GetContents(&entries, SELECTED)) &&
          SUCCEEDED(entries->GetAt(&item, 0)) && item->IsFolder()) {
        if (string path = item->Path) {
          avesta::FilePaste(path.str());
          return;
        }
      }
      Paste();
    }
    void AsyncGoAbsolute(io::IEntry * entry, GoType go) {
      entry->AddRef();
      PostMessage(WM_ASYNC_GOABSOLUTE, (WPARAM)go, (LPARAM)entry);
    }
    bool HandleQueryDrop(IDropTarget * *pp, LPARAM lParam) {
      if (!__super::HandleQueryDrop(pp, lParam)) QueryInterface(pp);
      return true;
    }
    LRESULT OnAsyncGoAbsolute(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
      SHEndMonitor();
      io::IEntry* entry = (io::IEntry*)lParam;
      GoType go = (GoType)wParam;
      GoAbsolute(entry, go);
      entry->Release();
      return 0;
    }
    LRESULT OnForwardMsg(UINT, WPARAM, LPARAM lParam, BOOL & bHandled) {
      bHandled = m_Address.HasFocus();
      return false;
    }
    bool HandleCreate(const CREATESTRUCT& cs) {
      RECT rc = {0, 0, 0, 480};  // これがドロップダウンリストの最大高さになるもよう
      HIMAGELIST hImageList = afx::ExpGetImageList(16);
      m_Address.Create(m_hWnd, rc, NULL, WS_CONTROL | CBS_DROPDOWN | CBS_AUTOHSCROLL);
      m_Address.SetFont(AtlGetDefaultGuiFont());
      m_Address.SetImageList(hImageList);
      m_Address.SetExtendedStyle(CBES_EX_PATHWORDBREAKPROC, CBES_EX_PATHWORDBREAKPROC);
      m_ComboBox.SubclassWindow(m_Address.GetComboCtrl());
      ASSERT(m_ComboBox.IsWindow());
      m_EditBox.SubclassWindow(m_Address.GetEditCtrl());
      ASSERT(m_EditBox.IsWindow());
      return true;
    }
    void HandleClose() {
      KillTimer(TIMER_STATUSTEXT);
      if (m_ComboBox) m_ComboBox.UnsubclassWindow();
      if (m_EditBox) m_EditBox.UnsubclassWindow();
      SHEndMonitor();
      DisposeShellView();
      m_pStorage.clear();
    }
    void HandleDestroy() {
      HandleClose();
      __super::HandleDestroy();
    }
    bool IsSpecialContextMenu() {
      if (IsKeyPressed(VK_CONTROL) || IsKeyPressed(VK_LBUTTON) || IsKeyPressed(VK_RBUTTON)) {
        return true;
      }
      return false;
    }
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled) {
      ListCtrl list = this->ListView;
      ASSERT(list);
      if (!list) {
        return 0;
      }
      // もう一度デフォルトのコンテキストメニューに戻す
      //__super::DefaultContextMenu(wParam, lParam);
      Point ptScreen(GET_XY_LPARAM(lParam));
      // メニューキーの場合の特別処理
      if (ptScreen.x == -1 || ptScreen.y == -1) {
        int index = list.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
        RECT rc;
        if (index >= 0 && list.GetItemRect(index, &rc, LVIR_BOUNDS)) {
          ptScreen.x = rc.left;
          ptScreen.y = rc.bottom;
        } else {  // 選択アイテムなし
          ptScreen.x = ptScreen.y = 0;
        }
        list.ClientToScreen(&ptScreen);
      }
      bool special = IsSpecialContextMenu();
      ref<IContextMenu> menu;
      CMenu popup;
      if (list.GetSelectedCount() >
          0) {  // アーカイブX は、選択項目が無くても選択項目に対するコンテキストメニューを取得できてしまうため
        popup = afx::SHBeginContextMenu(m_pShellView, SVGIO_SELECTION, &menu);
      }
      if (popup) {  // 選択アイテムがある場合
        UINT cmd = OnContextMenu_Selection(menu, popup.m_hMenu, ptScreen, special);
        // XP 名前の変更(&M) BUG ...
        // じゃないかも。名前の変更は、ビューによって操作が異なるので、デフォルトでは実装されていないのか？
        TCHAR text[MAX_PATH];
        if (popup.GetMenuString(cmd, text, MAX_PATH, MF_BYCOMMAND) > 0 && str::equals(text, SHSTR_MENU_RENAME)) {
          __super::EndContextMenu(menu, 0);
          Rename();
        } else {
          __super::EndContextMenu(menu, cmd);
        }
        return 0;
      } else if (null != (popup = afx::SHBeginContextMenu(m_pShellView, SVGIO_BACKGROUND, &menu))) {  // 選択アイテムが無い場合
        UINT cmd = OnContextMenu_Background(menu, popup.m_hMenu, ptScreen, special);
        __super::EndContextMenu(menu, cmd);
        return 0;
      } else {
        return __super::DefaultContextMenu(wParam, lParam);
      }
    }
    void CopyMenu(HMENU to, HMENU hFromParent, int nFronIndex) {
      HMENU from = ::GetSubMenu(hFromParent, nFronIndex);
      SendMessage(WM_INITMENUPOPUP, (WPARAM)from, MAKELPARAM(nFronIndex, false));
      std::vector<TCHAR> buffer;
      buffer.reserve(MAX_PATH);
      const int count = ::GetMenuItemCount(from);
      for (int i = 0; i < count; ++i) {
        MENUITEMINFO getsize = {sizeof(MENUITEMINFO), MIIM_STRING};
        if (::GetMenuItemInfo(from, i, true, &getsize)) {
          buffer.resize(getsize.cch + 1);
          MENUITEMINFO info = {sizeof(MENUITEMINFO), MIIM_BITMAP | MIIM_CHECKMARKS | MIIM_DATA | MIIM_FTYPE | MIIM_ID |
                                                         MIIM_STATE | MIIM_STRING | MIIM_SUBMENU};
          info.dwTypeData = &buffer[0];
          info.cch = buffer.size();
          if (::GetMenuItemInfo(from, i, true, &info)) {
            ::InsertMenuItem(to, (UINT)-1, true, &info);
          }
        }
      }
    }
    UINT OnContextMenu_Selection(IContextMenu * menu, CMenuHandle popup, Point ptScreen, bool special) {
      if (special) {
        int indexSendTo = -1, indexOpenWith = -1;
        const int count = popup.GetMenuItemCount();
        for (int i = 0; i < count; ++i) {
          TCHAR text[MAX_PATH];
          if (popup.GetMenuString(i, text, MAX_PATH, MF_BYPOSITION) > 0) {
            if (str::equals(text, _T("送る(&N)"))) {
              indexSendTo = i;
            } else if (str::equals(text, _T("プログラムから開く(&H)"))) {
              indexOpenWith = i;
            }
          }
        }
        if (indexSendTo != -1) {
          CMenuHandle menuSendTo = popup.GetSubMenu(indexSendTo);
          SendMessage(WM_INITMENUPOPUP, (WPARAM)(HMENU)menuSendTo, MAKELPARAM(indexSendTo, false));
          if (indexOpenWith != -1) {
            menuSendTo.AppendMenu(MF_SEPARATOR);
            CopyMenu(menuSendTo, popup, indexOpenWith);
          }
          return afx::SHPopupContextMenu(menu, menuSendTo, ptScreen);
        } else if (indexOpenWith != -1) {
          return afx::SHPopupContextMenu(menu, popup.GetSubMenu(indexOpenWith), ptScreen);
        }
      }
      // 通常コンテキストメニュー
      enum {
        ID_FOLDER_GO = 100,
      };
      // 選択中のアイテムがサブフォルダ持ちのフォルダか否かを調べる。
      ref<io::IEntryList> entries;
      ref<io::IEntry> focus;
      if (SUCCEEDED(Shell::GetContents(&entries, FOCUSED)) && SUCCEEDED(entries->GetAt(&focus, 0))) {
        ref<io::IEntry> resolved;
        focus->GetLinked(&resolved);
        if (focus->IsFolder()) {  // フォルダ
          popup.InsertMenu(0, MF_BYPOSITION, ID_FOLDER_GO, SHSTR_MENU_GO);
          popup.InsertMenu(1, MF_BYPOSITION | MF_SEPARATOR);
          popup.SetMenuDefaultItem(0, true);
        } else {  // 一般ファイル
        }
      }
      UINT cmd = afx::SHPopupContextMenu(menu, popup, ptScreen);
      switch (cmd) {
        case ID_FOLDER_GO:
          afx::SetModifierState(VK_APPS, GetCurrentModifiers());
          Go(focus);
          afx::RestoreModifierState(VK_APPS);
          break;
      }
      return cmd;
    }
    UINT OnContextMenu_Background(IContextMenu * menu, CMenuHandle popup, Point ptScreen, bool special) {
      if (special) {
        int indexNew = -1;
        const int count = popup.GetMenuItemCount();
        for (int i = 0; i < count; ++i) {
          TCHAR text[MAX_PATH];
          if (popup.GetMenuString(i, text, MAX_PATH, MF_BYPOSITION) > 0) {
            if (str::equals(text, _T("新規作成(&W)"))) indexNew = i;
          }
        }
        if (indexNew != -1) {
          return afx::SHPopupContextMenu(menu, popup.GetSubMenu(indexNew), ptScreen);
        }
      }
      return afx::SHPopupContextMenu(menu, popup, ptScreen);
    }
    void AddPath(LPCITEMIDLIST pidl, string text) {
      int count = m_Address.GetCount();
      SHFILEINFO info;
      VERIFY(afx::ILGetFileInfo(pidl, &info, SHGFI_SYSICONINDEX | SHGFI_SMALLICON));
      COMBOBOXEXITEM item = {CBEIF_TEXT | CBEIF_INDENT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE};
      item.iItem = count;
      item.iIndent = count;  // インデント単位は文字の幅と同じか？
      item.pszText = (PTSTR)text.str();
      item.iImage = item.iSelectedImage = info.iIcon;
      m_Address.InsertItem(&item);
    }
    bool HitTestComboIcon(Point ptScreen) {
      Point ptEdit = ptScreen;
      m_EditBox.ScreenToClient(&ptEdit);
      return ptEdit.x < 0;
    }
    LRESULT OnComboContextMenu(UINT, WPARAM, LPARAM lParam, BOOL & bHandled) {
      Point ptScreen(GET_XY_LPARAM(lParam));
      if (HitTestComboIcon(ptScreen) && GetCurrentEntry()) {  // on Icon
        ref<IContextMenu> menu;
        if SUCCEEDED (GetCurrentEntry()->QueryObject(&menu)) {  // 選択アイテムがある場合
          if (CMenu popup = afx::SHBeginContextMenu(menu)) {
            UINT cmd = afx::SHPopupContextMenu(menu, popup, ptScreen);
            __super::EndContextMenu(menu, cmd);
          }
        }
      }
      return 0;
    }
    LRESULT OnComboLRButtonDown(UINT, WPARAM, LPARAM lParam, BOOL & bHandled) {
      io::IEntry* entry = GetCurrentEntry();
      if (entry) {
        Point ptClient(GET_XY_LPARAM(lParam));
        Point ptScreen = ptClient;
        m_ComboBox.ClientToScreen(&ptScreen);
        if (HitTestComboIcon(ptScreen)) {
          if (m_ComboBox.DragDetect(ptClient)) {
            ref<io::IDragSource> source(__uuidof(io::DragSource));
            source->AddIDList(entry->ID);
            // TODO: 絵が出ません。
            source->DoDragDrop(io::DropEffectCopy | io::DropEffectMove | io::DropEffectLink);
            // source->DoDragDrop(DropEffectLink);
          }
          // デフォルトでは「エディットにフォーカスを移す」
          // これが悪さをするのでキャンセルする。
          return 0;
        }
      }
      bHandled = false;
      return 0;
    }
    LRESULT OnComboEditKeyDown(UINT, WPARAM wParam, LPARAM, BOOL & bHandled) {
      if (m_EditBoxAutoComplete && m_EditBox.HasFocus() && afx::ComboBoxEx_IsAutoCompleting(m_Address)) {
        bHandled = false;
        return 0;
      }
      switch (wParam) {
        case VK_DOWN:
          m_Address.ShowDropDown();
          m_ComboBox.SetFocus();
          break;
        case VK_TAB:
        case VK_ESCAPE:
          m_Address.SetCurSel(0);
          this->ShellView.SetFocus();
          break;
        default:
          bHandled = false;
      }
      return 0;
    }
    LRESULT OnComboEditChar(UINT, WPARAM wParam, LPARAM, BOOL & bHandled) {
      switch (wParam) {
        case 0x09:  // tab
        case 0x1b:  // esc
          break;    // ポーン鳴るのでデフォルトをキャンセルする
        default:
          if (!m_EditBoxAutoComplete) {  // なるべく必要になるまでは初期化しない
            m_EditBoxAutoComplete = true;
            SHAutoComplete(m_EditBox, SHACF_FILESYSTEM | SHACF_USETAB);
          }
          bHandled = false;
      }
      return 0;
    }
    LRESULT OnComboMouseWheel(UINT, WPARAM, LPARAM, BOOL&) {
      m_Address.ShowDropDown();
      m_ComboBox.SetFocus();
      return 0;
    }
    LRESULT OnComboDropDown(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL&) {
      if (m_Address.GetCount() == 1) {
        LPITEMIDLIST pidl = ILClone(GetCurrentEntry()->ID);
        while (ILRemoveLastID(pidl)) {
          SHFILEINFO info;
          VERIFY(afx::ILGetFileInfo(pidl, &info, SHGFI_DISPLAYNAME));
          PCTSTR name = info.szDisplayName;
          // Registroy Explorer は、名前を要求してもパスを返してくるので、ここでフォローしてやる
          if (afx::PathIsRegistory(name)) name = PathFindFileName(name);
          AddPath(pidl, name);
        }
        ILFree(pidl);
      }
      return 0;
    }
    LRESULT OnComboSelEndOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL&) {
      int sel = m_Address.GetCurSel();
      if (sel != 0) {
        Shell::GoUp(sel);
      }
      return 0;
    }
    LRESULT OnComboSelEndCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL&) {
      m_Address.SetCurSel(0);
      return 0;
    }
    LRESULT OnComboEndEdit(int, LPNMHDR nm, BOOL&) {
      NMCBEENDEDIT* edit = (NMCBEENDEDIT*)nm;
      if (edit->iWhy == CBENF_RETURN && GetCurrentEntry()->Path != edit->szText) {
        try {
          ref<io::IEntry> entry(__uuidof(io::Entry), string(edit->szText));
          if (entry->IsFolder()) {
            Go(entry);
          } else {
            avesta::ILExecute(entry->ID, null, null, null, m_hWnd);
          }
        } catch (mew::exceptions::Error& e) {
          MessageBox(e.Message.str(), NULL, MB_OK | MB_ICONERROR);
        }
      }
      return 0;
    }

   public:  // overridable
    void HandleUpdateLayout() {
      Rect rcClient;
      GetClientRect(&rcClient);
      if (rcClient.empty()) {
        return;
      }
      Rect rcEdit;
      m_Address.GetWindowRect(&rcEdit);
      rcEdit.left = rcClient.left;
      rcEdit.right = rcClient.right;
      rcEdit.bottom = rcEdit.bottom - rcEdit.top;
      rcEdit.top = 0;
      rcClient.top = rcEdit.bottom;
      m_Address.MoveWindow(&rcEdit);
      if (CWindowEx w = this->ShellView) {
        w.MoveWindow(&rcClient);
      }
    }
    HRESULT GetExtension(REFGUID which, REFINTF what) {
      if (which == __uuidof(IShellStorage)) {
        return m_pStorage.copyto(what);
      } else {
        return __super::GetExtension(which, what);
      }
    }
    HRESULT SetExtension(REFGUID which, IUnknown * what) {
      if (which == __uuidof(IShellStorage)) {
        return m_pStorage = cast(what), S_OK;
      } else {
        return __super::SetExtension(which, what);
      }
    }
    HRESULT Go(io::IEntry * folder) { return Shell::GoAbsolute(folder); }
    HRESULT Go(Direction dir, int level = 1) {
      switch (dir) {
        case DirNorth:
          return Shell::GoUp(level);
        case DirEast:
          return Shell::GoForward(level);
        case DirWest:
          return Shell::GoBack(level);
        default:
          TRESPASS_DBG(return E_NOTIMPL);
      }
    }

    size_t get_Count() {
      ListCtrl list = this->ListView;
      return list.IsWindow() ? list.GetItemCount() : 0;
    }
    HRESULT GetAt(REFINTF pp, size_t index) { return Shell::GetAt(pp, index); }
    HRESULT GetContents(REFINTF ppInterface, Status status) { return Shell::GetContents(ppInterface, status); }
    HRESULT GetStatus(IndexOr<IUnknown> item, DWORD * status, size_t* index = null) {
      ASSERT(item.is_index());
      ASSERT(!index);
      ListCtrl list = this->ListView;
      if (!list) {
        return E_UNEXPECTED;
      }
      int i = item.as_index();
      if (status) {
        *status = 0;
        UINT lvs = list.GetItemState(i, LVIS_FOCUSED | LVIS_SELECTED);
        if (lvs & LVIS_FOCUSED) {
          *status |= FOCUSED;
        }
        if (lvs & LVIS_SELECTED) {
          *status |= SELECTED;
        }
        if (get_CheckBox() && list.GetCheckState(i)) {
          *status |= CHECKED;
        }
      }
      return S_OK;
    }
    HRESULT SetStatus(IndexOr<IUnknown> item, Status status, bool unique = false) {
      if (item.is_index()) {
        return SetStatusByIndex(item, status, unique);
      } else {
        return SetStatusByUnknown(item, status, unique);
      }
    }

    ListStyle get_Style() { return Shell::get_Style(); }
    void set_Style(ListStyle style) { Shell::set_Style(style); }
    bool get_CheckBox() { return Shell::get_CheckBox(); }
    void set_CheckBox(bool value) { Shell::set_CheckBox(value); }
    bool get_FullRowSelect() { return Shell::get_FullRowSelect(); }
    void set_FullRowSelect(bool value) { Shell::set_FullRowSelect(value); }
    bool get_GridLine() { return Shell::get_GridLine(); }
    void set_GridLine(bool value) { Shell::set_GridLine(value); }
    bool get_Grouping() { return Shell::get_Grouping(); }
    void set_Grouping(bool value) { Shell::set_Grouping(value); }
    bool get_AutoArrange() { return Shell::get_AutoArrange(); }
    void set_AutoArrange(bool value) { Shell::set_AutoArrange(value); }
    bool get_ShowAllFiles() { return Shell::get_ShowAllFiles(); }
    void set_ShowAllFiles(bool value) { Shell::set_ShowAllFiles(value); }
    size_t get_SelectedCount() { return ListView ? ListView.GetSelectedCount() : 0; }
    bool get_RenameExtension() { return Shell::get_RenameExtension(); }
    void set_RenameExtension(bool value) { Shell::set_RenameExtension(value); }
    string get_PatternMask() { return m_PatternMask; }
    void set_PatternMask(string value) {
      m_PatternMask = value;
      Shell::Refresh();
      if (m_pCurrentEntry) {
        string address = FormatAddress(m_pCurrentEntry->GetName(io::IEntry::PATH_OR_NAME));
        COMBOBOXEXITEM item = {CBEIF_TEXT, 0, (PWSTR)address.str()};
        m_Address.SetItem(&item);
        m_Address.SetCurSel(0);
      }
    }
    string FormatAddress(string PathOrName) const {
      if (!m_PatternMask) {
        return PathOrName;
      }
      return string::format(L"$1 | $2", PathOrName, m_PatternMask);
    }
    HRESULT GetFolder(REFINTF ppFolder) {
      if (ppFolder.iid == __uuidof(IShellFolder)) {
        return objcpy(GetCurrentFolder(), ppFolder);
      } else {
        return objcpy(GetCurrentEntry(), ppFolder);
      }
    }
    string GetLastStatusText() { return m_StatusText; }
    HRESULT SelectChecked() { return Shell::SelectChecked(); }

    LRESULT OnGetFont(UINT, WPARAM wParam, LPARAM lParam, BOOL & bHandled) { return (LRESULT)m_ListFont.m_hFont; }
    LRESULT OnSetFont(UINT, WPARAM wParam, LPARAM lParam, BOOL & bHandled) {
      switch (lParam) {
        case 0:
          m_Address.SetFont((HFONT)wParam);
          break;
        case 1:
          m_ListFont = (HFONT)wParam;
          if (ListView) {
            ListView.SetFont(m_ListFont);
          }
          break;
      }
      Update();
      return 0;
    }
    LRESULT OnStatusBarText(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
      if (!this->ShellView.IsWindow()) {  // 破棄後に送られてくることがあるっぽい
        return 0;
      }
      string text;
      m_StatusText = (PCWSTR)lParam;
      // ここですぐに IShellView::GetItemObject() するとフリーズする可能性があり、
      // 短時間に連続して送られることも多いため、タイミングをずらす.
      SetTimer(TIMER_STATUSTEXT, DELAY_STATUSTEXT, NULL);
      return true;
    }
    LRESULT OnTimer(UINT, WPARAM wParam, LPARAM lParam, BOOL & bHandled) {
      if (wParam == TIMER_STATUSTEXT) {
        KillTimer(TIMER_STATUSTEXT);
        InvokeEvent<EventStatusText>(this, m_StatusText);
      } else {
        bHandled = false;
      }
      return 0;
    }

   protected:  // Shell override
    bool OnListWheel(WPARAM wParam, LPARAM lParam) {
      if (wParam & MK_SHIFT) {  // Shiftが押されたままだと、別ウィンドウで開いてしまうので……
        afx::SetModifierState(0, 0);
        if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) {
          GoUp();
        } else {
          GoBack();
        }
        afx::RestoreModifierState(0);
        return true;
      } else if (wParam & MK_CONTROL) {  // もしShift or Control が押されている場合は、通常のスクロールを行わない
        ::SendMessage(GetParent(), WM_MOUSEWHEEL, wParam, lParam);
        return true;
      }
      return false;
    }
    bool OnDirectoryChanging(io::IEntry * entry, GoType go) {
      if (go == GoReplace ||
          !GetCurrentEntry()) {  // ファイルシステム変更による自動移動 または 初めての移動 の場合はイベントを発生させない
        return true;
      }
      message reply = InvokeEvent<EventFolderChanging>(this, entry);
      bool cancel = reply["cancel"] | false;
      if (cancel) TRACE(_T("directory change is canceled"));
      return !cancel;
    }
    void OnDirectoryChanged(io::IEntry * entry, GoType go) {
      ref<io::IEntry> parent;
      if SUCCEEDED (entry->GetParent(&parent)) {
        SHChangeNotifyEntry shEntry = {parent->ID};
        SHBeginMonitor(1, &shEntry, m_hWnd, MEW_NM_SHELL, SHCNE_RENAMEFOLDER | SHCNE_RMDIR | SHCNE_DRIVEREMOVED);
      }
      InvokeEvent<EventFolderChange>(this, entry);
      for (int i = m_Address.GetCount(); i > 0; --i) {
        m_Address.DeleteItem(0);
      }
      string path = (afx::ILIsRoot(entry->ID) ? null : entry->Path);
      string name = entry->Name;
      AddPath(entry->ID, FormatAddress(!path ? name : path));
      m_Address.SetCurSel(0);
      string title;
      if (!path || lstrcmpi(io::PathFindLeaf(path), name.str()) != 0) {
        title = name;
      } else {
        title = path;
      }
      this->Name = title;
      HandleUpdateLayout();
    }
    void OnDefaultExecute(io::IEntry * entry) {
      message reply = InvokeEvent<EventExecuteEntry>(this, entry);
      bool cancel = reply["cancel"] | false;
      if (!cancel) {
        avesta::ILExecute(entry->ID, null, null, null, m_hWnd);
      }
    }
    HRESULT OnStateChange(IShellView * pShellView, ULONG uChange) {
      if (!this->ShellView.IsWindow()) {  // 破棄後に送られてくることがあるっぽい
        return 0;
      }
      // ステータステキスト変更を間借りする。
      SetTimer(TIMER_STATUSTEXT, DELAY_STATUSTEXT, NULL);
      return S_OK;
    }
    HRESULT OnQueryStream(DWORD grfMode, IStream * *ppStream) {
      if (m_pStorage) {
        if (io::IEntry* folder = GetCurrentEntry()) {
          return m_pStorage->QueryStream(ppStream, folder, (grfMode & 0x03) != STGM_READ);
        }
      }
      return E_FAIL;
    }

   public:
    LRESULT OnDeviceChange(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL & bHandled) {
      switch (wParam) {
        case DBT_DEVNODES_CHANGED:      // デバイス構成が変化した ← こっちが送られてくることが多いみたい
        case DBT_DEVICEREMOVECOMPLETE:  // デバイスが取り出された
          if (io::IEntry* entry =
                  GetCurrentEntry()) {  // 何が起こったのかを調べてもいいけど、Exists() を見るのが一番簡単なので、とりあえず。
            if (!entry->Exists()) {
              TRACE(_T("info: $1 : エントリが削除されたため、自動的に閉じます"), Name);
              PostMessage(WM_CLOSE);
            }
          }
          break;
        default:
          break;
      }
      bHandled = false;
      return 0;
    }

    LRESULT OnSHChangeNotify(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
      io::IEntry* entry = GetCurrentEntry();
      if (!entry) PostMessage(WM_CLOSE);

      MSG msg;
      if (PeekMessage(&msg, m_hWnd, WM_ASYNC_GOABSOLUTE, WM_ASYNC_GOABSOLUTE, PM_NOREMOVE)) {
        return 0;
      }

      LONG lEvent = 0;
      ITEMIDLIST** pidls = NULL;
      HANDLE hLock = ::SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &pidls, &lEvent);

      if (lEvent & (SHCNE_RMDIR | SHCNE_DRIVEREMOVED)) {
        if (ILIsEqual(entry->ID, pidls[0])) {
          TRACE(_T("info: $1 : エントリが削除されたため、自動的に閉じます"), Name);
          PostMessage(WM_CLOSE);
        }
      } else if (lEvent & SHCNE_RENAMEFOLDER) {
        if (ILIsEqual(entry->ID, pidls[0])) {
          try {
            ref<io::IEntry> moveto;
            // いったんパスに変換しないと、不正なITEMIDLISTになってしまう？
            TCHAR dstpath[MAX_PATH] = _T("");
            if SUCCEEDED (afx::ILGetPath(pidls[1], dstpath)) {
              moveto.create(__uuidof(io::Entry), string(dstpath));
            } else {  // パスが取得できない＝仮想フォルダ？
              // 失敗するかもしれないが、QueryObject()を使って処理してみる。
              ref<io::IEntry> root(__uuidof(io::Entry));
              root->QueryObject(&moveto, pidls[1]);
            }
            if (moveto) {
#ifdef _DEBUG
              TCHAR path1[MAX_PATH];
              afx::ILGetPath(pidls[0], path1);
              TRACE(_T("info: $1 : エントリがリネームされたため、自動的に移動します ($2 ⇒ $3)"), Name, path1, moveto->Path);
#endif
              AsyncGoAbsolute(moveto, GoReplace);
            }
          } catch (mew::exceptions::Error&) {
          }
        }
      }

      ::SHChangeNotification_Unlock(hLock);
      return 0;
    }

   public:  // IWallPaper
    string get_WallPaperFile() { return Shell::get_WallPaperFile(); }
    UINT32 get_WallPaperAlign() { return Shell::get_WallPaperAlign(); }
    void set_WallPaperFile(string value) { Shell::set_WallPaperFile(value); }
    void set_WallPaperAlign(UINT32 value) { Shell::set_WallPaperAlign(value); }

   private:  // IDropTarget
    ref<IDropTarget> QueryFolderDropTarget() const {
      if (io::IEntry* entry = Shell::GetCurrentEntry()) {
        ref<IDropTarget> pDropTarget;
        if SUCCEEDED (entry->QueryObject(&pDropTarget)) {
          return pDropTarget;
        }
      }
      TRACE(_T("warning: @ShellListView : DropTargetを取得できません"));
      return null;
    }

   public:  // IDropTarget
    STDMETHODIMP DragEnter(IDataObject * src, DWORD key, POINTL pt, DWORD * effect) {
      if (ref<IDropTarget> dst = QueryFolderDropTarget()) {
        DWORD dwEffectCopy = *effect;                                 // DragOver()を呼ぶとゼロにされてしまうので、
        if SUCCEEDED (dst->DragEnter(src, key, pt, &dwEffectCopy)) {  // こっちでエフェクトを設定する
          return ProcessDragEnter(src, Shell::GetCurrentEntry(), key, effect);
        }
      }
      return E_FAIL;
    }
    STDMETHODIMP DragOver(DWORD key, POINTL pt, DWORD * effect) {
      if (ref<IDropTarget> dst = QueryFolderDropTarget()) {
        DWORD dwEffectCopy = *effect;                           // DragOver()を呼ぶとゼロにされてしまうので、
        if SUCCEEDED (dst->DragOver(key, pt, &dwEffectCopy)) {  // こっちでエフェクトを設定する
          return ProcessDragOver(Shell::GetCurrentEntry(), key, effect);
        }
      }
      return E_FAIL;
    }
    STDMETHODIMP DragLeave() {
      ProcessDragLeave();
      if (ref<IDropTarget> dst = QueryFolderDropTarget()) {
        return dst->DragLeave();
      }
      return E_FAIL;
    }
    STDMETHODIMP Drop(IDataObject * src, DWORD key, POINTL pt, DWORD * effect) {
      return ProcessDrop(src, Shell::GetCurrentEntry(), pt, key, effect);
    }
  };

  AVESTA_EXPORT(ShellListView)
}
}
