// WTLSplitter.hpp
#pragma once

namespace WTLEX
{
	//==============================================================================

	class __declspec(novtable) CSplitterBase
	{
	public:
		enum DragMode
		{
			DragNone,
			DragHorz,
			DragVert,
		};

	protected:
		static HCURSOR s_hCursors[2];
		Size		m_Border;
		DragMode	m_DragMode;
		Point		m_DragStart;
		CWindowEx		m_wndDrag[2];

		CSplitterBase() : m_DragMode(DragNone) {}
		void SetSizeCursor(DragMode mode)
		{
			if(!s_hCursors[0])
			{
				s_hCursors[0] = ::LoadCursor(NULL, IDC_SIZENS);
				s_hCursors[1] = ::LoadCursor(NULL, IDC_SIZEWE);
			}
			switch(mode)
			{
			case DragVert:
				::SetCursor(s_hCursors[0]);
				break;
			case DragHorz:
				::SetCursor(s_hCursors[1]);
				break;
			}
		}
		static int DistanceOfPointAndRect(const POINT& pt, const RECT& rc)
		{
			int dx, dy;
			if(pt.x < rc.left)
				dx = rc.left - pt.x;
			else if(pt.x > rc.right)
				dx = pt.x - rc.right;
			else
				dx = 0;
			if(pt.y < rc.top)
				dy = rc.top - pt.y;
			else if(pt.y > rc.bottom)
				dy = pt.y - rc.bottom;
			else
				dy = 0;
			return dx + dy;
		}
		static DragMode HitTestImpl(const CWindowEx view[2], CWindowEx hwnds[2])
		{
			RECT rc[2];
			view[0].GetWindowRect(&rc[0]);
			view[1].GetWindowRect(&rc[1]);
			DragMode mode = ((rc[0].right < rc[1].left || rc[1].right < rc[0].left) ? DragHorz : DragVert);
			if(hwnds)
			{
				switch(mode)
				{
				case DragVert:
					if(rc[0].top < rc[1].top)	{ hwnds[0] = view[0]; hwnds[1] = view[1]; }
					else 						{ hwnds[0] = view[1]; hwnds[1] = view[0]; }
					break;
				case DragHorz:
					if(rc[0].left < rc[1].left)	{ hwnds[0] = view[0]; hwnds[1] = view[1]; }
					else						{ hwnds[0] = view[1]; hwnds[1] = view[0]; }
					break;
				default:
					TRESPASS();
				}
			}
			return mode;
		}

	public:
		LRESULT OnSettingChange(UINT, WPARAM, LPARAM, BOOL& bHandled)
		{
			m_Border.assign(::GetSystemMetrics(SM_CXSIZEFRAME), ::GetSystemMetrics(SM_CYSIZEFRAME));
			bHandled = false;
			return 0;
		}
		bool IsDragging() const	{ return m_DragMode != DragNone; }
	};

	__declspec(selectany) HCURSOR CSplitterBase::s_hCursors[2];

	//==============================================================================

	template < class T >
	class __declspec(novtable) CSplitter : public CSplitterBase
	{
	public: // override
		/// pt の位置でのクリックが、ドラッグを開始すべきか否かを返す.
		bool IsDragRegion(POINT pt)		{ return true; }
		/// w がドラッグ可能なウィンドウか否かを返す.
		bool IsDragTarget(CWindowEx w)	{ return w.IsWindowVisible() != 0; }
		/// 実際にドラッグ中のウィンドウをリサイズする.
		void OnResizeDragTargets(DragMode mode, CWindowEx wndDrag[2], POINT pt);

		const T& get_final() const throw()	{ return *static_cast<T*>(this); }
		      T& get_final()       throw()	{ return *static_cast<T*>(this); }
		__declspec(property(get=get_final)) T final;

	public: // msg map
		BEGIN_MSG_MAP_( ProcessSplitterMessage )
			MESSAGE_HANDLER(WM_SETCURSOR		, OnSetCursor )
			MESSAGE_HANDLER(WM_LBUTTONUP		, OnCancelDrag )
			MESSAGE_HANDLER(WM_CAPTURECHANGED	, OnCancelDrag )
			MESSAGE_HANDLER(WM_LBUTTONDOWN		, OnLButtonDown)
			MESSAGE_HANDLER(WM_MOUSEMOVE		, OnMouseMove)
			MESSAGE_HANDLER(WM_CREATE			, OnSettingChange)
			MESSAGE_HANDLER(WM_SETTINGCHANGE	, OnSettingChange)
		END_MSG_MAP()

	public:
		bool BeginDrag(Point pt)
		{
			ASSERT(!m_wndDrag[0]);
			ASSERT(!m_wndDrag[1]);
			m_DragMode = final.SplitterHitTest(pt, m_wndDrag);
			if(m_DragMode == DragNone)
				return false;
			m_DragStart = pt;
			final.SetCapture();
			SetSizeCursor(m_DragMode);
			return true;
		}
		void CancelDrag()
		{
			if(m_DragMode == DragNone)
				return;
			if(GetCapture() == final.m_hWnd)
				ReleaseCapture();
			m_wndDrag[0].Detach();
			m_wndDrag[1].Detach();
			m_DragMode = DragNone;
		}
		DragMode SplitterHitTest(Point pt, CWindowEx hwnds[2] = null)
		{
			if(!final.IsDragRegion(pt))
				return DragNone;
			CWindowEx view[2];
			int dis[2] = { INT_MAX, INT_MAX };
			for(CWindowEx w = final.GetWindow(GW_CHILD); w; w = w.GetWindow(GW_HWNDNEXT))
			{
				final.IsDragTarget(w);
				if(!IsDragTarget(w))
					continue;
				RECT rc;
				w.GetWindowRect(&rc);
				final.ScreenToClient(&rc);
				int d = DistanceOfPointAndRect(pt, rc);
				if(d < dis[0])
				{
					view[1] = view[0]; dis[1] = dis[0];
					view[0] = w; dis[0] = d;
				}
				else if(d < dis[1])
				{
					view[1] = w; dis[1] = d;
				}
			}
			if(!view[1] || !view[0])
				return DragNone;
			return HitTestImpl(view, hwnds);
		}
		LRESULT OnMouseMove(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			Point pt(GET_XY_LPARAM(lParam));
			if(!IsDragging())
			{
				if(!final.IsDragRegion(pt))
				{
					bHandled = false;
				}
				else
				{
					HWND hWnd = final.ChildWindowFromPointEx(pt, CWP_SKIPINVISIBLE);
					if(hWnd == final.m_hWnd)
						SetSizeCursor(SplitterHitTest(pt));
				}
			}
			else if(pt != m_DragStart)
			{
				final.OnResizeDragTargets(m_DragMode, m_wndDrag, pt);
				m_DragStart = pt;
			}
			return 0;
		}
		LRESULT OnLButtonDown(UINT, WPARAM, LPARAM lParam, BOOL& bHandled)
		{
			if(IsDragging())
				return 0;
			Point pt(GET_XY_LPARAM(lParam));
			if(!BeginDrag(pt))
				bHandled = false;
			return 0;
		}
		LRESULT OnCancelDrag(UINT, WPARAM, LPARAM, BOOL& bHandled)
		{
			if(IsDragging())
				CancelDrag();
			bHandled = false;
			return 0;
		}
		LRESULT OnSetCursor(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			if((HWND)wParam == final.m_hWnd && LOWORD(lParam) == HTCLIENT)
			{
				DWORD dwPos = ::GetMessagePos();
				Point pt(GET_XY_LPARAM(dwPos));
				final.ScreenToClient(&pt);
				if(final.IsDragRegion(pt) && final.ChildWindowFromPointEx(pt, CWP_SKIPINVISIBLE) == final.m_hWnd)
					return true;
			}
			bHandled = false;
			return false;
		}
	};
}
