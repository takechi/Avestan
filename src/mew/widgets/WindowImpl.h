// WindowImpl.h
#pragma once

#include "../server/resource.h"
#include "widgets.hpp"
#include "widgets.client.hpp"

#include "WindowMessage.h"
#include "WindowExtension.h"
#include "thread.hpp"

#pragma warning(disable : 4065)

const UINT WS_CONTROL = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
const UINT CCS_NOLAYOUT = CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NOMOVEY;

namespace mew {
namespace ui {

template <class TFinal, class TClient = CWindowEx, class TWinTraits = ATL::CControlWinTraits>
class __declspec(novtable) CWindowImplEx : public ATL::CWindowImpl<TFinal, TClient, TWinTraits> {
 public:
  const TFinal& get_final() const noexcept { return *static_cast<TFinal*>(this); }
  TFinal& get_final() noexcept { return *static_cast<TFinal*>(this); }
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
  bool SupportsEvent(EventCode code) const noexcept {
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
    if (!this->m_msgr) {  // 破棄中、下手をするとすでにRelease()済みなので、絶対にAddRef()してはならない
      return E_UNEXPECTED;
    }
    const IID* piid = (const IID*)wParam;
    void** ppObject = (void**)lParam;
#ifdef _DEBUG
    if (IsBadReadPtr(piid, sizeof(GUID)) ||
        IsBadWritePtr(ppObject, sizeof(void**))) {  // 間違って呼ばれる可能性があるので、防御は完璧に…
      __debugbreak();
      return E_POINTER;
    }
#endif
    return (LRESULT)(this->QueryInterface(*piid, ppObject));
  }
  LRESULT OnCopyDataEcho(UINT, WPARAM, LPARAM lParam, BOOL&) {
    string data;
    data.attach((IString*)lParam);
    if (data) {
      try {
        this->InvokeEvent<EventData>(static_cast<IWindow*>(this), data);
        ::PostThreadMessage(::GetCurrentThreadId(), WM_UPDATEUISTATE, 0, 0);
      } catch (mew::exceptions::Error&) {
      }
    }
    return 0;
  }
};

//==============================================================================

template <class TAtlWindow, class TImplements, class TMixin = mixin<WindowMessageSource> >
class __declspec(novtable) WindowImpl : public Root<TImplements, TMixin>, public TAtlWindow {
  using super = TAtlWindow;

 public:  // overridable
  /// コンストラクタの直後に呼ばれ、ウィンドウを作成する.
  void DoCreate(CWindowEx parent, ATL::_U_RECT rect = super::rcDefault, Direction dock = DirNone, DWORD dwStyle = 0,
                DWORD dwExStyle = 0) {
    super::m_Dock = dock;
    __super::Create(parent, rect, NULL, dwStyle, dwExStyle);
  }
  /// ウィンドウの作成直後に呼ばれる.
  bool HandleCreate(const CREATESTRUCT& cs) { return true; }
  /// ウィンドウの破棄直前に呼ばれる.
  void HandleClose() {}
  /// ウィンドウの破棄直後に呼ばれる.
  void HandleDestroy() {}
  /// 名前が変更された
  void HandleSetText(PCTSTR name) {}
  ///
  void HandleUpdateLayout() {}
  ///
  bool HandleQueryGesture(IGesture** pp, Point ptScreen, size_t length, const Gesture gesture[]) {
    return super::m_Extensions.ProcessQueryGesture(pp);
  }
  ///
  bool HandleQueryDrop(IDropTarget** pp, LPARAM lParam) { return super::m_Extensions.ProcessQueryDrop(pp); }

 public:  // Object
  using super::DefWindowProc;
  using super::DestroyWindow;
  using super::GetParent;
  using super::IsWindow;
  using super::PostMessage;
  WindowImpl() { DEBUG_ONLY(TRACE(_T("$1()"), GetFinalClassName())); }
  ~WindowImpl() { DEBUG_ONLY(TRACE(_T("~$1()"), GetFinalClassName())); }
  void __init__(IUnknown* arg) {
    HWND hWndParent = null;
    if (ref<IWindow> window = cast(arg)) {
      hWndParent = window->Handle;
    }
    if (!::IsWindow(hWndParent) || ::GetWindowThreadProcessId(hWndParent, null) != ::GetCurrentThreadId()) {
      throw mew::exceptions::ArgumentError(string::load(IDS_ERR_INVALIDPARENT), E_INVALIDARG);
    }
    //
    super::final.DoCreate(hWndParent);
    if (!super::m_hWnd) {
      throw mew::exceptions::RuntimeError(string(IDS_ERR_CREATEWINDOW), AtlHresultFromLastError());
    }
  }
  void Dispose() noexcept {
    if (this->m_msgr && IsWindow()) {
      DestroyWindow();
    }
    this->m_msgr.dispose();
  }
  HRESULT Connect(EventCode code, function fn, message msg = null) noexcept {
    if (super::final.SupportsEvent(code)) {
      return __super::Connect(code, fn, msg);
    } else {
      ASSERT(!"Unsupported event code");
      return E_INVALIDARG;
    }
  }
  HWND Recreate(_U_RECT rc = 0, PCTSTR name = 0, DWORD dwStyle = 0, DWORD dwStyleEx = 0) {
    TRACE(_T("warning: TODO ハンドラ系が全滅します"));
    CWindowEx parent = (HWND)GetParent();
    DestroyWindow();
    return __super::Create(parent, rc, name, dwStyle, dwStyleEx);
  }

 public:  // IWindow
  HWND get_Handle() { return super::m_hWnd; }

  Size get_DefaultSize() { return Size::Zero; }
  Direction get_Dock() { return super::m_Dock; }
  void set_Dock(Direction value) { super::set_Dock(value); }

  HRESULT Send(message msg) {
    if SUCCEEDED (super::Send(this, msg)) return S_OK;
    this->InvokeEvent<EventUnsupported>(static_cast<IWindow*>(this), msg);
    return S_OK;
  }

  void Close(bool sync = false) {
    if (!this->m_msgr) return;
    if (sync) {
      if (ProcessClose()) Dispose();
    } else {
      ::PostMessage(super::m_hWnd, WM_CLOSE, 0, 0);
    }
  }
  HRESULT GetExtension(REFGUID which, REFINTF what) { return super::m_Extensions.Get(which, what); }
  HRESULT SetExtension(REFGUID which, IUnknown* what) { return super::m_Extensions.Set(which, what); }
  void Update(bool sync = false) {
    if (sync) {
      ::RedrawWindow(super::m_hWnd, null, null, RDW_INVALIDATE | RDW_UPDATENOW);
      super::final.HandleUpdateLayout();
    } else {
      super::PostMessage(MEW_ECHO_UPDATE);
      ::InvalidateRect(super::m_hWnd, null, false);
    }
  }

 public:
  BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID = 0) {
    switch (dwMsgMapID) {
      case 0:
        switch (uMsg) {
          case WM_NCCREATE:
            this->m_msgr.create(__uuidof(Messenger));
            this->AddRef();  // HWND として保持される分.
            lResult = DefWindowProc();
            break;
          case WM_CREATE:
            lResult = DefWindowProc();
            if (!super::final.HandleCreate(*(LPCREATESTRUCT)lParam)) lResult = -1;
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
      MESSAGE_HANDLER(MEW_ECHO_COPYDATA, this->OnCopyDataEcho) MESSAGE_HANDLER(WM_CLOSE, OnClose)
          MESSAGE_HANDLER(WM_ENDSESSION, OnEndSession) MESSAGE_HANDLER(WM_SETTEXT, OnSetText) MESSAGE_HANDLER(WM_SIZE, OnSize)
      // query
      MESSAGE_HANDLER(MEW_QUERY_INTERFACE, this->OnQueryInterface) MESSAGE_HANDLER(MEW_QUERY_GESTURE, OnQueryGesture)
          MESSAGE_HANDLER(MEW_QUERY_DROP, OnQueryDrop)
      //
      DEFAULT_REFLECTION_HANDLER() REFLECT_NOTIFICATIONS() END_MSG_MAP()

          void OnFinalMessage(HWND) {
    Dispose();
    this->Release();  // HWND として保持される分.
  }
  bool ProcessClose() {
    message reply = this->InvokeEvent<EventPreClose>(static_cast<IWindow*>(this));
    bool cancel = (bool)reply["cancel"];
    if (!cancel) {
      this->InvokeEvent<EventClose>(static_cast<IWindow*>(this));
      super::final.HandleClose();
    }
    return !cancel;
  }
  LRESULT OnClose(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
    if (ProcessClose()) bHandled = false;
    return 0;
  }
  LRESULT OnEndSession(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
    if (wParam) {  // the session is being ended
      this->InvokeEvent<EventClose>(static_cast<IWindow*>(this));
      super::final.HandleClose();
    }
    bHandled = false;
    return 0;
  }
  LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled) {
    if (this->m_msgr) {
      ref<IMessenger> tmp = this->m_msgr;
      this->m_msgr.clear();
      if (function fn = tmp->Invoke(EventDispose)) {
        message args;
        Event<EventDispose>::event(args, static_cast<IWindow*>(this));
        fn(args);
      }
      super::final.HandleDestroy();
      tmp->Dispose();
    }
    this->m_Extensions.Dispose();
    bHandled = false;
    return 0;
  }
  LRESULT OnShowWindow(UINT, WPARAM wParam, LPARAM lParam, BOOL&) {
    if (lParam == 0) {
      if (wParam) super::final.HandleUpdateLayout();
      this->UpdateParent();
    }
    return 0;
  }
  LRESULT OnSetText(UINT, WPARAM, LPARAM lParam, BOOL& bHandled) {
    PCTSTR name = (PCTSTR)lParam;
    super::final.HandleSetText(name);
    this->InvokeEvent<EventRename>(static_cast<IWindow*>(this), name);
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
        this->InvokeEvent<EventResize>(static_cast<IWindow*>(this), Size(GET_XY_LPARAM(lParam)));
        PostMessage(MEW_ECHO_UPDATE);
        break;
    }
    bHandled = false;
    return 0;
  }
  LRESULT OnUpdateLayout(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    MSG msg;
    if (!::PeekMessage(&msg, super::m_hWnd, MEW_ECHO_UPDATE, MEW_ECHO_UPDATE,
                       PM_NOREMOVE)) {  // メッセージキューにこれ以上MEW_ECHO_UPDATEが無い場合のみ処理する。
      // 複数回レイアウト更新された場合に、無駄に再配置するのを防ぐ.
      super::final.HandleUpdateLayout();
    } else if (msg.hwnd != super::m_hWnd) {  // 自分の子供ウィンドウのメッセージも見つけてしまうので、特別処理
      PostMessage(MEW_ECHO_UPDATE);          // 最後にもう一度自分を通知して欲しい
    }
    return 0;
  }
  LRESULT OnQueryGesture(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    QueryGestureStruct* param = (QueryGestureStruct*)lParam;
    return super::final.HandleQueryGesture(param->ppGesture, param->ptScreen, param->length, param->gesture);
  }
  LRESULT OnQueryDrop(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    return super::final.HandleQueryDrop((IDropTarget**)wParam, lParam);
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
