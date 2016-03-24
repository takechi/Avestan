// fileop.cpp

#include "stdafx.h"
#include "avesta.hpp"
#include "struct.hpp"

using namespace avesta;

//================================================================================

namespace
{
	class SHFileOpHack : public CWindowImpl<SHFileOpHack>
	{
		enum
		{
			WM_GETSUBCLASS	= WM_USER + 1111,
			SLIP_TIMERID	= 0x1234,
			SLIP_INTERVAL	= 10,
			SLIP_DURATION	= 800,
		};

		DWORD	m_timeFrom;
		Point	m_ptFrom;
		Point	m_ptTo;
		int		m_index;

		static const int MAX_COUNT = 5; // +1
		static HWND s_hwndProgress[MAX_COUNT];

		static int GetUnused(HWND hwnd, const Rect& rcProgress, const Rect& rcDesktop)
		{
			for(int i = 0; i < MAX_COUNT; ++i)
			{
				if(!::IsWindow(s_hwndProgress[i]) || !::IsWindowVisible(s_hwndProgress[i]))
				{
					s_hwndProgress[i] = hwnd;
					return i;
				}
			}
			return 0;
		}
		static Point Destination(const Rect& rcProgress, const Rect& rcDesktop, int index)
		{
			Point pt;
			pt.x = rcDesktop.left + ((rcDesktop.w-rcProgress.w) * (MAX_COUNT-index) / MAX_COUNT);
			pt.y = rcDesktop.bottom - rcProgress.h;
			return pt;
		}

	public:
		SHFileOpHack(HWND hwnd)
		{
            SubclassWindow(hwnd);
			Rect rc, rcDesktop;
			GetWindowRect(&rc);
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rcDesktop, 0);
			m_index = GetUnused(hwnd, rc, rcDesktop);
			m_ptTo = Destination(rc, rcDesktop, m_index);
			m_ptFrom = rc.location;
			m_timeFrom = ::GetTickCount();
			SetTimer(SLIP_TIMERID, SLIP_INTERVAL);
		}
		void EndAnimation()
		{
			KillTimer(SLIP_TIMERID);
			m_timeFrom = 0;
		}

		BEGIN_MSG_MAP(_)
			MESSAGE_HANDLER(WM_ACTIVATE		, OnActivate)
			MESSAGE_HANDLER(WM_GETSUBCLASS	, OnGetSubclass)
			MESSAGE_HANDLER(WM_TIMER		, OnTimer)
		END_MSG_MAP()

		LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			bHandled = false;
			if(m_timeFrom != 0)
			{
				switch(LOWORD(wParam))
				{
				case WA_INACTIVE:
					break;
				case WA_CLICKACTIVE:
					EndAnimation();
					break;
				default:
					if(lParam)
						::SetActiveWindow((HWND)lParam);
					break;
				}
			}
			return 0;
		}
		LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			if(wParam == SLIP_TIMERID)
			{
				Point pt;
				INT t = ::GetTickCount() - m_timeFrom;
				if(t >= SLIP_DURATION)
				{
					EndAnimation();
					pt = m_ptTo;
				}
				else
				{
					double a = math::cos((double)(SLIP_DURATION-t) / SLIP_DURATION * math::PI) / 2 + 0.5;
					pt.x = (int)(m_ptTo.x * a + m_ptFrom.x * (1.0-a));
					pt.y = (int)(m_ptTo.y * a + m_ptFrom.y * (1.0-a));
				}
				SetWindowPos(NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_DEFERERASE);
			}
			else
			{
				bHandled = false;
			}
			return 0;
		}
		LRESULT OnGetSubclass(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			return 0x12345678;
		}
		void OnFinalMessage(HWND hwnd)
		{
			if(s_hwndProgress[m_index] == hwnd)
				s_hwndProgress[m_index] = null;
			delete this;
		}

		static DWORD GetFileOperationMode(HWND hwnd)
		{
			TCHAR clsname[7];
			::GetClassName(hwnd, clsname, MAX_PATH);
			if(lstrcmp(clsname, _T("#32770")) != 0)
				return 0;
			TCHAR wndtext[MAX_PATH];
			::GetWindowText(hwnd, wndtext, MAX_PATH);
			TRACE(L"$1 / $2", clsname, wndtext);
			if(lstrcmp(wndtext, _T("移動しています...")) == 0)
				return FO_MOVE;
			if(lstrcmp(wndtext, _T("コピーしています...")) == 0)
				return FO_COPY;
			if(lstrcmp(wndtext, _T("削除しています...")) == 0)
				return FO_DELETE;
			// TODO: これホント？見たことが無いのでタイトルがわかりません。
			if(lstrcmp(wndtext, _T("名前を変更しています...")) == 0)
				return FO_RENAME;
			return 0; // non-SHOp
		}

		static bool GetSubclass(HWND hwnd)
		{
			return ::SendMessage(hwnd, WM_GETSUBCLASS, 0, 0) == 0x12345678;
		}
	};

	HWND SHFileOpHack::s_hwndProgress[MAX_COUNT];
}

HRESULT avesta::FileOperationHack(HWND hwndProgress, HWND hwndOwner)
{
	DWORD op = SHFileOpHack::GetFileOperationMode(hwndProgress);
	if(!op)
		return E_FAIL;
	// SHFileOperationウィンドウの構成
	// - #32770
	//    +- SysAnimate32
	//    +- Static : FROM から TO へ
	//    +- Static : FILENAME
	//    +- msctls_progress32
	//    +- Static : 残り N分
	//    +- Button : キャンセル
	HRESULT hr = S_FALSE;
	if(!SHFileOpHack::GetSubclass(hwndProgress))
	{
		new SHFileOpHack(hwndProgress);
		hr = S_OK;
	}
	::SetActiveWindow(hwndOwner);
	::SetForegroundWindow(hwndOwner);
	::SetFocus(hwndOwner);
	return hr;
}

//================================================================================

namespace
{
	static bool IsEqualClassName(HWND hwnd, PCTSTR clsname)
	{
		if(!hwnd)
			return false;
		TCHAR buffer[MAX_PATH] = _T("");
		::GetClassName(hwnd, buffer, MAX_PATH);
		return str::equals(buffer, clsname);
	}
	static BOOL CALLBACK EnumFileDialog(HWND hwnd, LPARAM lParam)
	{
		HWND hComboBox = NULL;
		if(!::IsWindowVisible(hwnd))
			return TRUE;
		if(IsEqualClassName(hwnd, _T("#32770"))
		&& IsEqualClassName(::GetDlgItem(hwnd, 0x00000461), _T("SHELLDLL_DefView"))
		&& IsEqualClassName(hComboBox = ::GetDlgItem(hwnd, 0x0000047C), _T("ComboBoxEx32")))
		{
			// トップレベル ファイルダイアログ
			*(HWND*)lParam = hComboBox;
			return FALSE;
		}

		// ファイルダイアログではないので子供を捜す
		if(HWND hwndChild = FindWindowEx(hwnd, null, _T("#32770"), null))
		{
			if(IsEqualClassName(::GetDlgItem(hwndChild, 0x00000461), _T("SHELLDLL_DefView"))
			&& IsEqualClassName(hComboBox = ::GetDlgItem(hwndChild, 0x0000047C), _T("ComboBoxEx32")))
			{
				*(HWND*)lParam = hComboBox;
				return FALSE;
			}
		}
        return TRUE;
	}
	static HWND FindPathEditOfFileDialog()
	{
		HWND hwnd = null;
		EnumWindows(EnumFileDialog, (LPARAM)&hwnd);
		return hwnd;
	}
}

HRESULT avesta::FileDialogSetPath(PCWSTR path)
{
	HWND hComboBox = FindPathEditOfFileDialog();
	if(!hComboBox)
		return E_FAIL;

	WCHAR orig[MAX_PATH];
	::SendMessage(hComboBox, WM_GETTEXT, MAX_PATH, (LPARAM)orig);
	::SendMessage(hComboBox, WM_SETTEXT, 0, (LPARAM)path);
	::PostMessage(hComboBox, WM_KEYDOWN, VK_RETURN, 0);
	HWND dlg = ::GetAncestor(hComboBox, GA_ROOT);
	::BringWindowToTop(dlg);
	::SetActiveWindow(dlg);
	::SendMessage(hComboBox, WM_SETTEXT, 0, (LPARAM)orig);
	return S_OK;
}
