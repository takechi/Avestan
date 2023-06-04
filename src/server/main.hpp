// main.hpp
#pragma once

#include <limits>

#include "afx.hpp"
#include "error.hpp"
#include "io.hpp"
#include "math.hpp"
#include "resource.h"
#include "shell.hpp"
#include "widgets.hpp"
#include "xml.hpp"

#include "AvestaSDK.hpp"
#include "utils.hpp"

namespace module {
extern HINSTANCE Handle;
}

enum ThreeStateBool {
  BoolTrue = 1,
  BoolFalse = 0,
  BoolUnknown = -1,
};

enum AvestaCommand {
  AVESTA_New = 'new_',
  AVESTA_NewFolder = 'newf',
  AVESTA_SelectPattern = 'sptn',
  AVESTA_Show = 'show',
  AVESTA_Hide = 'hide',
  AVESTA_Export = 'eexp',
  AVESTA_CopyTo = 'cpto',
  AVESTA_MoveTo = 'mvto',
  AVESTA_PasteTo = 'psto',
  AVESTA_CopyToOther = 'cpt2',
  AVESTA_MoveToOther = 'mvt2',
  AVESTA_CopyCheckedTo = 'ccto',
  AVESTA_MoveCheckedTo = 'mcto',
  AVESTA_CopyHere = 'cphr',
  AVESTA_CopyBase = 'cpnb',
  AVESTA_CopyName = 'cpnm',
  AVESTA_CopyPath = 'cpph',
  AVESTA_RenameDialog = 'rndg',
  AVESTA_RenamePaste = 'psnm',
  AVESTA_PatternMask = 'mask',
  AVESTA_Find = 'find',
  AVESTA_SyncDesc = 'sync',
  AVESTA_AutoArrange = 'atap',
  AVEOBS_AutoArrange = 'atao',
  AVESTA_Grouping = 'grpp',
  AVEOBS_Grouping = 'grpo',
  AVESTA_ShowAllFiles = 'safp',
  AVEOBS_ShowAllFiles = 'safo',
};

avesta::Navigation ParseNavigate(PCWSTR text, avesta::Navigation defaultNavi);

//==============================================================================
// avesta function sets

class RelativePath {
 private:
  TCHAR m_path[MAX_PATH];

 public:
  RelativePath(PCTSTR path, PCTSTR basedir = _T("..\\..")) { init(path, basedir); }
  RelativePath(const mew::string& path, PCTSTR basedir = _T("..\\..")) { init(path.str(), basedir); }
  operator PCTSTR() const { return m_path; }

 private:
  void init(PCTSTR path, PCTSTR basedir) {
    if (PathIsRelative(path)) {
      ::GetModuleFileName(NULL, m_path, MAX_PATH);
      PathAppend(m_path, basedir);
      PathAppend(m_path, path);
    } else {
      mew::str::copy(m_path, path);
    }
  }
};

class Avesta : public avesta::IAvesta {
 public:
  mew::string m_DefaultNewName;
  mew::string m_NavigateSoundPath;
  UINT32 m_ExposeTime;
  UINT32 m_CommandLineInterval;
  UINT32 m_EditOptions;
  UINT32 m_MiddleClick;
  mew::ref<mew::io::IEntry> m_MyDocumentsEntry;

  RelativePath m_ProfilePath;
  bool m_booleans[avesta::NumOfBooleans];
  HFONT m_fonts[avesta::NumOfFonts];

  void LoadProfile();

  void LoadFromMessage(mew::message& msg);
  void SaveToMessage(mew::message& msg);
  HFONT FontFromMessage(avesta::Fonts what, mew::message& msg);

  Avesta();
  ~Avesta();

  virtual avesta::Navigation NavigateVerb(mew::ui::IShellListView* folder, mew::io::IEntry* where, bool locked,
                                          avesta::Navigation defaultVerb) = 0;
  virtual HRESULT SyncDescendants(mew::io::IEntry* pFolder) = 0;

  DWORD get_EditOptions() const {
    DWORD ret = m_EditOptions;
    if (m_booleans[avesta::BoolRenameExtension]) {
      ret |= afx::RenameExtension;
    }
    return ret;
  }
  __declspec(property(get = get_EditOptions)) DWORD EditOptions;

  UINT32 get_MiddleClick() const { return m_MiddleClick; }
  __declspec(property(get = get_MiddleClick)) UINT32 MiddleClick;

  //==============================================================================

#define DEF_BOOLEAN_PROPERTY(name)                                        \
  bool get_##name() const { return m_booleans[avesta::Bool##name]; }      \
  void set_##name(bool value) { m_booleans[avesta::Bool##name] = value; } \
  __declspec(property(get = get_##name, put = set_##name)) bool name

  DEF_BOOLEAN_PROPERTY(DistinguishTab);
  DEF_BOOLEAN_PROPERTY(LazyExecute);
  DEF_BOOLEAN_PROPERTY(LoopCursor);
  DEF_BOOLEAN_PROPERTY(GestureOnName);
  DEF_BOOLEAN_PROPERTY(MiddleSingle);
  DEF_BOOLEAN_PROPERTY(OpenDups);
  DEF_BOOLEAN_PROPERTY(PasteInFolder);
  DEF_BOOLEAN_PROPERTY(TreeAutoReflect);

#undef DEF_BOOLEAN_PROPERTY

  //==============================================================================

  PCTSTR GetDefaultNewName();
  mew::io::IEntry* GetMyDocuments();
  UINT32 GetCommandLineInterval();
  UINT32 GetExposeTime();

  PCTSTR GetProfilePath() const { return m_ProfilePath; }
  bool GetProfileBool(PCTSTR group, PCTSTR key, bool defaultValue) const {
    return mew::io::IniGetBool(GetProfilePath(), group, key, defaultValue);
  }
  INT32 GetProfileSint32(PCTSTR group, PCTSTR key, UINT32 defaultValue) const {
    return mew::io::IniGetSint32(GetProfilePath(), group, key, defaultValue);
  }
  mew::string GetProfileString(PCTSTR group, PCTSTR key, PCTSTR defaultValue) const {
    return mew::io::IniGetString(GetProfilePath(), group, key, defaultValue);
  }

  void ResetGlobalVariables();

  virtual mew::ref<mew::ui::IEditableTreeItem> CreateMRUTreeItem() = 0;
  virtual mew::ref<mew::ui::IEditableTreeItem> CreateFolderTreeItem(mew::ui::Direction dir) = 0;
};

extern Avesta* theAvesta;

//==============================================================================
// avesta interface

__interface __declspec(uuid("8770FE4B-0DEB-434E-9D3F-38C1E012031B")) ICallback : mew::IDisposable {
  mew::string Caption(const mew::string& name, const mew::string& path);
  mew::string StatusText(const mew::string& text, mew::ui::IShellListView* view);
  mew::string GestureText(const mew::ui::Gesture gesture[], size_t length, const mew::string& description);
  avesta::Navigation NavigateVerb(mew::io::IEntry * current, mew::io::IEntry * where, UINT mods, bool locked,
                                  avesta::Navigation defaultVerb);
  mew::string ExecuteVerb(mew::io::IEntry * current, mew::io::IEntry * what, UINT mods);
  mew::string WallPaper(const mew::string& filename, const mew::string& name, const mew::string& path);
};

//==============================================================================
// avesta class

class __declspec(uuid("A745A59B-943E-4707-B14A-66C2A335D39F")) FolderTree;
class __declspec(uuid("046BEBA9-FA05-47E8-B07B-15A419BC825C")) ShellStorage;
class __declspec(uuid("DD7C4DDD-8148-40E5-A19E-D58D306622A8")) DefaultCallback;
class __declspec(uuid("1AC80FAE-41A3-4C0F-AC4A-4C06DE7C7CEA")) PythonCallback;

HRESULT ImportExplorer(bool close);
HRESULT RenameDialog(HWND hWnd, mew::io::IEntry* parent, mew::io::IEntryList* entries, bool paste);

//==============================================================================
// XML resource

struct Template {
  // pattern
  int parent;
  mew::string type;
  GUID clsid;
  // string name;
  mew::string id;
  mew::string keyboard;
  mew::string mouse;
  mew::string item;
  mew::string icon;

  // volatile
  mew::ref<mew::ui::IWindow> window;
  bool visible;
};

using Templates = std::vector<Template>;

mew::ref<mew::ui::IKeymap> XmlLoadKeymap(mew::string xmlfile, mew::ICommands* commands, mew::xml::IXMLReader* sax);
mew::ref<mew::ui::IGesture> XmlLoadGesture(mew::string xmlfile, mew::ICommands* commands, mew::xml::IXMLReader* sax);
mew::ref<mew::ui::ITreeItem> XmlLoadTreeItem(mew::string xmlfile, mew::ICommands* commands, mew::xml::IXMLReader* sax);
mew::ref<mew::ui::ITreeItem> XmlLoadLinks(mew::string xmlfile, mew::function onOpen, mew::xml::IXMLReader* sax);

HRESULT FormTemplate(mew::string xmlfile, mew::xml::IXMLReader* sax, Templates& templates);
HRESULT FormGenerate(Templates& forms);
HRESULT FormDispose(Templates& forms);
HRESULT FormReload(Templates& forms, mew::ICommands* commands, mew::xml::IXMLReader* sax);
HRESULT FormComponentsHide(Templates& forms);
HRESULT FormComponentsRestore(Templates& forms);

namespace avesta {
REFCLSID ToCLSID(const mew::string& value);
mew::string XmlAttrText(mew::xml::XMLAttributes& attr);
int XmlAttrImage(mew::xml::XMLAttributes& attr);
UINT8 XmlAttrKey(mew::xml::XMLAttributes& attr);
UINT16 XmlAttrModifiers(mew::xml::XMLAttributes& attr);
mew::ref<mew::ui::IEditableTreeItem> XmlAttrTreeItem(mew::xml::XMLAttributes& attr, mew::ICommands* commands);
mew::ref<mew::ICommand> XmlAttrCommand(mew::xml::XMLAttributes& attr, mew::ICommands* commands);
}  // namespace avesta

inline bool IsLocked(mew::ui::IWindow* view, mew::ui::IList* parent) {
  DWORD status = 0;
  parent->GetStatus(view, &status);
  return (status & mew::CHECKED) != 0;
}

//==============================================================================

extern const CLIPFORMAT CF_SHELLIDLIST;
