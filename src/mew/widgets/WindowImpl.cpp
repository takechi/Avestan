// WindowImpl.cpp

#include "stdafx.h"
#include "../private.h"
#include "WindowImpl.h"

//==============================================================================

namespace mew {
namespace ui {
BOOL CWindowEx::ModifyClassStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags) throw() {
  HWND hWnd = m_hWnd;
  ASSERT(::IsWindow(hWnd));
  DWORD dwStyle = ::GetClassLong(hWnd, GCL_STYLE);
  DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
  if (dwStyle == dwNewStyle) return false;
  ::SetClassLong(hWnd, GCL_STYLE, dwNewStyle);
  if (nFlags != 0) ::SetWindowPos(hWnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | nFlags);
  return true;
}

bool CWindowEx::DragDetect(POINT pt) {
  int dx = GetSystemMetrics(SM_CXDRAG);
  int dy = GetSystemMetrics(SM_CYDRAG);
  RECT rc = {pt.x - dx, pt.y - dy, pt.x + dx, pt.y + dy};
  ::SetCapture(m_hWnd);
  while (true) {
    MSG msg;
    PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
    if (msg.message == WM_MOUSEMOVE) {
      if (msg.hwnd == m_hWnd) {
        if ((msg.wParam & MouseButtonMask) == 0) return false;
        Point cursor(GET_XY_LPARAM(msg.lParam));
        if (!PtInRect(&rc, cursor)) {
          ::ReleaseCapture();
          return true;
        }
      } else {  // 他のウィンドウが WM_MOUSEMOVE を受け取っている！
        // キャプチャが解除されたに違いない！
        return false;
      }
    } else if (msg.message == WM_QUIT || (WM_MOUSEFIRST <= msg.message && msg.message <= WM_MOUSELAST) ||
               (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)) {
      ::ReleaseCapture();
      return false;
    }
    PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

void CWindowEx::SendMessageToDescendants(UINT msg, WPARAM wParam, LPARAM lParam, BOOL(__stdcall* op)(HWND)) throw() {
  for (HWND hwnd = ::GetTopWindow(m_hWnd); hwnd != NULL; hwnd = ::GetNextWindow(hwnd, GW_HWNDNEXT)) {
    if (op(hwnd)) {
      ::SendMessage(hwnd, msg, wParam, lParam);
      if (::GetTopWindow(hwnd) != NULL) {
        // send to child windows after parent
        CWindowEx wnd(hwnd);
        wnd.SendMessageToDescendants(msg, wParam, lParam, op);
      }
    }
  }
}

//==============================================================================

HRESULT WindowImplBase::Send(mew::ui::IWindow* self, const message& msg) const {
  using namespace mew::ui;
  switch (msg.code) {
    case CommandClose:
      self->Close(msg["sync"] | false);
      break;
    case CommandUpdate:
      self->Update(msg["sync"] | false);
      break;
    case CommandShow: {
      variant var = msg["value"];
      if (var.empty()) {  // toggle
        self->set_Visible(!self->get_Visible());
      } else {  // set as value
        self->set_Visible((bool)var);
      }
      break;
    }
    default:
      return E_FAIL;
  }
  return S_OK;
}
void WindowImplBase::set_Dock(Direction value) {
  if (m_Dock == value) {
    return;
  }
  m_Dock = (Direction)(value & (DirCenter | DirWest | DirEast | DirNorth | DirSouth));
  if (m_Dock > DirCenter) {
    Rect bounds = afx::GetBounds(m_hWnd);
    bounds.w = bounds.h = math::min(bounds.w, bounds.h);
    afx::SetBounds(m_hWnd, bounds);
    UpdateParent();
  }
}
LRESULT WindowImplBase::OnForwardMsg(IWindow* self, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
  MSG* msg = (MSG*)lParam;
  if (m_Extensions.ProcessKeymap(self, msg)) return true;
  if (HWND hParent = afx::GetParentOrOwner(m_hWnd)) return ::SendMessage(hParent, uMsg, wParam, lParam);
  bHandled = false;
  return 0;
}

LRESULT WindowImplBase::OnCopyData(IWindow* self, UINT, WPARAM wParam, LPARAM lParam, BOOL&) {
  COPYDATASTRUCT* data = (COPYDATASTRUCT*)lParam;
  if (data->dwData == 'STRW') {
    // XXX: 末尾のNULL文字を含んでしまうかも。
    string text((PCWSTR)data->lpData, data->cbData / sizeof(WCHAR));
    if (text) PostMessage(MEW_ECHO_COPYDATA, 0, (LPARAM)text.detach());
    return true;
  } else {
    return false;
  }
}

}  // namespace ui
}  // namespace mew

HRESULT mew::ui::QueryInterfaceInWindow(HWND hWnd, REFINTF pp) {
  if (!::IsWindow(hWnd)) {
    return E_FAIL;
  }
  HRESULT hr = ::SendMessage(hWnd, MEW_QUERY_INTERFACE, (WPARAM)&pp.iid, (LPARAM)pp.pp);
  if FAILED (hr) {
    return hr;
  }
  if (!*pp.pp) {
    return E_NOINTERFACE;
  }
  return S_OK;
}

bool mew::ui::ResizeToDefault(IWindow* window) {
  Size size = window->DefaultSize;
  if (size.empty()) {
    return false;
  }
  Rect bounds = window->Bounds;
  window->Bounds = Rect(bounds.location, size);
  ref<IWindow> parent;
  if FAILED (QueryParent(window, &parent)) {
    return false;
  }
  parent->Update();
  return true;
}

/*
namespace
{
        enum
        {
                WINSTATE_DESTROYED = 0x00000001, // in atlwin.h
                WINSTATE_DESTROYING = 0x00000002,
        };
}

void CWindowEx::SendMessageToDescendants(HWND hParent, UINT msg, WPARAM wParam, LPARAM lParam, BOOL (__stdcall *fn)(HWND))
throw()
{
        for(HWND hwnd = ::GetTopWindow(hParent); hwnd != NULL; hwnd = ::GetNextWindow(hwnd, GW_HWNDNEXT))
        {
                if(fn(hwnd))
                {
                        ::SendMessage(hwnd, msg, wParam, lParam);
                        SendMessageToDescendants(hwnd, msg, wParam, lParam, fn);
                }
        }
}

HWND CWindowEx::Create(HWND hWndParent, DWORD dwStyle, DWORD dwExStyle, ATL::CWndClassInfo& wc)
{
        ATOM atom = wc.Register(&m_pfnSuperWindowProc);
        CWindowImplBaseT<CWindowEx, CControlWinTraits>::Create(hWndParent, NULL, NULL, dwStyle, dwExStyle, 0U, atom, NULL);
        if(!m_hWnd)
        {
                BREAK(L"ERROR: CreateWindow()");
                throw RuntimeError(string(IDS_ERR_CREATEWINDOW), AtlHresultFromLastError());
        }
        return m_hWnd;
}

HWND CWindowEx::Create(HWND hWndParent, DWORD dwStyle, DWORD dwExStyle, LPCTSTR strClass, DWORD dwClassStyle, INT clrBkgnd)
{
        ATL::CWndClassInfo wc =
        {
                { sizeof(WNDCLASSEX), dwClassStyle, StartWindowProc,
                0, 0, NULL, NULL, NULL, (HBRUSH)(clrBkgnd + 1), NULL, strClass, NULL },
                NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
        };
        return Create(hWndParent, dwStyle, dwExStyle, wc);
}

HWND CWindowEx::Create(HWND hWndParent, DWORD dwStyle, DWORD dwExStyle, LPCTSTR strClass, LPCTSTR strSuper)
{
        ATL::CWndClassInfo wc =
        {
                { sizeof(WNDCLASSEX), 0, StartWindowProc,
                0, 0, NULL, NULL, NULL, NULL, NULL, strClass, NULL },
                strSuper, NULL, NULL, TRUE, 0, _T("")
        };
        return Create(hWndParent, dwStyle, dwExStyle, wc);
}

void CWindowEx::Dispose()
{
        m_keymap = null;
        m_gesture = null;
        if(m_msgr)
        {
                m_msgr.dispose();
                if((m_dwState & WINSTATE_DESTROYING) == 0 && IsWindow())
                        DestroyWindow();
        }
}

void CWindowEx::OnFinalMessage(HWND)
{
        Dispose(); // WM_DESTROYからも呼ばれるが、念のため。
        this->Release(); // HWND として保持される分.
}

LRESULT CWindowEx::OnNcCreate(UINT, WPARAM, LPARAM, BOOL& bHandled)
{
        m_msgr.create(__uuidof(Messenger));
        this->AddRef(); // HWND として保持される分.
        bHandled = false;
        return 0;
}

LRESULT CWindowEx::OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled)
{
        // WM_DESTROY 中に、再び DestroyWindow() できてしまう！
        // WINSTATE_DESTROYING フラグで二重は気を抑制する。
        if((m_dwState & WINSTATE_DESTROYING) == 0)
        {
                m_dwState |= WINSTATE_DESTROYING;
                // 操作系メッセージが正常に処理されるのは、WM_DESTROY直前まで。
                // そのため、この場所で　Dispose を呼び出すのがよい。
                Dispose();
        }
        bHandled = false;
        return 0;
}

LRESULT CWindowEx::OnLayout(UINT, WPARAM, LPARAM, BOOL&)
{
        MSG msg;
        if(PeekMessage(&msg, m_hWnd, WM_LAYOUT, WM_LAYOUT, PM_NOREMOVE))
        { // WM_LAYOUTメッセージが残っているので、今回はスキップ。
        }
        else
        {
                Update();
        }
        return 0;
}

LRESULT CWindowEx::OnForwardMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&)
{
        MSG* msg = (MSG*)lParam;
        if(m_keymap)
        {
                switch(msg->message)
                {
                case WM_KEYDOWN:
                case WM_SYSKEYDOWN:
                        if SUCCEEDED(m_keymap->OnKeyDown(this, GetCurrentModifiers(), (UINT8)msg->wParam))
                                return true;
                }
        }
        if(CWindow parent = GetParent())
                return parent.SendMessage(uMsg, wParam, lParam);
        return 0;
}

LRESULT CWindowEx::OnGetUnknown(UINT, WPARAM, LPARAM, BOOL&)
{
        if(!m_msgr)
        { // 破棄中、下手をするとすでにRelease()済みなので、絶対にAddRef()してはならない
                return 0;
        }
        this->AddRef();
        return (LRESULT)this->OID;
}

/*
HRESULT avesta::ui::WindowFromHandle(REFINTF pp, HWND hwnd) throw()
{
        ref<IUnknown> p;
        p.attach((IUnknown*)SendMessage(hwnd, WM_GETUNKNOWN, 0, 0));
        if(!p)
                return E_UNEXPECTED;
        return objcpy(p, pp);
}

UINT16 avesta::ui::GetCurrentModifiers()
{
        UINT16 mod = 0;
        if(IsKeyPressed(VK_MENU))     mod |= ModifierAlt;
        if(IsKeyPressed(VK_CONTROL))  mod |= ModifierControl;
        if(IsKeyPressed(VK_SHIFT))    mod |= ModifierShift;
        if(IsKeyPressed(VK_LWIN))     mod |= ModifierWindows;
        if(IsKeyPressed(VK_RWIN))     mod |= ModifierWindows;
        if(IsKeyPressed(VK_LBUTTON))  mod |= MouseButtonLeft;
        if(IsKeyPressed(VK_RBUTTON))  mod |= MouseButtonRight;
        if(IsKeyPressed(VK_MBUTTON))  mod |= MouseButtonMiddle;
        if(IsKeyPressed(VK_XBUTTON1)) mod |= MouseButtonX1;
        if(IsKeyPressed(VK_XBUTTON2)) mod |= MouseButtonX2;
        return mod;
}
*/
