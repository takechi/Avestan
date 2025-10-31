// MenuProvider.cpp

#include "stdafx.h"
#include "../private.h"
#include "WindowImpl.h"
#include "MenuProvider.hpp"
#include "theme.hpp"
#include "drawing.hpp"

namespace {
const UINT ID_DEFAULT_MENU_ITEM = 0x0FFF;
static WORD ID_MENU_COMMAND = 0x1000;
inline static WORD GetNextMenuID() {
  if (++ID_MENU_COMMAND > 0x7FFF) {  // 一周することはまずないけど……。
    ID_MENU_COMMAND = 0x1000;
  }
  return ID_MENU_COMMAND;
}

struct MenuInfo : MENUINFO {
  MenuInfo(DWORD mask) {
    memset(this, 0, sizeof(*this));
    cbSize = sizeof(MENUINFO);
    fMask = mask;
  }
  mew::ui::ITreeItem* get_Root() const { return (mew::ui::ITreeItem*)dwMenuData; }
  void set_Root(mew::ui::ITreeItem* value) { dwMenuData = (ULONG_PTR)value; }
  __declspec(property(get = get_Root, put = set_Root)) mew::ui::ITreeItem* Menu;
};

struct MenuItemInfo : MENUITEMINFO {
  MenuItemInfo(DWORD mask) {
    memset(this, 0, sizeof(*this));
    cbSize = sizeof(MENUITEMINFO);
    fMask = mask;
  }
  mew::ui::ITreeItem* get_Root() const { return (mew::ui::ITreeItem*)dwItemData; }
  void set_Root(mew::ui::ITreeItem* value) { dwItemData = (ULONG_PTR)value; }
  __declspec(property(get = get_Root, put = set_Root)) mew::ui::ITreeItem* Menu;
};

static mew::ui::ITreeItem* Menu_FromHandle(HMENU hMenu) {
  MenuInfo info(MIM_MENUDATA);
  ::GetMenuInfo(hMenu, &info);
  return info.Menu;
}
static mew::ui::ITreeItem* Menu_FromHandle(HMENU hParentMenu, UINT index_or_command, BOOL isIndex) {
  if (!hParentMenu) {  // menu closing...
    return mew::null;
  }
  MenuItemInfo info(MIIM_DATA);
  if (::GetMenuItemInfo(hParentMenu, index_or_command, isIndex, &info)) {
    return info.Menu;
  } else if (!isIndex) {  // by command, recursive search
    return mew::ui::MenuProvider::FindByCommand(hParentMenu, index_or_command);
  } else {  // by index, not found
    return mew::null;
  }
}
static void Menu_Dispose(HMENU hMenu, bool destory = true) {
  if (!::IsMenu(hMenu)) {
    return;
  }
  // ToDo: 親子関係がある場合、本当は切り離してから削除しないといけない
  INT count = ::GetMenuItemCount(hMenu);
  ASSERT(count >= 0);
  for (INT i = 0; i < count; i++) {
    MenuItemInfo info(MIIM_DATA | MIIM_SUBMENU);
    if (::GetMenuItemInfo(hMenu, i, true, &info)) {
      if (info.hSubMenu) {
        Menu_Dispose(info.hSubMenu);
      }
      if (info.Menu) {
        info.Menu->Release();
      }
    }
  }
  if (mew::ui::ITreeItem* pMenu = Menu_FromHandle(hMenu)) {
    pMenu->Release();
  }
  if (destory) {
    ::DestroyMenu(hMenu);
  }
}
static void Menu_MakeEmpty(HMENU hMenu) {
  while (::GetMenuItemCount(hMenu) > 0) {
    MenuItemInfo info(MIIM_DATA | MIIM_SUBMENU);
    if (::GetMenuItemInfo(hMenu, 0, true, &info) && info.Menu) {
      if (info.hSubMenu) {
        Menu_Dispose(info.hSubMenu);
      }
      info.Menu->Release();
    }
    ::DeleteMenu(hMenu, 0, MF_BYPOSITION);
  }
}
static HMENU Menu_Create(mew::ui::ITreeItem* pMenu, bool isPopup) {
  HMENU hMenu = isPopup ? ::CreatePopupMenu() : ::CreateMenu();
  MenuInfo info(MIM_STYLE | MIM_MENUDATA | MIM_HELPID);
  ::GetMenuInfo(hMenu, &info);
  info.dwStyle |= MNS_NOTIFYBYPOS;
  info.Menu = pMenu;
  info.dwContextHelpID = (DWORD)-1;  // dwContextHelpID にタイムスタンプ情報を保存する.
  if (::SetMenuInfo(hMenu, &info)) {
    pMenu->AddRef();
  }
  return hMenu;
}
static void Menu_Insert(
    HMENU hMenu, int index,
    mew::ui::ITreeItem* pMenu) {  // オーナードローするので、dwTypeData（項目名）は、基本的には表示では使わない。
  // しかし、ニーモニックを処理してくれるので設定しておく。
  ASSERT(pMenu);
  size_t count = pMenu->GetChildCount();
  for (size_t i = 0; i < count; ++i) {
    mew::ref<mew::ui::ITreeItem> child;
    if FAILED (pMenu->GetChild(&child, i)) {
      continue;
    }
    mew::string text = child->Name;
    MenuItemInfo info(MIIM_FTYPE | MIIM_DATA);
    info.fType = MFT_OWNERDRAW;
    info.Menu = child;
    bool hasChildren = child->HasChildren();
    if (!child || !child->Name) {  // セパレータ
      info.fType = MFT_SEPARATOR;  // セパレータはオーナードローしない.
    } else if (hasChildren) {      // 非終端メニュー
      info.fMask |= MIIM_SUBMENU | MIIM_ID | MIIM_STRING;
      info.hSubMenu = Menu_Create(child, false);
      info.dwTypeData = const_cast<PTSTR>(text.str());
      info.wID = GetNextMenuID();
    } else {  // 終端アイテム
      // TODO: 終端アイテムが後から子供を持つようになった場合、メニューに変えなければならない.
      info.fMask |= MIIM_ID | MIIM_STRING;
      info.dwTypeData = const_cast<PTSTR>(text.str());
      info.wID = GetNextMenuID();
    }
    if (::InsertMenuItem(hMenu, (index >= 0 ? index + i : -1), true, &info) && info.Menu) {
      info.Menu->AddRef();
    }
  }
  // コマンド付き非終端メニュー
  if (pMenu->Command) {  // オーナードローにし、WM_MEASUREITEM でゼロを返すことで、
    // ダブルクリックに反応する非終端メニューを実現する.
    MenuItemInfo info(MIIM_DATA | MIIM_ID | MIIM_FTYPE | MIIM_STATE);
    info.wID = ID_DEFAULT_MENU_ITEM;
    info.Menu = pMenu;
    info.fType = MFT_OWNERDRAW;
    info.fState = MFS_DEFAULT;
    if (::InsertMenuItem(hMenu, (UINT)-1, true, &info)) {
      info.Menu->AddRef();
    }
  }
}
static void Menu_Reset(HMENU hMenu, mew::ui::ITreeItem* pMenu) {
  Menu_MakeEmpty(hMenu);
  Menu_Insert(hMenu, -1, pMenu);
}
static HMENU Menu_CreatePopup(mew::ui::ITreeItem* pMenu) {
  HMENU hMenu = Menu_Create(pMenu, true);
  Menu_Reset(hMenu, pMenu);
  return hMenu;
}
static void Menu_InvokeCommand(HMENU hMenu, UINT wID) {
  if (mew::ui::ITreeItem* pMenu = Menu_FromHandle(hMenu, wID, false)) {
    if (mew::ICommand* command = pMenu->Command) {
      command->Invoke();
    }
  }
}
static void Menu_OnMenuCommand(UINT index_and_command, HMENU hParentMenu) {
  // 98ではHIWORDらしい。LOWORDならまだ救いようがあるのに。
  UINT index = module::isNT ? index_and_command : HIWORD(index_and_command);
  if (mew::ui::ITreeItem* pMenu = Menu_FromHandle(hParentMenu, index, true)) {
    if (mew::ICommand* command = pMenu->Command) {
      command->Invoke();
    }
  }
}
static void Menu_OnInitMenuPopup(HMENU hMenu, UINT index, BOOL isSysMenu) {
  if (mew::ui::ITreeItem* pMenu = Menu_FromHandle(hMenu)) {
    UINT32 last = ::GetMenuContextHelpId(hMenu);
    UINT32 now = pMenu->OnUpdate();
    if ((INT32)(now - last) > 0) {
      Menu_Reset(hMenu, pMenu);
      ::SetMenuContextHelpId(hMenu, now);
    }
  }
  INT count = ::GetMenuItemCount(hMenu);
  ASSERT(count >= 0);
  for (INT i = 0; i < count; i++) {
    MenuItemInfo info(MIIM_DATA | MIIM_SUBMENU);
    if (::GetMenuItemInfo(hMenu, i, true, &info) && info.Menu) {
      MenuItemInfo state(MIIM_STATE);
      if (mew::ICommand* command = info.Menu->Command) {
        UINT32 uState = command->QueryState(info.Menu);
        state.fState |= ((uState & mew::ENABLED) ? MFS_ENABLED : MFS_DISABLED);
        state.fState |= ((uState & mew::CHECKED) ? MFS_CHECKED : MFS_UNCHECKED);
      } else if (!info.Menu->HasChildren()) {
        state.fState = MFS_DISABLED;
      }
      ::SetMenuItemInfo(hMenu, i, true, &state);
    }
  }
}
static void Menu_OnMenuSelect(UINT index_or_command, UINT flags, HMENU hParentMenu) {
  BOOL isPopup = (flags & MF_POPUP);
  if (index_or_command == 0 && !isPopup) {  // separator
    return;
  }
}
static bool Menu_OnMeasureItem(UINT wID, LPMEASUREITEMSTRUCT measure, IImageList* imagelist) {
  if (measure->CtlType != ODT_MENU) {
    return false;
  }
  if (measure->itemID == ID_DEFAULT_MENU_ITEM) {
    measure->itemWidth = 0;
    measure->itemHeight = 0;
  } else {
    mew::ui::ITreeItem* item = (mew::ui::ITreeItem*)measure->itemData;
    mew::string text;
    int nImage = -2;
    if (item) {
      text = item->Name;
      nImage = item->Image;
    }
    SIZE sz = mew::theme::MenuMeasureItem(text.str(), imagelist, nImage);
    measure->itemWidth = sz.cx;
    measure->itemHeight = sz.cy;
  }
  return true;
}
inline static DWORD ODS2Status(DWORD ods) {
  DWORD status = 0;
  if (ods & ODS_FOCUS) {
    status |= mew::FOCUSED;
  }
  if (ods & ODS_CHECKED) {
    status |= mew::CHECKED;
  }
  if (ods & ODS_HOTLIGHT) {
    status |= mew::HOT;
  }
  if (ods & ODS_SELECTED) {
    status |= mew::SELECTED;
  }
  if (!(ods & (ODS_GRAYED | ODS_DISABLED))) {
    status |= mew::ENABLED;
  }
  return status;
}
static bool Menu_OnDrawItem(UINT wID, DRAWITEMSTRUCT* draw, IImageList* imagelist) {
  if (draw->CtlType != ODT_MENU) {
    return false;
  }
  if (draw->itemID == ID_DEFAULT_MENU_ITEM) {
    return true;
  }
  mew::ui::ITreeItem* item = (mew::ui::ITreeItem*)draw->itemData;
  mew::string text;
  int nImage = -2;
  if (item) {
    text = item->Name;
    nImage = item->Image;
  }
  mew::theme::MenuDrawItem(draw->hDC, draw->rcItem, ODS2Status(draw->itemState), text.str(), imagelist, nImage);
  return true;
}

HRESULT Dispatch_WM_MENUCOMMAND(HWND hWnd, mew::ICommand** ppCommand) {
  // WM_MENUCOMMAND は非同期で送られるが、強制的に同期させる
  MSG msg;
  if (!::PeekMessage(&msg, hWnd, WM_MENUCOMMAND, WM_MENUCOMMAND, PM_NOREMOVE)) {
    return E_FAIL;
  }
  while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    if (msg.hwnd == hWnd && msg.message == WM_MENUCOMMAND) {
      // 98ではHIWORDらしい。LOWORDならまだ救いようがあるのに。
      UINT index = module::isNT ? msg.wParam : HIWORD(msg.wParam);
      if (mew::ui::ITreeItem* pMenu = Menu_FromHandle((HMENU)msg.lParam, index, true)) {
        return pMenu->Command.copyto(ppCommand);
      }
      return E_UNEXPECTED;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return E_FAIL;
}
}  // namespace

namespace mew {
namespace ui {

ref<ICommand> MenuProvider::PopupMenu(ITreeItem* pMenu, HWND hWnd, UINT tpm, int x, int y, const RECT* rcExclude) {
  ref<ICommand> command;
  if (HMENU hMenu = Menu_CreatePopup(pMenu)) {
    ref<IUnknown> addref(pMenu);
    UINT cmd = ui::PopupMenu(hMenu, tpm, x, y, hWnd, rcExclude);
    if (cmd) {
      Dispatch_WM_MENUCOMMAND(hWnd, &command);
    }
    DisposeMenu(hMenu);
  }
  return command;
}
HMENU MenuProvider::ConstructMenu(ITreeItem* pMenu, HMENU hMenu, int index) {
  if (!hMenu) {
    hMenu = Menu_Create(pMenu, false);
  }
  Menu_Insert(hMenu, index, pMenu);
  return hMenu;
}
void MenuProvider::DisposeMenu(HMENU hMenu, bool destroy) { Menu_Dispose(hMenu, destroy); }
BOOL MenuProvider::HandleMenuMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult) {
  switch (uMsg) {
    case WM_MENUCOMMAND:
      Menu_OnMenuCommand(wParam, (HMENU)lParam);
      return true;
    case WM_INITMENUPOPUP:
      Menu_OnInitMenuPopup((HMENU)wParam, LOWORD(lParam), HIWORD(lParam));
      return false;
    case WM_MENUSELECT:
      Menu_OnMenuSelect(LOWORD(wParam), HIWORD(wParam), (HMENU)lParam);
      return false;
    case WM_MEASUREITEM:
      return lResult = Menu_OnMeasureItem(wParam, (MEASUREITEMSTRUCT*)lParam, m_pImageList);
    case WM_DRAWITEM:
      return lResult = Menu_OnDrawItem(wParam, (DRAWITEMSTRUCT*)lParam, m_pImageList);
    case WM_SYSCOMMAND:
      if (wParam < 0xF000) {
        Menu_InvokeCommand(::GetSystemMenu(hWnd, false), wParam);
      }
      break;
    case WM_MENUCHAR: {
      UINT ch = LOWORD(wParam);
      UINT type = HIWORD(wParam);
      HMENU hMenu = (HMENU)lParam;
      switch (type) {
        case MF_POPUP:
          switch (ch) {
            case ' ': {
              int count = ::GetMenuItemCount(hMenu);
              for (int i = 0; i < count; ++i) {
                if (GetMenuState(hMenu, i, MF_BYPOSITION) & MFS_HILITE) {
                  TRACE(_T("Execute By Space"));
                  RECT rc;
                  ::GetMenuItemRect(NULL, hMenu, i, &rc);
                  afx::MimicDoubleClick((rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2);
                  // MNC_IGNORE MNC_CLOSE MNC_EXECUTE MNC_SELECT
                  lResult = MAKELRESULT(i, MNC_SELECT);
                  return true;
                }
              }
              break;
            }
            default:
              break;
          }
          break;
        case MF_SYSMENU:
        default:
          break;
      }
      break;
    }
  }
  return false;
}
ref<ITreeItem> MenuProvider::FindByCommand(HMENU hMenu, UINT wID) {
  if (!hMenu) {
    return null;
  }
  INT count = ::GetMenuItemCount(hMenu);
  for (INT i = 0; i < count; i++) {
    MenuItemInfo info(MIIM_DATA | MIIM_SUBMENU | MIIM_ID);
    if (::GetMenuItemInfo(hMenu, i, true, &info)) {
      if (info.Menu && info.wID == wID) {
        return info.Menu;
      }
      if (ITreeItem* ret = FindByCommand(info.hSubMenu, wID)) {
        return ret;
      }
    }
  }
  return null;
}

}  // namespace ui
}  // namespace mew