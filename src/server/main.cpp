// main.cpp

#include "stdafx.h"

// include order barrier

#include "afx.hpp"
#include "std/deque.hpp"

int theMainResult = 0;
PWSTR* theNewAvesta = NULL;
HINSTANCE thePygmy = NULL;

#include <mmsystem.h>

#include "CommandProvider.hpp"
#include "FolderList.hpp"
#include "TaskTrayProvider.hpp"
#include "impl/InitUtil.hpp"
#include "object.hpp"

//==============================================================================
// Command utils

struct CommandEntry {
  PCTSTR Name;
  PCTSTR Description;
};

inline mew::ref<mew::ICommands> CreateCommands(const CommandEntry entries[], size_t num) {
  mew::ref<mew::ICommands> table(__uuidof(mew::Commands));
  for (size_t i = 0; i < num; ++i) {
    table->Add(entries[i].Name, entries[i].Description);
  }
  return table;
}

#include "command.hpp"

// {E40DF43F-37CA-4b05-871E-41C17B911C04}
static const GUID STATUS_VERSION_1 = {0xe40df43f, 0x37ca, 0x4b05, {0x87, 0x1e, 0x41, 0xc1, 0x7b, 0x91, 0x1c, 0x4}};

// {91F09FA5-205A-4c81-9490-1A901ED23EDD}
static const GUID STATUS_VERSION_2 = {0x91f09fa5, 0x205a, 0x4c81, {0x94, 0x90, 0x1a, 0x90, 0x1e, 0xd2, 0x3e, 0xdd}};

// {B83EB453-04B5-4dd3-8EC3-6FAB62AADE71}
static const GUID STORAGE_VERSION_1 = {0xb83eb453, 0x4b5, 0x4dd3, {0x8e, 0xc3, 0x6f, 0xab, 0x62, 0xaa, 0xde, 0x71}};

//==============================================================================

const CLIPFORMAT CF_SHELLIDLIST = (CLIPFORMAT)::RegisterClipboardFormat(CFSTR_SHELLIDLIST);

Avesta* theAvesta;

//==============================================================================

const bool useRedirectToMyDocuments = true;

//==============================================================================

#include "CommandList.hpp"

//==============================================================================

template <class TBase>
class __declspec(novtable) MainBase : public mew::StaticLife<TBase>, public Avesta {
 public:
  Templates m_forms;

  mew::ui::IWindow* get_display() const { return m_forms[0].window; }
  __declspec(property(get = get_display)) mew::ui::IWindow* m_display;

  mew::ref<mew::ui::IForm> m_form;        ///< メインフレームウィンドウ.
  mew::ref<mew::ui::ITabPanel> m_tab;     ///< メインビューコンテナ.
  mew::ref<mew::ui::ITree> m_status;      ///< ステータスバー.
  mew::ref<mew::ui::ITreeView> m_tree;    ///< フォルダツリー.
  mew::ref<mew::ui::IPreview> m_preview;  ///< プレビュー.
  mew::ref<ICallback> m_callback;

 protected:  // status
  DWORD m_StatusLastModify;
  DWORD m_Notify;
  void SetStatusText(DWORD priority, mew::string text) {
    if (!m_status) {
      return;
    }
    // TODO: メッセージキュー化
    HWND hwndStatus = m_status->Handle;
    int len = SendMessage(hwndStatus, SB_GETTEXTLENGTH, 1, 0);
    DWORD now = ::GetTickCount();
    if (len == 0 || now - m_StatusLastModify > 1000 || m_Notify <= priority) {
      m_StatusLastModify = now;
      m_Notify = priority;
      /// FIXME: 「名前」と「テキスト」を別に持ちたいので、2つ目のペインを使う。
      SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM)text.str());
      // m_status->Name = text;
    }
  }
  mew::string FormatViewStatusText(mew::ui::IShellListView* view, mew::string text) {
    ASSERT(view);
    if (m_callback) {
      return m_callback->StatusText(text, view);
    } else {
      return text;
    }
  }
};

//==============================================================================

class Main : public mew::Root<mew::implements<IDropTarget, mew::ui::IGesture, mew::ui::IKeymap, mew::ui::IWallPaper>,
                              mew::mixin<FolderListContainer, TaskTrayProvider, CommandProvider, MainBase> > {
 private:
  mew::ref<mew::ui::IShellStorage> m_storage;

  MRUList m_mru;

  size_t m_ParseCommandLineTime;

 public:
  Main()  // : m_UpList(this, DirNorth), m_BackList(this, DirWest),
          // m_FwdList(this, DirEast)
  {
    ASSERT(!theAvesta);
    theAvesta = this;
    m_StatusLastModify = ::GetTickCount();
    m_Notify = 0;
    m_EditOptions = 0;
    m_MiddleClick = mew::ui::ModifierNone;
  }
  ~Main() {
    ASSERT(theAvesta == this);
    theAvesta = nullptr;
  }
  static mew::string GetConfigPath() { return RelativePath(_T("var\\config.xml")); }
#ifdef _M_X64
  static mew::string GetStoragePath() { return RelativePath(_T("var\\settings64.dat")); }
  static mew::string GetDefaultSaveName() { return RelativePath(_T("var\\default64.ave")); }
#else
  static string GetStoragePath() { return RelativePath(_T("var\\settings.dat")); }
  static string GetDefaultSaveName() { return RelativePath(_T("var\\default.ave")); }
#endif
  static mew::ui::Direction GetDock(mew::message& msg, PCSTR name, mew::ui::Direction defaultValue) {
    int dock = msg[name];
    dock &= (mew::ui::DirCenter | mew::ui::DirWest | mew::ui::DirEast | mew::ui::DirNorth | mew::ui::DirSouth);
    if (dock == 0 && defaultValue != mew::ui::DirNone) {
      return defaultValue;
    } else {
      return (mew::ui::Direction)dock;
    }
  }
  static bool IsMinimizeShowCommand(INT sw) {
    switch (sw) {
      case SW_MINIMIZE:
      case SW_SHOWMAXIMIZED:  // == SW_MAXIMIZE
      case SW_SHOWMINIMIZED:
      case SW_SHOWMINNOACTIVE:
      case SW_FORCEMINIMIZE:
        return true;
      default:
        return false;
    }
  }
  static bool SetFont(mew::ui::IWindow* window, HFONT font, int index = 0) {
    if (!window || !font) {
      return false;
    }
    ::SendMessage(window->Handle, WM_SETFONT, (WPARAM)font, index);
    return true;
  }
  void LoadStatus(INT& sw, bool& maximized) {
    m_dropLocation = m_form->Location;

    try {
      mew::message msg = mew::xml::LoadMessage(GetConfigPath());
      bool isSameResolution = (m_display->Bounds == msg["Display.Bounds"]);
      if (m_tree) {
        m_tree->Bounds = msg["Tree.Bounds"] | mew::Rect(0, 0, 200, 200);
        m_tree->Visible = msg["Tree.Visible"] | true;
      }
      if (m_status) {
        m_status->Visible = msg["Status.Visible"] | true;
      }
      if (m_preview) {
        if (isSameResolution) m_preview->Bounds = msg["Preview.Bounds"];
        m_preview->Visible = msg["Preview.Visible"] | true;
      }
      m_tab->InsertPosition = (mew::ui::InsertTo)(msg["Tab.Insert"] | 1);
      m_tab->Arrange = (mew::ui::ArrangeType)(msg["Tab.Arrange"] | 0);
      LoadFromMessage(msg);
      m_EditOptions = msg["Keybind"] | 0;
      m_MiddleClick = msg["MiddleClick"] | 0;
      m_form->AutoCopy = (mew::ui::CopyMode)(msg["AutoCopy"] | 0);
      WallPaperFile = msg["Folder.BkFile"] | mew::string();
      TaskTray_Load(msg);
      for (Templates::const_iterator i = m_forms.begin(); i != m_forms.end(); ++i) {
        if (mew::ref<mew::IPersistMessage> persist = cast(i->window)) {
          LoadPersistMessage(persist, msg, CW2AEX<16>(i->id.str()));
        }
      }
      maximized = msg["Form.Maximize"];
      if (isSameResolution) {  // 解像度が変わっていない場合のみ。
        m_form->Bounds = msg["Form.Bounds"];
        if (!IsMinimizeShowCommand(sw) && maximized) {
          sw = SW_SHOWMAXIMIZED;
        }
        m_dropLocation = msg["Drop.Location"] | m_dropLocation;
      }
      SetAlwaysTop(msg["Form.AlwaysTop"] | false);
    } catch (mew::exceptions::Error&) {
      // config.xml が無いのかな？
    }
    // フォント.
    if (m_tab && m_fonts[avesta::FontTab]) {
      SetFont(m_tab, m_fonts[avesta::FontTab]);
    }
    if (m_status && m_fonts[avesta::FontStatus]) {
      SetFont(m_status, m_fonts[avesta::FontStatus]);
    }
    // Python
    if (m_booleans[avesta::BoolPython]) {
      // チェックのためのロード
      if (HINSTANCE hPython = LoadLibrary(_T("python311"))) {
        thePygmy = LoadLibrary(_T("pygmy.pyd"));
        if (thePygmy) {
          m_callback.create(__uuidof(PythonCallback));
          // pygmy側でロードしているため、チェック用の参照を解放する
          ::FreeLibrary(hPython);
        }
      }
      if (!m_callback) {
        PCTSTR errormsg =
            _T("python311.dll または pygmy.pyd ")
            _T("が見つからないため、スクリプト拡張が\n")
            _T("使用できません。\n")
            _T("スクリプト拡張を使うためには、python ")
            _T("をインストールしてください。\n\n")
            _T("次回起動時に、もう一度 python の有無を確認しますか？\n")
            _T("「いいえ」の場合には、python 拡張を無効にします。");
        switch (::MessageBox(NULL, errormsg, _T("Avesta"), MB_YESNO | MB_ICONINFORMATION)) {
          case IDYES:
            m_booleans[avesta::BoolPython] = true;
            break;
          case IDNO:
            m_booleans[avesta::BoolPython] = false;
            break;
        }
        m_callback.create(__uuidof(DefaultCallback));
      }
    } else {
      m_callback.create(__uuidof(DefaultCallback));
    }
    //
    try {
      mew::Stream stream(__uuidof(mew::io::FileReader), GetStoragePath());
      GUID version;
      stream >> version;
      if (version == STORAGE_VERSION_1) {
        stream >> m_storage;
        stream >> m_mru;
      }
    } catch (mew::exceptions::Error&) {
    }
    if (!m_storage) {
      m_storage.create(__uuidof(ShellStorage));
    }
  }
  void SaveStatus() {
    mew::message msg;
    WINDOWPLACEMENT place = {sizeof(WINDOWPLACEMENT)};
    HWND hwnd = m_form->Handle;
    ::GetWindowPlacement(hwnd, &place);
    bool bMaximized =
        (place.showCmd == SW_MAXIMIZE || (IsMinimizeShowCommand(place.showCmd) && (place.flags & WPF_RESTORETOMAXIMIZED) != 0));
    RECT rc;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
    mew::Rect bounds(place.rcNormalPosition);
    bounds.x += rc.left;
    bounds.y += rc.top;
    msg["Display.Bounds"] = m_display->Bounds;
    msg["Form.Bounds"] = bounds;
    msg["Form.Maximize"] = bMaximized;
    msg["Drop.Location"] = m_dropLocation;
    msg["Keybind"] = m_EditOptions;
    msg["MiddleClick"] = m_MiddleClick;
    msg["AutoCopy"] = (int)m_form->AutoCopy;
    msg["Form.AlwaysTop"] = GetAlwaysTop();
    if (m_tree) {
      msg["Tree.Bounds"] = m_tree->Bounds;
      msg["Tree.Visible"] = m_tree->Visible;
    }
    if (m_status) {
      msg["Status.Visible"] = m_status->Visible;
    }
    if (m_preview) {
      msg["Preview.Bounds"] = m_preview->Bounds;
      msg["Preview.Visible"] = m_preview->Visible;
    }
    msg["Folder.BkFile"] = this->WallPaperFile;
    msg["Tab.Insert"] = (int)m_tab->InsertPosition;
    msg["Tab.Arrange"] = (int)m_tab->Arrange;
    SaveToMessage(msg);
    TaskTray_Save(msg);
    for (Templates::const_iterator i = m_forms.begin(); i != m_forms.end(); ++i) {
      if (mew::ref<mew::IPersistMessage> persist = cast(i->window)) {
        SavePersistMessage(persist, msg, CW2AEX<16>(i->id.str()));
      }
    }
    //
    try {
      mew::xml::SaveMessage(msg, mew::Stream(__uuidof(mew::io::FileWriter), GetConfigPath()));
    } catch (mew::exceptions::Error&) {
    }
    //
    if (mew::ref<mew::ISerializable> serial = cast(m_storage)) {
      try {
        mew::Stream stream(__uuidof(mew::io::FileWriter), GetStoragePath());
        stream << STORAGE_VERSION_1;
        stream << serial;
        stream << m_mru;
      } catch (mew::exceptions::Error&) {
      }
    }
  }
  void ShowForm(INT sw, bool maximized) {
    HWND hwnd = m_form->Handle;
    ASSERT(::IsWindow(hwnd));
    ::ShowWindow(hwnd, sw);
    if (maximized) {
      WINDOWPLACEMENT place = {sizeof(WINDOWPLACEMENT)};
      ::GetWindowPlacement(hwnd, &place);
      if (IsMinimizeShowCommand(place.showCmd)) {
        place.flags |= WPF_RESTORETOMAXIMIZED;
        ::SetWindowPlacement(hwnd, &place);
      }
    }
  }

  HRESULT HandleOtherFocus(mew::message m) {
    if (m_booleans[avesta::BoolQuietProgress] && m_form) {
      if (HWND hwnd = ::GetAncestor((HWND)(INT_PTR)m["hwnd"], GA_ROOT)) {
        avesta::FileOperationHack(hwnd, m_form->Handle);
      }
    }
    return S_OK;
  }
  void Run(PCWSTR args, INT sw) {
    try {
      InitForms();

      bool maximized = false;
      LoadStatus(sw, maximized);

      ::RegisterDragDrop(m_form->Handle, mew::cast<IDropTarget>(m_form));
      m_form->SetExtension(__uuidof(IDropTarget), OID);

      m_ParseCommandLineTime = GetTickCount();
      ParseCommandLine(args);

      if (!CurrentView() &&
          m_booleans[avesta::BoolRestoreCond]) {  // 起動時に開くフォルダが与えられていなければ、状態を復元する
        // 起動中はOpenNotifyを無効化
        bool tmp = m_booleans[avesta::BoolOpenNotify];
        m_booleans[avesta::BoolOpenNotify] = false;
        mew::string error;
        DoOpen(GetDefaultSaveName(), error);
        m_booleans[avesta::BoolOpenNotify] = tmp;
      }

      if (!CurrentView()) {  // 一つのフォルダを開いていない場合、キャプションが空のままになっている。
        UpdateCaption(mew::null);
      }

      ShowForm(sw, maximized);

      // main loop
      m_display->Update(true);
    } catch (mew::exceptions::Error&) {
      Dispose();
      throw;
    }
  }
  void Dispose() noexcept {
    __super::Dispose();
    FormDispose(m_forms);
    m_commands.dispose();
    m_status.dispose();
    m_preview.dispose();
    m_tree.dispose();
    m_tab.dispose();
    m_form.dispose();
    m_mru.clear();
    m_callback.dispose();
    m_storage.dispose();
    ResetGlobalVariables();
  }

 public:  // Avesta
  HRESULT OptionCheckBox(mew::message = mew::null) {
    m_booleans[avesta::BoolCheckBox] = !m_booleans[avesta::BoolCheckBox];
    for (mew::each<mew::ui::IShellListView> i = EnumFolders(mew::StatusNone); i.next();) {
      i->CheckBox = m_booleans[avesta::BoolCheckBox];
    }
    return S_OK;
  }
  HRESULT OptionFullRowSelect(mew::message = mew::null) {
    m_booleans[avesta::BoolFullRowSelect] = !m_booleans[avesta::BoolFullRowSelect];
    for (mew::each<mew::ui::IShellListView> i = EnumFolders(mew::StatusNone); i.next();) {
      i->FullRowSelect = m_booleans[avesta::BoolFullRowSelect];
    }
    return S_OK;
  }
  HRESULT OptionGridLine(mew::message = mew::null) {
    m_booleans[avesta::BoolGridLine] = !m_booleans[avesta::BoolGridLine];
    for (mew::each<mew::ui::IShellListView> i = EnumFolders(mew::StatusNone); i.next();) {
      i->GridLine = m_booleans[avesta::BoolGridLine];
    }
    return S_OK;
  }
  HRESULT OptionRenameExtension(mew::message = mew::null) {
    m_booleans[avesta::BoolRenameExtension] = !m_booleans[avesta::BoolRenameExtension];
    for (mew::each<mew::ui::IShellListView> i = EnumFolders(mew::StatusNone); i.next();) {
      i->RenameExtension = m_booleans[avesta::BoolRenameExtension];
    }
    return S_OK;
  }
  HRESULT OptionPython(mew::message = mew::null) {
    m_booleans[avesta::BoolPython] = !m_booleans[avesta::BoolPython];
    ave::InfoBox(m_form, L"再起動後有効になります");
    return S_OK;
  }
  HRESULT OptionBoolean(mew::message msg) {
    m_booleans[msg.code] = !m_booleans[msg.code];
    return S_OK;
  }
  HRESULT ObserveBoolean(mew::message msg) {
    msg["state"] = mew::ENABLED | (m_booleans[msg.code] ? mew::CHECKED : 0);
    return S_OK;
  }
  bool GetAlwaysTop() const {
    if (!m_form) {
      return false;
    }
    return 0 != (::GetWindowLong(m_form->Handle, GWL_EXSTYLE) & WS_EX_TOPMOST);
  }
  void SetAlwaysTop(bool top) {
    if (!m_form) {
      return;
    }
    ::SetWindowPos(m_form->Handle, top ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE);
  }
  HRESULT OptionAlwaysTop(mew::message = mew::null) {
    if (!m_form) {
      return E_FAIL;
    }
    SetAlwaysTop(!GetAlwaysTop());
    return S_OK;
  }
  HRESULT ObserveAlwaysTop(mew::message msg) {
    if (!m_form) {
      return E_FAIL;
    }
    msg["state"] = mew::ENABLED | (GetAlwaysTop() ? mew::CHECKED : 0);
    return S_OK;
  }
  mew::ref<mew::ui::IShellListView> FindFolder(mew::io::IEntry* entry) {
    if (!entry) {
      return mew::null;
    }
    for (mew::each<mew::ui::IShellListView> i = EnumFolders(mew::StatusNone); i.next();) {
      mew::ref<mew::io::IEntry> e;
      i->GetFolder(&e);
      if (entry->Equals(e, mew::io::IEntry::PATH)) {
        return i;
      }
    }
    return mew::null;
  }
  mew::ref<mew::ui::IShellListView> OpenEntry(mew::io::IEntry* entry, avesta::Navigation navi,
                                              avesta::Navigation* naviResult = nullptr) {
    ASSERT(entry);
    if (mew::string filepath = entry->Path) {
      if (PathMatchSpec(filepath.str(),
                        _T("*.ave"))) {  // *.ave ファイル用の特別処理
        mew::string error;
        if (m_tab && FAILED(DoOpen(filepath, error))) {
          ave::WarningBox(m_form, mew::string::format(_T("読み込みに失敗しました。\n\n$1"), error));
        }
        return mew::null;
      }
    }
    return OpenFolder(entry, navi, naviResult);
  }
  mew::ref<mew::ui::IShellListView> OpenPath(mew::string path, avesta::Navigation navi) {
    try {
      return OpenEntry(mew::ref<mew::io::IEntry>(__uuidof(mew::io::Entry), path), navi);
    } catch (mew::exceptions::Error& e) {
      theAvesta->Notify(avesta::NotifyError, mew::string::format(_T("error: '$1' を開けませんでした ($2)"), path, e.Message));
      return mew::null;
    }
  }
  mew::ref<mew::ui::IShellListView> OpenFolder(mew::io::IEntry* folder, avesta::Navigation navi,
                                               avesta::Navigation* naviResult = nullptr) {
    extern mew::ref<mew::ui::IShellListView> CreateFolderList(mew::ui::IList * parent, mew::io::IEntry * folder);
    try {
      if (!folder) {
        TRACE(L"error: OpenFolder(null)");
        return mew::null;
      }
      mew::ref<mew::io::IEntry> resolved;
      folder->GetLinked(&resolved);
      if (!resolved->Exists()) {
        TRACE(L"error: OpenFolder(not-existing)");
        return mew::null;
      }
      if (!resolved->IsFolder()) {
        mew::ref<mew::io::IEntry> parentEntry;
        if SUCCEEDED (resolved->GetParent(&parentEntry)) {
          mew::ref<mew::ui::IShellListView> view = OpenFolder(parentEntry, navi);
          if (view) {
            view->SetStatus(resolved, mew::SELECTED, true);
          }
          return view;
        }
        TRACE(L"error: OpenFolder(non-folder)");
        return mew::null;
      }
      if (useRedirectToMyDocuments) {
        resolved = RedirectToMyDocuments(resolved);
      }
      //
      mew::ref<mew::ui::IShellListView> current = CurrentView();
      bool locked = false;
      if (current) {
        mew::ref<mew::ui::IList> parent;
        if SUCCEEDED (QueryParent(current, &parent)) {
          DWORD status = 0;
          parent->GetStatus(current, &status);
          locked = (status & mew::CHECKED) != 0;
        }
      }
      avesta::Navigation naviModified = theAvesta->NavigateVerb(current, resolved, locked, navi);
      // check goto navigate
      switch (naviModified) {
        case avesta::NaviGoto:
        case avesta::NaviGotoAlways: {  // FIXME: m_callback を無効化することで、
          // NaviGotoを再評価することを避けている。美しくない！
          mew::ref<ICallback> reserve = m_callback;
          m_callback = mew::null;
          current->Go(resolved);
          m_callback = reserve;
          return current;
        }
        default:
          break;
      }
      // open navigate
      mew::ref<mew::ui::IShellListView> view;
      if (!m_booleans[avesta::BoolOpenDups]) {
        view = FindFolder(resolved);
      }
      if (view) {  // 既に開いていた
      } else {     // 新たに開く
        view = CreateFolderList(m_tab, folder);
        if (!view) {
          TRACE(L"OpenFolder failed.");
          return mew::null;
        }
        SetFont(view, m_fonts[avesta::FontAddress], 0);
        SetFont(view, m_fonts[avesta::FontList], 1);
        view->CheckBox = m_booleans[avesta::BoolCheckBox];
        view->FullRowSelect = m_booleans[avesta::BoolFullRowSelect];
        view->GridLine = m_booleans[avesta::BoolGridLine];
        view->RenameExtension = m_booleans[avesta::BoolRenameExtension];
        view->Connect(mew::ui::EventClose, mew::function(this, &Main::OnViewClose));
        view->Connect(mew::ui::EventFolderChange, mew::function(this, &Main::HandleViewChdir));
        if (m_status) {
          view->Connect(mew::ui::EventStatusText, mew::function(this, &Main::OnViewText));
        }
        view->SetExtension(__uuidof(mew::ui::IShellStorage), m_storage);
        view->SetExtension(__uuidof(IGesture), OID);
        view->SetExtension(__uuidof(IKeymap), OID);
        SetWallPaperToView(view, resolved);
        view->Go(resolved);
      }
      //
      switch (naviModified) {
        case avesta::NaviOpen:
        case avesta::NaviOpenAlways:
          m_tab->SetStatus(view, mew::SELECTED, true);
          break;
        case avesta::NaviAppend:
          m_tab->SetStatus(view, mew::SELECTED);
          break;
        case avesta::NaviReserve:
          break;
        case avesta::NaviSwitch:
          m_tab->SetStatus(view, mew::SELECTED);
          m_tab->SetStatus(view, mew::FOCUSED);
          break;
        case avesta::NaviReplace: {
          mew::ref<mew::ui::IWindow> current_;
          m_tab->GetContents(&current_, mew::FOCUSED);
          m_tab->SetStatus(view, mew::SELECTED);
          m_tab->SetStatus(view, mew::FOCUSED);
          if (current_) {
            m_tab->SetStatus(current_, mew::UNCHECKED);
            m_tab->SetStatus(current_, mew::UNSELECTED);
          }
          break;
        }
        default:
          break;
      }
      if (m_booleans[avesta::BoolOpenNotify] && view) {
        m_tab->SetStatus(view, mew::HOT);
      }
      if (naviResult) {
        *naviResult = naviModified;
      }
      //
      WindowVisibleTrue();
      m_tab->Focus();
      return view;
    } catch (mew::exceptions::Error& e) {
      ave::ErrorBox(m_form, e.Message);
      return mew::null;
    }
  }

  HRESULT OpenOrExecute(mew::io::IEntry* entry) {
    mew::ref<mew::io::IEntry> resolved;
    entry->GetLinked(&resolved);
    if (resolved->IsFolder() && !PathIsExe(resolved->Path.str()) &&
        OpenFolder(resolved, avesta::NaviOpen)) {  // OK. succeeded to open as folder
      return S_OK;
    } else {  // not a folder. execute as file.
      return avesta::ILExecute(entry->ID);
    }
  }

  mew::ref<mew::ui::IEditableTreeItem> CreateMRUTreeItem() {
    return mew::ref<CommandTreeItem>::from(new CommandTreeItem(m_mru));
  }
  mew::ref<mew::ui::IEditableTreeItem> CreateFolderTreeItem(mew::ui::Direction dir) {
    return mew::null;
    // switch(dir)
    //{
    // case DirNorth:
    //  return objnew<CommandTreeItem>(m_UpList);
    // case DirWest:
    //  return objnew<CommandTreeItem>(m_BackList);
    // case DirEast:
    //  return objnew<CommandTreeItem>(m_FwdList);
    // default:
    //  TRESPASS_DBG( return null );
    //}
  }

 private:
  void InitCommands();

  int FindFormByType(mew::REFINTF pp, LPCWSTR type) const {
    for (size_t i = 0; i < m_forms.size(); ++i) {
      if (m_forms[i].type == type && SUCCEEDED(m_forms[i].window.copyto(pp))) {
        return (int)i;
      }
    }
    return -1;
  }
  // string FindIdByWindow(IWindow* window) const
  //{
  //  if(!window)
  //    return null;
  //  for(size_t i = 0; i < m_forms.size(); ++i) {
  //    if(m_forms[i].window == window)
  //      return m_forms[i].id;
  //  }
  //  return null;
  //}

  void InitForms() {
    HRESULT hr;

    m_dropmode = BoolUnknown;

    mew::ref<mew::xml::IXMLReader> sax(__uuidof(mew::xml::XMLReader));

    if FAILED (hr = FormTemplate(mew::string(RelativePath(L"usr\\form.xml")), sax, m_forms)) {
      throw mew::exceptions::LogicError(mew::string::load(IDS_ERR_BADFORM), hr);
    }

    FormGenerate(m_forms);
    FindFormByType(&m_form, L"Form");
    FindFormByType(&m_tab, L"FolderList");
    FindFormByType(&m_tree, L"FolderTree");
    FindFormByType(&m_status, L"StatusBar");
    FindFormByType(&m_preview, L"Preview");

    if (!m_display) {
      throw mew::exceptions::LogicError(mew::string::load(IDS_ERR_FORMS_NO_DISPLAY), E_INVALIDARG);
    }
    if (!m_form) {
      throw mew::exceptions::LogicError(mew::string::load(IDS_ERR_FORMS_NO_FORM), E_INVALIDARG);
    }
    if (!m_tab) {
      throw mew::exceptions::LogicError(mew::string::load(IDS_ERR_FORMS_NO_TAB), E_INVALIDARG);
    }

    // display
    m_display->Connect(mew::ui::EventOtherFocus, mew::function(this, &Main::HandleOtherFocus));

    // form
    HandleEvent(m_form, mew::ui::EventDispose, m_display, mew::ui::CommandClose);
    HandleEvent(m_form, mew::ui::EventClose, &Main::OnFormClose);
    HandleEvent(m_form, mew::ui::EventData, &Main::OnFormData);
    HandleEvent(m_form, mew::ui::EventMouseWheel, &Main::OnFormWheel);
    TaskTray_InitComponents(m_form);

    // tabs
    m_tab->SetMinMaxTabWidth(GetProfileSint32(_T("Style"), _T("TabWidthMin"), 0),
                             GetProfileSint32(_T("Style"), _T("TabWidthMax"), INT_MAX));
    m_tab->Connect(mew::ui::EventItemFocus, mew::function(this, &Main::HandleTabFocus));

    if (m_tree) {  // tree
      HandleEvent(mew::cast<mew::ISignal>(m_tree->Root), mew::EventInvoke, &Main::OnOpenEntry);
      HandleEvent(m_tree, mew::ui::EventItemFocus, &Main::OnTreeItemFocus);
    }

    if (m_status) {  // status
      /// FIXME: 「名前」と「テキスト」を別に持ちたいので、2つ目のペインを使う。
      int w[] = {0, -1};
      SendMessage(m_status->Handle, SB_SIMPLE, FALSE, 0);
      SendMessage(m_status->Handle, SB_SETPARTS, 2, (LPARAM)w);
    }

    // コマンド初期化
    InitCommands();
    RegisterForms(m_forms, m_commands);

    ReloadResource(sax);
  }
  void Notify(DWORD priority, mew::string msg) {
    // sound
    if (priority >= avesta::NotifyError) {  // error
      MessageBeep(MB_ICONEXCLAMATION);
    } else if (priority >= avesta::NotifyWarning) {  // warning
      MessageBeep(0);
    }
    //
    if (msg) {
      SetStatusText(priority, msg);
    }
  }
  HRESULT SyncDescendants(mew::io::IEntry* pFolder) {
    if (!m_storage) {
      return E_UNEXPECTED;
    }
    return m_storage->SyncDescendants(pFolder);
  }
  avesta::Navigation NavigateVerb(mew::ui::IShellListView* folder, mew::io::IEntry* where, bool locked,
                                  avesta::Navigation defaultVerb) {
    if (!where) {
      return defaultVerb;
    }
    if (!m_callback) {
      return defaultVerb;
    }
    mew::ref<mew::io::IEntry> current;
    if (folder) {
      folder->GetFolder(&current);
    }
    return m_callback->NavigateVerb(current, where, mew::ui::GetCurrentModifiers(), locked, defaultVerb);
  }

 public:
  HRESULT AvestaExecute(mew::io::IEntry* entry) {
    ASSERT(entry);
    ASSERT(m_callback);
    if (!entry) {
      return E_INVALIDARG;
    }
    if (!m_callback) {
      return E_UNEXPECTED;
    }

    // TODO: current は string 型で十分だが、
    // pygmy.dll の互換性のため IEntry を使っている。
    // 次のメジャーバージョンアップで変更すること。
    mew::ref<mew::io::IEntry> current = CurrentFolder();

    if (mew::string path = entry->Path) {
      if (mew::str::equals_nocase(mew::io::PathFindLeaf(path), L"avesta.dll")) {
        switch (::MessageBox(m_form->Handle, mew::string::format(L"$1 に置き換えますか？", path).str(),
                             L"Avestaアップデート確認", MB_YESNOCANCEL | MB_ICONINFORMATION)) {
          case IDYES:
            Restart(path.str());
            return S_OK;
          case IDNO:
            break;
          case IDCANCEL:
            return S_OK;
        }
      }
    }

    mew::string verb = m_callback->ExecuteVerb(current, entry, mew::ui::GetCurrentModifiers());

    return avesta::ILExecute(entry->ID, verb.str(), nullptr, (current ? current->Path.str() : nullptr));
  }

 public:  // event handlers
  HRESULT ObserveMRU(mew::message m) {
    m["state"] = (m_mru.get_Count() > 0 ? mew::ENABLED : 0);
    return S_OK;
  }

  HRESULT ObserveShow(mew::message m) {
    if (mew::ref<mew::ui::IWindow> w = m["window"]) {
      m["state"] = (mew::ENABLED | (w->Visible ? mew::CHECKED : 0));
    }
    return S_OK;
  }
  void UpdateCaption(mew::ui::IShellListView* current) {
    if (!m_form || !m_callback) {
      return;
    }
    mew::ref<mew::io::IEntry> entry;
    mew::string name, path;
    if (current && SUCCEEDED(current->GetFolder(&entry)) && !!(name = entry->Name)) {
      path = entry->Path;
    }
    mew::string caption = m_callback->Caption(name, path);
    m_form->Name = caption;
    if (m_booleans[avesta::BoolTreeAutoReflect]) {
      ProcessTreeReflect();
    }
  }
  HRESULT HandleViewChdir(mew::message msg) {
    if (!m_form) {
      return E_FAIL;
    }
    mew::ref<mew::ui::IShellListView> from = msg["from"];
    if (objcmp(from, CurrentView())) {
      PlayNavigateSound();
      UpdateCaption(from);
    }
    return S_OK;
  }
  void PlayNavigateSound() {
    if (!m_form || !m_form->Visible) {
      return;
    }
    static bool init = false;
    if (!init) {
      init = true;
      TCHAR path[MAX_PATH];
      if (SUCCEEDED(afx::ExpGetNavigateSound(path)) && PathFileExists(path)) {
        m_NavigateSoundPath = path;
      }
    }
    if (m_NavigateSoundPath) {
      PlaySound(m_NavigateSoundPath.str(), NULL, SND_FILENAME | SND_ASYNC | SND_NOWAIT);
    }
  }
  HRESULT HandleTabFocus(mew::message msg) {
    if (!m_form) {
      return E_FAIL;
    }
    mew::ref<mew::ui::IShellListView> current = msg["what"];
    UpdateCaption(current);
    if (!current) {  // 最後のタブが閉じられたので、ステータスバーをクリアする.
      SetStatusText(avesta::NotifyInfo, mew::null);
      UpdatePreview(nullptr);
    }
    return S_OK;
  }

  void UpdatePreview(mew::ui::IShellListView* view) {
    if (!m_preview || !m_form) {
      return;
    }
    mew::ref<mew::io::IEntry> entry;
    if (view && view->SelectedCount > 0) {
      mew::ref<mew::io::IEntryList> entries;
      if (HWND hwnd = ::FindWindowEx(::FindWindowEx(view->Handle, NULL, _T("SHELLDLL_DefView"), NULL), NULL,
                                     _T("SysListView32"), NULL)) {
        int index = -1;
        if (mew::ui::IsKeyPressed(VK_LBUTTON)) {  // マウス左ボタン押下げ中＝ドラッグ選択中？
          LVHITTESTINFO hit;
          ::GetCursorPos(&hit.pt);
          ::ScreenToClient(hwnd, &hit.pt);
          index = ListView_HitTest(hwnd, &hit);
          if (index < 0 || ListView_GetItemState(hwnd, index, LVIS_SELECTED) !=
                               LVIS_SELECTED) {  // FIXME:
                                                 // この状態の場合、何をプレビュー候補に選択するかは難しい……
            TRACE(L"UpdatePreview.FIXME");
            return;
          }
        } else {
          index = ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED);
        }
        if (index >= 0) {
          view->GetAt(&entry, index);
        }
      }
      if (!entry) {
        if (SUCCEEDED(view->GetContents(&entries, mew::FOCUSED)) && entries->Count > 0) {
          entries->GetAt(&entry, 0);
        }
      }
    }
    m_preview->SetContents(entry);
  }

  HRESULT FileOpen(mew::message msg) {
    mew::ref<mew::ui::IShellListView> current;
    if (msg.code == 1 && !!(current = CurrentView())) {
      mew::ref<mew::io::IEntry> currentPath;
      current->GetFolder(&currentPath);
      if (mew::ref<mew::io::IEntry> folder = avesta::PathDialog(currentPath ? currentPath->Path : mew::null)) {
        current->Go(folder);
      }
    } else {
      if (mew::ref<mew::io::IEntry> folder = avesta::PathDialog()) {
        OpenFolder(folder, avesta::NaviOpen);
      }
    }
    return S_OK;
  }
  HRESULT FileMRU(mew::message msg = mew::null) {
    if (mew::ref<mew::io::IEntry> folder = m_mru.Pop()) {
      OpenFolder(folder, avesta::NaviOpen);
    } else {
      theAvesta->Notify(avesta::NotifyWarning, mew::string::load(IDS_NO_MRU));
    }
    return S_OK;
  }
  HRESULT OnFormData(mew::message msg) {
    if (mew::string data = msg["data"]) {
      ParseCommandLine(data.str());
    }
    return S_OK;
  }
  HRESULT OnFormWheel(mew::message msg) {
    if (!m_form || !m_tab) {
      return E_FAIL;
    }
    INT32 wheel = msg["wheel"];
    if (wheel < 0) {
      m_tab->Send(mew::ui::CommandGoDown);
    } else {
      m_tab->Send(mew::ui::CommandGoUp);
    }
    return S_OK;
  }
  HRESULT OnViewText(mew::message msg) {
    if (!m_tab) {
      return E_FAIL;
    }
    mew::ref<mew::ui::IShellListView> from = msg["from"];
    ASSERT(from);
    mew::string text = msg["text"];
    mew::ref<mew::ui::IShellListView> current = CurrentView();
    if (current && objcmp(current, from)) {
      SetStatusText(avesta::NotifyInfo, FormatViewStatusText(current, text));
      UpdatePreview(current);
    }
    return S_OK;
  }
  HRESULT OnViewClose(mew::message msg) {
    if (mew::ref<mew::ui::IShellListView> view = msg["from"]) {
      mew::ref<mew::io::IEntry> entry;
      if SUCCEEDED (view->GetFolder(&entry)) {
        m_mru.Push(entry);
      }
    }
    return S_OK;
  }

  HRESULT OnFormClose(mew::message msg) {
    if (m_form) {
      DropMode(false);
      m_form->Name = mew::string::load(theMainResult == 0 ? IDS_CLOSING : IDS_RESTARTING);
      ::RevokeDragDrop(m_form->Handle);
    }
    if (m_booleans[avesta::BoolRestoreCond]) {
      mew::string error;
      mew::string savename = GetDefaultSaveName();
      if (DoSave(savename, error) < 1) {
        ::DeleteFile(savename.str());
      }
    }
    m_callback.dispose();
    ForwardToAll(mew::ui::CommandSave);
    SaveStatus();
    return E_FAIL;  // detach
  }
  HRESULT OnOpenEntry(mew::message msg) {
    OpenOrExecute(msg["what"]);
    return S_OK;
  }
  HRESULT OnTreeItemFocus(mew::message msg) {
    ASSERT(m_tree);
    if (!m_booleans[avesta::BoolTreeAutoSync] || !m_tree->Visible) {
      return S_OK;
    }
    if (mew::ref<mew::io::IFolder> folder = msg["what"]) {
      if (mew::ref<mew::ui::IShellListView> current = CurrentView()) {
        afx::SetModifierState(0, 0);
        current->Go(folder->Entry);
        afx::RestoreModifierState(0);
      }
    }
    return S_OK;
  }
  static void RecursiveDelete(PCWSTR start, PCWSTR leaf) {
    TCHAR buf[MAX_PATH];
    PathCombine(buf, start, leaf);
    DeleteFile(buf);
    //
    WIN32_FIND_DATA find;
    PathCombine(buf, start, _T("*.*"));
    HANDLE hFind = ::FindFirstFile(buf, &find);
    if (hFind != INVALID_HANDLE_VALUE) {
      do {
        if ((find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
          continue;
        }
        if (lstrcmp(find.cFileName, _T(".")) == 0) {
          continue;
        }
        if (lstrcmp(find.cFileName, _T("..")) == 0) {
          continue;
        }
        TCHAR subdir[MAX_PATH];
        PathCombine(subdir, start, find.cFileName);
        RecursiveDelete(subdir, leaf);
      } while (::FindNextFile(hFind, &find));
      ::FindClose(hFind);
    }
  }
  HRESULT ProcessThumbSize(mew::message msg) {
    DWORD size = msg.code;
    if (size < 1 || 1024 < size) {
      return E_FAIL;
    }
    HRESULT hr = afx::ExpSetThumbnailSize(size);
    if (hr == S_OK) {
      if (mew::ref<mew::ui::IShellListView> view = CurrentView()) {
        mew::ref<mew::io::IEntry> entry;
        view->GetFolder(&entry);
        mew::string path = entry->Path;
        if (!!path) {
          TCHAR msg_[1024];
          wsprintf(msg_,
                   _T("キャッシュを更新するため、\"%s\" 以下のすべての ")
                   _T("Thumbs.db ファイルを消去します。\n\nよろしいですか？"),
                   path.str());
          if (ave::QuestionBox(m_form, msg_, MB_OKCANCEL) == IDOK) {
            CWaitCursor wait;
            RecursiveDelete(path.str(), _T("Thumbs.db"));
          }
        }
      }
    }
    InvokeCommand(_T("Current.Mode.Thumbnail"));
    return S_OK;
  }
  HRESULT ObserveThumbSize(mew::message msg) {
    DWORD size = msg.code;
    if (size < 1 || 1024 < size) {
      return E_FAIL;
    }
    msg["state"] = (mew::ENABLED | (size == afx::ExpGetThumbnailSize() ? mew::CHECKED : 0));
    return S_OK;
  }
  HRESULT ObserveGo(mew::message msg) {
    bool cango = false;
    if (mew::ref<mew::ui::IShellListView> current = CurrentView()) {
      cango = (current->Go((mew::ui::Direction)(int)msg.code, 0) > 0);
    }
    msg["state"] = (cango ? mew::ENABLED : 0);
    return S_OK;
  }
  HRESULT ProcessArrange(mew::message msg) {
    if (!m_tab) {
      return E_FAIL;
    }
    m_tab->Arrange = (mew::ui::ArrangeType)(int)msg.code;
    return S_OK;
  }
  HRESULT ObserveArrange(mew::message msg) {
    if (!m_tab) {
      return E_FAIL;
    }
    msg["state"] = (mew::ENABLED | ((m_tab->Arrange == (mew::ui::ArrangeType)(int)msg.code) ? mew::CHECKED : 0));
    return S_OK;
  }
  HRESULT ProcessKeybind(mew::message msg) {
    m_EditOptions = ((m_EditOptions & ~afx::KeybindMask) | msg.code);
    return S_OK;
  }
  HRESULT ObserveKeybind(mew::message msg) {
    msg["state"] = (mew::ENABLED | (((m_EditOptions & afx::KeybindMask) == msg.code) ? mew::CHECKED : 0));
    return S_OK;
  }
  HRESULT ProcessMiddleClick(mew::message msg) {
    m_MiddleClick = msg.code;
    return S_OK;
  }
  HRESULT ObserveMiddleClick(mew::message msg) {
    msg["state"] = (mew::ENABLED | ((m_MiddleClick == msg.code) ? mew::CHECKED : 0));
    return S_OK;
  }
  HRESULT ProcessAutoCopy(mew::message msg) {
    if (m_form) {
      m_form->AutoCopy = (mew::ui::CopyMode)(int)msg.code;
    }
    return S_OK;
  }
  HRESULT ObserveAutoCopy(mew::message msg) {
    msg["state"] = (mew::ENABLED | ((m_form->AutoCopy == (mew::ui::CopyMode)(int)msg.code) ? mew::CHECKED : 0));
    return S_OK;
  }
  HRESULT ProcessTreeRefresh(mew::message = mew::null) {
    if (m_tree) {
      m_tree->Update();
    }
    return S_OK;
  }
  bool CanTreeSync(mew::io::IFolder** ppFolder = nullptr) {
    if (!m_tree || !::IsWindowVisible(m_tree->Handle) || GetComponentCount(avesta::AvestaFolder) == 0) {
      return false;
    }
    mew::ref<mew::io::IFolder> folder;
    if FAILED (m_tree->GetContents(&folder, mew::FOCUSED)) {
      return false;
    }
    if (ppFolder) {
      folder->QueryInterface(ppFolder);
    }
    return true;
  }
  HRESULT ProcessTreeSync(mew::message = mew::null) {
    mew::ref<mew::io::IFolder> folder;
    if (CanTreeSync(&folder)) {
      afx::SetModifierState(0, 0);
      CurrentView()->Go(folder->Entry);
      afx::RestoreModifierState(0);
    }
    return S_OK;
  }
  HRESULT ObserveTreeSync(mew::message msg) {
    msg["state"] = (CanTreeSync() ? mew::ENABLED : 0);
    return S_OK;
  }
  bool CanTreeReflect(mew::io::IEntry** ppEntry = nullptr) {
    if (!m_tree || !::IsWindowVisible(m_tree->Handle) || GetComponentCount(avesta::AvestaFolder) == 0) {
      return false;
    }
    if (ppEntry) {
      return SUCCEEDED(CurrentView()->GetFolder(ppEntry));
    }
    return true;
  }
  mew::ref<mew::io::IEntry> RedirectToMyDocuments(mew::io::IEntry* entry) {
    ASSERT(entry);
    if (!entry) {
      return mew::null;
    }
    mew::string path = entry->Path;
    if (!path) {
      return entry;  // virtual-folder
    }
    mew::io::IEntry* entryMyDocuments = GetMyDocuments();
    mew::string pathMyDocuments = entryMyDocuments->Path;
    PCTSTR szPath = path.str(), szMyDocuments = pathMyDocuments.str();
    size_t lengthMyDocuments = pathMyDocuments.length();
    if (mew::str::compare(szPath, szMyDocuments, lengthMyDocuments) != 0) {
      return entry;  // not My Documents path
    }
    PCTSTR relative = szPath + lengthMyDocuments;
    mew::ref<mew::io::IEntry> newEntry;
    if FAILED (entryMyDocuments->ParseDisplayName(&newEntry, relative)) {
      return entry;
    }
    TRACE(_T("info: RedirectToMyDocuments($1)"), entry->Path);
    return newEntry;
  }
  HRESULT ProcessTreeReflect(mew::message = mew::null) {
    mew::ref<mew::io::IEntry> entry;
    if (CanTreeReflect(&entry)) {
      m_tree->SetStatus(useRedirectToMyDocuments ? RedirectToMyDocuments(entry) : entry, mew::FOCUSED);
    }
    return S_OK;
  }
  HRESULT ObserveTreeReflect(mew::message msg) {
    msg["state"] = (CanTreeReflect() ? mew::ENABLED : 0);
    return S_OK;
  }
  struct Tab {
    int index;
    mew::ref<IUnknown> obj;
    TCHAR path[MAX_PATH];

    friend bool operator<(const Tab& lhs, const Tab& rhs) {
      int i = mew::str::compare_nocase(lhs.path, rhs.path);
      if (i != 0) {
        return i < 0;
      } else {
        return lhs.index < rhs.index;  // stable-sort
      }
    }
  };
  HRESULT ProcessTabSort(mew::message = mew::null) {
    if (!m_tab) {
      return false;
    }

    std::vector<Tab> tabs;
    //
    for (mew::each<mew::ui::IShellListView> i = EnumFolders(mew::StatusNone); i.next();) {
      Tab tab;
      tab.index = tabs.size();
      tab.obj = i;
      if (mew::ref<mew::io::IEntry> entry = ave::GetFolderOfView(i)) {
        entry->Path.copyto(tab.path, MAX_PATH);
        for (PTSTR c = tab.path; *c; c = mew::str::inc(c)) {
          if (*c == _T('\\')) {
            *c = (TCHAR)1;  // NULL文字でない、小さな数字に置換する
          }
        }
      }
      tabs.push_back(tab);
    }
    //
    std::sort(tabs.begin(), tabs.end());
    // いったん隠しておくと、いちいちレイアウトが行われないため、高速化する。
    SetWindowPos(m_tab->Handle, nullptr, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_HIDEWINDOW);
    // せいぜい一桁なので、ヘタレなソート方法でごめんなさい
    bool done = false;
    while (!done) {
      done = true;
      for (size_t i = 0; i < tabs.size(); ++i) {
        HRESULT hr = m_tab->MoveTab(tabs[i].obj, i);
        if FAILED (hr) {
          break;
        }
        if (hr == S_OK) {
          done = false;
        }
      }
    }
    SetWindowPos(m_tab->Handle, nullptr, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    m_tab->Update();
    return S_OK;
  }
  HRESULT ProcessTabMove(mew::message msg) {
    if (!m_tab) {
      return false;
    }
    int from = CurrentFolderIndex();
    if (from < 0) {
      theAvesta->Notify(avesta::NotifyWarning, mew::string::load(IDS_WARN_NOTAB));
      return S_OK;
    }
    int to = from + (int)msg.code;
    if (to < 0) {
      to = GetComponentCount(avesta::AvestaFolder) - 1;
    } else if (to >= (int)GetComponentCount(avesta::AvestaFolder)) {
      to = 0;
    }
    if (m_tab->MoveTab(from, to) != S_OK) {
      theAvesta->Notify(avesta::NotifyError, mew::string::load(IDS_ERR_TABMOVE));
    }
    return S_OK;
  }
  HRESULT WindowVisibleTrue(mew::message = mew::null) {
    if (!m_form) {
      return false;
    }
    HWND hWnd = m_form->Handle;
    if (!IsWindowEnabled(hWnd)) {
      return S_OK;
    }
    if (IsIconic(hWnd)) {
      m_form->Send(mew::ui::CommandRestore);
    } else {
      SetForegroundWindow(hWnd);
    }
    return S_OK;
  }
  HRESULT WindowVisibleToggle(mew::message = mew::null) {
    if (!m_form) {
      return false;
    }
    HWND hWnd = m_form->Handle;
    if (!IsWindowEnabled(hWnd)) {
      return S_OK;
    }
    if (IsIconic(hWnd)) {
      m_form->Send(mew::ui::CommandRestore);
    } else if (GetForegroundWindow() == hWnd) {
      m_form->Send(mew::ui::CommandMinimize);
    } else {
      SetForegroundWindow(hWnd);
    }
    return S_OK;
  }

 private:  // Commands
  HRESULT ExplorerImport(mew::message msg) {
    ImportExplorer(msg.code != 0);
    return S_OK;
  }
  HRESULT ProcessRecycleBin(mew::message = mew::null) {
    SHEmptyRecycleBin(m_form->Handle, nullptr, 0);
    return S_OK;
  }
  HRESULT ObserveRecycleBin(mew::message msg) {
    SHQUERYRBINFO info = {sizeof(SHQUERYRBINFO)};
    if SUCCEEDED (SHQueryRecycleBin(nullptr, &info)) {
      bool isEmpty = (info.i64NumItems == 0);
      msg["state"] = (isEmpty ? 0 : mew::ENABLED);
      if (mew::ref<mew::ui::IEditableTreeItem> menu = msg["owner"]) {
        if (mew::string name = menu->Name) {
          TCHAR buffer[MAX_PATH];
          name.copyto(buffer, MAX_PATH);
          PTSTR tab = mew::str::find(buffer, _T('\t'));
          if (!tab) {
            const TCHAR magic[] = _T("{*}");
            const size_t length_of_magic = lengthof(magic) - 1;
            size_t len = mew::str::length(buffer);
            PTSTR appendpos = buffer + len - length_of_magic;
            if (len >= length_of_magic && mew::str::equals(appendpos, magic)) {
              tab = appendpos;
            }
          }
          if (tab) {
            TCHAR postfix[36] = _T("\t(");
            if (isEmpty) {
              lstrcat(postfix, _T("empty)"));
            } else {
              StrFormatByteSize64(info.i64Size, postfix + 2, 31);
              lstrcat(postfix, _T(")"));
            }
            mew::str::copy(tab, postfix);
            menu->Name = buffer;
          }
        }
      }
    }
    return S_OK;
  }
  HRESULT ProcessFolderOptionsShow(mew::message = mew::null) {
    // プロトタイプがいまいちよく分からないので、ShellExecute経由で呼ぶ
    ShellExecute(m_form->Handle, nullptr, _T("rundll32.exe"), _T("shell32.dll,Options_RunDLL 0"), nullptr, SW_SHOW);
    // このままだと、このアプリケーションよりも背面に表示してしまうので……
    HWND hFolderOptions = nullptr;
    for (int i = 0; i < 10 && !hFolderOptions; ++i) {
      Sleep(100);
      hFolderOptions = ::FindWindow(_T("#32770"), _T("フォルダ オプション"));
    }
    if (hFolderOptions) {
      ::BringWindowToTop(hFolderOptions);
      ::SetActiveWindow(hFolderOptions);
    }
    return S_OK;
  }

  HRESULT ProcessSyncFileDialog(mew::message = mew::null) {
    if (mew::string path = CurrentPath()) {
      if FAILED (avesta::FileDialogSetPath(path.str())) {
        Notify(avesta::NotifyWarning, mew::string::load(IDS_ERR_NOFILEDIALOG));
      }
    }
    return S_OK;
  }

  ///
  struct EntryAndStatus {
    mew::ref<mew::ui::IShellListView> view;
    mew::ref<mew::io::IEntry> entry;
    DWORD status;
    mew::string mask;
  };
  class OptionOnOpen {
   private:
    const bool m_TreeAutoReflect;

   public:
    OptionOnOpen() : m_TreeAutoReflect(theAvesta->TreeAutoReflect) { theAvesta->TreeAutoReflect = false; }
    ~OptionOnOpen() { theAvesta->TreeAutoReflect = m_TreeAutoReflect; }
  };
  ///
  class OptionOnMultiOpen : public OptionOnOpen {
   private:
    mew::ref<mew::ui::ITabPanel> m_tab;
    mew::ui::InsertTo m_ins;
    const bool m_OpenDups;

   public:
    OptionOnMultiOpen(mew::ui::ITabPanel* tab) : m_OpenDups(theAvesta->OpenDups) {
      theAvesta->OpenDups = true;
      m_tab = tab;
      m_ins = tab->InsertPosition;
      tab->InsertPosition = mew::ui::InsertTail;
    }
    ~OptionOnMultiOpen() {
      m_tab->InsertPosition = m_ins;
      theAvesta->OpenDups = m_OpenDups;
    }
  };
  ///
  int OpenMultipleFolders(std::vector<EntryAndStatus>& entries, avesta::Navigation navi) {
    int count = 0;
    if (true) {
      OptionOnMultiOpen ins(m_tab);
      for (std::vector<EntryAndStatus>::iterator i = entries.begin(); i != entries.end(); ++i) {
        if (i->view = OpenFolder(i->entry, navi)) {
          navi = avesta::NaviAppend;
          ++count;
        }
      }
    }
    if (m_booleans[avesta::BoolTreeAutoReflect]) {
      ProcessTreeReflect();
    }
    return count;
  }
  ///
  int OpenMultipleEntries(mew::io::IEntryList* entries, avesta::Navigation navi) {
    if (!entries) {
      return 0;
    }
    int count = 0;
    if (const int leafs = entries->Count) {
      OptionOnMultiOpen ins(m_tab);
      for (int i = 0; i < leafs; ++i) {
        mew::ref<mew::io::IEntry> entry;
        if FAILED (entries->GetAt(&entry, i)) {
          continue;
        }
        avesta::Navigation naviResult;
        if (mew::ref<mew::ui::IWindow> view = OpenEntry(entry, navi, &naviResult)) {
          switch (naviResult) {
            case avesta::NaviOpen:
            case avesta::NaviOpenAlways:
            case avesta::NaviReplace:
              navi = avesta::NaviAppend;
              break;
            default:
              navi = naviResult;
              break;
          }
          ++count;
        }
      }
    }
    if (m_booleans[avesta::BoolTreeAutoReflect]) {
      ProcessTreeReflect();
    }
    return count;
  }

  bool AskCloseAll() {
    mew::ref<mew::ui::IWindow> current = CurrentView();
    if (!current) {
      return true;
    }
    // 現在少なくともひとつ以上のフォルダが開かれている
    switch (ave::QuestionBox(m_form,
                             L"フォルダリストを開きます。\n\n現在 "
                             L"開かれているフォルダをすべて閉じますか？",
                             MB_YESNOCANCEL)) {
      case IDYES:
        ForwardToAll(mew::ui::CommandClose);
        m_display->Update();
        break;
      case IDNO:
        break;
      case IDCANCEL:
        return false;
      default:
        TRESPASS();
    }
    return true;
  }

  HRESULT DoOpenVersion(mew::Stream stream, mew::string& error, INT version) {
    size_t size;
    stream >> size;
    using Entries = std::vector<EntryAndStatus>;
    Entries entries;
    entries.reserve(size);
    for (size_t i = 0; i < size; ++i) {
      EntryAndStatus item;
      stream >> item.entry;
      stream >> item.status;
      if (version >= 2) {
        stream >> item.mask;
      }
      entries.push_back(item);
    }
    //
    if (entries.empty() || !AskCloseAll()) {
      return 0;
    }
    //
    int count = 0;
    if (true) {
      OptionOnOpen sup;
      count = OpenMultipleFolders(entries, avesta::NaviOpen);
      for (Entries::iterator i = entries.begin(); i != entries.end(); ++i) {
        if (!(i->status & mew::SELECTED)) {
          m_tab->SetStatus(i->view, mew::UNSELECTED);
        }
        if (i->status & mew::CHECKED) {
          m_tab->SetStatus(i->view, mew::CHECKED);
        }
        if (version >= 2 && i->mask) {
          i->view->PatternMask = i->mask;
        }
      }
    }
    if (m_booleans[avesta::BoolTreeAutoReflect]) {
      ProcessTreeReflect();
    }
    return count;
  }
  /// @return 開いたフォルダの個数. エラーの場合は負数.
  HRESULT DoOpen(mew::string filename, mew::string& error) {
    try {
      mew::Stream stream(__uuidof(mew::io::FileReader), filename);
      GUID version;
      stream >> version;
      if (version == STATUS_VERSION_1) {
        return DoOpenVersion(stream, error, 1);
      } else if (version == STATUS_VERSION_2) {
        return DoOpenVersion(stream, error, 2);
      } else {
        error = _T("作業状況ファイルの形式が不正です");
        return E_FAIL;
      }
    } catch (mew::exceptions::Error& e) {
      error = e.Message.str();
      return e.Code;
    }
  }
  /// @return 保存したフォルダの個数. エラーの場合は負数.
  HRESULT DoSave(mew::string filename, mew::string& error) {
    using Entries = std::vector<EntryAndStatus>;
    Entries entries;
    entries.reserve(10);
    for (mew::each<mew::ui::IShellListView> i = EnumFolders(mew::StatusNone); i.next();) {
      EntryAndStatus item;
      item.view = i;
      item.mask = i->PatternMask;
      if SUCCEEDED (i->GetFolder(&item.entry)) {
        m_tab->GetStatus(item.view, &item.status);
        entries.push_back(item);
      }
    }
    if (entries.empty()) {
      error = mew::string::load(IDS_ERR_NOFOLDER);
      return E_ABORT;
    }
    if (!filename) {
      filename = mew::ui::SaveDialog(m_form->Handle, mew::string::load(IDS_FILEFILTER), _T(".ave"));
      if (!filename) {
        return 0;
      }
    }
    try {
      mew::Stream stream(__uuidof(mew::io::FileWriter), filename);
      stream << STATUS_VERSION_2;
      stream << (size_t)entries.size();
      for (Entries::iterator i = entries.begin(); i != entries.end(); ++i) {
        stream << i->entry;
        stream << i->status;
        stream << i->mask;
      }
      return entries.size();
    } catch (mew::exceptions::Error& e) {
      error = e.Message.str();
      return e.Code;
    }
  }
  HRESULT FileLoad(mew::message) {
    if (mew::string filename = mew::ui::OpenDialog(m_form->Handle, mew::string::load(IDS_FILEFILTER))) {
      mew::string error;
      if FAILED (DoOpen(filename, error)) {
        ave::WarningBox(m_form, mew::string::format(_T("読み込みに失敗しました。\n\n$1"), error));
      }
    }
    return S_OK;
  }
  HRESULT FileSave(mew::message) {
    mew::string error;
    if FAILED (DoSave(mew::null, error)) {
      ave::WarningBox(m_form, mew::string::format(_T("保存に失敗しました。\n\n$1"), error));
    }
    return S_OK;
  }
  HRESULT OptionReload(mew::message = mew::null) {
    ReloadResource();
    return S_OK;
  }
  void ReloadResource(mew::xml::IXMLReader* sax = nullptr) {
    ResetGlobalVariables();
    mew::ref<mew::xml::IXMLReader> sax0;
    if (!sax) {
      sax0.create(__uuidof(mew::xml::XMLReader));
      sax = sax0;
    }
    //
    FormReload(m_forms, m_commands, sax);
    SpoilKeymapAndGesture(m_tab);
  }
  HRESULT OptionWallPaper(mew::message) {
    mew::ref<mew::ui::IWallPaperDialog> dlg(__uuidof(mew::ui::WallPaperDialog), m_tab);
    dlg->AddTarget(this, mew::string::load(IDS_WALLPAPER_FOLDER));
    for (Templates::const_iterator i = m_forms.begin(); i != m_forms.end(); ++i) {
      if (mew::ref<IWallPaper> wallpaper = cast(i->window)) {
        dlg->AddTarget(wallpaper, i->id);
      }
    }
    dlg->Go();
    return S_OK;
  }
  HRESULT OptionInsert(mew::message msg) {
    m_tab->InsertPosition = (mew::ui::InsertTo)(int)msg.code;
    return S_OK;
  }

#define MEW_MSG_BEGIN(name)        \
  HRESULT name(mew::message msg) { \
    switch (msg.code) {
#define MEW_MSG_END() \
  default:            \
    return E_FAIL;    \
    }                 \
    return S_OK;      \
    }
#define MEW_COMMAND_STATE(code, enabled, checked) \
  case code: {                                    \
    UINT32 state = 0;                             \
    if (enabled) {                                \
      state |= mew::ENABLED;                      \
    }                                             \
    if (checked) {                                \
      state |= mew::CHECKED;                      \
    }                                             \
    msg["state"] = state;                         \
    break;                                        \
  }

  MEW_MSG_BEGIN(ObserveInsert)
  MEW_COMMAND_STATE(mew::ui::InsertHead, true, m_tab->InsertPosition == mew::ui::InsertHead);
  MEW_COMMAND_STATE(mew::ui::InsertTail, true, m_tab->InsertPosition == mew::ui::InsertTail);
  MEW_COMMAND_STATE(mew::ui::InsertPrev, true, m_tab->InsertPosition == mew::ui::InsertPrev);
  MEW_COMMAND_STATE(mew::ui::InsertNext, true, m_tab->InsertPosition == mew::ui::InsertNext);
  MEW_MSG_END()

  HRESULT ObserveMode(mew::message msg) {
    UINT32 state = 0;
    if (mew::ref<mew::ui::IShellListView> current = CurrentView()) {
      state = mew::ENABLED | (current->Style == (int)msg.code ? mew::CHECKED : 0);
    }
    msg["state"] = state;
    return S_OK;
  }
  HRESULT ObserveClipboard(mew::message msg) {
    UINT32 state = 0;
    if (CurrentView()) {
      state = (::IsClipboardFormatAvailable(msg.code) ? mew::ENABLED : 0);
    }
    msg["state"] = state;
    return S_OK;
  }
  HRESULT ObserveClipToSelect(mew::message msg) {
    UINT32 state = 0;
    if (mew::ui::IShellListView* current = CurrentView()) {
      state = ((current->SelectedCount > 0 && ::IsClipboardFormatAvailable(msg.code)) ? mew::ENABLED : 0);
    }
    msg["state"] = state;
    return S_OK;
  }
  HRESULT SystemAbout(mew::message = mew::null) {
    WCHAR path[MAX_PATH];
    ::GetModuleFileName(module::Handle, path, MAX_PATH);
    mew::ui::AboutDialog(m_form->Handle, path);
    return S_OK;
  }
  void Restart(PCWSTR newAvesta = nullptr) {
    if (m_form) {
      try {
        if (theNewAvesta && newAvesta) {
          int len = lstrlen(newAvesta);
          *theNewAvesta = (PWSTR)::GlobalAlloc(GMEM_FIXED, sizeof(WCHAR) * (len + 1));
          lstrcpy(*theNewAvesta, newAvesta);
        }
      } catch (mew::exceptions::Error&) {
      }
      theMainResult = 1;
      m_form->Close();
    }
  }
  HRESULT SystemRestart(mew::message = mew::null) {
    Restart();
    return S_OK;
  }

 private:
  void OpenFromCommandLine(PCTSTR value, avesta::Navigation navigate, bool link) {
    if (link) {
      TCHAR resolved[MAX_PATH];
      if FAILED (afx::SHResolveLink(value, resolved)) {
        theAvesta->Notify(avesta::NotifyError, mew::string::load(IDS_ERR_NOTLINK, value));
      } else {
        PTSTR leaf = PathFindFileName(resolved);
        if (leaf > resolved) {
          *(leaf - 1) = _T('\0');
        }
        if (mew::ref<mew::ui::IShellListView> view = OpenPath(resolved, navigate)) {
          view->SetStatus(mew::string(leaf), mew::SELECTED, true);
        }
      }
    } else {
      OpenPath(value, navigate);
    }
  }

 public:
  void ParseCommandLine(PCTSTR args) {
    mew::ref<avesta::ICommandLine> cmd = avesta::ParseCommandLine(args);
    if (!cmd) {
      return;
    }

    bool optLink = false;
    avesta::Navigation optNavigate = avesta::NaviOpen;

    // 短時間の間に開かれた場合は、一度に開かれたとみなす
    DWORD now = GetTickCount();
    if (now - m_ParseCommandLineTime < theAvesta->GetCommandLineInterval()) {
      optNavigate = avesta::NaviAppend;
    }
    m_ParseCommandLineTime = now;

    PTSTR option, value;
    while (cmd->Next(&option, &value)) {
      if (mew::str::equals_nocase(option, _T("command"))) {  // -command
        if (!value && !cmd->Next(&option, &value)) {
          break;
        }
        if FAILED (InvokeCommand(value)) {
          theAvesta->Notify(avesta::NotifyWarning, mew::string::load(IDS_ERR_COMMAND_NOT_FOUND, value));
        }
      } else if (mew::str::equals_nocase(option, _T("link"))) {  // -link
        if (value) {                                             // link は一度で効果消滅
          OpenFromCommandLine(value, optNavigate, true);
          optLink = false;
        } else {  // 次のOpenコマンドまで効果が残る
          optLink = true;
        }
      } else {  // default
        optNavigate = ParseNavigate(option, optNavigate);
        if (value) {  // link は一度で効果消滅
          OpenFromCommandLine(value, optNavigate, optLink);
          optLink = false;
        }
      }
    }
  }

 public:  // IDropTarget
  STDMETHODIMP DragEnter(IDataObject* pDataObject, DWORD key, POINTL pt, DWORD* pdwEffect) {
    *pdwEffect = DROPEFFECT_LINK;
    return S_OK;
  }
  STDMETHODIMP DragOver(DWORD key, POINTL pt, DWORD* pdwEffect) {
    *pdwEffect = DROPEFFECT_LINK;
    return S_OK;
  }
  STDMETHODIMP DragLeave() { return S_OK; }
  STDMETHODIMP Drop(IDataObject* pDataObject, DWORD key, POINTL pt, DWORD* pdwEffect) {
    try {
      mew::ref<mew::io::IEntryList> entries(__uuidof(mew::io::EntryList), pDataObject);
      size_t count = entries->Count;
      if (count == 0) {
        return S_OK;
      }
      if (key & mew::ui::MouseButtonRight) {
        CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenu(MF_STRING, avesta::NaviOpen, _T("新規に開く(&N)"));
        menu.AppendMenu(MF_STRING, avesta::NaviAppend, _T("追加で開く(&A)"));
        menu.AppendMenu(MF_STRING, avesta::NaviReserve, _T("非表示で開く(&H)"));
        menu.AppendMenu(MF_SEPARATOR);
        menu.AppendMenu(MF_STRING, static_cast<UINT_PTR>(0), _T("キャンセル"));
        menu.SetMenuDefaultItem(0, MF_BYPOSITION);
        UINT cmd = menu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, m_form->Handle, nullptr);
        if (cmd != 0) {
          OpenMultipleEntries(entries, (avesta::Navigation)cmd);
        }
        return S_OK;
      } else {  // Left Drag
        OpenMultipleEntries(entries, avesta::NaviOpen);
      }
    } catch (mew::exceptions::Error& e) {
      return e.Code;
    }
    return S_OK;
  }

 public:  // IWallPaper
  mew::string get_WallPaperFile() { return m_WallPaper; }
  void set_WallPaperFile(mew::string value) {
    m_WallPaper = ave::ResolvePath(value);
    for (mew::each<mew::ui::IShellListView> i = EnumFolders(mew::StatusNone); i.next();) {
      SetWallPaperToView(i, ave::GetFolderOfView(i));
    }
  }
  UINT32 get_WallPaperAlign() { return mew::ui::DirMaskNS | mew::ui::DirMaskWE; }
  void set_WallPaperAlign(UINT32 value) {}
  void SetWallPaperToView(mew::ui::IShellListView* view, mew::io::IEntry* entry) {
    ASSERT(view);
    mew::string filename = m_WallPaper;
    if (m_callback && entry) {
      filename = ave::ResolvePath(m_callback->WallPaper(m_WallPaper, entry->Name, entry->Path));
    }
    mew::cast<IWallPaper>(view)->WallPaperFile = filename;
  }

 private:
  mew::string m_WallPaper;

  HRESULT OptionFont_Tab(mew::message m) {
    HFONT font = FontFromMessage(avesta::FontTab, m);
    SetFont(m_tab, font);
    return S_OK;
  }
  HRESULT OptionFont_Address(mew::message m) {
    HFONT font = FontFromMessage(avesta::FontAddress, m);
    for (mew::each<mew::ui::IWindow> i = EnumFolders(mew::StatusNone); i.next();) SetFont(i, font, 0);
    return S_OK;
  }
  HRESULT OptionFont_List(mew::message m) {
    HFONT font = FontFromMessage(avesta::FontList, m);
    for (mew::each<mew::ui::IWindow> i = EnumFolders(mew::StatusNone); i.next();) SetFont(i, font, 1);
    return S_OK;
  }
  HRESULT OptionFont_Status(mew::message m) {
    HFONT font = FontFromMessage(avesta::FontStatus, m);
    SetFont(m_status, font);
    return S_OK;
  }
  HRESULT OptionFont(mew::message) {
    if (!m_form) {
      return E_FAIL;
    }
    if (!m_form->Visible || m_tab->Count <= 0) {
      ave::WarningBox(m_form, L"処理の都合のため、一つ以上のフォルダを開いた状態で行ってください");
      return S_OK;
    }

    mew::ref<mew::ui::IExpose> expose(__uuidof(mew::ui::Expose));
    expose->SetTitle(_T("フォント設定箇所を選んでください"));
    HWND hwndRoot = ::GetAncestor(m_form->Handle, GA_ROOT);

    enum {
      ExposeTab,
      ExposeAddress,
      ExposeList,
      ExposeStatus,
    };

    if (m_tab) {
      mew::Rect rc;
      ::GetWindowRect(m_tab->Handle, &rc);
      mew::Rect tabrc = m_tab->GetTabRect(0);
      rc.bottom = rc.top + tabrc.h;
      afx::ScreenToClient(hwndRoot, &rc);
      expose->AddRect(ExposeTab, 0, rc, 'T');
    }
    for (mew::each<mew::ui::IShellListView> i = EnumFolders(mew::StatusNone); i.next();) {
      if (i->Visible) {  // TODO: もうちょいどうにか
        if (HWND hComboBox = ::FindWindowEx(i->Handle, NULL, _T("ComboBoxEx32"), NULL)) {
          mew::Rect rc;
          ::GetWindowRect(hComboBox, &rc);
          afx::ScreenToClient(hwndRoot, &rc);
          expose->AddRect(ExposeAddress, 0, rc, 'A');
        }
        if (HWND hDefView = ::FindWindowEx(i->Handle, NULL, _T("SHELLDLL_DefView"), NULL)) {
          mew::Rect rc;
          ::GetWindowRect(hDefView, &rc);
          afx::ScreenToClient(hwndRoot, &rc);
          expose->AddRect(ExposeList, 0, rc, 'L');
        }
      }
    }
    if (m_status) {
      mew::Rect rc;
      ::GetWindowRect(m_status->Handle, &rc);
      afx::ScreenToClient(hwndRoot, &rc);
      expose->AddRect(ExposeStatus, 0, rc, 'S');
    }
    expose->Select(0);

    HRESULT hr = expose->Go(hwndRoot, theAvesta->GetExposeTime());
    switch (hr) {
      case ExposeTab:
        mew::ui::FontDialog(m_form->Handle, m_fonts[avesta::FontTab], L"タブのフォント設定",
                            mew::function(this, &Main::OptionFont_Tab));
        break;
      case ExposeAddress:
        mew::ui::FontDialog(m_form->Handle, m_fonts[avesta::FontAddress], L"アドレスバーのフォント設定",
                            mew::function(this, &Main::OptionFont_Address));
        break;
      case ExposeList:
        mew::ui::FontDialog(m_form->Handle, m_fonts[avesta::FontList], L"ファイルリストのフォント設定",
                            mew::function(this, &Main::OptionFont_List));
        break;
      case ExposeStatus:
        mew::ui::FontDialog(m_form->Handle, m_fonts[avesta::FontStatus], L"ステータスバーのフォント設定",
                            mew::function(this, &Main::OptionFont_Status));
        break;
      default:
        break;
    }
    return S_OK;
  }

  //
  // Form.DropMode
  //
 private:
  ThreeStateBool m_dropmode;  //
  mew::Rect m_dropRestore;    // ドロップモード復元後
  mew::Point m_dropLocation;  // ドロップモードの位置
 public:
  void DropMode(bool mode) {
    if (!m_form || !m_tab) {
      return;
    }
    if (mode) {
      if (m_dropmode != BoolUnknown) {
        return;
      }

      CWindow form(m_form->Handle);
      if (form.IsIconic()) {
        form.ShowWindow(SW_RESTORE);
      }

      m_dropRestore = m_form->Bounds;
      FormComponentsHide(m_forms);
      m_dropmode = (GetAlwaysTop() ? BoolTrue : BoolFalse);
      form.ModifyStyleEx(0, WS_EX_TOOLWINDOW | WS_EX_APPWINDOW);
      // resize
      mew::Size sz = m_tab->DefaultSize;
      sz.w = mew::math::max(sz.w, 120);
      m_form->ClientSize = sz;
      m_form->Location = m_dropLocation;
      SetAlwaysTop(true);
    } else {
      if (m_dropmode == BoolUnknown) {
        return;
      }

      CWindow form(m_form->Handle);
      if (form.IsIconic()) {
        form.ShowWindow(SW_RESTORE);
      }

      m_dropLocation = m_form->Location;
      FormComponentsRestore(m_forms);
      form.ModifyStyleEx(WS_EX_TOOLWINDOW, 0);
      m_form->Bounds = m_dropRestore;
      SetAlwaysTop(m_dropmode == BoolTrue);
      m_dropmode = BoolUnknown;
    }
  }
  HRESULT ProcessDropMode(mew::message) {
    switch (m_dropmode) {
      case BoolTrue:
      case BoolFalse:
        DropMode(false);
        break;
      case BoolUnknown:
        DropMode(true);
        break;
    }
    return S_OK;
  }
  HRESULT ObserveDropMode(mew::message msg) {
    msg["state"] = mew::ENABLED | (m_dropmode != BoolUnknown ? mew::CHECKED : 0);
    return S_OK;
  }
};

//==============================================================================

extern "C" __declspec(dllexport) int AvestaMain(PCWSTR args, INT sw, PWSTR* newAvesta) {
  theMainResult = NULL;
  theNewAvesta = NULL;
  thePygmy = NULL;

  AtlInitCommonControls(ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_USEREX_CLASSES);
  mew::util::GdiInit gdi;
  mew::util::InitModule();

  OleInitialize(nullptr);

  // ロード＆アンロードを繰り返しているようなので、あらかじめロードしておく。
  const PCWSTR PRELOADS[] = {
      L"shimgvw.dll",
      L"shdocvw.dll",
  };

  HINSTANCE hPreloads[lengthof(PRELOADS)];
  for (int i = 0; i < lengthof(PRELOADS); ++i) {
    hPreloads[i] = ::LoadLibrary(PRELOADS[i]);
  }

  try {
    Main main;
    theNewAvesta = newAvesta;
    main.Run(args, sw);
  } catch (mew::exceptions::Error& e) {
    MSG msg;
    while (::PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE)) {
    }
    ::MessageBox(nullptr, e.Message.str(), _T("FETAL ERROR"), MB_OK | MB_ICONERROR);
  }

  theNewAvesta = NULL;
  while (FreeLibrary(thePygmy)) {
  }

  for (int i = 0; i < lengthof(PRELOADS); ++i) {
    if (hPreloads[i]) {
      ::FreeLibrary(hPreloads[i]);
    }
  }

  OleUninitialize();

  // 全てのメッセージを除去
  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
  }

  return theMainResult;
}

namespace module {
HINSTANCE Handle;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD what, void*) {
  switch (what) {
    case DLL_PROCESS_ATTACH:
#ifdef _DEBUG
      _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
#endif
      module::Handle = hModule;
      break;
    case DLL_PROCESS_DETACH:
      break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
      break;
  }
  return true;
}

//==============================================================================

HMODULE avesta::GetDLL() { return module::Handle; }

HWND avesta::GetForm() {
  if (mew::ui::IWindow* form = static_cast<Main*>(theAvesta)->m_form) {
    return form->Handle;
  }
  return nullptr;
}

HWND avesta::GetForm(DWORD dwThreadId) {
  if (mew::ui::IWindow* form = static_cast<Main*>(theAvesta)->m_form) {
    return form->Handle;
  }
  return nullptr;
}

HRESULT avesta::AvestaExecute(mew::io::IEntry* entry) { return static_cast<Main*>(theAvesta)->AvestaExecute(entry); }

bool avesta::GetOption(BoolOption what) { return static_cast<Main*>(theAvesta)->m_booleans[what]; }

//==============================================================================

#include "CommandHandler.hpp"