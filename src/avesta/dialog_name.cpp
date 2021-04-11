// dialog_name.cpp

#include "stdafx.h"
#include "../server/main.hpp"

using namespace avesta;

namespace {
class NameDlg : public Dialog {
 public:
  string m_path;
  TCHAR m_name[MAX_PATH];

 public:
  NameDlg() { str::clear(m_name); }
  INT_PTR Go(string path, string name, UINT nID) {
    m_path = path;
    name.copyto(m_name, MAX_PATH);
    return __super::Go(nID);
  }

 protected:
  bool OnCreate() {
    if (!__super::OnCreate()) return false;
    SetText(IDC_NEW_PATH, m_path.str());
    SetText(IDC_NEW_NAME, m_name);
    afx::Edit_SubclassSingleLineTextBox(GetItem(IDC_NEW_NAME), NULL, theAvesta->EditOptions);
    SetTip(IDC_NEW_PATH, _T("対象フォルダです。"));
    return true;
  }
  void OnCommand(UINT what, HWND ctrl) {
    switch (what) {
      case IDOK:
        GetText(IDC_NEW_NAME, m_name, MAX_PATH);
        m_path.clear();
        End(IDOK);
        break;
      case IDCANCEL:
        m_path.clear();
        End(IDCANCEL);
        break;
    }
  }
};
}  // namespace

HRESULT avesta::NameDialog(IString** pp, string path, string name, UINT resID) {
  ASSERT(pp);

  NameDlg dlg;
  if (dlg.Go(path, name, resID) != IDOK) {
    *pp = null;
    return E_ABORT;  // cancel
  }
  string(dlg.m_name).copyto(pp);
  return S_OK;
}
