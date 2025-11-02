// WallPaperDialog.cpp

#include "stdafx.h"
#include "../private.h"
#include "WindowImpl.h"
#include "widgets.client.hpp"

namespace mew {
namespace ui {

class WallPaperDialog : public Root<implements<IWallPaperDialog> >, public CDialogImpl<WallPaperDialog> {
 private:
  HWND m_hParent;

 public:
  void __init__(IUnknown* arg) {
    m_hParent = null;
    if (ref<IWindow> w = cast(arg)) {
      m_hParent = w->Handle;
    }
  }
  void Dispose() noexcept { RemoveAll(); }

 private:
  struct Target {
    ref<IWallPaper> object;
    string name;
    string file;
    DWORD align;
  };
  std::vector<Target> m_targets;
  size_t m_LastSel;

 public:
  enum { IDD = IDD_WALLPAPER };

 public:
  BEGIN_MSG_MAP(_)
  MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
  COMMAND_HANDLER(IDC_WALL_TARGET, CBN_SELCHANGE, OnTargetChange)
  COMMAND_ID_HANDLER(IDC_WALL_OPEN, OnOpen)
  COMMAND_ID_HANDLER(IDC_WALL_ERASE, OnErase)
  COMMAND_ID_HANDLER(IDC_WALL_APPLY, OnApply)
  COMMAND_ID_HANDLER(IDOK, OnOK)
  COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
  END_MSG_MAP()

  static int GetHorzAlign(DWORD align) {
    switch (align & DirMaskWE) {
      case DirWest:
        return 0;
      case 0:
        return 1;
      case DirEast:
        return 2;
      default:
        return 3;
    }
  }
  static int GetVertAlign(DWORD align) {
    switch (align & DirMaskNS) {
      case DirNorth:
        return 0;
      case 0:
        return 1;
      case DirSouth:
        return 2;
      default:
        return 3;
    }
  }
  void SetHorzAlign(Target& target, int index) {
    static const int dirs[4] = {DirWest, 0, DirEast, DirMaskWE};
    target.align = ((target.align & DirMaskNS) | dirs[index]);
  }
  void SetVertAlign(Target& target, int index) {
    static const int dirs[4] = {DirNorth, 0, DirSouth, DirMaskNS};
    target.align = ((target.align & DirMaskWE) | dirs[index]);
  }
  Target& GetCurrentTarget() {
    size_t index = SendDlgItemMessage(IDC_WALL_TARGET, CB_GETCURSEL, 0, 0);
    if (index < m_targets.size()) {
      return m_targets[index];
    } else {
      return m_targets.front();
    }
  }

  void AddToCombo(const Target& target) {
    SendDlgItemMessage(IDC_WALL_TARGET, CB_INSERTSTRING, (WPARAM)-1, (LPARAM)(PCTSTR)target.name.str());
  }
  LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL& bHandled) {
    bHandled = false;
    CenterWindow();
    algorithm::for_all(m_targets, [this](const Target& target) { AddToCombo(target); });
    m_LastSel = 0;
    SendDlgItemMessage(IDC_WALL_TARGET, CB_SETCURSEL, 0, 0);
    UpdateControls();
    return 0;
  }
  LRESULT OnOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL&) {
    if (string filename = OpenDialog(m_hWnd, string::load(IDS_DLG_IMAGES))) {
      SetDlgItemText(IDC_WALL_PATH, filename.str());
    }
    return 0;
  }
  LRESULT OnErase(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL&) {
    SetDlgItemText(IDC_WALL_PATH, NULL);
    return 0;
  }
  static void Apply(const Target& target) {
    target.object->WallPaperFile = target.file;
    target.object->WallPaperAlign = target.align;
  }
  LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL&) {
    DoApply();
    EndDialog(IDOK);
    return 0;
  }
  LRESULT OnApply(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL&) {
    DoApply();
    return 0;
  }
  void DoApply() {
    Target& target = GetCurrentTarget();
    UpdateTarget(target);
    algorithm::for_all(m_targets, Apply);
  }
  LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL&) {
    EndDialog(IDCANCEL);
    return 0;
  }
  LRESULT OnTargetChange(WORD, WORD, HWND, BOOL&) {
    Target& prev = m_targets[m_LastSel];
    UpdateTarget(prev);
    m_LastSel = SendDlgItemMessage(IDC_WALL_TARGET, CB_GETCURSEL, 0, 0);
    UpdateControls();
    return 0;
  }
  void UpdateControls() {
    Target& target = GetCurrentTarget();
    SetDlgItemText(IDC_WALL_PATH, target.file.str());
    for (int i = 0; i < 4; ++i) {
      CheckDlgButton(IDC_WALL_WEST + i, BST_UNCHECKED);
      CheckDlgButton(IDC_WALL_NORTH + i, BST_UNCHECKED);
    }
    CheckDlgButton(IDC_WALL_WEST + GetHorzAlign(target.align), BST_CHECKED);
    CheckDlgButton(IDC_WALL_NORTH + GetVertAlign(target.align), BST_CHECKED);
  }
  void UpdateTarget(Target& target) {
    TCHAR buffer[MAX_PATH];
    GetDlgItemText(IDC_WALL_PATH, buffer, MAX_PATH);
    target.file = buffer;
    for (int i = 0; i < 4; ++i) {
      if (IsDlgButtonChecked(IDC_WALL_WEST + i)) {
        SetHorzAlign(target, i);
        break;
      }
    }
    for (int i = 0; i < 4; ++i) {
      if (IsDlgButtonChecked(IDC_WALL_NORTH + i)) {
        SetVertAlign(target, i);
        break;
      }
    }
  }

  HRESULT AddTarget(IWallPaper* p, string name) {
    if (!p) {
      return E_INVALIDARG;
    }
    Target target;
    target.object = p;
    target.name = name;
    target.file = p->WallPaperFile;
    target.align = p->WallPaperAlign;
    m_targets.push_back(target);
    return S_OK;
  }
  void RemoveAll() { m_targets.clear(); }
  HRESULT Go() {
    ref<IUnknown> addref(this);
    if (m_targets.empty()) {
      return E_UNEXPECTED;
    }
    return DoModal(m_hParent) == IDOK ? S_OK : S_FALSE;
  }
};

AVESTA_EXPORT(WallPaperDialog)

}  // namespace ui
}  // namespace mew
