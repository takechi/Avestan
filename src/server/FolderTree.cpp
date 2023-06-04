// FolderTree.cpp

#include "stdafx.h"
#include "main.hpp"
#include "object.hpp"

//==============================================================================

void CreateFolderTree(mew::REFINTF ppInterface, IUnknown* arg) throw(...) {
  HRESULT hr;
  mew::ref<mew::ui::ITreeView> w(__uuidof(mew::ui::ShellTreeView), arg);
  w->Name = _T("ƒtƒHƒ‹ƒ_");
  if FAILED (hr = w.copyto(ppInterface)) {
    throw mew::exceptions::CastError(mew::string::load(IDS_ERR_NOINTERFACE, w, ppInterface.iid), hr);
  }
}

AVESTA_EXPORT_FUNC(FolderTree)

//==============================================================================
