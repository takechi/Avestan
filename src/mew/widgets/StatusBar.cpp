// StatusBar.cpp

#include "stdafx.h"
#include "../private.h"
#include "WindowImpl.h"
#include "impl/WTLControls.hpp"

namespace {
using StatusBarBase = CStatusBar<CStatusBarCtrlT<WindowImplBase> >;
}

//==============================================================================

namespace mew {
namespace ui {

class StatusBar
    : public WindowImpl<CWindowImplEx<StatusBar, StatusBarBase>, implements<ITree, IWindow, ISignal, IDisposable> > {
 public:
  bool SupportsEvent(EventCode code) const throw() {
    if (__super::SupportsEvent(code)) return true;
    switch (code) {
      // case EventItemFocus:
      //   return true;
      default:
        return false;
    }
  }

  void DoCreate(CWindowEx parent) {
    DWORD style = WS_CONTROL | CCS_NOPARENTALIGN | CCS_NORESIZE;
    if (parent.GetStyle() & WS_SIZEBOX) style |= SBARS_SIZEGRIP;
    Size sz = DefaultSize;
    RECT rc = {0, 0, sz.w, sz.h};
    __super::DoCreate(parent, rc, DirSouth, style);
  }
  void set_Dock(Direction value) {
    bool vert2 = false;
    switch (value) {
      case DirNone:
      case DirCenter:
      case DirNorth:
      case DirSouth:
        break;
      case DirWest:
      case DirEast:
        vert2 = true;
        break;
      default:
        TRESPASS();
    }
    DWORD dwCurrentStyle = GetStyle();
    bool vert1 = ((dwCurrentStyle & CCS_VERT) == CCS_VERT);
    bool grip1 = ((dwCurrentStyle & SBARS_SIZEGRIP) == SBARS_SIZEGRIP);
    bool grip2 = (value == DirSouth && (GetParent().GetStyle() & WS_SIZEBOX));
    if (vert1 != vert2 || grip1 != grip2) {
      Size sz = DefaultSize;
      RECT rc = {0, 0, sz.w, sz.h};
      DWORD dwStyle = WS_CONTROL | CCS_NOPARENTALIGN | CCS_NORESIZE;
      if (vert2) dwStyle |= CCS_VERT;
      if (grip2) dwStyle |= SBARS_SIZEGRIP;
      Recreate(rc, NULL, dwStyle);
    }
    __super::set_Dock(value);
  }
  Size get_DefaultSize() {
    HFONT hFont = null;
    if (!IsWindow())
      hFont = AtlGetDefaultGuiFont();
    else
      hFont = GetFont();
    HDC hDC = ::GetDC(NULL);
    HGDIOBJ hOldFont = ::SelectObject(hDC, hFont);
    Size sz;
    ::GetTextExtentPoint32(hDC, _T(" "), 1, &sz);
    ::SelectObject(hDC, hOldFont);
    ::ReleaseDC(NULL, hDC);
    return Size(sz.h + 4, sz.h + 4);
  }

 public:  // ITree
  ITreeItem* get_Root() {
    ref<ITree> tool;
    if FAILED (QueryInterfaceInWindow(GetWindow(GW_CHILD), &tool)) return null;
    return tool->get_Root();
  }
  void set_Root(ITreeItem* value) {
    ref<ITree> tool;
    QueryInterfaceInWindow(GetWindow(GW_CHILD), &tool);
    if (value) {
      if (!tool) tool.create(__uuidof(ToolBar), OID);
      tool->set_Root(value);
    } else if (tool)
      tool->Close();
  }
  IImageList* get_ImageList() {
    ref<ITree> tool;
    if FAILED (QueryInterfaceInWindow(GetWindow(GW_CHILD), &tool)) return null;
    return tool->get_ImageList();
  }
  void set_ImageList(IImageList* value) {
    ref<ITree> tool;
    QueryInterfaceInWindow(GetWindow(GW_CHILD), &tool);
    if (value) {
      if (!tool) tool.create(__uuidof(ToolBar), OID);
      tool->set_ImageList(value);
    } else if (tool)
      tool->Close();
    ResizeToDefault(this);
  }

 public:  // msg map
  DECLARE_WND_SUPERCLASS(_T("mew.ui.StatusBar"), __super::GetWndClassName());

  BEGIN_MSG_MAP_(HandleWindowMessage)
  MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
  MESSAGE_HANDLER(WM_SETFONT, OnSetFont)
  CHAIN_MSG_MAP_TO(__super::HandleWindowMessage)
  END_MSG_MAP()

  HRESULT Send(message msg) {
    if (!m_hWnd) return E_FAIL;
    return __super::Send(msg);
  }
  bool HandleCreate(const CREATESTRUCT& cs) {
    NONCLIENTMETRICS metrics = {sizeof(NONCLIENTMETRICS)};
    ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);
    SetFont(::CreateFontIndirect(&metrics.lfStatusFont));
    return true;
  }
  LRESULT OnSetFont(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    DefWindowProc(uMsg, wParam, lParam);
    Size sz = get_DefaultSize();
    InvokeEvent<EventResizeDefault>(static_cast<IWindow*>(this), sz);
    Rect bounds = this->Bounds;
    bounds.h = sz.h;
    this->Bounds = bounds;
    UpdateParent();
    Invalidate();
    return 0;
  }
  LRESULT OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    HFONT hFont = GetFont();
    DefWindowProc();
    if (hFont != AtlGetDefaultGuiFont()) SetFont(hFont);
    return 0;
  }
  void HandleUpdateLayout() {
    ref<ITree> tool;
    if FAILED (QueryInterfaceInWindow(GetWindow(GW_CHILD), &tool)) return;
    Size sz = tool->DefaultSize;
    Rect rc = this->ClientArea;
    if (rc.h < sz.h) {
      ResizeClient(rc.w, sz.h);
      UpdateParent();
    } else {
      int center = (rc.top + rc.bottom) / 2;
      rc.top = center - sz.h / 2;
      rc.bottom = center + sz.h / 2;
      rc.right -= rc.h;
      rc.left = rc.right - sz.w;
      tool->Bounds = rc;
    }
  }
};

}  // namespace ui
}  // namespace mew

AVESTA_EXPORT(StatusBar)
