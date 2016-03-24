// afx.hpp
#pragma once

__interface IShellFolder;
__interface IShellView;
struct _IDA;
typedef _IDA CIDA;
struct __declspec(uuid("000214F1-0000-0000-C000-000000000046")) ICommDlgBrowser;
struct __declspec(uuid("10339516-2894-11d2-9039-00C04F8EEB3E")) ICommDlgBrowser2;
struct __declspec(uuid("000214E4-0000-0000-C000-000000000046")) IContextMenu;
struct __declspec(uuid("000214F4-0000-0000-C000-000000000046")) IContextMenu2;
struct __declspec(uuid("BCFCE0A0-EC17-11d0-8D10-00A0C90F2719")) IContextMenu3;

#include "struct.hpp"
#include "string.hpp"

#include "avesta.hpp"

/// Windows拡張.
namespace afx
{
	//==============================================================================
	// ウィンドウ.

	bool PumpMessage();
	/// マウスボタンが押されていても、ツールチップを表示する.
	HRESULT TipRelayEvent(HWND hTip, HWND hOwner, int xScreen, int yScreen);
	HRESULT MimicDoubleClick(int x, int y);
	HRESULT PrintWindow(HWND hwnd, HDC hdcBlt, UINT nFlags);
	inline void MapWindowRect(HWND hwndFrom, HWND hwndTo, RECT* rc)
	{
		::MapWindowPoints(hwndFrom, hwndTo, (POINT*)rc, 2);
	}
	inline HWND GetParentOrOwner(HWND hWnd)
	{
		if(HWND hParent = ::GetParent(hWnd))
			return hParent;
		else
			return ::GetWindow(hWnd, GW_OWNER);
	}
	void SetModifierState(UINT vkey, DWORD modifires);
	void RestoreModifierState(UINT vkey);

	inline bool IsMenu(HWND hwnd)
	{
		TCHAR szClassName[7];
		::GetClassName(hwnd, szClassName, 7);
		return lstrcmp(_T("#32768"), szClassName) == 0;
	}

	inline void ScreenToClient(HWND hwnd, RECT* rc)
	{
		POINT* p = (POINT*)rc;
		::ScreenToClient(hwnd, p+0);
		::ScreenToClient(hwnd, p+1);
	}

	void BroadcastMessage(UINT uMsg, DWORD dwThreadID = ::GetCurrentThreadId());

	inline mew::string GetName(HWND hwnd)
	{
		int length = ::GetWindowTextLength(hwnd);
		TCHAR* name = (TCHAR*)alloca((length+1) * sizeof(TCHAR));
		int done = ::GetWindowText(hwnd, name, length+1);
		return mew::string(name, done);
	}
	inline void SetName(HWND hwnd, const mew::string& value)
	{
		::SetWindowText(hwnd, value.str());
	}
	inline mew::Rect GetBounds(HWND hwnd)
	{
		if(::IsIconic(hwnd))
		{
			WINDOWPLACEMENT place = { sizeof(WINDOWPLACEMENT) };
			::GetWindowPlacement(hwnd, &place);
			return (mew::Rect&)place.rcNormalPosition;
		}
		else
		{
			mew::Rect rc;
			::GetWindowRect(hwnd, &rc);
			if(HWND parent = ::GetAncestor(hwnd, GA_PARENT))
			{
				::ScreenToClient(parent,     (POINT*)&rc);
				::ScreenToClient(parent, 1 + (POINT*)&rc);
			}
			return rc;
		}
	}
	inline void SetBounds(HWND hwnd, const mew::Rect& value)
	{
		::MoveWindow(hwnd, value.x, value.y, value.w, value.h, true);
	}
	inline mew::Point GetLocation(HWND hwnd)
	{
		return GetBounds(hwnd).location;
	}
	inline void SetLocation(HWND hwnd, const mew::Point& value)
	{
		::SetWindowPos(hwnd, NULL, value.x, value.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
	inline mew::Rect GetClientArea(HWND hwnd)
	{
		mew::Rect rc;
		::GetClientRect(hwnd, &rc);
		return rc;
	}
	inline mew::Size GetClientSize(HWND hwnd)
	{
		mew::Rect rc;
		::GetClientRect(hwnd, &rc);
		ASSERT(rc.left == 0);
		ASSERT(rc.top  == 0);
		return rc.size;
	}
	inline void SetClientSize(HWND hwnd, mew::Size value)
	{
		RECT rc;
		::GetClientRect(hwnd, &rc);
		if(value.w != -1) rc.right = value.w;
		if(value.h != -1) rc.bottom = value.h;
		DWORD dwStyle = ::GetWindowLong(hwnd, GWL_STYLE);
		DWORD dwExStyle = ::GetWindowLong(hwnd, GWL_EXSTYLE);
		::AdjustWindowRectEx(&rc, dwStyle, (!(dwStyle & WS_CHILD) && (GetMenu(hwnd) != NULL)), dwExStyle);
		::SetWindowPos(hwnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOMOVE);
	}
	inline bool GetVisible(HWND hwnd)
	{
		return (::GetWindowLong(hwnd, GWL_STYLE) & WS_VISIBLE) != 0;
	}
	inline void SetVisible(HWND hwnd, bool value)
	{
		if(GetVisible(hwnd) != value)
		{
			::ShowWindow(hwnd, value ? SW_SHOW : SW_HIDE);
		}
	}

	//==============================================================================
	// エディット.

	enum SingleLineEditOptions
	{
		KeybindNormal		= 0x0000,
		KeybindEmacs		= 0x0001,
		KeybindAtok			= 0x0002,
		KeybindMask			= 0x000F,
		EditTypeNone		= 0x0000,
		EditTypeFileName	= 0x0010,
		EditTypeFullPath	= 0x0020,
		EditTypeMultiName	= 0x0030,
		EditTypeMask		= 0x00F0,
		RenameExtension		= 0x0100,
	};

	HRESULT Edit_SubclassSingleLineTextBox(HWND hwndEdit, LPCWSTR fullpath, DWORD options);

	//==============================================================================
	// リストビュー.

	HRESULT ListView_GetSortKey(HWND hwndListView, int* column, bool* ascending);
	HRESULT ListView_SetSortKey(HWND hwndListView, int  column, bool  ascending);
	HRESULT ListView_AdjustToWindow(HWND hwndListView);
	HRESULT ListView_AdjustToItems(HWND hwndListView);
	HRESULT ListView_SelectReverse(HWND hwndListView);

	//==============================================================================
	// コンボボックス.

	inline HWND ComboBoxEx_GetEdit(HWND hwndComboBoxEx)
	{
		return (HWND)::SendMessage(hwndComboBoxEx, CBEM_GETEDITCONTROL, 0, 0);
	}
	inline HWND ComboBoxEx_GetCombo(HWND hwndComboBoxEx)
	{
		return (HWND)::SendMessage(hwndComboBoxEx, CBEM_GETCOMBOCONTROL, 0, 0);
	}
	bool ComboBoxEx_IsAutoCompleting(HWND hwndComboBoxEx);

	//==============================================================================
	// パス.

	bool PathIsRegistory(PCWSTR name);
	bool PathIsFolder(PCWSTR path);
	HRESULT PathNormalize(WCHAR dst[MAX_PATH], PCWSTR src);

	//==============================================================================
	// シェルファイル処理.

	HICON ExtractIcon(LPCTSTR filename, int index, int w, int h);
	HRESULT SHResolveLink(PCWSTR shortcut, WCHAR resolved[MAX_PATH]);

	//==============================================================================
	// シェルメニュー.

	HMENU	SHBeginContextMenu(IContextMenu* pMenu);
	HMENU	SHBeginContextMenu(IShellView* pView, UINT svgio, IContextMenu** ppMenu);
	UINT	SHPopupContextMenu(IContextMenu* pMenu, HMENU hMenu, POINT ptScreen);
	HRESULT	SHEndContextMenu(IContextMenu* pMenu, int command, HWND hwnd, PWSTR verb = NULL);

	//==============================================================================
	// クリップボード.

	/// クリップボードにあるテキストを取得する.
	mew::string GetClipboardText(HWND hwnd = NULL);
	///
	bool SetClipboard(HGLOBAL hMem, int format, HWND hWnd = NULL);

	//==============================================================================
	// エクスプローラ関連.

	HANDLE	SHAllocShared(LPVOID data, ULONG size, DWORD pid);
	LPVOID	SHLockShared(HANDLE hData, DWORD dwOtherProcId);
	BOOL	SHUnlockShared(LPVOID lpvData);
	BOOL	SHFreeShared(HANDLE hData, DWORD dwSourceProcId);
	HIMAGELIST ExpGetImageList(int size);
	HRESULT ExpGetImageList(int size, IImageList** ppImageList);
	int		ExpGetImageIndex(LPCTSTR path);
	int		ExpGetImageIndex(const ITEMIDLIST* pidl);
	int		ExpGetImageIndex(int csidl);
	int		ExpGetImageIndexForFolder();
	HRESULT ExpEnumExplorers(HRESULT (*fnEnum)(HWND hExplorer, const ITEMIDLIST* pidl, LPARAM lParam), LPARAM lParam);
	bool    ExpEnableGroup(IShellView* pShellView, bool enable);
	DWORD   ExpGetThumbnailSize();
	HRESULT ExpSetThumbnailSize(DWORD dwSize);
	HRESULT ExpGetNavigateSound(WCHAR path[MAX_PATH]);

	//==============================================================================
	// ITEMIDLIST 関数.

	// ITEMIDLIST* ILCreateFromPathW(PCWSTR pwszPath);
	// void ILFree(ITEMIDLIST* pidl);

	// UINT ILGetSize(const ITEMIDLIST* pidl);
	// BOOL ILIsEqual(const ITEMIDLIST* pidl1, const ITEMIDLIST* pidl2);
	// BOOL ILIsParent(const ITEMIDLIST* pidlParent, const ITEMIDLIST* pidlBelow, BOOL fImmediate);
	// ITEMIDLIST* ILGetNext(const ITEMIDLIST* pidl);
	// ITEMIDLIST* ILFindChild(const ITEMIDLIST* pidlParent, const ITEMIDLIST* pidlChild);
	// ITEMIDLIST* ILFindLastID(const ITEMIDLIST* pidl);

	// BOOL ILRemoveLastID(ITEMIDLIST* pidl);
	// ITEMIDLIST* ILAppendID(ITEMIDLIST* pidl, LPSHITEMID pmkid, BOOL fAppend_or_Prepend);
	// ITEMIDLIST* ILClone(const ITEMIDLIST* pidl);
	// ITEMIDLIST* ILCloneFirst(const ITEMIDLIST* pidl);
	// ITEMIDLIST* ILCombine(const ITEMIDLIST* pidl1, const ITEMIDLIST* pidl2);

	// HRESULT ILLoadFromStream(IStream *pstm, ITEMIDLIST* *pidl);
	// HRESULT ILSaveToStream(IStream * pstm, const ITEMIDLIST* pidl);

	inline bool ILIsRoot(const ITEMIDLIST* pidl)	{ return pidl->mkid.cb == 0; }
	inline HRESULT ILGetPath(const ITEMIDLIST* pidl, CHAR path[MAX_PATH])	{ return SHGetPathFromIDListA(pidl, path) ? S_OK : E_FAIL; }
	inline HRESULT ILGetPath(const ITEMIDLIST* pidl, WCHAR path[MAX_PATH])	{ return SHGetPathFromIDListW(pidl, path) ? S_OK : E_FAIL; }
	ITEMIDLIST* ILFromPath(PCWSTR path, DWORD* pdwAttr);
	ITEMIDLIST* ILFromShared(HANDLE hMem, DWORD dwProcId);
	ITEMIDLIST* ILFromExplorer(HWND hwndExplorer);

	/// 親シェルフォルダの取得.
	inline HRESULT ILGetParentFolder(const ITEMIDLIST* pidl, IShellFolder** ppParent, const ITEMIDLIST** ppRelative = NULL)
	{
		return SHBindToParent(pidl, IID_IShellFolder, (void**)ppParent, ppRelative);
	}

	/// 自分自身のシェルフォルダの取得.
	HRESULT ILGetSelfFolder(const ITEMIDLIST* pidl, IShellFolder** ppFolder);

	/// ITEMIDLIST を比較し、-1, 0, +1 のいづれかを返す.
	inline int ILCompare(const ITEMIDLIST* lhs, const ITEMIDLIST* rhs)
	{
		if(lhs == rhs)	return  0;
		if(!lhs)		return -1;
		if(!rhs)		return  1;
		UINT lhsSize = ILGetSize(lhs);
		UINT rhsSize = ILGetSize(rhs);
		if(lhsSize < rhsSize)	return -1;
		if(lhsSize > rhsSize)	return  1;
		return memcmp(lhs, rhs, lhsSize);
	}

	/// SHGetFileInfo の IDList バージョン.
	inline DWORD_PTR ILGetFileInfo(const ITEMIDLIST* pidl, SHFILEINFO* info, DWORD flags, DWORD dwFileAttr = 0)
	{
		return ::SHGetFileInfo((PCWSTR)pidl, dwFileAttr, info, sizeof(SHFILEINFO), flags | SHGFI_PIDL);
	}

	HRESULT ILGetDisplayName(IShellFolder* folder, const ITEMIDLIST* leaf, DWORD shgdn, WCHAR name[], size_t bufsize);

	//==============================================================================
	// CIDA 関数.

	inline size_t CIDAGetCount(const CIDA* pCIDA)
	{
		ASSERT(pCIDA);
		return pCIDA->cidl;
	}
	inline const ITEMIDLIST* CIDAGetParent(const CIDA* pCIDA)
	{
		ASSERT(pCIDA);
		return (const ITEMIDLIST*)(((BYTE*)pCIDA) + pCIDA->aoffset[0]);
	}
	inline const ITEMIDLIST* CIDAGetAt(const CIDA* pCIDA, size_t index)
	{
		ASSERT(pCIDA);
		ASSERT(index < CIDAGetCount(pCIDA));
		return (const ITEMIDLIST*)(((BYTE*)pCIDA) + pCIDA->aoffset[index+1]);
	}
	inline size_t CIDAGetSize(const CIDA* pCIDA)
	{
		ASSERT(pCIDA);
		return pCIDA->aoffset[pCIDA->cidl] + ILGetSize(CIDAGetAt(pCIDA, CIDAGetCount(pCIDA)-1));
	}
	HGLOBAL CIDAFromSingleIDList(const ITEMIDLIST* pidl);
	HGLOBAL CIDAToHDROP(const CIDA* pCIDA);	///< CIDAをHDROPに変換.
	HGLOBAL CIDAToTextA(const CIDA* pCIDA);	///< CIDAをMBCSに変換.
	HGLOBAL CIDAToTextW(const CIDA* pCIDA);	///< CIDAをWCSに変換.

	//==============================================================================

	bool PatternEquals(PCWSTR wild, PCWSTR str);
}
