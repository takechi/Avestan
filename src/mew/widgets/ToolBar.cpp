// ToolBar.cpp

#include "stdafx.h"
#include "../private.h"
#include "WindowImpl.h"
#include "impl/WTLControls.hpp"
#include "MenuProvider.hpp"

#include "path.hpp"
#include "ShellFolder.h"
#include "theme.hpp"
#include "drawing.hpp"

#include "../server/main.hpp"  // もうぐちゃぐちゃ……

namespace {
const int ID_FIRST_ITEM = 100;
const int BTNOFF = 1;  // なぜかツールバーの一つ目のボタンは動作がおかしいので、ダミーを追加する

class __declspec(novtable) ToolBarBase : public WTLEX::CTypedToolBar<LPARAM, CToolBarCtrlT<mew::ui::WindowImplBase> >,
                                         public mew::ui::MenuProvider {
 protected:
  static const int ID_MSGMAP_PARENT = 2;

  mew::ref<mew::ui::ITreeItem> m_root;

  int m_nPopBtn;
  int m_nNextPopBtn;
  UINT m_uSysKey;

  bool m_bAutoPopup : 1;
  bool m_bMenuActive : 1;
  bool m_bPopupItem : 1;
  bool m_bEscapePressed : 1;
  bool m_bSkipMsg : 1;
  bool m_bUseKeyboardCues : 1;    ///< システム設定に従う
  bool m_bShowKeyboardCues : 1;   ///< システム設定に従う
  bool m_bAllowKeyboardCues : 1;  ///<

 public:
  ToolBarBase()
      : m_nPopBtn(-1),
        m_nNextPopBtn(-1),
        m_uSysKey(0),
        m_bAutoPopup(false),
        m_bMenuActive(false),
        m_bPopupItem(false),
        m_bSkipMsg(false),
        m_bEscapePressed(false),
        m_bUseKeyboardCues(false),
        m_bShowKeyboardCues(false),
        m_bAllowKeyboardCues(true) {}
  int GetButtonCountWithoutDummy() const { return GetButtonCount() - BTNOFF; }
  mew::ref<mew::ICommand> IdToCommand(INT wID) const {
    mew::ref<mew::ui::ITreeItem> menu;
    if SUCCEEDED (m_root->GetChild(&menu, CommandToIndex(wID) - BTNOFF)) {
      return menu->Command;
    } else {
      return mew::null;
    }
  }
  int IndexToCommand(int index) const {
    TBBUTTON tbb = {0};
    GetButton(index, &tbb);
    return tbb.idCommand;
  }
  void PostKeyDown(WPARAM vkey) { PostMessage(WM_KEYDOWN, vkey, 0); }
  void UpdateButtonState() {
    int count = 0;
    if (m_root) {
      m_root->OnUpdate();
      // UINT32 last = ::GetMenuContextHelpId(hMenu);
      // UINT32 now = pMenu->OnUpdate();
      // if((INT32)(now - last) > 0) {
      //   Menu_Reset(hMenu, pMenu);
      //   ::SetMenuContextHelpId(hMenu, now);
      // }
      count = m_root->GetChildCount();
    }
    int btns = GetButtonCountWithoutDummy();
    if (count != btns) ReCreateButtons();
    btns = GetButtonCountWithoutDummy();
    for (int i = 0; i < btns; ++i) {
      TBBUTTONINFO info = {sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_COMMAND | TBIF_STATE};
      if (GetButtonInfo(i + BTNOFF, &info)) {
        mew::ref<mew::ui::ITreeItem> menu;
        if SUCCEEDED (m_root->GetChild(&menu, CommandToIndex(info.idCommand) - BTNOFF)) {
          if (mew::ICommand* command = menu->Command) {
            UINT32 uState = command->QueryState(menu);
            mew::bitof(info.fsState, TBSTATE_ENABLED) = !!(uState & mew::ENABLED);
            mew::bitof(info.fsState, TBSTATE_CHECKED) = !!(uState & mew::CHECKED);
          } else if (menu->HasChildren()) {
            info.fsState = TBSTATE_ENABLED;
          }
          info.dwMask = TBIF_BYINDEX | TBIF_STATE;
          VERIFY(SetButtonInfo(i + BTNOFF, &info));
        }
      }
    }
  }
  void ResetButtons() {
    MakeEmpty();
    // 左端のボタンは動作がおかしいため、ダミーボタンを入れる
    TBBUTTON btn = {I_IMAGENONE};
    btn.fsStyle = BTNS_BUTTON;
    btn.fsState = TBSTATE_HIDDEN;
    btn.iString = -1;
    InsertButton(-1, &btn);
  }
  void ReCreateButtons() {
    ResetButtons();
    if (!m_root) {
      return;
    }
    mew::ui::SuppressRedraw lock(m_hWnd);

    bool doesBarUseImage = false;
    m_root->OnUpdate();
    size_t count = m_root->GetChildCount();
    for (size_t i = 0; i < count; ++i) {
      mew::ref<mew::ui::ITreeItem> menu;
      if FAILED (m_root->GetChild(&menu, i)) {
        continue;
      }
      mew::string text = menu->Name;
      bool hasChildren = menu->HasChildren();
      bool hasCommand = !!menu->Command;
      if (text || hasChildren || hasCommand) {
        int image = menu->Image;
        if (image >= 0) {
          doesBarUseImage = true;
        }
        TBBUTTON item = {0};
        item.iBitmap = (image >= 0 ? image : I_IMAGENONE);
        item.idCommand = i + ID_FIRST_ITEM;
        item.fsState = (hasChildren || hasCommand) ? TBSTATE_ENABLED : 0;
        item.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
        if (text) {
          item.fsStyle |= BTNS_SHOWTEXT;
          item.iString = (INT_PTR)text.str();
        }
        if (hasChildren) {
          if (hasCommand) {
            item.fsStyle |= BTNS_DROPDOWN;
          } else {
            item.fsStyle |= BTNS_WHOLEDROPDOWN;
          }
        } else if (hasCommand) {
          item.fsStyle |= BTNS_BUTTON;
        }
        InsertButton(-1, &item);
      } else {
        TBBUTTON item = {0};
        item.fsStyle = BTNS_SEP;
        InsertButton(-1, &item);
      }
    }
    // imagelist
    UpdateToolBarImageList(doesBarUseImage);
  }
  bool DoesBarUseImage() const {
    if (!m_root) {
      return false;
    }
    size_t count = m_root->GetChildCount();
    for (size_t i = 0; i < count; ++i) {
      mew::ref<mew::ui::ITreeItem> menu;
      if FAILED (m_root->GetChild(&menu, i)) {
        continue;
      }
      if (menu->Image >= 0) {
        return true;
      }
    }
    return false;
  }
  void UpdateToolBarImageList(bool enable) {
    mew::ref<IImageList> imagelist = (enable ? GetMenuImageList() : mew::null);
    SetImageList(imagelist);
    AutoSize();
  }

 protected:
  void TakeFocus() {
    if (::GetFocus() != m_hWnd) {
      SetFocus();
    }
  }
  void GiveFocusBack() {
    if (m_bUseKeyboardCues && m_bShowKeyboardCues) {
      ShowKeyboardCues(false);
    }
    if (HasFocus()) {
      GetTopLevelParent().SetFocus();
    }
  }
  void ShowKeyboardCues(bool bShow) {
    m_bShowKeyboardCues = bShow;
    SetDrawTextFlags(DT_HIDEPREFIX, m_bShowKeyboardCues ? 0 : DT_HIDEPREFIX);
    Invalidate();
    UpdateWindow();
  }

  // Implementation
  void InvokeButtonCommand(INT wID) {
    if (mew::ref<mew::ICommand> command = IdToCommand(wID)) {
      command->Invoke();
    }
    GiveFocusBack();
  }
  void DoPopupMenu(const int index, bool animate) {
    mew::ref<mew::ui::ITreeItem> menu;
    if (FAILED(m_root->GetChild(&menu, index - BTNOFF))) {
      return;
    }

    if (animate) {
      if (::GetFocus() != m_hWnd) {
        TakeFocus();
      }
      m_bEscapePressed = false;
    }

    // get button ID
    TBBUTTONINFO info = {sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_COMMAND | TBIF_STYLE};
    GetButtonInfo(index, &info);
    const int nCmdID = info.idCommand;

    m_nPopBtn = index;  // remember current button's index

    // press button and display popup menu

    if (menu->HasChildren()) {
      // get popup menu and it's position
      RECT rcExclude;
      GetItemRect(index, &rcExclude);
      afx::MapWindowRect(m_hWnd, NULL, &rcExclude);
      SetHotItem(index);
      if (info.fsStyle & BTNS_WHOLEDROPDOWN) {
        PressButton(nCmdID, true);
      }
      mew::ref<mew::ICommand> command =
          DoTrackPopupMenu(menu, animate ? TPM_VERPOSANIMATION : TPM_NOANIMATION, rcExclude.left, rcExclude.bottom, rcExclude);
      if (info.fsStyle & BTNS_WHOLEDROPDOWN) {
        PressButton(nCmdID, false);
      }
      if (::GetFocus() != m_hWnd) {
        SetHotItem(-1);
      }
      m_nPopBtn = -1;  // restore

      // eat next msg if click is on the same button
      MSG msg;
      if (::PeekMessage(&msg, m_hWnd, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_NOREMOVE) && ::PtInRect(&rcExclude, msg.pt)) {
        ::PeekMessage(&msg, m_hWnd, WM_LBUTTONDOWN, WM_LBUTTONDOWN, PM_REMOVE);
      }
      // check if another popup menu should be displayed
      if (m_nNextPopBtn != -1) {
        PostMessage(GetAutoPopupMessage(), m_nNextPopBtn & 0xFFFF);
        if (!(m_nNextPopBtn & 0xFFFF0000) && !m_bPopupItem && IsDropDown(m_nNextPopBtn & 0xFFFF)) {
          PostKeyDown(VK_DOWN);
        }
        m_nNextPopBtn = -1;
      } else {
        // If user didn't hit escape, give focus back
        if (!m_bEscapePressed) {
          if (m_bUseKeyboardCues && m_bShowKeyboardCues) {
            m_bAllowKeyboardCues = false;
          }
          GiveFocusBack();
        } else {
          SetHotItem(index);
        }
      }

      // invole command
      if (command) {
        command->Invoke();
      }
    } else {
      SetHotItem(index);
      m_nPopBtn = -1;  // restore
    }
  }

  mew::ref<mew::ICommand> DoTrackPopupMenu(mew::ui::ITreeItem* menu, UINT uAnimFlag, int x, int y, const RECT& rcExclude) {
    CWindowCreateCriticalSectionLock lock;
    lock.Lock();

    m_bPopupItem = false;
    m_bAutoPopup = true;
    m_bMenuActive = true;
    UINT uFlags = (uAnimFlag | TPM_LEFTBUTTON | TPM_VERTICAL | TPM_LEFTALIGN | TPM_TOPALIGN);
    mew::ref<mew::ICommand> command = PopupMenu(menu, m_hWnd, uFlags, x, y, &rcExclude);
    m_bMenuActive = false;
    m_bAutoPopup = false;

    lock.Unlock();

    ASSERT(mew::ui::GetMenuDepth() == 0);

    UpdateWindow();
    GetTopLevelParent().UpdateWindow();
    return command;
  }
  int GetPreviousMenuItem(int nBtn) const {
    if (nBtn == -1) {
      return -1;
    }
    RECT rcClient;
    GetClientRect(&rcClient);
    int nNextBtn;
    for (nNextBtn = nBtn - 1; nNextBtn != nBtn; nNextBtn--) {
      if (nNextBtn < 1) {
        nNextBtn = GetButtonCount() - 1;
      }
      TBBUTTON tbb = {0};
      GetButton(nNextBtn, &tbb);
      RECT rcBtn;
      GetItemRect(nNextBtn, &rcBtn);
      if (rcBtn.right > rcClient.right) {
        nNextBtn = -2;  // chevron
        break;
      }
      if ((tbb.fsState & TBSTATE_ENABLED) != 0 && (tbb.fsState & TBSTATE_HIDDEN) == 0) {
        break;
      }
    }
    return (nNextBtn != nBtn) ? nNextBtn : -1;
  }
  int GetNextMenuItem(int nBtn) const {
    if (nBtn == -1) {
      return -1;
    }
    RECT rcClient;
    GetClientRect(&rcClient);
    int nNextBtn;
    int nCount = GetButtonCount();
    for (nNextBtn = nBtn + 1; nNextBtn != nBtn; nNextBtn++) {
      if (nNextBtn >= nCount) {
        nNextBtn = 1;
      }
      TBBUTTON tbb = {0};
      GetButton(nNextBtn, &tbb);
      RECT rcBtn;
      GetItemRect(nNextBtn, &rcBtn);
      if (rcBtn.right > rcClient.right) {
        nNextBtn = -2;  // chevron
        break;
      }
      if ((tbb.fsState & TBSTATE_ENABLED) != 0 && (tbb.fsState & TBSTATE_HIDDEN) == 0) {
        break;
      }
    }
    return (nNextBtn != nBtn) ? nNextBtn : -1;
  }

  void GetSystemSettings() {
    BOOL bRet;

    // query keyboard cues mode (Windows 2000 or later)
    if (IsWindowsVistaOrGreater()) {
      BOOL bRetVal = true;
      bRet = ::SystemParametersInfo(SPI_GETKEYBOARDCUES, 0, &bRetVal, 0);
      m_bUseKeyboardCues = (bRet && !bRetVal);
      m_bAllowKeyboardCues = true;
      ShowKeyboardCues(!m_bUseKeyboardCues);
    }
  }
  LRESULT OnKeyDown(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    bHandled = false;
    // Simulate Alt+Space for the parent
    // if(wParam == VK_SPACE) {
    //   GetTopLevelParent().PostMessage(WM_SYSKEYDOWN, wParam, lParam | (1 << 29));
    //   bHandled = true;
    // } else
    switch (wParam) {
      case VK_LEFT:
      case VK_RIGHT:
        if (!m_bMenuActive) {
          bool bRTL = ((GetExStyle() & WS_EX_LAYOUTRTL) != 0);
          WPARAM wpNext = bRTL ? VK_LEFT : VK_RIGHT;
          int nBtn = GetHotItem();
          int nNextBtn = (wParam == wpNext) ? GetNextMenuItem(nBtn) : GetPreviousMenuItem(nBtn);
          if (nNextBtn == -2) {
            SetHotItem(-1);
          }
        }
        break;
      case VK_ESCAPE:
        SetHotItem(-1);
        GiveFocusBack();
        break;
    }
    return 0;
  }
  LRESULT OnChar(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
    if (wParam != VK_SPACE) {
      bHandled = false;
    } else {
      return 0;
    }
    // Security
    if (!GetTopLevelParent().IsWindowEnabled() || ::GetFocus() != m_hWnd) {
      return 0;
    }

    // Handle mnemonic press when we have focus
    int nCmd = 0;
    if (wParam != VK_RETURN && !MapAccelerator((TCHAR)LOWORD(wParam), nCmd)) {
      MessageBeep(0);
    } else {
      UINT nBtn = CommandToIndex(nCmd);
      RECT rcClient;
      GetClientRect(&rcClient);
      RECT rcBtn;
      GetItemRect(nBtn, &rcBtn);
      TBBUTTONINFO info = {sizeof(TBBUTTONINFO), TBIF_STYLE | TBIF_STATE | TBIF_BYINDEX};
      GetButtonInfo(nBtn, &info);
      if ((info.fsState & TBSTATE_ENABLED) != 0 && (info.fsState & TBSTATE_HIDDEN) == 0 && rcBtn.right <= rcClient.right) {
        if (wParam != VK_RETURN) {
          SetHotItem(nBtn);
          if (info.fsStyle & BTNS_WHOLEDROPDOWN) {
            PostKeyDown(VK_DOWN);
          } else if (info.fsStyle & BTNS_DROPDOWN) {
            PostKeyDown(VK_UP);
          }
        }
      } else {
        MessageBeep(0);
        bHandled = true;
      }
    }
    return 0;
  }
  LRESULT OnInternalAutoPopup(UINT, WPARAM wParam, LPARAM, BOOL&) {
    int index = (int)wParam;
    DoPopupMenu(index, false);
    return 0;
  }
  LRESULT OnInternalGetBar(UINT, WPARAM wParam, LPARAM, BOOL&) {
    // Let's make sure we're not embedded in another process
    if ((LPVOID)wParam != NULL) {
      *((DWORD*)wParam) = GetCurrentProcessId();
    }
    if (IsWindowVisible()) {
      return (LRESULT) static_cast<ToolBarBase*>(this);
    } else {
      return NULL;
    }
  }
  LRESULT OnUpdateUIState(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    UpdateButtonState();
    bHandled = false;
    return 0;
  }
  LRESULT OnMenuSelect(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    bHandled = false;
    m_bPopupItem = (lParam != NULL) && (HIWORD(wParam) & MF_POPUP) && ((HIWORD(wParam) & MF_DISABLED) == 0);
    return 0;
  }
  LRESULT OnMenuChar(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (m_bMenuActive) {
      bHandled = false;
      return 0;
    }

    int nCmd = 0;
    if (!MapAccelerator((TCHAR)LOWORD(wParam), nCmd)) {
      bHandled = false;
      PostMessage(TB_SETHOTITEM, (WPARAM)-1, 0L);
      GiveFocusBack();
    } else if (GetTopLevelParent().IsWindowEnabled()) {
      int nBtn = CommandToIndex(nCmd);
      RECT rcClient;
      GetClientRect(&rcClient);
      RECT rcBtn;
      GetItemRect(nBtn, &rcBtn);
      TBBUTTON tbb = {0};
      GetButton(nBtn, &tbb);
      if ((tbb.fsState & TBSTATE_ENABLED) != 0 && (tbb.fsState & TBSTATE_HIDDEN) == 0 && rcBtn.right <= rcClient.right) {
        if (m_bUseKeyboardCues && !m_bShowKeyboardCues) {
          m_bAllowKeyboardCues = true;
          ShowKeyboardCues(true);
        }
        TakeFocus();
        if (IsWholeDropDown(nBtn)) {  // デフォルトの動作は、メニューを開く。
          // さらに、メニューの最初の項目にカーソルを合わせるために下キーを入力する。
          PostKeyDown(VK_DOWN);
        }
        SetHotItem(nBtn);
      } else {
        MessageBeep(0);
      }
    }
    return MAKELRESULT(1, 1);
  }
  LRESULT OnHotItemChange(int, LPNMHDR pnmh, BOOL& bHandled) {
    LPNMTBHOTITEM lpNMHT = (LPNMTBHOTITEM)pnmh;

    // Check if this comes from us
    if (pnmh->hwndFrom != m_hWnd) {
      bHandled = false;
      return 0;
    }

    bool bBlockTracking = false;

    if ((!GetTopLevelParent().IsWindowEnabled() || bBlockTracking) && (lpNMHT->dwFlags & HICF_MOUSE)) {
      return 1;
    } else {
      bHandled = false;

      // Send WM_MENUSELECT to the app if it needs to display a status text
      // if(!(lpNMHT->dwFlags & HICF_MOUSE)
      //    && !(lpNMHT->dwFlags & HICF_ACCELERATOR)
      //    && !(lpNMHT->dwFlags & HICF_LMOUSE)) {
      //   if(lpNMHT->dwFlags & HICF_ENTERING)
      //     m_wndParent.SendMessage(WM_MENUSELECT, 0, (LPARAM)m_hMenu);
      //   if(lpNMHT->dwFlags & HICF_LEAVING)
      //     m_wndParent.SendMessage(WM_MENUSELECT, MAKEWPARAM(0, 0xFFFF), NULL);
      // }

      return 0;
    }
  }
  // message hook handlers
  LRESULT OnHookMouseMove(UINT, WPARAM, LPARAM, BOOL& bHandled) {
    static POINT s_point = {-1, -1};
    DWORD dwPoint = ::GetMessagePos();
    POINT point = {GET_XY_LPARAM(dwPoint)};

    bHandled = false;
    if (m_bMenuActive && m_bAutoPopup && ::WindowFromPoint(point) == m_hWnd) {
      if (CWindowEx wndMenu = mew::ui::GetMenu()) {
        ScreenToClient(&point);
        int nHit = HitTest(&point);

        if ((point.x != s_point.x || point.y != s_point.y) && nHit >= 0 && nHit < GetButtonCount() && nHit != m_nPopBtn &&
            m_nPopBtn != -1) {
          TBBUTTON tbb;
          GetButton(nHit, &tbb);
          if ((tbb.fsState & TBSTATE_ENABLED) != 0) {
            m_nNextPopBtn = nHit | 0xFFFF0000;

            // this one is needed to close a menu if mouse button was down
            wndMenu.PostMessage(WM_LBUTTONUP, 0, MAKELPARAM(point.x, point.y));
            // this one closes a popup menu
            wndMenu.PostMessage(WM_KEYDOWN, VK_ESCAPE, 0L);

            bHandled = true;
          }
        }
      }
    } else {
      ScreenToClient(&point);
    }

    s_point = point;
    return 0;
  }
  LRESULT OnHookSysKeyDown(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
    bHandled = false;

    if (wParam == VK_MENU && m_bUseKeyboardCues && !m_bShowKeyboardCues && m_bAllowKeyboardCues) {
      ShowKeyboardCues(true);
    }

    if (wParam != VK_SPACE && !m_bMenuActive && ::GetFocus() == m_hWnd) {
      m_bAllowKeyboardCues = false;
      PostMessage(TB_SETHOTITEM, (WPARAM)-1, 0L);
      GiveFocusBack();
      m_bSkipMsg = true;
    } else {
      if (wParam == VK_SPACE && m_bUseKeyboardCues && m_bShowKeyboardCues) {
        m_bAllowKeyboardCues = true;
        ShowKeyboardCues(false);
      }
      m_uSysKey = (UINT)wParam;
    }
    return 0;
  }
  LRESULT OnHookSysKeyUp(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
    m_bAllowKeyboardCues = true;
    return 0;
  }
  LRESULT OnHookKeyDown(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
    bHandled = false;
    switch (wParam) {
      case VK_ESCAPE:
        if (m_bMenuActive) {
          int nHot = GetHotItem();
          if (nHot == -1) {
            nHot = m_nPopBtn;
          }
          if (nHot == -1) {
            nHot = 0;
          }
          SetHotItem(nHot);
          bHandled = true;
          TakeFocus();
          m_bEscapePressed = true;  // To keep focus
        }
        break;
      case VK_RETURN:
      case VK_UP:
      case VK_DOWN:
        if (!m_bMenuActive && ::GetFocus() == m_hWnd) {
          int nHot = GetHotItem();
          if (nHot != -1 && wParam != VK_RETURN && IsDropDown(nHot)) {
            PostKeyDown(VK_DOWN);
          }
        }
        if (wParam == VK_RETURN && m_bMenuActive) {
          PostMessage(TB_SETHOTITEM, (WPARAM)-1, 0L);
          m_nNextPopBtn = -1;
          GiveFocusBack();
        }
        break;
      case VK_LEFT:
      case VK_RIGHT: {
        const bool bRTL = ((GetExStyle() & WS_EX_LAYOUTRTL) != 0);
        const WPARAM wpNext = bRTL ? VK_LEFT : VK_RIGHT;
        const WPARAM wpPrev = bRTL ? VK_RIGHT : VK_LEFT;
        HWND hWndMenu;
        if (m_bMenuActive && m_bAutoPopup && !(wParam == wpNext && m_bPopupItem) && (hWndMenu = mew::ui::GetMenu()) != NULL) {
          bool bAction = false;
          if (wParam == wpPrev && mew::ui::GetMenuDepth() == 1) {
            m_nNextPopBtn = GetPreviousMenuItem(m_nPopBtn);
            if (m_nNextPopBtn != -1) {
              bAction = true;
            }
          } else if (wParam == wpNext) {
            m_nNextPopBtn = GetNextMenuItem(m_nPopBtn);
            if (m_nNextPopBtn != -1) {
              bAction = true;
            }
          }
          // Close the popup menu
          if (bAction) {
            ::PostMessage(hWndMenu, WM_KEYDOWN, VK_ESCAPE, 0L);
            if (wParam == wpNext) {
              int cItem = mew::ui::GetMenuDepth() - 1;
              while (cItem >= 0) {
                hWndMenu = mew::ui::GetMenu(cItem);
                if (hWndMenu != NULL) {
                  ::PostMessage(hWndMenu, WM_KEYDOWN, VK_ESCAPE, 0L);
                }
                cItem--;
              }
            }
            if (m_nNextPopBtn == -2) {
              m_nNextPopBtn = -1;
            }
            bHandled = true;
          }
        }
        break;
      }
      default:
        break;
    }
    return 0;
  }
  LRESULT OnCommand(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    SetHotItem(-1);
    InvokeButtonCommand(wID);
    return 0;
  }
  LRESULT OnDropDown(int, LPNMHDR pnmh, BOOL& bHandled) {
    // Check if this comes from us
    if (pnmh->hwndFrom != m_hWnd) {
      bHandled = false;
      return 1;
    }
    LPNMTOOLBAR pNMToolBar = (LPNMTOOLBAR)pnmh;
    DoPopupMenu(CommandToIndex(pNMToolBar->iItem), true);
    return TBDDRET_DEFAULT;
  }

 public:
  // Implementation - internal msg helpers
  static UINT GetAutoPopupMessage() {
    static UINT uAutoPopupMessage = 0;
    if (uAutoPopupMessage == 0) {
      uAutoPopupMessage = ::RegisterWindowMessage(_T("mew.ToolBar.AutoPopup"));
    }
    ASSERT(uAutoPopupMessage != 0);
    return uAutoPopupMessage;
  }
};

//==============================================================================

template <class TFinal>
class ToolBarImpl
    : public mew::ui::WindowImpl<mew::ui::CWindowImplEx<TFinal, ToolBarBase>,
                                 mew::implements<mew::ui::ITree, mew::ui::IWindow, mew::ISignal, mew::IDisposable> > {
 public:
  void DoCreate(HWND hParent) {
    __super::DoCreate(hParent, NULL, mew::ui::DirNorth,
                      WS_CONTROL | CCS_NOLAYOUT | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT, 0);
    SendMessage(CCM_SETVERSION, 5, 0);
    SetButtonStructSize();
    GetSystemSettings();
    ResetButtons();
    mew::ui::RegisterMessageHook(static_cast<TFinal*>(this), OnHookMessage);
  }
  void HandleDestroy() {
    mew::ui::UnregisterMessageHook(static_cast<TFinal*>(this), OnHookMessage);
    m_root.clear();
  }
  bool HandleQueryDrop(IDropTarget** pp, LPARAM lParam) {
    if (__super::HandleQueryDrop(pp, lParam)) {
      return true;
    }
    mew::Point pt(GET_XY_LPARAM(lParam));
    ScreenToClient(&pt);
    int index = HitTest(&pt);
    if (index <= 0) {
      return false;
    }
    return SUCCEEDED(OID->QueryInterface(pp));
  }
  // bool SupportsEvent(EventCode code) const throw()
  //{
  // if(__super::SupportsEvent(code))
  //   return true;
  // switch(code) {
  //   default:
  //     return false;
  // }
  //}
  void set_Dock(mew::ui::Direction value) {
    DWORD style = 0;
    switch (value) {
      case mew::ui::DirNone:
      case mew::ui::DirCenter:
      case mew::ui::DirNorth:
      case mew::ui::DirSouth:
        style = 0;
        break;
      case mew::ui::DirWest:
      case mew::ui::DirEast:
        style = CCS_VERT;
        break;
      default:
        TRESPASS();
    }
    ModifyStyle(CCS_VERT, style);
    __super::set_Dock(value);
  }
  mew::Size get_DefaultSize() {
    RECT rc = {0, 0, 0, 0};
    GetItemRect(GetItemCount() - 1, &rc);
    return mew::Size(rc.right, rc.bottom);
  }
  mew::ui::ITreeItem* get_Root() { return m_root; }
  void OnChangeButtons() {
    ResizeToDefault(this);
    InvokeEvent<mew::ui::EventResizeDefault>(static_cast<mew::ui::IWindow*>(this), get_DefaultSize());
  }
  void set_Root(mew::ui::ITreeItem* value) {
    m_root = value;
    ReCreateButtons();
    OnChangeButtons();
  }
  IImageList* get_ImageList() { return GetMenuImageList(); }
  void set_ImageList(IImageList* value) {
    SetMenuImageList(value);
    if (mew::ref<mew::drawing::IImageList2> imagelist2 = mew::cast(value)) {
      if (HIMAGELIST hDisabled = imagelist2->Disabled) {
        SetDisabledImageList(hDisabled);
      }
      if (HIMAGELIST hHot = imagelist2->Hot) {
        SetHotImageList(hHot);
      }
    }
    UpdateToolBarImageList(DoesBarUseImage());
    ReCreateButtons();
    OnChangeButtons();
  }

  BEGIN_MSG_MAP_(HandleWindowMessage)
  CHAIN_MSG_MAP_TO(__super::HandleMenuMessage)
  MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
  MESSAGE_HANDLER(GetAutoPopupMessage(), OnInternalAutoPopup)
  MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
  MESSAGE_HANDLER(WM_UPDATEUISTATE, OnUpdateUIState)
  MESSAGE_HANDLER(WM_MENUSELECT, OnMenuSelect)
  MESSAGE_HANDLER(WM_MENUCHAR, OnMenuChar)
  MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
  MESSAGE_HANDLER(WM_CHAR, OnChar)
  MESSAGE_HANDLER(WM_FORWARDMSG, OnForwardMsg)
  REFLECTED_NOTIFY_CODE_HANDLER(TBN_HOTITEMCHANGE, OnHotItemChange)
  REFLECTED_NOTIFY_CODE_HANDLER(TBN_DROPDOWN, OnDropDown)
  COMMAND_RANGE_HANDLER(ID_FIRST_ITEM, ID_FIRST_ITEM + GetButtonCount() - 1, OnCommand)
  REFLECTED_COMMAND_RANGE_HANDLER(ID_FIRST_ITEM, ID_FIRST_ITEM + GetButtonCount() - 1, OnCommand)
  CHAIN_MSG_MAP_TO(__super::HandleWindowMessage)
  END_MSG_MAP()

  BEGIN_MSG_MAP_(ProcessHookMessage)
  MESSAGE_HANDLER(WM_MOUSEMOVE, OnHookMouseMove)
  MESSAGE_HANDLER(WM_SYSKEYDOWN, OnHookSysKeyDown)
  MESSAGE_HANDLER(WM_SYSKEYUP, OnHookSysKeyUp)
  MESSAGE_HANDLER(WM_KEYDOWN, OnHookKeyDown)
  END_MSG_MAP()

  LRESULT OnKillFocus(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    SetHotItem(-1);
    bHandled = false;
    return 0;
  }
  LRESULT OnForwardMsg(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    MSG* msg = (MSG*)lParam;
    switch (msg->message) {
      case WM_KEYDOWN:
      case WM_SYSKEYDOWN:
        bHandled = true;
        break;
      default:
        bHandled = false;
        break;
    }
    return 0;
  }
  static LRESULT CALLBACK OnHookMessage(void* self, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    LRESULT lRet = 0;
    static_cast<TFinal*>(self)->ProcessHookMessage(hWnd, uMsg, wParam, lParam, lRet);
    return lRet;
  }
  LRESULT OnSettingChange(UINT, WPARAM, LPARAM, BOOL& bHandled) {
    GetSystemSettings();
    bHandled = false;
    return 0;
  }
};
}  // namespace

//==============================================================================

namespace mew {
namespace ui {

class ToolBar : public ToolBarImpl<ToolBar> {
 protected:
  CToolTipCtrlT<CWindowEx> m_tip;

 public:
  void DoCreate(CWindowEx parent) {
    __super::DoCreate(parent);
    ImmAssociateContext(m_hWnd, null);
    SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
    m_tip.Create(m_hWnd, NULL, NULL, TTS_ALWAYSTIP);
  }
  void HandleDestroy() {
    if (m_tip.IsWindow()) {
      m_tip.DestroyWindow();
      m_tip = null;
    }
    __super::HandleDestroy();
  }
  void set_Root(ITreeItem* value) {
    ResetToolTip();
    __super::set_Root(value);
  }
  void HandleUpdateLayout() { ResetToolTip(); }
  bool UpdateToolTip() {
    if (!m_tip.IsWindow()) {
      return false;
    }
    if (m_tip.GetToolCount() > 0) {
      return true;
    }
    if (!m_root) {
      return false;
    }
    int tips = 0;
    size_t count = m_root->GetChildCount();
    for (size_t i = 0; i < count; ++i) {
      ref<ITreeItem> menu;
      if FAILED (m_root->GetChild(&menu, i)) {
        continue;
      }
      ref<ICommand> command = menu->Command;
      if (!command) {
        continue;
      }
      string description = command->Description;
      if (!description) {
        continue;
      }
      RECT rc;
      GetItemRect(i + BTNOFF, &rc);
      VERIFY(m_tip.AddTool(m_hWnd, description.str(), &rc, ++tips));
    }
    return true;
  }
  void ResetToolTip() {
    if (!m_tip.IsWindow()) {
      return;
    }
    int tips = m_tip.GetToolCount();
    for (int i = 0; i < tips; ++i) {
      m_tip.DelTool(m_hWnd, i + 1);
    }
  }

 public:  // msg map
  DECLARE_WND_SUPERCLASS(_T("mew.ui.ToolBar"), __super::GetWndClassName());

  BEGIN_MSG_MAP_(HandleWindowMessage)
  MESSAGE_HANDLER(WM_FORWARDMSG, OnForwardMsg)
  // REFLECTED_NOTIFY_CODE_HANDLER(TBN_GETDISPINFOA, OnGetDispInfo)
  // REFLECTED_NOTIFY_CODE_HANDLER(TBN_GETDISPINFOW, OnGetDispInfo)
  CHAIN_MSG_MAP_TO(__super::HandleWindowMessage)
  END_MSG_MAP()

  // HRESULT Send(message msg) {
  //   if(!m_hWnd) return E_FAIL;
  //   return __super::Send(msg);
  // }
  LRESULT OnForwardMsg(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    bHandled = false;
    switch (wParam) {
      case 0:  // from mew.widget framework
        if (UpdateToolTip()) {
          m_tip.RelayEvent((LPMSG)lParam);
        }
        return 0;
      default:
        break;
    }
    return 0;
  }
  // LRESULT OnGetDispInfo(int, LPNMHDR pnmh, BOOL&) {
  //   DebugBreak();
  //   return 0;
  // }
};

class LinkBar : public Object<ToolBar, implements<IDropTarget> > {
 public:  // msg map
  DECLARE_WND_SUPERCLASS(_T("mew.ui.LinkBar"), __super::GetWndClassName());

  BEGIN_MSG_MAP_(HandleWindowMessage)
  MESSAGE_HANDLER(WM_MBUTTONDOWN, OnMButtonDown)
  MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
  CHAIN_MSG_MAP_TO(__super::HandleWindowMessage)
  END_MSG_MAP()

  LRESULT OnMButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    UINT32 m = theAvesta->MiddleClick;
    if (m != 0) {
      Point ptClient(GET_XY_LPARAM(lParam));
      int index = HitTest(&ptClient);
      if (index > 0) {
        // TODO: ユーザがカスタマイズできるように
        UINT32 mods = ui::GetCurrentModifiers();
        if (mods == 0) {
          mods = m;
        }
        afx::SetModifierState(0, mods);
        InvokeButtonCommand(IndexToCommand(index));
        afx::RestoreModifierState(0);
        return true;
      }
    }
    return 0;
  }

  LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    Point ptScreen(GET_XY_LPARAM(lParam));
    Point ptClient;
    int index = -1;
    if (ptScreen.x < 0 || ptScreen.y < 0) {
      int hot = GetHotItem();
      if (hot > 0) {
        index = hot;
        Rect rc;
        GetItemRect(index, &rc);
        ptClient.x = rc.left;
        ptClient.y = rc.bottom;
        ptScreen = ptClient;
        ClientToScreen(&ptScreen);
      }
    } else {
      ptClient = ptScreen;
      ScreenToClient(&ptClient);
      index = HitTest(&ptClient);
    }
    if (index > 0) {
      ref<io::IFolder> folder;
      if SUCCEEDED (m_root->GetChild(&folder, index - 1)) {
        ref<IContextMenu> menu;
        if SUCCEEDED (folder->Entry->QueryObject(&menu)) {
          if (CMenu popup = afx::SHBeginContextMenu(menu)) {
            UINT cmd = PopupContextMenu(menu, popup.m_hMenu, ptScreen, index);
            afx::SHEndContextMenu(menu, cmd, m_hWnd);
          }
        }
        return 0;
      }
    }
    bHandled = false;
    return 0;
  }
  UINT PopupContextMenu(IContextMenu* menu, CMenuHandle popup, POINT ptScreen, int index) {
    // ショートカットの作成(&S), 削除(&D), 名前の変更(&M) を取り除く
    for (int i = popup.GetMenuItemCount() - 1; i >= 0; --i) {
      TCHAR text[MAX_PATH];
      if (popup.GetMenuString(i, text, MAX_PATH, MF_BYPOSITION) == 0) {
        continue;
      }
      if (str::equals(text, _T("ショートカットの作成(&S)")) || str::equals(text, _T("削除(&D)")) ||
          str::equals(text, _T("名前の変更(&M)"))) {
        popup.DeleteMenu(i, MF_BYPOSITION);
      }
    }
    //
    enum {
      ID_FOLDER_GO = 100,
    };
    popup.InsertMenu(0, MF_BYPOSITION, ID_FOLDER_GO, SHSTR_MENU_GO);
    popup.InsertMenu(1, MF_BYPOSITION | MF_SEPARATOR);
    popup.SetMenuDefaultItem(0, true);
    m_bMenuActive = true;
    UINT cmd = afx::SHPopupContextMenu(menu, popup, ptScreen);
    m_bMenuActive = false;
    switch (cmd) {
      case ID_FOLDER_GO:
        afx::SetModifierState(VK_APPS, GetCurrentModifiers());
        InvokeButtonCommand(IndexToCommand(index));
        afx::RestoreModifierState(VK_APPS);
        break;
    }
    return cmd;
  }

 public:  // IDropTarget
  ref<io::IEntry> HitTestEntryFromScreenPoint(int x, int y, int* pIndex = null) {
    Point ptClient(x, y);
    ScreenToClient(&ptClient);
    int index = HitTest(&ptClient);
    if (pIndex) *pIndex = index;
    ref<io::IFolder> folder;
    if (index > 0 && SUCCEEDED(m_root->GetChild(&folder, index - 1))) {
      if (ref<io::IEntry> e = folder->Entry) {
        ref<io::IEntry> linked;
        if SUCCEEDED (e->GetLinked(&linked)) return linked;
      }
    }
    return null;
  }
  STDMETHODIMP DragEnter(IDataObject* src, DWORD key, POINTL pt, DWORD* pdwEffect) {
    return ProcessDragEnter(src, HitTestEntryFromScreenPoint(pt.x, pt.y), key, pdwEffect);
  }
  STDMETHODIMP DragOver(DWORD key, POINTL pt, DWORD* pdwEffect) {
    afx::TipRelayEvent(m_tip, m_hWnd, pt.x, pt.y);
    int index = -1;
    ref<io::IEntry> dst = HitTestEntryFromScreenPoint(pt.x, pt.y, &index);
    SetHotItem(index);
    return ProcessDragOver(dst, key, pdwEffect);
  }
  STDMETHODIMP DragLeave() {
    SetHotItem(-1);
    return ProcessDragLeave();
  }
  STDMETHODIMP Drop(IDataObject* src, DWORD key, POINTL pt, DWORD* pdwEffect) {
    HRESULT hr = ProcessDrop(src, HitTestEntryFromScreenPoint(pt.x, pt.y), pt, key, pdwEffect);
    SetHotItem(-1);
    if FAILED (hr) {
      MessageBeep(0);
    }
    return hr;
  }
};

AVESTA_EXPORT(ToolBar)
AVESTA_EXPORT(LinkBar)

}  // namespace ui
}  // namespace mew

//==============================================================================

namespace mew {
namespace ui {

class MenuBar : public ToolBarImpl<MenuBar> {
 private:
  CContainedWindow m_wndParent;

  // Constructor/destructor
 public:
  MenuBar() : m_wndParent(this, ID_MSGMAP_PARENT) {}

  // message map and handlers
  BEGIN_MSG_MAP_(HandleWindowMessage)
  REFLECTED_NOTIFY_CODE_LAMBDA(NM_CUSTOMDRAW, { return OnCustomDraw((LPNMHDR)lParam, lResult); })
  CHAIN_MSG_MAP_TO(__super::HandleWindowMessage)
  ALT_MSG_MAP(ID_MSGMAP_PARENT)  // Parent window messages
  MESSAGE_HANDLER(WM_MENUCHAR, OnParentMenuChar)
  MESSAGE_HANDLER(WM_SYSCOMMAND, OnParentSysCommand)
  END_MSG_MAP()

  BOOL OnCustomDraw(LPNMHDR pnmh, LRESULT& lResult) {
    if (pnmh->hwndFrom != m_hWnd) {
      lResult = CDRF_DODEFAULT;
      return false;
    }
    NMTBCUSTOMDRAW* draw = (NMTBCUSTOMDRAW*)pnmh;
    if (draw->nmcd.dwDrawStage == CDDS_PREPAINT) {
      lResult = CDRF_NOTIFYITEMDRAW;
      return true;
    } else if (draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
      bool bDropDown = (m_bMenuActive && (int)CommandToIndex(draw->nmcd.dwItemSpec) == m_nPopBtn);
      if (theme::MenuDrawButton(draw, GetFont(), m_bShowKeyboardCues, bDropDown)) {
        lResult = CDRF_SKIPDEFAULT;
        return true;
      }
    }
    lResult = CDRF_DODEFAULT;
    return false;
  }

  // Parent window msg handlers
  LRESULT OnParentMenuChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    return OnMenuChar(uMsg, wParam, lParam, bHandled);
  }
  LRESULT OnParentSysCommand(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
    bHandled = false;
    if ((m_uSysKey == VK_MENU || (m_uSysKey == VK_F10 && !(::GetKeyState(VK_SHIFT) & 0x80)) || m_uSysKey == VK_SPACE) &&
        wParam == SC_KEYMENU) {
      if (::GetFocus() == m_hWnd) {
        GiveFocusBack();  // exit menu "loop"
        PostMessage(TB_SETHOTITEM, (WPARAM)-1, 0L);
      } else if (m_uSysKey != VK_SPACE && !m_bSkipMsg) {
        if (m_bUseKeyboardCues && !m_bShowKeyboardCues && m_bAllowKeyboardCues) {
          ShowKeyboardCues(true);
        }

        TakeFocus();  // enter menu "loop"
        bHandled = true;
      } else if (m_uSysKey != VK_SPACE) {
        bHandled = true;
      }
    }
    m_bSkipMsg = false;
    return 0;
  }

 public:
  bool SupportsEvent(EventCode code) const throw() {
    if (__super::SupportsEvent(code)) {
      return true;
    }
    switch (code) {
      // case EventItemFocus:
      //   return true;
      default:
        return false;
    }
  }
  void DoCreate(CWindowEx parent) {
    __super::DoCreate(parent);
    SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS);
    AutoSize();
    m_wndParent.SubclassWindow(GetAncestor(GA_ROOT));
  }
  void HandleDestroy() {
    if (m_wndParent.IsWindow()) {
      m_wndParent.UnsubclassWindow();
    }
    __super::HandleDestroy();
  }

 public:  // msg map
  DECLARE_WND_SUPERCLASS(_T("mew.ui.MenuBar"), __super::GetWndClassName());

  // HRESULT Send(message msg) {
  //   if(!m_hWnd) return E_FAIL;
  //   return __super::Send(msg);
  // }
};

AVESTA_EXPORT(MenuBar)

}  // namespace ui
}  // namespace mew
