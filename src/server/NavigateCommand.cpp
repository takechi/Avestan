// NavigateCommand.cpp

#include "stdafx.h"
#include "main.hpp"
#include "std/buffer.hpp"
#include "object.hpp"

namespace
{
	class NavigateCommand : public Root< implements<ICommand> >
	{
	private:
		string		m_path;
		Navigation	m_type;

	public:
		NavigateCommand(string path, Navigation type) : m_path(path), m_type(type) {}
		string get_Description()			{ return m_path; }
		UINT32 QueryState(IUnknown* owner)	{ return ENABLED; }
		void Invoke()
		{
			try
			{
				ref<IEntry> entry(__uuidof(Entry), m_path);
				switch(m_type)
				{
				case NaviOpen:
					Open(entry);
					break;
				case NaviOpenAlways:
					OpenAlways(entry);
					break;
				case NaviAppend:
					OpenAppend(entry);
					break;
				case NaviReserve:
					OpenReserve(entry);
					break;
				case NaviGoto:
					Goto(entry);
					break;
				case NaviGotoAlways:
					GotoAlways(entry);
					break;
				case NaviSwitch:
				case NaviReplace:
				default:
					Open(entry);
					break;
				}
			}
			catch(Error& e)
			{
				TRACE(e.Message);
				theAvesta->Notify(NotifyError, e.Message);
			}
		}
		
	private:
		/// 開く（修飾キーは自動）.
		static void Open(IEntry* entry)			{ theAvesta->OpenFolder(entry, NaviOpen); }
		/// 開き、それのみを選択する.
		static void OpenAlways(IEntry* entry)	{ theAvesta->OpenFolder(entry, NaviOpenAlways); }
		/// 追加で開く.
		static void OpenAppend(IEntry* entry)	{ theAvesta->OpenFolder(entry, NaviAppend); }
		/// 非表示で開く.
		static void OpenReserve(IEntry* entry)	{ theAvesta->OpenFolder(entry, NaviReserve); }
		/// 移動する（修飾キーは自動）.
		static void Goto(IEntry* entry)
		{
			ASSERT(entry);
			ref<IShellListView> current;
			if SUCCEEDED(theAvesta->GetComponent(&current, AvestaFolder))
				current->Go(entry);
			else
				theAvesta->OpenFolder(entry, NaviOpen);
		}
		/// 常に移動する.
		static void GotoAlways(IEntry* entry)
		{
			// TODO: これバグ！
			afx::SetModifierState(0, 0);
			Goto(entry);
			afx::RestoreModifierState(0);
		}
	};
}

ref<ICommand> CreateNavigateCommand(string path, string args)
{
	if(!path)
		return null;
	Navigation navi = ParseNavigate(args.str(), NaviOpen);
	return objnew<NavigateCommand>(path, navi);
}
