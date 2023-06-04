// xmlcmd.cpp

#include "stdafx.h"
#include "../server/main.hpp"

extern mew::ref<mew::ICommand> CreateExecuteCommand(mew::string execute, mew::string args);
extern mew::ref<mew::ICommand> CreateNavigateCommand(mew::string navigate, mew::string args);

namespace {
const PCWSTR ATTR_COMMAND = L"command";
const PCWSTR ATTR_PATH = L"path";
const PCWSTR ATTR_ARGS = L"args";
const PCWSTR ATTR_EXECUTE = L"execute";
const PCWSTR ATTR_NAVIGATE = L"navigate";

mew::ref<mew::ui::IEditableTreeItem> CreateSpecialMenu(const mew::string& cmd) {
  ASSERT(cmd);
  if (cmd.equals_nocase(L"File.MRU")) {
    return theAvesta->CreateMRUTreeItem();
  } else if (cmd.equals_nocase(L"Current.Go.Up")) {
    return theAvesta->CreateFolderTreeItem(mew::ui::DirNorth);
  } else if (cmd.equals_nocase(L"Current.Go.Back")) {
    return theAvesta->CreateFolderTreeItem(mew::ui::DirWest);
  } else if (cmd.equals_nocase(L"Current.Go.Forward")) {
    return theAvesta->CreateFolderTreeItem(mew::ui::DirEast);
  } else {
    return mew::null;
  }
}
}  // namespace

//==============================================================================

namespace {
mew::ref<mew::ICommand> CreateBuiltInCommand(mew::ICommands* commands, const mew::string& command, const mew::string& args) {
  mew::ref<mew::ICommand> value;
  commands->Find(&value, command);
  return value;
}
}  // namespace

mew::ref<mew::ICommand> avesta::XmlAttrCommand(mew::xml::XMLAttributes& attr, mew::ICommands* commands) {
  mew::string args = attr[ATTR_ARGS];

  // @command => Built-in command
  if (mew::string command = attr[ATTR_COMMAND]) {
    return CreateBuiltInCommand(commands, command, args);
  }

  // @execute => Execute command
  if (mew::string execute = attr[ATTR_EXECUTE]) {
    return CreateExecuteCommand(execute, args);
  }

  // @navigate => Navigate command
  if (mew::string navigate = attr[ATTR_NAVIGATE]) {
    return CreateNavigateCommand(navigate, args);
  }

  // // @path => Path command
  // if(string path = attr[ATTR_PATH])
  //   return app->CreatePathCommand(path, args);

  // not a command, regards as a separator
  return mew::null;
}

mew::ref<mew::ui::IEditableTreeItem> avesta::XmlAttrTreeItem(mew::xml::XMLAttributes& attr, mew::ICommands* commands) {
  mew::ref<mew::ui::IEditableTreeItem> item;
  if (mew::string command = attr[ATTR_COMMAND]) {
    item = CreateSpecialMenu(command);  // 特殊メニューの作成を試みる
  }
  if (!item) {  // 一般メニュー
    item.create(__uuidof(mew::ui::DefaultTreeItem));
    if (mew::ref<mew::ICommand> command = XmlAttrCommand(attr, commands)) {
      item->Command = command;
    }
  }
  //
  item->Image = XmlAttrImage(attr);
  item->Name = XmlAttrText(attr);
  return item;
}
