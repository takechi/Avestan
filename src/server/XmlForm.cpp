// XmlForm.cpp

#include "stdafx.h"
#include "main.hpp"
#include "object.hpp"

//==============================================================================

namespace {
class FormReader : public XMLHandlerImpl {
 private:
  Templates m_forms;
  int m_current;

 public:
  FormReader() {}

  HRESULT StartDocument() {
    m_current = -1;
    return S_OK;
  }

  HRESULT StartElement(PCWSTR name, size_t cch, XMLAttributes& attr) {
    Template t;
    t.parent = m_current;
    t.type = string(name, cch);
    t.clsid = ToCLSID(string(name, cch));
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

HRESULT FormTemplate(string xmlfile, IXMLReader* sax, Templates& forms) {
  HRESULT hr;
  FormReader reader;
  if FAILED (hr = sax->Parse(&reader, xmlfile)) return hr;
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
    if (t.type == FORMTYPE_FORM)
      indexForm = i;
    else if (t.type == FORMTYPE_PANEL)
      indexPanel = i;
    else if (t.type == FORMTYPE_MAIN)
      indexMain = i;
    //
    ref<IWindow> parent;
    if (t.parent >= 0) parent = forms[t.parent].window;
    t.window.create(t.clsid, parent);
    t.window->Name = t.id;
    // dock
    if (i == indexPanel || i == indexMain)
      t.window->Dock = DirCenter;
    else if (t.parent == indexForm) {
      if (i < indexPanel)
        t.window->Dock = DirNorth;
      else
        t.window->Dock = DirSouth;
    } else if (t.parent == indexPanel) {
      if (i < indexMain)
        t.window->Dock = DirWest;
      else
        t.window->Dock = DirEast;
      t.window->Bounds = Rect(0, 0, 160, 160);
    }
  }
  return S_OK;
}

HRESULT FormDispose(Templates& forms) {
  for (Templates::iterator i = forms.begin(); i != forms.end(); ++i) i->window.dispose();
  return S_OK;
}

namespace {
template <typename T>
ref<T> LoadResource(string relname, ICommands* commands, IXMLReader* sax, ref<T> (*fn)(string, ICommands*, IXMLReader*),
                    UINT errmsg) {
  if (!relname) return null;
  TRACE(L"LoadResource($1)", relname);
  RelativePath path(relname, L"..\\..\\usr");
  if (!PathFileExists(path)) {
    // file not found
    WarningBox(/*m_forms*/ null, string::format(L"$1 が見つかりません", relname));
    return null;
  }
  if (ref<T> res = fn(path, commands, sax)) return res;
  // error
  WarningBox(/*m_forms*/ null, string::load(IDS_ERR_LOAD_GESTURE, relname));
  return null;
}

ref<IImageList> GetShellImageList(int size) {
  ref<IImageList> imagelist;
  afx::ExpGetImageList(size, &imagelist);
  return imagelist;
}

ref<IImageList> LoadImageList(string relname) {
  static const WCHAR SHELLICON_PREFIX[] = L"shell:";
  static const size_t SHELLICON_PREFIX_LEN = 6;
  LPCWSTR buffer = relname.str();
  ASSERT(buffer);
  if (str::compare_nocase(buffer, SHELLICON_PREFIX, SHELLICON_PREFIX_LEN) == 0)
    return GetShellImageList(str::atoi(buffer + SHELLICON_PREFIX_LEN));
  RelativePath path(relname, L"..\\..\\usr");
  if (!PathFileExists(path)) {
    // file not found
    WarningBox(/*m_forms*/ null, string::format(L"$1 が見つかりません", relname));
    return null;
  }
  try {
    return ref<IImageList>(__uuidof(ImageList), string(path));
  } catch (Error&) {
    WarningBox(/*m_forms*/ null, string::load(IDS_ERR_LOAD_IMAGE, relname));
    return null;
  }
}

template <typename T>
void SetWindowExtension(IWindow* window, string relname, ICommands* commands, IXMLReader* sax,
                        ref<T> (*fn)(string, ICommands*, IXMLReader*), UINT errmsg) {
  if (!window) return;
  if (ref<T> extension = LoadResource(relname, commands, sax, fn, errmsg)) {
    VERIFY_HRESULT(window->SetExtension(__uuidof(T), extension));
  }
}

ref<IEditableTreeItem> LoadLinkTreeFromDirectory(PCTSTR dir, function fn) {
  WIN32_FIND_DATA find;
  io::Path basedir(dir);
  basedir.Append(_T("*.*"));
  HANDLE hFind = ::FindFirstFile(basedir, &find);
  if (hFind == INVALID_HANDLE_VALUE) return null;
  ref<IEditableTreeItem> root(__uuidof(DefaultTreeItem));
  do {
    if (lstrcmp(find.cFileName, _T(".")) == 0) continue;
    if (lstrcmp(find.cFileName, _T("..")) == 0) continue;
    io::Path fullpath(dir);
    fullpath.Append(find.cFileName);
    try {
      ref<IFolder> pSubMenu(__uuidof(FolderMenu), string(fullpath));
      cast<ISignal>(pSubMenu)->Connect(EventInvoke, fn);
      root->AddChild(pSubMenu);
    } catch (Error& e) {
      TRACE(e.Message);
    }
  } while (::FindNextFile(hFind, &find));
  ::FindClose(hFind);
  return root;
}
ref<ITreeItem> LoadLinkTree(string relname, function onOpen, IXMLReader* sax) {
  RelativePath xml(relname, L"..\\..\\usr");
  ref<ITreeItem> root;
  if (PathIsDirectory(xml)) {  // link from directory
    return LoadLinkTreeFromDirectory(xml, onOpen);
  } else if (PathFileExists(xml)) {  // link from xml
    return XmlLoadLinks(xml, onOpen, sax);
  }
  return null;
}

HRESULT OnOpenEntry(message msg) {
  theAvesta->OpenOrExecute(msg["what"]);
  return S_OK;
}

// 読み込み失敗用ダミーツリー
ref<ITreeItem> CreateDummyTree(string relname) {
  ref<IEditableTreeItem> root(__uuidof(DefaultTreeItem));
  ref<IEditableTreeItem> disable(__uuidof(DefaultTreeItem));
  disable->Name = string::load(IDS_ERR_OPENFILE, relname);
  root->AddChild(disable);
  return root;
}

void ReloadForm(Template& t, ICommands* commands, IXMLReader* sax) {
  if (!t.window) return;
  SetWindowExtension(t.window, t.keyboard, commands, sax, XmlLoadKeymap, IDS_ERR_LOAD_KEYMAP);
  SetWindowExtension(t.window, t.mouse, commands, sax, XmlLoadGesture, IDS_ERR_LOAD_GESTURE);
  if (ref<ITree> tree = cast(t.window)) {
    if (t.icon) {
      tree->ImageList = LoadImageList(t.icon);
    }
    if (t.item) {
      tree->Root = (t.type == L"LinkBar") ? LoadLinkTree(t.item, OnOpenEntry, sax)
                                          : LoadResource(t.item, commands, sax, XmlLoadTreeItem, IDS_ERR_LOAD_COMMAND);
      if (!tree->Root) tree->Root = CreateDummyTree(t.item);
    }
  }
}
}  // namespace

HRESULT FormReload(Templates& forms, ICommands* commands, IXMLReader* sax) {
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
