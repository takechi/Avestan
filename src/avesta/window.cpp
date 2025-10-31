// window.cpp

#include "stdafx.h"
#include "avesta.hpp"
#include "afx.hpp"

namespace {

static const UINT_PTR SubclassID = 0x12345678;

bool HasStyle(HWND hwnd, DWORD dwStyle) { return 0 != (::GetWindowLong(hwnd, GWL_STYLE) & dwStyle); }

enum {
  FLAG_DYING = 1 << 0,
  FLAG_LAYOUT = 1 << 1,
};

struct BroadcastParam {
  UINT msg;
  WPARAM wParam;
  LPARAM lParam;
  INT count;
};

BOOL CALLBACK BroadcastProc(HWND hwnd, LPARAM lParam) {
  BroadcastParam* m = (BroadcastParam*)lParam;
  ::PostMessage(hwnd, m->msg, m->wParam, m->lParam);
  m->count++;
  return TRUE;
}
}  // namespace

namespace avesta {

HRESULT WindowClose(HWND hwnd) {
  ::PostMessage(hwnd, WM_CLOSE, 0, 0);
  return S_OK;
}

HRESULT WindowSetFocus(HWND hwnd) {
  if (!::IsWindowEnabled(hwnd)) {
    return S_FALSE;
  }
  afx::SetVisible(hwnd, true);
  ::SetFocus(hwnd);
  return S_OK;
}

Window::Window() : m_hwnd(nullptr), m_flags(0) {}

Window::~Window() { Dispose(); }

Window* Window::FromHandle(HWND hwnd) {
  Window* self;
  if (!::GetWindowSubclass(hwnd, MainProc, SubclassID, (DWORD_PTR*)&self)) {
    return nullptr;
  }
  ASSERT(self->m_hwnd == hwnd);
  return self;
}

Window* Window::FromHandleParent(HWND hwnd) {
  for (; hwnd; hwnd = ::GetAncestor(hwnd, GA_PARENT)) {
    if (Window* window = Window::FromHandle(hwnd)) {
      ASSERT(window || (window->m_hwnd == hwnd));
      return window;
    }
  }
  return nullptr;
}

bool Window::Attach(HWND hwnd) {
  ASSERT(!m_hwnd);
  m_flags = 0;
  m_hwnd = hwnd;
  if (::SetWindowSubclass(m_hwnd, MainProc, SubclassID, (DWORD_PTR)this)) {
    return true;
  }
  m_hwnd = nullptr;
  return false;
}

HWND Window::Detach() {
  if (!m_hwnd || !::RemoveWindowSubclass(m_hwnd, MainProc, SubclassID)) {
    return nullptr;
  }
  HWND hwnd = m_hwnd;
  m_hwnd = nullptr;
  return hwnd;
}

HWND Window::Create(WNDCLASSEX wc, HWND parent, DWORD dwStyle, DWORD dwStyleEx, INT x, INT y, INT w, INT h) {
  ASSERT(!m_hwnd);
  wc.lpfnWndProc = &Window::StartupProc;
  ::RegisterClassEx(&wc);
  m_flags = 0;
  HWND hwnd = ::CreateWindowEx(dwStyleEx, wc.lpszClassName, nullptr, dwStyle, x, y, w, h, parent, nullptr, wc.hInstance, this);
  if (!hwnd) {
    return nullptr;
  }
  ASSERT(hwnd == m_hwnd);
  return hwnd;
}

void Window::Dispose() {
  if (m_hwnd && !(m_flags & FLAG_DYING)) {
    ::DestroyWindow(m_hwnd);
  }
}

void Window::Close() {
  ASSERT(::IsWindow(m_hwnd));
  ::PostMessage(m_hwnd, WM_CLOSE, 0, 0);
}

bool Window::Show(INT sw) {
  ASSERT(::IsWindow(m_hwnd));
  return 0 != ::ShowWindow(m_hwnd, sw);
}

void Window::Move(INT x, INT y, INT w, INT h) {
  ASSERT(::IsWindow(m_hwnd));
  ::MoveWindow(m_hwnd, x, y, w, h, true);
}

void Window::MoveToCenter(HWND hwnd) {
  CWindow self(m_hwnd);
  self.CenterWindow(hwnd);
}

void Window::Resize(INT w, INT h) {
  ASSERT(::IsWindow(m_hwnd));
  ::SetWindowPos(m_hwnd, nullptr, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

LRESULT Window::OnMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_UPDATEUISTATE:
      if (m_flags & FLAG_LAYOUT) {
        m_flags &= ~FLAG_LAYOUT;
        OnLayout();
      }
      break;
    case WM_SHOWWINDOW:
      if (lParam == 0) {  // by ShowWindow()
                          // Raise("onVisible", newtuple(wParam != 0));
      }
      if (Window* parent = Window::FromHandle(::GetParent(m_hwnd))) {
        parent->m_flags |= FLAG_LAYOUT;
      }
      break;
    case WM_SIZE:
      m_flags |= FLAG_LAYOUT;
      break;
    case WM_PARENTNOTIFY:
      switch (LOWORD(wParam)) {
        case WM_CREATE:
        case WM_DESTROY:
          if (HWND hwnd = (HWND)lParam) {
            if (::GetParent(hwnd) == m_hwnd && HasStyle(hwnd, WS_CHILD)) {
              m_flags |= FLAG_LAYOUT;
            }
          }
          break;
      }
      break;
    case WM_CLOSE:
      if (!OnClose()) {
        return false;
      }
      break;
    case WM_COMMAND:
      OnCommand(LOWORD(wParam), (HWND)lParam);
      return 0;
    case WM_CREATE:
      if (::DefSubclassProc(m_hwnd, msg, wParam, lParam) != 0) {
        return 1;
      }
      return OnCreate() ? 0 : 1;
    case WM_DESTROY:
      if (!(m_flags & FLAG_DYING)) {
        m_flags |= FLAG_DYING;
        OnDispose();
      }
      break;
  }
  return ::DefSubclassProc(m_hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK Window::MainProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data) {
  ASSERT(id == SubclassID);

  Window* self = (Window*)data;
  ASSERT(self);
  ASSERT(self->m_hwnd == hwnd);

  LRESULT lResult = self->OnMessage(msg, wParam, lParam);

  if (msg == WM_NCDESTROY) {
    self->Detach();
  }

  return lResult;
}

LRESULT CALLBACK Window::StartupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg == WM_NCCREATE) {
    CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
    Window* self = (Window*)cs->lpCreateParams;
    self->Attach(hwnd);
  }
  return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

int Window::Broadcast(UINT msg, WPARAM wParam, LPARAM lParam) {
  struct Param {
    UINT msg;
    WPARAM wParam;
    LPARAM lParam;
    INT count;

    static BOOL CALLBACK Proc(HWND hwnd, LPARAM lParam) {
      Param* m = (Param*)lParam;
      ::PostMessage(hwnd, m->msg, m->wParam, m->lParam);
      m->count++;
      return TRUE;
    }
  };

  Param m = {msg, wParam, lParam, 0};
  ::EnumThreadWindows(::GetCurrentThreadId(), &Param::Proc, (LPARAM)&m);
  return m.count;
}

/// ユーザ入力系のメッセージを転送する.
bool Window::Filter(MSG& msg) {
  if (::IsWindowEnabled(msg.hwnd) && Thread::IsLocalLoop()) {
    // if (ProcessMouseMessage(msg))
    //   return true;
    if (Window* window = Window::FromHandleParent(msg.hwnd)) {
      return window->OnFilter(msg);
    }
  }
  return false;
}

}  // namespace avesta
