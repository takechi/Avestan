/// @file application.hpp
/// アプリケーションフレームワーク.
#pragma once

#include "message.hpp"

namespace mew
{
	/// 汎用ステータス.
	enum Status
	{
		StatusNone		= 0x00000000, ///< .

		FOCUSED		= 0x00000001, ///< フォーカス.
		SELECTED	= 0x00000002, ///< 選択.
		CHECKED		= 0x00000004, ///< チェック.
		ENABLED		= 0x00000008, ///< 有効化.
		EXPANDED	= 0x00000010, ///< 展開.
		HOT			= 0x00000020, ///< ホットトラック.

		UNFOCUSED	= 0x00010000, ///< .
		UNSELECTED	= 0x00020000, ///< .
		UNCHECKED	= 0x00040000, ///< .
		DISABLED	= 0x00080000, ///< .
		COLLAPSED	= 0x00100000, ///< .
		UNHOT		= 0x00200000, ///< .

		ToggleFocus		= 0x00010001, ///< .
		ToggleSelect	= 0x00020002, ///< .
		ToggleCheck		= 0x00040004, ///< .
		ToggleEnable	= 0x00080008, ///< .
		ToggleExpand	= 0x00100010, ///< .
		ToggleHot		= 0x00200020, ///< .
	};

	__interface __declspec(uuid("2EFEAB15-24BF-498B-A1F7-6E85087026C0")) ICommand;
	__interface __declspec(uuid("E2B2FC4C-561B-4A38-96EB-B484E47237D8")) ICommands;

	/// コマンドテーブル.
	class __declspec(uuid("E35429F9-AA06-42C3-BDAF-9361FF54501E")) Commands;

	/// コマンド.
	__interface ICommand : IUnknown
	{
#ifndef DOXYGEN
		string	get_Description();
#endif // DOXYGEN

		/// コマンドを実行する.
		void Invoke();
		/// コマンドの説明 [get].
		__declspec(property(get=get_Description)) string Description;

		/// ステータスを取得する.
		UINT32 QueryState(IUnknown* owner = null);
	};

	/// コマンドテーブル.
	__interface ICommands : IUnknown
	{
		HRESULT Add(string name, ICommand* command);
		HRESULT Add(string name, string description);
		HRESULT Alias(string alias, string name);
		HRESULT Find(REFINTF ppInterface, string name);
		HRESULT Remove(string name);
		HRESULT SetHandler(string name, function fn, message msg = null);
		HRESULT SetObserver(string name, function fn, message msg = null);
	};

	/// コマンドメッセージコード.
	enum CommandMessage
	{
		EventInvoke		= 'CMIV', ///< コマンドが実行された.
		EventQueryState	= 'CMST', ///< コマンドのステータスが問い合わせられた.
	};
}
