// utils.hpp
#pragma once

#include "path.hpp"

namespace ave
{
	HRESULT EntryNameToClipboard(IEntryList* entries, IEntry::NameType what);
	HRESULT EntryNameToClipboard(IShellListView* view, Status status, IEntry::NameType what);

	inline ref<IEntry> GetFolderOfView(IShellListView* view)
	{
		if(!view)
			return null;
		ref<IEntry> folder;
		if FAILED(view->GetFolder(&folder))
			return null;
		return folder;
	}
	inline string GetNameOfView(IShellListView* view, IEntry::NameType what)
	{
		if(ref<IEntry> folder = GetFolderOfView(view))
			return folder->GetName(what);
		else
			return null;
	}
	inline string GetPathOfView(IShellListView* view)
	{
		return GetNameOfView(view, IEntry::PATH);
	}

	struct WindowRef
	{
		HWND	hWnd;
		WindowRef(const Null&) : hWnd(0)	{}
		WindowRef(IWindow* window) : hWnd(window ? window->Handle : 0) {}
		template < class T > WindowRef(const ref<T>& window) : hWnd(window ? window->Handle : 0) {}
		WindowRef(HWND hwnd) : hWnd(hwnd) {}
		operator HWND () const	{ return hWnd; }
	};

	struct StringRef
	{
		PCWSTR str;
		StringRef(const string& s) : str(s.str()) {}
		StringRef(PCWSTR s) : str(s) {}
		operator PCWSTR() const	{ return str; }
	};

	inline int MessageBox(HWND hWnd, PCTSTR text, UINT type = MB_OK)
	{
		TCHAR caption[MAX_PATH] = _T("");
		HWND hParent = ::GetAncestor(hWnd, GA_ROOT);
		if(hParent)
			::GetWindowText(hParent, caption, MAX_PATH);
		return ::MessageBox(hParent, text, caption, type);
	}
	inline int InfoBox    (WindowRef window, StringRef text, UINT type = MB_OK)	{ return MessageBox(window, text, type | MB_ICONINFORMATION); }
	inline int QuestionBox(WindowRef window, StringRef text, UINT type = MB_OK)	{ return MessageBox(window, text, type | MB_ICONQUESTION); }
	inline int WarningBox (WindowRef window, StringRef text, UINT type = MB_OK)	{ return MessageBox(window, text, type | MB_ICONWARNING); }
	inline int ErrorBox   (WindowRef window, StringRef text, UINT type = MB_OK)	{ return MessageBox(window, text, type | MB_ICONERROR); }
}
