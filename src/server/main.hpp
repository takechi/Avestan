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

using namespace mew;
using namespace mew::exceptions;
using namespace mew::ui;
using namespace mew::io;
using namespace mew::xml;

#include "AvestaSDK.hpp"
#include "utils.hpp"

using namespace ave;
using namespace avesta;

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

Navigation ParseNavigate(PCWSTR text, Navigation defaultNavi);

//==============================================================================
// avesta function sets

class RelativePath {
 private:
  TCHAR m_path[MAX_PATH];

 public:
  RelativePath(PCTSTR path, PCTSTR basedir = _T("..\\..")) { init(path, basedir); }
  RelativePath(const string& path, PCTSTR basedir = _T("..\\..")) { init(path.str(), basedir); }
  operator PCTSTR() const { return m_path; }
  operator string() const { return string(m_path); }

 private:
  void init(PCTSTR path, PCTSTR basedir) {
    if (PathIsRelative(path)) {
      ::GetModuleFileName(NULL, m_path, MAX_PATH);
      PathAppend(m_path, basedir);
      PathAppend(m_path, path);
    } else {
      str::copy(m_path, path);
    }
  }
};

class Avesta : public IAvesta {
 public:
  string m_DefaultNewName;
  string m_NavigateSoundPath;
  UINT32 m_ExposeTime;
  UINT32 m_CommandLineInterval;
  UINT32 m_EditOptions;
  UINT32 m_MiddleClick;
  ref<IEntry> m_MyDocumentsEntry;

  RelativePath m_ProfilePath;
  bool m_booleans[NumOfBooleans];
  HFONT m_fonts[NumOfFonts];

  void LoadProfile();

  void LoadFromMessage(message& msg);
  void SaveToMessage(message& msg);
  HFONT FontFromMessage(Fonts what, message& msg);

  Avesta();
  ~Avesta();

  virtual Navigation NavigateVerb(IShellListView* folder, IEntry* where, bool locked, Navigation defaultVerb) = 0;
  virtual HRESULT SyncDescendants(io::IEntry* pFolder) = 0;

  DWORD get_EditOptions() const {
    DWORD ret = m_EditOptions;
    if (m_booleans[BoolRenameExtension]) ret |= afx::RenameExtension;
    return ret;
  }
  __declspec(property(get = get_EditOptions)) DWORD EditOptions;

  UINT32 get_MiddleClick() const { return m_MiddleClick; }
  __declspec(property(get = get_MiddleClick)) UINT32 MiddleClick;

  //==============================================================================

#define DEF_BOOLEAN_PROPERTY(name)                                \
  bool get_##name() const { return m_booleans[Bool##name]; }      \
  void set_##name(bool value) { m_booleans[Bool##name] = value; } \
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
  IEntry* GetMyDocuments();
  UINT32 GetCommandLineInterval();
  UINT32 GetExposeTime();

  PCTSTR GetProfilePath() const { return m_ProfilePath; }
  bool GetProfileBool(PCTSTR group, PCTSTR key, bool defaultValue) const {
    return io::IniGetBool(GetProfilePath(), group, key, defaultValue);
  }
  INT32 GetProfileSint32(PCTSTR group, PCTSTR key, UINT32 defaultValue) const {
    return io::IniGetSint32(GetProfilePath(), group, key, defaultValue);
  }
  string GetProfileString(PCTSTR group, PCTSTR key, PCTSTR defaultValue) const {
    return io::IniGetString(GetProfilePath(), group, key, defaultValue);
  }

  void ResetGlobalVariables();

  virtual ref<IEditableTreeItem> CreateMRUTreeItem() = 0;
  virtual ref<IEditableTreeItem> CreateFolderTreeItem(Direction dir) = 0;
};

extern Avesta* theAvesta;

//==============================================================================
// avesta interface

__interface __declspec(uuid("8770FE4B-0DEB-434E-9D3F-38C1E012031B")) ICallback : IDisposable {
  string Caption(const string& name, const string& path);
  string StatusText(const string& text, IShellListView* view);
  string GestureText(const Gesture gesture[], size_t length, const string& description);
  Navigation NavigateVerb(IEntry * current, IEntry * where, UINT mods, bool locked, Navigation defaultVerb);
  string ExecuteVerb(IEntry * current, IEntry * what, UINT mods);
  string WallPaper(const string& filename, const string& name, const string& path);
};

//==============================================================================
// avesta class

class __declspec(uuid("A745A59B-943E-4707-B14A-66C2A335D39F")) FolderTree;
class __declspec(uuid("046BEBA9-FA05-47E8-B07B-15A419BC825C")) ShellStorage;
class __declspec(uuid("DD7C4DDD-8148-40E5-A19E-D58D306622A8")) DefaultCallback;
class __declspec(uuid("1AC80FAE-41A3-4C0F-AC4A-4C06DE7C7CEA")) PythonCallback;

HRESULT ImportExplorer(bool close);
HRESULT RenameDialog(HWND hWnd, IEntry* parent, IEntryList* entries, bool paste);

//==============================================================================
// XML resource

struct Template {
  // pattern
  int parent;
  string type;
  GUID clsid;
  // string name;
  string id;
  string keyboard;
  string mouse;
  string item;
  string icon;

  // volatile
  ref<IWindow> window;
  bool visible;
};

using Templates = std::vector<Template>;

ref<IKeymap> XmlLoadKeymap(string xmlfile, ICommands* commands, IXMLReader* sax);
ref<IGesture> XmlLoadGesture(string xmlfile, ICommands* commands, IXMLReader* sax);
ref<ITreeItem> XmlLoadTreeItem(string xmlfile, ICommands* commands, IXMLReader* sax);
ref<ITreeItem> XmlLoadLinks(string xmlfile, function onOpen, IXMLReader* sax);

HRESULT FormTemplate(string xmlfile, IXMLReader* sax, Templates& templates);
HRESULT FormGenerate(Templates& forms);
HRESULT FormDispose(Templates& forms);
HRESULT FormReload(Templates& forms, ICommands* commands, IXMLReader* sax);
HRESULT FormComponentsHide(Templates& forms);
HRESULT FormComponentsRestore(Templates& forms);

namespace avesta {
REFCLSID ToCLSID(const string& value);
string XmlAttrText(XMLAttributes& attr);
int XmlAttrImage(XMLAttributes& attr);
UINT8 XmlAttrKey(XMLAttributes& attr);
UINT16 XmlAttrModifiers(XMLAttributes& attr);
ref<IEditableTreeItem> XmlAttrTreeItem(XMLAttributes& attr, ICommands* commands);
ref<ICommand> XmlAttrCommand(XMLAttributes& attr, ICommands* commands);
}  // namespace avesta

inline bool IsLocked(IWindow* view, IList* parent) {
  DWORD status = 0;
  parent->GetStatus(view, &status);
  return (status & CHECKED) != 0;
}

//==============================================================================

extern const CLIPFORMAT CF_SHELLIDLIST;
