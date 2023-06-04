// Command.h
#pragma once

namespace {
inline mew::message FocusHeaderMessage() {
  mew::message m(mew::ui::CommandFocusHeader);
  m["time"] = theAvesta->GetExposeTime();
  return m;
}

inline mew::message StyleMsg(INT32 style) {
  mew::message msg(mew::ui::CommandSetStyle);
  msg["style"] = style;
  return msg;
}
}  // namespace

void Main::InitCommands() {
  // ƒRƒ}ƒ“ƒh
  m_commands = CreateCommands(COMMANDS, sizeof(COMMANDS) / sizeof(COMMANDS[0]));
  mew::function current(this, &Main::ForwardToCurrent);
  mew::function all(this, &Main::ForwardToAll);
  mew::function shown(this, &Main::ForwardToShown);
  mew::function hidden(this, &Main::ForwardToHidden);
  mew::function left(this, &Main::ForwardToLeft);
  mew::function right(this, &Main::ForwardToRight);
  mew::function others(this, &Main::ForwardToOthers);
  mew::function dups(this, &Main::ForwardToDuplicate);
  mew::function anyFolder(this, &Main::EnableIfAnyFolder);
  mew::function anySelect(this, &Main::EnableIfAnySelection);

  // System
  CommandProcess(L"System.Exit", m_form, mew::ui::CommandClose);
  CommandProcess(L"System.Restart", &Main::SystemRestart);
  CommandProcess(L"System.About", &Main::SystemAbout);

  // File
  CommandProcess(L"Explorer.Clone", &Main::ExplorerImport, 0);
  CommandProcess(L"Explorer.Import", &Main::ExplorerImport, 1);
  CommandProcess(L"File.Open", &Main::FileOpen, 0);
  CommandProcess(L"File.Load", &Main::FileLoad);
  CommandHandler(L"File.Save", &Main::FileSave, mew::null, &Main::EnableIfAnyFolder, mew::null);
  CommandHandler(L"File.MRU", &Main::FileMRU, mew::null, &Main::ObserveMRU, mew::null);
  CommandHandler(L"RecycleBin.Empty", &Main::ProcessRecycleBin, mew::null, &Main::ObserveRecycleBin, mew::null);

  // Current
  CommandHandler(L"Current.Lock(true)", &Main::CurrentLock, BoolTrue, &Main::ObserveLock, BoolTrue);
  CommandHandler(L"Current.Lock(false)", &Main::CurrentLock, BoolFalse, &Main::ObserveLock, BoolFalse);
  CommandHandler(L"Current.Lock(toggle)", &Main::CurrentLock, BoolUnknown, &Main::ObserveLock, BoolUnknown);
  CommandToFocus(L"Current.Menu", mew::ui::CommandMenu);

  CommandToFocus(L"Current.New", AVESTA_New);
  CommandToFocus(L"Current.NewFolder", AVESTA_NewFolder);
  CommandToFocus(L"Current.SelectPattern", AVESTA_SelectPattern);
  CommandToFocus(L"Current.Close", mew::ui::CommandClose);
  CommandToFocus(L"Current.Show(false)", AVESTA_Hide);
  CommandToFocus(L"Current.Export", AVESTA_Export);
  CommandToFocus(L"Current.Refresh", mew::ui::CommandUpdate);
  CommandToFocus(L"Current.Find", AVESTA_Find);
  CommandToFocus(L"Current.PatternMask", AVESTA_PatternMask);

  CommandToSelect(L"Current.Cut", mew::ui::CommandCut);
  CommandToSelect(L"Current.Copy", mew::ui::CommandCopy);
  CommandToSelect(L"Current.Copy.Base", AVESTA_CopyBase);
  CommandToSelect(L"Current.Copy.Name", AVESTA_CopyName);
  CommandToSelect(L"Current.Copy.Path", AVESTA_CopyPath);
  CommandToSelect(L"Current.Copy.Here", AVESTA_CopyHere);
  CommandToSelect(L"Current.Delete", mew::ui::CommandDelete);
  CommandToSelect(L"Current.Bury", mew::ui::CommandBury);
  CommandToSelect(L"Current.Rename", mew::ui::CommandRename);
  CommandToSelect(L"Current.Rename.Dialog", AVESTA_RenameDialog);
  CommandToSelect(L"Current.Property", mew::ui::CommandProperty);
  CommandToSelect(L"Current.CopyTo", AVESTA_CopyTo);
  CommandToSelect(L"Current.MoveTo", AVESTA_MoveTo);
  CommandToSelect(L"Current.CopyToOther", AVESTA_CopyToOther);
  CommandToSelect(L"Current.MoveToOther", AVESTA_MoveToOther);
  CommandToSelect(L"Current.CopyCheckedTo", AVESTA_CopyCheckedTo);
  CommandToSelect(L"Current.MoveCheckedTo", AVESTA_MoveCheckedTo);
  CommandToFocus(L"Current.SyncDescendants", AVESTA_SyncDesc);

  CommandToFocus(L"Current.Key.Up", mew::ui::CommandKeyUp);
  CommandToFocus(L"Current.Key.Down", mew::ui::CommandKeyDown);
  CommandToFocus(L"Current.Key.Left", mew::ui::CommandKeyLeft);
  CommandToFocus(L"Current.Key.Right", mew::ui::CommandKeyRight);
  CommandToFocus(L"Current.Key.Home", mew::ui::CommandKeyHome);
  CommandToFocus(L"Current.Key.End", mew::ui::CommandKeyEnd);
  CommandToFocus(L"Current.Key.PageUp", mew::ui::CommandKeyPageUp);
  CommandToFocus(L"Current.Key.PageDown", mew::ui::CommandKeyPageDown);
  CommandToFocus(L"Current.Key.Space", mew::ui::CommandKeySpace);
  CommandToFocus(L"Current.Key.Enter", mew::ui::CommandKeyEnter);
  m_commands->Alias(L"Current.Cursor.Up", L"Current.Key.Up");
  m_commands->Alias(L"Current.Cursor.Down", L"Current.Key.Down");
  m_commands->Alias(L"Current.Cursor.Left", L"Current.Key.Left");
  m_commands->Alias(L"Current.Cursor.Right", L"Current.Key.Right");
  m_commands->Alias(L"Current.Cursor.Home", L"Current.Key.Home");
  m_commands->Alias(L"Current.Cursor.End", L"Current.Key.End");
  m_commands->Alias(L"Current.Cursor.PageUp", L"Current.Key.PageUp");
  m_commands->Alias(L"Current.Cursor.PageDown", L"Current.Key.PageDown");

  CommandToSelect(L"Current.SelectNone", mew::ui::CommandSelectNone);
  CommandToCheck(L"Current.CheckAll", mew::ui::CommandCheckAll);
  CommandToCheck(L"Current.CheckNone", mew::ui::CommandCheckNone);
  CommandToFocus(L"Current.SelectAll", mew::ui::CommandSelectAll);
  CommandToFocus(L"Current.SelectChecked", mew::ui::CommandSelectChecked);
  CommandToFocus(L"Current.SelectReverse", mew::ui::CommandSelectReverse);
  CommandToFocus(L"Current.SelectToFirst", mew::ui::CommandSelectToFirst);
  CommandToFocus(L"Current.SelectToLast", mew::ui::CommandSelectToLast);

  CommandToFocus(L"Current.Undo", mew::ui::CommandUndo);

  CommandProcess(L"Current.SyncFileDialog", &Main::ProcessSyncFileDialog);

  CommandHandler(L"Current.Paste", current, mew::ui::CommandPaste, &Main::ObserveClipboard, CF_SHELLIDLIST);
  CommandHandler(L"Current.PasteTo", current, AVESTA_PasteTo, &Main::ObserveClipToSelect, CF_HDROP);
  CommandHandler(L"Current.Rename.Paste", current, AVESTA_RenamePaste, &Main::ObserveClipToSelect, CF_UNICODETEXT);
  CommandHandler(L"Current.Go.Up", current, mew::ui::CommandGoUp, &Main::ObserveGo, mew::ui::DirNorth);
  CommandHandler(L"Current.Go.Back", current, mew::ui::CommandGoBack, &Main::ObserveGo, mew::ui::DirWest);
  CommandHandler(L"Current.Go.Forward", current, mew::ui::CommandGoForward, &Main::ObserveGo, mew::ui::DirEast);
  CommandHandler(L"Current.Go.Location", &Main::FileOpen, 1, anyFolder, mew::null);
  CommandHandler(L"Current.AutoArrange", current, AVESTA_AutoArrange, current, AVEOBS_AutoArrange);
  CommandHandler(L"Current.Grouping", current, AVESTA_Grouping, current, AVEOBS_Grouping);
  CommandHandler(L"Current.ShowAllFiles", current, AVESTA_ShowAllFiles, current, AVEOBS_ShowAllFiles);

  CommandHandler(L"Current.Mode.Icon", current, StyleMsg(mew::ui::ListStyleIcon), &Main::ObserveMode, mew::ui::ListStyleIcon);
  CommandHandler(L"Current.Mode.List", current, StyleMsg(mew::ui::ListStyleList), &Main::ObserveMode, mew::ui::ListStyleList);
  CommandHandler(L"Current.Mode.Details", current, StyleMsg(mew::ui::ListStyleDetails), &Main::ObserveMode,
                 mew::ui::ListStyleDetails);
  CommandHandler(L"Current.Mode.Thumbnail", current, StyleMsg(mew::ui::ListStyleThumnail), &Main::ObserveMode,
                 mew::ui::ListStyleThumnail);
  CommandHandler(L"Current.Mode.Tile", current, StyleMsg(mew::ui::ListStyleTile), &Main::ObserveMode, mew::ui::ListStyleTile);

  CommandToFocus(L"Current.AdjustToItem", mew::ui::CommandAdjustToItem);
  CommandToFocus(L"Current.AdjustToWindow", mew::ui::CommandAdjustToWindow);

  // All
  CommandHandler(L"All.Close", all, mew::ui::CommandClose, anyFolder, mew::null);
  CommandHandler(L"All.Refresh", all, mew::ui::CommandUpdate, anyFolder, mew::null);
  CommandHandler(L"All.Export", all, AVESTA_Export, anyFolder, mew::null);
  CommandHandler(L"All.Show(true)", all, AVESTA_Show, anyFolder, mew::null);

  // Shown
  CommandHandler(L"Shown.Close", shown, mew::ui::CommandClose, anyFolder, mew::null);
  CommandHandler(L"Shown.Refresh", shown, mew::ui::CommandUpdate, anyFolder, mew::null);
  CommandHandler(L"Shown.Export", shown, AVESTA_Export, anyFolder, mew::null);
  CommandHandler(L"Shown.AdjustToItem", shown, mew::ui::CommandAdjustToItem, anyFolder, mew::null);
  CommandHandler(L"Shown.AdjustToWindow", shown, mew::ui::CommandAdjustToWindow, anyFolder, mew::null);
  CommandHandler(L"Shown.ToLeft", m_tab, mew::ui::CommandShownToLeft, anyFolder, mew::null);
  CommandHandler(L"Shown.ToRight", m_tab, mew::ui::CommandShownToRight, anyFolder, mew::null);

  // Locked
  CommandHandler(L"Locked.ToLeft", m_tab, mew::ui::CommandLockedToLeft, anyFolder, mew::null);
  CommandHandler(L"Locked.ToRight", m_tab, mew::ui::CommandLockedToRight, anyFolder, mew::null);

  // Hidden
  CommandHandler(L"Hidden.Close", hidden, mew::ui::CommandClose, anyFolder, mew::null);
  CommandHandler(L"Hidden.Refresh", hidden, mew::ui::CommandUpdate, anyFolder, mew::null);
  CommandHandler(L"Hidden.Export", hidden, AVESTA_Export, anyFolder, mew::null);

  // Duplicate
  CommandHandler(L"Duplicate.Close", dups, mew::ui::CommandClose, anyFolder, mew::null);

  // Left
  CommandHandler(L"Left.Close", left, mew::ui::CommandClose, anyFolder, mew::null);
  CommandHandler(L"Left.Show(false)", left, AVESTA_Hide, anyFolder, mew::null);
  CommandHandler(L"Left.Show(true)", left, AVESTA_Show, anyFolder, mew::null);

  // Right
  CommandHandler(L"Right.Close", right, mew::ui::CommandClose, anyFolder, mew::null);
  CommandHandler(L"Right.Show(false)", right, AVESTA_Hide, anyFolder, mew::null);
  CommandHandler(L"Right.Show(true)", right, AVESTA_Show, anyFolder, mew::null);

  // Others
  CommandHandler(L"Others.Close", others, mew::ui::CommandClose, anyFolder, mew::null);
  CommandHandler(L"Others.Show(false)", others, AVESTA_Hide, anyFolder, mew::null);

  // Tab
  CommandProcess(L"Tab.Focus", m_tab, &mew::ui::IWindow::Focus);
  CommandHandler(L"Tab.Sort", &Main::ProcessTabSort, mew::null, anyFolder, mew::null);
  CommandHandler(L"Tab.Next", m_tab, mew::ui::CommandGoForward, anyFolder, mew::null);
  CommandHandler(L"Tab.Next2", m_tab, mew::ui::CommandGoDown, anyFolder, mew::null);
  CommandHandler(L"Tab.Prev", m_tab, mew::ui::CommandGoBack, anyFolder, mew::null);
  CommandHandler(L"Tab.Prev2", m_tab, mew::ui::CommandGoUp, anyFolder, mew::null);
  CommandHandler(L"Current.ToLeft", &Main::ProcessTabMove, -1, anyFolder, mew::null);
  CommandHandler(L"Current.ToRight", &Main::ProcessTabMove, +1, anyFolder, mew::null);

  // Single Tab
  for (int i = 0; i < 9; ++i) {
    TCHAR name[MAX_PATH];
    wsprintf(name, L"Tab[%d].Show(toggle)", (i + 1));
    CommandHandler(name, &Main::ProcessTabShow, i, &Main::ObserveTabShow, i);
    wsprintf(name, L"Tab[%d].Focus", (i + 1));
    CommandHandler(name, &Main::ProcessTabFocus, i, &Main::ObserveTabFocus, i);
  }

  // Form
  CommandProcess(L"Form.Show(true)", &Main::WindowVisibleTrue);
  CommandProcess(L"Form.Show(false)", m_form, mew::ui::CommandMinimize);
  CommandProcess(L"Form.Show(toggle)", &Main::WindowVisibleToggle);
  CommandProcess(L"Form.Maximize", m_form, mew::ui::CommandMaximize);
  CommandProcess(L"Form.Restore", m_form, mew::ui::CommandRestore);
  CommandProcess(L"Form.Resize", m_form, mew::ui::CommandResize);
  CommandProcess(L"Form.Move", m_form, mew::ui::CommandMove);
  CommandProcess(L"Form.Menu", m_form, mew::ui::CommandMenu);
  CommandHandler(L"Form.DropMode", &Main::ProcessDropMode, mew::null, &Main::ObserveDropMode, mew::null);

  m_commands->Alias(L"Form.Zoom", L"Form.Maximize");
  m_commands->Alias(L"Form.Minimize", L"Form.Show(false)");
  m_commands->Alias(L"Form.Close", L"System.Exit");

  m_commands->Alias(L"Window.Show(true)", L"Form.Show(true)");
  m_commands->Alias(L"Window.Show(false)", L"Form.Show(false)");
  m_commands->Alias(L"Window.Show(toggle)", L"Form.Show(toggle)");
  m_commands->Alias(L"Window.Maximize", L"Form.Maximize");
  m_commands->Alias(L"Window.Restore", L"Form.Restore");
  m_commands->Alias(L"Window.Resize", L"Form.Resize");
  m_commands->Alias(L"Window.Move", L"Form.Move");
  m_commands->Alias(L"Window.Menu", L"Form.Menu");
  m_commands->Alias(L"Window.Zoom", L"Form.Maximize");
  m_commands->Alias(L"Window.Minimize", L"Form.Show(false)");
  m_commands->Alias(L"Window.Close", L"System.Exit");

  // Address
  CommandToFocus(L"Address.Focus", mew::ui::CommandFocusAddress);
  // Header
  CommandToFocus(L"Header.Focus", FocusHeaderMessage());
  // Tree
  CommandProcess(L"Tree.Refresh", &Main::ProcessTreeRefresh);
  CommandHandler(L"Tree.Sync", &Main::ProcessTreeSync, mew::null, &Main::ObserveTreeSync, mew::null);
  CommandHandler(L"Tree.Reflect", &Main::ProcessTreeReflect, mew::null, &Main::ObserveTreeReflect, mew::null);
  CommandHandler(L"Tree.AutoSync", &Main::OptionBoolean, avesta::BoolTreeAutoSync, &Main::ObserveBoolean,
                 avesta::BoolTreeAutoSync);
  CommandHandler(L"Tree.AutoReflect", &Main::OptionBoolean, avesta::BoolTreeAutoReflect, &Main::ObserveBoolean,
                 avesta::BoolTreeAutoReflect);
  CommandProcess(L"Tree.Cut", m_tree, mew::ui::CommandCut);
  CommandProcess(L"Tree.Copy", m_tree, mew::ui::CommandCopy);
  CommandProcess(L"Tree.Copy.Base", m_tree, AVESTA_CopyBase);
  CommandProcess(L"Tree.Copy.Name", m_tree, AVESTA_CopyName);
  CommandProcess(L"Tree.Copy.Path", m_tree, AVESTA_CopyPath);
  CommandProcess(L"Tree.Copy.Here", m_tree, AVESTA_CopyHere);
  CommandProcess(L"Tree.Delete", m_tree, mew::ui::CommandDelete);
  CommandProcess(L"Tree.Bury", m_tree, mew::ui::CommandBury);
  CommandProcess(L"Tree.Rename", m_tree, mew::ui::CommandRename);
  CommandProcess(L"Tree.Paste", m_tree, mew::ui::CommandPaste);
  CommandProcess(L"Tree.Property", m_tree, mew::ui::CommandProperty);
  CommandProcess(L"Tree.MoveTo", m_tree, AVESTA_MoveTo);
  CommandProcess(L"Tree.CopyTo", m_tree, AVESTA_CopyTo);

  // Option
  CommandProcess(L"Option.Reload", &Main::OptionReload);
  CommandProcess(L"Option.Font", &Main::OptionFont);
  CommandProcess(L"FolderOptions.Show", &Main::ProcessFolderOptionsShow);

  CommandHandler(L"Option.Thumbnail.64", &Main::ProcessThumbSize, 64, &Main::ObserveThumbSize, 64);
  CommandHandler(L"Option.Thumbnail.96", &Main::ProcessThumbSize, 96, &Main::ObserveThumbSize, 96);
  CommandHandler(L"Option.Thumbnail.128", &Main::ProcessThumbSize, 128, &Main::ObserveThumbSize, 128);
  CommandHandler(L"Option.Thumbnail.192", &Main::ProcessThumbSize, 192, &Main::ObserveThumbSize, 192);
  CommandHandler(L"Option.Thumbnail.256", &Main::ProcessThumbSize, 256, &Main::ObserveThumbSize, 256);
  CommandHandler(L"Arrange.Auto", &Main::ProcessArrange, mew::ui::ArrangeAuto, &Main::ObserveArrange, mew::ui::ArrangeAuto);
  CommandHandler(L"Arrange.Horz", &Main::ProcessArrange, mew::ui::ArrangeHorz, &Main::ObserveArrange, mew::ui::ArrangeHorz);
  CommandHandler(L"Arrange.Vert", &Main::ProcessArrange, mew::ui::ArrangeVert, &Main::ObserveArrange, mew::ui::ArrangeVert);
  CommandHandler(L"Keybind.Normal", &Main::ProcessKeybind, afx::KeybindNormal, &Main::ObserveKeybind, afx::KeybindNormal);
  CommandHandler(L"Keybind.Atok", &Main::ProcessKeybind, afx::KeybindAtok, &Main::ObserveKeybind, afx::KeybindAtok);
  CommandHandler(L"Keybind.Emacs", &Main::ProcessKeybind, afx::KeybindEmacs, &Main::ObserveKeybind, afx::KeybindEmacs);
  CommandHandler(L"MiddleClick.Disable", &Main::ProcessMiddleClick, mew::ui::ModifierNone, &Main::ObserveMiddleClick,
                 mew::ui::ModifierNone);
  CommandHandler(L"MiddleClick.Control", &Main::ProcessMiddleClick, mew::ui::ModifierControl, &Main::ObserveMiddleClick,
                 mew::ui::ModifierControl);
  CommandHandler(L"MiddleClick.Shift", &Main::ProcessMiddleClick, mew::ui::ModifierShift, &Main::ObserveMiddleClick,
                 mew::ui::ModifierShift);
  CommandHandler(L"AutoCopy.None", &Main::ProcessAutoCopy, mew::ui::CopyNone, &Main::ObserveAutoCopy, mew::ui::CopyNone);
  CommandHandler(L"AutoCopy.Base", &Main::ProcessAutoCopy, mew::ui::CopyBase, &Main::ObserveAutoCopy, mew::ui::CopyBase);
  CommandHandler(L"AutoCopy.Name", &Main::ProcessAutoCopy, mew::ui::CopyName, &Main::ObserveAutoCopy, mew::ui::CopyName);
  CommandHandler(L"AutoCopy.Path", &Main::ProcessAutoCopy, mew::ui::CopyPath, &Main::ObserveAutoCopy, mew::ui::CopyPath);

  // ‚ ‚Æ‚Å‚È‚­‚È‚é‚à‚Ì
  CommandProcess(L"Option.WallPaper", &Main::OptionWallPaper);
  CommandHandler(L"Option.Insert.Head", &Main::OptionInsert, mew::ui::InsertHead, &Main::ObserveInsert, mew::ui::InsertHead);
  CommandHandler(L"Option.Insert.Tail", &Main::OptionInsert, mew::ui::InsertTail, &Main::ObserveInsert, mew::ui::InsertTail);
  CommandHandler(L"Option.Insert.Prev", &Main::OptionInsert, mew::ui::InsertPrev, &Main::ObserveInsert, mew::ui::InsertPrev);
  CommandHandler(L"Option.Insert.Next", &Main::OptionInsert, mew::ui::InsertNext, &Main::ObserveInsert, mew::ui::InsertNext);
  CommandHandler(L"Option.TaskTray", &Main::OptionTaskTray, mew::null, &Main::ObserveTaskTray, mew::null);
  CommandHandler(L"Option.AlwaysTray", &Main::OptionAlwaysTray, mew::null, &Main::ObserveAlwaysTray, mew::null);
  CommandHandler(L"Option.CloseToTray", &Main::OptionCloseToTray, mew::null, &Main::ObserveCloseToTray, mew::null);
  CommandHandler(L"Option.AlwaysTop", &Main::OptionAlwaysTop, mew::null, &Main::ObserveAlwaysTop, mew::null);
  CommandHandler(L"Option.CheckBox", &Main::OptionCheckBox, avesta::BoolCheckBox, &Main::ObserveBoolean, avesta::BoolCheckBox);
  CommandHandler(L"Option.DnDCopyInterDrive", &Main::OptionBoolean, avesta::BoolDnDCopyInterDrv, &Main::ObserveBoolean,
                 avesta::BoolDnDCopyInterDrv);
  CommandHandler(L"Option.DistinguishTab", &Main::OptionBoolean, avesta::BoolDistinguishTab, &Main::ObserveBoolean,
                 avesta::BoolDistinguishTab);
  CommandHandler(L"Option.FullRowSelect", &Main::OptionFullRowSelect, avesta::BoolFullRowSelect, &Main::ObserveBoolean,
                 avesta::BoolFullRowSelect);
  CommandHandler(L"Option.GestureOnName", &Main::OptionBoolean, avesta::BoolGestureOnName, &Main::ObserveBoolean,
                 avesta::BoolGestureOnName);
  CommandHandler(L"Option.GridLine", &Main::OptionGridLine, avesta::BoolGridLine, &Main::ObserveBoolean, avesta::BoolGridLine);
  CommandHandler(L"Option.OpenDups", &Main::OptionBoolean, avesta::BoolOpenDups, &Main::ObserveBoolean, avesta::BoolOpenDups);
  CommandHandler(L"Option.OpenNotify", &Main::OptionBoolean, avesta::BoolOpenNotify, &Main::ObserveBoolean,
                 avesta::BoolOpenNotify);
  CommandHandler(L"Option.RestoreConditions", &Main::OptionBoolean, avesta::BoolRestoreCond, &Main::ObserveBoolean,
                 avesta::BoolRestoreCond);
  CommandHandler(L"Option.LazyExecute", &Main::OptionBoolean, avesta::BoolLazyExecute, &Main::ObserveBoolean,
                 avesta::BoolLazyExecute);
  CommandHandler(L"Option.LockClose", &Main::OptionBoolean, avesta::BoolLockClose, &Main::ObserveBoolean,
                 avesta::BoolLockClose);
  CommandHandler(L"Option.LoopCursor", &Main::OptionBoolean, avesta::BoolLoopCursor, &Main::ObserveBoolean,
                 avesta::BoolLoopCursor);
  CommandHandler(L"MiddleClick.SingleExecute", &Main::OptionBoolean, avesta::BoolMiddleSingle, &Main::ObserveBoolean,
                 avesta::BoolMiddleSingle);
  CommandHandler(L"Option.RenameExtension", &Main::OptionRenameExtension, mew::null, &Main::ObserveBoolean,
                 avesta::BoolRenameExtension);
  CommandHandler(L"Option.PasteInFolder", &Main::OptionBoolean, avesta::BoolPasteInFolder, &Main::ObserveBoolean,
                 avesta::BoolPasteInFolder);
  CommandHandler(L"Option.Python", &Main::OptionPython, avesta::BoolPython, &Main::ObserveBoolean, avesta::BoolPython);
  CommandHandler(L"Option.QuietProgress", &Main::OptionBoolean, avesta::BoolQuietProgress, &Main::ObserveBoolean,
                 avesta::BoolQuietProgress);
}
