// Tab.cpp

#include "stdafx.h"
#include "../private.h"
#include "WindowImpl.h"
#include "impl/WTLControls.hpp"
#include "impl/WTLSplitter.hpp"
#include "theme.hpp"
#include "drawing.hpp"
#include <cfloat>

#include "../server/main.hpp"  // もうぐちゃぐちゃ……

// #define ENABLE_DEFAULT_KEYMAP

using namespace mew::drawing;

// どうしても複数選択タブが思い通りに動かないので、ツールバーを使って再実装する
// ボトムタブを実現できない。ツールバーとコンテナ領域を分けたほうがよかったかも。

namespace {
inline UINT MF_STRING_IF(bool exp) { return MF_STRING | (exp ? 0 : MF_DISABLED | MF_GRAYED); }

inline bool IsVisible(const TBBUTTONINFO& info) { return (info.fsState & TBSTATE_CHECKED) != 0; }
inline bool IsLocked(const TBBUTTONINFO& info) { return (info.fsState & TBSTATE_ENABLED) == 0; }
}  // namespace

namespace {
class TabCtrlLook {
 private:
  static const int HEADER_UNDERLINE = 3;
  COLORREF m_ColorBkgnd;
  COLORREF m_ColorActiveTab;
  COLORREF m_ColorActiveText;
  COLORREF m_ColorInactiveText;
  CFontHandle m_FontNormal;
  CFont m_FontFocus;

 public:
  void ResizeRects(RECT& rcHeader, RECT& rcClient) const {
    rcHeader.bottom += HEADER_UNDERLINE + 1;
    rcClient.top += HEADER_UNDERLINE + 1;
  }
  void DrawBackground(CDCHandle dc, RECT& rcHeader) const {
    rcHeader.bottom -= HEADER_UNDERLINE;
    RECT rcUnderline = {rcHeader.left, rcHeader.bottom, rcHeader.right, rcHeader.bottom + HEADER_UNDERLINE};
    mew::theme::TabDrawHeader(dc, rcHeader);
    dc.FillSolidRect(&rcHeader, m_ColorBkgnd);
    dc.FillSolidRect(&rcUnderline, m_ColorBkgnd);
  }
  void CompactPath(HDC hDC, PTSTR buffer, INT32 minWidth, INT32 maxWidth) {
    HGDIOBJ hOldFont = ::SelectObject(hDC, GetFocusFont());
    PathCompactPath(hDC, buffer, maxWidth);
    mew::Size sz;
    int len = lstrlen(buffer);
    if (len > 0) {
      GetTextExtentPoint32(hDC, buffer, len, &sz);
      if (sz.w < minWidth) {
        mew::Size szSpace(8, 8);
        GetTextExtentPoint32(hDC, L" ", 1, &szSpace);
        int n = (minWidth - sz.w + szSpace.w * 2 - 1) / (szSpace.w * 2);
        if (n > 0 && len + n * 2 < MAX_PATH) {
          for (int i = len - 1; i >= 0; --i) {
            buffer[n + i] = buffer[i];
          }
          for (int i = 0; i < n; ++i) {
            buffer[i] = buffer[len + n + i] = _T(' ');
          }
          buffer[len + n * 2] = _T('\0');
        }
      }
    }
    ::SelectObject(hDC, hOldFont);
  }
  void DrawTab(CDCHandle dc, RECT bounds, PCWSTR text, DWORD status) const {
    mew::theme::TabDrawItem(dc, bounds, text, status, m_FontNormal, GetFocusFont(), m_ColorActiveTab, m_ColorActiveText,
                            m_ColorInactiveText);
  }
  CFontHandle GetFocusFont() const { return m_FontFocus ? m_FontFocus.m_hFont : m_FontNormal.m_hFont; }
  void OnSettingChange(HFONT hFont) {
    if (hFont) {
      m_FontNormal = hFont;
    }
    if (m_FontFocus) {
      m_FontFocus.DeleteObject();
    }
    m_FontFocus.Attach(AtlCreateBoldFont(m_FontNormal));
    m_ColorBkgnd = mew::theme::DotNetTabBkgndColor();
    m_ColorActiveTab = ::GetSysColor(COLOR_BTNFACE);
    m_ColorActiveText = ::GetSysColor(COLOR_BTNTEXT);
    m_ColorInactiveText = mew::theme::DotNetInactiveTextColor();
    if (MaxColorDistance(m_ColorBkgnd, m_ColorInactiveText) < 16) {  // 背景と非アクティブ色が近すぎる
      if (MaxColorDistance(m_ColorBkgnd) < 128) {
        m_ColorInactiveText = RGB(255, 255, 255);  // 背景が黒に近い場合
      } else {
        m_ColorInactiveText = RGB(0, 0, 0);  // 背景が白に近い場合
      }
    }
  }
};
}  // namespace

namespace mew {
namespace ui {

class TabPanel : public WindowImpl<CWindowImplEx<TabPanel, WTLEX::CTypedToolBar<HWND, CToolBarCtrlT<WindowImplBase> > >,
                                   implements<ITabPanel, IList, IWindow, ISignal, IDisposable, IDropTarget> >,
                 public WTLEX::CSplitter<TabPanel> {
 private:
  int m_SerialNumber;    ///< すべてのボタンが異なるコマンド値を持つために
  CWindowEx m_wndFocus;  ///< フォーカスを持っている子供ウィンドウ.
  CToolTipCtrlT<CWindowEx> m_tip;
  TabCtrlLook m_look;
  InsertTo m_InsertPosition;
  INT32 m_DraggingTab;
  bool m_DragSwap;
  bool m_RButtonDown;
  ArrangeType m_Arrange;
  int m_TabWidthMin;
  int m_TabWidthMax;

 public:
  bool SupportsEvent(EventCode code) const throw() {
    if (__super::SupportsEvent(code)) return true;
    switch (code) {
      case EventItemFocus:
        return true;
      default:
        return false;
    }
  }
  void DoCreate(CWindowEx parent) {
    m_TabWidthMin = 0;
    m_TabWidthMax = UINT_MAX;
    m_Arrange = ArrangeHorz;
    m_InsertPosition = InsertTail;
    m_SerialNumber = 0;
    m_DraggingTab = -1;
    m_DragSwap = false;
    m_RButtonDown = false;
    __super::DoCreate(parent, NULL, DirNone,
                      WS_CONTROL | CCS_NOLAYOUT | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_CUSTOMERASE | TBSTYLE_WRAPABLE,
                      WS_EX_CONTROLPARENT);
    ImmAssociateContext(m_hWnd, null);
  }
  void HandleDestroy() {
    if (m_tip.IsWindow()) m_tip.DestroyWindow();
    __super::HandleDestroy();
  }
  Size get_DefaultSize() {
    RECT rc = {0, 0, 0, 0};
    GetItemRect(GetItemCount() - 1, &rc);
    return Size(rc.right + 4, rc.bottom + 4);
  }
  HRESULT GetContents(REFINTF pp, Status status) {
    if (pp.iid == __uuidof(IEnumUnknown)) {
      size_t count = GetItemCount();
      using EnumWindow = Enumerator<IWindow>;
      ref<EnumWindow> e;
      e.attach(new EnumWindow());
      for (size_t i = 0; i < count; ++i) {
        switch (status) {
          case StatusNone:
            break;
          case FOCUSED:
            if (m_wndFocus != GetItemData(i)) continue;
            break;
          case SELECTED:
            if (!GetChecked(i)) continue;
            break;
          case UNSELECTED:
            if (GetChecked(i)) continue;
            break;
          default:
            TRESPASS();
        }
        ref<IWindow> w;
        if SUCCEEDED (QueryInterfaceInWindow(GetItemData(i), &w)) e->push_back(w);
      }
      return e.copyto(pp);
    } else {
      ASSERT(status == FOCUSED);
      if (m_wndFocus)
        return QueryInterfaceInWindow(m_wndFocus, pp);
      else
        return E_FAIL;
    }
  }
  size_t get_Count() { return GetItemCount() - 1; }
  HRESULT GetAt(REFINTF pp, size_t index) { return QueryInterfaceInWindow(GetItemData(index + 1), pp); }
  InsertTo get_InsertPosition() { return m_InsertPosition; }
  void set_InsertPosition(InsertTo pos) { m_InsertPosition = pos; }
  ArrangeType get_Arrange() { return m_Arrange; }
  void set_Arrange(ArrangeType value) {
    if (m_Arrange == value) return;
    m_Arrange = value;
    Update();
  }
  HRESULT GetStatus(IndexOr<IUnknown> item, DWORD* status, size_t* index = null) {
    if (status) *status = 0;
    if (index) *index = 0;
    if (item.is_index())
      return DoGetStatus(item, status, index);
    else if (ref<IWindow> w = cast(item))
      return DoGetStatus(IndexFromParam(w->Handle) - 1, status, index);
    else
      return E_INVALIDARG;
  }
  HRESULT DoGetStatus(size_t index, DWORD* out_status, size_t* out_index = null) {
    ++index;  // 0はダミーボタン
    if (index >= (size_t)GetItemCount()) return E_INVALIDARG;

    if (out_status) {
      TBBUTTONINFO info = {sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_STATE};
      GetButtonInfo(index, &info);
      if (IsVisible(info)) *out_status |= SELECTED;
      if (IsLocked(info)) *out_status |= CHECKED;
    }
    if (out_index) {
      *out_index = index - 1;
    }
    return S_OK;
  }
  HRESULT SetStatus(IndexOr<IUnknown> item, Status status = SELECTED, bool unique = false) {
    if (item.is_index())
      return DoSetStatus(item, status, unique);
    else if (ref<IWindow> w = cast(item))
      return DoSetStatus(IndexFromParam(w->Handle) - 1, status, unique);
    else
      return E_INVALIDARG;
  }
  HRESULT DoSetStatus(size_t index, Status status = SELECTED, bool unique = false) {
    ++index;  // 0はダミーボタン
    if (index >= (size_t)GetItemCount()) return E_INVALIDARG;
    if (unique) {
      switch (status) {
        case SELECTED:
          SelectOnly(index);
          break;
        default:
          TRESPASS();
      }
    } else {
      if (status == ToggleSelect) status = (GetChecked(index) ? UNSELECTED : SELECTED);
      switch (status) {
        case FOCUSED:
          FocusChild(index);
          break;
        case SELECTED:
          if (GetChecked(index)) return S_OK;
          SetChecked(index, true);
          break;
        case UNSELECTED:
          if (GetCheckedCount() <= 1 || !GetEnabled(index) || !GetChecked(index)) return E_ABORT;
          SetChecked(index, false);
          break;
        case CHECKED:
          SetEnabled(index, false);
          return S_OK;
        case UNCHECKED:
          SetEnabled(index, true);
          return S_OK;
        case ToggleCheck:
          ReverseEnable(index);
          return S_OK;
        case HOT:
          HotPopoup_Add(GetItemData(index));
          return S_OK;
        default:
          TRESPASS();
      }
    }
    if (!GetChecked(IndexFromParam(m_wndFocus))) {
      SetFocusWindow(FindFocusTarget());
      if (HasFocus()) m_wndFocus.SetFocus();
    }
    Update();
    return S_OK;
  }
  void CountStatus(int* checked, int* locked) const {
    int ch = 0, lc = 0;
    int count = GetItemCount();
    for (int i = 1; i < count; ++i) {
      TBBUTTONINFO info = {sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_STATE};
      GetButtonInfo(i, &info);
      if (IsVisible(info)) {
        ++ch;
      }
      if (IsLocked(info)) {
        ++lc;
      }
    }
    if (checked) *checked = ch;
    if (locked) *locked = lc;
  }
  int GetCheckedCount() const {
    int checked;
    CountStatus(&checked, null);
    return checked;
  }
  void SetFocusWindow(HWND hWnd) {
    if (m_wndFocus != hWnd) {
      m_wndFocus = hWnd;
      ref<IWindow> w;
      QueryInterfaceInWindow(m_wndFocus, &w);
      InvokeEvent<EventItemFocus>(this, w);
    }
  }
  void FocusChild(HWND hWnd) {
    if (!GetChecked(IndexFromParam(hWnd))) return;
    if (m_wndFocus != hWnd) Invalidate();
    SetFocusWindow(hWnd);
    if (m_wndFocus) {
      if (HasFocus()) m_wndFocus.SetFocus();
    }
  }
  void FocusChild(int index) {
    if (!GetChecked(index)) return;
    CWindowEx wnd = GetItemData(index);
    if (!wnd.IsWindow()) return;
    if (m_wndFocus != wnd) Invalidate();
    SetFocusWindow(wnd);
    if (m_wndFocus) {
      if (HasFocus()) m_wndFocus.SetFocus();
    }
  }
  // フォーカス候補を探す
  CWindowEx FindFocusTarget(int start = 1, int sgn = 1) const {
    int count = GetItemCount() - 1;
    for (int i = 0; i < count; ++i) {
      int index = (start - 1 + i * sgn + count) % count + 1;
      if (GetChecked(index)) return GetItemData(index);
    }
    return 0;
  }
  void SelectOnly(int index) {
    int count = GetItemCount();
    for (int i = 1; i < count; ++i) {
      SetChecked(i, i == index);
    }
    FocusChild(index);
  }
  void GetHeaderAndClient(RECT& rcHeader, RECT& rcClient) const {
    GetItemRect(0, &rcHeader);
    const int buttonHeigt = rcHeader.bottom - rcHeader.top;
    GetClientRect(&rcClient);
    rcHeader.left = rcClient.left;
    rcHeader.right = rcClient.right;
    rcHeader.top = rcClient.top;
    rcHeader.bottom += buttonHeigt * (GetRows() - 1);
    rcClient.top = rcHeader.bottom;
    m_look.ResizeRects(rcHeader, rcClient);
  }

 public:  // override
  void ArrangeViews(int horz, const Rect& rcClient) {
    int count = GetItemCount();
    int checked = 0;
    struct Win {
      HWND hWnd;
      bool Visible;
    };
    Win* windows = (Win*)_alloca(sizeof(Win) * count);
    for (int i = 1; i < count; ++i) {
      TBBUTTONINFO info = {sizeof(TBBUTTONINFO)};
      info.dwMask = TBIF_STATE | TBIF_LPARAM | TBIF_BYINDEX;
      GetButtonInfo(i, &info);
      ASSERT(::IsWindow((HWND)info.lParam));
      windows[i].hWnd = (HWND)info.lParam;
      if (IsVisible(info)) {
        ++checked;
        windows[i].Visible = true;
      } else {
        windows[i].Visible = false;
      }
    }
    int vert = checked / horz;

    const int padding = 0;
    int width = (rcClient.w - padding * 2) - m_Border.w * (horz - 1);
    int height = (rcClient.h - padding * 2) - m_Border.h * (vert - 1);
    int h = 0, v = 0;
    for (int i = 1; i < count; ++i) {
      ref<IWindow> p;
      if FAILED (QueryInterfaceInWindow(windows[i].hWnd, &p)) continue;
      if (windows[i].Visible) {
        const int x = rcClient.left + h * width / horz + h * m_Border.w + padding;
        const int y = rcClient.top + v * height / vert + v * m_Border.h + padding;
        Rect bounds(x, y, x + width / horz, y + height / vert);
        if (h == horz - 1) {  // 一番右端のペインは、枠ぴったりに
          bounds.right = rcClient.right - padding;
        }
        if (v == vert - 1) {  // 一番下端のペインは、枠ぴったりに
          bounds.bottom = rcClient.bottom - padding;
        }
        p->Bounds = bounds;
        if (++h == horz) {
          h = 0;
          ++v;
        }
      }
      p->Visible = windows[i].Visible;
    }
  }
  void HandleUpdateLayout() {
    if (IsIconic() || !Visible) return;
    Rect rcHeader, rcClient;
    GetHeaderAndClient(rcHeader, rcClient);

    int count = GetItemCount() - 1;
    int checked = GetCheckedCount();
    if (count == 0 || checked == 0) {
      ResetToolTip();
      return;
    }

    SuppressRedraw redraw(m_hWnd);
    switch (m_Arrange) {
      case ArrangeHorz:
        ArrangeViews(checked, rcClient);
        break;
      case ArrangeVert:
        ArrangeViews(1, rcClient);
        break;
      case ArrangeAuto: {
        const int padding = 0;
        int horz = 1;
        float ratioMinusOneMin = FLT_MAX;
        for (int h = 1; h <= checked; ++h) {
          if (checked % h != 0) continue;
          int v = checked / h;
          float width = (float)((rcClient.w - padding * 2) - m_Border.w * (h - 1)) / h;
          float height = (float)((rcClient.h - padding * 2) - m_Border.h * (v - 1)) / v;
          float ratioMinusOne;
          if (width > height)
            ratioMinusOne = (float)width / height - 1;
          else
            ratioMinusOne = (float)height / width - 1;
          if (ratioMinusOne < ratioMinusOneMin) {
            ratioMinusOneMin = ratioMinusOne;
            horz = h;
          }
        }
        ArrangeViews(horz, rcClient);
        break;
      }
      default:
        TRESPASS();
    }
    ResetToolTip();
  }
  bool IsDragRegion(Point pt) {
    if (GetItemCount() <= 1) return false;
    Rect rcHeader, rcClient;
    GetHeaderAndClient(rcHeader, rcClient);
    return rcClient.contains(pt);
  }
  bool IsDragTarget(CWindowEx w) { return w.IsWindowVisible() != 0; }
  void OnResizeDragTargets(DragMode mode, CWindowEx wndDrag[2], POINT pt) {
    if (mode == DragHorz) {
      RECT rc[2];
      wndDrag[0].GetWindowRect(&rc[0]);
      ScreenToClient(&rc[0]);
      rc[0].right += pt.x - m_DragStart.x;
      wndDrag[0].MoveWindow(&rc[0]);
      wndDrag[1].GetWindowRect(&rc[1]);
      ScreenToClient(&rc[1]);
      rc[1].left += pt.x - m_DragStart.x;
      wndDrag[1].MoveWindow(&rc[1]);
    } else {
      RECT rc[2];
      wndDrag[0].GetWindowRect(&rc[0]);
      ScreenToClient(&rc[0]);
      rc[0].bottom += pt.y - m_DragStart.y;
      wndDrag[0].MoveWindow(&rc[0]);
      wndDrag[1].GetWindowRect(&rc[1]);
      ScreenToClient(&rc[1]);
      rc[1].top += pt.y - m_DragStart.y;
      wndDrag[1].MoveWindow(&rc[1]);
    }
  }

 public:  // msg map
  DECLARE_WND_SUPERCLASS(L"mew.ui.TabPanel", __super::GetWndClassName());

  BEGIN_MSG_MAP_(HandleWindowMessage)
  MSG_LAMBDA(WM_ERASEBKGND, { lParam = 1; })
  MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
  MESSAGE_HANDLER(WM_FORWARDMSG, OnForwardMsg)
  MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
  MESSAGE_HANDLER(WM_SETFONT, OnSetFont)
  MESSAGE_HANDLER(WM_MBUTTONUP, OnMButtonUp)
  MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
  MESSAGE_HANDLER(WM_UPDATEUISTATE, OnUpdateUIState)
  MESSAGE_HANDLER(OCM_COMMAND, OnCommand)  // ToolBarCtrl リフレクション
  REFLECTED_NOTIFY_CODE_HANDLER(NM_CUSTOMDRAW, OnCustomDraw)
  // 先にスプリッタにマウスを処理させた後…
  CHAIN_MSG_MAP_TO(ProcessSplitterMessage)
  // ボタン上でのマウスメッセージを処理
  MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
  MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
  MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClk)
  MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
  MESSAGE_HANDLER(WM_RBUTTONUP, OnRButtonUp)
  MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
  MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
  MESSAGE_HANDLER(WM_CAPTURECHANGED, OnCaptureChanged)
  MESSAGE_HANDLER(WM_TIMER, OnTimer)
  MESSAGE_HANDLER(WM_PARENTNOTIFY, OnParentNotify)
  MSG_LAMBDA(WM_RBUTTONDBLCLK, {})  // 面倒を避けるために握りつぶします
  // REFLECTED_NOTIFY_CODE_HANDLER(TBN_DROPDOWN, OnDropDown)
  CHAIN_MSG_MAP_TO(__super::HandleWindowMessage)
  END_MSG_MAP()

  // LRESULT OnDropDown(int, LPNMHDR pnmh, BOOL& bHandled) {
  //  // Check if this comes from us
  //  if(pnmh->hwndFrom != m_hWnd) {
  //    bHandled = false;
  //    return 1;
  //  }
  //  LPNMTOOLBAR pNMToolBar = (LPNMTOOLBAR)pnmh;
  //  RECT rc;
  //  int index = CommandToIndex(pNMToolBar->iItem);
  //  GetItemRect(index, &rc);
  //  POINT ptScreen = { rc.right, rc.bottom };
  //  ClientToScreen(&ptScreen);
  //  PopupContextMenu(index, ptScreen, TPM_RIGHTALIGN | TPM_TOPALIGN);
  //  return TBDDRET_DEFAULT;
  //}
  HRESULT Send(message msg) {
    if (!m_hWnd) return E_FAIL;
    switch (msg.code) {
      case CommandGoForward:  // next
        WalkTab(false, false);
        break;
      case CommandGoBack:  // prev
        WalkTab(true, false);
        break;
      case CommandGoUp:  // prev2
        WalkTab(true, true);
        break;
      case CommandGoDown:  // next2
        WalkTab(false, true);
        break;
      case CommandShownToLeft:
        ShownToLeft();
        break;
      case CommandShownToRight:
        ShownToRight();
        break;
      case CommandLockedToLeft:
        LockedToLeft();
        break;
      case CommandLockedToRight:
        LockedToRight();
        break;
      default:
        return __super::Send(msg);
    }
    return S_OK;
  }
  void WalkTab(bool shift, bool control) {
    int count = GetItemCount() - 1;
    if (count <= 1) return;
    int checked, locked;
    CountStatus(&checked, &locked);
    if (checked == 0) return;
    int index = IndexFromParam(m_wndFocus) - 1;
    SuppressRedraw redraw(m_hWnd);
    if (checked == 1) {
      int next;
      if (shift)
        next = (index == 0 ? count - 1 : index - 1) + 1;
      else
        next = (index == count - 1 ? 0 : index + 1) + 1;
      SetChecked(index + 1, false);
      SetChecked(next, true);
      FocusChild(next);
      Update();
    } else if (control && checked < count && locked < count) {
      if (shift) {
        for (int i = count - 1; i > 0; --i) {
          int next = (index + i) % count + 1;
          if (GetEnabled(next)) {
            if (!GetChecked(next)) {
              SetChecked(index + 1, false);
              SetChecked(next, true);
            }
            SetChecked(next, true);
            FocusChild(next);
            Update();
            break;
          }
        }
      } else {
        for (int i = 1; i < count; ++i) {
          int next = (index + i) % count + 1;
          if (GetEnabled(next)) {
            if (!GetChecked(next)) {
              SetChecked(index + 1, false);
              SetChecked(next, true);
            }
            FocusChild(next);
            Update();
            break;
          }
        }
      }
    } else {
      if (shift) {
        for (int i = count - 1; i > 0; --i) {
          int next = (index + i) % count + 1;
          if (GetChecked(next)) {
            FocusChild(next);
            break;
          }
        }
      } else {
        for (int i = 1; i < count; ++i) {
          int next = (index + i) % count + 1;
          if (GetChecked(next)) {
            FocusChild(next);
            break;
          }
        }
      }
    }
  }
  LRESULT OnForwardMsg(UINT, WPARAM, LPARAM lParam, BOOL& bHandled) {
    MSG* msg = (MSG*)lParam;
    if (UpdateToolTip()) m_tip.RelayEvent(msg);
#ifdef ENABLE_DEFAULT_KEYMAP
    if (!m_Extensions.m_keymap) {  // default-keymap
      switch (msg->message) {
        case WM_KEYDOWN:
          switch (msg->wParam) {
            case VK_TAB:
              WalkTab(IsKeyPressed(VK_SHIFT), IsKeyPressed(VK_CONTROL));
              return true;
          }
          break;
      }
    }
#endif  // ENABLE_DEFAULT_KEYMAP
    bHandled = false;
    return 0;
  }
  void OnSettingChange(HFONT hFont) {
    m_look.OnSettingChange(hFont);
    DefWindowProc(WM_SETFONT, (WPARAM)m_look.GetFocusFont().m_hFont, 0);
    if (hFont) AutoSize();
    RECT rcDummyButton;
    if (GetItemRect(0, &rcDummyButton))
      SetIndent(3 - rcDummyButton.right);
    else
      SetIndent(0);
    Invalidate();
    Update();
  }
  LRESULT OnSettingChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    DefWindowProc();
    OnSettingChange(null);
    return 0;
  }
  LRESULT OnSetFont(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    OnSettingChange((HFONT)wParam);
    return 0;
  }
  bool HandleCreate(const CREATESTRUCT& cs) {
    // SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);
    ModifyClassStyle(CS_HREDRAW | CS_VREDRAW, CS_DBLCLKS);  // ちらつくので外す.
    SetButtonStructSize();
    SetButtonWidth(32, 240);
    // ボタンの高さの算出のためにダミーのボタンを入れる
    // セパレータだと縦線が描画されるため、無効化状態の通常ボタンを使用する
    // 左端のボタンは、dwDataが設定できないような気がする。
    TBBUTTON btn = {I_IMAGENONE};
    btn.fsStyle = BTNS_AUTOSIZE;
    btn.iString = -1;
    InsertButton(-1, &btn);
    //
    OnSettingChange(AtlGetDefaultGuiFont());
    // ツールチップ
    m_tip.Create(m_hWnd, NULL, NULL, TTS_ALWAYSTIP | TTS_NOPREFIX);
    return true;
  }
  LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    int wheel = GET_WHEEL_DELTA_WPARAM(wParam);
    WalkTab(wheel > 0, true);
    return 0;
  }
  HRESULT OnChildRename(message msg) {
    if (!m_hWnd) return E_FAIL;
    ref<IWindow> from = msg["from"];
    string name = msg["name"];
    if (from && name) {
      int index = IndexFromParam(from->Handle);
      if (index >= 0) {
        string tabtext = UpdateTabsText(index, name.str());
        SetTabText(index, tabtext.str());
        ResetToolTip();
      }
    }
    return S_OK;
  }
  LRESULT OnParentNotify(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    UINT uMsg = LOWORD(wParam);
    switch (uMsg) {
      case WM_CREATE:
      case WM_DESTROY:
        if (::GetParent((HWND)lParam) == m_hWnd) HandleChildCreate(uMsg == WM_CREATE, (HWND)lParam);
        break;
    }
    bHandled = false;
    return 0;
  }
  void HandleChildCreate(bool create, CWindowEx w) {
    ref<IWindow> p;
    if FAILED (QueryInterfaceInWindow(w, &p)) return;
    if (create) {
      p->Connect(EventRename, function(this, &TabPanel::OnChildRename));
      HandleChildCreate_InsertTab(w);
    } else {
      p->Disconnect(0, function(this, &TabPanel::Send));
      int index = IndexFromParam(w);
      if (index > 0) {
        int count = GetItemCount();
        int checked = GetCheckedCount();
        if (GetChecked(index)) --checked;

        if (checked == 0) {  // 現在唯一のアクティブビューが閉じられた場合
          int next = (index >= count - 1 ? count - 2 : index + 1);
          SetChecked(next, true);
          FocusChild(next);
        } else if (m_wndFocus == w) {  // 複数選択状態で、フォーカスビューが閉じられた
          FocusChild(index + 1);
        }
        DeleteButton(index);
        if (GetItemCount() <= 1) {  // ダミー以外のすべてのボタンが削除された
          SetFocusWindow(null);
        }
      }
    }
    Update();  // 同期だとまだ新しいウィンドウが有効でない場合がある.
  }
  void HandleChildCreate_InsertTab(CWindowEx w) {
    TCHAR text[MAX_PATH];
    w.GetWindowText(text, MAX_PATH);
    string tabtext = UpdateTabsText(-1, text);
    Item item;
    item.iBitmap = I_IMAGENONE;
    item.idCommand = ++m_SerialNumber;
    item.iString = (INT_PTR)tabtext.str();
    item.Param = (HWND)w;
    item.fsState = TBSTATE_ENABLED;
    item.fsStyle = BTNS_CHECK | BTNS_AUTOSIZE | BTNS_NOPREFIX;  // | BTNS_DROPDOWN;
    //
    int currentIndex = IndexFromParam(m_wndFocus);
    if (currentIndex >= 1) {  // アクティブの右に追加
      switch (m_InsertPosition) {
        case InsertHead:
          VERIFY(InsertTab(1, item));
          break;
        case InsertTail:
          VERIFY(InsertTab(-1, item));
          break;
        case InsertPrev:
          VERIFY(InsertTab(currentIndex, item));
          break;
        case InsertNext:
          VERIFY(InsertTab(currentIndex + 1, item));
          break;
        default:
          TRESPASS();
      }
    } else {  // 最初のアイテム
      VERIFY(InsertTab(-1, item));
      SetChecked(1, true);
      FocusChild(w);
    }
  }

  std::vector<CWindowEx> m_wndHot;

  class NotifyTip : public CWindowImpl<NotifyTip> {
   private:
    static CBitmap s_bitmap;

   public:
    DECLARE_WND_CLASS_EX(L"NotifyTip", 0, -1)

    static NotifyTip* New(HWND hwndParent, const Rect& bounds) {
      if (bounds.empty()) return null;
      if (!s_bitmap) {
        io::Path path;
        ::GetModuleFileName(null, path, MAX_PATH);
        path.Append(L"..\\..\\usr\\arrow.bmp");
        s_bitmap = (HBITMAP)::LoadImage(NULL, path, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        if (!s_bitmap) return null;
      }
      return new NotifyTip(hwndParent, bounds);
    }

    NotifyTip(HWND hwndParent, const Rect& bounds) {
      BITMAP info;
      s_bitmap.GetBitmap(&info);

      RECT rc;
      Point center = bounds.center;
      rc.left = center.x - info.bmWidth / 2;
      rc.right = center.x + info.bmWidth / 2;
      rc.bottom = center.y;
      rc.top = rc.bottom - info.bmHeight;
      Create(hwndParent, rc, NULL, WS_POPUP, WS_EX_LAYERED);
    }
    void OnFinalMessage(HWND) { delete this; }

    BEGIN_MSG_MAP(_)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    END_MSG_MAP()

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
      SetLayeredWindowAttributes(m_hWnd, RGB(255, 0, 255), 0, LWA_COLORKEY);
      SetTimer(HOTPOPUP_ID, 1000);
      ShowWindow(SW_SHOW);
      bHandled = false;
      return 0;
    }
    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
      ::SetFocus((HWND)wParam);
      return 0;
    }
    LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
      switch (LOWORD(wParam)) {
        case WA_INACTIVE:
        case WA_CLICKACTIVE:
          bHandled = false;
          break;
        default:
          if (lParam) ::SetActiveWindow((HWND)lParam);
          break;
      }
      return 0;
    }
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
      switch (wParam) {
        case HOTPOPUP_ID:
          KillTimer(HOTPOPUP_ID);
          PostMessage(WM_CLOSE);
          break;
        default:
          bHandled = false;
          break;
      }
      return 0;
    }
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
      PAINTSTRUCT ps;
      BeginPaint(&ps);
      OnDraw(ps.hdc, ps.rcPaint);
      EndPaint(&ps);
      return 0;
    }
    void OnDraw(HDC hDC, const RECT& update) {
      BITMAP info;
      s_bitmap.GetBitmap(&info);
      HDC hdcSrc = ::CreateCompatibleDC(hDC);
      HGDIOBJ hbmpOld = ::SelectObject(hdcSrc, s_bitmap);
      ::BitBlt(hDC, 0, 0, info.bmWidth, info.bmHeight, hdcSrc, 0, 0, SRCCOPY);
      ::SelectObject(hdcSrc, hbmpOld);
      ::DeleteDC(hdcSrc);
    }
  };

  void HotPopoup_Add(CWindowEx w) {
    m_wndHot.push_back(w);
    SetTimer(HOTPOPUP_ID, HOTPOPUP_DELAY, NULL);
  }
  void HotPopup_OnTimer() {
    KillTimer(HOTPOPUP_ID);
    if (m_wndHot.empty()) return;

    std::vector<int> hotIndex;
    hotIndex.reserve(m_wndHot.size());
    int count = GetItemCount();
    int shown = 0;
    for (int i = 1; i < count; ++i) {
      TBBUTTONINFO info = {sizeof(TBBUTTONINFO)};
      info.dwMask = TBIF_STATE | TBIF_LPARAM | TBIF_BYINDEX;
      GetButtonInfo(i, &info);
      if (IsVisible(info)) ++shown;
      //
      CWindowEx w = (HWND)info.lParam;
      if (!w.IsWindow()) continue;                      // already destroyed.
      if (!algorithm::contains(m_wndHot, w)) continue;  // not hot.
      hotIndex.push_back(i);
    }
    //
    m_wndHot.clear();
    if (hotIndex.size() < 1 || (shown == 1 && hotIndex.size() == 1 &&
                                GetChecked(hotIndex[0]))) {  // ただ一つが表示されている場合は、通知しなくても明確。
      return;
    }
    // いったん隠しておくと、いちいちレイアウトが行われないため、高速化する。
    SetWindowPos(null, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_HIDEWINDOW);
    for (size_t i = 0; i < hotIndex.size(); ++i) {
      Rect rc = GetTabRectByIndex(hotIndex[i]);
      ClientToScreen(&rc);
      NotifyTip::New(m_hWnd, rc);
    }
    SetWindowPos(null, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
  }

  void ResetToolTip() {
    if (!m_tip.IsWindow()) return;
    int tips = m_tip.GetToolCount();
    for (int i = 0; i < tips; ++i) {
      m_tip.DelTool(m_hWnd, i + 1);
    }
  }
  bool UpdateToolTip() {
    if (!m_tip.IsWindow()) return false;
    if (m_tip.GetToolCount() > 0) return true;
    int count = GetItemCount();
    for (int i = 1; i < count; ++i) {
      HWND hWnd = GetItemData(i);
      TCHAR name[MAX_PATH];
      ::GetWindowText(hWnd, name, MAX_PATH);
      RECT rc;
      GetItemRect(i, &rc);
      VERIFY(m_tip.AddTool(m_hWnd, name, &rc, i));
    }
    return true;
  }
  LRESULT OnSetFocus(UINT, WPARAM, LPARAM, BOOL&) {
    if (!m_wndFocus.IsWindow() ||
        !GetChecked(IndexFromParam(m_wndFocus))) {  // 以前フォーカスを持っていたビューがすでに無効になっている
      SetFocusWindow(FindFocusTarget());
    }
    if (m_wndFocus) m_wndFocus.SetFocus();
    return true;
  }
  LRESULT OnUpdateUIState(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    CWindowEx focus = ::GetFocus();
    if (focus.IsWindow() && (focus.GetStyle() & WS_CHILD) && IsChild(focus) && m_wndFocus != focus &&
        (!m_wndFocus || !m_wndFocus.IsWindow() || !m_wndFocus.IsChild(focus))) {
      int count = GetItemCount();
      for (int i = 1; i < count; ++i) {
        CWindowEx w = GetItemData(i);
        if (GetChecked(i) && (w == focus || w.IsChild(focus))) {
          if (m_wndFocus != w) {
            SetFocusWindow(w);
            Invalidate();
          }
          break;
        }
      }
    }
    return 0;
  }
  LRESULT OnCaptureChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    m_DraggingTab = -1;
    m_DragSwap = false;
    m_RButtonDown = false;
    bHandled = false;
    return 0;
  }

  LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (m_DraggingTab < 0) {
      bHandled = false;
      return 0;
    }
    if (!(GET_KEYSTATE_WPARAM(wParam) & MK_LBUTTON)) {
      m_DraggingTab = -1;
      m_DragSwap = false;
      bHandled = false;
      return 0;
    }
    Point pt(GET_XY_LPARAM(lParam));
    int index = HitTest(&pt);
    if (index > 0 && m_DraggingTab != index) {
      RECT rc0, rc1;
      GetItemRect(m_DraggingTab, &rc0);
      GetItemRect(index, &rc1);
      if ((m_DraggingTab < index && (rc0.left + rc1.right) / 2 < pt.x) ||
          (m_DraggingTab > index && (rc0.right + rc1.left) / 2 > pt.x)) {  // チャタリングを防ぐための判定
        MoveTabInternal(m_DraggingTab, index);
        m_DraggingTab = index;
        m_DragSwap = true;
        SetCursor(::LoadCursor(NULL, IDC_HAND));
      }
    }
    return 0;
  }
  LRESULT OnLButtonDblClk(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    Point pt(GET_XY_LPARAM(lParam));
    int index = HitTest(&pt);
    if (index > 0) {
      TBBUTTONINFO info = {sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_STATE};
      GetButtonInfo(index, &info);
      bool needsUpdate = (IsVisible(info) == 0);
      info.fsState |= TBSTATE_CHECKED;
      info.fsState ^= TBSTATE_ENABLED;
      SetButtonInfo(index, &info);
      if (needsUpdate) Update();
    }
    return 0;
  }
  LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam,
                        BOOL& bHandled) {  // 右ボタンが押されていると不思議な動作をするため、いつも押していないとみなす
    SetFocus();
    m_DraggingTab = -1;
    m_DragSwap = false;
    m_RButtonDown = false;
    if (!(GET_KEYSTATE_WPARAM(wParam) & ~MK_LBUTTON) && GetItemCount() > 1) {  // 左ボタンのほかには何も押されていない
      Point pt(GET_XY_LPARAM(lParam));
      int index = HitTest(&pt);
      if (index > 0) m_DraggingTab = index;
    }
    DefWindowProc(uMsg, wParam & ~MK_RBUTTON, lParam);
    return 0;
  }
  LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
    if (!m_DragSwap) DefWindowProc(uMsg, wParam, lParam);
    m_DraggingTab = -1;
    m_DragSwap = false;
    if ((GET_KEYSTATE_WPARAM(wParam) && MouseButtonMask) == 0 && GetCapture() == m_hWnd) ::ReleaseCapture();
    return 0;
  }
  LRESULT OnMButtonUp(UINT, WPARAM, LPARAM lParam, BOOL&) {
    if (m_DraggingTab >= 0 && m_DragSwap) return 0;
    m_DraggingTab = -1;
    m_DragSwap = false;
    Point pt(GET_XY_LPARAM(lParam));
    int index = HitTest(&pt);
    if (index >= 0) CloseView(index);
    return 0;
  }
  LRESULT OnRButtonDown(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (m_DraggingTab >= 0 && m_DragSwap) return 0;
    m_DraggingTab = -1;
    m_DragSwap = false;
    // if(GetCapture() != m_hWnd) // キャプチャすると、ツールバーコントロールが悪さをするみたい
    //   SetCapture();
    if ((wParam & MK_LBUTTON) == 0) m_RButtonDown = true;
    return 0;
  }
  LRESULT OnRButtonUp(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (m_DraggingTab >= 0 && m_DragSwap) return 0;
    m_DraggingTab = -1;
    m_DragSwap = false;
    if (GET_KEYSTATE_WPARAM(wParam) & MK_LBUTTON) {
      Point ptClient(GET_XY_LPARAM(lParam));
      int index = HitTest(&ptClient);
      if (index > 0) {
        bool check = !GetChecked(index);
        SetChecked(index, check);
        m_RButtonDown = true;
        SendMessage(OCM_COMMAND, MAKEWPARAM(index, GetDlgCtrlID()), (LPARAM)m_hWnd);
      }
      m_RButtonDown = false;
      if (GetCapture() == m_hWnd) ::ReleaseCapture();
    } else {
      if (m_RButtonDown) {  // 右が押され、その後左が押されることなく右が離された場合のみコンテキストメニュー
        PostMessage(WM_CONTEXTMENU, (WPARAM)m_hWnd, ::GetMessagePos());
      }
      m_RButtonDown = false;
      if ((GET_KEYSTATE_WPARAM(wParam) && MouseButtonMask) == 0 && GetCapture() == m_hWnd) ::ReleaseCapture();
    }
    return 0;
  }
  void PopupContextMenu(int index, POINT ptScreen, UINT flags = 0) {
    enum IDs {
      ID_CLOSE = 1,
      ID_CLOSE_LEFT,
      ID_CLOSE_RIGHT,
      ID_CLOSE_SHOWN,
      ID_CLOSE_HIDDEN,
      ID_CLOSE_OTHERS,
      ID_CLOSE_ALL,
      ID_VIEW,
      ID_LOCK,
      ID_CHECKED_LEFT,
      ID_CHECKED_RIGHT,
      ID_LOCKED_LEFT,
      ID_LOCKED_RIGHT,
      ID_ARRANGE_HORZ,
      ID_ARRANGE_VERT,
      ID_ARRANGE_AUTO,
    };

    CMenu menu;
    menu.CreatePopupMenu();

    const bool lockClose = avesta::GetOption(avesta::BoolLockClose);
    int checked, locked;
    CountStatus(&checked, &locked);
    const int count = GetButtonCount();
    if (index > 0) {
      TBBUTTONINFO info = {sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_STATE};
      GetButtonInfo(index, &info);
      // Close
      int close = MF_STRING;
      if (!lockClose && IsLocked(info)) close |= MF_DISABLED | MF_GRAYED;
      menu.AppendMenu(close, ID_CLOSE, L"閉じる(&C)");
      // Show/Hide
      int show = MF_STRING;
      if ((checked == 1 && IsVisible(info)) || IsLocked(info)) show |= MF_DISABLED | MF_GRAYED;
      if (IsVisible(info)) show |= MF_CHECKED;
      menu.AppendMenu(show, ID_VIEW, L"表示(&V)");
      // Navigate Lock
      int lock = MF_STRING;
      if (!IsVisible(info)) lock |= MF_DISABLED | MF_GRAYED;
      if (IsLocked(info)) lock |= MF_CHECKED;
      menu.AppendMenu(lock, ID_LOCK, L"ナビゲートロック(&L)");
      menu.AppendMenu(MF_SEPARATOR);
    }
    //
    if (count > 1) {
      if (index > 0) {
        menu.AppendMenu(MF_STRING_IF(index > 1), ID_CLOSE_LEFT, L"左を閉じる(&L)");
        menu.AppendMenu(MF_STRING_IF(index < count - 1), ID_CLOSE_RIGHT, L"右を閉じる(&R)");
        menu.AppendMenu(MF_STRING_IF(count > 2), ID_CLOSE_OTHERS, L"これ以外を閉じる(&O)");
      }
      menu.AppendMenu(MF_STRING, ID_CLOSE_SHOWN, L"表示中を閉じる(&S)");
      menu.AppendMenu(MF_STRING_IF(checked < count - 1), ID_CLOSE_HIDDEN, L"非表示を閉じる(&H)");
      menu.AppendMenu(MF_STRING, ID_CLOSE_ALL, L"すべて閉じる(&A)");
      menu.AppendMenu(MF_SEPARATOR);
    }
    if (checked > 0 && count > 2 && checked < count - 1) {
      menu.AppendMenu(MF_STRING, ID_CHECKED_LEFT, L"左へ表示中のタブを集める");
      menu.AppendMenu(MF_STRING, ID_CHECKED_RIGHT, L"右へ表示中のタブを集める");
    }
    if (locked > 0 && count > 2 && locked < count - 1) {
      menu.AppendMenu(MF_STRING, ID_LOCKED_LEFT, L"左へロック中のタブを集める");
      menu.AppendMenu(MF_STRING, ID_LOCKED_RIGHT, L"右へロック中のタブを集める");
    }
    if (menu.GetMenuItemCount() > 0) {
      MENUITEMINFO info = {sizeof(MENUITEMINFO), MIIM_TYPE};
      if (menu.GetMenuItemInfo(menu.GetMenuItemCount() - 1, true, &info) && !(info.fType & MF_SEPARATOR)) {
        menu.AppendMenu(MF_SEPARATOR);
      }
    }
    menu.AppendMenu(MF_STRING | (m_Arrange == ArrangeHorz ? MF_CHECKED | MF_GRAYED : 0), ID_ARRANGE_HORZ, L"横に並べる(&H)");
    menu.AppendMenu(MF_STRING | (m_Arrange == ArrangeVert ? MF_CHECKED | MF_GRAYED : 0), ID_ARRANGE_VERT, L"縦に並べる(&V)");
    menu.AppendMenu(MF_STRING | (m_Arrange == ArrangeAuto ? MF_CHECKED | MF_GRAYED : 0), ID_ARRANGE_AUTO,
                    L"自動的に並べる(&A)");
    int cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RETURNCMD | flags, ptScreen.x, ptScreen.y, m_hWnd, NULL);
    switch (cmd) {
      case ID_CLOSE:
        CloseView(index);
        break;
      case ID_VIEW:
        ReverseCheck(index);
        if (m_wndFocus == GetItemData(index) && !GetChecked(index)) {
          FocusChild(FindFocusTarget(index + 1));
        }
        Update();
        break;
      case ID_LOCK:
        ReverseEnable(index);
        break;
      case ID_CLOSE_LEFT:
        for (int i = 1; i < index; ++i) CloseView(i);
        break;
      case ID_CLOSE_RIGHT:
        for (int i = index + 1; i < count; ++i) CloseView(i);
        break;
      case ID_CLOSE_SHOWN:
        for (int i = 1; i < count; ++i) CloseViewCheckVisible(i, true);
        break;
      case ID_CLOSE_HIDDEN:
        for (int i = 1; i < count; ++i) CloseViewCheckVisible(i, false);
        break;
      case ID_CLOSE_OTHERS:
        for (int i = 1; i < count; ++i)
          if (i != index) CloseView(i);
        break;
      case ID_CLOSE_ALL:
        for (int i = 1; i < count; ++i) CloseView(i);
        break;
      case ID_CHECKED_LEFT:
        ShownToLeft();
        break;
      case ID_CHECKED_RIGHT:
        ShownToRight();
        break;
      case ID_LOCKED_LEFT:
        LockedToLeft();
        break;
      case ID_LOCKED_RIGHT:
        LockedToRight();
        break;
      case ID_ARRANGE_HORZ:
        set_Arrange(ArrangeHorz);
        break;
      case ID_ARRANGE_VERT:
        set_Arrange(ArrangeVert);
        break;
      case ID_ARRANGE_AUTO:
        set_Arrange(ArrangeAuto);
        break;
    }
  }
  void ShownToLeft() {
    const int count = GetButtonCount();
    int left = 1;
    for (int i = left; i < count; ++i) {
      if (GetChecked(i)) MoveTabInternal(i, left++);
    }
  }
  void ShownToRight() {
    const int count = GetButtonCount();
    int right = count - 1;
    for (int i = right; i > 0; --i) {
      if (GetChecked(i)) MoveTabInternal(i, right--);
    }
  }
  void LockedToLeft() {
    const int count = GetButtonCount();
    int left = 1;
    for (int i = 1; i < count; ++i) {
      if (!GetEnabled(i)) MoveTabInternal(i, left++);
    }
  }
  void LockedToRight() {
    const int count = GetButtonCount();
    int right = count - 1;
    for (int i = right; i > 0; --i) {
      if (!GetEnabled(i)) MoveTabInternal(i, right--);
    }
  }
  LRESULT OnContextMenu(UINT, WPARAM, LPARAM lParam, BOOL&) {
    POINT ptScreen = {GET_XY_LPARAM(lParam)};
    POINT ptClient = ptScreen;
    ScreenToClient(&ptClient);
    int index = HitTest(&ptClient);
    PopupContextMenu(index, ptScreen);
    return 0;
  }
  Rect GetTabRectByIndex(size_t index) const {
    RECT rc;
    if (GetItemRect(index, &rc))
      return Rect(rc);
    else
      return Rect::Zero;
  }
  Rect GetTabRect(IndexOr<IUnknown> item) {
    int itemIndex;
    if (item.is_index())
      itemIndex = (int)item + 1;
    else if (ref<IWindow> w = cast(item))
      itemIndex = IndexFromParam(w->Handle);
    else
      return Rect::Zero;
    return GetTabRectByIndex(itemIndex);
  }
  HRESULT MoveTabInternal(int from, int to) {
    if (to < 1) to = 1;
    if (from < 1) return E_INVALIDARG;
    if (from == to) return S_FALSE;
    MoveButton(from, to);
    Update();
    return S_OK;
  }
  HRESULT MoveTab(IndexOr<IUnknown> from, int to) {
    int fromIndex;
    if (from.is_index())
      fromIndex = (int)from + 1;
    else if (ref<IWindow> w = cast(from))
      fromIndex = IndexFromParam(w->Handle);
    else
      return E_INVALIDARG;
    return MoveTabInternal(fromIndex, to + 1);
  }
  HRESULT SetMinMaxTabWidth(size_t min, size_t max) {
    m_TabWidthMin = min;
    m_TabWidthMax = math::max(min, max);
    return S_OK;
  }
  LRESULT OnCommand(UINT, WPARAM wParam, LPARAM, BOOL&) {
    int cmd = LOWORD(wParam);
    int index = CommandToIndex(cmd);
    if (index >= 0) {
      if (m_RButtonDown || IsKeyPressed(VK_CONTROL) || IsKeyPressed(VK_RBUTTON)) {
        int checked = GetCheckedCount();
        if (checked == 0) {  // 選択なしになってしまう場合はキャンセルする
          SetChecked(index, true);
          return 0;  // この場合はHandleUpdateLayout()は必要ないはず……
        }
        CWindowEx w = GetItemData(index);
        if (GetChecked(index))
          FocusChild(w);
        else if (m_wndFocus == w)
          FocusChild(FindFocusTarget());
      } else {  // 修飾キーなしでのクリックは、それのみをアクティブにする
        SelectOnly(index);
      }
    }
    Update();
    return 0;
  }
  inline static DWORD CDIS2Status(DWORD cdis) {
    DWORD status = 0;
    if (cdis & CDIS_FOCUS) status |= FOCUSED;
    if (cdis & CDIS_CHECKED) status |= CHECKED;
    if (cdis & CDIS_HOT) status |= HOT;
    if (cdis & CDIS_SELECTED) status |= SELECTED;
    if (!(cdis & CDIS_DISABLED)) status |= ENABLED;
    return status;
  }
  LRESULT OnCustomDraw(int, LPNMHDR pnmh, BOOL& bHandled) {
    NMTBCUSTOMDRAW* draw = (NMTBCUSTOMDRAW*)pnmh;
    switch (draw->nmcd.dwDrawStage) {
      case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;
      case CDDS_PREERASE: {
        RECT rcHeader, rcClient;
        GetHeaderAndClient(rcHeader, rcClient);
        CDCHandle dc = draw->nmcd.hdc;
        m_look.DrawBackground(dc, rcHeader);
        dc.FillRect(&rcClient, GetItemCount() <= 1 ? COLOR_APPWORKSPACE : COLOR_3DFACE);
        return CDRF_SKIPDEFAULT;
      }
      case CDDS_ITEMPREPAINT: {
        if (draw->nmcd.dwItemSpec == 0) return CDRF_SKIPDEFAULT;
        CDCHandle dc = draw->nmcd.hdc;
        RECT bounds = draw->nmcd.rc;
        bounds.top += 4;  // 最上部ピクセルはボーダーが引かれているためずらす
        TCHAR text[MAX_PATH];
        GetItemText_ByCommand(draw->nmcd.dwItemSpec, text, MAX_PATH);
        HWND hView = (HWND)draw->nmcd.lItemlParam;
        ASSERT(::IsWindow(hView));
        dc.SetBkMode(TRANSPARENT);
        DWORD status = CDIS2Status(draw->nmcd.uItemState);
        if (hView == m_wndFocus)
          status |= FOCUSED;
        else
          status &= ~FOCUSED;
        m_look.DrawTab(dc, bounds, text, status);
        return CDRF_SKIPDEFAULT;
      }
      default:
        bHandled = false;
        return CDRF_DODEFAULT;
    }
  }

  HWND _getViewIfClosable(int index) const {
    TBBUTTONINFO info = {sizeof(TBBUTTONINFO), TBIF_BYINDEX | TBIF_STATE | TBIF_LPARAM};
    if (!GetButtonInfo(index, &info)) {
      return null;
    }
    if (!avesta::GetOption(avesta::BoolLockClose) && IsLocked(info)) {
      return null;
    }
    return (HWND)info.lParam;
  }
  void CloseView(int index) {
    if (HWND hwnd = _getViewIfClosable(index)) ::PostMessage(hwnd, WM_CLOSE, 0, 0);
  }
  void CloseViewCheckVisible(int index, bool visible) {
    if (HWND hwnd = _getViewIfClosable(index)) {
      if (afx::GetVisible(hwnd) == visible) {
        ::PostMessage(hwnd, WM_CLOSE, 0, 0);
      }
    }
  }

 private:
  // FIXME: ある程度複雑になるのは避けられないが、もうちょい分かりやすく出来ないものか……。
  struct TempTabText {
    string text;
    int level;  // 切りそろえるレベル。level < 0 の場合は、代わりにnewTab.levelを参照すること。
  };
  /// @param index 更新が発生したインデックス
  /// @param name  そのテキスト
  string UpdateTabsText(int index, string name) {
    if (theAvesta->DistinguishTab) {
      const int count = GetItemCount();
      std::vector<TempTabText> names(count);
      for (int i = 0; i < count; ++i) {
        if (i == index) {
          names[i].text = name;
          names[i].level = 1;
        } else {
          TCHAR text[MAX_PATH] = L"";
          ::GetWindowText(GetItemData(i), text, MAX_PATH);
          names[i].text = text;
          names[i].level = 1;
        }
      }
      TempTabText newTab;
      if (index >= 0) {
        newTab = names[index];
      } else {
        newTab.text = name;
        newTab.level = 1;
      }
      for (int i = 0; i < count; ++i) {
        if (i == index) continue;
        TempTabText& otherTab = names[i];
        if (newTab.text == otherTab.text) {
          // そもそも全く同じ名前
          otherTab.level = -1;
          continue;
        }
        int minLevel = math::min(newTab.level, otherTab.level);
        while (str::equals_nocase(GetTextForTab(newTab.text, minLevel), GetTextForTab(otherTab.text, minLevel))) {
          ++minLevel;
        }
        newTab.level = math::max(newTab.level, minLevel);
        otherTab.level = math::max(otherTab.level, minLevel);
      }
      for (int i = 0; i < count; ++i) {
        if (i == index) continue;
        int level = names[i].level;
        if (level < 0) level = newTab.level;
        SetTabText(i, GetTextForTab(names[i].text, level));
      }
      return GetTextForTab(newTab.text, newTab.level);
    } else {
      PCWSTR s = name.str();
      int ln = name.length();
      if (ln >= 2 && (s[1] == _T(':') || (s[0] == _T('\\') && s[1] == _T('\\'))))
        return PathFindFileName(s);
      else if (afx::PathIsRegistory(s))
        return PathFindFileName(s);
      else
        return name;
    }
  }
  static PCTSTR GetTextForTab(string name, int level) {
    PCWSTR text = name.str();
    int ln = name.length();
    if ((ln >= 2 && (text[1] == _T(':') || (text[0] == _T('\\') && text[1] == _T('\\')))) || afx::PathIsRegistory(text)) {
      for (size_t i = ln; i > 0; --i) {
        switch (text[i - 1]) {
          case L'\\':
          case L'/':
            if (--level <= 0) return text + i;
            break;
        }
      }
    }
    return name.str();
  }
  void CompactPath(PTSTR buffer) {
    HDC hDC = GetDC();
    m_look.CompactPath(hDC, buffer, m_TabWidthMin, m_TabWidthMax);
    ReleaseDC(hDC);
  }
  BOOL InsertTab(int index, Item& item) {
    TCHAR buffer[MAX_PATH];
    str::copy(buffer, (PCTSTR)item.iString);
    item.iString = (INT_PTR)buffer;
    CompactPath(buffer);
    return InsertButton(index, &item);
  }
  void SetTabText(int index, PCTSTR text) {
    TCHAR buffer[MAX_PATH];
    str::copy(buffer, text);
    CompactPath(buffer);
    SetItemText(index, buffer);
  }

 private:  // IDropTarget
  ref<IDropTarget> m_drop;
  ref<IDataObject> m_pDropData;
  DWORD m_dwDropEffect;
  DWORD m_LastMouseTime;
  POINTL m_LastMousePos;

  static const UINT AUTOACTIVATE_ID = 1234;
  static const UINT AUTOACTIVATE_CHECK = 100;      // ms
  static const UINT AUTOACTIVATE_HOVER_MIN = 500;  // ms

  static const UINT HOTPOPUP_ID = 1235;
  static const UINT HOTPOPUP_DELAY = 100;  // ms

  void KillAutoActivateTimer() {
    KillTimer(AUTOACTIVATE_ID);
    m_pDropData.clear();
    m_dwDropEffect = 0;
  }

  LRESULT OnTimer(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    switch (wParam) {
      case AUTOACTIVATE_ID:
        if (ref<IWindow> w = cast(m_drop)) {
          if (!w->Visible) {
            UINT AUTOACTIVATE_HOVER = 0;
            SystemParametersInfo(SPI_GETMOUSEHOVERTIME, 0, &AUTOACTIVATE_HOVER, 0);
            // そのまま使うと体感的にかなり短めのなので、ツールチップホバーの2倍または500ms の長いほうにする。
            AUTOACTIVATE_HOVER *= 2;
            if (AUTOACTIVATE_HOVER < AUTOACTIVATE_HOVER_MIN) AUTOACTIVATE_HOVER = AUTOACTIVATE_HOVER_MIN;
            if (GetTickCount() - m_LastMouseTime > AUTOACTIVATE_HOVER) {
              SetStatus(w, SELECTED, false);
            }
          }
        }
        break;
      case HOTPOPUP_ID:
        HotPopup_OnTimer();
        break;
      default:
        bHandled = false;
        break;
    }
    return 0;
  }
  ref<IDropTarget> QueryDropTargetInTab(POINTL ptScreen) {
    Point pt(ptScreen.x, ptScreen.y);
    ScreenToClient(&pt);
    int index = HitTest(&pt);
    if (index > 0) {
      SetHotItem(index);
      HWND hWnd = GetItemData(index);
      ref<IDropTarget> pDropTarget;
      if (QueryDropTargetInWindow(hWnd, &pDropTarget, (const POINT&)ptScreen)) return pDropTarget;
    }
    SetHotItem(-1);
    return null;
  }

 public:  // IDropTarget
  bool HandleQueryDrop(IDropTarget** pp, LPARAM lParam) {
    if (!__super::HandleQueryDrop(pp, lParam)) {
      Point pt(GET_XY_LPARAM(lParam));
      ScreenToClient(&pt);
      int index = HitTest(&pt);
      if (index <= 0) return false;
      QueryInterface(pp);
    }
    return true;
  }
  STDMETHODIMP DragEnter(IDataObject* pDataObject, DWORD key, POINTL pt, DWORD* pdwEffect) {
    m_LastMousePos = pt;
    m_LastMouseTime = GetTickCount();

    SetTimer(AUTOACTIVATE_ID, AUTOACTIVATE_CHECK, NULL);
    m_dwDropEffect = *pdwEffect;
    m_pDropData = pDataObject;
    if (m_drop = QueryDropTargetInTab(pt)) m_drop->DragEnter(pDataObject, key, pt, pdwEffect);
    // ここで拒否すると今後のDragOverが呼ばれないので、とりあえずすべてを受け入れる
    *pdwEffect = DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK;
    return S_OK;
  }
  STDMETHODIMP DragOver(DWORD key, POINTL pt, DWORD* pdwEffect) {
    DWORD dwCurrentTick = GetTickCount();

    if (UpdateToolTip()) afx::TipRelayEvent(m_tip, m_hWnd, pt.x, pt.y);

    // マウスホバー
    if (m_LastMousePos.x != pt.x || m_LastMousePos.y != pt.y) {
      m_LastMousePos = pt;
      m_LastMouseTime = dwCurrentTick;
    }

    ref<IDropTarget> pDropTarget = QueryDropTargetInTab(pt);
    if (!objcmp(pDropTarget, m_drop)) {  // ターゲットが代わったので、IDropTargetをエミュレートする
      if (m_drop) m_drop->DragLeave();
      m_drop = pDropTarget;
      if (m_drop) {
        DWORD dwCopy = m_dwDropEffect;
        HRESULT hr = m_drop->DragEnter(m_pDropData, key, pt, &dwCopy);
        if FAILED (hr) {
          m_drop.clear();
          return hr;
        }
        return m_drop->DragOver(key, pt, pdwEffect);
      } else {
        return E_NOTIMPL;
      }
    } else {
      if (m_drop)
        return m_drop->DragOver(key, pt, pdwEffect);
      else
        return E_NOTIMPL;
    }
  }
  STDMETHODIMP DragLeave() {
    KillAutoActivateTimer();
    if (m_drop) {
      m_drop->DragLeave();
      m_drop.clear();
    }
    SetHotItem(-1);
    return S_OK;
  }
  STDMETHODIMP Drop(IDataObject* pDataObject, DWORD key, POINTL pt, DWORD* pdwEffect) {
    KillAutoActivateTimer();

    HRESULT hr = E_NOTIMPL;
    if (m_drop) {
      ref<IDropTarget> tmp = m_drop;
      m_drop.clear();
      hr = tmp->Drop(pDataObject, key, pt, pdwEffect);
    } else {
      TRACE(L"TabPanel.Drop() - no drop target");
    }
    SetHotItem(-1);
    return hr;
  }
};

AVESTA_EXPORT(TabPanel)

}  // namespace ui
}  // namespace mew

CBitmap mew::ui::TabPanel::NotifyTip::s_bitmap;