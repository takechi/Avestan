// stdafx.cpp

#include "stdafx.h"

namespace module {
HMODULE Handle;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD what, void*) {
  switch (what) {
    case DLL_PROCESS_ATTACH:
#ifdef _DEBUG
      _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
#endif
      module::Handle = hModule;
      break;
    case DLL_PROCESS_DETACH:
      break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
      break;
  }
  return true;
}
