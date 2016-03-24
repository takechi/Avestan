// Display.cpp

#include "stdafx.h"
#include "../private.h"
#include "std/list.hpp"
#include "widgets.hpp"
#include "widgets.client.hpp"
#include "thread.hpp"

using namespace mew::ui;

//==============================================================================

template <> struct Event<EventOtherFocus>
{
	static void event(message& msg, IDisplay* from, HWND hwnd)
	{
		msg["from"] = from;
		msg["hwnd"] = (INT_PTR)hwnd;
	}
};

namespace mew { namespace ui {

class Display : public Root< implements<IDisplay, IWindow, ISignal, IDisposable>, mixin<SignalImpl, DynamicLife> >
{
private:
	class GlobalKeymap
	{
	private:
		ref<IKeymapTable>	m_keymap;
		std::vector<ATOM>	m_hotkeys;

	private:
		static UINT ToMOD(UINT mods)
		{
			UINT ret = 0;
			if(mods & ModifierControl)	ret |= MOD_CONTROL;
			if(mods & ModifierShift)	ret |= MOD_SHIFT;
			if(mods & ModifierAlt)		ret |= MOD_ALT;
			if(mods & ModifierWindows)	ret |= MOD_WIN;
			return ret;
		}
		static UINT16 FromMOD(UINT mods)
		{
			UINT16 ret = 0;
			if(mods & MOD_CONTROL)	ret |= ModifierControl;
			if(mods & MOD_SHIFT)	ret |= ModifierShift;
			if(mods & MOD_ALT)		ret |= ModifierAlt;
			if(mods & MOD_WIN)		ret |= ModifierWindows;
			return ret;
		}

	public:
		bool ProcessKeymap(IWindow* self, LPARAM lParam)
		{
			return m_keymap && SUCCEEDED(m_keymap->OnKeyDown(self, FromMOD(LOWORD(lParam)), (UINT8)HIWORD(lParam)));
		}
		void Dispose()
		{
			if(m_keymap)
			{
//				ASSERT(m_hotkeys.size() == m_keymap->Count);
				size_t count = m_hotkeys.size();
				for(size_t i = 0; i < count; ++i)
				{
					ATOM atom = m_hotkeys[i];
					::UnregisterHotKey(NULL, (int)atom);
					::GlobalDeleteAtom(atom);
				}
			}
			m_hotkeys.clear();
			m_keymap.clear();
		}
		HRESULT Get(REFGUID which, REFINTF what) const
		{
			if(which == __uuidof(IKeymap))
			{
				return m_keymap.copyto(what);
			}
			else
			{
				return E_NOTIMPL;
			}
		}
		HRESULT Set(REFGUID which, IUnknown* what)
		{
			if(which == __uuidof(IKeymap))
			{
				Dispose();
				m_keymap = cast(what);
				if(!m_keymap)
					return E_INVALIDARG;

				size_t count = m_keymap->Count;
				TCHAR module[MAX_PATH];
				::GetModuleFileName(NULL, module, MAX_PATH);
				int len = lstrlen(module);
				for(size_t i = 0; i < count; ++i)
				{
					UINT16 mod;
					UINT8 vkey;
					ref<ICommand> command;
					m_keymap->GetBind(i, &mod, &vkey, &command);
					TCHAR name[256];
					wsprintf(name, _T("%s:%04X:%02X:%08X"), module + math::max(len-230, 0), mod, vkey, (DWORD)(ICommand*)command);
					ATOM atom = ::GlobalAddAtom(name);
					ASSERT(atom);
					VERIFY( ::RegisterHotKey(NULL, (int)atom, ToMOD(mod), vkey) );
					m_hotkeys.push_back(atom);
				}
				ASSERT(m_hotkeys.size() == m_keymap->Count);
				return S_OK;
			}
			else
			{
				return E_NOTIMPL;
			}
		}
	};

	//==============================================================================

	class MenuData
	{
	private:
		CMenuHandle			m_menu;
		std::vector<CWindowEx>	m_stack;
	public:
		MenuData(HMENU menu) : m_menu(menu)
		{
		}
		size_t GetDepth() const throw()	{ return m_stack.size(); }
		void Push(HWND hwndMenu) throw()
		{
			m_stack.push_back(hwndMenu);
		}
		void Pop(HWND hwndMenu) throw()
		{
			if(!m_stack.empty() && m_stack.back() == hwndMenu)
				m_stack.pop_back();
		}
		CWindowEx GetAt(int index) const throw()
		{
			if(m_stack.empty()) return NULL;
			size_t i = (size_t)index;
			if(i >= m_stack.size())
                return m_stack.back();
			else
				return m_stack[i];
		}
	};

	//==============================================================================

	struct HookHandler
	{
		void*		self;
		WNDPROCEX	wndproc;

		HookHandler(void* s, WNDPROCEX w) : self(s), wndproc(w) {}

		friend bool operator == (const HookHandler& lhs, const HookHandler& rhs)
		{
			return lhs.self == rhs.self && lhs.wndproc == rhs.wndproc;
		}
	};

	typedef std::list<MenuData>	MenuStack;
	typedef std::list<HookHandler>	Hooks;

public:
	static Display* theDisplay;

private:
	DWORD			m_dwThreadID;
	MenuStack		m_menuStack;
	HHOOK			m_hHookCBT;
	HHOOK			m_hHookMSG;
	CWindowEx			m_wndLastWheel;
	Hooks			m_hooks;
	GlobalKeymap	m_GlobalKeymap;
	bool			m_needsUpdateUIState;
	bool			m_quitRequested;

public: // Object
	void __init__(IUnknown* arg)
	{
		ASSERT(!theDisplay);
		theDisplay = this;
		m_dwThreadID = ::GetCurrentThreadId();
		HMODULE hModule = ::GetModuleHandle(NULL);
		m_hHookCBT = ::SetWindowsHookEx(WH_CBT       , &Display::HookCBT, hModule, m_dwThreadID);
		m_hHookMSG = ::SetWindowsHookEx(WH_GETMESSAGE, &Display::HookMSG, hModule, m_dwThreadID);
		m_needsUpdateUIState = false;
		m_quitRequested = false;
		m_msgr.create(__uuidof(Messenger));
	}
	void Dispose()
	{
		OnQuitRequested();
		m_GlobalKeymap.Dispose();
		m_msgr.dispose();
		if(m_hHookCBT)
		{
			::UnhookWindowsHookEx(m_hHookCBT);
			m_hHookCBT = NULL;
		}
		if(m_hHookMSG)
		{
			::UnhookWindowsHookEx(m_hHookMSG);
			m_hHookMSG = NULL;
		}
		theDisplay = null;
		PostQuitMessage(0);
	}

public: // ISignal
	bool SupportsEvent(EventCode code) const throw()
	{
		switch(code)
		{
		case EventOtherFocus:
			return true;
		default:
			return false;
		}
	}

public: // IDisplay
	UINT PopupMenu(HMENU hMenu, UINT tpm, int x, int y, HWND hOwner, const RECT* rcExclude)
	{
		m_menuStack.push_back(MenuData(hMenu));
		TPMPARAMS params, *pParams = NULL;
		if(rcExclude)
		{
			params.cbSize = sizeof(TPMPARAMS);
			params.rcExclude = *rcExclude;
			pParams = &params;
		}
		UINT ret = ::TrackPopupMenuEx(hMenu, tpm, x, y, hOwner, pParams);
		ASSERT(m_menuStack.back().GetDepth() == 0);
		m_menuStack.pop_back();
		return ret;
	}
	size_t GetMenuDepth() throw()
	{
		if(m_menuStack.empty())
			return 0;
		return m_menuStack.back().GetDepth();
	}
	HWND GetMenu(int index = -1) throw()
	{
		if(m_menuStack.empty())
			return 0;
		return m_menuStack.back().GetAt(index);
	}
	void RegisterMessageHook(void* self, WNDPROCEX wndproc)
	{
		ASSERT(wndproc);
		if(wndproc)
			m_hooks.push_back(HookHandler(self, wndproc));
	}
	void UnregisterMessageHook(void* self, WNDPROCEX wndproc)
	{
		m_hooks.remove(HookHandler(self, wndproc));
	}

protected:
	void OnQuitRequested()
	{
		m_hooks.clear();
		m_quitRequested = true;
	}
	void OnNeedUpdateUIState()
	{
		m_needsUpdateUIState = true;
	}

private: // CBT hook thunk.
	static LRESULT CALLBACK HookCBT(int nCode, WPARAM wParam, LPARAM lParam)
	{
		Display* self = static_cast<Display*>(ui::GetThread());
		if(!self)
			return 0;
		HHOOK hHookNext = self->m_hHookCBT;
		self->OnHookCBT(nCode, wParam, lParam);
		return CallNextHookEx(hHookNext, nCode, wParam, lParam);
	}
	void OnHookCBT(int nCode, WPARAM wParam, LPARAM lParam)
	{
		switch(nCode)
		{
		case HCBT_CREATEWND:
			if(!m_menuStack.empty() && afx::IsMenu((HWND)wParam))
				m_menuStack.back().Push((HWND)wParam);
			break;
		case HCBT_DESTROYWND:
			if(!m_menuStack.empty() && afx::IsMenu((HWND)wParam))
				m_menuStack.back().Pop((HWND)wParam);
			break;
		case HCBT_SETFOCUS:
			OnHookCBT_SetFocus((HWND)wParam);
			break;
		case HCBT_ACTIVATE:		// The system is about to activate a window.
		case HCBT_MINMAX:		// A window is about to be minimized or maximized.
		case HCBT_MOVESIZE:		// A window is about to be moved or sized.
		case HCBT_SYSCOMMAND:	// A system command is about to be carried out.
		default:
			break;
		}
	}
	void OnHookCBT_SetFocus(HWND hwnd)
	{
		if(!m_msgr || !hwnd)
			return;
		DWORD pid = ::GetCurrentProcessId();
		DWORD tid = ::GetCurrentThreadId();
		DWORD t, p;
		t = ::GetWindowThreadProcessId(hwnd, &p);
		if(p == pid && t != tid)
		{	// このプロセスが作った、メインスレッドでないウィンドウ。
			InvokeEvent<EventOtherFocus>(this, hwnd);
		}
	}

private: // MSG hook thunk.
	static LRESULT CALLBACK HookMSG(int nCode, WPARAM wParam, LPARAM lParam)
	{
		Display* self = static_cast<Display*>(ui::GetThread());
		if(!self)
			return 0;
		HHOOK hHookNext = self->m_hHookMSG;
		MSG* msg = (MSG*)lParam;
		if(nCode == HC_ACTION && wParam == PM_REMOVE)
		{
			if(self->OnHookMSG(*msg))
			{	// メッセージをハンドルしたので、メッセージ自体を無効化する必要がある。
				// 戻り値やCallNextHookEx()を呼ばないなどの方法ではなく、
				// メッセージを変更して「何もしないメッセージ」にしてしまうのが良いようだ。
				msg->message = WM_NULL;
				return 0;
			}
		}
		return CallNextHookEx(hHookNext, nCode, wParam, lParam);
	}
	bool OnHookMSG_ProcessMessage(MSG& msg)
	{
		switch(msg.message)
		{
		case WM_QUIT:
			OnQuitRequested();
			return false;
		case WM_MOUSEMOVE:
			return ForwardMessage(msg);
		case WM_KEYDOWN:     case WM_SYSKEYDOWN:
		case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDOWN: case WM_XBUTTONUP: case WM_XBUTTONDBLCLK:
			OnNeedUpdateUIState();
			return ForwardMessage(msg);
		case WM_MOUSEWHEEL:
			OnNeedUpdateUIState();
			RedirectMouseWheel(msg);
			return false;
		case WM_CHAR:
		case WM_ACTIVATEAPP:
		case WM_CLOSE:
		case WM_COMMAND:
		case WM_MENUCOMMAND:
			OnNeedUpdateUIState();
			return false;
		case WM_UPDATEUISTATE:
			if(msg.hwnd)
			{	// 後にもう一度 WM_UPDATEUISTATE されるので、今回分はキャンセルする.
				return m_needsUpdateUIState;
			}
			else
			{	// トップレベルウィンドウに WM_UPDATEUISTATE をブロードキャストする.
				afx::BroadcastMessage(WM_UPDATEUISTATE);
				return true;
			}
		//case WM_INITMENUPOPUP:
		//case WM_MENUCOMMAND:
		//case WM_MENUSELECT:
		//case WM_MEASUREITEM:
		//case WM_DRAWITEM:
		//case WM_SYSCOMMAND:
		//case WM_MENUCHAR:
		case WM_HOTKEY:
			if(m_GlobalKeymap.ProcessKeymap(this, msg.lParam))
			{
				OnNeedUpdateUIState();
				return true;
			}
			return false;
		default:
			return false;
		}
	}
	bool OnHookMSG(MSG& msg)
	{
		if(m_quitRequested)
			return false;
		// CommandBar
		switch(msg.message)
		{
		case WM_MOUSEMOVE:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
			for(Hooks::iterator i = m_hooks.begin(); i != m_hooks.end(); ++i)
			{
				i->wndproc(i->self, msg.hwnd, msg.message, msg.wParam, msg.lParam);
			}
			break;
		}
		// UpdateUIState
		bool handled = OnHookMSG_ProcessMessage(msg);
		UpdateUIStateIfNeeded();
		return handled;
	}

	bool UpdateUIStateIfNeeded()
	{
		if(m_needsUpdateUIState && HIWORD(GetQueueStatus(QS_ALLINPUT)) == 0)
		{
			m_needsUpdateUIState = false;
			PostMessage(NULL, WM_UPDATEUISTATE, 0, 0);
			return true;
		}
		return false;
	}

	/// マウスホイールをカーソル直下のウィンドウに送る.
	void RedirectMouseWheel(MSG& msg)
	{
		// WM_MOUSEWHEELのマウス位置はスクリーン座標系なので、修正する必要なし.
		POINT pt = { GET_XY_LPARAM(msg.lParam) };
		HWND hWnd = ::WindowFromPoint(pt);
		if(hWnd && ::GetWindowThreadProcessId(hWnd, NULL) == ::GetCurrentThreadId() && IsWindowEnabled(hWnd))
			m_wndLastWheel = msg.hwnd = hWnd;
		else if(m_wndLastWheel)
			msg.hwnd = m_wndLastWheel;
	}
	/// ユーザ入力系のメッセージを転送する.
	bool ForwardMessage(MSG& msg) const
	{
		if(m_menuStack.empty()
		&& IsWindowEnabled(msg.hwnd)
		&& !IsInMenuOrMoveOrSize(::GetWindowThreadProcessId(msg.hwnd, NULL)))
		{
			ref<IWindow> p;
			for(HWND hwnd = msg.hwnd; hwnd; hwnd = afx::GetParentOrOwner(hwnd))
			{
				if SUCCEEDED(QueryInterfaceInWindow(hwnd, &p))
				{
					if(!IsWindowEnabled(hwnd))
						return false;
					return 0 != ::SendMessage(hwnd, WM_FORWARDMSG, 0, (LPARAM)&msg);
				}
			}
		}
		return false;
	}
	/// メニューループまたはウィンドウの移動・リサイズ中の場合は真.
	static bool IsInMenuOrMoveOrSize(DWORD dwThreadID) throw()
	{
		GUITHREADINFO info = { sizeof(GUITHREADINFO) };
		return ::GetGUIThreadInfo(dwThreadID, &info) && (info.flags & (GUI_INMENUMODE | GUI_INMOVESIZE)) != 0;
	}

public: // IWindow
	HRESULT Send(message msg)
	{
		using namespace mew::ui;
		switch(msg.code)
		{
		case CommandClose:
			this->Close();
			break;
		default:
			TRACE(L"warning: unsupported msg : $1", msg.code);
			return E_FAIL;
		}
		return S_OK;
	}
	HWND get_Handle()			{ return ::GetDesktopWindow(); }
	Size get_DefaultSize()		{ return Size::Zero; }
	void Update(bool sync = false)
	{
		if(sync)
		{
			PostMessage(0, WM_UPDATEUISTATE, 0, 0);
			while(true)
			{
				if(!afx::PumpMessage())
					OnQuitRequested();
				if(m_quitRequested)
					break;
				if(!UpdateUIStateIfNeeded())
					WaitMessage();
			}
		}
		else
		{
			if(!afx::PumpMessage())
				OnQuitRequested();
		}
	}
	void Close(bool sync = false)
	{
		afx::BroadcastMessage(WM_CLOSE);
		OnQuitRequested();
	}
	HRESULT GetExtension(REFGUID which, REFINTF what)	{ return m_GlobalKeymap.Get(which, what); }
	HRESULT SetExtension(REFGUID which, IUnknown* what)	{ return m_GlobalKeymap.Set(which, what); }
	Direction get_Dock()	{ return DirNone; }
	void      set_Dock(Direction value)	{}
};

Display* Display::theDisplay;

} }

AVESTA_EXPORT( Display )

IDisplay* mew::ui::GetThread(DWORD dwThreadID)
{
	ASSERT(Display::theDisplay);
	// TODO: Get Display from TLS
	return Display::theDisplay;
}
