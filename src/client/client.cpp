// avesta.cpp

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <psapi.h>
#pragma comment(lib, "psapi")

#ifdef UNICODE
	const DWORD DATA_HEADER = 'STRW';
#else
	const DWORD DATA_HEADER = 'STRA';
#endif

const LPCTSTR APPNAME = _T("Avesta");
const LPCTSTR DLLNAME = _T("avesta.dll");
static TCHAR EXEPATH[MAX_PATH], DLLPATH[MAX_PATH];

//======================================================================

const PCWSTR MSG_DLL_ERROR    = L"avesta.dll が不正です。";
const PCWSTR MSG_INIT_ERROR   = L"avesta.dll を初期化できません。";
const PCWSTR MSG_PASS_ERROR   = L"引数の受け渡しに失敗しました";
const PCWSTR MSG_UPDATE_OK    = L"オンラインアップデートに成功しました。\n再起動しますか？";
const PCWSTR MSG_UPDATE_ERROR = L"オンラインアップデートに失敗しました。";

inline int MsgBox(PCWSTR msg, int mb)
{
	return ::MessageBox(NULL, msg, APPNAME, mb);
}

//======================================================================

inline int PassArgs(HWND hwnd, PWSTR args)
{
	COPYDATASTRUCT data;
	data.dwData = DATA_HEADER;
	data.cbData = lstrlen(args) * sizeof(TCHAR);
	data.lpData = args;
	if(::SendMessage(hwnd, WM_COPYDATA, NULL, (LPARAM)&data))
	{
		::AttachThreadInput(::GetCurrentThreadId(), ::GetWindowThreadProcessId(hwnd, NULL), true);
		if(::IsIconic(hwnd))
			::ShowWindow(hwnd, SW_RESTORE);
		if(!::IsWindowVisible(hwnd))
			::ShowWindow(hwnd, SW_SHOW);
		::BringWindowToTop(hwnd);
		::SetActiveWindow(hwnd);
		return 1;
	}
	else
	{
		MsgBox(MSG_PASS_ERROR, MB_OK | MB_ICONERROR);
		return -1;
	}
}

inline static bool UpdateAvesta(PCWSTR newAvesta)
{
	return CopyFile(newAvesta, DLLPATH, FALSE) != 0;
}

LPCTSTR DLLs[] =
{
	_T("shell32.dll"),
};

static void ForceUnload(HINSTANCE hDLL)
{
	try
	{
		while(::FreeLibrary(hDLL)) {}
	}
	catch(...)
	{
	}
}

inline static int RunAvesta(PWSTR args, INT sw)
{
	int ret = -1;
	PWSTR newAvesta = NULL;
	HINSTANCE hDllAvesta = ::LoadLibrary(DLLPATH);
	if(hDllAvesta)
	{
		typedef int (*AvestaMain)(PCWSTR args, INT sw, PWSTR* newAvesta);
		if(AvestaMain fnMain = (AvestaMain)::GetProcAddress(hDllAvesta, "AvestaMain"))
			ret = fnMain(args, sw, &newAvesta);
		else
			MsgBox(MSG_DLL_ERROR, MB_OK | MB_ICONERROR);
	}
	else
	{
		MsgBox(MSG_INIT_ERROR, MB_OK | MB_ICONERROR);
	}

	// 再起動が要求されているので、アンロードする。
	if(ret > 0)
	{
		ForceUnload(hDllAvesta);
		for(size_t i = 0; i < sizeof(DLLs)/sizeof(DLLs[0]); ++i)
			ForceUnload(::LoadLibrary(DLLs[i]));
	}

	// ホットアップデート
	if(newAvesta)
	{
		if(UpdateAvesta(newAvesta))
		{
			if(MsgBox(MSG_UPDATE_OK, MB_OKCANCEL | MB_ICONINFORMATION) != IDOK)
				ret = 0;
		}
		else
		{
			MsgBox(MSG_UPDATE_ERROR, MB_OK | MB_ICONERROR);
		}
		::GlobalFree(newAvesta);
	}
	return ret;
}

static bool GetProcessFromWindow(HWND hWnd, PWSTR filename, DWORD nSize)
{
	bool result = false;
    DWORD dwProcessId;
    GetWindowThreadProcessId(hWnd , &dwProcessId);
    if(HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessId))
    {
        HMODULE hModule;
        DWORD dwNeed;
        if(EnumProcessModules(hProcess, &hModule, sizeof(hModule), &dwNeed))
        {
            // モジュールハンドルからフルパス名を取得
            if(GetModuleFileNameEx(hProcess, hModule, filename, nSize))
				result = true;
        }
        CloseHandle(hProcess);
    }
	return result;
}

static BOOL CALLBACK EnumAvestaWindow(HWND hWnd, LPARAM lParam)
{
	TCHAR classname[MAX_PATH];
	if(::GetClassName(hWnd, classname, MAX_PATH))
	{
		if(lstrcmp(classname, APPNAME) == 0)
		{
			WCHAR path[MAX_PATH];
			if(GetProcessFromWindow(hWnd, path, MAX_PATH))
			{
				if(lstrcmpi(EXEPATH, path) == 0)
				{
					*(HWND*)lParam = hWnd;
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

static void MyPathCombine(LPTSTR dst, LPCTSTR lhs, LPCTSTR rhs)
{
	int len = lstrlen(lhs);
	while(lhs[len-1] != _T('\\')) { --len; }
	lstrcpyn(dst, lhs, len+1);
	lstrcpy(dst+len, rhs);
}

int WINAPI _tWinMain(HINSTANCE, HINSTANCE, PWSTR args, int sw)
{
	::GetModuleFileName(NULL, EXEPATH, MAX_PATH);
	MyPathCombine(DLLPATH, EXEPATH, DLLNAME);
	HWND hwnd = NULL;
	EnumWindows(EnumAvestaWindow, (LPARAM)&hwnd);
	if(hwnd)
	{
		return PassArgs(hwnd, args);
	}
	while(RunAvesta(args, sw) > 0) { args = L""; }
	return 0;
}
