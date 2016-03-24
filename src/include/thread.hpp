/// @file thread.hpp
/// ƒe[ƒ}.
#pragma once

namespace mew
{
	namespace ui
	{
		IDisplay* GetThread(DWORD dwThreadID = ::GetCurrentThreadId());

		inline UINT PopupMenu(HMENU hMenu, UINT tpm, int x, int y, HWND hOwner, const RECT* rcExclude)
		{
			if(IDisplay* thread = GetThread())
				return thread->PopupMenu(hMenu, tpm, x, y, hOwner, rcExclude);
			else
				return 0;
		}

		inline size_t GetMenuDepth() throw()
		{
			if(IDisplay* thread = GetThread())
				return thread->GetMenuDepth();
			else
				return 0;
		}

		inline HWND GetMenu(int index = -1) throw()
		{
			if(IDisplay* thread = GetThread())
				return thread->GetMenu(index);
			else
				return 0;
		}

		inline void RegisterMessageHook(void* self, IDisplay::WNDPROCEX wndproc)
		{
			if(IDisplay* thread = GetThread())
				thread->RegisterMessageHook(self, wndproc);
		}

		inline void UnregisterMessageHook(void* self, IDisplay::WNDPROCEX wndproc)
		{
			if(IDisplay* thread = GetThread())
				thread->UnregisterMessageHook(self, wndproc);
		}
	}
}
