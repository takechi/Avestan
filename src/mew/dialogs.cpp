// dialogs.cpp

#include "stdafx.h"
#include "widgets.hpp"
#include "widgets.client.hpp"
#include "shell.hpp"
#include "private.h"
#include "std/buffer.hpp"

using namespace mew::ui;

//==============================================================================
// File

namespace
{
	void CreateFileFilter(StringBuffer& buffer, const string& filter)
	{
		size_t len = filter.length();
		buffer.reserve(len+2);
		PCTSTR src = filter.str();
		for(size_t i = 0; i < len; ++i)
		{
			if(src[i] == _T('|'))
				buffer.push_back(_T('\0'));
			else
				buffer.push_back(src[i]);
		}
		buffer.push_back(_T('\0'));
		buffer.push_back(_T('\0'));
	}

	//==============================================================================
	// ダイアログをセンタリングするため.
	class FileDlg : public CFileDialogImpl<FileDlg>
	{
		using super = CFileDialogImpl<FileDlg>;
	public:
		FileDlg(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
			PCTSTR lpszDefExt = NULL,
			PCTSTR lpszFileName = NULL,
			DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
			PCTSTR lpszFilter = NULL,
			HWND hWndParent = NULL)
			: super(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, hWndParent)
		{ }
		void OnInitDone(LPOFNOTIFY /*lpon*/)
		{
			GetParent().CenterWindow();
		}
		string GetFilePath() const
		{
			return string(m_szFileName);
		}
	};
}

//==============================================================================

string mew::ui::OpenDialog(HWND hwnd, string filter)
{
	StringBuffer buffer;
	CreateFileFilter(buffer, filter);
	FileDlg dlg(TRUE, 0, 0, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, buffer, hwnd);
	if(dlg.DoModal() != IDOK)
		return null;
	return dlg.GetFilePath();
}

string mew::ui::SaveDialog(HWND hwnd, string filter, PCTSTR strDefExt)
{
	StringBuffer buffer;
	CreateFileFilter(buffer, filter);
	FileDlg dlg(FALSE, strDefExt, 0, OFN_OVERWRITEPROMPT, buffer, hwnd);
	if(dlg.DoModal() != IDOK)
		return null;
	return dlg.GetFilePath();
}

//==============================================================================
// About

namespace
{
	class AboutDlg : public CDialogImpl<AboutDlg>
	{
	public:
		io::Version	m_Version;
		CHyperLink m_link;

		enum { IDD = IDD_ABOUT };

		BEGIN_MSG_MAP(_)
			MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
			COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
			COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		END_MSG_MAP()

		LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&)
		{
			CenterWindow();
			PCTSTR name    = m_Version.QueryValue(_T("ProductName"));
			PCTSTR version = m_Version.QueryValue(_T("FileVersion"));
			PCTSTR author  = m_Version.QueryValue(_T("LegalCopyright"));
			PCTSTR comment = m_Version.QueryValue(_T("Comments"));
      PCTSTR changer = m_Version.QueryValue(_T("ChangedBy"));
			// name & author
			TCHAR ver[1024] = { 0 };
			for(int i = 0, j = 0; version[i]; ++i)
			{
				switch(version[i])
				{
				case _T(' '):
					break;
				case _T(','):
					ver[j++] = _T('.');
					break;
				default:
					ver[j++] = version[i];
					break;
				}
			}
			TCHAR buffer[1024];
			wsprintf(buffer, _T("%s %s\r\n%s"), name, ver, author);
			SetDlgItemText(IDC_ABOUTTEXT, buffer);
			// url
			SetDlgItemText(IDC_ABOUTURL, comment);
			m_link.SubclassWindow(GetDlgItem(IDC_ABOUTURL));
			m_link.SetHyperLink(comment);
			// changed by
			wsprintf(buffer, _T("(changed by %s)"), changer);
			SetDlgItemText(IDC_CHANGERTEXT, buffer);
			// icon
			HICON hIconLarge = null;
			//string path = m_Version.GetPath();
			//ExtractIconEx(path.str(), 0, &hIconLarge, null, 1);
			//ASSERT(hIconLarge);
			hIconLarge = GetParent().GetIcon();
			CWindowEx wndIcon = GetDlgItem(IDC_ABOUTICON);
			ASSERT(wndIcon.IsWindow());
			wndIcon.SendMessage(STM_SETICON, (WPARAM)hIconLarge, 0);
			return TRUE;
		}
		LRESULT OnCloseCmd(WORD, WORD wID, HWND, BOOL&)
		{
			//HWND hwndIcon = GetDlgItem(IDC_ABOUTICON);
			//if(HICON hIcon = (HICON)::SendMessage(hwndIcon, STM_GETICON , 0, 0))
			//{
			//	::DestroyIcon(hIcon);
			//	::SendMessage(hwndIcon, STM_SETICON , (WPARAM)(HICON)0, 0);
			//}
			EndDialog(wID);
			return 0;
		}
	};
}

void mew::ui::AboutDialog(HWND hwnd, string module)
{
	if(!module)
	{
		TCHAR exe[MAX_PATH];
		::GetModuleFileName(null, exe, MAX_PATH);
		module = exe;
	}
	AboutDlg dlg;
	if(dlg.m_Version.Open(module.str()))
	{
		dlg.DoModal(hwnd);
	}
}

//==============================================================================
// Font

namespace
{
	class FontDlg : public WTL::CFontDialogImpl<FontDlg>
	{
		using super = WTL::CFontDialogImpl<FontDlg>;
		string		m_caption;
		function	m_apply;
		LOGFONT		m_prev;

	public:
		FontDlg(LOGFONT* info, string caption, function apply) :
			super(info, CF_APPLY | CF_NOVERTFONTS | CF_SCREENFONTS | CF_NOSCRIPTSEL),
			m_caption(caption), m_apply(apply)
		{
			memset(&m_prev, 0, sizeof(LOGFONT));
		}

		BEGIN_MSG_MAP(FontDlg)
			MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
			COMMAND_HANDLER(1026, BN_CLICKED, OnApply)
		END_MSG_MAP()

		LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			SetWindowText(m_caption.str());
			CenterWindow();
			bHandled = false;
			return 0;
		}
		LRESULT OnApply(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL&)
		{
			GetCurrentFont(&m_lf);
			Apply();
			return 0;
		}

		void Apply()
		{
			if(m_apply && memcmp(&m_prev, &m_lf, sizeof(LOGFONT)))
			{
				memcpy(&m_prev, &m_lf, sizeof(LOGFONT));
				m_lf.lfCharSet = DEFAULT_CHARSET;
				HFONT hFont = ::CreateFontIndirect(&m_lf);
				message m;
				m["font"] = (INT_PTR)hFont;
				m_apply(m);
			}
		}
	};
}

void mew::ui::FontDialog(HWND hwnd, HFONT hFont, string caption, function apply)
{
	if(!hFont)
		hFont = AtlGetDefaultGuiFont();
	LOGFONT info;
	::GetObject(hFont, sizeof(LOGFONT), &info);
	FontDlg dlg(&info, caption, apply);
	if(dlg.DoModal(hwnd) == IDOK)
		dlg.Apply();
}

//==============================================================================
