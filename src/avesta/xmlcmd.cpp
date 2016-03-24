// xmlcmd.cpp

#include "stdafx.h"
#include "../server/main.hpp"

extern ref<ICommand> CreateExecuteCommand(string execute, string args);
extern ref<ICommand> CreateNavigateCommand(string navigate, string args);

namespace
{
	const PCWSTR ATTR_COMMAND	= L"command";
	const PCWSTR ATTR_PATH		= L"path";
	const PCWSTR ATTR_ARGS		= L"args";
	const PCWSTR ATTR_EXECUTE	= L"execute";
	const PCWSTR ATTR_NAVIGATE	= L"navigate";

	ref<IEditableTreeItem> CreateSpecialMenu(const string& cmd)
	{
		ASSERT(cmd);
		if(cmd.equals_nocase(L"File.MRU"))
			return theAvesta->CreateMRUTreeItem();
		else if(cmd.equals_nocase(L"Current.Go.Up"))
			return theAvesta->CreateFolderTreeItem(DirNorth);
		else if(cmd.equals_nocase(L"Current.Go.Back"))
			return theAvesta->CreateFolderTreeItem(DirWest);
		else if(cmd.equals_nocase(L"Current.Go.Forward"))
			return theAvesta->CreateFolderTreeItem(DirEast);
		else
			return null;
	}
}

//==============================================================================

namespace
{
	ref<ICommand> CreateBuiltInCommand(ICommands* commands, const string& command, const string& args)
	{
		ref<ICommand> value;
		commands->Find(&value, command);
		return value;
	}
}

ref<ICommand> avesta::XmlAttrCommand(XMLAttributes& attr, ICommands* commands)
{
	string args = attr[ATTR_ARGS];

	// @command => Built-in command
	if(string command = attr[ATTR_COMMAND])
		return CreateBuiltInCommand(commands, command, args);

	// @execute => Execute command
	if(string execute = attr[ATTR_EXECUTE])
		return CreateExecuteCommand(execute, args);

	// @navigate => Navigate command
	if(string navigate = attr[ATTR_NAVIGATE])
		return CreateNavigateCommand(navigate, args);

//	// @path => Path command
//	if(string path = attr[ATTR_PATH])
//		return app->CreatePathCommand(path, args);

	// not a command, regards as a separator
	return null;
}

ref<IEditableTreeItem> avesta::XmlAttrTreeItem(XMLAttributes& attr, ICommands* commands)
{
	ref<IEditableTreeItem> item;
	if(string command = attr[ATTR_COMMAND])
		item = CreateSpecialMenu(command); // 特殊メニューの作成を試みる
	if(!item)
	{	// 一般メニュー
		item.create(__uuidof(DefaultTreeItem));
		if(ref<ICommand> command = XmlAttrCommand(attr, commands))
			item->Command = command;
	}
	//
	item->Image	= XmlAttrImage(attr);
	item->Name	= XmlAttrText(attr);
	return item;
}
