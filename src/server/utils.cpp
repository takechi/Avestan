// utils.cpp

#include "stdafx.h"
#include "main.hpp"
#include "utils.hpp"

HRESULT ave::EntryNameToClipboard(IEntryList* entries, IEntry::NameType what)
{
	size_t count = entries->Count;
	if(count == 0)
		return S_FALSE;
	bool first = true;
	Stream stream;
	HGLOBAL hGlobal = StreamCreateOnHGlobal(&stream, 0, false);
	for(size_t i = 0; i < count; ++i)
	{
		ref<IEntry> entry;
		if SUCCEEDED(entries->GetAt(&entry, i))
		{
			string path = entry->GetName(what);
			if(!!path)
			{
				PCTSTR text = path.str();
				size_t length = path.length();
				if(first)
					first = false;
				else
					stream.write(L"\r\n", 2 * sizeof(WCHAR));
				stream.write(text, length * sizeof(TCHAR));
			}
		}
	}
	stream.write(_T("\0"), sizeof(TCHAR));
	stream.clear();
	afx::SetClipboard(hGlobal, CF_UNICODETEXT);
	return S_OK;
}

HRESULT ave::EntryNameToClipboard(IShellListView* view, Status status, IEntry::NameType what)
{
	HRESULT hr;
	ref<IEntryList> entries;
	if FAILED(hr = view->GetContents(&entries, status))
		return hr;
	return EntryNameToClipboard(entries, what);
}
