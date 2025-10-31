// utils.cpp

#include "stdafx.h"
#include "main.hpp"
#include "utils.hpp"

HRESULT ave::EntryNameToClipboard(mew::io::IEntryList* entries, mew::io::IEntry::NameType what) {
  size_t count = entries->Count;
  if (count == 0) {
    return S_FALSE;
  }
  bool first = true;
  mew::Stream stream;
  HGLOBAL hGlobal = mew::io::StreamCreateOnHGlobal(&stream, 0, false);
  for (size_t i = 0; i < count; ++i) {
    mew::ref<mew::io::IEntry> entry;
    if SUCCEEDED (entries->GetAt(&entry, i)) {
      mew::string path = entry->GetName(what);
      if (!!path) {
        PCTSTR text = path.str();
        size_t length = path.length();
        if (first) {
          first = false;
        } else {
          stream.write(L"\r\n", 2 * sizeof(WCHAR));
        }
        stream.write(text, length * sizeof(TCHAR));
      }
    }
  }
  stream.write(_T("\0"), sizeof(TCHAR));
  stream.clear();
  afx::SetClipboard(hGlobal, CF_UNICODETEXT);
  return S_OK;
}

HRESULT ave::EntryNameToClipboard(mew::ui::IShellListView* view, mew::Status status, mew::io::IEntry::NameType what) {
  HRESULT hr;
  mew::ref<mew::io::IEntryList> entries;
  if FAILED (hr = view->GetContents(&entries, status)) {
    return hr;
  }
  return EntryNameToClipboard(entries, what);
}
