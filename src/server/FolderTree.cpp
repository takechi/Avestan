// FolderTree.cpp

#include "stdafx.h"
#include "main.hpp"
#include "object.hpp"

//==============================================================================

void CreateFolderTree(REFINTF ppInterface, IUnknown* arg) throw(...)
{
	HRESULT hr;
	ref<ITreeView> w(__uuidof(ShellTreeView), arg);
	w->Name = _T("ƒtƒHƒ‹ƒ_");
	if FAILED(hr = w.copyto(ppInterface))
		throw CastError(string::load(IDS_ERR_NOINTERFACE, w, ppInterface.iid), hr);
}

AVESTA_EXPORT_FUNC( FolderTree )

//==============================================================================
