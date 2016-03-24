// widgets.client.hpp
#pragma once

#ifndef __ATLWIN_H__
	#error widgets.client.hpp requires atlwin.h to be included first
#endif

namespace mew
{
	namespace ui
	{
		class CWindowEx : public ATL::CWindow
		{
			typedef ATL::CWindow super;
		public:
			CWindowEx()	{}
			CWindowEx(HWND hWnd) : super(hWnd)	{}
			CWindowEx& operator = (HWND hwnd)	{ super::operator = (hwnd); return *this; }

			CWindowEx GetAncestor(UINT gaFlags) const
			{
				return ::GetAncestor(m_hWnd, gaFlags);
			}
			bool HasFocus() const
			{
				if(!m_hWnd)
					return false;
				HWND hCurrentFocus = ::GetFocus();
				return m_hWnd == hCurrentFocus || ::IsChild(m_hWnd, hCurrentFocus);
			}

			BOOL ModifyClassStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags = 0) throw();
			bool DragDetect(POINT pt);
			using super::SendMessageToDescendants;
			void SendMessageToDescendants(UINT msg, WPARAM wParam, LPARAM lParam, BOOL (__stdcall *op)(HWND)) throw();
		};
	}
}

//==============================================================================

#define BEGIN_MSG_MAP_(name) \
	BOOL name(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID = 0) \
	{										\
		BOOL bHandled = true;				\
		(hWnd);								\
		(uMsg);								\
		(wParam);							\
		(lParam);							\
		(lResult);							\
		(bHandled);							\
		switch(dwMsgMapID)					\
		{									\
		case 0:

#define CHAIN_MSG_MAP_TO(name)								\
	{														\
		if(name(hWnd, uMsg, wParam, lParam, lResult))		\
			return true;									\
	}

// bool OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
#define MSG_HANDLER(msg, func)							\
	if(uMsg == msg)										\
	{													\
		if((bool)func(uMsg, wParam, lParam, lResult))	\
			return true;								\
	}

#define MSG_LAMBDA(msg, lambda)		\
	if (uMsg == msg)				\
	{								\
		lResult = 0;				\
		lambda;						\
		return true;				\
	}

#define REFLECTED_NOTIFY_CODE_LAMBDA(cd, lambda)				\
	if(uMsg == OCM_NOTIFY && cd == ((LPNMHDR)lParam)->code)		\
	{					\
		lResult = 0;	\
		lambda;			\
		return true;	\
	}
