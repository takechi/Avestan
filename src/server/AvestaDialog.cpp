// AvestaDialog.cpp

#include "stdafx.h"
#include "main.hpp"

//==============================================================================
// common

namespace
{
	string GetDirectoryOfView(IShellListView* view)
	{
		if(ref<IEntry> entry = GetFolderOfView(view))
		{
			string path = entry->Path;
			if(!!path)
				return path;
			else
				theAvesta->Notify(NotifyError, string::load(IDS_ERR_VIRTUALFOLDER, entry->Name));
		}
		else
			theAvesta->Notify(NotifyError, string::load(IDS_BAD_FODLER));
		return null;
	}

	bool Recheck(IShellListView* view, string path)
	{
		// ダイアログを表示している間にフォルダビューが無効 or パスが変わる可能性があるため、もう一度チェックする.
		if(path == GetPathOfView(view))
			return true;
		theAvesta->Notify(NotifyWarning, string::load(IDS_ERR_BADFOLDER));
		return false;
	}
}

//==============================================================================
// New

namespace
{
	class NewFileDlg : public Dialog
	{
	public:
		string	m_location;
		TCHAR	m_extension[MAX_PATH];
		TCHAR	m_names[MAX_PATH];
		bool	m_select;

	public:
		NewFileDlg()
		{
			str::clear(m_extension);
			str::clear(m_names);
			m_select = true;
		}
		INT_PTR Go(string location)
		{
			m_location = location;
			return __super::Go(IDD_NEWFILE);
		}

	protected:
		virtual bool OnCreate()
		{
			if(!__super::OnCreate())
				return false;
			SetText(IDC_NEW_PATH, m_location.str());
			SetText(IDC_NEW_EXT , m_extension);
			afx::Edit_SubclassSingleLineTextBox(GetItem(IDC_NEW_EXT), NULL, theAvesta->EditOptions | afx::EditTypeMultiName);
			SetText(IDC_NEW_NAME, m_names);
			afx::Edit_SubclassSingleLineTextBox(GetItem(IDC_NEW_NAME), NULL, theAvesta->EditOptions);
			SetChecked(IDC_NEW_SELECT, m_select);
			SetTip(IDC_NEW_PATH  , _T("ファイルが作成される場所です。"));
			SetTip(IDC_NEW_NAME  , _T("'/' や ';' で区切ると複数同時に作成できます。空の場合はデフォルトの名前がつけられます。"));
			SetTip(IDC_NEW_EXT   , _T("空の場合はフォルダが作成されます。'.' は付けても付けなくてもかまいません。"));
			SetTip(IDC_NEW_SELECT, _T("作成後にファイルを選択状態にする場合はチェックします。"));
			return true;
		}

		void OnCommand(UINT what, HWND ctrl)
		{
			switch(what)
			{
			case IDOK:
				GetText(IDC_NEW_EXT , m_extension, MAX_PATH);
				if(m_extension[0] != __T('\0') && m_extension[0] != _T('.'))
					str::prepend(m_extension, _T("."));
				GetText(IDC_NEW_NAME, m_names, MAX_PATH);
				m_select = GetChecked(IDC_NEW_SELECT);
				m_location.clear();
				End(IDOK);
				break;
			case IDCANCEL:
				m_location.clear();
				End(IDCANCEL);
				break;
			}
		}
	};

#define FORBIDDEN_PATH_CHARS	L"\\/:\"<>|*?\t\r\n"

	void CreateFileOrFolder(std::vector<string>& newfiles, const string& path, PCWSTR names, PCWSTR extension)
	{
		// セミコロンは本来ファイルパス用の文字として使えるが、ここではパス区切りとして扱うことにする。
		const PCWSTR SEPARATOR = FORBIDDEN_PATH_CHARS L";";
		const PCWSTR TRIM      = FORBIDDEN_PATH_CHARS L"; ";
		const bool isEmptyExtension = str::empty(extension);
		for(StringSplit token(names, SEPARATOR, TRIM);;)
		{
			string leaf = token.next();
			if(!leaf)
				break;
			WCHAR file[MAX_PATH];
			path.copyto(file, MAX_PATH);
			PathAppendW(file, leaf.str());

			HRESULT hr;
			if(isEmptyExtension)
			{
				if(PathFileExistsW(file))
				{
					PathMakeUniqueName(file, MAX_PATH, NULL, leaf.str(), path.str());
				}
				int win32err = SHCreateDirectory(avesta::GetForm(), file);
				hr = (win32err == ERROR_SUCCESS ? S_OK : AtlHresultFromWin32(win32err));
			}
			else
			{
				ASSERT(extension[0] == _T('.'));
				CT2CW wext(extension);
				lstrcatW(file, wext);
				if(PathFileExistsW(file))
				{
					WCHAR leafWithExt[MAX_PATH];
					leaf.copyto(leafWithExt);
					lstrcatW(leafWithExt, wext);
					PathMakeUniqueName(file, MAX_PATH, NULL, leafWithExt, path.str());
				}
				hr = avesta::FileNew(file);
			}

			if(SUCCEEDED(hr))
				newfiles.push_back(file);
			else
				theAvesta->Notify(NotifyWarning, string::format(L"$1 の作成に失敗しました", file));
		}
	}

	enum AfterCreateEffect
	{
		AfterCreateNone,
		AfterCreateSelect,
		AfterCreateRename,
	};

	static void CreateAndSelect(IShellListView* view, const string& path, PCWSTR names, PCWSTR extension, AfterCreateEffect after)
	{
		std::vector<string> newfiles;
		if(str::empty(names))
			names = theAvesta->GetDefaultNewName();

		CreateFileOrFolder(newfiles, path, names, extension);
		if(after == AfterCreateNone || newfiles.empty())
			return;

		view->Send(CommandSelectNone);
		// 10回回っても選択できないようならばあきらめる
		for(int count = 0; count < 10; ++count)
		{
			// ファイルシステムへの変更に時間がかかるため、すぐには選択できない場合がある。
			// このスリープとメッセージディスパッチは、変更が通知されるのを待つために必要だと思われる。
			Sleep(200); // ←いまいち時間が分からない。
			afx::PumpMessage();
			//
			bool unique = true;
			for(size_t i = 0; i < newfiles.size(); ++i)
			{
				PCWSTR newfile = newfiles[i].str();
				PCWSTR newname = PathFindFileName(newfile);
				VERIFY_HRESULT( view->SetStatus(string(newname), SELECTED, unique) );
				unique = false;
			}
			ref<IEntryList> entries;
			if(SUCCEEDED(view->GetContents(&entries, SELECTED)) && entries->Count == newfiles.size())
			{	// unique選択でいったん選択数がゼロになるため、個数のみの判別で十分なはず。
				TRACE(_T("info: 新規作成ファイルの選択に $1 回のループが必要でした"), count);
				if(after == AfterCreateRename)
					view->Send(CommandRename);
				break;
			}
		}
	}
}

void NewFolder(IShellListView* view)
{
	if(string path = GetDirectoryOfView(view))
		CreateAndSelect(view, path, null, null, AfterCreateRename);
}

void DlgNew(IShellListView* view)
{
	string path = GetDirectoryOfView(view);
	if(!path)
		return;
	static NewFileDlg dlg;
	ref<IUnknown> unk(view); // AddRef()のため
	if(dlg.Go(path) == IDOK)
	{
		if(Recheck(view, path))
			CreateAndSelect(view, path, dlg.m_names, dlg.m_extension, (dlg.m_select ? AfterCreateSelect : AfterCreateNone));
	}
}

//==============================================================================
// Select & Pattern

namespace
{
	static void DoSelect(IShellListView* view, const string& path, PCTSTR pattern)
	{
		bool unique = !ui::IsKeyPressed(VK_CONTROL);
		if(unique)
			view->Send(CommandSelectNone);

		TCHAR buf[MAX_PATH];
		PathCombine(buf, path.str(), L"*.*");
		int count = 0;
		WIN32_FIND_DATA find;
		HANDLE hFind = ::FindFirstFile(buf, &find);
		if(hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if(lstrcmp(find.cFileName, _T(".")) == 0)
					continue;
				if(lstrcmp(find.cFileName, _T("..")) == 0)
					continue;
				if(!afx::PatternEquals(pattern, find.cFileName))
					continue;
				if SUCCEEDED(view->SetStatus(string(find.cFileName), SELECTED, unique))
				{
					++count;
					unique = false;
				}
			} while(::FindNextFile(hFind, &find));
			::FindClose(hFind);
		}
		if(count == 0)
		{	// 指定されたパターンが一つも見つからなかった。
			theAvesta->Notify(NotifyWarning, string::load(IDS_WARN_NOSELECT));
		}
	}
}

void DlgSelect(IShellListView* view)
{
	static string theLastPattern;

	string path = GetDirectoryOfView(view);
	if(!path)
		return;

	ref<IUnknown> addref(view);
	string pattern;
	if SUCCEEDED(avesta::NameDialog(&pattern, path, theLastPattern, IDD_SELECT))
	{
		if(pattern && Recheck(view, path))
		{
			theLastPattern = pattern;
			DoSelect(view, path, pattern.str());
		}
	}
}

void DlgPattern(IShellListView* view)
{
	string path = GetDirectoryOfView(view);
	if(!path)
		return;

	ref<IUnknown> addref(view);
	string mask;
	if SUCCEEDED(avesta::NameDialog(&mask, path, view->PatternMask, IDD_PATTERN))
	{
		if(Recheck(view, path))
			view->PatternMask = mask;
	}
}
