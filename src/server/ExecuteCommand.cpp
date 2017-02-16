// ExecuteCommand.cpp

#include "stdafx.h"
#include "main.hpp"
#include "std/buffer.hpp"
#include "object.hpp"

//#define ENABLE_UN

namespace
{
	typedef std::vector<string>	StringVector;

	enum ExecuteType
	{
		ExecuteNoArg = -1,
		ExecuteFolderCurrent,
		ExecuteFolderAll,
		ExecuteFolderShown,
		ExecuteFolderHidden,
		ExecuteFolderParent,
		ExecuteItems,
		ExecuteItemsSelected,
		ExecuteItemsChecked,
#ifdef ENABLE_UN
		ExecuteItemsUnselected,
		ExecuteItemsUnchecked,
#endif
		NumExecuteArgs,
	};

	static HRESULT Execute(PCWSTR path, PCWSTR args)
	{
		// 自己呼び出しの最適化
		TCHAR selfname[MAX_PATH];
		::GetModuleFileName(NULL, selfname, MAX_PATH);
		if(str::equals_nocase(path, selfname))
		{
			theAvesta->ParseCommandLine(args);
			return S_OK;
		}
		// 自分以外
		string dir = theAvesta->CurrentPath();
		return avesta::PathExecute(path, null, args, dir.str());
	}

	class ExecuteCommandNoArgs : public Root< implements<ICommand> >
	{
	private:
		string	m_path;
		string	m_args;

	public:
		ExecuteCommandNoArgs(string path, string args) : m_path(path), m_args(args) {}
		string get_Description()			{ return m_path; }
		UINT32 QueryState(IUnknown* owner)	{ return ENABLED; }
		void Invoke()						{ Execute(m_path.str(), m_args.str()); }

	};

	class __declspec(novtable) ExecuteCommandBase : public Root< implements<ICommand> >
	{
	protected:
		ExecuteType m_type;
		string	m_path, m_argsL, m_argsR;

	protected:
		ExecuteCommandBase(string path, ExecuteType type, PCWSTR args, PCWSTR le, PCWSTR rs)
			: m_path(path), m_type(type)
		{
			m_argsL.assign(args, le-args);
			m_argsR.assign(rs);
		}

	public:
		string get_Description()	{ return m_path; }
		UINT32 QueryState(IUnknown* owner)
		{
			switch(m_type)
			{
			case ExecuteFolderCurrent:
			case ExecuteFolderAll:
			case ExecuteFolderShown:
			case ExecuteFolderHidden:
			case ExecuteFolderParent:
			case ExecuteItems:
			case ExecuteItemsChecked:
#ifdef ENABLE_UN
			case ExecuteItemsUnselected:
			case ExecuteItemsUnchecked:
#endif
			{
				ref<IShellListView> current;
				if(SUCCEEDED(theAvesta->GetComponent(&current, AvestaFolder)))
					return ENABLED;
				else
					return 0;
			}
			case ExecuteItemsSelected:
			{
				ref<IShellListView> current;
				if(SUCCEEDED(theAvesta->GetComponent(&current, AvestaFolder)) && current->SelectedCount > 0)
					return ENABLED;
				else
					return 0;
			}
			default:
				TRESPASS_DBG(return 0);
			}
		}

	protected:
		bool QueryFiles(StringVector& files)
		{
			switch(m_type)
			{
			case ExecuteFolderCurrent:
				return QueryFolders(FOCUSED, files);
			case ExecuteFolderAll:
				return QueryFolders(StatusNone, files);
			case ExecuteFolderShown:
				return QueryFolders(SELECTED, files);
			case ExecuteFolderHidden:
				return QueryFolders(UNSELECTED, files);
			case ExecuteFolderParent:
				return QueryParent(FOCUSED, files);
			case ExecuteItems:
				return QueryItems(StatusNone, files);
			case ExecuteItemsSelected:
				return QueryItems(SELECTED, files);
			case ExecuteItemsChecked:
				return QueryItems(CHECKED, files);
#ifdef ENABLE_UN
			case ExecuteItemsUnselected:
				return QueryItems(UNSELECTED, files);
			case ExecuteItemsUnchecked:
				return QueryItems(UNCHECKED, files);
#endif
			default:
				TRESPASS_DBG(return 0);
			}
		}
		static bool QueryFolders(Status status, StringVector& files)
		{
			files.clear();
			ref<IList> tab;
			if FAILED(theAvesta->GetComponent(&tab, AvestaTab))
				return false;
			// current folder path.
			ref<IEnumUnknown> views;
			if FAILED(tab->GetContents(&views, status))
				return false;
			for(each<IShellListView> i(views); i.next();)
			{
				if(ref<IEntry> entry = GetFolderOfView(i))
				{
					if(string path = entry->Path)
						files.push_back(path);
				}
			}
			return !files.empty();
		}
		static bool QueryParent(Status status, StringVector& files)
		{
			QueryFolders(status, files);
			for(size_t i = 0; i < files.size(); ++i)
			{
				if(PathIsRoot(files[i].str()))
				{
					files[i] = GUID_MyComputer; // ドライブの上のフォルダはマイコンピュータ
				}
				else
				{
					WCHAR parent[MAX_PATH];
					files[i].copyto(parent);
					PathRemoveFileSpec(parent);
					files[i] = parent;
				}
			}
			return true;
		}
		static bool QueryItems(Status status, StringVector& files)
		{
			ref<IEntryList> items;
			if(!QueryItems(status, &items))
				return false;
			const size_t count = items->Count;
			for(size_t i = 0; i < count; ++i)
			{
				ref<IEntry> entry;
				if FAILED(items->GetAt(&entry, i))
					continue;
				string path = entry->Path;
				if(!path)
					continue;
				files.push_back(path);
			}
			return !files.empty();
		}
		static bool QueryItems(Status status, IEntryList** items)
		{
			ASSERT(items);
			ref<IShellListView> current;
			ref<IEntry> entry;
			if(FAILED(theAvesta->GetComponent(&current, AvestaFolder))
			|| FAILED(current->GetFolder(&entry)))
			{
				return false;
			}
			// selections
			return SUCCEEDED(current->GetContents(items, status));
		}
	};

	class ExecuteEachCommand : public ExecuteCommandBase
	{
	private:
		typedef ExecuteCommandBase super;
	public:
		ExecuteEachCommand(string path, ExecuteType type, PCWSTR args, PCWSTR le, PCWSTR rs) : super(path, type, args, le, rs) {}
		void Invoke()
		{
			StringVector files;
			string directory;
			if(!QueryFiles(files))
			{
				theAvesta->Notify(NotifyWarning, string::load(IDS_WARN_NOTARGET));
				return;
			}
			const size_t count = files.size();
			StringBuffer params;
			params.reserve(m_argsL.length() + m_argsR.length() + MAX_PATH);
			for(size_t i = 0; i < count; ++i)
			{
				params.clear();
				params.append(m_argsL.str());
				params.append_path(files[i].str());
				params.append(m_argsR.str());
				params.append(L'\0');
				Execute(m_path.str(), params);
			}
		}
	};

	class ExecuteAllCommand : public ExecuteCommandBase
	{
	private:
		typedef ExecuteCommandBase super;
	public:
		ExecuteAllCommand(string path, ExecuteType type, PCWSTR args, PCWSTR le, PCWSTR rs) : super(path, type, args, le, rs) {}
		void Invoke()
		{
			StringVector files;
			if(!QueryFiles(files))
			{
				theAvesta->Notify(NotifyWarning, string::load(IDS_WARN_NOTARGET));
				return;
			}
			const size_t count = files.size();
			StringBuffer params;
			params.reserve(m_argsL.length() + m_argsR.length() + MAX_PATH + 128*count); // 適当な量を確保
			params.append(m_argsL.str());
			for(size_t i = 0; i < count; ++i)
			{
				params.append_path(files[i].str());
				params.append(' ');
			}
			params.append(m_argsR.str());
			params.append(L'\0');
			Execute(m_path.str(), params);
		}
	};

	const PCWSTR ExecuteArgsText[] =
	{
		L"current",
		L"all",
		L"shown",
		L"hidden",
		L"parent",
		L"items",
		L"selected",
		L"checked",
#ifdef ENABLE_UN
		L"unselected",
		L"unchecked",
#endif
	};

	void SetArgName(PWSTR name, WCHAR left, WCHAR right, PCWSTR base, size_t len)
	{
		name[0] = left;
		wcsncpy(name+1, base, len);
		name[len+1] = right;
		name[len+2] = L'\0';
	}

	ExecuteType SearchVarsInArgs(PCWSTR args, WCHAR bracket[2], PCWSTR* le, PCWSTR* rs)
	{
		WCHAR varname[32];
		// 'all' variables
		for(size_t i = 0; i < NumExecuteArgs; ++i)
		{
			PCWSTR var = ExecuteArgsText[i];
			size_t varlen = wcslen(var);
			SetArgName(varname, bracket[0], bracket[1], var, varlen);
			PCWSTR at = wcsstr(args, varname);
			if(at)
			{
				*le = at;
				*rs = at + varlen + 2;
				return (ExecuteType)i;
			}
		}
		return ExecuteNoArg;
	}
}

namespace
{
	bool PathExtensionExists(string& basename, PCWSTR extension)
	{
		WCHAR path[MAX_PATH];
		basename.copyto(path, MAX_PATH);
		str::append(path, extension);
		if(!::PathFileExists(path))
			return false;
		basename = path;
		return true;
	}

	bool PathIsNonFile(PCWSTR path)
	{
		if(str::compare(path, L"http://", 7) == 0) return true;
		if(str::compare(path, L"https://", 8) == 0) return true;
		if(str::compare(path, L"ftp://" , 6) == 0) return true;
		return false;
	}
}

ref<ICommand> CreateExecuteCommand(string path, string args)
{
	// path のパスを解決する.
	if(!PathIsNonFile(path.str()))
	{
		path = ResolvePath(path);
		if(!path)
			return null;
		if(!PathFileExists(path.str()))
		{	// 不完全なパス？
			// とりあえず、自動補完される実行ファイルの拡張子を補って試してみる。
			if(!PathExtensionExists(path, L".exe")
			&& !PathExtensionExists(path, L".bat")
			&& !PathExtensionExists(path, L".cmd"))
			{
				return null;
			}
		}
	}
	// args.
	PCWSTR wcsArgs = args.str();
	if(wcsArgs)
	{
		PCWSTR le, rs;
		ExecuteType type;
		// 'all' variables
		type = SearchVarsInArgs(wcsArgs, L"{}", &le, &rs);
		if(type != ExecuteNoArg)
			return objnew<ExecuteAllCommand>(path, type, wcsArgs, le, rs);
		// 'each' variables
		type = SearchVarsInArgs(wcsArgs, L"[]", &le, &rs);
		if(type != ExecuteNoArg)
			return objnew<ExecuteEachCommand>(path, type, wcsArgs, le, rs);
		// no args, fall through
	}
	return objnew<ExecuteCommandNoArgs>(path, args);
}
