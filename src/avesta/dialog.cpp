// dialog.cpp

#include "stdafx.h"
#include "avesta.hpp"

namespace avesta {

Dialog::Dialog() : m_hwndToolTip(NULL) {}

INT_PTR Dialog::Go(UINT nID, HWND parent, HINSTANCE module) {
  ASSERT(!m_hwnd);
  m_flags = 0;
  return ::DialogBoxParamW(module, MAKEINTRESOURCE(nID), parent, DlgProc, (LPARAM)this);
}

BOOL Dialog::End(INT_PTR result) { return ::EndDialog(m_hwnd, result); }

bool Dialog::OnCreate() {
  MoveToCenter();
  m_hwndToolTip = ::CreateWindowEx(0, TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP, 0, 0, 0, 0, m_hwnd, NULL,
                                   (HINSTANCE)::GetWindowLongPtrW(m_hwnd, GWLP_HINSTANCE), NULL);
  return __super::OnCreate();
}

void Dialog::OnDispose() {
  if (m_hwndToolTip) {
    ::DestroyWindow(m_hwndToolTip);
    m_hwndToolTip = NULL;
  }
}

INT_PTR CALLBACK Dialog::DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg == WM_INITDIALOG) {
    Dialog* self = (Dialog*)lParam;
    self->Attach(hwnd);
    return self->OnCreate();
  }
  return FALSE;
}

HWND Dialog::GetItem(UINT nID) const { return ::GetDlgItem(m_hwnd, nID); }

int Dialog::GetText(UINT nID, PTSTR text, int nMaxCount) const { return ::GetDlgItemText(m_hwnd, nID, text, nMaxCount); }

void Dialog::SetText(UINT nID, PCTSTR text) { ::SetDlgItemText(m_hwnd, nID, text); }

bool Dialog::GetEnabled(UINT nID) const { return ::IsWindowEnabled(GetItem(nID)) != 0; }

void Dialog::SetEnabled(UINT nID, bool enable) { ::EnableWindow(GetItem(nID), enable); }

bool Dialog::GetChecked(UINT nID) const {  // BST_CHECKED, BST_INDETERMINATE, BST_UNCHECKED
  return ::SendMessage(GetItem(nID), BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void Dialog::SetChecked(UINT nID, bool check) {  // BST_CHECKED, BST_INDETERMINATE, BST_UNCHECKED
  ::SendMessage(GetItem(nID), BM_SETCHECK, check ? BST_CHECKED : BST_UNCHECKED, 0);
}

void Dialog::SetTip(UINT nID, PCTSTR text) {
  TOOLINFO tip = {sizeof(TOOLINFO)};
  tip.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
  tip.uId = (UINT)GetDlgItem(m_hwnd, nID);
  tip.lpszText = const_cast<PTSTR>(text);
  ASSERT(::IsWindow(m_hwndToolTip));
  VERIFY(SendMessage(m_hwndToolTip, TTM_ADDTOOL, 0, (LPARAM)&tip));
}

}  // namespace avesta