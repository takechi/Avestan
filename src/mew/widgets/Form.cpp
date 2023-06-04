// Form.cpp

#include "stdafx.h"
#include "../private.h"
#include "WindowImpl.h"
#include "MenuProvider.hpp"
#include "impl/WTLSplitter.hpp"
#include "std/array_set.hpp"
#include "shell.hpp"
#include "drawing.hpp"

static bool ProcessMouseGesture(HWND hwnd, const MSG* msg);

namespace {
template <class TFinal, class TList>
class __declspec(novtable) DockBase
    : public mew::ui::WindowImpl<mew::ui::CWindowImplEx<TFinal, mew::ui::WindowImplBase>, TList> {
 protected:
  mew::ui::CWindowEx m_wndLastFocus;

 public:
  mew::Size GetBorder() const { return mew::Size::Zero; }

 public:
  struct CompareDockStyle {
    bool operator()(mew::ui::IWindow* lhs, mew::ui::IWindow* rhs) const { return lhs->Dock > rhs->Dock; }
  };
  static void Move(HDWP& hDWP, mew::ui::IWindow* view, int x, int y, int w, int h) {
    ASSERT(view);
    ASSERT(view->Handle);
    hDWP = ::DeferWindowPos(hDWP, view->Handle, nullptr, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
  }
  void HandleUpdateLayout() {
    if (IsIconic() || !Visible) {
      return;
    }
    UpdateLayout(ClientArea);
  }
  void UpdateLayout(mew::Rect rc) {
    if (IsIconic() || !Visible) {
      return;
    }
    mew::Size border = final.GetBorder();
    using Views = mew::array_set<mew::ref<IWindow>, false, CompareDockStyle>;
    Views views;
    for (CWindowEx w = GetWindow(GW_CHILD); w; w = w.GetWindow(GW_HWNDNEXT)) {
      mew::ref<IWindow> view;
      if (FAILED(QueryInterfaceInWindow(w, &view)) || !view->Visible || view->Dock == mew::ui::DirNone) {
        continue;
      }
      views.insert(view);
    }
    if (views.empty()) {
      return;
    }
    HDWP hDWP = ::BeginDeferWindowPos(views.size());
    for (Views::const_iterator i = views.begin(); i != views.end(); ++i) {
      mew::Rect bounds = (*i)->Bounds;
      switch ((*i)->Dock) {
        case mew::ui::DirCenter:
          Move(hDWP, *i, rc.x, rc.y, rc.w, rc.h);
          break;
        case mew::ui::DirWest:
          Move(hDWP, *i, rc.x, rc.y, bounds.w, rc.h);
          rc.left += bounds.w + border.w;
          break;
        case mew::ui::DirEast:
          Move(hDWP, *i, rc.right - bounds.w, rc.y, bounds.w, rc.h);
          rc.right -= bounds.w + border.w;
          break;
        case mew::ui::DirNorth:
          Move(hDWP, *i, rc.x, rc.y, rc.w, bounds.h);
          rc.top += bounds.h + border.h;
          break;
        case mew::ui::DirSouth:
          Move(hDWP, *i, rc.x, rc.bottom - bounds.h, rc.w, bounds.h);
          rc.bottom -= bounds.h + border.h;
          break;
        default:
#ifdef _DEBUG
          TRACE(_T("error: Invalid Dock Value : $1"), (DWORD)(*i)->Dock);

#endif
          TRESPASS();
      }
    }
    ::EndDeferWindowPos(hDWP);
  }

 public:  // msg map
  BEGIN_MSG_MAP_(HandleWindowMessage)
  MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
  CHAIN_MSG_MAP_TO(__super::HandleWindowMessage)
  END_MSG_MAP()

  LRESULT OnSetFocus(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
    bHandled = false;
    CWindowEx lost = (HWND)wParam;
    if (m_wndLastFocus != lost && m_wndLastFocus.IsWindow() && m_wndLastFocus.IsWindowVisible() &&
        m_wndLastFocus.IsWindowEnabled()) {
      m_wndLastFocus.SetFocus();
    } else {
      m_wndLastFocus = NULL;
      for (CWindowEx w = GetWindow(GW_CHILD); w; w = w.GetWindow(GW_HWNDNEXT)) {
        mew::ref<mew::ui::IWindow> p;
        if (SUCCEEDED(QueryInterfaceInWindow(w, &p)) && p->Dock == mew::ui::DirCenter) {
          m_wndLastFocus = w.m_hWnd;
          p->Focus();
          break;
        }
      }
    }
    return 0;
  }
};
}  // namespace

//==============================================================================

namespace mew {
namespace ui {

//==============================================================================

class DockPanel : public DockBase<DockPanel, implements<IWindow, ISignal, IDisposable>>, public WTLEX::CSplitter<DockPanel> {
 public:
  void DoCreate(CWindowEx parent) { __super::DoCreate(parent); }
  Size GetBorder() const { return m_Border; }
  static int Distance(const Point& pt, const Rect& rc) {
    int dx, dy;
    if (pt.x < rc.left) {
      dx = rc.left - pt.x;
    } else if (pt.x > rc.right) {
      dx = pt.x - rc.right;
    } else {
      dx = 0;
    }
    if (pt.y < rc.top) {
      dy = rc.top - pt.y;
    } else if (pt.y > rc.bottom) {
      dy = pt.y - rc.bottom;
    } else {
      dy = 0;
    }
    return dx + dy;
  }

 public:  // message map
  DECLARE_WND_CLASS_EX(_T("mew.ui.DockPanel"), CS_BYTEALIGNWINDOW, -1)

  BEGIN_MSG_MAP_(HandleWindowMessage)
  MESSAGE_HANDLER(WM_PAINT, OnPaint)  // WM_ERASEBKGNDよりも、クリッピングの関係でWM_PAINTのほうがよさげ
  MESSAGE_HANDLER(WM_PRINTCLIENT, OnPrintClient)
  MESSAGE_HANDLER(WM_PARENTNOTIFY, OnParentNotify)
  CHAIN_MSG_MAP_TO(ProcessSplitterMessage)
  CHAIN_MSG_MAP_TO(__super::HandleWindowMessage)
  END_MSG_MAP()

  HRESULT Send(message msg) {
    if (!m_hWnd) {
      return E_FAIL;
    }
    return __super::Send(msg);
  }
  LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL&) {
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(&ps);
    OnDraw(hDC, ps.rcPaint);
    EndPaint(&ps);
    return 0;
  }
  LRESULT OnPrintClient(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    HDC hDC = (HDC)wParam;
    if (lParam & PRF_CLIENT) {
      OnDraw(hDC, ClientArea);
    }
    return 0;
  }
  void OnDraw(HDC hDC, const RECT& rcUpdate) { ::FillRect(hDC, &rcUpdate, ::GetSysColorBrush(COLOR_3DFACE)); }
  LRESULT OnParentNotify(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    UINT uMsg = LOWORD(wParam);
    switch (uMsg) {
      case WM_CREATE:
      case WM_DESTROY:
        if (::GetParent((HWND)lParam) == m_hWnd) {
          PostMessage(MEW_ECHO_UPDATE);
        }
        break;
    }
    bHandled = false;
    return 0;
  }
  bool IsDragTarget(CWindowEx w) {
    ref<IWindow> v;
    return SUCCEEDED(QueryInterfaceInWindow(w, &v)) && v->Visible && v->Dock != DirNone;
  }
  void OnResizeDragTargets(DragMode mode, CWindowEx wndDrag[2], Point pt) {
    ref<IWindow> view[2];
    VERIFY_HRESULT(QueryInterfaceInWindow(wndDrag[0], &view[0]));
    VERIFY_HRESULT(QueryInterfaceInWindow(wndDrag[1], &view[1]));
    switch (mode) {
      case DragHorz:
        if (view[0]->Dock == DirWest) {
          Rect bounds = view[0]->Bounds;
          bounds.right += pt.x - m_DragStart.x;
          view[0]->Bounds = bounds;
        }
        if (view[1]->Dock == DirEast) {
          Rect bounds = view[1]->Bounds;
          bounds.left += pt.x - m_DragStart.x;
          view[1]->Bounds = bounds;
        }
        break;
      case DragVert:
        if (view[0]->Dock == DirNorth) {
          Rect bounds = view[0]->Bounds;
          bounds.bottom += pt.y - m_DragStart.y;
          view[0]->Bounds = bounds;
        }
        if (view[1]->Dock == DirSouth) {
          Rect bounds = view[1]->Bounds;
          bounds.top += pt.y - m_DragStart.y;
          view[1]->Bounds = bounds;
        }
        break;
      default:
        TRESPASS();
    }
    PostMessage(MEW_ECHO_UPDATE);
  }
};

//==============================================================================

class Form : public DockBase<Form, implements<IForm, ITree, IWindow, ISignal, IDisposable, IDropTarget>>, public MenuProvider {
 private:  // variables
  ref<ITreeItem> m_root;
  bool m_tasktray;

 public:  // Object
  HWND Create(HWND hParent, PCTSTR name, PCTSTR classname, Direction dock = DirNone, DWORD dwStyle = 0, DWORD dwExStyle = 0) {
    set_Dock(dock);

    if (GetWndClassInfo().m_lpszOrigName == NULL) {
      GetWndClassInfo().m_lpszOrigName = GetWndClassName();
    }
    GetWndClassInfo().m_wc.lpszClassName = (!str::empty(classname) ? classname : _T("mew.ui.Form"));
    ATOM atom = GetWndClassInfo().Register(&m_pfnSuperWindowProc);

    dwStyle = GetWndStyle(dwStyle);
    dwExStyle = GetWndExStyle(dwExStyle);

    HWND hwnd =
        CWindowImplBaseT<WindowImplBase, CControlWinTraits>::Create(hParent, rcDefault, name, dwStyle, dwExStyle, 0U, atom, 0);
    GetWndClassInfo().m_wc.lpszClassName = null;
    return hwnd;
  }
  void __init__(IUnknown* arg) {
    m_tasktray = false;
    m_CopyMode = CopyNone;
    m_hwndNextCopy = null;
    HWND hWndParent = null;
    if (ref<IWindow> window = cast(arg)) {
      hWndParent = window->Handle;
    }
    //
    io::Path path;
    PCTSTR name = null, classname = null;
    io::Version version;
    ::GetModuleFileName(null, path, MAX_PATH);
    //
    if (!hWndParent || hWndParent == ::GetDesktopWindow()) {
      if (version.Open(path.str())) {
        classname = version.QueryValue(_T("ProductName"));
        if (classname) {
          name = classname;
        } else {
          name = path.FindLeaf();
        }
      }
    }
    //
    HWND hWnd =
        Create(hWndParent, name, classname, DirNone, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);
    if (!hWnd) {
      throw mew::exceptions::RuntimeError(string(IDS_ERR_CREATEWINDOW), AtlHresultFromLastError());
    }
    // ウィンドウアイコンをEXEと同じものにする.
    HICON hIconSmall = null, hIconLarge = null;
    io::Path pathIcon = path;
    pathIcon.Append(L"..\\..\\usr\\main.ico");
    if (PathFileExists(pathIcon)) {
      ExtractIconEx(pathIcon, 0, &hIconLarge, &hIconSmall, 1);
    } else {
      ExtractIconEx(path, 0, &hIconLarge, &hIconSmall, 1);
    }
    SetIcon(hIconLarge, true);
    SetIcon(hIconSmall, false);
  }

 public:  // ISignal
  bool SupportsEvent(EventCode code) const throw() {
    if (__super::SupportsEvent(code)) {
      return true;
    }
    switch (code) {
      case EventMouseWheel:
        return true;
      default:
        return false;
    }
  }

 public:  // ITree
  ITreeItem* get_Root() { return m_root; }
  void set_Root(ITreeItem* value) {
    if (m_root) {
      DisposeMenu(GetSystemMenu(false), false);
    }
    m_root = value;
    GetSystemMenu(true);  // revert
    if (m_root) {
      CMenuHandle sysmenu = GetSystemMenu(false);
      sysmenu.InsertMenu(0, MF_BYPOSITION | MF_SEPARATOR);
      ConstructMenu(m_root, sysmenu, 0);
    }
  }
  IImageList* get_ImageList() { return GetMenuImageList(); }
  void set_ImageList(IImageList* value) { SetMenuImageList(value); }

 public:  // message map
  DECLARE_WND_CLASS_EX(NULL, CS_BYTEALIGNWINDOW, -1)

  BEGIN_MSG_MAP_(HandleWindowMessage)
  // menu events
  CHAIN_MSG_MAP_TO(__super::HandleMenuMessage)
  // internal
  MESSAGE_HANDLER(WM_FORWARDMSG, OnForwardMsg)
  MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
  MSG_HANDLER(WM_UPDATEUISTATE, OnUpdateUIState)
  MSG_HANDLER(WM_SETTINGCHANGE, OnBroadcastMessage)
  MSG_HANDLER(WM_DEVICECHANGE, OnBroadcastMessage)
  MSG_HANDLER(WM_DRAWCLIPBOARD, OnDrawClipboard)
  MSG_HANDLER(WM_CHANGECBCHAIN, OnChangeCBChain)
  MSG_HANDLER(MEW_NM_TASKTRAY, OnNotifyTaskTray)
  //
  CHAIN_MSG_MAP_TO(__super::HandleWindowMessage)
  // mouse events
  MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
  END_MSG_MAP()

  HRESULT Send(message msg) {
    if (!m_hWnd) {
      return false;
    }
    switch (msg.code) {
      case CommandMaximize:
        if (IsWindowEnabled()) {
          this->Visible = true;
          if (!IsZoomed()) {
            PostMessage(WM_SYSCOMMAND, SC_MAXIMIZE, -1);
          }
        }
        break;
      case CommandMinimize:
        if (IsWindowEnabled() && !IsIconic()) {
          PostMessage(WM_SYSCOMMAND, SC_MINIMIZE, -1);
        }
        break;
      case CommandRestore:
        if (IsWindowEnabled()) {
          this->Visible = true;
          if ((IsZoomed() || IsIconic())) {
            PostMessage(WM_SYSCOMMAND, SC_RESTORE, -1);
          }
        }
        break;
      case CommandResize:
        if (IsWindowEnabled() && this->Visible && !IsIconic() && !IsZoomed() && (GetStyle() & WS_THICKFRAME)) {
          PostMessage(WM_SYSCOMMAND, SC_SIZE, -1);
        }
        break;
      case CommandMenu: {
        Point pt(0, 0);
        ClientToScreen(&pt);
        PopupSystemContextMenu(pt);
        break;
      }
      case CommandMove:
        if (IsWindowEnabled() && this->Visible && !IsIconic() && !IsZoomed()) {
          PostMessage(WM_SYSCOMMAND, SC_MOVE, -1);
        }
        break;
      default:
        return __super::Send(msg);
    }
    return true;
  }
  void HandleSetText(PCTSTR name) {
    if (m_tasktray) {  // nodify trayicon tooltip
      NOTIFYICONDATA icon = {sizeof(NOTIFYICONDATA)};
      icon.hWnd = m_hWnd;
      icon.uID = (UINT)this;
      icon.uFlags = NIF_TIP;
      str::copy(icon.szTip, name, 64);
      Shell_NotifyIcon(NIM_MODIFY, &icon);
    }
  }

  //==============================================================================
  // Internal msg

  LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    ::PostMessage(null, WM_UPDATEUISTATE, 0, 0);
    bHandled = false;
    return 0;
  }

  LRESULT OnForwardMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    MSG* msg = (MSG*)lParam;
    switch (msg->message) {
      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_MBUTTONDOWN:
      case WM_XBUTTONDOWN:
      case WM_LBUTTONDBLCLK:
      case WM_RBUTTONDBLCLK:
      case WM_MBUTTONDBLCLK:
      case WM_XBUTTONDBLCLK:
        // マウスジェスチャサポート.
        if (ProcessMouseGesture(m_hWnd, msg)) {
          return true;
        }
        break;
      default:
        break;
    }
    bHandled = false;
    return false;
  }
  bool OnUpdateUIState(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT&) {
    SendMessageToDescendants(uMsg, wParam, lParam, ::IsWindowVisible);
    CWindowEx focus = ::GetFocus();
    if (IsChild(focus) && (focus.GetStyle() & WS_CHILD)) {
      m_wndLastFocus = focus;
    }
    return false;
  }
  bool OnBroadcastMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT&) {
    SendMessageToDescendants(uMsg, wParam, lParam);
    return false;
  }
  bool OnNotifyTaskTray(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT&) {
    ASSERT(m_tasktray);
    if (lParam == WM_MOUSEMOVE) {
      return true;
    }
    switch (lParam) {
      case WM_RBUTTONUP:
      case WM_CONTEXTMENU: {
        Point pt;
        GetCursorPos(&pt);
        PopupSystemContextMenu(pt);
        break;
      }
      case WM_LBUTTONDOWN:
        if (!Visible || IsIconic()) {
          Send(CommandRestore);
        } else {
          Send(CommandMinimize);
        }
        break;
      default:
        break;
    }
    return true;
  }
  void PopupSystemContextMenu(Point pt) {
    CMenuHandle menu = GetSystemMenu(false);
    if (!menu) {
      return;
    }
    // 前回使ったときの有効性が残っているので、自前でOn/Offを設定する
    if (IsIconic()) {
      menu.EnableMenuItem(SC_MAXIMIZE, MF_BYCOMMAND | MF_ENABLED);
      menu.EnableMenuItem(SC_MINIMIZE, MF_BYCOMMAND | MF_GRAYED);
      menu.EnableMenuItem(SC_RESTORE, MF_BYCOMMAND | MF_ENABLED);
      menu.EnableMenuItem(SC_MOVE, MF_BYCOMMAND | MF_GRAYED);
      menu.EnableMenuItem(SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
    } else if (IsZoomed()) {
      menu.EnableMenuItem(SC_MAXIMIZE, MF_BYCOMMAND | MF_GRAYED);
      menu.EnableMenuItem(SC_MINIMIZE, MF_BYCOMMAND | MF_ENABLED);
      menu.EnableMenuItem(SC_RESTORE, MF_BYCOMMAND | MF_ENABLED);
      menu.EnableMenuItem(SC_MOVE, MF_BYCOMMAND | MF_GRAYED);
      menu.EnableMenuItem(SC_SIZE, MF_BYCOMMAND | MF_GRAYED);
    } else {
      menu.EnableMenuItem(SC_MAXIMIZE, MF_BYCOMMAND | MF_ENABLED);
      menu.EnableMenuItem(SC_MINIMIZE, MF_BYCOMMAND | MF_ENABLED);
      menu.EnableMenuItem(SC_RESTORE, MF_BYCOMMAND | MF_GRAYED);
      menu.EnableMenuItem(SC_MOVE, MF_BYCOMMAND | MF_ENABLED);
      menu.EnableMenuItem(SC_SIZE, MF_BYCOMMAND | MF_ENABLED);
    }
    // これをしないと、非アクティブになったときにメニューが消えなくなる.
    SetForegroundWindow(m_hWnd);
    if (UINT wID = menu.TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, m_hWnd, null)) {
      if (ref<ITreeItem> item = FindByCommand(menu, wID)) {
        if (ref<ICommand> command = item->Command) {
          command->Invoke();
        }
      } else {
        PostMessage(WM_SYSCOMMAND, wID, MAKELPARAM(pt.x, pt.y));
      }
    }
  }

  //==============================================================================
  // Input event

  LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    Point where(Point(GET_XY_LPARAM(lParam)));
    ScreenToClient(&where);  // WM_MOUSEWHEELのマウス位置はスクリーン座標系.
    UINT32 state = GET_KEYSTATE_WPARAM(wParam);
    if (IsKeyPressed(VK_MENU)) {
      state |= ModifierAlt;
    }
    if (IsKeyPressed(VK_LWIN) || IsKeyPressed(VK_RWIN)) {
      state |= ModifierWindows;
    }
    INT32 wheel = GET_WHEEL_DELTA_WPARAM(wParam);
    InvokeEvent<EventMouseWheel>(this, where, state, wheel);
    return 0;
  }

  //==============================================================================
  // Generic event

  void HandleDestroy() {
    // dispose extended copy
    set_AutoCopy(CopyNone);
    // dispose drag drop
    DisposeDragDrop();
    if (HICON hIconLarge = GetIcon(true)) {
      ::DestroyIcon(hIconLarge);
      SetIcon(NULL, true);
    }
    // dispose tasktray
    set_TaskTray(false);
    // dispose icons
    if (HICON hIconSmall = GetIcon(false)) {
      ::DestroyIcon(hIconSmall);
      SetIcon(NULL, false);
    }
    // dispose menu
    if (m_root) {
      DisposeMenu(GetSystemMenu(false), false);
    }
    m_root.clear();
  }

 public:  // IDropTarget
  ref<IDropTargetHelper> m_pDropTargetHelper;
  ref<IDropTarget> m_drop;
  ref<IDataObject> m_pDropData;
  DWORD m_dwDropEffect;
  DWORD m_dwDragKeys;

  ref<IDropTarget> QueryDropTargetToDescent(POINTL ptScreen) {
    HWND hWnd = m_hWnd;
    // ChildWindowFromPoint() は、直下の子供しか検索しないため、ループして子供を下っていく.
    while (true) {
      POINT ptClient = {ptScreen.x, ptScreen.y};
      ::ScreenToClient(hWnd, &ptClient);
      HWND hChild = ::ChildWindowFromPointEx(hWnd, ptClient, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED | CWP_SKIPTRANSPARENT);
      if (!hChild || hChild == hWnd) {  // hChild == null は、位置がクライアント領域外の場合に起こりうる.
        // hChild == hWnd は、位置に子供がない場合に起こりうる.
        break;
      }
      hWnd = hChild;
    }
    for (; hWnd; hWnd = ::GetAncestor(hWnd, GA_PARENT)) {
      ref<IDropTarget> pDropTarget;
      if (QueryDropTargetInWindow(hWnd, &pDropTarget, (const POINT&)ptScreen)) {
        return pDropTarget;
      }
    }
    return null;
  }
  void DisposeDragDrop() {
    m_pDropData.clear();
    m_dwDropEffect = 0;
    if (m_drop) {
      m_drop->DragLeave();
      m_drop.clear();
    }
  }
  STDMETHODIMP DragEnter(IDataObject* pDataObject, DWORD key, POINTL pt, DWORD* pdwEffect) {
    BOOL bDragFullWindows = FALSE;
    SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &bDragFullWindows, 0);

    m_dwDropEffect = *pdwEffect;
    m_pDropData = pDataObject;
    m_dwDragKeys = 0;
    if (IsKeyPressed(VK_LBUTTON)) {
      m_dwDragKeys |= MouseButtonLeft;
    }
    if (IsKeyPressed(VK_RBUTTON)) {
      m_dwDragKeys |= MouseButtonRight;
    }
    key |= m_dwDragKeys;
    if (bDragFullWindows && !m_pDropTargetHelper) {
      CoCreateInstance(CLSID_DragDropHelper, null, CLSCTX_INPROC_SERVER, IID_IDropTargetHelper, (LPVOID*)&m_pDropTargetHelper);
    }
    if (bDragFullWindows && m_pDropTargetHelper) {
      m_pDropTargetHelper->DragEnter(m_hWnd, pDataObject, (POINT*)&pt, *pdwEffect);
    }
    if (m_drop = QueryDropTargetToDescent(pt)) {
      if (!bDragFullWindows) {
        ::LockWindowUpdate(NULL);
      }
      m_drop->DragEnter(pDataObject, key, pt, pdwEffect);
    }
    // ここで拒否すると今後のDragOverが呼ばれないので、とりあえずすべてを受け入れる
    *pdwEffect = DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK;
    return S_OK;
  }
  STDMETHODIMP DragOver(DWORD key, POINTL pt, DWORD* pdwEffect) {
    BOOL bDragFullWindows = FALSE;
    SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &bDragFullWindows, 0);

    key |= m_dwDragKeys;
    if (bDragFullWindows && m_pDropTargetHelper) {
      m_pDropTargetHelper->DragOver((POINT*)&pt, *pdwEffect);
    }
    ref<IDropTarget> pDropTarget = QueryDropTargetToDescent(pt);
    if (!objcmp(pDropTarget, m_drop)) {
      if (m_drop) m_drop->DragLeave();
      m_drop = pDropTarget;
      if (m_drop) {
        DWORD dwCopy = m_dwDropEffect;
        HRESULT hr = m_drop->DragEnter(m_pDropData, key, pt, &dwCopy);
        if FAILED (hr) {
          m_drop.clear();
          return hr;
        }
        if (!bDragFullWindows) {
          ::LockWindowUpdate(NULL);
        }
        return m_drop->DragOver(key, pt, pdwEffect);
      } else {
        return E_NOTIMPL;
      }
    } else {
      m_drop = pDropTarget;
      if (m_drop) {
        HRESULT hr = m_drop->DragOver(key, pt, pdwEffect);
        return hr;
      } else {
        return E_NOTIMPL;
      }
    }
  }
  STDMETHODIMP DragLeave() {
    DisposeDragDrop();
    if (m_pDropTargetHelper) {
      m_pDropTargetHelper->DragLeave();
    }
    return S_OK;
  }
  STDMETHODIMP Drop(IDataObject* pDataObject, DWORD key, POINTL pt, DWORD* pdwEffect) {
    key |= m_dwDragKeys;
    m_pDropData.clear();
    m_dwDropEffect = 0;

    HRESULT hr = E_NOTIMPL;
    if (m_drop) {
      ref<IDropTarget> tmp = m_drop;
      m_drop.clear();
      hr = tmp->Drop(pDataObject, key, pt, pdwEffect);
    } else if (ref<IDropTarget> pDropTarget = QueryDropTargetToDescent(pt)) {
      hr = pDropTarget->Drop(pDataObject, key, pt, pdwEffect);
    } else {
      TRACE(_T("info: Drop() - no drop target"));
    }
    if (m_pDropTargetHelper) {
      m_pDropTargetHelper->Drop(pDataObject, (POINT*)&pt, *pdwEffect);
    }
    return hr;
  }

 public:  // Extension
  bool get_TaskTray() { return m_tasktray; }
  void set_TaskTray(bool value) {
    if (m_tasktray == value) {
      return;
    }
    if (m_tasktray && !value) {  // disable
      NOTIFYICONDATA icon = {sizeof(NOTIFYICONDATA)};
      icon.hWnd = m_hWnd;
      icon.uID = (UINT)this;
      Shell_NotifyIcon(NIM_DELETE, &icon);
    } else if (!m_tasktray && value) {  // enable
      NOTIFYICONDATA icon = {sizeof(NOTIFYICONDATA)};
      icon.hWnd = m_hWnd;
      icon.uID = (UINT)this;
      icon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
      icon.uCallbackMessage = MEW_NM_TASKTRAY;
      icon.hIcon = GetIcon(ICON_SMALL);
      this->Name.copyto(icon.szTip, 63);
      Shell_NotifyIcon(NIM_ADD, &icon);
    }
    m_tasktray = value;
    // Taskbar restart msg will be sent to us if the icon needs to be redrawn
    //  gs_msgRestartTaskbar = RegisterWindowMessage("TaskbarCreated");
  }

  CopyMode m_CopyMode;
  HWND m_hwndNextCopy;

  CopyMode get_AutoCopy() { return m_CopyMode; }
  void set_AutoCopy(CopyMode value) {
    if (m_CopyMode == value) {
      return;
    }
    m_CopyMode = value;
    switch (m_CopyMode) {
      case CopyNone:
        if (m_hwndNextCopy) {
          ChangeClipboardChain(m_hwndNextCopy);
          m_hwndNextCopy = null;
        }
        break;
      default:
        if (!m_hwndNextCopy) {
          m_hwndNextCopy = SetClipboardViewer();
        }
        break;
    }
  }
  bool OnDrawClipboard(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT&) {
    switch (m_CopyMode) {
      case CopyNone:
        break;
      case CopyName:
      case CopyPath:
      case CopyBase:
        OnClipboardChange();
        break;
    }
    if (m_hwndNextCopy) {
      ::SendMessage(m_hwndNextCopy, uMsg, wParam, lParam);
    }
    return true;
  }
  bool OnChangeCBChain(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT&) {
    HWND hwndRemove = (HWND)wParam;
    HWND hwndNext = (HWND)lParam;
    if (m_hwndNextCopy == hwndRemove) {
      m_hwndNextCopy = hwndNext;
    } else if (m_hwndNextCopy) {
      ::SendMessage(m_hwndNextCopy, WM_CHANGECBCHAIN, wParam, lParam);
    }
    return true;
  }
  void OnClipboardChange() {
    UINT cbFormat = CF_HDROP;
    if (GetPriorityClipboardFormat(&cbFormat, 1) == CF_HDROP) {
      if (OpenClipboard()) {
        if (HDROP hDrop = (HDROP)GetClipboardData(CF_HDROP)) {
          int count = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
          bool first = true;
          Stream stream;
          HGLOBAL hGlobal = io::StreamCreateOnHGlobal(&stream, 0, false);
          for (int i = 0; i < count; ++i) {
            WCHAR path[MAX_PATH];
            PCWSTR start = path;
            int len = DragQueryFile(hDrop, i, path, MAX_PATH);
            switch (m_CopyMode) {
              case CopyName:
                start = PathFindFileName(path);
                len = str::length(start);
              case CopyPath:
                break;  // ok;
              case CopyBase:
                start = PathFindFileName(path);
                len = PathFindExtension(start) - start;
                break;
              default:
                TRESPASS();
            }
            if (first) {
              first = false;
            } else {
              stream.write(L"\r\n", 2 * sizeof(WCHAR));
            }
            stream.write(start, len * sizeof(WCHAR));
          }
          stream.write(L"\0", sizeof(WCHAR));
          stream.clear();
          ::SetClipboardData(CF_UNICODETEXT, hGlobal);
        }
        CloseClipboard();
      }
    }
  }
};

AVESTA_EXPORT(DockPanel)
AVESTA_EXPORT(Form)

}  // namespace ui
}  // namespace mew

//==============================================================================

namespace {
const int GESTURE_SKIP_DISTANCE = 10;  // [px] この距離以下のマウス移動は感知しない
const int GESTURE_WAIT_TIME = 300;  // [msec] これ以上時間が経った場合は新たなジェスチャとみなす（↑↑など）

struct GestureDesc {
  mew::ui::CWindowEx owner;
  mew::ui::CWindowEx window;
  mew::Point cursor;
  DWORD lastTime;
  UINT16 lastMods;
  std::vector<mew::ui::Gesture> history;
  mew::ref<mew::ui::IGesture> gesture;
};

static mew::ui::Gesture MessageToGesture(const MSG* msg) {
  switch (msg->message) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
      return mew::ui::GestureButtonLeft;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
      return mew::ui::GestureButtonRight;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
      return mew::ui::GestureButtonMiddle;
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_XBUTTONDBLCLK:
      return GET_XBUTTON_WPARAM(msg->wParam) == XBUTTON1 ? mew::ui::GestureButtonX1 : mew::ui::GestureButtonX2;
    case WM_MOUSEWHEEL:
      return GET_WHEEL_DELTA_WPARAM(msg->wParam) > 0 ? mew::ui::GestureWheelUp : mew::ui::GestureWheelDown;
    default:
      return (mew::ui::Gesture)-1;
  }
}
static size_t MessageToClicks(const MSG* msg) {
  switch (msg->message) {
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN:
      return 1;
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDBLCLK:
      return 2;
    default:
      return 0;
  }
}

HRESULT EndGesture(GestureDesc& desc) {
  if (!desc.gesture) {
    return S_FALSE;
  }
  if (::GetCapture() == desc.owner) {
    ::ReleaseCapture();
  }
  HRESULT hr = S_OK;
  if (desc.gesture) {
    hr = desc.gesture->OnGestureFinish(0, 0, 0);
  }
  desc.history.clear();
  desc.gesture.clear();
  return hr;
}

// ジェスチャシーケンスが変化した時に呼ばれる。
void OnGestureUpdate(GestureDesc& desc) {
  ASSERT(desc.gesture);
  if (desc.gesture) {
    ASSERT(!desc.history.empty());
    desc.gesture->OnGestureUpdate(desc.lastMods, desc.history.size(), &desc.history[0]);
  }
}

// ジェスチャが正常に完了した場合に呼ばれる。
HRESULT OnGestureFinish(GestureDesc& desc) {
  ASSERT(desc.gesture);
  if (!desc.gesture) {
    return E_FAIL;
  }
  ASSERT(!desc.history.empty());
  return desc.gesture->OnGestureFinish(desc.lastMods, desc.history.size(), &desc.history[0]);
}

void OnGestureButton(GestureDesc& desc, mew::ui::Gesture what) {
  mew::Point pt;
  ::GetCursorPos(&pt);
  desc.cursor = pt;
  desc.history.push_back(what);
  desc.lastTime = ::GetTickCount();
  OnGestureUpdate(desc);
}

void OnGestureUp(GestureDesc& desc, const MSG* msg) {
  if (!desc.gesture) {
    return;
  }
  ::ReleaseCapture();
  if (FAILED(OnGestureFinish(desc)) && desc.history.size() == 1) {  // ジェスチャが無いので、通常の右クリック
    mew::Point pt = desc.cursor;
    desc.window.ScreenToClient(&pt);
    LPARAM lParamClient = MAKELPARAM(pt.x, pt.y);
    // ≪テクニック≫
    // WM_RBUTTONUP をメッセージキューに入れた後、WM_RBUTTONDOWN を直接SendMessageする。
    // 実際に受け取る順番は、RDOWN => RUP => CONTEXTMENU。
    // SendMessage()の前にポストしておく必要があるのは、
    // 右ドラッグが有効なタイプのコントロール（リスト、ツリーなど）で
    // WM_RBUTTONDOWN から帰ってこなくなるため。
    // ちなみに、WM_RBUTTONDOWN をPostMessageすると、マウスジェスチャと絡んで無限ループになる。
    switch (msg->message) {
      case WM_LBUTTONUP:
        desc.window.PostMessage(WM_LBUTTONUP, msg->wParam, lParamClient);
        desc.window.SendMessage(WM_LBUTTONDOWN, msg->wParam | MK_LBUTTON, lParamClient);
        break;
      case WM_RBUTTONUP:
        desc.window.PostMessage(WM_RBUTTONUP, msg->wParam, lParamClient);
        desc.window.SendMessage(WM_RBUTTONDOWN, msg->wParam | MK_RBUTTON, lParamClient);
        break;
      case WM_MBUTTONUP:
        desc.window.PostMessage(WM_MBUTTONUP, msg->wParam, lParamClient);
        desc.window.SendMessage(WM_MBUTTONDOWN, msg->wParam | MK_MBUTTON, lParamClient);
        break;
      case WM_XBUTTONUP:
        desc.window.PostMessage(WM_XBUTTONUP, msg->wParam, lParamClient);
        desc.window.SendMessage(WM_XBUTTONDOWN,
                                msg->wParam | (GET_XBUTTON_WPARAM(msg->wParam) == XBUTTON1 ? MK_XBUTTON1 : MK_XBUTTON2),
                                lParamClient);
        break;
    }
  }
  EndGesture(desc);
}

static mew::ui::Gesture CalcDirection(int dx, int dy) {
  static const mew::ui::Gesture DIR[2][2] = {{mew::ui::GestureNorth, mew::ui::GestureWest},
                                             {mew::ui::GestureEast, mew::ui::GestureSouth}};
  return DIR[dy + dx > 0 ? 1 : 0][dy - dx > 0 ? 1 : 0];
}

void OnGestureKey(GestureDesc& desc, WPARAM, LPARAM) {
  if (desc.history.empty()) {
    return;
  }
  UINT16 modifiers = mew::ui::GetCurrentModifiers();
  if (modifiers != desc.lastMods) {
    desc.lastMods = modifiers;
    OnGestureUpdate(desc);
  }
}
void OnGestureMouseMove(GestureDesc& desc, WPARAM, LPARAM lParam) {
  mew::Point pt;
  ::GetCursorPos(&pt);

  int dx = pt.x - desc.cursor.x, dy = pt.y - desc.cursor.y;
  if ((dx * dx + dy * dy) < GESTURE_SKIP_DISTANCE * GESTURE_SKIP_DISTANCE) {  // 不感距離
    return;
  }

  mew::ui::Gesture dir = CalcDirection(dx, dy);
  desc.cursor = pt;

  DWORD dwTimeNow = ::GetTickCount();
  if (desc.history.empty() || desc.history.back() != dir ||
      (dwTimeNow - desc.lastTime) > GESTURE_WAIT_TIME) {  // 新しいジェスチャ
    desc.history.push_back(dir);
    OnGestureUpdate(desc);
  }

  desc.lastTime = dwTimeNow;
}

inline bool QueryGestureInWindow(HWND hWnd, mew::ui::IGesture** ppGesture, mew::Point ptScreen, size_t length,
                                 const mew::ui::Gesture gesture[]) {
  mew::ui::QueryGestureStruct param = {ppGesture, ptScreen, length, gesture};
  return SendMessage(hWnd, mew::ui::MEW_QUERY_GESTURE, 0, (LPARAM)&param) != 0;
}

inline mew::ref<mew::ui::IGesture> QueryGestureToAncestor(HWND hwnd, const GestureDesc& desc) {
  for (mew::ui::CWindowEx w = hwnd; w; w = ::GetAncestor(w, GA_PARENT)) {
    mew::ref<mew::ui::IGesture> gesture;
    if (QueryGestureInWindow(w, &gesture, desc.cursor, desc.history.size(), &desc.history[0]) && gesture &&
        SUCCEEDED(gesture->OnGestureAccept(hwnd, desc.cursor, desc.history.size(), &desc.history[0]))) {
      ::SetFocus(hwnd);
      return gesture;
    }
  }
  return mew::null;
}
}  // namespace

static bool ProcessMouseGesture(HWND hwnd, const MSG* msg) {
  const mew::ui::Gesture start = MessageToGesture(msg);
  const size_t clicks = MessageToClicks(msg);
  if (start == (mew::ui::Gesture)-1 || clicks == 0) {
    return false;
  }

  GestureDesc desc;
  desc.cursor.assign(GET_XY_LPARAM(msg->lParam));
  desc.owner = hwnd;
  desc.window = msg->hwnd;
  desc.window.ClientToScreen(&desc.cursor);
  for (size_t i = 0; i < clicks; ++i) {
    desc.history.push_back(start);
  }

  if (!(desc.gesture = QueryGestureToAncestor(msg->hwnd, desc))) {
    return false;
  }

  ::SetCapture(desc.owner);
  desc.lastTime = ::GetTickCount();
  desc.lastMods = mew::ui::GetCurrentModifiers();
  OnGestureUpdate(desc);

  while (desc.gesture) {
    MSG msg_;
    if (!::GetMessage(&msg_, NULL, 0, 0)) {  // WM_QUIT
      EndGesture(desc);
      PostQuitMessage(msg_.wParam);
      return false;
    }
    if (::GetCapture() != desc.owner) {
      EndGesture(desc);
      break;
    }
    switch (msg_.message) {
      case WM_MOUSEMOVE:
        OnGestureMouseMove(desc, msg_.wParam, msg_.lParam);
        break;
      case WM_LBUTTONUP:
      case WM_RBUTTONUP:
      case WM_MBUTTONUP:
      case WM_XBUTTONUP:
        if (MessageToGesture(&msg_) == start) {
          OnGestureUp(desc, &msg_);
        }
        break;
      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_MBUTTONDOWN:
      case WM_XBUTTONDOWN:
      case WM_MOUSEWHEEL:
        OnGestureButton(desc, MessageToGesture(&msg_));
        break;
      case WM_LBUTTONDBLCLK:
      case WM_RBUTTONDBLCLK:
      case WM_MBUTTONDBLCLK:
      case WM_XBUTTONDBLCLK:
        break;
      case WM_KEYDOWN:
      case WM_SYSKEYDOWN:
      case WM_KEYUP:
      case WM_SYSKEYUP:
        OnGestureKey(desc, msg_.wParam, msg_.lParam);
        break;
      default:
        ::TranslateMessage(&msg_);
        ::DispatchMessage(&msg_);
    }
  }
  return true;
}
