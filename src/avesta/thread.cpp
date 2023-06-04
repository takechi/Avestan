// thread.cpp

#include "stdafx.h"
#include "avesta.hpp"
#include <process.h>
#include <memory>

namespace {
const int MAX_THREADS = 32;

DWORD theMainThreadId;
DWORD theThreadStateId;
int theNumThreads = 0;
HANDLE theThreads[MAX_THREADS];

class MessageHook {
 private:
  HHOOK m_hHookMSG;
  HWND m_hwndLastWheel;
  bool m_needsUpdateUIState;

 public:
  MessageHook() : m_hHookMSG(nullptr), m_hwndLastWheel(nullptr), m_needsUpdateUIState(true) {
    ASSERT(!Get());
    DWORD dwThreadID = ::GetCurrentThreadId();
    HMODULE hModule = ::GetModuleHandle(NULL);
    ::TlsSetValue(theThreadStateId, this);
    m_hHookMSG = ::SetWindowsHookEx(WH_GETMESSAGE, &MessageHook::OnHook, hModule, dwThreadID);
  }

  ~MessageHook() {
    ASSERT(Get() == this);
    if (m_hHookMSG) {
      HHOOK tmp = m_hHookMSG;
      m_hHookMSG = NULL;
      ::UnhookWindowsHookEx(tmp);
    }
    ::TlsSetValue(theThreadStateId, nullptr);
  }

  int Loop(HWND hwnd) {
    ASSERT(::IsWindow(hwnd));
    if (!::IsWindowVisible(hwnd)) {
      ::ShowWindow(hwnd, SW_SHOW);
    }

    while (::IsWindow(hwnd)) {
      MSG msg;
      if (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
      } else if (m_needsUpdateUIState) {  // ���b�Z�[�W�L���[����ɂȂ����̂ŁAWM_UPDATEUISTATE ���|�X�g����B
        m_needsUpdateUIState = false;
        avesta::Window::Broadcast(WM_UPDATEUISTATE);
      } else {
        ::WaitMessage();
      }
    }
    return 0;
  }

  int GetMenuStackDepth() const { return 0; }

  static MessageHook* Get() { return (MessageHook*)::TlsGetValue(theThreadStateId); }

 private:
  static LRESULT CALLBACK OnHook(int nCode, WPARAM wParam, LPARAM lParam) {
    MessageHook* self = Get();
    HHOOK hHook = self->m_hHookMSG;
    MSG* msg = (MSG*)lParam;
    if (nCode == HC_ACTION && wParam == PM_REMOVE) {
      if (self->OnHook(*msg)) {  // ���b�Z�[�W���n���h�������̂ŁA���b�Z�[�W���̂𖳌�������K�v������B
        // �߂�l��CallNextHookEx()���Ă΂Ȃ��Ȃǂ̕��@�ł͂Ȃ��A
        // ���b�Z�[�W��ύX���āu�������Ȃ����b�Z�[�W�v�ɂ��Ă��܂��̂��ǂ��悤���B
        msg->message = WM_NULL;
        return 0;
      }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
  }

  bool OnHook(MSG& msg) {
    bool handled = false;

    switch (msg.message) {
      case WM_KEYDOWN:
      case WM_SYSKEYDOWN:
      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
      case WM_LBUTTONDBLCLK:
      case WM_RBUTTONDOWN:
      case WM_RBUTTONUP:
      case WM_RBUTTONDBLCLK:
      case WM_MBUTTONDOWN:
      case WM_MBUTTONUP:
      case WM_MBUTTONDBLCLK:
      case WM_XBUTTONDOWN:
      case WM_XBUTTONUP:
      case WM_XBUTTONDBLCLK:
        m_needsUpdateUIState = true;
        // fall down
      case WM_MOUSEMOVE:
        handled = avesta::Window::Filter(msg);
        break;
      case WM_MOUSEWHEEL:
        m_needsUpdateUIState = true;
        RedirectMouseWheel(msg);
        break;
      case WM_CHAR:
      case WM_ACTIVATEAPP:
      case WM_CLOSE:
      case WM_COMMAND:
      case WM_MENUCOMMAND:
        m_needsUpdateUIState = true;
        break;
    }

    if (m_needsUpdateUIState &&
        HIWORD(GetQueueStatus(QS_ALLINPUT)) == 0) {  // ���b�Z�[�W�L���[����ɂȂ����̂ŁAWM_UPDATEUISTATE ���|�X�g����B
      m_needsUpdateUIState = false;
      avesta::Window::Broadcast(WM_UPDATEUISTATE);
    }
    return handled;
  }

  /// �}�E�X�z�C�[�����J�[�\�������̃E�B���h�E�ɑ���.
  void RedirectMouseWheel(MSG& msg) {
    // WM_MOUSEWHEEL�̃}�E�X�ʒu�̓X�N���[�����W�n�Ȃ̂ŁA�C������K�v�Ȃ�.
    POINT pt = {GET_XY_LPARAM(msg.lParam)};
    HWND hwnd = ::WindowFromPoint(pt);
    if (hwnd && ::GetWindowThreadProcessId(hwnd, NULL) == ::GetCurrentThreadId() && ::IsWindowEnabled(hwnd)) {
      m_hwndLastWheel = msg.hwnd = hwnd;
    } else if (::IsWindow(m_hwndLastWheel)) {
      msg.hwnd = m_hwndLastWheel;
    } else {
      m_hwndLastWheel = nullptr;
    }
  }
};

bool RemoveThread(int at) {
  if (at >= theNumThreads) {
    return false;
  }
  ::CloseHandle(theThreads[at]);
  memmove(theThreads + at, theThreads + at + 1, sizeof(HANDLE));
  --theNumThreads;
  return true;
}

int MessageLoop() {
  while (true) {
    // ���[�J���ϐ��ɃR�s�[���邪�A��� nCount == theNumThreads
    const size_t nCount = theNumThreads;

    if (nCount == 0) {
      return 0;
    }

    DWORD ret = ::MsgWaitForMultipleObjects((DWORD)nCount, theThreads, false, INFINITE, QS_ALLINPUT);
    if (ret == WAIT_OBJECT_0 + nCount) {  // ���b�Z�[�W�ɂ��N��
      MSG msg;
      while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        switch (msg.message) {
          case WM_APP:
            avesta::Thread::New((avesta::Thread::Routine)msg.wParam, (void*)msg.lParam);
            break;
        }
      }
    } else if (WAIT_OBJECT_0 <= ret && ret < WAIT_OBJECT_0 + nCount) {  // �X���b�h�̂ǂꂩ���I������
      RemoveThread(ret - WAIT_OBJECT_0);
    } else if (WAIT_ABANDONED_0 <= ret && ret < WAIT_ABANDONED_0 + nCount) {  // �X���b�h�ǂꂩ���j���ς݁H �����ɂ͗��Ȃ������B
      RemoveThread(ret - WAIT_ABANDONED_0);
    } else {  // �G���[�B���ɃX���b�h���I�����Ă���\��������B
      // �����̃E�B���h�E�𓯎��ɕ����ꍇ�ɔ������₷���B
      for (int i = theNumThreads - 1; i >= 0; --i) {
        DWORD code;
        if (!::GetExitCodeThread(theThreads[i], &code) || code != STILL_ACTIVE) {
          RemoveThread(i);
        }
      }
    }
  }
}
}  // namespace

namespace avesta {

HANDLE Thread::New(Routine fn, void* args) {
  ASSERT(fn);
  if (::GetCurrentThreadId() == theMainThreadId) {
    if (theNumThreads >= MAX_THREADS) {
      return nullptr;
    }
    unsigned id;
    HANDLE handle = (HANDLE)_beginthreadex(nullptr, 0, fn, args, 0, &id);
    if (!handle) {
      return nullptr;
    }
    theThreads[theNumThreads++] = handle;
    return handle;
  } else {
    // XXX: theThreads �� CriticalSection �Ŏ��΁A�}�[�V�������O����K�v�͖��������H
    ::PostThreadMessage(theMainThreadId, WM_APP, (WPARAM)fn, (LPARAM)args);
    return ::GetCurrentThread();
  }
}

int Thread::Loop(HWND hwnd) { return std::auto_ptr<MessageHook>(new MessageHook())->Loop(hwnd); }

int Thread::Run(Routine fn, void* args) {
  ASSERT(!theMainThreadId);

  theMainThreadId = ::GetCurrentThreadId();
  theThreadStateId = ::TlsAlloc();

  New(fn, args);
  int ret = MessageLoop();

  ::TlsFree(theThreadStateId);

  return ret;
}

/// ���j���[���[�v�܂��̓E�B���h�E�̈ړ��E���T�C�Y���̏ꍇ�͐^.
bool Thread::IsLocalLoop() {
  if (MessageHook::Get()->GetMenuStackDepth() > 0) return true;
  GUITHREADINFO info = {sizeof(GUITHREADINFO)};
  return ::GetGUIThreadInfo(::GetCurrentThreadId(), &info) && (info.flags & (GUI_INMENUMODE | GUI_INMOVESIZE)) != 0;
}

}  // namespace avesta