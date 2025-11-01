// XmlResource.cpp

#include "stdafx.h"
#include "main.hpp"
#include "object.hpp"

//==============================================================================

#include "../mew/widgets/MenuProvider.hpp"

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
class PopupCommand : public mew::Root<mew::implements<mew::ICommand> > {
 private:
  mew::ref<mew::ui::ITreeItem> m_menu;

 public:
  PopupCommand(mew::ui::ITreeItem* menu) : m_menu(menu) {}
  mew::string get_Description() { return mew::null; }
  UINT32 QueryState(IUnknown* owner) { return mew::ENABLED; }
  void Invoke() {
    mew::ref<mew::ui::IWindow> owner;
    if FAILED (theAvesta->GetComponent(&owner, avesta::AvestaFolder)) {
      if FAILED (theAvesta->GetComponent(&owner, avesta::AvestaForm)) {
        return;
      }
    }
    HWND hwnd = owner->Handle;
    mew::Rect rc;
    ::GetWindowRect(hwnd, &rc);
    mew::Point pt = rc.center;
    // FIXME: ugly...
    if (mew::ref<ICommand> cmd = mew::ui::MenuProvider::PopupMenu(
            m_menu, ::GetAncestor(hwnd, GA_ROOT), TPM_RIGHTBUTTON | TPM_CENTERALIGN | TPM_VCENTERALIGN, pt.x, pt.y)) {
      cmd->Invoke();
    }
  }
};

mew::ref<mew::ICommand> CommandFromMenu(mew::ui::ITreeItem* menu) {
  return mew::ref<mew::ICommand>::from(new PopupCommand(menu));
}
}  // namespace

//==============================================================================

namespace {
class TreeItemReader : public mew::xml::XMLHandlerImpl {
 private:
  mew::ref<mew::ui::IEditableTreeItem> m_root;
  mew::array<mew::ui::IEditableTreeItem> m_stack;  // ITreeItem は親へのリンクを持たないため、外部的に管理する

 protected:
  TreeItemReader() { m_stack.reserve(8); }

  void PushTreeItem(mew::ui::IEditableTreeItem* item) {
    if (!m_root) {
      ASSERT(m_stack.empty());
      m_root = item;
      m_stack.push_back(item);
    } else if (m_stack.empty()) {  // ルートレベルに複数のメニューが並んでいる。ダミールートを挿入する。
      // ref<ITreeItem> root = CreateStandardTreeItem();
      mew::ref<mew::ui::IEditableTreeItem> root(__uuidof(mew::ui::DefaultTreeItem));
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
  mew::ref<mew::ui::IEditableTreeItem> PopTreeItem() {
    mew::ref<mew::ui::IEditableTreeItem> item = m_stack.back();
    m_stack.pop_back();
    return item;
  }

 public:
  mew::ref<mew::ui::ITreeItem> DetachTreeItem() {
    mew::ref<mew::ui::ITreeItem> tmp = m_root;
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
  mew::ref<mew::ICommands> m_commands;

 public:
  MenuReader(mew::ICommands* commands) : m_commands(commands) {}
  ~MenuReader() {}
  HRESULT StartElement(PCWSTR name, size_t cch, mew::xml::XMLAttributes& attr) {
    if (equals(name, ITEM_NODE, cch)) {
      mew::ref<mew::ui::IEditableTreeItem> item = avesta::XmlAttrTreeItem(attr, m_commands);
      ASSERT(item);
      PushTreeItem(item);
    } else {
      TRACE(L"INVALID_ELEMENT");
    }
    return S_OK;
  }
  HRESULT EndElement(PCWSTR name, size_t cch) {
    if (equals(name, ITEM_NODE, cch)) {
      mew::ref<mew::ui::IEditableTreeItem> back = PopTreeItem();
      if (back->Name && !back->Command && !back->HasChildren()) {
        back->Name = mew::string::format(_T("コマンドが見つかりません：$1"), back->Name);
      }
    }
    return S_OK;
  }
};
}  // namespace

mew::ref<mew::ui::ITreeItem> XmlLoadTreeItem(mew::string xmlfile, mew::ICommands* commands, mew::xml::IXMLReader* sax) {
  MenuReader reader(commands);
  sax->Parse(&reader, xmlfile);
  return reader.DetachTreeItem();
}

//==============================================================================

namespace {
class KeymapReader : public MenuReader {
 private:
  mew::ref<mew::ui::IKeymapTable> m_keymap;
  UINT16 m_mods;
  UINT8 m_vkey;

 public:
  KeymapReader(mew::ICommands* commands) : MenuReader(commands) {
    m_mods = 0;
    m_vkey = 0;
  }
  ~KeymapReader() {}
  mew::ref<mew::ui::IKeymap> GetProduct() { return m_keymap; }
  HRESULT StartElement(PCWSTR name, size_t cch, mew::xml::XMLAttributes& attr) {
    if (equals(name, KEYMAP_ROOT, cch)) {
      m_keymap.create(__uuidof(mew::ui::KeymapTable));
      return S_OK;
    } else if (equals(name, KEYMAP_BIND, cch)) {
      if (!m_keymap) {
        return E_UNEXPECTED;
      }
      // 仮想キーコード
      m_vkey = avesta::XmlAttrKey(attr);
      if (m_vkey == 0) {
        TRACE(L"INVALID_KEYNAME");
        return S_OK;
      }
      // 修飾子の処理
      m_mods = avesta::XmlAttrModifiers(attr);
      // コマンドの処理
      if (mew::ref<mew::ICommand> command = avesta::XmlAttrCommand(attr, m_commands)) {
        m_keymap->SetBind(m_mods, m_vkey, command);
      } else {  // XXX: このブロックは不要かもしれない
        mew::ref<mew::ui::IEditableTreeItem> menu;
        menu.create(__uuidof(mew::ui::DefaultTreeItem));
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
        if (mew::ref<mew::ui::ITreeItem> menu = DetachTreeItem()) {
          m_keymap->SetBind(m_mods, m_vkey, CommandFromMenu(menu));
        }
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

mew::ref<mew::ui::IKeymap> XmlLoadKeymap(mew::string xmlfile, mew::ICommands* commands, mew::xml::IXMLReader* sax) {
  KeymapReader reader(commands);
  sax->Parse(&reader, xmlfile);
  return reader.GetProduct();
}

//==============================================================================

namespace {
class GestureReader : public mew::xml::XMLHandlerImpl {
 private:
  mew::ref<mew::ui::IGestureTable> m_gesture;
  mew::ref<mew::ICommands> m_commands;

 public:
  GestureReader(mew::ICommands* commands) : m_commands(commands) {}
  ~GestureReader() {}
  mew::ref<mew::ui::IGestureTable> GetProduct() { return m_gesture; }
  HRESULT StartElement(PCWSTR name, size_t cch, mew::xml::XMLAttributes& attr) {
    if (equals(name, GESTURE_ROOT, cch)) {
      m_gesture.create(__uuidof(mew::ui::GestureTable));
      return S_OK;
    } else if (equals(name, GESTURE_BIND, cch)) {
      if (!m_gesture) {
        return E_UNEXPECTED;
      }
      // 処理
      mew::string input = attr[GESTURE_INPUT];
      if (!input) {
        ASSERT(!"gesture/@input が見つかりません");
        return S_OK;
      }
      std::vector<mew::ui::Gesture> gesture;
      if (!StringToGesture(input, gesture)) {
        TRACE(L"gesture/@input が不正です : $1", input);
        ASSERT(!"gesture/@input が不正です");
        return S_OK;
      }
      // コマンド
      if (mew::ref<mew::ICommand> command = avesta::XmlAttrCommand(attr, m_commands)) {
        m_gesture->SetGesture(avesta::XmlAttrModifiers(attr), gesture.size(), &gesture[0], command);
      }
      return S_OK;
    } else {
      ASSERT(!"INVALID_ELEMENT");
    }
    return S_OK;
  }

 private:
  static bool StringToGesture(const mew::string& str, std::vector<mew::ui::Gesture>& gesture) {
    gesture.clear();
    PCWSTR s = str.str();
    size_t length = str.length();
    for (size_t i = 0; i < length; ++i) {
      WCHAR c = s[i];
      if (mew::str::find(L"L1Ｌ１①左㊧", c)) {
        gesture.push_back(mew::ui::GestureButtonLeft);
      } else if (mew::str::find(L"R2Ｒ２②右㊨", c)) {
        gesture.push_back(mew::ui::GestureButtonRight);
      } else if (mew::str::find(L"M3Ｍ③中㊥", c)) {
        gesture.push_back(mew::ui::GestureButtonMiddle);
      } else if (mew::str::find(L"4４④", c)) {
        gesture.push_back(mew::ui::GestureButtonX1);
      } else if (mew::str::find(L"5５⑤", c)) {
        gesture.push_back(mew::ui::GestureButtonX2);
      } else if (mew::str::find(L"UＵ上㊤∧△▲", c)) {
        gesture.push_back(mew::ui::GestureWheelUp);
      } else if (mew::str::find(L"DＤ下㊦∨▽▼", c)) {
        gesture.push_back(mew::ui::GestureWheelDown);
      } else if (mew::str::find(L"WＷ←", c)) {
        gesture.push_back(mew::ui::GestureWest);
      } else if (mew::str::find(L"EＥ→", c)) {
        gesture.push_back(mew::ui::GestureEast);
      } else if (mew::str::find(L"NＮ↑", c)) {
        gesture.push_back(mew::ui::GestureNorth);
      } else if (mew::str::find(L"SＳ↓", c)) {
        gesture.push_back(mew::ui::GestureSouth);
      } else {
        return false;
      }
    }
    return !gesture.empty();
  }
};
}  // namespace

mew::ref<mew::ui::IGesture> XmlLoadGesture(mew::string xmlfile, mew::ICommands* commands, mew::xml::IXMLReader* sax) {
  GestureReader reader(commands);
  sax->Parse(&reader, xmlfile);
  return reader.GetProduct();
}

//==============================================================================

namespace {
class LinkMenuItem : public mew::Root<mew::implements<mew::ui::ITreeItem, mew::ui::IEditableTreeItem, mew::io::IFolder> > {
 private:  // variables
  mew::string m_Text;
  mew::ref<mew::io::IFolder> m_pFolder;
  UINT32 m_TimeStamp;
  mew::array<mew::ui::ITreeItem> m_Children;

 public:  // Object
  LinkMenuItem(mew::io::IFolder* folder) : m_pFolder(folder) {
    ASSERT(folder);
    m_TimeStamp = 0;
  }
  void Dispose() noexcept {
    m_Text.clear();
    m_pFolder.clear();
    m_Children.clear();
    ++m_TimeStamp;
  }

 public:  // IFolder
  mew::io::IEntry* get_Entry() { return m_pFolder->get_Entry(); }
  bool get_IncludeFiles() { return m_pFolder->get_IncludeFiles(); }
  void set_IncludeFiles(bool value) { m_pFolder->set_IncludeFiles(value); }
  int get_Depth() { return m_pFolder->get_Depth(); }
  void set_Depth(int depth) { m_pFolder->set_Depth(depth); }
  void Reset() { m_pFolder->Reset(); }

 public:  // ITreeItem
  mew::string get_Name() { return m_Text; }
  mew::ref<mew::ICommand> get_Command() { return cast(m_pFolder); }
  int get_Image() { return m_pFolder->Image; }
  void set_Name(mew::string value) {
    m_Text = value;
    ++m_TimeStamp;
  }
  void set_Command(mew::ICommand* value) { TRESPASS(); }
  void set_Image(int value) { TRESPASS(); }

  bool HasChildren() { return !m_Children.empty() || m_pFolder->HasChildren(); }
  size_t GetChildCount() { return m_Children.size() + m_pFolder->GetChildCount(); }
  HRESULT GetChild(mew::REFINTF ppChild, size_t index) {
    const size_t count = GetChildCount();
    if (index >= count) {
      return E_INVALIDARG;
    }
    if (index < m_Children.size()) {
      return m_Children[index]->QueryInterface(ppChild);
    } else {
      return m_pFolder->GetChild(ppChild, index - m_Children.size());
    }
  }
  UINT32 OnUpdate() { return m_TimeStamp = mew::math::max(m_TimeStamp, m_pFolder->OnUpdate()); }
  void AddChild(mew::ui::ITreeItem* child) {
    m_Children.push_back(child);
    ++m_TimeStamp;
  }
  bool RemoveChild(mew::ui::ITreeItem* child) {
    if (!m_Children.erase(child)) {
      return false;
    }
    ++m_TimeStamp;
    return true;
  }
};

//==============================================================================

class LinkReader : public TreeItemReader {
 private:
  mew::function m_invokeHandler;
  int m_FolderImageIndex;

 public:
  LinkReader(mew::function fn) : m_invokeHandler(fn) { m_FolderImageIndex = afx::ExpGetImageIndexForFolder(); }
  ~LinkReader() {}
  HRESULT StartElement(PCWSTR name, size_t cch, mew::xml::XMLAttributes& attr) {
    if (equals(name, ITEM_NODE, cch)) {
      int depth = -1;
      bool includeFiles = false;
      mew::string text = attr[ITEM_TEXT];
      mew::string path = attr[LINK_ATTR_PATH];
      if (mew::string depth_s = attr[LINK_ATTR_DEPTH]) {
        depth = mew::str::atoi(depth_s.str());
      }
      if (mew::string includeFiles_s = attr[LINK_ATTR_FILES]) {
        includeFiles = (includeFiles_s.equals_nocase(_T("true")) || includeFiles_s.equals_nocase(_T("yes")) ||
                        mew::str::atoi(includeFiles_s.str()) != 0);
      }
      mew::ref<mew::io::IFolder> folder;
      if (path) {
        try {
          folder.create(__uuidof(mew::io::FolderMenu), path);
          folder->IncludeFiles = includeFiles;
          folder->Depth = depth;
          mew::cast<mew::ISignal>(folder)->Connect(mew::EventInvoke, m_invokeHandler);
        } catch (mew::exceptions::Error& e) {
          TRACE(e.Message);
        }
      }
      mew::ref<mew::ui::IEditableTreeItem> menu;
      if (folder) {
        menu.attach(new LinkMenuItem(folder));
      } else {
        menu.create(__uuidof(mew::ui::DefaultTreeItem));
        menu->Image = m_FolderImageIndex;
      }
      menu->Name = avesta::XmlAttrText(attr);
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

mew::ref<mew::ui::ITreeItem> XmlLoadLinks(mew::string xmlfile, mew::function onOpen, mew::xml::IXMLReader* sax) {
  LinkReader reader(onOpen);
  sax->Parse(&reader, xmlfile);
  return reader.DetachTreeItem();
}
