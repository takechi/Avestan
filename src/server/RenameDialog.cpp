// RenameDialog.cpp

#include "stdafx.h"
#include "main.hpp"

namespace {
class RenameDlg : public avesta::Dialog {
 private:
  struct PathAndLeaf {
    mew::string path;
    mew::string leaf;
  };
  std::vector<PathAndLeaf> m_src;
  std::vector<mew::string> m_dst;
  bool m_focusListView;
  mew::ref<mew::io::IEntry> m_entry;

 public:
  RenameDlg(size_t reserve, bool focusListView, mew::io::IEntry* entry) {
    m_src.reserve(reserve);
    m_dst.reserve(reserve);
    m_focusListView = focusListView;
    m_entry = entry;
  }
  INT_PTR Go(UINT nID) { return __super::Go(nID); }
  mew::string GetSrcLeaf(size_t i) const { return m_src[i].leaf; }
  mew::string GetSrcPath(size_t i) const { return m_src[i].path; }
  size_t GetDstCount() const { return m_dst.size(); }
  mew::string GetDstLeaf(size_t i) const { return m_dst[i]; }
  void AddDst(const mew::string& name) { m_dst.push_back(name); }
  void AddSrc(mew::io::IEntryList* entries) {
    size_t count = entries->Count;
    for (size_t i = 0; i < count; ++i) {
      mew::ref<mew::io::IEntry> entry;
      entries->GetAt(&entry, i);
      PathAndLeaf s;
      s.leaf = entry->GetName(mew::io::IEntry::LEAF_OR_NAME);
      s.path = entry->GetName(mew::io::IEntry::PATH);
      m_src.push_back(s);
    }
  }
  bool AddDestination(PCTSTR beg, PCTSTR end) {
    if (m_src.size() <= m_dst.size()) return false;
    mew::string path = GetSrcPath(m_dst.size());
    if (!path || ::PathIsDirectory(path.str())) {
      m_dst.push_back(mew::string(beg, end));
    } else {
      TCHAR newname[MAX_PATH] = _T("");
      memcpy(newname, beg, (end - beg) * sizeof(TCHAR));
      ::PathAddExtension(newname, ::PathFindExtension(path.str()));
      m_dst.push_back(newname);
    }
    return true;
  }
  void AutoNumbering(PCTSTR text) {  // �����D�݂̃��l�[�����[�`���B���̐l���g���₷�����ǂ����͒m��Ȃ��B
    size_t selectCount = m_src.size();
    int n = 0;
    for (size_t cnt = selectCount; cnt > 0; cnt /= 10) {
      ++n;
    }
    size_t baselen = lstrlen(text);
    TCHAR postfix[] = _T(" %0?d%s");
    postfix[3] = (TCHAR)('0' + n);
    m_dst.clear();
    for (size_t i = 0; i < selectCount; ++i) {
      TCHAR newname[MAX_PATH];
      lstrcpy(newname, text);
      wsprintf(newname + baselen, postfix, i + 1, PathFindExtension(GetSrcLeaf(i).str()));
      m_dst.push_back(newname);
    }
  }
  static void EditLabel(HWND hList) {
    int index = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    if (index >= 0) ListView_EditLabel(hList, index);
  }
  static void GetLocation(PCTSTR text, int* index, int* beg, int* end) {
    if (PCTSTR bra = mew::str::find(text, L'<')) {
      PCTSTR num = bra + 1;
      while (*num != '\0' && !mew::str::find(_T("0123456789>"), *num)) {
        ++num;
      }
      *index = mew::str::atoi(num);
      *beg = bra - text;
      if (PCTSTR cket = mew::str::find(bra, L'>')) {
        *end = cket - text;
      } else {
        *end = mew::str::length(text);
      }
    } else {
      *index = 1;  // default: 1 base index
      *beg = *end = mew::str::length(text);
    }
  }
  void ReplaceAll(HWND hListView, HWND hEdit, bool preserveExtension) {
    TCHAR szTemplate[MAX_PATH] = {0};
    ::GetWindowText(hEdit, szTemplate, MAX_PATH);
    int index, beg, end;
    GetLocation(szTemplate, &index, &beg, &end);

    const size_t count = ListView_GetItemCount(hListView);

    int width = end - beg - 1;
    int n = 0;
    for (size_t cnt = count; cnt > 0; cnt /= 10) {
      ++n;
    }
    TCHAR postfix[] = _T("%0?d");
    postfix[2] = (TCHAR)('0' + mew::math::max(width, n));

    for (size_t i = 0; i < count; ++i) {
      TCHAR src[MAX_PATH];
      TCHAR dst[MAX_PATH];
      LVITEM item;
      item.mask = LVIF_TEXT;
      item.iItem = i;
      item.iSubItem = 0;
      if (preserveExtension) {
        item.pszText = src;
        item.cchTextMax = MAX_PATH;
        ListView_GetItem(hListView, &item);
      }
      mew::str::copy(dst, szTemplate, beg);
      wsprintf(dst + beg, postfix, index + i);
      mew::str::append(dst, szTemplate + end + 1);
      if (preserveExtension) {
        mew::str::append(dst, PathFindExtension(src));
      }
      item.pszText = dst;
      ListView_SetItem(hListView, &item);
      m_dst[item.iItem] = dst;
    }
  }

 private:
  void BeginLabelEdit(int index) {
    if (index < m_src.size()) {
      HWND hEdit = ListView_GetEditControl(GetItem(IDC_RENAMELIST));
      ASSERT(hEdit);
      mew::ref<IShellFolder> folder;
      if SUCCEEDED (m_entry->QueryObject(&folder)) {
        ::SHLimitInputEdit(hEdit, folder);
      }
      afx::Edit_SubclassSingleLineTextBox(hEdit, m_src[index].path.str(), theAvesta->EditOptions | afx::EditTypeFileName);
    }
  }
  bool EndLabelEdit(HWND hListView, NMLVDISPINFOW* nm) {
    size_t index = nm->item.iItem;
    PCWSTR text = nm->item.pszText;
    if (!text || index >= m_dst.size()) return false;
    m_dst[index] = text;
    if (index + 1 < m_dst.size()) {  // ���̃A�C�e����ҏW
      ::PostMessage(hListView, LVM_EDITLABEL, index + 1, 0);
    } else {  // OK�{�^���Ƀt�H�[�J�X���ڂ�
      ::PostMessage(m_hwnd, WM_KEYDOWN, VK_TAB, 0);
    }
    return true;
  }

 protected:
  bool OnCreate() {
    if (!__super::OnCreate()) return false;

    HWND hwndEdit = GetItem(IDC_REPLACE_EDIT);
    afx::Edit_SubclassSingleLineTextBox(hwndEdit, NULL, theAvesta->EditOptions);
    ::SetWindowText(hwndEdit, _T("<1>"));

    SetTip(IDC_REPLACE_EDIT, _T("�g<�h�g>�h�ň͂񂾕������A�Ԃɒu������܂��B��F�gprefix-<00777>-postfix�h"));
    SetTip(IDC_RENAME_REPLACE, _T("�g���q�͕ۑ�����܂��B�V�t�g�L�[�������Ȃ���̏ꍇ�͊g���q���u���������܂��B"));

    HWND hListView = GetItem(IDC_RENAMELIST);
    RECT rcListView;
    ::SetWindowLongPtr(hListView, GWL_STYLE, ::GetWindowLongPtr(hListView, GWL_STYLE) | LVS_EDITLABELS);
    ::GetClientRect(hListView, &rcListView);
    ::SetFocus(hListView);
    LVCOLUMN column = {LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH | LVCF_ORDER};
    column.cx = (rcListView.right - ::GetSystemMetrics(SM_CXVSCROLL)) / 2;
    column.iSubItem = 0;
    column.iOrder = 0;
    column.pszText = _T("�ύX�O");
    ListView_InsertColumn(hListView, 0, &column);
    column.iSubItem = 1;
    column.iOrder = 1;
    column.pszText = _T("�ύX��");
    ListView_InsertColumn(hListView, 0, &column);

    size_t count = m_src.size();
    for (size_t i = 0; i < count; ++i) {
      LVITEM srcfile, dstfile;
      srcfile.mask = dstfile.mask = LVIF_TEXT;
      srcfile.iItem = dstfile.iItem = i;
      srcfile.iSubItem = 1;
      srcfile.pszText = (PTSTR)m_src[i].leaf.str();
      dstfile.iSubItem = 0;
      dstfile.pszText = (PTSTR)m_dst[i].str();
      ListView_InsertItem(hListView, &dstfile);
      ListView_SetItem(hListView, &srcfile);
    }

    if (m_focusListView) {
      ::SetFocus(hListView);
      ListView_EditLabel(hListView, 0);
      return false;
    }

    return true;
  }

  LRESULT OnMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
      case WM_NOTIFY:
        NMHDR* nm = (NMHDR*)lParam;
        if (nm->idFrom == IDC_RENAMELIST) {
          switch (nm->code) {
            case NM_CLICK: {
              NMITEMACTIVATE* item = (NMITEMACTIVATE*)nm;
              ListView_EditLabel(nm->hwndFrom, item->iItem);
              break;
            }
            case LVN_KEYDOWN: {
              NMLVKEYDOWN* key = (NMLVKEYDOWN*)nm;
              switch (key->wVKey) {
                case VK_RETURN:
                case VK_SPACE:
                case VK_F2:
                  EditLabel(nm->hwndFrom);
                  break;
              }
              break;
            }
            case LVN_BEGINLABELEDITW:
              BeginLabelEdit(((NMLVDISPINFOW*)lParam)->item.iItem);
              return 0;
            case LVN_ENDLABELEDITW:
              return EndLabelEdit(nm->hwndFrom, (NMLVDISPINFOW*)nm);
            default:
              break;
          }
        }
        break;
    }
    return __super::OnMessage(msg, wParam, lParam);
  }

  void OnCommand(UINT what, HWND ctrl) {
    switch (what) {
      case IDOK:
        if (!ctrl) {  // by enter
          if (GetKeyState(VK_CONTROL) & 0x8000) {
            End(IDOK);  // �R���g���[���������Ȃ���̏ꍇ�̓_�C�A���O��OK�I���Ƃ݂Ȃ�
          } else if (::GetFocus() == GetItem(IDC_REPLACE_EDIT)) {
            ReplaceAll(GetItem(IDC_RENAMELIST), GetItem(IDC_REPLACE_EDIT), !mew::ui::IsKeyPressed(VK_SHIFT));
          } else {
            EditLabel(GetItem(IDC_RENAMELIST));
          }
        }
        break;
      case IDC_RENAME_OK:
        End(IDOK);
        break;
      case IDCANCEL:
        End(IDCANCEL);
        break;
      case IDC_RENAME_REPLACE:
        ReplaceAll(GetItem(IDC_RENAMELIST), GetItem(IDC_REPLACE_EDIT), !mew::ui::IsKeyPressed(VK_SHIFT));
        break;
    }
  }
};
}  // namespace

static HRESULT RenameFromClipboard(RenameDlg& dlg, mew::io::IEntryList* entries) {
  mew::string text = afx::GetClipboardText();
  if (!text) {
    theAvesta->Notify(avesta::NotifyError, mew::string::load(IDS_ERR_CLIPNOTTEXT));
    return E_FAIL;
  }
  // query selected items
  dlg.AddSrc(entries);
  // parse text
  for (PCTSTR beg = text.str(); beg;) {
    if (PCTSTR end = mew::str::find_some_of(beg, _T("\r\n"))) {
      if (end - beg > 1) {  // �󕶎��s�͖���
        dlg.AddDestination(beg, end);
      }
      beg = end + 1;
      while (*beg != _T('\0') && *beg == _T('\n')) {
        ++beg;
      }
    } else {
      size_t len = lstrlen(beg);
      if (len > 0) {  // �󕶎��s�͖���
        dlg.AddDestination(beg, beg + len);
      }
      break;
    }
  }
  size_t selectCount = entries->Count;
  if (dlg.GetDstCount() == 1 && selectCount > 1) {
    dlg.AutoNumbering(text.str());
  }
  if (dlg.GetDstCount() < selectCount) {
    theAvesta->Notify(avesta::NotifyError, mew::string::load(IDS_ERR_CLIPTEXTLACK));
    return E_FAIL;
  }
  return S_OK;
}

static HRESULT SimpleRename(RenameDlg& dlg, mew::io::IEntryList* entries) {
  size_t selectCount = entries->Count;
  // query selected items
  dlg.AddSrc(entries);
  // source
  for (size_t i = 0; i < selectCount; ++i) {
    dlg.AddDst(dlg.GetSrcLeaf(i));
  }
  return S_OK;
}

void DlgRename(mew::ui::IShellListView* view, bool paste) {
  ASSERT(view);
  HRESULT hr;
  mew::ref<mew::io::IEntry> folder;
  mew::ref<mew::io::IEntryList> entries;

  if (view->SelectedCount == 0) {
    return;
  }
  if FAILED (view->GetFolder(&folder)) {
    return;
  }
  if FAILED (view->GetContents(&entries, mew::SELECTED)) {
    return;
  }

  size_t count = entries->Count;
  RenameDlg dlg(count, !paste, folder);

  if (paste) {
    hr = RenameFromClipboard(dlg, entries);
  } else {
    hr = SimpleRename(dlg, entries);
  }
  if FAILED (hr) {
    return;
  }

  if (dlg.Go(IDD_RENAME) == IDOK) {
    for (size_t i = 0; i < count; ++i) {
      if (!dlg.GetSrcLeaf(i)) {
        continue;
      }
      TCHAR srcpath[MAX_PATH] = _T(""), dstpath[MAX_PATH];
      dlg.GetSrcPath(i).copyto(srcpath);
      lstrcpyn(dstpath, srcpath, MAX_PATH);
      PathRemoveFileSpec(dstpath);
      PathAppend(dstpath, dlg.GetDstLeaf(i).str());
      avesta::FileRename(srcpath, dstpath);
    }
  }
}
