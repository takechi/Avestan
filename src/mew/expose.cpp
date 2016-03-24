// expose.cpp

#include "stdafx.h"
#include "private.h"
#include "widgets.hpp"
#include "widgets.client.hpp"
#include "drawing.hpp"
#include "std/vector.hpp"

using namespace mew::ui;
using namespace mew::drawing;

//==============================================================================

namespace mew { namespace ui {

class Expose : public Root< implements<IExpose> >, public CWindowImpl<Expose, CWindowEx>
{
private:
	CBitmap	m_bmpActive;
	CBitmap	m_bmpInactive;
	CBitmap	m_bmpBack;
	struct Component
	{
		INT32	id;
		INT32	group;
		Rect	bounds;
		UINT8	hotkey;
	};
	typedef std::vector<Component> Components;
	Components	m_Components;
	CWindowEx	m_wndParent;
	DWORD	m_Alpha;
	INT32	m_Selected;
	string	m_Title;
	Point	m_LastCursorPos; // 初期マウスカーソル位置。
	enum Status
	{
		StatusLoop,
		StatusOK,
		StatusCancel,
	};
	Status	m_Status;

public:
	void __init__(IUnknown* arg)
	{
		HWND hWndParent = null;
		if(ref<IWindow> window = cast(arg))
		{
			hWndParent = window->Handle;
		}
		m_Selected = -1;
	}

private:
	void ReCreateBitmap(CWindowEx parent, RECT* prcWindow)
	{
		if(m_bmpActive)		m_bmpActive.DeleteObject();
		if(m_bmpInactive)	m_bmpInactive.DeleteObject();
		if(m_bmpBack)		m_bmpBack.DeleteObject();

		Rect rcClient;
		parent.GetWindowRect(prcWindow);
		parent.GetClientRect(&rcClient);
		const int w = rcClient.w, h = rcClient.h;

		CDCHandle dcDevice = ::GetDC(null);
		// create dc and bmp
		CDC dc1, dc2;
		dc1.CreateCompatibleDC(dcDevice);
		dc2.CreateCompatibleDC(dcDevice);
		m_bmpActive.CreateCompatibleBitmap(dcDevice, w, h);
		m_bmpInactive.CreateCompatibleBitmap(dcDevice, w, h);
		m_bmpBack.CreateCompatibleBitmap(dcDevice, w, h);
		// init active image
		CBitmapHandle bmp1 = dc1.SelectBitmap(m_bmpActive);
		POINT pt = { 0, 0 }, ptOrg;
		parent.ClientToScreen(&pt);
		dc1.SetWindowOrg(pt.x-prcWindow->left, pt.y-prcWindow->top, &ptOrg);
		afx::PrintWindow(parent, dc1, 0);
		dc1.SetWindowOrg(ptOrg);
		// init inactive image
		CBitmapHandle bmp2 = dc2.SelectBitmap(m_bmpInactive);
		dc2.PatBlt(0, 0, w, h, BLACKNESS);
		//HBRUSH hWhiteBrush = (HBRUSH)::GetStockObject(GRAY_BRUSH);
		//for(int i = 0; i < GetComponentCount(); ++i)
		//{
		//	Rect rc = m_Components[i].bounds;
		//	for(int j = 0; j < 2; ++j)
		//	{
		//		::InflateRect(&rc, -1, -1);
		//		dc2.FrameRect(&rc, hWhiteBrush);
		//	}
		//}
		BLENDFUNCTION blendfunc = { 0, 0, 128, 0 };
		::AlphaBlend(dc2, 0, 0, w, h, dc1, 0, 0, w, h, blendfunc);
		// cleanup
		dc2.SelectBitmap(bmp2);
		dc2.DeleteDC();
		dc1.SelectBitmap(bmp1);
		dc1.DeleteDC();
		::ReleaseDC(null, dcDevice);
	}

public: // IExpose
	void AddRect(INT32 id, INT32 group, const Rect& bounds, UINT8 hotkey)
	{
		Component c = { id, group, bounds, hotkey };
		m_Components.push_back(c);
	}
	void Select(UINT32 id)
	{
		m_Selected = id;
	}
	void SetTitle(string title)
	{
		m_Title = title;
	}
	HRESULT Go(HWND hwndParent, UINT32 dwTransitionTime)
	{
		ASSERT(!IsWindow());
		if(dwTransitionTime > 1000*10)
		{
			ASSERT(!"えくすぽぜ 長すぎるんちゃうん？");
			dwTransitionTime = 1000*10;
		}

		if(m_Selected < 0)
			m_Selected = 0;

		m_wndParent.Attach(hwndParent);
		RECT rcWindow;
		ReCreateBitmap(m_wndParent, &rcWindow);
		DWORD dwStyle = (m_wndParent.GetStyle()) & ~(WS_MAXIMIZE | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);
		DWORD dwExStyle = m_wndParent.GetExStyle() & ~(WS_EX_LAYERED);
		if(dwTransitionTime == 0)
			dwTransitionTime = 1;
		m_Status = StatusLoop;

		m_Alpha = 0;
		Create(m_wndParent, rcWindow, m_Title.str(), dwStyle, dwExStyle);
		if(dwStyle & WS_SYSMENU)
		{
			SetIcon(m_wndParent.GetIcon(false), false);
			CMenuHandle menu = GetSystemMenu(false);
			menu.DeleteMenu(SC_SIZE    , MF_BYCOMMAND);
			menu.DeleteMenu(SC_RESTORE , MF_BYCOMMAND);
			menu.DeleteMenu(SC_MAXIMIZE, MF_BYCOMMAND);
			menu.DeleteMenu(SC_MINIMIZE, MF_BYCOMMAND);
		}
		ImmAssociateContext(m_hWnd, null);
		SetFocus();
		::GetCursorPos(&m_LastCursorPos);
		ScreenToClient(&m_LastCursorPos);
		// semi-modal-window
		DWORD dwStartTime = GetTickCount();
		m_wndParent.EnableWindow(false);
		while(true)
		{
			MSG msg;

			while(PeekMessage(&msg, null, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
			if(m_Status != StatusLoop || !IsWindow())
				break;
			// fade-in
			if(m_Alpha != 255)
			{
				DWORD dwTime = GetTickCount();
				DWORD alpha = (dwTime - dwStartTime) * 255 / dwTransitionTime;
				if(m_Alpha != alpha)
				{
					m_Alpha = math::min<DWORD>(alpha, 255);
					Invalidate();
				}
				else
				{
					Sleep(1);
				}
			}
			else
			{
				WaitMessage();
			}
		}
		//
		if(m_Status == StatusOK)
			return GetCurrentID();
		else
			return E_FAIL;
	}
	int GetComponentCount() const	{ return (int)m_Components.size(); }
	bool IsValidIndex(int index) const
	{
		return 0 <= index && index < GetComponentCount();
	}
	int GetCurrentID() const
	{
		if(!IsValidIndex(m_Selected))
			return -1;
		else
			return m_Components[m_Selected].id;
	}
	int HitTest(int x, int y) const
	{
		for(int i = 0; i < GetComponentCount(); ++i)
		{
			if(m_Components[i].bounds.contains(x, y))
				return i;
		}
		return -1;
	}
	void ExitExpose(bool succeeded)
	{
		m_Status = succeeded ? StatusOK : StatusCancel;
		PostMessage(WM_CLOSE);
	}
	bool InClientArea(LPARAM lParam)
	{
		Rect rcClient;
		GetClientRect(&rcClient);
		return rcClient.contains(GET_XY_LPARAM(lParam));
	}
	void WalkTab(bool shift, bool control)
	{
		if(!IsValidIndex(m_Selected))
		{
			if(!m_Components.empty())
			{
				m_Selected = 0;
				Invalidate();
			}
			return;
		}
		const Component& c = m_Components[m_Selected];
		const INT32 count = GetComponentCount();
		const INT32 dir = shift ? count-1 : 1;
		if(control)
		{
			for(INT32 i = (m_Selected+dir) % count; i != m_Selected; i = (i+dir) % count)
			{
				if(m_Components[i].group != c.group)
				{
					m_Selected = i;
					Invalidate();
					return;
				}
			}
		}
		for(INT32 i = (m_Selected+dir) % count; i != m_Selected; i = (i+dir) % count)
		{
			if(m_Components[i].group == c.group)
			{
				m_Selected = i;
				Invalidate();
				return;
			}
		}
		return; // no effect
	}

	DECLARE_WND_CLASS_EX(L"mew.ui.Expose", 0, -1)

	BEGIN_MSG_MAP(Expose)
		MESSAGE_HANDLER(WM_NCCREATE		, OnNcCreate)
		MESSAGE_HANDLER(WM_CLOSE		, OnClose)
		MESSAGE_HANDLER(WM_MOVE			, OnMove)
		MESSAGE_HANDLER(WM_KEYDOWN		, OnKeyDown)
		MESSAGE_HANDLER(WM_PAINT		, OnPaint)
		MESSAGE_HANDLER(WM_PRINTCLIENT	, OnPrintClient)
		MESSAGE_HANDLER(WM_NCHITTEST	, OnNcHitTest)
		MESSAGE_HANDLER(WM_MOUSEMOVE	, OnMouseMove)
		MESSAGE_HANDLER(WM_LBUTTONUP	, OnLButtonUp)
		MESSAGE_HANDLER(WM_RBUTTONUP	, OnRButtonUp)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	LRESULT OnNcCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		AddRef(); // for HWND
		bHandled = false;
		return 0;
	}
	void OnFinalMessage(HWND)
	{
		Release(); // for HWND
	}
	LRESULT OnClose(UINT uMsg, WPARAM , LPARAM , BOOL& bHandled)
	{
		// WM_CLOSE 中の GetParent() は失敗するらしい
		if(m_wndParent.IsWindow())
			m_wndParent.EnableWindow(true);
		m_wndParent.Detach();
		bHandled = false;
		return 0;
	}
	LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		LRESULT lResult = DefWindowProc(uMsg, wParam, lParam);
		switch(lResult)
		{
		case HTCLOSE:
		case HTMAXBUTTON:
		case HTMINBUTTON:
		case HTCAPTION:
		case HTSYSMENU:
			break;
		default:
			lResult = HTCLIENT;
			break;
		}
		return lResult;
	}
	LRESULT OnMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		RECT rc;
		GetWindowRect(&rc);
		m_wndParent.SetWindowPos(NULL, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
		return 0;
	}
	LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		Point pt(GET_XY_LPARAM(lParam));
		Point v = m_LastCursorPos - pt;
		if(math::hypot2(v.x, v.y) > 100)
		{
			m_LastCursorPos.assign(-0x4000, -0x4000); // とりあえず遠くへ
			int index = HitTest(GET_XY_LPARAM(lParam));
			if(m_Selected != index)
			{
				m_Selected = index;
				Invalidate();
			}
		}
		if(!InClientArea(lParam) && 0 == (GET_KEYSTATE_WPARAM(wParam) & MouseButtonMask))
			ReleaseCapture();
		else if(GetCapture() != m_hWnd)
			SetCapture();
		return 0;
	}
	LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if(GetCapture() != m_hWnd)
			return 0;
		int index = HitTest(GET_XY_LPARAM(lParam));
		if(index >= 0)
		{
			m_Selected = index;
			ExitExpose(true);
		}
		if(!InClientArea(lParam) && 0 == (GET_KEYSTATE_WPARAM(wParam) & MouseButtonMask))
			ReleaseCapture();
		return 0;
	}
	LRESULT OnRButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if(GetCapture() != m_hWnd)
			return 0;
		if(InClientArea(lParam))
			ExitExpose(false);
		else if(0 == (GET_KEYSTATE_WPARAM(wParam) & MouseButtonMask))
			ReleaseCapture();
		return 0;
	}
	LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		// キー入力後は、マウスを不感帯を設ける。
		::GetCursorPos(&m_LastCursorPos);
		ScreenToClient(&m_LastCursorPos);
		switch(wParam)
		{
		case VK_ESCAPE:
			ExitExpose(false);
			break;
		case VK_SPACE:
		case VK_RETURN:
			ExitExpose(true);
			break;
		case VK_TAB:
			WalkTab(IsKeyPressed(VK_SHIFT), IsKeyPressed(VK_CONTROL));
			break;
		case VK_UP:
		case VK_DOWN:
		case VK_LEFT:
		case VK_RIGHT:
			if(!IsValidIndex(m_Selected))
			{
				if(!m_Components.empty())
				{
					m_Selected = 0;
					Invalidate();
				}
			}
			else
			{
				int nextCursor = m_Selected;
				int minDis = INT_MAX;
				for(int i = 0; i < GetComponentCount(); ++i)
				{
					if(i == m_Selected)
						continue;
					const Rect& rcNext = m_Components[i].bounds;
					const Rect& rcCurrent = m_Components[m_Selected].bounds;
					switch(wParam)
					{
					case VK_UP:		if(rcNext.top >= rcCurrent.top || rcNext.bottom >= rcCurrent.bottom) continue; else break;
					case VK_DOWN:	if(rcNext.top <= rcCurrent.top || rcNext.bottom <= rcCurrent.bottom) continue; else break;
					case VK_LEFT:	if(rcNext.left >= rcCurrent.left || rcNext.right >= rcCurrent.right) continue; else break;
					case VK_RIGHT:	if(rcNext.left <= rcCurrent.left || rcNext.right <= rcCurrent.right) continue; else break;
					default:		TRESPASS();
					}
					Point v = rcNext.center - rcCurrent.center;
					int dis = math::hypot2(v.x, v.y);
					if(dis < minDis)
					{
						minDis = dis;
						nextCursor = i;
					}
				}
				if(nextCursor != m_Selected)
				{
					m_Selected = nextCursor;
					Invalidate();
				}
			}
			break;
		default:
			for(int i = 0; i < GetComponentCount(); ++i)
			{
				if(m_Components[i].hotkey == wParam)
				{
					if(m_Selected != i)
						Invalidate();
					m_Selected = i;
					UpdateWindow();
					ExitExpose(true);
					break;
				}
			}
			break;
		}
		return 0;
	}
	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		CPaintDC dc(m_hWnd);
		OnDraw(dc.m_hDC, (Rect&)dc.m_ps.rcPaint);
		return 0;
	}
	LRESULT OnPrintClient(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		HDC hDC = (HDC)wParam;
		Rect bounds;
		GetClientRect(&bounds);
		OnDraw(hDC, bounds);
		return 0;
	}
	void Draw(CDCHandle dc, const Rect& bounds)
	{
		const int MAX_FONT_HEIGHT = 200;

		CDC dcSrc;
		dcSrc.CreateCompatibleDC();
		// inactive
		CBitmapHandle bmp = dcSrc.SelectBitmap(m_bmpInactive);
		dc.BitBlt(bounds.left, bounds.top, bounds.w, bounds.h, dcSrc, bounds.left, bounds.top, SRCCOPY);
		// active
		int currentID = GetCurrentID();
		dcSrc.SelectBitmap(m_bmpActive);
		for(int i = 0; i < GetComponentCount(); ++i)
		{
			Rect rc = m_Components[i].bounds;
			const int fontSizePixel = math::min(MAX_FONT_HEIGHT, rc.w, rc.h);
			const int fontSize = MulDiv(fontSizePixel, GetDeviceCaps(dc, LOGPIXELSY), 72);

			LOGFONT logfont;
			::GetObject(AtlGetDefaultGuiFont(), sizeof(LOGFONT), &logfont);
			logfont.lfHeight = -fontSize;
			logfont.lfWidth  = 0;
			CFont font;
			CPen pen;
			COLORREF textColor;
			if(m_Components[i].id == currentID)
			{
				dc.BitBlt(rc.x, rc.y, rc.w, rc.h, dcSrc, rc.x, rc.y, SRCCOPY);
				if(i == m_Selected)
				{	// カーソル上
					logfont.lfUnderline = true;
					pen.CreatePen(PS_SOLID, math::max(10, fontSizePixel/10), GetSysColor(COLOR_WINDOW));
					textColor = GetSysColor(COLOR_HOTLIGHT);
				}
				else
				{	// 同じIDだが、カーソル上ではない
					COLORREF colorWindow = GetSysColor(COLOR_WINDOW);
					pen.CreatePen(PS_SOLID, math::max(10, fontSizePixel/10), colorWindow);
					textColor = BlendRGB(GetSysColor(COLOR_BTNTEXT), colorWindow, 50);
				}
			}
			else
			{	// 非アクティブ
				pen.CreatePen(PS_SOLID, math::max(10, fontSizePixel/10), GetSysColor(COLOR_BTNTEXT));
				textColor = GetSysColor(COLOR_WINDOW);
			}

			font.CreateFontIndirect(&logfont);

			Point center = rc.center;
			rc.w = rc.h = math::max(rc.w, rc.h);
			rc.center = center;
			RECT rect = rc;
			TCHAR text[2] = { m_Components[i].hotkey, 0 };

			CFontHandle  oldfont  = dc.SelectFont(font);
			CPenHandle   oldPen   = dc.SelectPen(pen);

			dc.SetBkMode(TRANSPARENT);
			dc.BeginPath(); 
			dc.DrawText(text, 1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			dc.EndPath(); 
			dc.StrokePath(); 
			dc.SetTextColor(textColor);
			dc.DrawText(text, 1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

			dc.SelectFont(oldfont);
			dc.SelectPen(oldPen);
		}
		if(m_Alpha < 255)
		{
			dcSrc.SelectBitmap(m_bmpActive);
			AlphaBlend(dc, bounds, dcSrc, bounds, (BYTE)(255 - m_Alpha));
		}
		//
		dcSrc.SelectBitmap(bmp);
		dcSrc.DeleteDC();
	}
	void OnDraw(CDCHandle dc, const Rect& bounds)
	{
		CDC dcBack;
		dcBack.CreateCompatibleDC(dc);
		CBitmapHandle bmpOld = dcBack.SelectBitmap(m_bmpBack);
		Draw(dcBack.m_hDC, bounds);
		dc.BitBlt(bounds.left, bounds.top, bounds.w, bounds.h, dcBack, bounds.left, bounds.top, SRCCOPY);
		dcBack.SelectBitmap(bmpOld);
		dcBack.DeleteDC();
	}
	static void AlphaBlend(HDC hdcDst, const Rect& dstrc, HDC hdcSrc, const Rect& srcrc, BYTE alpha)
	{
		BLENDFUNCTION blendfunc = { 0, 0, alpha, 0 };
		::AlphaBlend(hdcDst, dstrc.left, dstrc.top, dstrc.w, dstrc.h, hdcSrc, srcrc.left, srcrc.top, srcrc.w, srcrc.h, blendfunc);
	}
};

} }

//==============================================================================

AVESTA_EXPORT( Expose )
