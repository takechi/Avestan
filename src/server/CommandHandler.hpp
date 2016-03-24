// Command.h
#pragma once

namespace
{
	inline message FocusHeaderMessage()
	{
		message m(CommandFocusHeader);
		m["time"] = theAvesta->GetExposeTime();
		return m;
	}

	inline message StyleMsg(INT32 style)
	{
		message msg(CommandSetStyle);
		msg["style"] = style;
		return msg;
	}
}

void Main::InitCommands()
{
	// ƒRƒ}ƒ“ƒh
	m_commands = CreateCommands(COMMANDS, sizeof(COMMANDS) / sizeof(COMMANDS[0]));
	function current(this, &Main::ForwardToCurrent);
	function all    (this, &Main::ForwardToAll);
	function shown  (this, &Main::ForwardToShown);
	function hidden (this, &Main::ForwardToHidden);
	function left   (this, &Main::ForwardToLeft);
	function right  (this, &Main::ForwardToRight);
	function others (this, &Main::ForwardToOthers);
	function dups   (this, &Main::ForwardToDuplicate);
	function anyFolder(this, &Main::EnableIfAnyFolder);
	function anySelect(this, &Main::EnableIfAnySelection);

	// System
	CommandProcess(L"System.Exit"	, m_form, CommandClose);
	CommandProcess(L"System.Restart", &Main::SystemRestart);
	CommandProcess(L"System.About"	, &Main::SystemAbout);

	// File
	CommandProcess(L"Explorer.Clone"	, &Main::ExplorerImport, 0);
	CommandProcess(L"Explorer.Import"	, &Main::ExplorerImport, 1);
	CommandProcess(L"File.Open"			, &Main::FileOpen, 0);
	CommandProcess(L"File.Load"			, &Main::FileLoad);
	CommandHandler(L"File.Save"			, &Main::FileSave			, null, &Main::EnableIfAnyFolder, null);
	CommandHandler(L"File.MRU"			, &Main::FileMRU			, null, &Main::ObserveMRU, null);
	CommandHandler(L"RecycleBin.Empty"	, &Main::ProcessRecycleBin	, null, &Main::ObserveRecycleBin, null);

	// Current
	CommandHandler(L"Current.Lock(true)"	, &Main::CurrentLock, BoolTrue		, &Main::ObserveLock, BoolTrue);
	CommandHandler(L"Current.Lock(false)"	, &Main::CurrentLock, BoolFalse		, &Main::ObserveLock, BoolFalse);
	CommandHandler(L"Current.Lock(toggle)"	, &Main::CurrentLock, BoolUnknown	, &Main::ObserveLock, BoolUnknown);
	CommandToFocus(L"Current.Menu"			, CommandMenu);

	CommandToFocus(L"Current.New"				, AVESTA_New);
	CommandToFocus(L"Current.NewFolder"			, AVESTA_NewFolder);
	CommandToFocus(L"Current.SelectPattern"		, AVESTA_SelectPattern);
	CommandToFocus(L"Current.Close"				, CommandClose);
	CommandToFocus(L"Current.Show(false)"		, AVESTA_Hide);
	CommandToFocus(L"Current.Export"			, AVESTA_Export);
	CommandToFocus(L"Current.Refresh"			, CommandUpdate);
	CommandToFocus(L"Current.Find"				, AVESTA_Find);
	CommandToFocus(L"Current.PatternMask"		, AVESTA_PatternMask);

	CommandToSelect(L"Current.Cut"				, CommandCut);
	CommandToSelect(L"Current.Copy"				, CommandCopy);
	CommandToSelect(L"Current.Copy.Base"		, AVESTA_CopyBase);
	CommandToSelect(L"Current.Copy.Name"		, AVESTA_CopyName);
	CommandToSelect(L"Current.Copy.Path"		, AVESTA_CopyPath);
	CommandToSelect(L"Current.Copy.Here"		, AVESTA_CopyHere);
	CommandToSelect(L"Current.Delete"			, CommandDelete);
	CommandToSelect(L"Current.Bury"				, CommandBury);
	CommandToSelect(L"Current.Rename"			, CommandRename);
	CommandToSelect(L"Current.Rename.Dialog"	, AVESTA_RenameDialog);
	CommandToSelect(L"Current.Property"			, CommandProperty);
	CommandToSelect(L"Current.CopyTo"			, AVESTA_CopyTo);
	CommandToSelect(L"Current.MoveTo"			, AVESTA_MoveTo);
	CommandToSelect(L"Current.CopyToOther"		, AVESTA_CopyToOther);
	CommandToSelect(L"Current.MoveToOther"		, AVESTA_MoveToOther);
	CommandToSelect(L"Current.CopyCheckedTo"	, AVESTA_CopyCheckedTo);
	CommandToSelect(L"Current.MoveCheckedTo"	, AVESTA_MoveCheckedTo);
	CommandToFocus(L"Current.SyncDescendants"	, AVESTA_SyncDesc);

	CommandToFocus(L"Current.Key.Up"		, CommandKeyUp);
	CommandToFocus(L"Current.Key.Down"		, CommandKeyDown);
	CommandToFocus(L"Current.Key.Left"		, CommandKeyLeft);
	CommandToFocus(L"Current.Key.Right"		, CommandKeyRight);
	CommandToFocus(L"Current.Key.Home"		, CommandKeyHome);
	CommandToFocus(L"Current.Key.End"		, CommandKeyEnd);
	CommandToFocus(L"Current.Key.PageUp"	, CommandKeyPageUp);
	CommandToFocus(L"Current.Key.PageDown"	, CommandKeyPageDown);
	CommandToFocus(L"Current.Key.Space"		, CommandKeySpace);
	CommandToFocus(L"Current.Key.Enter"		, CommandKeyEnter);
	m_commands->Alias(L"Current.Cursor.Up"		, L"Current.Key.Up"			);
	m_commands->Alias(L"Current.Cursor.Down"	, L"Current.Key.Down"		);	
	m_commands->Alias(L"Current.Cursor.Left"	, L"Current.Key.Left"		);	
	m_commands->Alias(L"Current.Cursor.Right"	, L"Current.Key.Right"		);	
	m_commands->Alias(L"Current.Cursor.Home"	, L"Current.Key.Home"		);	
	m_commands->Alias(L"Current.Cursor.End"		, L"Current.Key.End"		);	
	m_commands->Alias(L"Current.Cursor.PageUp"	, L"Current.Key.PageUp"		);
	m_commands->Alias(L"Current.Cursor.PageDown", L"Current.Key.PageDown"	);	

	CommandToSelect(L"Current.SelectNone"		, CommandSelectNone);
	CommandToCheck(L"Current.CheckAll"			, CommandCheckAll);
	CommandToCheck(L"Current.CheckNone"			, CommandCheckNone);
	CommandToFocus(L"Current.SelectAll"			, CommandSelectAll);
	CommandToFocus(L"Current.SelectChecked"		, CommandSelectChecked);
	CommandToFocus(L"Current.SelectReverse"		, CommandSelectReverse);
	CommandToFocus(L"Current.SelectToFirst"		, CommandSelectToFirst);
	CommandToFocus(L"Current.SelectToLast"		, CommandSelectToLast);

	CommandToFocus(L"Current.Undo"				, CommandUndo);

	CommandProcess(L"Current.SyncFileDialog"	, &Main::ProcessSyncFileDialog);

	CommandHandler(L"Current.Paste"			, current, CommandPaste			, &Main::ObserveClipboard		, CF_SHELLIDLIST);
	CommandHandler(L"Current.PasteTo"		, current, AVESTA_PasteTo		, &Main::ObserveClipToSelect	, CF_HDROP);
	CommandHandler(L"Current.Rename.Paste"	, current, AVESTA_RenamePaste	, &Main::ObserveClipToSelect	, CF_UNICODETEXT);
	CommandHandler(L"Current.Go.Up"			, current, CommandGoUp			, &Main::ObserveGo	, DirNorth);
	CommandHandler(L"Current.Go.Back"		, current, CommandGoBack		, &Main::ObserveGo	, DirWest);
	CommandHandler(L"Current.Go.Forward"	, current, CommandGoForward		, &Main::ObserveGo	, DirEast);
	CommandHandler(L"Current.Go.Location"	, &Main::FileOpen, 1, anyFolder, null);
	CommandHandler(L"Current.AutoArrange"	, current, AVESTA_AutoArrange	, current, AVEOBS_AutoArrange);
	CommandHandler(L"Current.Grouping"		, current, AVESTA_Grouping		, current, AVEOBS_Grouping);
	CommandHandler(L"Current.ShowAllFiles"	, current, AVESTA_ShowAllFiles	, current, AVEOBS_ShowAllFiles);

	CommandHandler(L"Current.Mode.Icon"		, current, StyleMsg(ListStyleIcon)		, &Main::ObserveMode, ListStyleIcon);
	CommandHandler(L"Current.Mode.List"		, current, StyleMsg(ListStyleList)		, &Main::ObserveMode, ListStyleList);
	CommandHandler(L"Current.Mode.Details"	, current, StyleMsg(ListStyleDetails)	, &Main::ObserveMode, ListStyleDetails);
	CommandHandler(L"Current.Mode.Thumbnail", current, StyleMsg(ListStyleThumnail)	, &Main::ObserveMode, ListStyleThumnail);
	CommandHandler(L"Current.Mode.Tile"		, current, StyleMsg(ListStyleTile)		, &Main::ObserveMode, ListStyleTile);

	CommandToFocus(L"Current.AdjustToItem"	, CommandAdjustToItem);
	CommandToFocus(L"Current.AdjustToWindow", CommandAdjustToWindow);

	// All
	CommandHandler(L"All.Close"				, all, CommandClose			, anyFolder, null);
	CommandHandler(L"All.Refresh"			, all, CommandUpdate		, anyFolder, null);
	CommandHandler(L"All.Export"			, all, AVESTA_Export		, anyFolder, null);
	CommandHandler(L"All.Show(true)"		, all, AVESTA_Show			, anyFolder, null);

	// Shown
	CommandHandler(L"Shown.Close"			, shown	, CommandClose			, anyFolder, null);
	CommandHandler(L"Shown.Refresh"			, shown	, CommandUpdate			, anyFolder, null);
	CommandHandler(L"Shown.Export"			, shown	, AVESTA_Export			, anyFolder, null);
	CommandHandler(L"Shown.AdjustToItem"	, shown	, CommandAdjustToItem	, anyFolder, null);
	CommandHandler(L"Shown.AdjustToWindow"	, shown	, CommandAdjustToWindow	, anyFolder, null);
	CommandHandler(L"Shown.ToLeft"			, m_tab, CommandShownToLeft		, anyFolder, null);
	CommandHandler(L"Shown.ToRight"			, m_tab, CommandShownToRight	, anyFolder, null);

	// Locked
	CommandHandler(L"Locked.ToLeft"		, m_tab	, CommandLockedToLeft	, anyFolder, null);
	CommandHandler(L"Locked.ToRight"	, m_tab	, CommandLockedToRight	, anyFolder, null);

	// Hidden
	CommandHandler(L"Hidden.Close"		, hidden, CommandClose	, anyFolder, null);
	CommandHandler(L"Hidden.Refresh"	, hidden, CommandUpdate	, anyFolder, null);
	CommandHandler(L"Hidden.Export"		, hidden, AVESTA_Export	, anyFolder, null);

	// Duplicate
	CommandHandler(L"Duplicate.Close"	, dups	, CommandClose	, anyFolder, null);

	// Left
	CommandHandler(L"Left.Close"		, left	, CommandClose	, anyFolder, null);
	CommandHandler(L"Left.Show(false)"	, left	, AVESTA_Hide	, anyFolder, null);
	CommandHandler(L"Left.Show(true)"	, left	, AVESTA_Show	, anyFolder, null);

	// Right
	CommandHandler(L"Right.Close"		, right	, CommandClose	, anyFolder, null);
	CommandHandler(L"Right.Show(false)"	, right	, AVESTA_Hide	, anyFolder, null);
	CommandHandler(L"Right.Show(true)"	, right	, AVESTA_Show	, anyFolder, null);

	// Others
	CommandHandler(L"Others.Close"		, others, CommandClose	, anyFolder, null);
	CommandHandler(L"Others.Show(false)", others, AVESTA_Hide	, anyFolder, null);

	// Tab
	CommandProcess(L"Tab.Focus"	, m_tab, &IWindow::Focus);
	CommandHandler(L"Tab.Sort"	, &Main::ProcessTabSort, null, anyFolder, null);
	CommandHandler(L"Tab.Next"	, m_tab, CommandGoForward	, anyFolder, null);
	CommandHandler(L"Tab.Next2"	, m_tab, CommandGoDown		, anyFolder, null);
	CommandHandler(L"Tab.Prev"	, m_tab, CommandGoBack		, anyFolder, null);
	CommandHandler(L"Tab.Prev2"	, m_tab, CommandGoUp		, anyFolder, null);
	CommandHandler(L"Current.ToLeft"	, &Main::ProcessTabMove, -1, anyFolder, null);
	CommandHandler(L"Current.ToRight"	, &Main::ProcessTabMove, +1, anyFolder, null);

	// Single Tab
	for(int i = 0; i < 9; ++i)
	{
		TCHAR name[MAX_PATH];
		wsprintf(name, L"Tab[%d].Show(toggle)", (i+1));
		CommandHandler(name, &Main::ProcessTabShow , i, &Main::ObserveTabShow , i);
		wsprintf(name, L"Tab[%d].Focus", (i+1));
		CommandHandler(name, &Main::ProcessTabFocus, i, &Main::ObserveTabFocus, i);
	}

	// Form
	CommandProcess(L"Form.Show(true)"	, &Main::WindowVisibleTrue);
	CommandProcess(L"Form.Show(false)"	, m_form, CommandMinimize);
	CommandProcess(L"Form.Show(toggle)"	, &Main::WindowVisibleToggle);
	CommandProcess(L"Form.Maximize"		, m_form, CommandMaximize);
	CommandProcess(L"Form.Restore"		, m_form, CommandRestore);
	CommandProcess(L"Form.Resize"		, m_form, CommandResize);
	CommandProcess(L"Form.Move"			, m_form, CommandMove);
	CommandProcess(L"Form.Menu"			, m_form, CommandMenu);
	CommandHandler(L"Form.DropMode"		, &Main::ProcessDropMode, null, &Main::ObserveDropMode, null);

	m_commands->Alias(L"Form.Zoom"		, L"Form.Maximize");
	m_commands->Alias(L"Form.Minimize"	, L"Form.Show(false)");
	m_commands->Alias(L"Form.Close"		, L"System.Exit");

	m_commands->Alias(L"Window.Show(true)"		, L"Form.Show(true)"	);
	m_commands->Alias(L"Window.Show(false)"		, L"Form.Show(false)"	);
	m_commands->Alias(L"Window.Show(toggle)"	, L"Form.Show(toggle)"	);
	m_commands->Alias(L"Window.Maximize"		, L"Form.Maximize"		);
	m_commands->Alias(L"Window.Restore"			, L"Form.Restore"		);
	m_commands->Alias(L"Window.Resize"			, L"Form.Resize"		);
	m_commands->Alias(L"Window.Move"			, L"Form.Move"			);
	m_commands->Alias(L"Window.Menu"			, L"Form.Menu"			);
	m_commands->Alias(L"Window.Zoom"			, L"Form.Maximize"		);
	m_commands->Alias(L"Window.Minimize"		, L"Form.Show(false)"	);
	m_commands->Alias(L"Window.Close"			, L"System.Exit"		);

	// Address
	CommandToFocus(L"Address.Focus"	, CommandFocusAddress);
	// Header
	CommandToFocus(L"Header.Focus"	, FocusHeaderMessage());
	// Tree
	CommandProcess(L"Tree.Refresh"		, &Main::ProcessTreeRefresh);
	CommandHandler(L"Tree.Sync"			, &Main::ProcessTreeSync	, null, &Main::ObserveTreeSync		, null);
	CommandHandler(L"Tree.Reflect"		, &Main::ProcessTreeReflect	, null, &Main::ObserveTreeReflect	, null);
	CommandHandler(L"Tree.AutoSync"		, &Main::OptionBoolean	, BoolTreeAutoSync		, &Main::ObserveBoolean	, BoolTreeAutoSync);
	CommandHandler(L"Tree.AutoReflect"	, &Main::OptionBoolean	, BoolTreeAutoReflect	, &Main::ObserveBoolean	, BoolTreeAutoReflect);
	CommandProcess(L"Tree.Cut"			, m_tree, CommandCut);
	CommandProcess(L"Tree.Copy"			, m_tree, CommandCopy);
	CommandProcess(L"Tree.Copy.Base"	, m_tree, AVESTA_CopyBase);
	CommandProcess(L"Tree.Copy.Name"	, m_tree, AVESTA_CopyName);
	CommandProcess(L"Tree.Copy.Path"	, m_tree, AVESTA_CopyPath);
	CommandProcess(L"Tree.Copy.Here"	, m_tree, AVESTA_CopyHere);
	CommandProcess(L"Tree.Delete"		, m_tree, CommandDelete);
	CommandProcess(L"Tree.Bury"			, m_tree, CommandBury);
	CommandProcess(L"Tree.Rename"		, m_tree, CommandRename);
	CommandProcess(L"Tree.Paste"		, m_tree, CommandPaste);
	CommandProcess(L"Tree.Property"		, m_tree, CommandProperty);
	CommandProcess(L"Tree.MoveTo"		, m_tree, AVESTA_MoveTo);
	CommandProcess(L"Tree.CopyTo"		, m_tree, AVESTA_CopyTo);

	// Option
	CommandProcess(L"Option.Reload"			, &Main::OptionReload);
	CommandProcess(L"Option.Font"			, &Main::OptionFont);
	CommandProcess(L"FolderOptions.Show"	, &Main::ProcessFolderOptionsShow);

	CommandHandler(L"Option.Thumbnail.64"	, &Main::ProcessThumbSize, 64 , &Main::ObserveThumbSize, 64 );
	CommandHandler(L"Option.Thumbnail.96"	, &Main::ProcessThumbSize, 96 , &Main::ObserveThumbSize, 96 );
	CommandHandler(L"Option.Thumbnail.128"	, &Main::ProcessThumbSize, 128, &Main::ObserveThumbSize, 128);
	CommandHandler(L"Option.Thumbnail.192"	, &Main::ProcessThumbSize, 192, &Main::ObserveThumbSize, 192);
	CommandHandler(L"Option.Thumbnail.256"	, &Main::ProcessThumbSize, 256, &Main::ObserveThumbSize, 256);
	CommandHandler(L"Arrange.Auto"			, &Main::ProcessArrange, ArrangeAuto, &Main::ObserveArrange, ArrangeAuto);
	CommandHandler(L"Arrange.Horz"			, &Main::ProcessArrange, ArrangeHorz, &Main::ObserveArrange, ArrangeHorz);
	CommandHandler(L"Arrange.Vert"			, &Main::ProcessArrange, ArrangeVert, &Main::ObserveArrange, ArrangeVert);
	CommandHandler(L"Keybind.Normal"		, &Main::ProcessKeybind, afx::KeybindNormal , &Main::ObserveKeybind, afx::KeybindNormal );
	CommandHandler(L"Keybind.Atok"			, &Main::ProcessKeybind, afx::KeybindAtok , &Main::ObserveKeybind, afx::KeybindAtok );
	CommandHandler(L"Keybind.Emacs"			, &Main::ProcessKeybind, afx::KeybindEmacs, &Main::ObserveKeybind, afx::KeybindEmacs);
	CommandHandler(L"MiddleClick.Disable"	, &Main::ProcessMiddleClick, ModifierNone   , &Main::ObserveMiddleClick, ModifierNone   );
	CommandHandler(L"MiddleClick.Control"	, &Main::ProcessMiddleClick, ModifierControl, &Main::ObserveMiddleClick, ModifierControl);
	CommandHandler(L"MiddleClick.Shift"		, &Main::ProcessMiddleClick, ModifierShift  , &Main::ObserveMiddleClick, ModifierShift  );
	CommandHandler(L"AutoCopy.None"			, &Main::ProcessAutoCopy, CopyNone, &Main::ObserveAutoCopy, CopyNone);
	CommandHandler(L"AutoCopy.Base"			, &Main::ProcessAutoCopy, CopyBase, &Main::ObserveAutoCopy, CopyBase);
	CommandHandler(L"AutoCopy.Name"			, &Main::ProcessAutoCopy, CopyName, &Main::ObserveAutoCopy, CopyName);
	CommandHandler(L"AutoCopy.Path"			, &Main::ProcessAutoCopy, CopyPath, &Main::ObserveAutoCopy, CopyPath);

	// ‚ ‚Æ‚Å‚È‚­‚È‚é‚à‚Ì
	CommandProcess(L"Option.WallPaper"		, &Main::OptionWallPaper);
	CommandHandler(L"Option.Insert.Head"	, &Main::OptionInsert, InsertHead, &Main::ObserveInsert, InsertHead);
	CommandHandler(L"Option.Insert.Tail"	, &Main::OptionInsert, InsertTail, &Main::ObserveInsert, InsertTail);
	CommandHandler(L"Option.Insert.Prev"	, &Main::OptionInsert, InsertPrev, &Main::ObserveInsert, InsertPrev);
	CommandHandler(L"Option.Insert.Next"	, &Main::OptionInsert, InsertNext, &Main::ObserveInsert, InsertNext);
	CommandHandler(L"Option.TaskTray"		, &Main::OptionTaskTray		, null, &Main::ObserveTaskTray		, null);
	CommandHandler(L"Option.AlwaysTray"		, &Main::OptionAlwaysTray	, null, &Main::ObserveAlwaysTray	, null);
	CommandHandler(L"Option.CloseToTray"	, &Main::OptionCloseToTray	, null, &Main::ObserveCloseToTray	, null);
	CommandHandler(L"Option.AlwaysTop"		, &Main::OptionAlwaysTop	, null, &Main::ObserveAlwaysTop		, null);
	CommandHandler(L"Option.CheckBox"			, &Main::OptionCheckBox		, BoolCheckBox			, &Main::ObserveBoolean, BoolCheckBox);
	CommandHandler(L"Option.DnDCopyInterDrive"	, &Main::OptionBoolean		, BoolDnDCopyInterDrv	, &Main::ObserveBoolean, BoolDnDCopyInterDrv);
	CommandHandler(L"Option.DistinguishTab"		, &Main::OptionBoolean		, BoolDistinguishTab	, &Main::ObserveBoolean, BoolDistinguishTab);
	CommandHandler(L"Option.FullRowSelect"		, &Main::OptionFullRowSelect, BoolFullRowSelect		, &Main::ObserveBoolean, BoolFullRowSelect);
	CommandHandler(L"Option.GestureOnName"		, &Main::OptionBoolean		, BoolGestureOnName		, &Main::ObserveBoolean, BoolGestureOnName);
	CommandHandler(L"Option.GridLine"			, &Main::OptionGridLine		, BoolGridLine			, &Main::ObserveBoolean, BoolGridLine);
	CommandHandler(L"Option.OpenDups"			, &Main::OptionBoolean		, BoolOpenDups			, &Main::ObserveBoolean, BoolOpenDups);
	CommandHandler(L"Option.OpenNotify"			, &Main::OptionBoolean		, BoolOpenNotify		, &Main::ObserveBoolean, BoolOpenNotify);
	CommandHandler(L"Option.RestoreConditions"	, &Main::OptionBoolean		, BoolRestoreCond		, &Main::ObserveBoolean, BoolRestoreCond);
	CommandHandler(L"Option.LazyExecute"		, &Main::OptionBoolean		, BoolLazyExecute		, &Main::ObserveBoolean, BoolLazyExecute);
	CommandHandler(L"Option.LockClose"			, &Main::OptionBoolean		, BoolLockClose			, &Main::ObserveBoolean, BoolLockClose);
	CommandHandler(L"Option.LoopCursor"			, &Main::OptionBoolean		, BoolLoopCursor		, &Main::ObserveBoolean, BoolLoopCursor);
	CommandHandler(L"MiddleClick.SingleExecute"	, &Main::OptionBoolean		, BoolMiddleSingle		, &Main::ObserveBoolean, BoolMiddleSingle);
	CommandHandler(L"Option.RenameExtension"	, &Main::OptionRenameExtension, null, &Main::ObserveBoolean, BoolRenameExtension);
	CommandHandler(L"Option.PasteInFolder"		, &Main::OptionBoolean		, BoolPasteInFolder		, &Main::ObserveBoolean, BoolPasteInFolder);
	CommandHandler(L"Option.Python"				, &Main::OptionPython		, BoolPython			, &Main::ObserveBoolean, BoolPython);
	CommandHandler(L"Option.QuietProgress"		, &Main::OptionBoolean		, BoolQuietProgress		, &Main::ObserveBoolean, BoolQuietProgress);
}
