// AvestaDialog.cpp

#include "stdafx.h"
#include "main.hpp"

//==============================================================================
// common

namespace {
mew::string GetDirectoryOfView(mew::ui::IShellListView* view) {
  if (mew::ref<mew::io::IEntry> entry = ave::GetFolderOfView(view)) {
    mew::string path = entry->Path;
    if (!!path) {
      return path;
    } else {
      theAvesta->Notify(avesta::NotifyError, mew::string::load(IDS_ERR_VIRTUALFOLDER, entry->Name));
    }
  } else {
    theAvesta->Notify(avesta::NotifyError, mew::string::load(IDS_BAD_FODLER));
  }
  return mew::null;
}

bool Recheck(mew::ui::IShellListView* view, mew::string path) {
  // �_�C�A���O��\�����Ă���ԂɃt�H���_�r���[������ or �p�X���ς��\�������邽�߁A������x�`�F�b�N����.
  if (path == ave::GetPathOfView(view)) {
    return true;
  }
  theAvesta->Notify(avesta::NotifyWarning, mew::string::load(IDS_ERR_BADFOLDER));
  return false;
}
}  // namespace

//==============================================================================
// New

namespace {
class NewFileDlg : public avesta::Dialog {
 public:
  mew::string m_location;
  TCHAR m_extension[MAX_PATH];
  TCHAR m_names[MAX_PATH];
  bool m_select;

 public:
  NewFileDlg() {
    mew::str::clear(m_extension);
    mew::str::clear(m_names);
    m_select = true;
  }
  INT_PTR Go(mew::string location) {
    m_location = location;
    return __super::Go(IDD_NEWFILE);
  }

 protected:
  virtual bool OnCreate() {
    if (!__super::OnCreate()) {
      return false;
    }
    SetText(IDC_NEW_PATH, m_location.str());
    SetText(IDC_NEW_EXT, m_extension);
    afx::Edit_SubclassSingleLineTextBox(GetItem(IDC_NEW_EXT), NULL, theAvesta->EditOptions | afx::EditTypeMultiName);
    SetText(IDC_NEW_NAME, m_names);
    afx::Edit_SubclassSingleLineTextBox(GetItem(IDC_NEW_NAME), NULL, theAvesta->EditOptions);
    SetChecked(IDC_NEW_SELECT, m_select);
    SetTip(IDC_NEW_PATH, _T("�t�@�C�����쐬�����ꏊ�ł��B"));
    SetTip(IDC_NEW_NAME, _T("'/' �� ';' �ŋ�؂�ƕ��������ɍ쐬�ł��܂��B��̏ꍇ�̓f�t�H���g�̖��O�������܂��B"));
    SetTip(IDC_NEW_EXT, _T("��̏ꍇ�̓t�H���_���쐬����܂��B'.' �͕t���Ă��t���Ȃ��Ă����܂��܂���B"));
    SetTip(IDC_NEW_SELECT, _T("�쐬��Ƀt�@�C����I����Ԃɂ���ꍇ�̓`�F�b�N���܂��B"));
    return true;
  }

  void OnCommand(UINT what, HWND ctrl) {
    switch (what) {
      case IDOK:
        GetText(IDC_NEW_EXT, m_extension, MAX_PATH);
        if (m_extension[0] != __T('\0') && m_extension[0] != _T('.')) {
          mew::str::prepend(m_extension, _T("."));
        }
        GetText(IDC_NEW_NAME, m_names, MAX_PATH);
        m_select = GetChecked(IDC_NEW_SELECT);
        m_location.clear();
        End(IDOK);
        break;
      case IDCANCEL:
        m_location.clear();
        End(IDCANCEL);
        break;
    }
  }
};

#define FORBIDDEN_PATH_CHARS L"\\/:\"<>|*?\t\r\n"

void CreateFileOrFolder(std::vector<mew::string>& newfiles, const mew::string& path, PCWSTR names, PCWSTR extension) {
  // �Z�~�R�����͖{���t�@�C���p�X�p�̕����Ƃ��Ďg���邪�A�����ł̓p�X��؂�Ƃ��Ĉ������Ƃɂ���B
  const PCWSTR SEPARATOR = FORBIDDEN_PATH_CHARS L";";
  const PCWSTR TRIM = FORBIDDEN_PATH_CHARS L"; ";
  const bool isEmptyExtension = mew::str::empty(extension);
  for (mew::StringSplit token(names, SEPARATOR, TRIM);;) {
    mew::string leaf = token.next();
    if (!leaf) {
      break;
    }
    WCHAR file[MAX_PATH];
    path.copyto(file, MAX_PATH);
    PathAppendW(file, leaf.str());

    HRESULT hr;
    if (isEmptyExtension) {
      if (PathFileExistsW(file)) {
        PathMakeUniqueName(file, MAX_PATH, NULL, leaf.str(), path.str());
      }
      int win32err = SHCreateDirectory(avesta::GetForm(), file);
      hr = (win32err == ERROR_SUCCESS ? S_OK : AtlHresultFromWin32(win32err));
    } else {
      ASSERT(extension[0] == _T('.'));
      CT2CW wext(extension);
      lstrcatW(file, wext);
      if (PathFileExistsW(file)) {
        WCHAR leafWithExt[MAX_PATH];
        leaf.copyto(leafWithExt);
        lstrcatW(leafWithExt, wext);
        PathMakeUniqueName(file, MAX_PATH, NULL, leafWithExt, path.str());
      }
      hr = avesta::FileNew(file);
    }

    if (SUCCEEDED(hr))
      newfiles.push_back(file);
    else
      theAvesta->Notify(avesta::NotifyWarning, mew::string::format(L"$1 �̍쐬�Ɏ��s���܂���", file));
  }
}

enum AfterCreateEffect {
  AfterCreateNone,
  AfterCreateSelect,
  AfterCreateRename,
};

static void CreateAndSelect(mew::ui::IShellListView* view, const mew::string& path, PCWSTR names, PCWSTR extension,
                            AfterCreateEffect after) {
  std::vector<mew::string> newfiles;
  if (mew::str::empty(names)) names = theAvesta->GetDefaultNewName();

  CreateFileOrFolder(newfiles, path, names, extension);
  if (after == AfterCreateNone || newfiles.empty()) return;

  view->Send(mew::ui::CommandSelectNone);
  // 10�����Ă��I���ł��Ȃ��悤�Ȃ�΂�����߂�
  for (int count = 0; count < 10; ++count) {
    // �t�@�C���V�X�e���ւ̕ύX�Ɏ��Ԃ������邽�߁A�����ɂ͑I���ł��Ȃ��ꍇ������B
    // ���̃X���[�v�ƃ��b�Z�[�W�f�B�X�p�b�`�́A�ύX���ʒm�����̂�҂��߂ɕK�v���Ǝv����B
    Sleep(200);  // �����܂������Ԃ�������Ȃ��B
    afx::PumpMessage();
    //
    bool unique = true;
    for (size_t i = 0; i < newfiles.size(); ++i) {
      PCWSTR newfile = newfiles[i].str();
      PCWSTR newname = PathFindFileName(newfile);
      VERIFY_HRESULT(view->SetStatus(mew::string(newname), mew::SELECTED, unique));
      unique = false;
    }
    mew::ref<mew::io::IEntryList> entries;
    if (SUCCEEDED(view->GetContents(&entries, mew::SELECTED)) &&
        entries->Count == newfiles.size()) {  // unique�I���ł�������I�𐔂��[���ɂȂ邽�߁A���݂̂̔��ʂŏ\���Ȃ͂��B
      TRACE(_T("info: �V�K�쐬�t�@�C���̑I���� $1 ��̃��[�v���K�v�ł���"), count);
      if (after == AfterCreateRename) {
        view->Send(mew::ui::CommandRename);
      }
      break;
    }
  }
}
}  // namespace

void NewFolder(mew::ui::IShellListView* view) {
  if (mew::string path = GetDirectoryOfView(view)) {
    CreateAndSelect(view, path, nullptr, nullptr, AfterCreateRename);
  }
}

void DlgNew(mew::ui::IShellListView* view) {
  mew::string path = GetDirectoryOfView(view);
  if (!path) return;
  static NewFileDlg dlg;
  mew::ref<IUnknown> unk(view);  // AddRef()�̂���
  if (dlg.Go(path) == IDOK) {
    if (Recheck(view, path))
      CreateAndSelect(view, path, dlg.m_names, dlg.m_extension, (dlg.m_select ? AfterCreateSelect : AfterCreateNone));
  }
}

//==============================================================================
// Select & Pattern

namespace {
static void DoSelect(mew::ui::IShellListView* view, const mew::string& path, PCTSTR pattern) {
  bool unique = !mew::ui::IsKeyPressed(VK_CONTROL);
  if (unique) {
    view->Send(mew::ui::CommandSelectNone);
  }

  TCHAR buf[MAX_PATH];
  PathCombine(buf, path.str(), L"*.*");
  int count = 0;
  WIN32_FIND_DATA find;
  HANDLE hFind = ::FindFirstFile(buf, &find);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (lstrcmp(find.cFileName, _T(".")) == 0) {
        continue;
      }
      if (lstrcmp(find.cFileName, _T("..")) == 0) {
        continue;
      }
      if (!afx::PatternEquals(pattern, find.cFileName)) {
        continue;
      }
      if SUCCEEDED (view->SetStatus(mew::string(find.cFileName), mew::SELECTED, unique)) {
        ++count;
        unique = false;
      }
    } while (::FindNextFile(hFind, &find));
    ::FindClose(hFind);
  }
  if (count == 0) {  // �w�肳�ꂽ�p�^�[�������������Ȃ������B
    theAvesta->Notify(avesta::NotifyWarning, mew::string::load(IDS_WARN_NOSELECT));
  }
}
}  // namespace

void DlgSelect(mew::ui::IShellListView* view) {
  static mew::string theLastPattern;

  mew::string path = GetDirectoryOfView(view);
  if (!path) {
    return;
  }

  mew::ref<IUnknown> addref(view);
  mew::string pattern;
  if SUCCEEDED (avesta::NameDialog(&pattern, path, theLastPattern, IDD_SELECT)) {
    if (pattern && Recheck(view, path)) {
      theLastPattern = pattern;
      DoSelect(view, path, pattern.str());
    }
  }
}

void DlgPattern(mew::ui::IShellListView* view) {
  mew::string path = GetDirectoryOfView(view);
  if (!path) {
    return;
  }

  mew::ref<IUnknown> addref(view);
  mew::string mask;
  if SUCCEEDED (avesta::NameDialog(&mask, path, view->PatternMask, IDD_PATTERN)) {
    if (Recheck(view, path)) {
      view->PatternMask = mask;
    }
  }
}
