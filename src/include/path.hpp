/// @file path.hpp
/// パス操作.
#pragma once

#include "string.hpp"

namespace mew
{
	namespace io
	{
		//==============================================================================
		// 特殊フォルダのパス

		const WCHAR GUID_MyDocument  [] = L"::{450D8FBA-AD25-11D0-98A8-0800361B1103}";
		const WCHAR GUID_Network     []	= L"::{208D2C60-3AEA-1069-A2D7-08002B30309D}"; 
		const WCHAR GUID_RecycleBin  [] = L"::{645FF040-5081-101B-9F08-00AA002F954E}";
		const WCHAR GUID_Internet    []	= L"::{871C5380-42A0-1069-A2EA-08002B30309D}";
		const WCHAR GUID_MyComputer  [] = L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}";
		const WCHAR GUID_ControlPanel[] = L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{21EC2020-3AEA-1069-A2DD-08002B30309D}";
		const WCHAR GUID_Printer     []	= L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{2227A280-3AEA-1069-A2DE-08002B30309D}";
		const WCHAR GUID_WebFolder   [] = L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{BDEADF00-C265-11d0-BCED-00A0C90AB50F}";
		const WCHAR GUID_DialUp      []	= L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{992CFFA0-F557-101A-88EC-00DD010CCC48}";
		const WCHAR GUID_Task        []	= L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{D6277990-4C6A-11CF-8D87-00AA0060F5BF}";

		const INT_PTR IDList_Self   = 0;
		const INT_PTR IDList_Parent = 1;
		const INT_PTR IDList_Linked = 0x0000FFFF;

		//==============================================================================
		// インタフェース

		interface __declspec(uuid("409E5925-F847-44F8-A505-26A619AB793D")) IEntry;
		__interface __declspec(uuid("4F030BAB-544F-4898-A73A-8901BB631D35")) IEntryList;
		__interface __declspec(uuid("B44AEEF2-4EC8-436C-B87D-F28DD1CC2C6A")) IFolder;
		__interface __declspec(uuid("3F2929E4-BCC8-4D51-A9C9-A30CC0C3FA22")) IDragSource;

		//==============================================================================
		// 作成可能なクラス

		class __declspec(uuid("B9847445-80E8-4EBB-BB30-09FF879A31B2")) Entry;
		class __declspec(uuid("0BB31216-4BB7-4B10-957E-83AFF7B4C212")) EntryList;
		class __declspec(uuid("1D502BC3-DA23-4AB1-8E65-6354D1EAA39D")) FolderMenu;
		class __declspec(uuid("6B69FE4C-66D3-4C06-B112-423732354AF4")) DragSource;

		//==============================================================================
		// インタフェース定義.

		typedef IndexOr<const ITEMIDLIST>	IndexOrIDList;

		/// シェルエントリ(ITEMIDLIST).
		interface __declspec(novtable) IEntry : IUnknown
		{
			/// エントリの名前の種類.
			enum NameType
			{
				NAME,			///< エクスプローラで表示される名前.
				PATH,			///< フルパス.
				LEAF,			///< 末端ファイル名.
				BASE,			///< 拡張子を除いた末端ファイル名.
				EXTENSION,		///< 拡張子のみ.
				URL,			///< URL形式. file:///C:/foo.txt
				IDENTIFIER,		///< シェル名前空間でのフォルダ名.
				PATH_OR_NAME,	///< フルパス、空の場合は名前.
				LEAF_OR_NAME,	///< 末端ファイル名、空の場合は名前.
				BASE_OR_NAME,	///< 拡張子を除いた末端ファイル名、空の場合は名前.
			};

#ifndef DOXYGEN
			string get_Name()	{ return GetName(NAME); }
			string get_Path()	{ return GetName(PATH); }
			virtual const ITEMIDLIST* get_ID() = 0;
			virtual int get_Image() = 0;
#endif
			__declspec(property(get=get_Name )) string Name;
			__declspec(property(get=get_Path )) string Path;
			__declspec(property(get=get_ID   )) const ITEMIDLIST* ID;	///< シェルIDリスト.
			__declspec(property(get=get_Image)) int Image;			///< イメージリストインデクス.

			/// エントリの名前を取得する.
			virtual string GetName(NameType what) = 0;
			/// エントリの名前を設定する.
			virtual HRESULT SetName(PCWSTR name, HWND hwnd = null) = 0;

			/// 相対パスを指定してオブジェクトを取得する.
			/// フォルダでない場合は失敗する.
			virtual HRESULT QueryObject(
				REFINTF       ppObject,		///< IEntry or IShellFolder.
				IndexOrIDList relpath = 0	///< 相対パス / nullの場合は自分自身 / 65535以下の場合は親方向.
			) = 0;
			///
			virtual HRESULT ParseDisplayName(
				REFINTF ppObject,	///< IEntry or IShellFolder.
				PCWSTR relpath		///< 相対パス / nullの場合は自分自身 / 65535以下の場合は親方向.
			) = 0;
			/// エクスプローラでフォルダとして表示できるか否か.
			virtual bool IsFolder() = 0;
			/// 
			virtual bool Exists() = 0;
			///
			virtual bool Equals(IEntry* rhs, NameType what = IDENTIFIER) = 0;
			///
			virtual ref<IEnumUnknown> EnumChildren(bool includeFiles) = 0;

			// Alias
			HRESULT GetParent(REFINTF ppObject)	{ return QueryObject(ppObject, IDList_Parent); }
			HRESULT GetLinked(REFINTF ppObject)	{ return QueryObject(ppObject, IDList_Linked); }
		};

		//==============================================================================
		// パス.

		bool	PathIsRegistory(PCWSTR name);
		bool	PathIsFolder(PCWSTR path);
		inline LPCWSTR PathFindLeaf(STRING path)	{ return ::PathFindFileName(path); }

		enum PathFrom
		{
			None,	// 相対パスの場合はエラー.
			Top,	// AVESTA
			Bin,	// AVESTA/bin
			Man,	// AVESTA/man
			Usr,	// AVESTA/usr
			Var,	// AVESTA/var
		};

		PCWSTR	PathOf(PathFrom from);

		HRESULT	CreatePath(WCHAR dst[MAX_PATH], STRING src, PathFrom from = None);
		HRESULT	CreatePath(ITEMIDLIST** pp, STRING src, PathFrom from = None);
		string	CreatePath(STRING src, PathFrom from = None);

		HRESULT	CreateEntry(IEntry** pp, STRING src, PathFrom from = None);
		HRESULT	CreateEntry(IEntry** pp, const ITEMIDLIST* src, PathFrom from = None);
		ref<IEntry> CreateEntry(const ITEMIDLIST* src, PathFrom from = None);
		ref<IEntry> CreateEntry(STRING src, PathFrom from = None);

	//HICON	LoadIcon(PCWSTR filename, int index, int w, int h);

	//HRESULT	PathFromObject(WCHAR dst[MAX_PATH], Object* src);
	//HRESULT	PathGetLink(ITEMIDLIST** dst, PCWSTR src);
	//HRESULT	PathGetLink(WCHAR dst[MAX_PATH], PCWSTR src);

		//==============================================================================
		// INIファイル.

		string	IniGetString(PCTSTR filename, PCTSTR group, PCTSTR key, PCTSTR defaultValue);
		bool	IniGetBool  (PCTSTR filename, PCTSTR group, PCTSTR key, bool defaultValue);
		inline INT32 IniGetSint32(PCTSTR filename, PCTSTR group, PCTSTR key, UINT32 defaultValue)
		{
			return ::GetPrivateProfileInt(group, key, defaultValue, filename);
		}
	}
}
