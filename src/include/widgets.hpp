/// @file widgets.hpp
/// ウィジェット階層.
#pragma once

#include "application.hpp"
#include "signal.hpp"
#include "afx.hpp"

namespace mew
{
	namespace io
	{
		interface IEntry;
		__interface IEntryList;
	}

	/// ウィジェット階層.
	namespace ui
	{
		enum InternalMessage
		{
			MEW_WM_FIRST		= WM_USER + 1000,

			//======================================================================
			// 非同期コマンド系
			//======================================================================

			MEW_ECHO_UPDATE,	// (void)wParam, (void)lParam
			MEW_ECHO_COPYDATA,	// (void)wParam, (IString*)lParam

			//======================================================================
			// 通知系
			//======================================================================

			MEW_NM_SHELL,		// SHChangeNotifyRegister
			MEW_NM_TASKTRAY,	// Shell_NotifyIcon

			//======================================================================
			// クエリ系
			//======================================================================

			MEW_QUERY_INTERFACE,	/// HRESULT QueryInterface(const IID* wParam, void** lParam)
			MEW_QUERY_GESTURE,		/// bool (IGesture** wParam, POINTS lParam)
			MEW_QUERY_DROP,			/// bool (IDropTarget** wParam, POINTS lParam)
		};

		//==============================================================================
		// インタフェース

		// basic windows
		interface __declspec(uuid("F9CC1F5C-8041-48D8-8F2E-C000FEAE0B46")) IWindow;
		interface __declspec(uuid("C399ACBE-FE54-4EBF-B39D-6974F4616E24")) IList;
		interface __declspec(uuid("F2989F89-2E1E-43A8-9543-619A86FCCBAD")) ITree;
		
		// basic controls
		interface __declspec(uuid("3D7134DE-EF5D-4965-921F-75C997D0D35D")) IListView;
		interface __declspec(uuid("2A10F046-6C29-4169-976B-8653954AD083")) ITreeView;
		interface __declspec(uuid("4FC56D6B-3250-4636-8C97-439CBA76A7C8")) ITabPanel;
		interface __declspec(uuid("56795899-5291-4AFF-AC38-FABB00A63AF9")) IForm;
		interface __declspec(uuid("AAFCFDA5-87FA-4478-A341-DD68E645A93E")) IDisplay;

		// io windows : mew::io に移動するかも。
		interface __declspec(uuid("9278A704-5BE3-4B73-92CE-D66DB650E9C8")) IPreview;
		interface __declspec(uuid("1943A27C-EA08-4C48-9FAA-9E193E3168EE")) IShellListView;
		__interface __declspec(uuid("60CFE2DB-365B-4972-88C5-2D2BDB922D02")) IShellStorage;

		// extented attributes
		__interface __declspec(uuid("12B10280-CC9C-4BDB-B442-33750FCF3B01")) IWallPaper;

		// command and view-items
		__interface __declspec(uuid("738D6988-289E-4E5B-A144-B75F22996EE2")) ITreeItem;
		__interface __declspec(uuid("69DBFA8F-A8FC-4520-A256-068FC8058FE0")) IEditableTreeItem;

		// window extensions
		__interface __declspec(uuid("84FC60AA-E00E-42B3-97B3-CF02A4FBF7EF")) IKeymap;
		__interface __declspec(uuid("6C815DC3-1A62-451C-AA25-D1DC1620EBAA")) IKeymapTable;
		__interface __declspec(uuid("D980EBD5-AC2C-413B-A887-A7125DB0E68B")) IGesture;
		__interface __declspec(uuid("7FB98F0C-F020-4B31-AB46-8D108D7441A7")) IGestureTable;

		// dialogs
		__interface __declspec(uuid("209A447F-DBEE-4178-B770-B73AD1B9719C")) IWallPaperDialog;
		__interface __declspec(uuid("02B6F4BE-8EFD-4573-85AC-A3A7D2131A2A")) IExpose;

		//==============================================================================
		// ダイアログ関数.

		/// バージョン情報を表示する.
		void AboutDialog(HWND hwnd, string module = null);
		/// 読み込みファイル名を問い合わせる.
		string OpenDialog(HWND hwnd, string filter);
		/// 書き出しファイル名を問い合わせる.
		string SaveDialog(HWND hwnd, string filter, PCTSTR strDefExt = null);
		/// フォント選択ダイアログ.
		void FontDialog(HWND hwnd, HFONT hFont, string caption, function apply);

		//==============================================================================
		// 作成可能なクラス

		class __declspec(uuid("5EDC9069-65BC-4095-8E2C-1D2BAC74CF8C")) Display;
		class __declspec(uuid("EBB3ACF9-1AFA-45CF-ABF0-552F465F0375")) Form;
		class __declspec(uuid("9CB0D88E-4FD8-4222-AF16-580C826B9272")) MenuBar;
		class __declspec(uuid("32A55C2D-0D1E-429D-B89E-E3678B1EA5F5")) ToolBar;		
		class __declspec(uuid("2C5168C6-1679-4EAB-A55B-05F40763A947")) LinkBar;		
		class __declspec(uuid("B5A66FD0-85EF-4732-B2D3-0DB684DC505D")) TreeView;	
		class __declspec(uuid("EE72AD26-62B6-42D0-B6B0-E242F30E4510")) DockPanel;	
		class __declspec(uuid("96BC8DCC-8824-47C1-98CF-9C6E7C87DBB3")) TabPanel;	
		class __declspec(uuid("4BD891C3-0AD0-436D-BF80-10225A076683")) ReBar;		
		class __declspec(uuid("210CCBAE-4429-42AF-A1B0-1C5E1514A616")) ShellListView;
		class __declspec(uuid("90973EDF-FBB4-4B1D-9406-FE7CC9E0A0BF")) ShellTreeView;
		class __declspec(uuid("236CBFD9-6E4D-4336-AE91-3E60EE8B54BC")) Preview;
		class __declspec(uuid("6C27F5A7-67B5-4418-9FB5-4C4A4D3D7376")) StatusBar;
		class __declspec(uuid("27DE63DE-35FC-4E1F-8F6B-405950ECE85E")) DefaultTreeItem;
		class __declspec(uuid("9B81653E-F275-47E9-9136-4928C49C9382")) KeymapTable;
		class __declspec(uuid("B1435AF3-E660-4CD5-9D63-80ADF6C507F4")) GestureTable;
		class __declspec(uuid("EE813675-A87E-467B-90CD-A17EA1543BF1")) WallPaperDialog;
		class __declspec(uuid("10106D22-9371-4F07-B57F-E96655CE5BF0")) Expose;		

		//==============================================================================
		// 定数

		/// マウスボタンと修飾キー.
		enum MouseAndModifier
		{
			MouseButtonNone		= 0x0000, ///< なし.
			MouseButtonLeft		= 0x0001, ///< 左ボタン.
			MouseButtonRight	= 0x0002, ///< 右ボタン.
			MouseButtonMiddle	= 0x0010, ///< 中ボタン.
			MouseButtonX1		= 0x0020, ///< 追加ボタン1.
			MouseButtonX2		= 0x0040, ///< 追加ボタン2.
			MouseButtonMask		= 0x0073, ///< マウスのボタンすべて.

			ModifierNone		= 0x0000, ///< なし.
			ModifierShift		= 0x0004, ///< シフトキー.
			ModifierControl		= 0x0008, ///< コントロールキー.
			ModifierAlt			= 0x0100, ///< Alt.
			ModifierWindows		= 0x0200, ///< Windows.
			ModifierMask		= 0x030C, ///< 修飾キーすべて.
		};

		/// 方向スタイル.
		enum Direction
		{
			DirNone			= 0x0000, ///< 方向なし.
			DirCenter		= 0x0001, ///< 中央.
			DirWest			= 0x0010, ///< 西・左.
			DirEast			= 0x0020, ///< 東・右.
			DirNorth		= 0x0040, ///< 北・上.
			DirSouth		= 0x0080, ///< 南・下.
			DirMaskWE		= DirWest  | DirEast, ///< 東西・左右マスク.
			DirMaskNS		= DirNorth | DirSouth, ///< 南北・上下マスク.
		};

		/// マウスジェスチャ.
		enum Gesture
		{
			GestureButtonLeft,
			GestureButtonRight,
			GestureButtonMiddle,
			GestureButtonX1,
			GestureButtonX2,
			GestureWheelUp,
			GestureWheelDown,
			GestureWest,
			GestureEast,
			GestureNorth,
			GestureSouth,
		};

		//==============================================================================
		// インタフェース定義.

		/// ウィンドウ.
		interface __declspec(novtable) IWindow : ISignal
		{
#ifndef DOXYGEN
			virtual HWND      get_Handle() = 0;
			virtual Size      get_DefaultSize() = 0;
			virtual Direction get_Dock() = 0;
			virtual void      set_Dock(Direction value) = 0;
//			virtual IKeymap*  get_Keymap() = 0;
//			virtual void      set_Keymap(IKeymap* value) = 0;
//			virtual IGesture* get_Gesture() = 0;
//			virtual void      set_Gesture(IGesture* value) = 0;

			string  get_Name()					{ return afx::GetName(get_Handle()); }
			void    set_Name(string value)		{ afx::SetName(get_Handle(), value); }
			bool    get_Visible()				{ return afx::GetVisible(get_Handle()); }
			void    set_Visible(bool value)		{ afx::SetVisible(get_Handle(), value); }
			Rect    get_Bounds()				{ return afx::GetBounds(get_Handle()); }
			void    set_Bounds(Rect value)		{ afx::SetBounds(get_Handle(), value); }
			Point   get_Location()				{ return afx::GetLocation(get_Handle()); }
			void    set_Location(Point value)	{ afx::SetLocation(get_Handle(), value); }
			Rect    get_ClientArea()			{ return afx::GetClientArea(get_Handle()); }
			Size    get_ClientSize()			{ return afx::GetClientSize(get_Handle()); }
			void    set_ClientSize(Size value)	{ afx::SetClientSize(get_Handle(), value); }
#endif // DOXYGEN

			/// ウィンドウハンドル [get].
			__declspec(property(get=get_Handle)) HWND Handle;
			/// 望ましいサイズ [get]. 特に望ましいサイズを持たない場合は、Size::Zero を返す.
			__declspec(property(get=get_DefaultSize)) Size DefaultSize;
			/// ドッキングスタイル [get, set].
			__declspec(property(get=get_Dock, put=set_Dock)) Direction Dock;
			/// 名前 [get, set].
			__declspec(property(get=get_Name, put=set_Name)) string Name;
			/// 可視状態 [get, set].
			__declspec(property(get=get_Visible, put=set_Visible)) bool Visible;
			/// ウィンドウ外接領域 [get, set].
			/// 親を持つ場合は、親座標系での位置.
			/// ルートウィンドウの場合は、スクリーン座標系での位置.
			__declspec(property(get=get_Bounds, put=set_Bounds)) Rect Bounds;
			/// ウィンドウ左上位置 [get, set].
			__declspec(property(get=get_Location, put=set_Location)) Point Location;
			/// クライアント領域 [get].
			__declspec(property(get=get_ClientArea)) Rect ClientArea;
			/// クライアント領域のサイズ [get, set].
			__declspec(property(get=get_ClientSize, put=set_ClientSize)) Size ClientSize;

			/// メッセージを同期送信する.
			virtual HRESULT Send(message msg) = 0;
			/// ウィンドウを閉じる.
			/// すでに破棄済みの場合は何もしない.
			virtual void Close(bool sync = false) = 0;
			/// 再描画.
			virtual void Update(bool sync = false) = 0;
			/// 拡張ハンドラを取得する.
			virtual HRESULT GetExtension(REFGUID which, REFINTF what) = 0;
			/// 拡張ハンドラを設定する.
			/// @param which 拡張の種類. 少なくとも以下の拡張は実装すべき.
			///              - __uuidof(IGesture)     : マウスジェスチャを乗っ取る.
			///              - __uuidof(IKeymap)      : キーマップを乗っ取る.
			///              - __uuidof(IDropTarget)  : ドロップ処理を乗っ取る.
			/// @param what 格調ハンドラオブジェクト.
			virtual HRESULT SetExtension(REFGUID which, IUnknown* what) = 0;

			/// フォーカスを与える.
//			HRESULT Close(message msg = null)		{ return avesta::WindowClose(get_Handle()); }
			HRESULT Focus(message msg = null)		{ return avesta::WindowSetFocus(get_Handle()); }
//			HRESULT Show (message msg = null)		{ return avesta::Show(get_Handle(), (msg["value"] | -1)); }

//			HRESULT Restore(message msg = null)		{ return afx::Restore(get_Handle()); }
//			HRESULT Resize(message msg = null)		{ return afx::Resize(get_Handle()); }
//			HRESULT Move(message msg = null)		{ return afx::Move(get_Handle()); }
//			HRESULT Maximize(message msg = null)	{ return afx::Maximize(get_Handle()); }
//			HRESULT Minimize(message msg = null)	{ return afx::Minimize(get_Handle()); }
		};

		/// リストアイテムコンテナ.
		interface __declspec(novtable) IList : IWindow
		{
#ifndef DOXYGEN
			virtual size_t get_Count() = 0;
#endif // DOXYGEN

			/// 項目数を取得する.
			__declspec(property(get=get_Count)) size_t Count;///< 項目数.
			///
			virtual HRESULT GetAt(REFINTF pp, size_t index) = 0;
			/// 指定の状態を持つアイテムを、指定のインタフェースに合わせて取得する.
			/// @param ppInterface 取得するインタフェース. IEnumUnknown の場合は複数、それ以外は単数のアイテムを取得する.
			/// @param status 取得するアイテムの状態.
			virtual HRESULT GetContents(REFINTF ppInterface, Status status) = 0;
			/// アイテムの状態を取得する.
			virtual HRESULT GetStatus(
				IndexOr<IUnknown> item,	///< 対象のアイテムまたはインデックス.
				DWORD*   status,		///< アイテムの状態. nullの場合は取得しない.
				size_t*  index = null	///< アイテムのインデックス. nullの場合は取得しない.
			) = 0;
			/// アイテムの状態を設定する.
			virtual HRESULT SetStatus(
				IndexOr<IUnknown> item,	///< 対象のアイテムまたはインデックス.
				Status   status,		///< アイテムの状態.
				bool     unique = false	///< 指定したアイテム以外のstatus状態を解除するか否か.
			) = 0;
		};

		/// ツリーアイテムコンテナ.
		interface __declspec(novtable) ITree : IWindow
		{
#ifndef DOXYGEN
			virtual ITreeItem*  get_Root() = 0;
			virtual void        set_Root(ITreeItem* value) = 0;
			virtual IImageList* get_ImageList() = 0;
			virtual void        set_ImageList(IImageList* value) = 0;
#endif // DOXYGEN

			/// ルートツリーアイテム [get, set].
			__declspec(property(get=get_Root, put=set_Root)) ITreeItem* Root;
			/// イメージリスト [get, set].
			__declspec(property(get=get_ImageList, put=set_ImageList)) IImageList* ImageList;
		};

		enum CopyMode
		{
			CopyNone,
			CopyName,
			CopyPath,
			CopyBase,
		};

		/// フレームウィンドウ.
		interface __declspec(novtable) IForm : ITree
		{
#ifndef DOXYGEN
			virtual bool		get_TaskTray() = 0;
			virtual void		set_TaskTray(bool value) = 0;
			virtual CopyMode	get_AutoCopy() = 0;
			virtual void		set_AutoCopy(CopyMode value) = 0;
#endif // DOXYGEN

			/// タスクトレイ [get, set].
			__declspec(property(get=get_TaskTray, put=set_TaskTray)) bool TaskTray;
			/// コピーモード [get, set].
			__declspec(property(get=get_AutoCopy, put=set_AutoCopy)) CopyMode AutoCopy;
		};

		/// ディスプレイ.
		interface __declspec(novtable) IDisplay : IWindow
		{
			using WNDPROCEX = LRESULT (__stdcall *)(void* self, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

			virtual UINT PopupMenu(HMENU hMenu, UINT tpm, int x, int y, HWND hOwner, const RECT* rcExclude) throw() = 0;
			virtual size_t GetMenuDepth() throw() = 0;
			virtual HWND GetMenu(int index = -1) throw() = 0;
			virtual void RegisterMessageHook(void* self, WNDPROCEX wndproc) = 0;
			virtual void UnregisterMessageHook(void* self, WNDPROCEX wndproc) = 0;
		};

		/// 挿入位置.
		enum InsertTo
		{
			InsertHead,	///< 一番最初に挿入.
			InsertTail,	///< 一番最後に挿入.
			InsertPrev,	///< 現在の一つ前に挿入.
			InsertNext,	///< 現在の一つ後に挿入.
		};

		/// 並べ方.
		enum ArrangeType
		{
			ArrangeHorz,	///< 横に並べる.
			ArrangeVert,	///< 縦に並べる.
			ArrangeAuto,	///< いい感じに並べる.
		};

		/// タブコンテナ.
		interface __declspec(novtable) ITabPanel : IList
		{
#ifndef DOXYGEN
			virtual InsertTo	get_InsertPosition() = 0;
			virtual void		set_InsertPosition(InsertTo pos) = 0;
			virtual ArrangeType	get_Arrange() = 0;
			virtual void		set_Arrange(ArrangeType value) = 0;
#endif // DOXYGEN

			/// アイテムを自動追加する場合の挿入位置 [get, set].
			__declspec(property(get=get_InsertPosition, put=set_InsertPosition)) InsertTo InsertPosition;
			///< 並べ方.
			__declspec(property(get=get_Arrange, put=set_Arrange)) ArrangeType Arrange;
			///
			virtual Rect GetTabRect(IndexOr<IUnknown> item) = 0;
			///
			virtual HRESULT MoveTab(IndexOr<IUnknown> from, int to) = 0;
			/// タブの最小幅、最大幅を指定する.
			virtual HRESULT SetMinMaxTabWidth(size_t min, size_t max) = 0;
		};

		/// リストビューの表示スタイル.
		enum ListStyle
		{
			ListStyleIcon		= 0x01,
			ListStyleLargeIcon	= ListStyleIcon,
			ListStyleSmallIcon	= 0x02,
			ListStyleList		= 0x03,
			ListStyleDetails	= 0x04,
			ListStyleThumnail	= 0x05,
			ListStyleTile		= 0x06,
		};

		/// リストビュー.
		interface __declspec(novtable) IListView : IList
		{
#ifndef DOXYGEN
			virtual size_t		get_SelectedCount() = 0;
			virtual ListStyle	get_Style() = 0;
			virtual void		set_Style(ListStyle value) = 0; 
			virtual bool		get_AutoArrange() = 0;
			virtual void		set_AutoArrange(bool value) = 0;
			virtual bool		get_CheckBox() = 0;
			virtual void		set_CheckBox(bool value) = 0;
			virtual bool		get_FullRowSelect() = 0;
			virtual void		set_FullRowSelect(bool value) = 0;
			virtual bool		get_GridLine() = 0;
			virtual void		set_GridLine(bool value) = 0;
			virtual bool		get_Grouping() = 0;
			virtual void		set_Grouping(bool value) = 0;
#endif // DOXYGEN

			__declspec(property(get=get_SelectedCount                       )) size_t SelectedCount;///< 選択された項目数.
			__declspec(property(get=get_Style        , put=set_Style        )) ListStyle Style;		///< 表示スタイル.
			__declspec(property(get=get_AutoArrange  , put=set_AutoArrange  )) bool AutoArrange;	///< 自動整列.
			__declspec(property(get=get_CheckBox     , put=set_CheckBox     )) bool CheckBox;		///< チェックボックス.
			__declspec(property(get=get_FullRowSelect, put=set_FullRowSelect)) bool FullRowSelect;	///< 一行選択.
			__declspec(property(get=get_GridLine     , put=set_GridLine     )) bool GridLine;		///< グリッド.
			__declspec(property(get=get_Grouping     , put=set_Grouping     )) bool Grouping;		///< グループ.
		};

		/// ツリービュー.
		interface __declspec(novtable) ITreeView : ITree
		{
#ifndef DOXYGEN
#endif // DOXYGEN

			/// 指定の状態を持つアイテムを、指定のインタフェースに合わせて取得する.
			/// @param ppInterface 取得するインタフェース. IEnumUnknown の場合は複数、それ以外は単数のアイテムを取得する.
			/// @param status 取得するアイテムの状態.
			virtual HRESULT GetContents(REFINTF ppInterface, Status status) = 0;
			/// アイテムの状態を取得する.
			virtual HRESULT GetStatus(
				IUnknown* item,		///< 対象のアイテム.
				DWORD*   status		///< アイテムの状態. nullの場合は取得しない.
			) = 0;
			/// アイテムの状態を設定する.
			virtual HRESULT SetStatus(
				IUnknown* item,		///< 対象のアイテム.
				Status   status		///< アイテムの状態.
			) = 0;
		};

		/// プレビュー.
		interface __declspec(novtable) IPreview : IWindow
		{
			virtual HRESULT GetContents(REFINTF ppInterface) = 0;
			virtual HRESULT SetContents(IUnknown* contents) = 0;
		};

		/// シェルストレージ.
		__interface IShellStorage : IUnknown
		{
			HRESULT QueryStream(IStream** ppStream, io::IEntry* pFolder, bool writable);
			HRESULT SyncDescendants(io::IEntry* pFolder);
		};

		/// フォルダビュー.
		interface __declspec(novtable) IShellListView : IListView
		{
			/// 拡張ハンドラを設定する.
			/// @param which IWindowに加え、
			///              - __uuidof(IShellStorage) : ストレージ.
			/// @param what 格調ハンドラオブジェクト.
			virtual HRESULT SetExtension(REFGUID which, IUnknown* what) = 0;

#ifndef DOXYGEN
			virtual bool	get_ShowAllFiles() = 0;
			virtual void	set_ShowAllFiles(bool value) = 0;
			virtual bool	get_RenameExtension() = 0;
			virtual void	set_RenameExtension(bool value) = 0;
			virtual string	get_PatternMask() = 0;
			virtual void	set_PatternMask(string value) = 0;
#endif // DOXYGEN

			__declspec(property(get=get_ShowAllFiles, put=set_ShowAllFiles)) bool ShowAllFiles; ///< 隠しファイルの表示.
			__declspec(property(get=get_RenameExtension, put=set_RenameExtension)) bool RenameExtension; ///< リネーム時に拡張子を選択する.
			__declspec(property(get=get_PatternMask, put=set_PatternMask)) string PatternMask; ///< フィルタ文字列

			/// フォルダを移動する.
			virtual HRESULT Go(
				io::IEntry* folder	///< 移動先のフォルダ.
			) = 0;
			/// フォルダを移動する.
			virtual HRESULT Go(
				Direction dir,	///< 方向. DirWest=戻る, DirEast=進む, DirNorth=進む.
				int level = 1	///< 段階.
			) = 0;
			/// 現在のフォルダを取得する.
			virtual HRESULT GetFolder(
				REFINTF ppFolder	///< io::IEntry または IShellFolder を取得できる.
			) = 0;

			virtual string GetLastStatusText() = 0;
			virtual HRESULT SelectChecked() = 0;
		};

		/// 階層構造を持つアイテム.
		__interface ITreeItem : IUnknown
		{
#ifndef DOXYGEN
			string		    get_Name();
			ref<ICommand>	get_Command();
			int			    get_Image();
#endif // DOXYGEN

			__declspec(property(get=get_Name   )) string        Name;    ///< 表示に使われる名前.
			__declspec(property(get=get_Command)) ref<ICommand> Command; ///< 関連付けられているコマンド.
			__declspec(property(get=get_Image  )) int           Image;   ///< イメージリストインデクス.

			/// 画面に表示される直前に呼ばれる.
			/// @result 構成が変化した場合にインクリメントされる最終更新時間.
			UINT32 OnUpdate();

			/// 子供を持っている場合はtrue.
			bool HasChildren();
			size_t GetChildCount();
			/// 子供を取得.
			HRESULT GetChild(REFINTF ppChild, size_t index);
		};

		/// 編集可能な階層構造を持つアイテム.
		__interface IEditableTreeItem : ITreeItem
		{
#ifndef DOXYGEN
			void set_Name(string value);
			void set_Command(ICommand* value);
			void set_Image(int value);
#endif // DOXYGEN

			__declspec(property(get=get_Name   , put=set_Name   )) string        Name;
			__declspec(property(get=get_Command, put=set_Command)) ref<ICommand> Command;
			__declspec(property(get=get_Image  , put=set_Image  )) int           Image;

			void AddChild(ITreeItem* child);
			bool RemoveChild(ITreeItem* child);
		};

		/// キーマップ.
		__interface IKeymap : IUnknown
		{
			/// キーが押された.
			HRESULT OnKeyDown(IWindow* window, UINT16 modifiers, UINT8 vkey);
		};

		/// コマンド登録型のキーマップ.
		__interface IKeymapTable : IKeymap
		{
#ifndef DOXYGEN
			size_t get_Count();
#endif // DOXYGEN

			/// バインド数を取得する.
			__declspec(property(get=get_Count)) size_t Count;
			/// インデクスを指定してバインドとコマンドを取得する.
			HRESULT GetBind(size_t index, UINT16* modifiers, UINT8* vkey, REFINTF ppCommand);
			/// バインドを指定してコマンドを取得する.
			HRESULT GetBind(UINT16  modifiers, UINT8 vkey, REFINTF ppCommand);
			/// キーバインドを追加する.
			HRESULT SetBind(UINT16 modifiers, UINT8 vkey, ICommand* pCommand);
		};

		/// マウスジェスチャをフックする.
		__interface IGesture : IUnknown
		{
			/// 
			HRESULT OnGestureAccept(HWND hWnd, Point ptScreen, size_t length, const Gesture gesture[]);
			/// 
			HRESULT OnGestureUpdate(UINT16 modifiers, size_t length, const Gesture gesture[]);
			/// 
			HRESULT OnGestureFinish(UINT16 modifiers, size_t length, const Gesture gesture[]);
		};

		/// コマンド登録型のマウスジェスチャ.
		__interface IGestureTable : IGesture
		{
			/// バインド数を取得する.
			size_t get_Count();
			/// インデクスを指定してコマンドを取得する.
			HRESULT GetGesture(size_t index, REFINTF ppCommand);
			/// ジェスチャを指定してコマンドを取得する.
			HRESULT GetGesture(UINT16 modifiers, size_t length,	const Gesture gesture[], REFINTF ppCommand);
			/// ジェスチャを追加する.
			HRESULT SetGesture(UINT16 modifiers, size_t length,	const Gesture gesture[], ICommand* pCommand);
		};

		/// 背景画像サポート.
		__interface IWallPaper : IUnknown
		{
#ifndef DOXYGEN
			string get_WallPaperFile();
			void   set_WallPaperFile(string value);
			UINT32 get_WallPaperAlign();
			void   set_WallPaperAlign(UINT32 value);
#endif // DOXYGEN

			__declspec(property(get=get_WallPaperFile , put=set_WallPaperFile )) string WallPaperFile;
			__declspec(property(get=get_WallPaperAlign, put=set_WallPaperAlign)) UINT32 WallPaperAlign;
		};

		/// 壁紙指定ダイアログ.
		__interface IWallPaperDialog : IUnknown
		{
			HRESULT Go();
			HRESULT AddTarget(IWallPaper* target, string name);
		};

		/// えくすぽぜ？
		__interface IExpose : IUnknown
		{
			void AddRect(INT32 id, INT32 group, const Rect& bounds, UINT8 hotkey);
			void Select(UINT32 id);
			void SetTitle(string title);
			HRESULT Go(HWND hwndParent, UINT32 time);
		};

		//==============================================================================
		// メッセージ

		/// ウィンドウコマンド.
		enum WindowCommand
		{
			// standard window
			CommandClose			= 'CLOS', ///< IWindow::Close().
			CommandUpdate			= 'UPDT', ///< IWindow::Update(bool sync = false).
			CommandShow				= 'SHOW', ///< IWindow::set_Visible(bool value = {none=>toggle})
			// editable window
			CommandCut				= 'CUT_', ///< カット.
			CommandCopy				= 'COPY', ///< コピー.
			CommandPaste			= 'PAST', ///< 貼り付け.
			CommandDelete			= 'DELE', ///< 削除.
			CommandBury				= 'BURY', ///< 完全に削除.
			CommandUndo				= 'UNDO', ///< Undo.
			CommandRedo				= 'REDO', ///< Redo.
			CommandSelectAll		= 'SELA', ///< すべてのアイテムを選択する.
			CommandSelectChecked	= 'SELC', ///< チェックされたアイテムを選択する.
			CommandSelectNone		= 'SELN', ///< すべてのアイテムの選択を解除します.
			CommandSelectReverse	= 'SELR', ///< 選択項目を反転します.
			CommandSelectToFirst	= 'SELF', ///< 最初まで選択する.
			CommandSelectToLast		= 'SELL', ///< 最初まで選択する.
			CommandCheckAll			= 'CHKA', ///< すべてのアイテムをチェックする.
			CommandCheckNone		= 'CHKN', ///< すべてのアイテムのチェックを解除します.
			CommandProperty			= 'PROP', ///< 
			CommandOpen				= 'OPEN', ///< 開く.
			CommandSave				= 'SAVE', ///< 保存.
			// navigating window
			CommandGoBack			= 'GOBK', ///< 戻る or 左へ.
			CommandGoForward		= 'GOFW', ///< 進む or 右へ.
			CommandGoUp				= 'GOUP', ///< 上へ.
			CommandGoDown			= 'GODW', ///< 下へ.
			// form window
			CommandMaximize			= 'MAXI', ///< 
			CommandMinimize			= 'MINI', ///< 
			CommandRestore			= 'REST', ///< 
			CommandResize			= 'RESZ', ///< 
			CommandMove				= 'MOVE', ///< 
			CommandMenu				= 'MENU', ///< 
			//
			CommandKeyUp			= 'KEYU',  ///< カーソル上
			CommandKeyDown			= 'KEYD',  ///< カーソル下
			CommandKeyLeft			= 'KEYL',  ///< カーソル左
			CommandKeyRight			= 'KEYR',  ///< カーソル右
			CommandKeyHome			= 'KEYH',  ///< Home
			CommandKeyEnd			= 'KEYE',  ///< End
			CommandKeyPageUp		= 'KEYP',  ///< PageUp
			CommandKeyPageDown		= 'KEYN',  ///< PageDown
			CommandKeySpace			= 'KEYS',  ///< Space
			CommandKeyEnter			= 'KEYT',  ///< Enter

			CommandShownToLeft		= 'STOL', ///< タブを左へ
			CommandShownToRight		= 'STOR', ///< タブを右へ
			CommandLockedToLeft		= 'LTOL', ///< タブを左へ
			CommandLockedToRight	= 'LTOR', ///< タブを右へ

			CommandSetStyle			= 'STYS', ///< IListView::set_Style(INT32 style)
			CommandRename			= 'RENA', ///< Rename.
			CommandFocusAddress		= 'FCAD', ///< アドレスバーにフォーカスを移す.
			CommandFocusHeader		= 'FCHD', ///< ヘッダにフォーカスを移す.
			CommandAdjustToItem		= 'ADJI', ///< 
			CommandAdjustToWindow	= 'ADJW', ///< 
		};

		/// ウィンドウメッセージコード.
		enum WindowMessage
		{
			EventDispose		= 'WDEL', ///< ウィンドウが破棄された.
			EventPreClose		= 'WPCL', ///< ウィンドウの閉じようとしている.
			EventClose			= 'WCLS', ///< ウィンドウが閉じられた.
			EventRename			= 'WREN', ///< ウィンドウの名前が変更された.
			EventResize			= 'WRSZ', ///< ウィンドウのサイズが変更された.
			EventResizeDefault	= 'WRSD', ///< 推奨サイズが変化した.
			EventData			= 'DATA', ///< WM_COPYDATAを受け取った.
			EventUnsupported	= 'UNSP', ///< 理解できないメッセージ.

			EventItemFocus		= 'ITFC', ///< 項目のフォーカス状態が変化した.
			EventMouseWheel		= 'MSWH', ///< マウスホイールが回転した.
			EventOtherFocus		= 'OTHR', ///< 他のスレッドのウィンドウがフォーカスを持った.
		};

		/// シェルリストメッセージコード.
		enum ShellListMessage
		{
			EventFolderChanging = 'SLCI', ///< フォルダを移動しようとしている.
			EventFolderChange   = 'SLCG', ///< 表示フォルダが変更された.
			EventExecuteEntry	= 'SLEX', ///< エントリを実行しようとしている.
			EventStatusText		= 'SLST', ///< ステータスバーのテキストが変更された.
		};
	}
}

namespace mew
{
	namespace ui
	{
		HRESULT ProcessDragEnter(IDataObject* src, io::IEntry* dst, DWORD key, DWORD* effect);
		HRESULT ProcessDragOver(io::IEntry* dst, DWORD key, DWORD* effect);
		HRESULT ProcessDragLeave();
		HRESULT ProcessDrop(IDataObject* src, io::IEntry* dst, POINTL pt, DWORD key, DWORD* effect);

		inline bool IsKeyPressed(int vkey)
		{
			return (::GetKeyState(vkey) & 0x8000) != 0;
		}

		inline UINT16 GetCurrentModifiers()
		{
			UINT16 mod = 0;
			if(IsKeyPressed(VK_MENU))    mod |= ModifierAlt;
			if(IsKeyPressed(VK_CONTROL)) mod |= ModifierControl;
			if(IsKeyPressed(VK_SHIFT))   mod |= ModifierShift;
			if(IsKeyPressed(VK_LWIN))    mod |= ModifierWindows;
			if(IsKeyPressed(VK_RWIN))    mod |= ModifierWindows;
			return mod;
		}

		HRESULT QueryInterfaceInWindow(HWND hWnd, REFINTF pp);

		inline HRESULT QueryParent(HWND hWnd, REFINTF pp)
		{
			return QueryInterfaceInWindow(::GetAncestor(hWnd, GA_PARENT), pp);
		}
		inline HRESULT QueryParent(IWindow* window, REFINTF pp)
		{
			if(!window)
				return E_POINTER;
			return QueryParent(window->Handle, pp);
		}

		inline HRESULT QueryParentOrOwner(HWND hWnd, REFINTF pp)
		{
			return QueryInterfaceInWindow(afx::GetParentOrOwner(hWnd), pp);
		}
	}
}
