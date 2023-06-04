// XmlForm.cpp

#include "stdafx.h"
#include "main.hpp"
#include "object.hpp"

//==============================================================================

namespace {
class FormReader : public mew::xml::XMLHandlerImpl {
 private:
  Templates m_forms;
  int m_current;

 public:
  FormReader() {}

  HRESULT StartDocument() {
    m_current = -1;
    return S_OK;
  }

  HRESULT StartElement(PCWSTR name, size_t cch, mew::xml::XMLAttributes& attr) {
    Template t;
    t.parent = m_current;
    t.type = mew::string(name, cch);
    t.clsid = avesta::ToCLSID(mew::string(name, cch));
    t.id = attr[L"id"];
    // t.name = attr[L"name"];
    t.keyboard = attr[L"keyboard"];
    t.mouse = attr[L"mouse"];
    t.item = attr[L"item"];
    t.icon = attr[L"icon"];
    m_current = m_forms.size();
    m_forms.push_back(t);
    return S_OK;
  }

  HRESULT EndElement(PCWSTR name, size_t cch) {
    int current = m_current;
    m_current = m_forms[m_current].parent;
    if (m_forms[current].clsid == GUID_NULL) {  // unknown class name
      m_forms.resize(current);
    }
    return S_OK;
  }

  HRESULT CopyTo(Templates& forms) {
    forms = m_forms;
    return S_OK;
  }
};
}  // namespace

HRESULT FormTemplate(mew::string xmlfile, mew::xml::IXMLReader* sax, Templates& forms) {
  HRESULT hr;
  FormReader reader;
  if FAILED (hr = sax->Parse(&reader, xmlfile)) {
    return hr;
  }
  return reader.CopyTo(forms);
}

namespace {
const WCHAR FORMTYPE_FORM[] = L"Form";
const WCHAR FORMTYPE_PANEL[] = L"Main";
const WCHAR FORMTYPE_MAIN[] = L"FolderList";
}  // namespace

HRESULT FormGenerate(Templates& forms) {
  int indexForm = INT_MAX;
  int indexPanel = INT_MAX;
  int indexMain = INT_MAX;
  FormDispose(forms);
  for (int i = 0; i < (int)forms.size(); ++i) {
    Template& t = forms[i];
    //
    if (t.type == FORMTYPE_FORM) {
      indexForm = i;
    } else if (t.type == FORMTYPE_PANEL) {
      indexPanel = i;
    } else if (t.type == FORMTYPE_MAIN) {
      indexMain = i;
    }
    //
    mew::ref<mew::ui::IWindow> parent;
    if (t.parent >= 0) {
      parent = forms[t.parent].window;
    }
    t.window.create(t.clsid, parent);
    t.window->Name = t.id;
    // dock
    if (i == indexPanel || i == indexMain) {
      t.window->Dock = mew::ui::DirCenter;
    } else if (t.parent == indexForm) {
      if (i < indexPanel) {
        t.window->Dock = mew::ui::DirNorth;
      } else {
        t.window->Dock = mew::ui::DirSouth;
      }
    } else if (t.parent == indexPanel) {
      if (i < indexMain) {
        t.window->Dock = mew::ui::DirWest;
      } else {
        t.window->Dock = mew::ui::DirEast;
      }
      t.window->Bounds = mew::Rect(0, 0, 160, 160);
    }
  }
  return S_OK;
}

HRESULT FormDispose(Templates& forms) {
  for (Templates::iterator i = forms.begin(); i != forms.end(); ++i) {
    i->window.dispose();
  }
  return S_OK;
}

namespace {
template <typename T>
mew::ref<T> LoadResource(mew::string relname, mew::ICommands* commands, mew::xml::IXMLReader* sax,
                         mew::ref<T> (*fn)(mew::string, mew::ICommands*, mew::xml::IXMLReader*), UINT errmsg) {
  if (!relname) {
    return mew::null;
  }
  TRACE(L"LoadResource($1)", relname);
  RelativePath path(relname, L"..\\..\\usr");
  if (!PathFileExists(path)) {
    // file not found
    ave::WarningBox(/*m_forms*/ mew::null, mew::string::format(L"$1 が見つかりません", relname));
    return mew::null;
  }
  if (mew::ref<T> res = fn(mew::string(path), commands, sax)) {
    return res;
  }
  // error
  ave::WarningBox(/*m_forms*/ mew::null, mew::string::load(IDS_ERR_LOAD_GESTURE, relname));
  return mew::null;
}

mew::ref<IImageList> GetShellImageList(int size) {
  mew::ref<IImageList> imagelist;
  afx::ExpGetImageList(size, &imagelist);
  return imagelist;
}

mew::ref<IImageList> LoadImageList(mew::string relname) {
  static const WCHAR SHELLICON_PREFIX[] = L"shell:";
  static const size_t SHELLICON_PREFIX_LEN = 6;
  LPCWSTR buffer = relname.str();
  ASSERT(buffer);
  if (mew::str::compare_nocase(buffer, SHELLICON_PREFIX, SHELLICON_PREFIX_LEN) == 0) {
    return GetShellImageList(mew::str::atoi(buffer + SHELLICON_PREFIX_LEN));
  }
  RelativePath path(relname, L"..\\..\\usr");
  if (!PathFileExists(path)) {
    // file not found
    ave::WarningBox(/*m_forms*/ mew::null, mew::string::format(L"$1 が見つかりません", relname));
    return mew::null;
  }
  try {
    return mew::ref<IImageList>(__uuidof(ImageList), mew::string(path));
  } catch (mew::exceptions::Error&) {
    ave::WarningBox(/*m_forms*/ mew::null, mew::string::load(IDS_ERR_LOAD_IMAGE, relname));
    return mew::null;
  }
}

template <typename T>
void SetWindowExtension(mew::ui::IWindow* window, mew::string relname, mew::ICommands* commands, mew::xml::IXMLReader* sax,
                        mew::ref<T> (*fn)(mew::string, mew::ICommands*, mew::xml::IXMLReader*), UINT errmsg) {
  if (!window) {
    return;
  }
  if (mew::ref<T> extension = LoadResource(relname, commands, sax, fn, errmsg)) {
    VERIFY_HRESULT(window->SetExtension(__uuidof(T), extension));
  }
}

mew::ref<mew::ui::IEditableTreeItem> LoadLinkTreeFromDirectory(PCTSTR dir, mew::function fn) {
  WIN32_FIND_DATA find;
  mew::io::Path basedir(dir);
  basedir.Append(_T("*.*"));
  HANDLE hFind = ::FindFirstFile(basedir, &find);
  if (hFind == INVALID_HANDLE_VALUE) {
    return mew::null;
  }
  mew::ref<mew::ui::IEditableTreeItem> root(__uuidof(mew::ui::DefaultTreeItem));
  do {
    if (lstrcmp(find.cFileName, _T(".")) == 0) {
      continue;
    }
    if (lstrcmp(find.cFileName, _T("..")) == 0) {
      continue;
    }
    mew::io::Path fullpath(dir);
    fullpath.Append(find.cFileName);
    try {
      mew::ref<mew::io::IFolder> pSubMenu(__uuidof(mew::io::FolderMenu), mew::string(fullpath));
      mew::cast<mew::ISignal>(pSubMenu)->Connect(mew::EventInvoke, fn);
      root->AddChild(pSubMenu);
    } catch (mew::exceptions::Error& e) {
      TRACE(e.Message);
    }
  } while (::FindNextFile(hFind, &find));
  ::FindClose(hFind);
  return root;
}
mew::ref<mew::ui::ITreeItem> LoadLinkTree(mew::string relname, mew::function onOpen, mew::xml::IXMLReader* sax) {
  RelativePath xml(relname, L"..\\..\\usr");
  mew::ref<mew::ui::ITreeItem> root;
  if (PathIsDirectory(xml)) {  // link from directory
    return LoadLinkTreeFromDirectory(xml, onOpen);
  } else if (PathFileExists(xml)) {  // link from xml
    return XmlLoadLinks(mew::string(xml), onOpen, sax);
  }
  return mew::null;
}

HRESULT OnOpenEntry(mew::message msg) {
  theAvesta->OpenOrExecute(msg["what"]);
  return S_OK;
}

// 読み込み失敗用ダミーツリー
mew::ref<mew::ui::ITreeItem> CreateDummyTree(mew::string relname) {
  mew::ref<mew::ui::IEditableTreeItem> root(__uuidof(mew::ui::DefaultTreeItem));
  mew::ref<mew::ui::IEditableTreeItem> disable(__uuidof(mew::ui::DefaultTreeItem));
  disable->Name = mew::string::load(IDS_ERR_OPENFILE, relname);
  root->AddChild(disable);
  return root;
}

void ReloadForm(Template& t, mew::ICommands* commands, mew::xml::IXMLReader* sax) {
  if (!t.window) {
    return;
  }
  SetWindowExtension(t.window, t.keyboard, commands, sax, XmlLoadKeymap, IDS_ERR_LOAD_KEYMAP);
  SetWindowExtension(t.window, t.mouse, commands, sax, XmlLoadGesture, IDS_ERR_LOAD_GESTURE);
  if (mew::ref<mew::ui::ITree> tree = cast(t.window)) {
    if (t.icon) {
      tree->ImageList = LoadImageList(t.icon);
    }
    if (t.item) {
      tree->Root = (t.type == L"LinkBar") ? LoadLinkTree(t.item, OnOpenEntry, sax)
                                          : LoadResource(t.item, commands, sax, XmlLoadTreeItem, IDS_ERR_LOAD_COMMAND);
      if (!tree->Root) {
        tree->Root = CreateDummyTree(t.item);
      }
    }
  }
}
}  // namespace

HRESULT FormReload(Templates& forms, mew::ICommands* commands, mew::xml::IXMLReader* sax) {
  for (Templates::iterator i = forms.begin(); i != forms.end(); ++i) {
    ReloadForm(*i, commands, sax);
  }
  return S_OK;
}

HRESULT FormComponentsHide(Templates& forms) {
  for (Templates::iterator i = forms.begin(); i != forms.end(); ++i) {
    if (!i->window) continue;
    if (i->type == FORMTYPE_FORM || i->type == FORMTYPE_PANEL || i->type == FORMTYPE_MAIN) {
    } else {
      i->visible = i->window->Visible;
      i->window->Visible = false;
    }
  }
  return S_OK;
}

HRESULT FormComponentsRestore(Templates& forms) {
  for (Templates::iterator i = forms.begin(); i != forms.end(); ++i) {
    if (!i->window) continue;
    if (i->type == FORMTYPE_FORM || i->type == FORMTYPE_PANEL || i->type == FORMTYPE_MAIN) {
    } else {
      i->window->Visible = i->visible;
    }
  }
  return S_OK;
}
