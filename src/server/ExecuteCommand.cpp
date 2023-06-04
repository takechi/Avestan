// ExecuteCommand.cpp

#include "stdafx.h"
#include "main.hpp"
#include "std/buffer.hpp"
#include "object.hpp"

// #define ENABLE_UN

namespace {
using StringVector = std::vector<mew::string>;

enum ExecuteType {
  ExecuteNoArg = -1,
  ExecuteFolderCurrent,
  ExecuteFolderAll,
  ExecuteFolderShown,
  ExecuteFolderHidden,
  ExecuteFolderParent,
  ExecuteItems,
  ExecuteItemsSelected,
  ExecuteItemsChecked,
#ifdef ENABLE_UN
  ExecuteItemsUnselected,
  ExecuteItemsUnchecked,
#endif
  NumExecuteArgs,
};

static HRESULT Execute(PCWSTR path, PCWSTR args) {
  // ���ȌĂяo���̍œK��
  TCHAR selfname[MAX_PATH];
  ::GetModuleFileName(NULL, selfname, MAX_PATH);
  if (mew::str::equals_nocase(path, selfname)) {
    theAvesta->ParseCommandLine(args);
    return S_OK;
  }
  // �����ȊO
  mew::string dir = theAvesta->CurrentPath();
  return avesta::PathExecute(path, nullptr, args, dir.str());
}

class ExecuteCommandNoArgs : public mew::Root<mew::implements<mew::ICommand> > {
 private:
  mew::string m_path;
  mew::string m_args;

 public:
  ExecuteCommandNoArgs(mew::string path, mew::string args) : m_path(path), m_args(args) {}
  mew::string get_Description() { return m_path; }
  UINT32 QueryState(IUnknown* owner) { return mew::ENABLED; }
  void Invoke() { Execute(m_path.str(), m_args.str()); }
};

class __declspec(novtable) ExecuteCommandBase : public mew::Root<mew::implements<mew::ICommand> > {
 protected:
  ExecuteType m_type;
  mew::string m_path, m_argsL, m_argsR;

 protected:
  ExecuteCommandBase(mew::string path, ExecuteType type, PCWSTR args, PCWSTR le, PCWSTR rs) : m_path(path), m_type(type) {
    m_argsL.assign(args, le - args);
    m_argsR.assign(rs);
  }

 public:
  mew::string get_Description() { return m_path; }
  UINT32 QueryState(IUnknown* owner) {
    switch (m_type) {
      case ExecuteFolderCurrent:
      case ExecuteFolderAll:
      case ExecuteFolderShown:
      case ExecuteFolderHidden:
      case ExecuteFolderParent:
      case ExecuteItems:
      case ExecuteItemsChecked:
#ifdef ENABLE_UN
      case ExecuteItemsUnselected:
      case ExecuteItemsUnchecked:
#endif
      {
        mew::ref<mew::ui::IShellListView> current;
        if (SUCCEEDED(theAvesta->GetComponent(&current, avesta::AvestaFolder))) {
          return mew::ENABLED;
        } else {
          return 0;
        }
      }
      case ExecuteItemsSelected: {
        mew::ref<mew::ui::IShellListView> current;
        if (SUCCEEDED(theAvesta->GetComponent(&current, avesta::AvestaFolder)) && current->SelectedCount > 0) {
          return mew::ENABLED;
        } else {
          return 0;
        }
      }
      default:
        TRESPASS_DBG(return 0);
    }
  }

 protected:
  bool QueryFiles(StringVector& files) {
    switch (m_type) {
      case ExecuteFolderCurrent:
        return QueryFolders(mew::FOCUSED, files);
      case ExecuteFolderAll:
        return QueryFolders(mew::StatusNone, files);
      case ExecuteFolderShown:
        return QueryFolders(mew::SELECTED, files);
      case ExecuteFolderHidden:
        return QueryFolders(mew::UNSELECTED, files);
      case ExecuteFolderParent:
        return QueryParent(mew::FOCUSED, files);
      case ExecuteItems:
        return QueryItems(mew::StatusNone, files);
      case ExecuteItemsSelected:
        return QueryItems(mew::SELECTED, files);
      case ExecuteItemsChecked:
        return QueryItems(mew::CHECKED, files);
#ifdef ENABLE_UN
      case ExecuteItemsUnselected:
        return QueryItems(UNSELECTED, files);
      case ExecuteItemsUnchecked:
        return QueryItems(UNCHECKED, files);
#endif
      default:
        TRESPASS_DBG(return 0);
    }
  }
  static bool QueryFolders(mew::Status status, StringVector& files) {
    files.clear();
    mew::ref<mew::ui::IList> tab;
    if FAILED (theAvesta->GetComponent(&tab, avesta::AvestaTab)) {
      return false;
    }
    // current folder path.
    mew::ref<IEnumUnknown> views;
    if FAILED (tab->GetContents(&views, status)) {
      return false;
    }
    for (mew::each<mew::ui::IShellListView> i(views); i.next();) {
      if (mew::ref<mew::io::IEntry> entry = ave::GetFolderOfView(i)) {
        if (mew::string path = entry->Path) {
          files.push_back(path);
        }
      }
    }
    return !files.empty();
  }
  static bool QueryParent(mew::Status status, StringVector& files) {
    QueryFolders(status, files);
    for (size_t i = 0; i < files.size(); ++i) {
      if (PathIsRoot(files[i].str())) {
        files[i] = mew::io::GUID_MyComputer;  // �h���C�u�̏�̃t�H���_�̓}�C�R���s���[�^
      } else {
        WCHAR parent[MAX_PATH];
        files[i].copyto(parent);
        PathRemoveFileSpec(parent);
        files[i] = parent;
      }
    }
    return true;
  }
  static bool QueryItems(mew::Status status, StringVector& files) {
    mew::ref<mew::io::IEntryList> items;
    if (!QueryItems(status, &items)) {
      return false;
    }
    const size_t count = items->Count;
    for (size_t i = 0; i < count; ++i) {
      mew::ref<mew::io::IEntry> entry;
      if FAILED (items->GetAt(&entry, i)) {
        continue;
      }
      mew::string path = entry->Path;
      if (!path) {
        continue;
      }
      files.push_back(path);
    }
    return !files.empty();
  }
  static bool QueryItems(mew::Status status, mew::io::IEntryList** items) {
    ASSERT(items);
    mew::ref<mew::ui::IShellListView> current;
    mew::ref<mew::io::IEntry> entry;
    if (FAILED(theAvesta->GetComponent(&current, avesta::AvestaFolder)) || FAILED(current->GetFolder(&entry))) {
      return false;
    }
    // selections
    return SUCCEEDED(current->GetContents(items, status));
  }
};

class ExecuteEachCommand : public ExecuteCommandBase {
 private:
  using super = ExecuteCommandBase;

 public:
  ExecuteEachCommand(mew::string path, ExecuteType type, PCWSTR args, PCWSTR le, PCWSTR rs) : super(path, type, args, le, rs) {}
  void Invoke() {
    StringVector files;
    mew::string directory;
    if (!QueryFiles(files)) {
      theAvesta->Notify(avesta::NotifyWarning, mew::string::load(IDS_WARN_NOTARGET));
      return;
    }
    const size_t count = files.size();
    mew::StringBuffer params;
    params.reserve(m_argsL.length() + m_argsR.length() + MAX_PATH);
    for (size_t i = 0; i < count; ++i) {
      params.clear();
      params.append(m_argsL.str());
      params.append_path(files[i].str());
      params.append(m_argsR.str());
      params.append(L'\0');
      Execute(m_path.str(), params);
    }
  }
};

class ExecuteAllCommand : public ExecuteCommandBase {
 private:
  using super = ExecuteCommandBase;

 public:
  ExecuteAllCommand(mew::string path, ExecuteType type, PCWSTR args, PCWSTR le, PCWSTR rs) : super(path, type, args, le, rs) {}
  void Invoke() {
    StringVector files;
    if (!QueryFiles(files)) {
      theAvesta->Notify(avesta::NotifyWarning, mew::string::load(IDS_WARN_NOTARGET));
      return;
    }
    const size_t count = files.size();
    mew::StringBuffer params;
    params.reserve(m_argsL.length() + m_argsR.length() + MAX_PATH + 128 * count);  // �K���ȗʂ��m��
    params.append(m_argsL.str());
    for (size_t i = 0; i < count; ++i) {
      params.append_path(files[i].str());
      params.append(' ');
    }
    params.append(m_argsR.str());
    params.append(L'\0');
    Execute(m_path.str(), params);
  }
};

const PCWSTR ExecuteArgsText[] = {
    L"current",    L"all",       L"shown", L"hidden", L"parent", L"items", L"selected", L"checked",
#ifdef ENABLE_UN
    L"unselected", L"unchecked",
#endif
};

void SetArgName(PWSTR name, WCHAR left, WCHAR right, PCWSTR base, size_t len) {
  name[0] = left;
  wcsncpy(name + 1, base, len);
  name[len + 1] = right;
  name[len + 2] = L'\0';
}

ExecuteType SearchVarsInArgs(PCWSTR args, WCHAR bracket[2], PCWSTR* le, PCWSTR* rs) {
  WCHAR varname[32];
  // 'all' variables
  for (size_t i = 0; i < NumExecuteArgs; ++i) {
    PCWSTR var = ExecuteArgsText[i];
    size_t varlen = wcslen(var);
    SetArgName(varname, bracket[0], bracket[1], var, varlen);
    PCWSTR at = wcsstr(args, varname);
    if (at) {
      *le = at;
      *rs = at + varlen + 2;
      return (ExecuteType)i;
    }
  }
  return ExecuteNoArg;
}
}  // namespace

namespace {
bool PathExtensionExists(mew::string& basename, PCWSTR extension) {
  WCHAR path[MAX_PATH];
  basename.copyto(path, MAX_PATH);
  mew::str::append(path, extension);
  if (!::PathFileExists(path)) {
    return false;
  }
  basename = path;
  return true;
}

bool PathIsNonFile(PCWSTR path) {
  if (mew::str::compare(path, L"http://", 7) == 0) {
    return true;
  }
  if (mew::str::compare(path, L"https://", 8) == 0) {
    return true;
  }
  if (mew::str::compare(path, L"ftp://", 6) == 0) {
    return true;
  }
  return false;
}
}  // namespace

mew::ref<mew::ICommand> CreateExecuteCommand(mew::string path, mew::string args) {
  // path �̃p�X����������.
  if (!PathIsNonFile(path.str())) {
    path = ave::ResolvePath(path);
    if (!path) {
      return mew::null;
    }
    if (!PathFileExists(path.str())) {  // �s���S�ȃp�X�H
      // �Ƃ肠�����A�����⊮�������s�t�@�C���̊g���q�����Ď����Ă݂�B
      if (!PathExtensionExists(path, L".exe") && !PathExtensionExists(path, L".bat") && !PathExtensionExists(path, L".cmd")) {
        return mew::null;
      }
    }
  }
  // args.
  PCWSTR wcsArgs = args.str();
  if (wcsArgs) {
    PCWSTR le, rs;
    ExecuteType type;
    // 'all' variables
    type = SearchVarsInArgs(wcsArgs, L"{}", &le, &rs);
    if (type != ExecuteNoArg) {
      return mew::objnew<ExecuteAllCommand>(path, type, wcsArgs, le, rs);
    }
    // 'each' variables
    type = SearchVarsInArgs(wcsArgs, L"[]", &le, &rs);
    if (type != ExecuteNoArg) {
      return mew::objnew<ExecuteEachCommand>(path, type, wcsArgs, le, rs);
    }
    // no args, fall through
  }
  return mew::objnew<ExecuteCommandNoArgs>(path, args);
}
