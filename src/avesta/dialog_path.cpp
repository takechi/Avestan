// dialog_path.cpp

#include "stdafx.h"
#include "../server/main.hpp"

using namespace avesta;

namespace {
class PathDlg : public Dialog {
  using MRUList = std::vector<string>;

 public:
  TCHAR m_path[MAX_PATH];
  MRUList m_mru;
  ref<IEntry> m_entry;

 public:
  PathDlg() { str::clear(m_path); }
  INT_PTR Go(const string& path) {
    if (path) path.copyto(m_path, MAX_PATH);
    return __super::Go(IDD_OPENLOCATION);
  }

 private:
  template <class Iter>
  static void AddMRUListToComboBox(HWND hComboBox, Iter beg, Iter end) {
    for (Iter i = beg; i != end; ++i) {
      string s = *i;
      COMBOBOXEXITEM item = {CBEIF_TEXT, -1, const_cast<PTSTR>(s.str())};
      ::SendMessage(hComboBox, CBEM_INSERTITEM, 0, (LPARAM)&item);
    }
  }
  MRUList::iterator MRUListContains(PCTSTR path) {
    for (MRUList::iterator i = m_mru.begin(); i != m_mru.end(); ++i) {
      if ((*i).equals_nocase(path)) return i;
    }
    return m_mru.end();
  }
  void AddToMRUList(PCTSTR path) {
    if (str::empty(path)) return;
    MRUList::iterator i = MRUListContains(path);
    if (i != m_mru.end()) {
      m_mru.erase(i);
    } else {
      int eraseCount = m_mru.size() - 20;
      if (eraseCount > 0) {
        m_mru.erase(m_mru.begin(), m_mru.begin() + eraseCount);
      }
    }
    m_mru.push_back(path);
  }

 protected:
  bool OnCreate() {
    if (!__super::OnCreate()) return false;
    SetText(IDC_PATH, m_path);
    CComboBoxEx combo = GetItem(IDC_PATH);
    combo.SetExtendedStyle(CBES_EX_PATHWORDBREAKPROC, CBES_EX_PATHWORDBREAKPROC);
    SHAutoComplete(combo.GetEditCtrl(), SHACF_FILESYSTEM | SHACF_USETAB);
    afx::Edit_SubclassSingleLineTextBox(combo.GetEditCtrl(), NULL, theAvesta->EditOptions | afx::EditTypeFullPath);
    AddMRUListToComboBox(combo, m_mru.rbegin(), m_mru.rend());
    return true;
  }
  void OnCommand(UINT what, HWND ctrl) {
    switch (what) {
      case IDOK:
        GetText(IDC_PATH, m_path, MAX_PATH);
        try {
          if (str::empty(m_path))
            m_entry = theAvesta->GetMyDocuments();
          else
            m_entry.create(__uuidof(Entry), string(m_path));
          AddToMRUList(m_path);
          End(IDOK);
        } catch (Error& e) {
          WarningBox(m_hwnd, e.Message);
        }
        break;
      case IDCANCEL:
        End(IDCANCEL);
        break;
    }
  }
};
}  // namespace

ref<IEntry> avesta::PathDialog(string path) {
  static PathDlg dlg;
  if (dlg.Go(path) == IDOK) {
    ref<IEntry> entry = dlg.m_entry;
    dlg.m_entry = null;
    return entry;
  }
  // cancel;
  return null;
}
