// WindowImpl.h
#pragma once

#include "../server/resource.h"
#include "widgets.hpp"
#include "widgets.client.hpp"

#include "WindowMessage.h"
#include "WindowExtension.h"
#include "thread.hpp"

namespace ATL {}
using namespace ATL;
namespace WTL {}
using namespace WTL;

#pragma warning(disable : 4065)

const UINT WS_CONTROL = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
const UINT CCS_NOLAYOUT = CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NOMOVEY;

namespace mew {
namespace ui {

template <class TFinal, class TClient = CWindowEx, class TWinTraits = ATL::CControlWinTraits>
class __declspec(novtable) CWindowImplEx : public ATL::CWindowImpl<TFinal, TClient, TWinTraits> {
 public:
  const TFinal& get_final() const throw() { return *static_cast<TFinal*>(this); }
  TFinal& get_final() throw() { return *static_cast<TFinal*>(this); }
  __declspec(property(get = get_final)) TFinal final;

  DECLARE_EMPTY_MSG_MAP()

#ifdef _DEBUG
  PCTSTR GetFinalClassName() const {
    if (TFinal::GetWndClassInfo().m_wc.lpszClassName)
      return TFinal::GetWndClassInfo().m_wc.lpszClassName;
    else
      return _T("(no window class name)");
  }
#endif
};

struct QueryGestureStruct {
  IGesture** ppGesture;
  Point ptScreen;
  size_t length;
  const Gesture* gesture;
};

inline bool QueryDropTargetInWindow(HWND hWnd, IDropTarget** ppDropTarget, POINT ptScreen) {
  return SendMessage(hWnd, MEW_QUERY_DROP, (WPARAM)ppDropTarget, MAKELPARAM(ptScreen.x, ptScreen.y)) != 0;
}

bool ResizeToDefault(IWindow* window);

class SuppressRedraw {
 private:
  CWindowEx m_window;

 public:
  SuppressRedraw(HWND hWnd) : m_window(hWnd) { m_window.SetRedraw(false); }
  ~SuppressRedraw() {
    m_window.SetRedraw(true);
    m_window.Invalidate();
  }
};

//==============================================================================

class WindowImplBase : public CWindowEx {
 protected:  // variables
  Direction m_Dock;
  Extension m_Extensions;

 public:
  WindowImplBase() {}
  WindowImplBase(HWND hWnd) : CWindowEx(hWnd) {}

 public:  // method_thunk
  HRESULT Send(IWindow* self, const message& msg) const;
  void set_Dock(Direction value);

 public:  // additinal operation
  void UpdateParent() {
    ref<IWindow> parent;
    if SUCCEEDED (QueryParent(m_hWnd, &parent)) {
      parent->Update();
    }
  }

 protected:
  LRESULT OnForwardMsg(IWindow* self, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCopyData(IWindow* self, UINT, WPARAM wParam, LPARAM lParam, BOOL&);
};

//==============================================================================
// CWindowEx msg source

template <class TBase>
class WindowMessageSource : public SignalImpl<DynamicLife<TBase> > {
 public:
  bool SupportsEvent(EventCode code) const throw() {
    switch (code) {
      case EventDispose:
      case EventPreClose:
      case EventClose:
      case EventRename:
      case EventResize:
      case EventResizeDefault:
      case EventData:
      case EventUnsupported:
        return true;
      default:
        return false;
    }
  }

 protected:
  LRESULT OnQueryInterface(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (!m_msgr) {  // �j�����A���������Ƃ��ł�Release()�ς݂Ȃ̂ŁA��΂�AddRef()���Ă͂Ȃ�Ȃ�
      return E_UNEXPECTED;
    }
    const IID* piid = (const IID*)wParam;
    void** ppObject = (void**)lParam;
#ifdef _DEBUG
    if (IsBadReadPtr(piid, sizeof(GUID)) ||
        IsBadWritePtr(ppObject, sizeof(void**))) {  // �Ԉ���ČĂ΂��\��������̂ŁA�h��͊����Ɂc
      __debugbreak();
      return E_POINTER;
    }
#endif
    return (LRESULT)QueryInterface(*piid, ppObject);
  }
  LRESULT OnCopyDataEcho(UINT, WPARAM, LPARAM lParam, BOOL&) {
    string data;
    data.attach((IString*)lParam);
    if (data) {
      try {
        InvokeEvent<EventData>(static_cast<IWindow*>(this), data);
        ::PostThreadMessage(::GetCurrentThreadId(), WM_UPDATEUISTATE, 0, 0);
      } catch (Error&) {
      }
    }
    return 0;
  }
};

//==============================================================================

template <class TAtlWindow, class TImplements, class TMixin = mixin<WindowMessageSource> >
class __declspec(novtable) WindowImpl : public Root<TImplements, TMixin>, public TAtlWindow {
  using super = TAtlWindow;

 private:
  HWND Create(HWND hParent);

 public:  // overridable
  /// �R���X�g���N�^�̒���ɌĂ΂�A�E�B���h�E���쐬����.
  void DoCreate(CWindowEx parent, ATL::_U_RECT rect = rcDefault, Direction dock = DirNone, DWORD dwStyle = 0,
                DWORD dwExStyle = 0) {
    m_Dock = dock;
    __super::Create(parent, rect, NULL, dwStyle, dwExStyle);
  }
  /// �E�B���h�E�̍쐬����ɌĂ΂��.
  bool HandleCreate(const CREATESTRUCT& cs) { return true; }
  /// �E�B���h�E�̔j�����O�ɌĂ΂��.
  void HandleClose() {}
  /// �E�B���h�E�̔j������ɌĂ΂��.
  void HandleDestroy() {}
  /// ���O���ύX���ꂽ
  void HandleSetText(PCTSTR name) {}
  ///
  void HandleUpdateLayout() {}
  ///
  bool HandleQueryGesture(IGesture** pp, Point ptScreen, size_t length, const Gesture gesture[]) {
    return m_Extensions.ProcessQueryGesture(pp);
  }
  ///
  bool HandleQueryDrop(IDropTarget** pp, LPARAM lParam) { return m_Extensions.ProcessQueryDrop(pp); }

 public:  // Object
  WindowImpl() { DEBUG_ONLY(TRACE(_T("$1()"), GetFinalClassName())); }
  ~WindowImpl() { DEBUG_ONLY(TRACE(_T("~$1()"), GetFinalClassName())); }
  void __init__(IUnknown* arg) {
    HWND hWndParent = null;
    if (ref<IWindow> window = cast(arg)) {
      hWndParent = window->Handle;
    }
    if (!::IsWindow(hWndParent) || ::GetWindowThreadProcessId(hWndParent, null) != ::GetCurrentThreadId()) {
      throw ArgumentError(string::load(IDS_ERR_INVALIDPARENT), E_INVALIDARG);
    }
    //
    final.DoCreate(hWndParent);
    if (!m_hWnd) throw RuntimeError(string(IDS_ERR_CREATEWINDOW), AtlHresultFromLastError());
  }
  void Dispose() throw() {
    if (m_msgr && IsWindow()) DestroyWindow();
    m_msgr.dispose();
  }
  HRESULT Connect(EventCode code, function fn, message msg = null) throw() {
    if (final.SupportsEvent(code)) {
      return __super::Connect(code, fn, msg);
    } else {
      ASSERT(!"Unsupported event code");
      return E_INVALIDARG;
    }
  }
  HWND Recreate(_U_RECT rc = 0, PCTSTR name = 0, DWORD dwStyle = 0, DWORD dwStyleEx = 0) {
    TRACE(_T("warning: TODO �n���h���n���S�ł��܂�"));
    CWindowEx parent = GetParent();
    DestroyWindow();
    return __super::Create(parent, rc, name, dwStyle, dwStyleEx);
  }

 public:  // IWindow
  HWND get_Handle() { return m_hWnd; }

  Size get_DefaultSize() { return Size::Zero; }
  Direction get_Dock() { return m_Dock; }
  void set_Dock(Direction value) { super::set_Dock(value); }

  HRESULT Send(message msg) {
    if SUCCEEDED (super::Send(this, msg)) return S_OK;
    InvokeEvent<EventUnsupported>(static_cast<IWindow*>(this), msg);
    return S_OK;
  }

  void Close(bool sync = false) {
    if (!m_msgr) return;
    if (sync) {
      if (ProcessClose()) Dispose();
    } else {
      ::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
    }
  }
  HRESULT GetExtension(REFGUID which, REFINTF what) { return m_Extensions.Get(which, what); }
  HRESULT SetExtension(REFGUID which, IUnknown* what) { return m_Extensions.Set(which, what); }
  void Update(bool sync = false) {
    if (sync) {
      ::RedrawWindow(m_hWnd, null, null, RDW_INVALIDATE | RDW_UPDATENOW);
      final.HandleUpdateLayout();
    } else {
      PostMessage(MEW_ECHO_UPDATE);
      ::InvalidateRect(m_hWnd, null, false);
    }
  }

 public:
  BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID = 0) {
    switch (dwMsgMapID) {
      case 0:
        switch (uMsg) {
          case WM_NCCREATE:
            m_msgr.create(__uuidof(Messenger));
            this->AddRef();  // HWND �Ƃ��ĕێ�����镪.
            lResult = DefWindowProc();
            break;
          case WM_CREATE:
            lResult = DefWindowProc();
            if (!final.HandleCreate(*(LPCREATESTRUCT)lParam)) lResult = -1;
            break;
          default:
            break;
        }
    }
    BOOL bHandled = HandleWindowMessage(hWnd, uMsg, wParam, lParam, lResult, dwMsgMapID);
    if (bHandled) return true;
    switch (uMsg) {
      case WM_NCCREATE:
      case WM_CREATE:
        return true;
      default:
        return false;
    }
  }

  virtual BEGIN_MSG_MAP_(HandleWindowMessage) CHAIN_MSG_MAP_TO(super::ProcessWindowMessage)
      // self
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy) MESSAGE_HANDLER(WM_SHOWWINDOW, OnShowWindow)
          MESSAGE_HANDLER(MEW_ECHO_UPDATE, OnUpdateLayout)
      // base window
      MESSAGE_HANDLER(WM_FORWARDMSG, OnForwardMsg) MESSAGE_HANDLER(WM_COPYDATA, OnCopyData)
          MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
      // msg source
      MESSAGE_HANDLER(MEW_ECHO_COPYDATA, OnCopyDataEcho) MESSAGE_HANDLER(WM_CLOSE, OnClose)
          MESSAGE_HANDLER(WM_ENDSESSION, OnEndSession) MESSAGE_HANDLER(WM_SETTEXT, OnSetText) MESSAGE_HANDLER(WM_SIZE, OnSize)
      // query
      MESSAGE_HANDLER(MEW_QUERY_INTERFACE, OnQueryInterface) MESSAGE_HANDLER(MEW_QUERY_GESTURE, OnQueryGesture)
          MESSAGE_HANDLER(MEW_QUERY_DROP, OnQueryDrop)
      //
      DEFAULT_REFLECTION_HANDLER() REFLECT_NOTIFICATIONS() END_MSG_MAP()

          void OnFinalMessage(HWND) {
    Dispose();
    this->Release();  // HWND �Ƃ��ĕێ�����镪.
  }
  bool ProcessClose() {
    message reply = InvokeEvent<EventPreClose>(static_cast<IWindow*>(this));
    bool cancel = (bool)reply["cancel"];
    if (!cancel) {
      InvokeEvent<EventClose>(static_cast<IWindow*>(this));
      final.HandleClose();
    }
    return !cancel;
  }
  LRESULT OnClose(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
    if (ProcessClose()) bHandled = false;
    return 0;
  }
  LRESULT OnEndSession(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
    if (wParam) {  // the session is being ended
      InvokeEvent<EventClose>(static_cast<IWindow*>(this));
      final.HandleClose();
    }
    bHandled = false;
    return 0;
  }
  LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled) {
    if (m_msgr) {
      ref<IMessenger> tmp = m_msgr;
      m_msgr.clear();
      if (function fn = tmp->Invoke(EventDispose)) {
        message args;
        Event<EventDispose>::event(args, static_cast<IWindow*>(this));
        fn(args);
      }
      final.HandleDestroy();
      tmp->Dispose();
    }
    m_Extensions.Dispose();
    bHandled = false;
    return 0;
  }
  LRESULT OnShowWindow(UINT, WPARAM wParam, LPARAM lParam, BOOL&) {
    if (lParam == 0) {
      if (wParam) final.HandleUpdateLayout();
      UpdateParent();
    }
    return 0;
  }
  LRESULT OnSetText(UINT, WPARAM, LPARAM lParam, BOOL& bHandled) {
    PCTSTR name = (PCTSTR)lParam;
    final.HandleSetText(name);
    InvokeEvent<EventRename>(static_cast<IWindow*>(this), name);
    bHandled = false;
    return 0;
  }
  LRESULT OnSize(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    switch (wParam) {
      case SIZE_MAXHIDE:
      case SIZE_MAXSHOW:
        break;
      case SIZE_MINIMIZED:
      case SIZE_MAXIMIZED:
      case SIZE_RESTORED:
        InvokeEvent<EventResize>(static_cast<IWindow*>(this), Size(GET_XY_LPARAM(lParam)));
        PostMessage(MEW_ECHO_UPDATE);
        break;
    }
    bHandled = false;
    return 0;
  }
  LRESULT OnUpdateLayout(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    MSG msg;
    if (!::PeekMessage(&msg, m_hWnd, MEW_ECHO_UPDATE, MEW_ECHO_UPDATE,
                       PM_NOREMOVE)) {  // ���b�Z�[�W�L���[�ɂ���ȏ�MEW_ECHO_UPDATE�������ꍇ�̂ݏ�������B
      // �����񃌃C�A�E�g�X�V���ꂽ�ꍇ�ɁA���ʂɍĔz�u����̂�h��.
      final.HandleUpdateLayout();
    } else if (msg.hwnd != m_hWnd) {  // �����̎q���E�B���h�E�̃��b�Z�[�W�������Ă��܂��̂ŁA���ʏ���
      PostMessage(MEW_ECHO_UPDATE);  // �Ō�ɂ�����x������ʒm���ė~����
    }
    return 0;
  }
  LRESULT OnQueryGesture(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    QueryGestureStruct* param = (QueryGestureStruct*)lParam;
    return final.HandleQueryGesture(param->ppGesture, param->ptScreen, param->length, param->gesture);
  }
  LRESULT OnQueryDrop(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    return final.HandleQueryDrop((IDropTarget**)wParam, lParam);
  }
  LRESULT OnSettingChange(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    Update();
    bHandled = false;
    return 0;
  }
  LRESULT OnForwardMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    return __super::OnForwardMsg(this, uMsg, wParam, lParam, bHandled);
  }
  LRESULT OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    return __super::OnCopyData(this, uMsg, wParam, lParam, bHandled);
  }
};

//==============================================================================

}  // namespace ui
}  // namespace mew
