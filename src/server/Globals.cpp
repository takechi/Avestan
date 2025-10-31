// Globals.cpp

#include "stdafx.h"
#include "main.hpp"
#include "afx.hpp"
#include "std/buffer.hpp"

const UINT32 INVALID_VALUE = (UINT32)-1;

static const struct {
  const char* name;
  bool value;
} BOOLEANS[] = {
    {
        "CheckBox",
        false,
    },
    {
        "DnDCopyInterDrv",
        true,
    },
    {
        "DistinguishTab",
        true,
    },
    {
        "FullRowSelect",
        false,
    },
    {
        "GestureOnName",
        false,
    },
    {
        "GridLine",
        false,
    },
    {
        "LazyExecute",
        true,
    },
    {
        "LockClose",
        true,
    },
    {
        "LoopCursor",
        true,
    },
    {
        "MiddleSingle",
        true,
    },
    {
        "OpenDups",
        false,
    },
    {
        "OpenNotify",
        true,
    },
    {
        "RestoreCond",
        true,
    },
    {
        "RenameExtension",
        true,
    },
    {
        "PasteInFolder",
        false,
    },
    {
        "Python",
        true,
    },
    {
        "QuietProgress",
        false,
    },
    {
        "TreeAutoSync",
        true,
    },
    {
        "TreeAutoReflect",
        true,
    },
};

STATIC_ASSERT(lengthof(BOOLEANS) == avesta::NumOfBooleans);

static const struct {
  const char* name;
} FONTS[] = {
    {
        "TabFont",
    },
    {
        "AddressFont",
    },
    {
        "ListFont",
    },
    {
        "StatusFont",
    },
};

STATIC_ASSERT(lengthof(FONTS) == avesta::NumOfFonts);

namespace {
static HFONT LoadFont(mew::message m) {
  if (!m) {
    return nullptr;
  }
  try {
    LOGFONT info = {0};
    info.lfHeight = m["size"] | 0;
    info.lfWeight = m["weight"] | 0;
    info.lfItalic = m["italic"] | false;
    ((mew::string)m["name"]).copyto(info.lfFaceName, 32);
    info.lfCharSet = DEFAULT_CHARSET;
    return ::CreateFontIndirect(&info);
  } catch (mew::exceptions::Error&) {
    return nullptr;
  }
}
static mew::message SaveFont(HFONT font) {
  if (!font) {
    return mew::null;
  }
  LOGFONT info;
  ::GetObject(font, sizeof(LOGFONT), &info);
  mew::message m;
  m["name"] = info.lfFaceName;
  m["size"] = (int)info.lfHeight;
  m["italic"] = (bool)(info.lfItalic != 0);
  m["weight"] = (int)info.lfWeight;
  return m;
}
}  // namespace

Avesta::Avesta() : m_ProfilePath(_T("usr\\profile.ini")) {
  for (size_t i = 0; i < avesta::NumOfBooleans; ++i) {
    m_booleans[i] = BOOLEANS[i].value;
  }
  for (size_t i = 0; i < avesta::NumOfFonts; ++i) {
    m_fonts[i] = NULL;
  }
  ResetGlobalVariables();
}

Avesta::~Avesta() {
  for (size_t i = 0; i < avesta::NumOfFonts; ++i) {
    if (m_fonts[i]) {
      ::DeleteObject(m_fonts[i]);
    }
  }
}

void Avesta::LoadFromMessage(mew::message& msg) {
  for (size_t i = 0; i < avesta::NumOfBooleans; ++i) {
    m_booleans[i] = msg[BOOLEANS[i].name] | BOOLEANS[i].value;
  }
  for (size_t i = 0; i < avesta::NumOfFonts; ++i) {
    if (mew::message m = msg[FONTS[i].name]) {
      m_fonts[i] = LoadFont(m);
    }
  }
}

void Avesta::SaveToMessage(mew::message& msg) {
  for (size_t i = 0; i < avesta::NumOfBooleans; ++i) {
    msg[BOOLEANS[i].name] = m_booleans[i];
  }
  for (size_t i = 0; i < avesta::NumOfFonts; ++i) {
    if (mew::message m = SaveFont(m_fonts[i])) {
      msg[FONTS[i].name] = m;
    }
  }
}

HFONT Avesta::FontFromMessage(avesta::Fonts what, mew::message& msg) {
  if (HFONT hFont = (HFONT)(INT_PTR)msg["font"]) {
    if (m_fonts[what]) {
      ::DeleteObject(m_fonts[what]);
    }
    m_fonts[what] = hFont;
  }
  return m_fonts[what];
}

void Avesta::LoadProfile() {
  m_MyDocumentsEntry.clear();
  m_MyDocumentsEntry.create(__uuidof(mew::io::Entry), mew::string(mew::io::GUID_MyDocument));
  m_DefaultNewName = GetProfileString(L"Misc", L"DefaultNewName", L"Untitled");
  m_CommandLineInterval = mew::math::clamp(GetProfileSint32(L"Misc", L"CommandLineInterval", 1000), 0, 60000);
  m_ExposeTime = GetProfileSint32(L"Misc", L"ExposeTime", 400);
}

PCTSTR Avesta::GetDefaultNewName() {
  if (!m_DefaultNewName) {
    LoadProfile();
  }
  return m_DefaultNewName.str();
}

mew::io::IEntry* Avesta::GetMyDocuments() {
  if (!m_MyDocumentsEntry) {
    LoadProfile();
  }
  return m_MyDocumentsEntry;
}

UINT32 Avesta::GetCommandLineInterval() {
  if (m_CommandLineInterval == INVALID_VALUE) {
    LoadProfile();
  }
  return m_CommandLineInterval;
}

UINT32 Avesta::GetExposeTime() {
  if (m_ExposeTime == INVALID_VALUE) {
    LoadProfile();
  }
  return m_ExposeTime;
}

void Avesta::ResetGlobalVariables() {
  m_CommandLineInterval = INVALID_VALUE;
  m_ExposeTime = INVALID_VALUE;
  m_DefaultNewName.clear();
  m_MyDocumentsEntry.clear();
  m_NavigateSoundPath.clear();
}

//==============================================================================

namespace {
static DWORD theDefaultEffect = 0;
static WCHAR theSourcePath[MAX_PATH];

void UpdateDefaultEffect(mew::io::IEntry* dst) {
  if (!dst || !avesta::GetOption(avesta::BoolDnDCopyInterDrv)) {  // dst が指定されていない場合は変更しない
    return;
  } else if (mew::str::empty(theSourcePath)) {  // ソースパスが空……よくわからんのでデフォルトを設定しない
    theDefaultEffect = 0;
  } else {
    mew::string dstpath = dst->Path;
    if (!dstpath) {  // コピー先パスが空……よくわからんのでデフォルトを設定しない
      theDefaultEffect = 0;
    } else if (PathIsSameRoot(theSourcePath, dstpath.str())) {  // 同じドライブなので、移動
      theDefaultEffect = DROPEFFECT_MOVE;
    } else {  // 異なるドライブなので、移動
      theDefaultEffect = DROPEFFECT_COPY;
    }
  }
}

void SetDropEffect(DWORD key, DWORD* effect) {
  // ALT or CTRL+SHIFT -- DROPEFFECT_LINK
  // CTRL              -- DROPEFFECT_COPY
  // SHIFT             -- DROPEFFECT_MOVE
  // no modifier       -- MOVE -> COPY -> LINK
  DWORD op;
  if (key & MK_CONTROL) {
    if (key & MK_SHIFT) {
      op = DROPEFFECT_LINK;
    } else {
      op = DROPEFFECT_COPY;
    }
  } else if (key & MK_SHIFT) {
    op = DROPEFFECT_MOVE;
  } else if (key & MK_ALT) {
    op = DROPEFFECT_LINK;
  } else if (*effect & theDefaultEffect) {
    op = theDefaultEffect;
  } else if (*effect & DROPEFFECT_MOVE) {
    op = DROPEFFECT_MOVE;
  } else if (*effect & DROPEFFECT_COPY) {
    op = DROPEFFECT_COPY;
  } else {
    op = DROPEFFECT_LINK;
  }
  *effect = (op & *effect);
}
}  // namespace

HRESULT mew::ui::ProcessDragEnter(IDataObject* src, mew::io::IEntry* dst, DWORD key, DWORD* effect) {
  if (avesta::GetOption(avesta::BoolDnDCopyInterDrv)) {
    try {
      mew::ref<mew::io::IEntryList> entries(__uuidof(mew::io::EntryList), src);
      if FAILED (afx::ILGetPath(entries->Parent, theSourcePath)) {
        str::clear(theSourcePath);
      }
    } catch (mew::exceptions::Error& e) {
      TRACE(e.Message);
    }
  }
  theDefaultEffect = 0;
  UpdateDefaultEffect(dst);
  SetDropEffect(key, effect);
  return S_OK;
}

HRESULT mew::ui::ProcessDragOver(mew::io::IEntry* dst, DWORD key, DWORD* effect) {
  UpdateDefaultEffect(dst);
  SetDropEffect(key, effect);
  return S_OK;
}

HRESULT mew::ui::ProcessDragLeave() {
  str::clear(theSourcePath);
  theDefaultEffect = 0;
  return S_OK;
}

namespace {
HRESULT DropToExe(IDataObject* src, mew::io::IEntry* dst) {
  mew::string exe = dst->Path;
  if (!PathIsExe(exe.str())) {
    return E_FAIL;
  }
  mew::ref<mew::io::IEntryList> entries(__uuidof(mew::io::EntryList), src);

  size_t count = entries->Count;
  mew::string directory = mew::null;
  mew::StringBuffer args;
  args.reserve(1024);
  for (size_t i = 0; i < count; ++i) {
    mew::ref<mew::io::IEntry> item;
    if SUCCEEDED (entries->GetAt(&item, i)) {
      if (mew::string path = item->Path) {
        if (!directory) {
          WCHAR dir[MAX_PATH];
          path.copyto(dir);
          PathRemoveFileSpec(dir);
          directory = dir;
        }
        args.append_path(path.str());
        args.append(' ');
      }
    }
  }
  args.push_back(_T('\0'));
  HINSTANCE result = ::ShellExecute(nullptr, nullptr, exe.str(), args.str(), directory.str(), SW_SHOWNORMAL);
  return (int)result >= 32 ? S_OK : E_FAIL;
}
}  // namespace

HRESULT mew::ui::ProcessDrop(IDataObject* src, mew::io::IEntry* dst, POINTL pt, DWORD key, DWORD* effect) {
  UpdateDefaultEffect(dst);
  HRESULT hr = E_FAIL;
  if (dst) {
    try {
      ref<IDropTarget> drop;
      if (!(key & MouseButtonRight) && SUCCEEDED(hr = DropToExe(src, dst))) {  // EXE へのドロップ
      } else if SUCCEEDED (dst->QueryObject(&drop)) {
        if (key & MouseButtonRight) {
          if ((key & ModifierMask) ==
              0) {  // 右ドロップ時にキーが押されていないと「ショートカットを作成」がデフォルトになってしまうため。
            switch (theDefaultEffect) {
              case DROPEFFECT_COPY:
                key |= ModifierControl;
                break;
              case DROPEFFECT_LINK:
                key |= ModifierAlt;
                break;
              case DROPEFFECT_MOVE:
              default:
                key |= ModifierShift;
                break;
            }
          }
        } else {
          SetDropEffect(key, effect);
        }
        hr = drop->Drop(src, key, pt, effect);
      }
    } catch (...) {
    }
  }
  str::clear(theSourcePath);
  theDefaultEffect = 0;
  return hr;
}

namespace {
struct ImpExpStruct {
  ImpExpStruct(bool c) : close(c), navigate(avesta::NaviOpen) {}
  bool close;
  avesta::Navigation navigate;
};
static HRESULT EnumShellWindow(HWND hwnd, LPCITEMIDLIST pidl, LPARAM lParam) {
  mew::ref<mew::io::IEntry> folder;
  if (SUCCEEDED(CreateEntry(&folder, pidl)) && folder->IsFolder()) {
    ImpExpStruct* imp = (ImpExpStruct*)lParam;
    avesta::Navigation naviResult;
    if (mew::ref<mew::ui::IShellListView> view = theAvesta->OpenFolder(folder, imp->navigate, &naviResult)) {
      switch (naviResult) {
        case avesta::NaviOpen:
        case avesta::NaviOpenAlways:
          imp->navigate = avesta::NaviAppend;
          break;
        default:
          imp->navigate = naviResult;
          break;
      }
    }
    if (imp->close) {
      ::PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
  }
  return S_OK;
}
}  // namespace

HRESULT ImportExplorer(bool close) {
  ImpExpStruct imp(close);
  afx::ExpEnumExplorers(EnumShellWindow, (LPARAM)&imp);
  return S_OK;
}
