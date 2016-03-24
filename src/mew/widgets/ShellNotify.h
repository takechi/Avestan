// ShellNotify.h
#pragma once

class SHNotifyBase
{
protected:
	ULONG m_nSHChangeNotifyID;

public:
	SHNotifyBase() : m_nSHChangeNotifyID(0)
	{
	}
	~SHNotifyBase()
	{
		SHEndMonitor();
	}
	// ------------------------------------------------------
	//SHCNE_ALLEVENTS
	//SHCNE_ASSOCCHANGED
	//SHCNE_ATTRIBUTES
	//SHCNE_FREESPACE
	//SHCNE_UPDATEIMAGE

	//SHCNE_CREATE       : A nonfolder item has been created
	//SHCNE_DELETE       : A nonfolder item has been deleted.
	//SHCNE_MKDIR        : A folder has been created
	//SHCNE_RENAMEFOLDER : The name of a folder has changed (prev, next)
	//SHCNE_RENAMEITEM   : The name of a nonfolder item has changed.
	//SHCNE_RMDIR        : A folder has been removed (item, null)
	//SHCNE_UPDATEDIR    : The contents of an existing folder have changed, but the folder still exists and has not been renamed. 
	//SHCNE_UPDATEITEM   : An existing nonfolder item has changed, but the item still exists and has not been renamed. 

	//SHCNE_DRIVEADD
	//SHCNE_DRIVEADDGUI
	//SHCNE_DRIVEREMOVED
	//SHCNE_MEDIAINSERTED
	//SHCNE_MEDIAREMOVED
	//SHCNE_NETSHARE
	//SHCNE_NETUNSHARE
	//SHCNE_SERVERDISCONNECT
	//SHCNE_DISKEVENTS
	//SHCNE_GLOBALEVENTS
	//SHCNE_INTERRUPT

	// ------------------------------------------------------
	// fSources
	// SHCNRF_InterruptLevel
	// SHCNRF_ShellLevel
	// SHCNRF_RecursiveInterrupt
	// SHCNRF_NewDelivery
	//	SHChangeNotification_Lock
	//	SHChangeNotification_Unlock
	HRESULT SHBeginMonitor(int numEntries, SHChangeNotifyEntry entries[], HWND hWnd, UINT uMsg, LONG shcneMask, int shcnrf = SHCNRF_ShellLevel | SHCNRF_NewDelivery)
	{
		SHEndMonitor();
		m_nSHChangeNotifyID = ::SHChangeNotifyRegister(hWnd, shcnrf, shcneMask, uMsg, numEntries, entries);
		return m_nSHChangeNotifyID != 0 ? S_OK : E_FAIL;
	}
	HRESULT SHEndMonitor()
	{
		if(m_nSHChangeNotifyID == 0)
			return S_FALSE;
		BOOL res = ::SHChangeNotifyDeregister(m_nSHChangeNotifyID);
		m_nSHChangeNotifyID = 0;
		return res ? S_OK : E_FAIL;
	}
#ifdef _DEBUG
	void SHDumpEvent(LONG lEvent)
	{
		// シェルの変更通知
		if (lEvent & SHCNE_RENAMEITEM)
		{
			// アイテムの名称が変更された
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_RENAMEITEM"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_CREATE)
		{
			// アイテムが作成された
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_CREATE"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_DELETE)
		{
			// アイテムが削除された
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_DELETE"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_MKDIR)
		{
			// ディレクトリが作成された
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_MKDIR"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_RMDIR)
		{
			// ディレクトリが削除された
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_RMDIR"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_MEDIAINSERTED)
		{
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_MEDIAINSERTED"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_MEDIAREMOVED)
		{
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_MEDIAREMOVED"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_DRIVEREMOVED)
		{
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_DRIVEREMOVED"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_DRIVEADD)
		{
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_DRIVEADD"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_NETSHARE)
		{
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_NETSHARE"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_NETUNSHARE)
		{
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_NETUNSHARE"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_ATTRIBUTES)
		{
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_ATTRIBUTES"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_UPDATEDIR)
		{
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_UPDATEDIR"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_UPDATEITEM)
		{
			// フォルダ・アイテムの属性が変更された
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_UPDATEITEM"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_SERVERDISCONNECT)
		{
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_SERVERDISCONNECT"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_UPDATEIMAGE)
		{
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_UPDATEIMAGE"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_DRIVEADDGUI)
		{
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_DRIVEADDGUI"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_RENAMEFOLDER)
		{
			// フォルダの名称が変更された
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_RENAMEFOLDER"), m_nSHChangeNotifyID);
		}
		if (lEvent & SHCNE_FREESPACE)
		{
			// フリースペースが増減した
			TRACE(_T("OnSHChangeNotify($1) : SHCNE_FREESPACE"), m_nSHChangeNotifyID);
		}
	}
#endif
};

template < class T >
class __declspec(novtable) SHNotifySink : public SHNotifyBase
{
public:
	LRESULT OnSHChangeNotify(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LONG lEvent = 0;
		ITEMIDLIST** pidls = NULL;
		HANDLE hLock = ::SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &pidls, &lEvent);
		//DEBUG_ONLY( SHDumpEvent(lEvent); );
		static_cast<T*>(this)->HandleShellChangeNotify(lEvent, pidls[0], pidls[1]);
		::SHChangeNotification_Unlock(hLock);
		return 0;
	}
	void HandleShellChangeNotify(LONG lEvent, const ITEMIDLIST* pidl1, const ITEMIDLIST* pidl2)
	{
	}
};
