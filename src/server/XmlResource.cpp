// XmlResource.cpp

#include "stdafx.h"
#include "main.hpp"
#include "object.hpp"

//==============================================================================

#include "../mew/widgets/MenuProvider.hpp"

using namespace mew::algorithm;

namespace {
// item
const PCWSTR ITEM_NODE = L"menu";
const PCWSTR ITEM_TEXT = L"text";
// link
const PCWSTR LINK_ATTR_PATH = L"path";
const PCWSTR LINK_ATTR_DEPTH = L"depth";
const PCWSTR LINK_ATTR_FILES = L"include-files";
// keyboard
const PCWSTR KEYMAP_ROOT = L"keyboard";
const PCWSTR KEYMAP_BIND = L"bind";
// mouse
const PCWSTR GESTURE_ROOT = L"mouse";
const PCWSTR GESTURE_BIND = L"gesture";
const PCWSTR GESTURE_INPUT = L"input";

//==============================================================================

inline static bool equals(PCWSTR lhs, PCWSTR rhs, size_t cch) { return wcsncmp(lhs, rhs, cch) == 0 && wcslen(rhs) == cch; }
}  // namespace

//==============================================================================

namespace {
class PopupCommand : public Root<implements<ICommand> > {
 private:
  ref<ITreeItem> m_menu;

 public:
  PopupCommand(ITreeItem* menu) : m_menu(menu) {}
  string get_Description() { return null; }
  UINT32 QueryState(IUnknown* owner) { return ENABLED; }
  void Invoke() {
    ref<IWindow> owner;
    if FAILED (theAvesta->GetComponent(&owner, AvestaFolder))
      if FAILED (theAvesta->GetComponent(&owner, AvestaForm)) return;
    HWND hwnd = owner->Handle;
    Rect rc;
    ::GetWindowRect(hwnd, &rc);
    Point pt = rc.center;
    // FIXME: ugly...
    if (ref<ICommand> cmd = mew::ui::MenuProvider::PopupMenu(m_menu, ::GetAncestor(hwnd, GA_ROOT),
                                                             TPM_RIGHTBUTTON | TPM_CENTERALIGN | TPM_VCENTERALIGN, pt.x, pt.y))
      cmd->Invoke();
  }
};

ref<ICommand> CommandFromMenu(ITreeItem* menu) { return ref<ICommand>::from(new PopupCommand(menu)); }
}  // namespace

//==============================================================================

namespace {
class TreeItemReader : public XMLHandlerImpl {
 private:
  ref<IEditableTreeItem> m_root;
  array<IEditableTreeItem> m_stack;  // ITreeItem は親へのリンクを持たないため、外部的に管理する

 protected:
  TreeItemReader() { m_stack.reserve(8); }

  void PushTreeItem(IEditableTreeItem* item) {
    if (!m_root) {
      ASSERT(m_stack.empty());
      m_root = item;
      m_stack.push_back(item);
    } else if (m_stack.empty()) {  // ルートレベルに複数のメニューが並んでいる。ダミールートを挿入する。
      // ref<ITreeItem> root = CreateStandardTreeItem();
      ref<IEditableTreeItem> root(__uuidof(DefaultTreeItem));
      m_stack.push_back(root);
      m_stack.push_back(item);
      root->AddChild(m_root);
      root->AddChild(item);
      m_root = root;
    } else {
      m_stack.back()->AddChild(item);
      m_stack.push_back(item);
    }
  }
  ref<IEditableTreeItem> PopTreeItem() {
    ref<IEditableTreeItem> item = m_stack.back();
    m_stack.pop_back();
    return item;
  }

 public:
  ref<ITreeItem> DetachTreeItem() {
    ref<ITreeItem> tmp = m_root;
    m_stack.clear();
    m_root.clear();
    return tmp;
  }
};
}  // namespace

//==============================================================================

namespace {
class MenuReader : public TreeItemReader {
 protected:
  ref<ICommands> m_commands;

 public:
  MenuReader(ICommands* commands) : m_commands(commands) {}
  ~MenuReader() {}
  HRESULT StartElement(PCWSTR name, size_t cch, XMLAttributes& attr) {
    if (equals(name, ITEM_NODE, cch)) {
      ref<IEditableTreeItem> item = XmlAttrTreeItem(attr, m_commands);
      ASSERT(item);
      PushTreeItem(item);
    } else {
      TRACE(L"INVALID_ELEMENT");
    }
    return S_OK;
  }
  HRESULT EndElement(PCWSTR name, size_t cch) {
    if (equals(name, ITEM_NODE, cch)) {
      ref<IEditableTreeItem> back = PopTreeItem();
      if (back->Name && !back->Command && !back->HasChildren()) {
        back->Name = string::format(_T("コマンドが見つかりません：$1"), back->Name);
      }
    }
    return S_OK;
  }
};
}  // namespace

ref<ITreeItem> XmlLoadTreeItem(string xmlfile, ICommands* commands, IXMLReader* sax) {
  MenuReader reader(commands);
  sax->Parse(&reader, xmlfile);
  return reader.DetachTreeItem();
}

//==============================================================================

namespace {
class KeymapReader : public MenuReader {
 private:
  ref<IKeymapTable> m_keymap;
  UINT16 m_mods;
  UINT8 m_vkey;

 public:
  KeymapReader(ICommands* commands) : MenuReader(commands) {
    m_mods = 0;
    m_vkey = 0;
  }
  ~KeymapReader() {}
  ref<IKeymap> GetProduct() { return m_keymap; }
  HRESULT StartElement(PCWSTR name, size_t cch, XMLAttributes& attr) {
    if (equals(name, KEYMAP_ROOT, cch)) {
      m_keymap.create(__uuidof(KeymapTable));
      return S_OK;
    } else if (equals(name, KEYMAP_BIND, cch)) {
      if (!m_keymap) return E_UNEXPECTED;
      // 仮想キーコード
      m_vkey = XmlAttrKey(attr);
      if (m_vkey == 0) {
        TRACE(L"INVALID_KEYNAME");
        return S_OK;
      }
      // 修飾子の処理
      m_mods = XmlAttrModifiers(attr);
      // コマンドの処理
      if (ref<ICommand> command = XmlAttrCommand(attr, m_commands)) {
        m_keymap->SetBind(m_mods, m_vkey, command);
      } else {  // XXX: このブロックは不要かもしれない
        ref<IEditableTreeItem> menu;
        menu.create(__uuidof(DefaultTreeItem));
        PushTreeItem(menu);
      }
    } else {
      return __super::StartElement(name, cch, attr);
    }
    return S_OK;
  }
  HRESULT EndElement(PCWSTR name, size_t cch) {
    if (equals(name, KEYMAP_BIND, cch)) {
      if (m_vkey != 0) {
        if (ref<ITreeItem> menu = DetachTreeItem()) m_keymap->SetBind(m_mods, m_vkey, CommandFromMenu(menu));
        m_mods = 0;
        m_vkey = 0;
      }
    } else {
      return __super::EndElement(name, cch);
    }
    return S_OK;
  }
};
}  // namespace

ref<IKeymap> XmlLoadKeymap(string xmlfile, ICommands* commands, IXMLReader* sax) {
  KeymapReader reader(commands);
  sax->Parse(&reader, xmlfile);
  return reader.GetProduct();
}

//==============================================================================

namespace {
class GestureReader : public XMLHandlerImpl {
 private:
  ref<IGestureTable> m_gesture;
  ref<ICommands> m_commands;

 public:
  GestureReader(ICommands* commands) : m_commands(commands) {}
  ~GestureReader() {}
  ref<IGestureTable> GetProduct() { return m_gesture; }
  HRESULT StartElement(PCWSTR name, size_t cch, XMLAttributes& attr) {
    if (equals(name, GESTURE_ROOT, cch)) {
      m_gesture.create(__uuidof(GestureTable));
      return S_OK;
    } else if (equals(name, GESTURE_BIND, cch)) {
      if (!m_gesture) return E_UNEXPECTED;
      // 処理
      string input = attr[GESTURE_INPUT];
      if (!input) {
        ASSERT(!"gesture/@input が見つかりません");
        return S_OK;
      }
      std::vector<Gesture> gesture;
      if (!StringToGesture(input, gesture)) {
        TRACE(L"gesture/@input が不正です : $1", input);
        ASSERT(!"gesture/@input が不正です");
        return S_OK;
      }
      // コマンド
      if (ref<ICommand> command = XmlAttrCommand(attr, m_commands)) {
        m_gesture->SetGesture(XmlAttrModifiers(attr), gesture.size(), &gesture[0], command);
      }
      return S_OK;
    } else {
      ASSERT(!"INVALID_ELEMENT");
    }
    return S_OK;
  }

 private:
  static bool StringToGesture(const string& str, std::vector<Gesture>& gesture) {
    gesture.clear();
    PCWSTR s = str.str();
    size_t length = str.length();
    for (size_t i = 0; i < length; ++i) {
      WCHAR c = s[i];
      if (str::find(L"L1Ｌ１①左㊧", c))
        gesture.push_back(GestureButtonLeft);
      else if (str::find(L"R2Ｒ２②右㊨", c))
        gesture.push_back(GestureButtonRight);
      else if (str::find(L"M3Ｍ③中㊥", c))
        gesture.push_back(GestureButtonMiddle);
      else if (str::find(L"4４④", c))
        gesture.push_back(GestureButtonX1);
      else if (str::find(L"5５⑤", c))
        gesture.push_back(GestureButtonX2);
      else if (str::find(L"UＵ上㊤∧△▲", c))
        gesture.push_back(GestureWheelUp);
      else if (str::find(L"DＤ下㊦∨▽▼", c))
        gesture.push_back(GestureWheelDown);
      else if (str::find(L"WＷ←", c))
        gesture.push_back(GestureWest);
      else if (str::find(L"EＥ→", c))
        gesture.push_back(GestureEast);
      else if (str::find(L"NＮ↑", c))
        gesture.push_back(GestureNorth);
      else if (str::find(L"SＳ↓", c))
        gesture.push_back(GestureSouth);
      else
        return false;
    }
    return !gesture.empty();
  }
};
}  // namespace

ref<IGesture> XmlLoadGesture(string xmlfile, ICommands* commands, IXMLReader* sax) {
  GestureReader reader(commands);
  sax->Parse(&reader, xmlfile);
  return reader.GetProduct();
}

//==============================================================================

namespace {
class LinkMenuItem : public Root<implements<ITreeItem, IEditableTreeItem, IFolder> > {
 private:  // variables
  string m_Text;
  ref<IFolder> m_pFolder;
  UINT32 m_TimeStamp;
  array<ITreeItem> m_Children;

 public:  // Object
  LinkMenuItem(IFolder* folder) : m_pFolder(folder) {
    ASSERT(folder);
    m_TimeStamp = 0;
  }
  void Dispose() throw() {
    m_Text.clear();
    m_pFolder.clear();
    m_Children.clear();
    ++m_TimeStamp;
  }

 public:  // IFolder
  IEntry* get_Entry() { return m_pFolder->get_Entry(); }
  bool get_IncludeFiles() { return m_pFolder->get_IncludeFiles(); }
  void set_IncludeFiles(bool value) { m_pFolder->set_IncludeFiles(value); }
  int get_Depth() { return m_pFolder->get_Depth(); }
  void set_Depth(int depth) { m_pFolder->set_Depth(depth); }
  void Reset() { m_pFolder->Reset(); }

 public:  // ITreeItem
  string get_Name() { return m_Text; }
  ref<ICommand> get_Command() { return cast(m_pFolder); }
  int get_Image() { return m_pFolder->Image; }
  void set_Name(string value) {
    m_Text = value;
    ++m_TimeStamp;
  }
  void set_Command(ICommand* value) { TRESPASS(); }
  void set_Image(int value) { TRESPASS(); }

  bool HasChildren() { return !m_Children.empty() || m_pFolder->HasChildren(); }
  size_t GetChildCount() { return m_Children.size() + m_pFolder->GetChildCount(); }
  HRESULT GetChild(REFINTF ppChild, size_t index) {
    const size_t count = GetChildCount();
    if (index >= count) return E_INVALIDARG;
    if (index < m_Children.size())
      return m_Children[index]->QueryInterface(ppChild);
    else
      return m_pFolder->GetChild(ppChild, index - m_Children.size());
  }
  UINT32 OnUpdate() { return m_TimeStamp = math::max(m_TimeStamp, m_pFolder->OnUpdate()); }
  void AddChild(ITreeItem* child) {
    m_Children.push_back(child);
    ++m_TimeStamp;
  }
  bool RemoveChild(ITreeItem* child) {
    if (!m_Children.erase(child)) return false;
    ++m_TimeStamp;
    return true;
  }
};

//==============================================================================

class LinkReader : public TreeItemReader {
 private:
  function m_invokeHandler;
  int m_FolderImageIndex;

 public:
  LinkReader(function fn) : m_invokeHandler(fn) { m_FolderImageIndex = afx::ExpGetImageIndexForFolder(); }
  ~LinkReader() {}
  HRESULT StartElement(PCWSTR name, size_t cch, XMLAttributes& attr) {
    if (equals(name, ITEM_NODE, cch)) {
      int depth = -1;
      bool includeFiles = false;
      string text = attr[ITEM_TEXT];
      string path = attr[LINK_ATTR_PATH];
      if (string depth_s = attr[LINK_ATTR_DEPTH]) {
        depth = str::atoi(depth_s.str());
      }
      if (string includeFiles_s = attr[LINK_ATTR_FILES]) {
        includeFiles = (includeFiles_s.equals_nocase(_T("true")) || includeFiles_s.equals_nocase(_T("yes")) ||
                        str::atoi(includeFiles_s.str()) != 0);
      }
      ref<IFolder> folder;
      if (path) {
        try {
          folder.create(__uuidof(FolderMenu), path);
          folder->IncludeFiles = includeFiles;
          folder->Depth = depth;
          cast<ISignal>(folder)->Connect(EventInvoke, m_invokeHandler);
        } catch (mew::exceptions::Error& e) {
          TRACE(e.Message);
        }
      }
      ref<IEditableTreeItem> menu;
      if (folder) {
        menu.attach(new LinkMenuItem(folder));
      } else {
        menu.create(__uuidof(DefaultTreeItem));
        menu->Image = m_FolderImageIndex;
      }
      menu->Name = XmlAttrText(attr);
      PushTreeItem(menu);
    } else {
      TRACE(L"INVALID_ELEMENT");
    }
    return S_OK;
  }
  HRESULT EndElement(PCWSTR name, size_t cch) {
    if (equals(name, ITEM_NODE, cch)) {
      PopTreeItem();
    }
    return S_OK;
  }
};
}  // namespace

ref<ITreeItem> XmlLoadLinks(string xmlfile, function onOpen, IXMLReader* sax) {
  LinkReader reader(onOpen);
  sax->Parse(&reader, xmlfile);
  return reader.DetachTreeItem();
}
