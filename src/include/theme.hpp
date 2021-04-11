/// @file theme.hpp
/// テーマ.
#pragma once

#include <uxtheme.h>

namespace mew {
/// テーマ.
namespace theme {
//==============================================================================
// Basic

void HandleSystemSettingChange();
COLORREF DotNetTabBkgndColor();
COLORREF DotNetInactiveTextColor();

//==============================================================================
// Button

void ButtonDrawDrop(HDC hDC, RECT rc, UINT cdis);

void ButtonDrawClose(HDC hDC, RECT rc, UINT cdis);

//==============================================================================
// Tab

void TabDrawHeader(HDC hDC, RECT& rc);

void TabDrawItem(HDC hDC, RECT bounds, PCWSTR text, DWORD status, HFONT hFontNormal, HFONT hFontBold, COLORREF clrActiveTab,
                 COLORREF clrActiveText, COLORREF clrInactiveText);

// void TabDrawBackground(const RECT& rcBounds, size_t index, size_t count, DWORD mode);

// int TabHitTest(const RECT& rcBounds, POINT pt);

// void TabCalcLayout(const RECT& rcBounds, size_t index, size_t count, DWORD mode, RECT& rcItem);

//==============================================================================
// Menu

/// メニューバーのボタンを描きます.
bool MenuDrawButton(NMTBCUSTOMDRAW* draw, HFONT hFont, bool bShowKeyboardCues, bool bIsMenuDropped);
/// ポップアップメニューの項目を描きます.
void MenuDrawItem(HDC hDC, const RECT& rcBounds, DWORD dwStatus, PCWSTR wcsText, IImageList* pImageList, int nImage);
/// ポップアップメニューの項目の高さを計算します.
SIZE MenuMeasureItem(PCWSTR wcsText, IImageList* pImageList, int nImage);
}  // namespace theme
}  // namespace mew

namespace avesta {
using mew::null;

class Theme {
 private:
  HTHEME m_hTheme;

 public:
  Theme();
  ~Theme();
  bool Create(HWND hwnd, PCWSTR scheme);
  void Dispose() throw();

  void Icon(HDC hDC, int part, int state, const RECT* rc, HIMAGELIST hImageList, int index);
  void Background(HDC hDC, int part, int state, const RECT* rc, const RECT* clip = null);
  void Edge(HDC hDC, int part, int state, const RECT* rc, UINT uEdge, UINT uFlags, RECT* pContentRect = null);
  void Text(HDC hDC, int part, int state, const RECT* rc, PCWSTR text, DWORD dwFlagsDT, bool gray = false);
};
}  // namespace avesta
