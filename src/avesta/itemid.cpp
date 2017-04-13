// itemid.cpp

#include "stdafx.h"
#include "avesta.hpp"
#include "std/str.hpp"

using namespace avesta;

//==============================================================================
// ITEMIDLIST.

namespace
{
	HRESULT HresultFromShellExecute(HINSTANCE hInstance, PCWSTR path, PCWSTR verb, HWND hwnd)
	{
		if(hInstance > (HINSTANCE)32)
			return S_OK;
		if(hwnd)
		{	// hwnd が指定された場合のみエラーを表示
			string msg;
			switch((int)hInstance)
			{
			case SE_ERR_FNF:
			case SE_ERR_PNF:
				msg = string::format(L"\"$1\" が見つかりません", path);
				break;
			case SE_ERR_ACCESSDENIED:
			case SE_ERR_SHARE:
				msg = string::format(L"\"$1\" にアクセスできません", path);
				break;
			case SE_ERR_OOM: // Out of Memory
				msg = L"メモリが不足しているため実行できませんでした";
				break;
			case SE_ERR_DLLNOTFOUND:
				msg = string::format(L"\"$1\" を実行するために必要なDLLが見つかりませんでした", path);
				break;
			case SE_ERR_ASSOCINCOMPLETE:
			case SE_ERR_NOASSOC:
				msg = string::format(L"\"$1\" に対して アクション \"$2\" が関連付けられていません", path, verb);
				break;
			case SE_ERR_DDETIMEOUT:
			case SE_ERR_DDEFAIL:
			case SE_ERR_DDEBUSY:
				msg = string::format(L"DDEエラーが発生したため \"$1\" を実行できませんでした", path);
				break;
			default:
				msg = string::format(L"$1 を実行できませんでした", path);
				break;
			}
			if(msg)
				::MessageBox(hwnd, msg.str(), null, MB_OK | MB_ICONERROR);
		}
		return AtlHresultFromLastError();
	}
}

namespace
{
	HRESULT DoPathExecute(PCWSTR path, PCWSTR verb, PCWSTR args, PCWSTR dir, HWND hwnd)
	{
		ASSERT(!str::empty(path));

		// path
		WCHAR quoted[MAX_PATH+2];
		// exe
		PCWSTR exe = null;
		WCHAR exepath[MAX_PATH];
		// dir
		WCHAR dirpath[MAX_PATH];
		// verb
		WCHAR verbbuf[MAX_PATH] = L"";
		if(!str::empty(verb))
		{
			ExpandEnvironmentStrings(verb, verbbuf, MAX_PATH);
			verb = verbbuf;
			if(verb[0] == _T('.'))
			{	// verb = 拡張子
				if FAILED(avesta::RegGetAssocExe(verb, exepath))
					return E_INVALIDARG; // 関連付けされたEXEを取得できなかった
				exe = exepath;
			}
			else if(*PathFindExtension(verb) == _T('.'))
			{	// verb = EXEファイルではない
				exe = verb;
			}
		}

		if(str::empty(dir))
		{	// ワーキングディレクトリは、起動するファイルと同じ
			str::copy(dirpath, path);
			PathRemoveFileSpec(dirpath);
			dir = dirpath;
		}

		if(hwnd)
			hwnd = ::GetAncestor(hwnd, GA_ROOTOWNER);

		HINSTANCE hInstance;
		if(exe)
		{
			if(str::find(path, L' '))
			{
				size_t ln = str::length(path);
				quoted[0] = _T('\"');
				str::copy(quoted+1, path);
				quoted[ln+1] = _T('\"');
				quoted[ln+2] = _T('\0');
				path = quoted;
			}
#if 0 // NOT IMPLEMENTED
			if(args)
				...;
#endif
			hInstance = ::ShellExecute(hwnd, null, exe, path, dir, SW_SHOWNORMAL);
		}
		else
		{
			hInstance = ::ShellExecute(hwnd, verb, path, args, dir, SW_SHOWNORMAL);
		}
		return HresultFromShellExecute(hInstance, path, verb, hwnd);
	}

	HRESULT DoILExecute(LPCITEMIDLIST pidl, PCWSTR path, PCWSTR verb, PCWSTR args, PCWSTR dir, HWND hwnd)
	{
		SHELLEXECUTEINFO info = { sizeof(SHELLEXECUTEINFO), SEE_MASK_IDLIST, ::GetAncestor(hwnd, GA_ROOTOWNER) };
		if(!hwnd)
			info.fMask |= SEE_MASK_FLAG_NO_UI;
		info.nShow = SW_SHOWNORMAL;
		info.lpIDList = (LPITEMIDLIST)pidl;
		info.lpDirectory = dir;
		info.lpVerb = verb;
		info.lpParameters = args;
		TCHAR dirpath[MAX_PATH];
		if(str::empty(dir) && !str::empty(path))
		{	// ワーキングディレクトリは、起動するファイルと同じ
			str::copy(dirpath, path, MAX_PATH);
			PathRemoveFileSpec(dirpath);
			info.lpDirectory = dirpath;
		}
		if(ShellExecuteEx(&info))
			return S_OK;
		return HresultFromShellExecute(info.hInstApp, path, verb, hwnd);
	}
}

HRESULT avesta::ILExecute(LPCITEMIDLIST pidl, PCWSTR verb, PCWSTR args, PCWSTR dir, HWND hwnd)
{
	if(!pidl)
		return E_INVALIDARG;

	bool lazy = GetOption(BoolLazyExecute);
	if(!hwnd)
		hwnd = GetForm();

	WCHAR path[MAX_PATH] = L"";

	if(SHGetPathFromIDListW(pidl, path) && !str::empty(verb))
	{
		HRESULT hr;
		// 失敗した場合にダイアログが出るので、lazy の場合はUIなし (hwnd=null)。
		if SUCCEEDED(hr = DoPathExecute(path, verb, args, dir, (!lazy ? hwnd : null)))
			return S_OK;
		else if(!lazy)
			return hr;
		else
			verb = null; // この動詞では実行できない。
	}
	
	return DoILExecute(pidl, path, verb, args, dir, hwnd);
}


HRESULT avesta::PathExecute(PCWSTR path, PCWSTR verb, PCWSTR args, PCWSTR dir, HWND hwnd)
{
	if(str::empty(path))
		return E_INVALIDARG;

	bool lazy = GetOption(BoolLazyExecute);
	if(!hwnd)
		hwnd = GetForm();

	if(!str::empty(verb))
	{
		HRESULT hr;
		// 失敗した場合にダイアログが出るので、lazy の場合はUIなし (hwnd=null)。
		if SUCCEEDED(hr = DoPathExecute(path, verb, args, dir, (!lazy ? hwnd : null)))
			return S_OK;
		else if(!lazy)
			return hr;
		else
			verb = null; // この動詞では実行できない。
	}
	
	return DoPathExecute(path, verb, args, dir, hwnd);
}
