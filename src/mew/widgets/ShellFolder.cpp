// ShellFolder.cpp
#pragma once

#include "stdafx.h"

#include "../private.h"
#include "io.hpp"
#include "ShellFolder.h"

#include "../server/main.hpp" // もうぐちゃぐちゃ……

static const UINT MAX_HISTORY = 20;

const DWORD MEW_FWF_ALWAYS    = FWF_SHOWSELALWAYS | FWF_NOWEBVIEW;
const DWORD MEW_FWF_ALWAYSNOT = FWF_ABBREVIATEDNAMES | FWF_SNAPTOGRID | FWF_OWNERDATA | FWF_BESTFITWINDOW | FWF_DESKTOP | FWF_SINGLESEL | FWF_NOSUBFOLDERS | FWF_TRANSPARENT | FWF_NOCLIENTEDGE | FWF_NOSCROLL | FWF_NOICONS | FWF_SINGLECLICKACTIVATE | FWF_NOWEBVIEW | FWF_HIDEFILENAMES;
// FWF_TRANSPARENT : 効果が無いぽい
// FWF_NOICONS     : 何も描画されなくなる

#pragma comment(lib, "imm32.lib")

enum ShellInternalMessage
{
	// IShellBrowserを実装するウィンドウはこのメッセージを処理しなくてはならないらしいが、
	// XPでは呼ばれないような気がする。
	WM_GETISHELLBROWSER = WM_USER+7,
	WM_ENABLECHECKBOXCHANGE, // (wParam)bool enabled
};

//==============================================================================

namespace mew { namespace ui {
  #define USE_ADHOC_FIX_FOR_SHELLBROWSER_LISTVIEW_DETAILS
  #if defined(USE_ADHOC_FIX_FOR_SHELLBROWSER_LISTVIEW_DETAILS)
  VOID CALLBACK setViewModeDetailsTimerProc(HWND hwndList, UINT, UINT_PTR idEvent, DWORD) {
    ::KillTimer(hwndList, idEvent);
    POINT pt;
    ListView_GetOrigin(hwndList, &pt);
    if (pt.y < 0) {
      ListView_SetView(hwndList, LV_VIEW_SMALLICON);
      ListView_SetView(hwndList, LV_VIEW_DETAILS);
    }
  }
  #endif

using namespace mew::io;

//==============================================================================

Shell::History::History()
{
	m_cursor = 0;
}
bool Shell::History::Add(IEntry* entry)
{
	ASSERT(entry);
	if(!m_history.empty())
	{
		if(entry->Equals(GetRelativeHistory(0)))
			return false;
	}
	m_history.resize(m_cursor);
	m_history.push_back(entry);
	if(m_cursor > MAX_HISTORY)
		m_history.pop_front();
	m_cursor = m_history.size();
	return true;
}
void Shell::History::Replace(IEntry* entry)
{
	ASSERT(entry);
	if(m_cursor == 0)
		return;
	--m_cursor;
	Add(entry);
}
IEntry* Shell::History::GetRelativeHistory(int step, size_t* index)
{
	size_t at = (size_t)math::clamp<int>(m_cursor + step, 1, m_history.size())-1;
	if(index) *index = at;
	return m_history[at];
}
size_t Shell::History::Back(size_t step)
{
	if(m_cursor < step)
	{
		size_t c = m_cursor;
		m_cursor = 0;
		return c;
	}
	else
	{
		m_cursor -= step;
		return step;
	}
}
size_t Shell::History::BackLength() const
{
	return m_cursor <= 1 ? 0 : m_cursor-1;
}
size_t Shell::History::Forward(size_t step)
{
	if(m_cursor + step > m_history.size())
	{
		size_t c = m_cursor;
		m_cursor = m_history.size();
		return m_history.size() - c;
	}
	else
	{
		m_cursor += step;
		return step;
	}
}
size_t Shell::History::ForwardLength() const
{
	return m_history.size() - m_cursor;
}

//==============================================================================

namespace
{
	static SHELLFLAGSTATE theShellFlagState;

	// メンバには触らないのでスタティックに一つだけ作る
	class SHELLDLL_DefView : public CMessageMap
	{
	public:
		BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapId = 0)
		{
			switch(uMsg)
			{
			case WM_CONTEXTMENU: // そのまま親に転送する
				lResult = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
				return true;
			case WM_MOUSEWHEEL:
				lResult = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
				return true;
			case WM_ERASEBKGND:
				lResult = 1;
				return true;
			case WM_NOTIFY:
				switch(((NMHDR*)lParam)->code)
				{
				case LVN_BEGINLABELEDIT:
					SendMessage(GetParent(hWnd), uMsg, wParam, lParam);
					break;
				case LVN_ITEMCHANGING:
					lResult = SendMessage(GetParent(hWnd), uMsg, wParam, lParam);
					if(lResult)
						return true;
					break;
				}
				break;
			}
			return false;
		}
	};
	static SHELLDLL_DefView theDefViewMap;

	// メンバには触らないのでスタティックに一つだけ作る
	class SHELLDLL_ListView : public CMessageMap
	{
	private:
		static bool ListView_LoopCursor(HWND hWnd, UINT direction, UINT vkeyReplace)
		{
			if(!theAvesta->LoopCursor)
				return false;
			int focus = ListView_GetNextItem(hWnd, -1, LVNI_FOCUSED);
			if(focus < 0)
				return false;
			int next = ListView_GetNextItem(hWnd, focus, direction);
			if(next >= 0 && next != focus)
				return false;
			::SendMessage(hWnd, WM_KEYDOWN, vkeyReplace, 0);
			return true;
		}
	public:
		BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapId = 0)
		{
			switch(uMsg)
			{
			case WM_CONTEXTMENU: // そのまま親に転送する
				lResult = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
				return true;
			case WM_LBUTTONDOWN:
			{
				LVHITTESTINFO hit = { GET_XY_LPARAM(lParam), 0, -1, 0 };
				int index = ListView_HitTest(hWnd, &hit);
				if(index >= 0)
				{
					if((hit.flags & (LVHT_ONITEMICON | LVHT_ONITEMLABEL)) && IsKeyPressed(VK_SHIFT))
					{
						int selected = ListView_GetNextItem(hWnd, -1, LVNI_ALL | LVNI_SELECTED);
						if(selected == -1 || (selected == index && ListView_GetSelectedCount(hWnd) == 1))
						{	// 無選択状態での Shift+左クリック の動作が気に入らないため。
							ListView_SetItemState(hWnd, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
							::SendMessage(::GetParent(::GetParent(hWnd)), WM_ENABLECHECKBOXCHANGE, (WPARAM)false, lParam);
							return TRUE;
						}
					}
					bool onCheckBox = ((hit.flags & LVHT_ONITEMSTATEICON) != 0);
					return ::SendMessage(::GetParent(::GetParent(hWnd)), WM_ENABLECHECKBOXCHANGE, (WPARAM)onCheckBox, lParam);
				}
				break;
			}
			case WM_MBUTTONDOWN:
			{
				UINT32 m = theAvesta->MiddleClick;
				if(m != 0)
				{
					LVHITTESTINFO hit = { GET_XY_LPARAM(lParam), 0, -1, 0 };
					int index = ListView_HitTest(hWnd, &hit);
					if(index >= 0 && (hit.flags & (LVHT_ONITEMICON | LVHT_ONITEMLABEL)))
					{
						if(theAvesta->MiddleSingle)
							ListView_SetItemState(hWnd, -1, 0, LVIS_SELECTED);
						ListView_SetItemState(hWnd, index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
						afx::PumpMessage();
						// TODO: ユーザがカスタマイズできるように
						UINT32 mods = ui::GetCurrentModifiers();
						if(mods == 0)
							mods = m;
						afx::SetModifierState(VK_RETURN, mods);
						::SendMessage(hWnd, WM_KEYDOWN, VK_RETURN, 0);
						afx::RestoreModifierState(VK_RETURN);
						return true;
					}
				}
				break;
			}
			case WM_KEYDOWN:
				switch(wParam)
				{
				case VK_SPACE:
					::SendMessage(::GetParent(::GetParent(hWnd)), WM_ENABLECHECKBOXCHANGE, (WPARAM)true, 0xFFFFFFFF);
					break;
				case VK_UP:
					if(ListView_LoopCursor(hWnd, LVNI_ABOVE, VK_END))
						return true;
					break;
				case VK_DOWN:
					if(ListView_LoopCursor(hWnd, LVNI_BELOW, VK_HOME))
						return true;
					break;
				}
				break;
			case WM_LBUTTONUP:
				if(wParam & MK_RBUTTON)
				{
					POINT pt = { GET_XY_LPARAM(lParam) };
					::ClientToScreen(hWnd, &pt);
					::SendMessage(hWnd, WM_CONTEXTMENU, (WPARAM)hWnd, MAKELPARAM(pt.x, pt.y));
					return true;
				}
				break;
			case WM_MOUSEWHEEL:
				lResult = ::SendMessage(GetParent(GetParent(hWnd)), uMsg, wParam, lParam);
				if(lResult)
					return true;
				break; // default.
			case WM_SYSCHAR:
				switch(wParam)
				{
				case 13: // enter : これをつぶしておかないと、ビープが鳴るため。
					return true;
				}
				break;
			case LVM_SETVIEW:
				::PostMessage(::GetParent(::GetParent(hWnd)), LVM_SETVIEW, wParam, lParam);
				break;
			}
			return false;
		}
	};
	static SHELLDLL_ListView theShellListMap;

	// メンバには触らないのでスタティックに一つだけ作る
	class SHELLDLL_Header : public CMessageMap
	{
	public:
		BOOL ProcessWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapId = 0)
		{
			switch(uMsg)
			{
			case WM_SETFOCUS:
			{
				TRACE(_T("Header WM_SETFOCUS"));
				break;
			}
			case WM_KILLFOCUS:
			{
				TRACE(_T("Header WM_KILLFOCUS"));
				break;
			}
			default:
				break;
			}
			return false;
		}
	};
	static SHELLDLL_Header theShellHeaderMap;
}

//==============================================================================

class ShellBrowser : public Root< implements<IOleWindow, IShellBrowser, ICommDlgBrowser, ICommDlgBrowser2> >
{
private:
	Shell&	m_Owner;

public:
	ShellBrowser(Shell* owner) : m_Owner(*owner) {}

public: // IOleWindow
	STDMETHODIMP GetWindow(HWND * lphwnd)	{ *lphwnd = m_Owner; return S_OK;  }
	STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode)	{ return E_NOTIMPL; }

public: // ICommDlgBrowser
	static HRESULT QueryFolderAsDefault(IEntry* entry, REFINTF pp)
	{
		string path = entry->Path;
		if(!path || afx::PathIsFolder(path.str()))
			return entry->QueryObject(pp);
		else
			return E_FAIL;
	}
	static bool IsVirtualFolder(IEntry* folder)
	{
		ASSERT(folder);
		if(afx::ILIsRoot(folder->ID))
			return false; // desktop is not a virtual folder.
		string path = folder->Path;
		return !path;
	}
	STDMETHODIMP OnDefaultCommand(IShellView* pShellView)
	{	//handle double click and ENTER key if needed
		ref<IEntryList> selections;
		if FAILED(m_Owner.GetContents(&selections, SELECTED))
			return S_FALSE;
		ref<IShellFolder> pCurrentFolder = m_Owner.GetCurrentFolder();
		const int count = selections->Count;
		if(IsVirtualFolder(m_Owner.GetCurrentEntry()))
		{	// 仮想フォルダ。この場合は、どうせデフォルト実行以外できないので、OnDefaultExecute()しない。
			bool containsFolder = false;
			array<IEntry> entries;
			entries.reserve(count);
			// まず、1pass目でフォルダを含んでいるか否かを調べる.
			for(int i = 0; i < count; ++i)
			{
				ref<IEntry> item;
				if FAILED(selections->GetAt(&item, i))
					continue;
				ref<IEntry> resolved;
				item->GetLinked(&resolved);
				entries.push_back(resolved);
				if(containsFolder)
				{	// すでにフォルダを含むことが分かっている場合は、これ以上のチェックを行う必要は無い
					continue;
				}
				ref<IShellFolder> folder;
				containsFolder = SUCCEEDED(QueryFolderAsDefault(resolved, &folder));
			}
			// 2pass目。フォルダを含まない場合は、デフォルト実行に任せる。
			if(!containsFolder)
				return S_FALSE;
			// フォルダを含んでいる場合...
			for(size_t i = 0; i < entries.size(); ++i)
			{
				ref<IShellFolder> folder;
				if SUCCEEDED(entries[i]->QueryObject(&folder))
				{
					if(pCurrentFolder != m_Owner.GetCurrentFolder())
					{
						TRACE(_T("複数のフォルダに対して移動しようとしている"));
						continue;
					}
					m_Owner.OnDefaultDirectoryChange(entries[i], folder);
				}
				else
				{	// zipfldr の場合は、この後で呼ばれるであろう ShellExecuteEx() に失敗するが、まぁ仕方ない……。
					m_Owner.OnDefaultExecute(entries[i]);
				}
			}
		}
		else
		{	// 実フォルダ。
			for(int i = 0; i < count; ++i)
			{
				ref<IEntry> item;
				if FAILED(selections->GetAt(&item, i))
					continue;
				ref<IEntry> resolved;
				item->GetLinked(&resolved);
				ref<IShellFolder> folder;
				if SUCCEEDED(QueryFolderAsDefault(resolved, &folder))
				{
					if(pCurrentFolder != m_Owner.GetCurrentFolder())
					{
						TRACE(_T("複数のフォルダに対して移動しようとしている"));
						continue;
					}
					m_Owner.OnDefaultDirectoryChange(resolved, folder);
				}
				else
				{
					m_Owner.OnDefaultExecute(item);
				}
			}
		}
		return S_OK;
	}
	STDMETHODIMP OnStateChange(IShellView* pShellView, ULONG uChange)
	{	//handle selection, rename, focus if needed
//#define caseTrace(what) case what: TRACE(L#what); break;
//		switch(uChange)
//		{
//		caseTrace(CDBOSC_SETFOCUS    )// The focus has been set to the view. 
//		caseTrace(CDBOSC_KILLFOCUS   )// The view has lost the focus.  
//		caseTrace(CDBOSC_SELCHANGE   )// The selected item has changed. 
//		caseTrace(CDBOSC_RENAME      )// An item has been renamed. 
//		caseTrace(CDBOSC_STATECHANGE )// An item has been checked or unchecked 
//		}
//#undef caseTrace
		return m_Owner.OnStateChange(pShellView, uChange); 
	}
	STDMETHODIMP IncludeObject(IShellView* pShellView, LPCITEMIDLIST pidl)
	{	// 表示ファイルのフィルタリング。
		// S_OK: show, S_FALSE: hide
		return m_Owner.IncludeObject(pShellView, pidl);
	}

public: // ICommDlgBrowser2
	STDMETHODIMP GetDefaultMenuText(IShellView*pshv, WCHAR* pszText, int cchMax)
	{	// デフォルトの選択項目は「選択」なのでほとんど意味がない
		return S_FALSE;
	}
	STDMETHODIMP GetViewFlags(DWORD *pdwFlags)
	{	// すべてのファイルを表示すべきか否かの問い合わせ。
		// CDB2GVF_SHOWALLFILES: 隠しファイルを表示する。
		// 0: 隠しファイルを表示しない。
		return m_Owner.GetViewFlags(pdwFlags);
	}
	STDMETHODIMP Notify(IShellView*pshv, DWORD dwNotifyType)
	{	// コンテキストメニューが始まったor終わった
		return S_OK;
	}

public: // IShellBrowser
	STDMETHODIMP InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)			{ return E_NOTIMPL; }
	STDMETHODIMP SetMenuSB(HMENU hmenuShared, HOLEMENU holemenuReserved,HWND hwndActiveObject)	{ return E_NOTIMPL; }
	STDMETHODIMP RemoveMenusSB(HMENU hmenuShared)												{ return E_NOTIMPL; }
	STDMETHODIMP SetStatusTextSB(LPCOLESTR lpszStatusText)										{ return E_NOTIMPL; }
	STDMETHODIMP EnableModelessSB(BOOL fEnable)													{ return E_NOTIMPL; }
	STDMETHODIMP SetToolbarItems(LPTBBUTTON lpButtons, UINT nButtons,UINT uFlags)				{ return E_NOTIMPL; }
	STDMETHODIMP TranslateAcceleratorSB(LPMSG lpmsg, WORD wID)									{ return E_NOTIMPL; }
	STDMETHODIMP OnViewWindowActive(IShellView* pShellView)										{ return E_NOTIMPL; }
	STDMETHODIMP GetViewStateStream(DWORD grfMode, IStream** ppStream)
	{
		return m_Owner.GetViewStateStream(grfMode, ppStream);
	}
	STDMETHODIMP GetControlWindow(UINT id, HWND* lphwnd)
	{
		if(!lphwnd)
			return E_POINTER;
		switch(id)
		{
		case FCW_STATUS:
			*lphwnd = m_Owner;
			return S_OK;
		}
		return E_NOTIMPL;
	}
	STDMETHODIMP SendControlMsg(UINT id, UINT uMsg, WPARAM wParam,LPARAM lParam, LRESULT *pret)
	{
		LRESULT lResult;
		switch(id)
		{
		case FCW_STATUS:
			lResult = m_Owner.SendMessage(uMsg, wParam, lParam);
			if(pret) *pret = lResult;
			break;
		}
		return E_NOTIMPL;
	}
	STDMETHODIMP QueryActiveShellView(IShellView** ppShellView)	{ return m_Owner.m_pShellView.copyto(ppShellView); }
	STDMETHODIMP BrowseObject(LPCITEMIDLIST pidl, UINT wFlags)	{ return E_NOTIMPL; }
};

//==============================================================================

Shell::Shell() : m_WallPaperAlign(DirNone), m_wndShell(&theDefViewMap), m_wndList(&theShellListMap), m_wndHeader(&theShellHeaderMap)
{
	m_FolderSettings.ViewMode = ListStyleDetails;
	m_FolderSettings.fFlags   = FWF_AUTOARRANGE | MEW_FWF_ALWAYS;
	m_AutoArrange   = !!(m_FolderSettings.fFlags & FWF_AUTOARRANGE);
	m_CheckBoxChangeEnabled = true;
	m_CheckBox      = false;
	m_FullRowSelect = false;
	m_GridLine      = false;
	m_ShowAllFiles  = false;
	m_RenameExtension = true;
	UpdateShellState();
}
Shell::~Shell()
{
	DisposeShellView();
}
void Shell::DisposeShellView() throw()
{
	SaveViewState();
	if(m_wndHeader)	m_wndHeader.UnsubclassWindow();
	if(m_wndList)	m_wndList.UnsubclassWindow();
	if(m_wndShell)	m_wndShell.UnsubclassWindow();
	if(m_pShellView)
	{
		IShellView* tmp = m_pShellView.detach();
		tmp->UIActivate(SVUIA_DEACTIVATE);
		tmp->DestroyViewWindow();
		tmp->Release();
	}
	m_pCurrentEntry.clear();
	m_pShellBrowser.clear();
}
HRESULT Shell::SaveViewState()
{
	if(!m_pShellView)
		return E_UNEXPECTED;
	m_pShellView->GetCurrentInfo(&m_FolderSettings);
	return m_pShellView->SaveViewState();
}
HRESULT Shell::UpdateShellState()
{
	SHGetSettings(&theShellFlagState, SSF_SHOWALLOBJECTS);
	m_ShowAllFiles = !!theShellFlagState.fShowAllObjects;
	return S_OK;
}
inline static MouseAndModifier WM_MOUSE_to_Button(UINT uMsg, WPARAM wParam)
{
	switch(uMsg)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
		return MouseButtonLeft;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
		return MouseButtonRight;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
		return MouseButtonMiddle;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_XBUTTONDBLCLK:
		switch(GET_XBUTTON_WPARAM(wParam))
		{
		case XBUTTON1:	return MouseButtonX1;
		case XBUTTON2:	return MouseButtonX2;
		default:		return ModifierNone;
		}
	default:
		return ModifierNone;
	}
}
BOOL Shell::ProcessShellMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	switch(uMsg)
	{
	case WM_NOTIFY:
		switch(((NMHDR*)lParam)->code)
		{
		case LVN_BEGINLABELEDIT:
			if(NMLVDISPINFO* info = (NMLVDISPINFO*)lParam)
			{
				ref<IEntry> entry;
				if SUCCEEDED(GetAt(&entry, info->item.iItem))
				{
					HWND hEdit = m_wndList.GetEditControl();
					ASSERT(hEdit);
					afx::Edit_SubclassSingleLineTextBox(hEdit, entry->Path.str(), theAvesta->EditOptions | afx::EditTypeFileName);
				}
			}
			return true;
		case LVN_ITEMCHANGING:
			if(NMLISTVIEW* info = (NMLISTVIEW*)lParam)
			{	// チェック状態の変化
				UINT uNewCheck = (info->uNewState & 0x3000);
				UINT uOldCheck = (info->uOldState & 0x3000);
				lResult = (!m_CheckBoxChangeEnabled && uNewCheck && uOldCheck && uNewCheck != uOldCheck);
			}
			return true;
		default:
			break;
		}
		break; // default.
	case WM_CREATE:
		m_pShellBrowser.attach(new ShellBrowser(this));
		break;
	case WM_DESTROY:
		DisposeShellView();
		break;
	case WM_FORWARDMSG:
		return lResult = PreTranslateMessage((MSG*)lParam);
	case WM_GETISHELLBROWSER:
		//AddRef() は必要ない。
		//SetWindowLongPtr(DWL_MSGRESULT, (LONG)(IShellBrowser*)this); //use this if dialog
		lResult = (LRESULT)(IShellBrowser*)m_pShellBrowser;
		return true;
	case WM_ENABLECHECKBOXCHANGE:
		lResult = false;
		if(lParam != 0xFFFFFFFF && m_pShellView && m_CheckBox && get_Style() == ListStyleTile)
		{	// TILE モードのときは、チェックボックスが正常に働かない
			LVHITTESTINFO hit = { GET_XY_LPARAM(lParam), 0, -1, 0 };
			m_CheckBoxChangeEnabled = false;
			int index = m_wndList.HitTest(&hit);
			if(index >= 0)
			{
				if(ref<IFolderView> pFolderView = cast(m_pShellView))
				{
					LPITEMIDLIST pidl = null;
					Point ptItem;
					if(SUCCEEDED(pFolderView->Item(index, &pidl)) && SUCCEEDED(pFolderView->GetItemPosition(pidl, &ptItem)))
					{
						Point ptTranslate;
						::GetCursorPos(&ptTranslate);
						UINT svsi = SVSI_TRANSLATEPT; // これだけだと、TRANSLATEPT のついでに選択状態を変化させてしまうため、
						svsi |= (m_wndList.GetItemState(index, LVIS_SELECTED) ? SVSI_SELECT : SVSI_DESELECT); // 選択状態を保存してやる。
						if SUCCEEDED(pFolderView->SelectAndPositionItems(1, const_cast<LPCITEMIDLIST*>(&pidl), &ptTranslate, svsi))
						{
							enum
							{
								XCENTER = 40,
								YCENTER = 10,
								RADIUS  =  8,
							};
							int dx = math::abs(ptTranslate.x - ptItem.x - XCENTER);
							int dy = math::abs(ptTranslate.y - ptItem.y - YCENTER);
							if(dx < RADIUS && dy < RADIUS)
							{	// onCheckBox
								m_CheckBoxChangeEnabled = true;
								m_wndList.SetCheckState(index, !m_wndList.GetCheckState(index));
								lResult = true; // cancel default action
							}
						}
					}
					ILFree(pidl);
				}
			}
		}
		else
		{
			m_CheckBoxChangeEnabled = (wParam != 0);
		}
		return true;
	case WM_SETFOCUS:
		if(m_wndShell)
			m_wndShell.SetFocus();
		return false;
	case WM_SETTINGCHANGE:
		UpdateShellState();
		break;
	case WM_MOUSEWHEEL:
		lResult = OnListWheel(wParam, lParam);
		return true;
	case LVM_SETVIEW:
		OnListViewModeChanged();
		return true;
	}
	return false;
}

HRESULT Shell::MimicKeyDown(UINT vkey, UINT mods)
{
	if(!m_pShellView)
		return E_UNEXPECTED;
	afx::SetModifierState(vkey, mods);
	MSG msg = { m_wndList, WM_KEYDOWN, vkey, static_cast<LPARAM>(MapVirtualKey(vkey, 0) | 1) };
	HRESULT hr = m_pShellView->TranslateAccelerator(&msg);
	if(hr == S_FALSE)
	{	// 処理されないので、ListViewにKEYDOWNを送ってエミュレート
		m_wndList.SendMessage(WM_KEYDOWN, vkey, 0);
	}
	// ダイアログが表示されるなどし、TranslateAccelerator()が瞬間的に終わらなかった場合に対処する。
	afx::RestoreModifierState(vkey);
	return hr;
}

HRESULT Shell::ReCreateViewWindow(IShellFolder* pShellFolder, bool reload)
{
	HRESULT hr;

	ASSERT(m_pShellBrowser);
	if(!m_pShellBrowser)
		return E_INVALIDARG;
	RECT rcView = { 0, 0, 0, 0 };
	if(m_wndShell)
	{
		m_wndShell.GetWindowRect(&rcView);
		ScreenToClient(&rcView);
	}

	bool hasFocus = m_wndShell.HasFocus();

	ASSERT(pShellFolder);
	ref<IShellView> pNewShellView;
	hr = pShellFolder->CreateViewObject(m_hWnd, IID_IShellView, (void**)&pNewShellView);
	if FAILED(hr)
	{
		TRACE(_T("error: CreateViewObject()"));
		return hr;
	}

	if(m_wndHeader)	m_wndHeader.UnsubclassWindow();
	if(m_wndList)	m_wndList.UnsubclassWindow();
	if(m_wndShell)	m_wndShell.UnsubclassWindow();

	m_pViewStateStream.clear();
	if(reload && SUCCEEDED(OnQueryStream(STGM_READ, &m_pViewStateStream)))
	{
		FOLDERSETTINGS settings;
		if SUCCEEDED(hr = m_pViewStateStream->Read(&settings, sizeof(FOLDERSETTINGS), NULL))
		{
			m_FolderSettings = settings;
			bitof(m_FolderSettings.fFlags, FWF_CHECKSELECT) = m_CheckBox;
		}
		else
		{
			return hr;
		}
	}
	HWND hwndShell = null;
	m_FolderSettings.fFlags |= MEW_FWF_ALWAYS;
	m_FolderSettings.fFlags &= ~MEW_FWF_ALWAYSNOT;
	if(ref<IShellView2> view2 = cast(pNewShellView))
	{
		SV2CVW2_PARAMS params = { sizeof(SV2CVW2_PARAMS), m_pShellView, &m_FolderSettings, m_pShellBrowser, &rcView };
		if SUCCEEDED(hr = view2->CreateViewWindow2(&params))
			hwndShell = params.hwndView;
	}
	else
	{
		hr = pNewShellView->CreateViewWindow(m_pShellView, &m_FolderSettings, m_pShellBrowser, &rcView, &hwndShell);
	}
	m_pViewStateStream.clear();
	if FAILED(hr)
	{
		TRACE(_T("error: ShellListView.ReCreateViewWindow(), CreateViewWindow()"));
		return hr;
	}
	// 作成に成功した。
	ASSERT(!m_wndShell);
	m_wndShell.SubclassWindow(hwndShell);
	if(m_pShellView)
	{
		m_pShellView->UIActivate(SVUIA_DEACTIVATE);
		m_pShellView->DestroyViewWindow();
		m_pShellView.clear();
	}
	m_pCurrentFolder = pShellFolder;
	m_pShellView = pNewShellView;
	m_pShellView->UIActivate(hasFocus ? SVUIA_ACTIVATE_FOCUS : SVUIA_ACTIVATE_NOFOCUS);

	// 通常は、一番目の子供がリストビューなのだが、フォントフォルダの表示時は異なる
	ASSERT(!m_wndList);
	if(HWND hListView = ::FindWindowEx(m_wndShell, NULL, _T("SysListView32"), NULL))
	{
		m_wndList.SubclassWindow(hListView);
		DWORD dwExStyle = 0;
		if(m_CheckBox)      dwExStyle |= LVS_EX_CHECKBOXES;
		if(m_FullRowSelect) dwExStyle |= LVS_EX_FULLROWSELECT;
		if(m_GridLine)      dwExStyle |= LVS_EX_GRIDLINES;
		m_wndList.SetExtendedListViewStyle(dwExStyle, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);
		ImmAssociateContext(m_wndList, null);
		if(HWND hHeader = m_wndList.GetHeader())
			m_wndHeader.SubclassWindow(hHeader);
		if(HFONT hFont = GetFont())
			m_wndList.SetFont(hFont);
	}

	m_Grouping = (m_wndList ? m_wndList.SendMessage(LVM_ISGROUPVIEWENABLED, 0, 0) != 0 : false);

	return S_OK;
}
HRESULT Shell::Refresh()
{
	if(!m_pShellView)
		return E_UNEXPECTED;
	int column = 0;
	bool ascending = true;
	GetSortKey(&column, &ascending);
	HRESULT hr = m_pShellView->Refresh();
	if FAILED(hr)
		return hr;
	SetSortKey(column, ascending);
	UpdateBackground();
	return S_OK;
}
HRESULT Shell::SetStatusByIndex(int index, Status status, bool unique)
{
	if(ref<IFolderView> folder = cast(m_pShellView))
	{
		switch(status)
		{
		case FOCUSED:
			ASSERT(!unique);
			return folder->SelectItem(index, SVSI_FOCUSED | SVSI_FOCUSED | SVSI_ENSUREVISIBLE);
		case SELECTED:
			if(unique)
				return folder->SelectItem(index, SVSI_DESELECTOTHERS | SVSI_FOCUSED | SVSI_ENSUREVISIBLE);
			else
				return folder->SelectItem(index, SVSI_SELECT | SVSI_FOCUSED | SVSI_ENSUREVISIBLE);
			break;
		case CHECKED:
			ASSERT(!unique);
			return folder->SelectItem(index, SVSI_CHECK | SVSI_FOCUSED | SVSI_ENSUREVISIBLE);
		case UNSELECTED:
			ASSERT(!unique);
			return folder->SelectItem(index, SVSI_DESELECT | SVSI_FOCUSED | SVSI_ENSUREVISIBLE);
		default:
			return E_NOTIMPL;
		}
	}
	else
	{
		ASSERT(0);
		return E_NOTIMPL;
	}
}
HRESULT Shell::SetStatusByUnknown(IUnknown* unk, Status status, bool unique)
{
	IShellFolder* folder = GetCurrentFolder();
	if(!folder)
		return E_UNEXPECTED;
	bool newLeaf = false;
	LPITEMIDLIST leaf = null;
	ref<IEntry> entry = cast(unk);
	if(entry)
	{
		leaf = ILFindLastID(entry->ID);
	}
	else if(string name = cast(unk))
	{
		newLeaf = SUCCEEDED(folder->ParseDisplayName(m_hWnd, NULL, const_cast<PWSTR>(name.str()), NULL, &leaf, NULL));
	}
	if(!leaf)
	{
		try
		{
			ASSERT(!entry);
			entry.create(__uuidof(Entry), unk);
			leaf = ILFindLastID(entry->ID);
		}
		catch(Error& e)
		{
			TRACE(e.Message);
			return e.Code;
		}
	}
	ASSERT(leaf);
	HRESULT hr = SetStatusByIDList(leaf, status, unique);
	if(newLeaf)
		ILFree(leaf);
	return hr;
}
HRESULT Shell::SetStatusByIDList(LPCITEMIDLIST leaf, Status status, bool unique)
{
	if(!leaf)
		return E_POINTER;
	switch(status)
	{
	case SELECTED:
		if(unique)
			SelectNone();
		return Select(leaf, SVSI_SELECT | SVSI_FOCUSED | SVSI_ENSUREVISIBLE);
	default:
		return E_INVALIDARG;
	}
}
HRESULT Shell::Select(LPCITEMIDLIST leaf, UINT svsi)
{
	if(!m_pShellView)
		return E_UNEXPECTED;
	return m_pShellView->SelectItem(leaf, svsi);
}
HRESULT Shell::SelectNone()
{
	return m_pShellView ? m_pShellView->SelectItem(NULL, SVSI_DESELECTOTHERS) : E_FAIL;
}
HRESULT Shell::SelectAll()
{
	if(!m_wndList.IsWindow())
		return E_UNEXPECTED;
	m_wndList.SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED); // -1 で全選択
	return S_OK;
}
HRESULT Shell::CheckAll(bool check)
{
	if(!m_CheckBox)
		return S_FALSE;
	if(!m_wndList.IsWindow())
		return E_UNEXPECTED;
	m_CheckBoxChangeEnabled = true;
	m_wndList.SetCheckState(-1, check); // -1 で全選択
	return S_OK;
}
HRESULT Shell::SelectChecked()
{
	if(!m_CheckBox)
		return S_FALSE;
#if 0 // XP では SVGIO_CHECKED は取得できないっぽい
	ref<IEntryList> entries;
	HRESULT hr;
	if FAILED(hr = GetContents(&entries, CHECKED))
		return hr;
	int count = entries->Count;
	SelectNone();
	for(int i = 0; i < count; i++)
	{
		m_pShellView->SelectItem(entries->Leaf[i], SVSI_SELECT | SVSI_CHECK);
	}
#else // 仕方ないので、リストビューを参照しながらチェックされたアイテムを選択する
	if(!m_wndList.IsWindow())
		return E_UNEXPECTED;
	int count = m_wndList.GetItemCount();
	int focus = m_wndList.GetNextItem(-1, LVNI_FOCUSED);
	int focusNext = INT_MAX;
	for(int i = 0; i < count; i++)
	{
		if(m_wndList.GetCheckState(i))
		{
			m_wndList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			if(math::abs(focusNext-focus) > math::abs(i-focus))
				focusNext = i;
		}
		else
		{
			m_wndList.SetItemState(i, 0, LVIS_SELECTED);
		}
	}
	// 以前のフォーカスに最も近い、選択中の項目をフォーカスする。
	if(focusNext < count)
	{
		m_wndList.SetItemState(focusNext, LVIS_FOCUSED, LVIS_FOCUSED);
		m_wndList.EnsureVisible(focusNext, false);
	}
#endif
	return S_OK;
}
HRESULT Shell::GetAt(REFINTF pp, size_t index)
{
	if(ref<IFolderView> folder = cast(m_pShellView))
	{
		LPITEMIDLIST pidl;
		HRESULT hr = folder->Item(index, &pidl);
		if FAILED(hr)
			return hr;
		hr = m_pCurrentEntry->QueryObject(pp, pidl);
		ILFree(pidl);
		return hr;
	}
	return E_NOTIMPL;
}
HRESULT Shell::GetContents(REFINTF ppInterface, Status option)
{
	int svgio = 0;
	switch(option)
	{
	case StatusNone:
		svgio = SVGIO_ALLVIEW | SVGIO_FLAG_VIEWORDER;
		break;
	case FOCUSED:
		svgio = SVGIO_SELECTION;
		break;
	case SELECTED:
		svgio = SVGIO_SELECTION | SVGIO_FLAG_VIEWORDER;
		break;
	case CHECKED:
		svgio = SVGIO_CHECKED | SVGIO_FLAG_VIEWORDER;
		break;
	default:
		TRESPASS();
	}
	if SUCCEEDED(GetContents_FolderView(ppInterface, svgio))
		return S_OK;
	else
		return GetContents_ShellView(ppInterface, svgio);
}
HRESULT Shell::GetContents_FolderView(REFINTF ppInterface, int svgio)
{
	if(ref<IFolderView> folder = cast(m_pShellView))
	{
		ref<IDataObject> pDataObject;
		HRESULT hr = folder->Items(svgio, IID_IDataObject, (void**)&pDataObject);
		if FAILED(hr)
			return hr;
		try
		{
			ref<IEntryList> entries(__uuidof(EntryList), pDataObject);
			return entries->QueryInterface(ppInterface);
		}
		catch(Error& e)
		{
			return e.Code;
		}
	}
	return E_FAIL;
}
HRESULT Shell::GetContents_ShellView(REFINTF ppInterface, int svgio)
{
	HRESULT hr;
	if(!m_pShellView)
		return E_UNEXPECTED;
	// 特別処理
	if(svgio == (SVGIO_SELECTION | SVGIO_FLAG_VIEWORDER))
		return GetContents_Select(ppInterface);
	if(svgio == (SVGIO_CHECKED | SVGIO_FLAG_VIEWORDER))
		return GetContents_Checked(ppInterface);

	ref<IDataObject> pDataObject;
	if FAILED(hr = m_pShellView->GetItemObject(svgio, IID_IDataObject, (void**)&pDataObject))
		return hr;
	try
	{
		ref<IEntryList> entries(__uuidof(EntryList), pDataObject);
		return entries->QueryInterface(ppInterface);
	}
	catch(Error& e)
	{
		return e.Code;
	}
}
HRESULT Shell::GetContents_Select(REFINTF ppInterface)
{
	if(!m_wndList.IsWindow())
		return E_UNEXPECTED;
	// IShellView::Item(SVGIO_SELECTION | SVGIO_FLAG_VIEWORDER) は、順番が狂ってしまう。
	// 仕方ないので、全部取得し、リストビューを調べて再構築する.
	HRESULT hr;
	ref<IEntryList> entries;
	if FAILED(hr = GetContents(&entries, StatusNone))
		return hr;
	ASSERT(entries->Count == (size_t)m_wndList.GetItemCount());
	int index = -1;
	std::vector<size_t> subset;
	while(-1 != (index = m_wndList.GetNextItem(index, LVNI_SELECTED)))
	{
		subset.push_back(index);
	}
	if(subset.empty())
		return E_FAIL;
	return entries->CloneSubset(ppInterface, &subset[0], subset.size());
}
HRESULT Shell::GetContents_Checked(REFINTF ppInterface)
{
	if(!m_wndList.IsWindow())
		return E_UNEXPECTED;
	HRESULT hr;
	ref<IEntryList> entries;
	if FAILED(hr = GetContents(&entries, StatusNone))
		return hr;
	ASSERT(entries->Count == (size_t)m_wndList.GetItemCount());
	std::vector<size_t> subset;
	int count = m_wndList.GetItemCount();
	for(int i = 0; i < count; i++)
	{
		if(m_wndList.GetCheckState(i))
			subset.push_back(i);
	}
	if(subset.empty())
		return E_FAIL;
	return entries->CloneSubset(ppInterface, &subset[0], subset.size());
}
HRESULT Shell::GoUp(size_t step, bool selectPrev)
{
	if(!m_pCurrentEntry)
		return E_FAIL;
	if(step == 0)
		return !afx::ILIsRoot(m_pCurrentEntry->ID);
	HRESULT hr;
	ref<IEntry> current = m_pCurrentEntry;
	ref<IEntry> parent;
	if FAILED(hr = m_pCurrentEntry->QueryObject(&parent, step))
		return hr;
	if FAILED(hr = GoAbsolute(parent, GoNew))
		return hr;
	// 以前いたフォルダにカーソルを合わせる.
	if(selectPrev)
	{
		if(LPITEMIDLIST prev = ILCloneFirst((LPITEMIDLIST)(((BYTE*)current->ID) + ILGetSize(parent->ID) - 2)))
		{
			Select(prev, SVSI_FOCUSED | SVSI_SELECT | SVSI_ENSUREVISIBLE);
			ILFree(prev);
		}
	}
	return S_OK;
}
// parent 相対の child のIDリストを返す. 親子関係が間違っている場合の動作は未定義. 
static LPCITEMIDLIST ILFindRelative(LPCITEMIDLIST parent, LPCITEMIDLIST child)
{
	size_t size = ILGetSize(parent) - 2; // ターミネータの分だけ-2する
	return (LPCITEMIDLIST)(((BYTE*)child) + size);
}
static LPITEMIDLIST ILCreateChild(LPCITEMIDLIST parent, LPCITEMIDLIST child)
{
	return ILCloneFirst(ILFindRelative(parent, child));
}
HRESULT Shell::GoBack(size_t step, bool selectPrev)
{
	if(!m_pCurrentEntry)
		return E_FAIL;
	if(step == 0)
		return m_History.BackLength();
	HRESULT hr;
	ref<IEntry> prev = m_pCurrentEntry;
	ref<IEntry> next = m_History.GetRelativeHistory(-(int)step);
	if(!next || next->Equals(prev))
		return E_FAIL;
	if FAILED(hr = GoAbsolute(next, GoAgain))
		return hr;
	m_History.Back(step);
	// 以前いたフォルダにカーソルを合わせる.
	if(selectPrev && ILIsParent(next->ID, prev->ID, false))
	{
		LPITEMIDLIST child = ILCreateChild(next->ID, prev->ID);
		Select(child, SVSI_FOCUSED | SVSI_SELECT | SVSI_ENSUREVISIBLE);
		ILFree(child);
	}
	return hr;
}
HRESULT Shell::GoForward(size_t step, bool selectPrev)
{
	if(!m_pCurrentEntry)
		return E_FAIL;
	if(step == 0)
		return m_History.ForwardLength();
	HRESULT hr;
	ref<IEntry> prev = m_pCurrentEntry;
	ref<IEntry> next = m_History.GetRelativeHistory(step);
	if(!next || next->Equals(prev))
		return E_FAIL;
	if FAILED(hr = GoAbsolute(next, GoAgain))
		return hr;
	m_History.Forward(step);
	// 以前いたフォルダにカーソルを合わせる.
	if(selectPrev && ILIsParent(next->ID, prev->ID, false))
	{
		LPITEMIDLIST child = ILCreateChild(next->ID, prev->ID);
		Select(child, SVSI_FOCUSED | SVSI_SELECT | SVSI_ENSUREVISIBLE);
		ILFree(child);
	}
	return hr;
}
HRESULT Shell::GoAbsolute(IEntry* path, GoType go)
{
	HRESULT hr;
	if(!path)
		return E_INVALIDARG;

	// 「フォルダを別ウィンドウで開く」という動作の際に現在の設定が反映されないため、
	// OnDirectoryChanging() の前に呼ぶように変更した。
	SaveViewState();

	if(!OnDirectoryChanging(path, go))
		return E_ABORT;

	ref<IEntry> resolved;
	path->GetLinked(&resolved);
	if(!path->Exists())
	{
		TRACE(_T("error: OpenFolder(not-existing)"));
		return STG_E_FILENOTFOUND;
	}
	if(!resolved->IsFolder())
	{
		ref<IEntry> parentEntry;
		if(SUCCEEDED(hr = resolved->GetParent(&parentEntry))
		&& SUCCEEDED(hr = GoAbsolute(parentEntry, GoNew)))
		{
			SetStatusByIDList(ILFindLastID(resolved->ID), SELECTED);
			return S_OK;
		}
		else
		{
			return hr;
		}
	}

	ref<IShellFolder> pShellFolder;
	if(FAILED(hr = resolved->QueryObject(&pShellFolder)))
	{
		TRACE(_T("error: ShellListView.GoAbsolute(), GetSelfFolder()"));
		// 仕方ないのでデスクトップを。
		if FAILED(hr = SHGetDesktopFolder(&pShellFolder))
			return hr;
	}

	if(m_pCurrentEntry && m_pCurrentEntry->Equals(resolved))
	{	// 同じフォルダへの移動
		return S_OK;
	}

	ref<IEntry> pPrevEntry = m_pCurrentEntry;
	m_pCurrentEntry = resolved;
	if FAILED(hr = ReCreateViewWindow(pShellFolder, true))
	{	// 失敗した場合に元に戻す
		m_pCurrentEntry = pPrevEntry;
		return hr;
	}

	switch(go)
	{
	case GoNew:
		m_History.Add(resolved);
		break;
	case GoAgain:
		break;
	case GoReplace:
		m_History.Replace(resolved);
		break;
	default:
		TRESPASS();
	}
	OnDirectoryChanged(resolved, go);
	OnListViewModeChanged();

	// 先頭のフォルダ/ファイルにフォーカスを合わせる.
	each<IEntry> i = resolved->EnumChildren(true);
	if(i.next()){
		LPITEMIDLIST child = ILCreateChild(resolved->ID, i->ID);
		Select(child, SVSI_FOCUSED | SVSI_ENSUREVISIBLE);
		ILFree(child);
	}

  m_wndList.SetRedraw(true);

	return S_OK;
}
HRESULT Shell::EndContextMenu(IContextMenu* pMenu, UINT cmd)
{
	WCHAR text[MAX_PATH];
	HRESULT hr = afx::SHEndContextMenu(pMenu, cmd, m_hWnd, text);
	if SUCCEEDED(hr)
	{
		if(str::equals_nocase(text, L"グループで表示(&G)"))
		{
			TRACE(L"INVOKE COMMAND : グループで表示(&G)");
			m_Grouping = !m_Grouping;
		}
	}
	return hr;
}
void Shell::OnDefaultDirectoryChange(IEntry* folder, IShellFolder* pShellFolder)
{
	if SUCCEEDED(GoAbsolute(folder))
	{
		if(IsKeyPressed(VK_RETURN))
		{
			LVITEM item = { LVIF_STATE, 0 };
			item.state = item.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
			m_wndList.SetItem( &item);
			m_wndList.EnsureVisible(0, false);
		}
		else
		{	// ステータスバーに何も表示されなくなるので、ダミーのメッセージを送る
			SendMessage(SB_SETTEXT, 0, 0);
		}
	}
}
HRESULT Shell::GetViewFlags(DWORD* dw)
{
	if(m_ShowAllFiles)
		*dw = CDB2GVF_SHOWALLFILES;
	else
		*dw = 0;
	return S_OK;
}

HRESULT Shell::IncludeObject(IShellView* pShellView, LPCITEMIDLIST pidl)
{
	// 隠しファイルを隠すは、GetViewFlags()だけでは効果が無いため、ここで個別にフィルタリングする
	if(!m_pCurrentFolder || !m_pCurrentEntry)
		return S_OK;
	// フィルタ.
	if(m_PatternMask)
	{
		WCHAR name[MAX_PATH];
		if SUCCEEDED(afx::ILGetDisplayName(m_pCurrentFolder, pidl, SHGDN_NORMAL | SHGDN_INFOLDER, name, MAX_PATH))
		{
			if(!afx::PatternEquals(m_PatternMask.str(), name))
			{
				SFGAOF flags = SFGAO_FOLDER;
				if SUCCEEDED(m_pCurrentFolder->GetAttributesOf(1, &pidl, &flags))
				{	// パターンがマッチしなかった、非フォルダのみ隠す。
					if((flags & SFGAO_FOLDER) == 0)
						return S_FALSE;
				}
			}
		}
	}
	if(!theShellFlagState.fShowAllObjects)
		return S_OK; // すでにフィルタリング済み
	if(m_ShowAllFiles)
		return S_OK; // 全部表示するから
	SFGAOF flags = SFGAO_GHOSTED;
	if SUCCEEDED(m_pCurrentFolder->GetAttributesOf(1, &pidl, &flags))
	{
		return (flags & SFGAO_GHOSTED) ? S_FALSE : S_OK;
	}
	else
	{	// なぜか失敗することがある。仕方ないので、Win32API経由で。
		HRESULT result = S_OK;
		// SHGetRealIDL(m_pCurrentFolder, pidl, &fullpath)　も失敗するので、自前で連結することにする.
		LPITEMIDLIST fullpath = ILCombine(m_pCurrentEntry->ID, pidl);
		TCHAR path[MAX_PATH];
		if SUCCEEDED(afx::ILGetPath(fullpath, path))
		{
			DWORD dwAttrs = GetFileAttributes(path);
			if(dwAttrs != (DWORD)-1 && (dwAttrs & FILE_ATTRIBUTE_HIDDEN))
				result = S_FALSE;
		}
		ILFree(fullpath);
		return result;
	}
}
HRESULT Shell::GetViewStateStream(DWORD grfMode, IStream** ppStream)
{
	if(IEntry* folder = GetCurrentEntry())
	{
		if(grfMode & STGM_WRITE)
		{
			HRESULT hr = OnQueryStream(grfMode, ppStream);
			if FAILED(hr)
				return hr;
			(*ppStream)->Write(&m_FolderSettings, sizeof(FOLDERSETTINGS), NULL);
			return S_OK;
		}
		else if(m_pViewStateStream)
		{
			return m_pViewStateStream->QueryInterface(ppStream);
		}
	}
	return E_FAIL;
}
HRESULT Shell::UpdateStyle()
{
	m_FolderSettings.fFlags = MEW_FWF_ALWAYS;
	if(m_AutoArrange)	m_FolderSettings.fFlags |= FWF_AUTOARRANGE;
	if(m_CheckBox)		m_FolderSettings.fFlags |= FWF_CHECKSELECT;
	if(!m_pCurrentFolder)
		return S_FALSE;
	int column = 0;
	bool ascending = true;
	GetSortKey(&column, &ascending);
	HRESULT hr = ReCreateViewWindow(m_pCurrentFolder, false);
	if FAILED(hr)
		return hr;
	SetSortKey(column, ascending);
	return S_OK;
}
ListStyle Shell::get_Style() const
{
	if(m_pShellView)
		m_pShellView->GetCurrentInfo(const_cast<FOLDERSETTINGS*>(&m_FolderSettings));
	return (ListStyle)m_FolderSettings.ViewMode;
}
HRESULT Shell::set_Style(ListStyle style)
{
	if(style < 1 || 7 < style)
	{
		ASSERT(!"error: Invalid ListStyle");
		return E_INVALIDARG;
	}
	m_FolderSettings.ViewMode = style;
	if(ref<IFolderView> folder = cast(m_pShellView))
		return folder->SetCurrentViewMode(m_FolderSettings.ViewMode);
	else
		return UpdateStyle(); // LVS_* でスタイルを設定すると誤動作する。仕方ないので作り直す。
}
bool Shell::get_AutoArrange() const
{
	Shell* pThis = const_cast<Shell*>(this);
	if(m_pShellView)
		m_pShellView->GetCurrentInfo(&pThis->m_FolderSettings);
	pThis->m_AutoArrange = !!(m_FolderSettings.fFlags & FWF_AUTOARRANGE);
  #if defined(USE_ADHOC_FIX_FOR_SHELLBROWSER_LISTVIEW_DETAILS)
   if (get_Style() == ListStyle::ListStyleDetails) {
    ::SetTimer(m_wndList, 256, 1, setViewModeDetailsTimerProc);
   }
  #endif
	return pThis->m_AutoArrange;
}
HRESULT Shell::set_AutoArrange(bool value)
{
	if(get_AutoArrange() == value)
		return S_OK;
	m_AutoArrange = value;
	return UpdateStyle();
}
HRESULT Shell::set_CheckBox(bool value)
{
	if(m_CheckBox == value)
		return S_OK;
	m_CheckBox = value;
	// FOLDERSETTINGS でやらなくても一応大丈夫そう
	if(m_wndList)
		m_wndList.SetExtendedListViewStyle(m_CheckBox ? LVS_EX_CHECKBOXES : 0, LVS_EX_CHECKBOXES);
	return S_OK;
}
HRESULT Shell::set_FullRowSelect(bool value)
{
	if(m_FullRowSelect == value)
		return S_OK;
	m_FullRowSelect = value;
	if(m_wndList)
		m_wndList.SetExtendedListViewStyle(m_FullRowSelect ? LVS_EX_FULLROWSELECT : 0, LVS_EX_FULLROWSELECT);
	return S_OK;
}
HRESULT Shell::set_GridLine(bool value)
{
	if(m_GridLine == value)
		return S_OK;
	m_GridLine = value;
	if(m_wndList)
		m_wndList.SetExtendedListViewStyle(m_GridLine ? LVS_EX_GRIDLINES : 0, LVS_EX_GRIDLINES);
	return S_OK;
}
HRESULT Shell::set_Grouping(bool value)
{
	if(!m_pShellView)
		return E_UNEXPECTED;
	if(m_Grouping == value)
		return S_OK;
	m_Grouping = afx::ExpEnableGroup(m_pShellView, value);
	return S_OK;
}
HRESULT Shell::set_ShowAllFiles(bool value)
{
	if(m_ShowAllFiles == value)
		return S_OK;
	m_ShowAllFiles = value;
	return UpdateStyle();
}
LRESULT Shell::DefaultContextMenu(WPARAM wParam, LPARAM lParam)
{
	// 無限ループに陥るの避けるため、サブクラス化されている場合のみ。
	if(m_wndShell.m_pfnSuperWindowProc)
		return m_wndShell.DefWindowProc(WM_CONTEXTMENU, wParam, lParam);
	return 0;
}

bool Shell::PreTranslateMessage(MSG* msg)
{
	if(!m_wndShell || !m_pShellView)
	{	// 準備が出来ていない
		return false;
	}
	if(m_wndShell != msg->hwnd && !m_wndShell.IsChild(msg->hwnd))
	{	// シェルビューまたはその子供でなければ処理しない
		return false;
	}
	else if(m_wndList.IsChild(msg->hwnd))
	{	// リストビューの子供＝ファイルリネーム中のエディットなので処理を任せる
		if(m_pShellView->TranslateAccelerator(msg) == S_OK)
			return true;
		else
			return false;
	}
	else
	{	// デフォルトでは、喰ってしまうのに処理しないキーアクセラレータを処理する.
		switch(msg->message)
		{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			return false;
		}
		return m_pShellView->TranslateAccelerator(msg) == S_OK;
	}
}
void Shell::set_WallPaperFile(string value)
{
	if(m_WallPaperFile == value)
		return;
	m_WallPaperFile = value;
	UpdateBackground();
}
void Shell::set_WallPaperAlign(UINT32 value)
{
	if(m_WallPaperAlign == value)
		return;
	m_WallPaperAlign = value;
	UpdateBackground();
}
void Shell::UpdateBackground()
{
	// どうやら、実質的にタイリングと左上以外は使い物にならないようだ
	if(!m_wndList.IsWindow())
		return;

	LVBKIMAGE bkgnd = { 0 };
	if(!m_WallPaperFile)
	{
		bkgnd.ulFlags |= LVBKIF_SOURCE_NONE;
	}
	else
	{
		bkgnd.ulFlags |= LVBKIF_SOURCE_URL;
		bkgnd.pszImage = const_cast<PTSTR>(m_WallPaperFile.str());
		if(m_WallPaperAlign == DirNone)
		{	// タイリング
			bkgnd.ulFlags |= LVBKIF_STYLE_TILE;
		}
		else
		{
			bkgnd.ulFlags |= LVBKIF_STYLE_NORMAL;
		}
	}
	m_wndList.SetBkImage(&bkgnd);
}

namespace
{
	const int MAX_VIEW_MODE = 4;

	const int DEFAULT_ICON_SIZE[MAX_VIEW_MODE] = { 48, 32, 16, 16 };
	const PCWSTR PROFILE_ICON_NAME[MAX_VIEW_MODE] =
	{
		L"FolderTile",
		L"FolderIcon",
		L"FolderList",
		L"FolderDetails",
	};
	static int theIconSize[MAX_VIEW_MODE] = { 0 };

	int GetIconSize(ListStyle style)
	{
		int index;
		switch(style)
		{
		case ListStyleIcon:			index = 1; break;
		case ListStyleSmallIcon:	index = 2; break;
		case ListStyleList:			index = 2; break;
		case ListStyleDetails:		index = 3; break;
		//case ListStyleThumnail:
		case ListStyleTile:			index = 0; break;
		default:					return 0;
		}

		if(theIconSize[0] == 0)
		{
			for(int i = 0; i < MAX_VIEW_MODE; ++i)
				theIconSize[i] = theAvesta->GetProfileSint32(L"Style", PROFILE_ICON_NAME[i], DEFAULT_ICON_SIZE[i]);
		}
		if(theIconSize[index] == DEFAULT_ICON_SIZE[index])
			return 0; // 変更する必要なし。
		return theIconSize[index];
	}
}

void Shell::OnListViewModeChanged()
{
	if(!m_wndList.IsWindow())
		return;

	m_wndList.SetRedraw(false);

	int size = GetIconSize(get_Style());
	if(size > 0)
	{
		ref<IImageList> imagelist;
		afx::ExpGetImageList(size, &imagelist);
		HIMAGELIST hImageList = (HIMAGELIST)imagelist.detach(); 
		m_wndList.SetImageList(hImageList, LVSIL_SMALL);
		m_wndList.SetImageList(hImageList, LVSIL_NORMAL);
	}

	UpdateBackground();

	m_wndList.SetRedraw(true);
}

//==============================================================================

} } // namespace

