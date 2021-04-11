// FolderMenu.cpp

#include "stdafx.h"
#include "private.h"
#include "io.hpp"
#include "shell.hpp"
#include "signal.hpp"

using namespace mew::ui;
using namespace mew::io;

namespace {
static string STRING_EMPTY(_T("ÅiÇ»ÇµÅj"));

class EmptyMenu : public Root<implements<ITreeItem>, mixin<StaticLife> > {
 public:  // ITreeItem
  string get_Name() { return STRING_EMPTY; }
  ref<ICommand> get_Command() { return null; }
  int get_Image() { return -2; }
  bool HasChildren() { return false; }
  UINT32 OnUpdate() { return 1; }
  size_t GetChildCount() { return 0; }
  HRESULT GetChild(REFINTF ppChild, size_t index) { return E_INVALIDARG; }
};

static EmptyMenu theEmptyMenu;

class ShellMenuBase : public Root<implements<IFolder, ITreeItem, ICommand> > {
 private:
  ref<IEntry> m_pEntry;
  ref<IUnknown> m_ArgForEntry;

 protected:
  array<ITreeItem> m_Children;
  string m_Name;
  int m_Level;

 protected:
  ShellMenuBase() : m_Level(0) {}
  void InitWithEntry(IEntry* entry) {
    ASSERT(!m_pEntry);
    ASSERT(!m_ArgForEntry);
    ASSERT(entry);
    m_pEntry = entry;
  }
  void InitWithArg(IUnknown* arg) {
    ASSERT(!m_pEntry);
    ASSERT(!m_ArgForEntry);
    if (ref<IEntry> entry = cast(arg)) {  // arg is IEntry
      InitWithEntry(entry);
    } else if (arg) {  // some arg
      m_ArgForEntry = arg;
    } else {                                 // no arg
      m_pEntry.create(__uuidof(io::Entry));  // desktop;
    }
  }
  virtual FolderMenu* GetRoot() const throw() = 0;
  virtual void InitSubMenu() {
    if (!m_Children.empty()) return;
    AddSubMenu();
  }
  void AddSubMenu();

 public:
  void Dispose() throw() {
    m_pEntry.clear();
    m_ArgForEntry.clear();
    m_Children.clear();
  }

 public:  // IFolder
  IEntry* get_Entry() {
    if (!m_pEntry && m_ArgForEntry) {
      try {
        ref<IUnknown> arg = m_ArgForEntry;
        m_ArgForEntry.clear();
        m_pEntry.create(__uuidof(io::Entry), arg);
      } catch (Error& e) {
        TRACE(e.Message);
      }
    }
    return m_pEntry;
  }

 public:  // ITreeItem
  string get_Name() {
    if (!m_Name) {
      if (IEntry* entry = get_Entry()) {
        string name = entry->Name;
        PCTSTR namestr = name.str();
        if (afx::PathIsRegistory(namestr))
          m_Name = PathFindFileName(namestr);
        else
          m_Name = name;
      }
    }
    return m_Name;
  }
  ref<ICommand> get_Command() { return this; }
  int get_Image() {
    if (IEntry* entry = get_Entry())
      return entry->Image;
    else
      return -2;
  }
  bool HasChildren();
  bool CheckDepth() const;
  size_t GetChildCount() {
    InitSubMenu();
    return m_Children.size();
  }
  HRESULT GetChild(REFINTF ppChild, size_t index) {
    InitSubMenu();
    if (index >= m_Children.size()) return E_INVALIDARG;
    return m_Children[index]->QueryInterface(ppChild);
  }
  UINT32 OnUpdate() {
    InitSubMenu();
    return 1;
  }
  void Invoke();
  void Reset() { m_Children.clear(); }

 public:  // ICommand
  string get_Description() {
    if (IEntry* entry = get_Entry()) {
      return entry->GetName(IEntry::PATH_OR_NAME);
    } else if (m_ArgForEntry) {
      string s;
      ObjectToString(&s, m_ArgForEntry);
      return s;
    } else {
      return string::load(IDS_ERR_FILENOTFOUND);
    }
  }
  UINT32 QueryState(IUnknown* owner) {
    if (IEntry* entry = get_Entry()) {
      if (entry->Exists()) return ENABLED;
    }
    return 0;
  }
};

class SubMenu : public ShellMenuBase {
 private:
  FolderMenu* m_pRoot;

 public:
  SubMenu(IEntry* entry, FolderMenu* root, int depth) : m_pRoot(root) {
    InitWithEntry(entry);
    m_Level = depth;
  }
  void Dispose() throw() {
    m_pRoot = null;
    __super::Dispose();
  }
  bool get_IncludeFiles();
  void set_IncludeFiles(bool value) { TRESPASS(); }
  int get_Depth();
  void set_Depth(int depth) { TRESPASS(); }
  virtual FolderMenu* GetRoot() const throw() { return m_pRoot; }
};
}  // namespace

//==============================================================================

namespace mew {
namespace io {

class FolderMenu : public Object<ShellMenuBase, implements<ISignal>, mixin<SignalImpl> > {
 private:
  UINT32 m_LastModify;
  bool m_IncludeFiles;
  int m_Depth;

 public:  // Object
  void __init__(IUnknown* arg) {
    m_LastModify = 1;
    m_Depth = -1;
    m_IncludeFiles = false;
    InitWithArg(arg);
    m_msgr.create(__uuidof(Messenger));
  }
  void Dispose() throw() {
    m_msgr.clear();
    ShellMenuBase::Dispose();
  }
  bool get_IncludeFiles() { return m_IncludeFiles; }
  void set_IncludeFiles(bool value) {
    if (m_IncludeFiles == value) return;
    m_IncludeFiles = value;
    m_Children.clear();
    ++m_LastModify;
  }
  int get_Depth() { return m_Depth; }
  void set_Depth(int depth) { m_Depth = depth; }
  UINT32 OnUpdate() {
    __super::OnUpdate();
    return m_LastModify;
  }
  ref<ICommand> get_Command() { return this; }
  void InitSubMenu() {
    if (!m_Children.empty()) return;
    __super::AddSubMenu();
  }

  HRESULT Connect(EventCode code, function fn, message msg = null) throw() {
    switch (code) {
      case EventInvoke:
        return __super::Connect(code, fn, msg);
      default:
        TRACE(_T("error: invalid event code ($1)"), code);
        return E_INVALIDARG;
    }
  }
  IMessenger* GetMessenger() const throw() { return m_msgr; }
  virtual FolderMenu* GetRoot() const throw() { return const_cast<FolderMenu*>(this); }
};

AVESTA_EXPORT(FolderMenu)

}  // namespace io
}  // namespace mew

//==============================================================================

namespace {
void ShellMenuBase::AddSubMenu() {
  if (!CheckDepth()) return;
  if (IEntry* e = get_Entry()) {
    CWaitCursor wait;
    ref<IEntry> entry;
    e->GetLinked(&entry);
    for (each<IEntry> i = entry->EnumChildren(GetRoot()->IncludeFiles); i.next();) {
      SubMenu* sub = new SubMenu(i, GetRoot(), m_Level + 1);
      m_Children.push_back(sub);
      sub->Release();
    }
    if (m_Children.empty()) m_Children.push_back(&theEmptyMenu);
  }
}
void ShellMenuBase::Invoke() {
  if (IEntry* entry = get_Entry()) {
    if (function fn = GetRoot()->GetMessenger()->Invoke(EventInvoke)) {
      message args;
      args["from"] = (ICommand*)this;
      args["what"] = entry;
      fn(args);
    }
  }
}
bool ShellMenuBase::CheckDepth() const {
  int depth = GetRoot()->get_Depth();
  return depth < 0 || (depth - m_Level > 0);
}
bool ShellMenuBase::HasChildren() {
  if (CheckDepth()) {
    if (IEntry* e = get_Entry()) {
      ref<IEntry> entry;
      e->GetLinked(&entry);
      if (entry->IsFolder()) {  // is folder
        return m_Children.size() != 1 || !objcmp(m_Children[0], &theEmptyMenu);
      }
    }
  }
  // not a folder
  return false;
}
bool SubMenu::get_IncludeFiles() { return GetRoot()->get_IncludeFiles(); }
int SubMenu::get_Depth() {
  int depth = GetRoot()->get_Depth();
  return depth < 0 ? -1 : depth - m_Level;
}
}  // namespace
