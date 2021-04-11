// theme.cpp

#include "stdafx.h"
#include "theme.hpp"
#include "widgets.hpp"
#include "drawing.hpp"
#include "../mew/private.h"

#pragma comment(lib, "uxtheme")

using namespace mew;
using namespace mew::ui;
using namespace mew::drawing;
using namespace avesta;

/*
CloseThemeData
DrawThemeEdge
DrawThemeParentBackground
DrawThemeText
*/

Theme::Theme() : m_hTheme(null) {}

Theme::~Theme() { Dispose(); }

bool Theme::Create(HWND hwnd, PCWSTR scheme) {
  ASSERT(!m_hTheme);
  m_hTheme = ::OpenThemeData(hwnd, scheme);
  return m_hTheme != null;
}

void Theme::Dispose() throw() {
  if (m_hTheme) {
    ::CloseThemeData(m_hTheme);
    m_hTheme = null;
  }
}

void Theme::Icon(HDC hDC, int part, int state, const RECT* rc, HIMAGELIST hImageList, int index) {
  if (m_hTheme) {
    HRESULT hr = DrawThemeIcon(m_hTheme, hDC, part, state, rc, hImageList, index);
    hr;
  } else {
  }
}

void Theme::Background(HDC hDC, int part, int state, const RECT* rc, const RECT* clip) {
  if (m_hTheme) {
    // RECT extent;
    // GetThemeBackgroundExtent(m_hTheme, hDC, part, state, rc, &extent);
    // HRESULT hr = DrawThemeBackground(m_hTheme, hDC, part, state, &extent, clip);
    HRESULT hr = DrawThemeBackground(m_hTheme, hDC, part, state, rc, clip);
    hr;
  } else {
  }
}

/*
BDR_RAISEDINNER
BDR_SUNKENINNER
BDR_RAISEDOUTER
BDR_SUNKENOUTER
EDGE_BUMP
EDGE_ETCHED
EDGE_RAISED
EDGE_SUNKEN

BF_FLAT
BF_DIAGONAL
BF_DIAGONAL_ENDBOTTOMLEFT
BF_DIAGONAL_ENDBOTTOMRIGHT
BF_DIAGONAL_ENDTOPLEFT
BF_DIAGONAL_ENDTOPRIGHT
BF_LEFT
BF_MIDDLE
BF_MONO
BF_RECT
BF_RIGHT
BF_SOFT
BF_TOP
BF_TOPLEFT
BF_TOPRIGHT
BF_BOTTOM
BF_BOTTOMLEFT
BF_BOTTOMRIGHT
*/
void Theme::Edge(HDC hDC, int part, int state, const RECT* rc, UINT uEdge, UINT uFlags, RECT* pContentRect) {
  if (m_hTheme) {
    if (pContentRect) uFlags |= BF_ADJUST;
    HRESULT hr = DrawThemeEdge(m_hTheme, hDC, part, state, rc, uEdge, uFlags, pContentRect);
    hr;
  } else {
  }
}

void Theme::Text(HDC hDC, int part, int state, const RECT* rc, PCWSTR text, DWORD dwFlagsDT, bool gray) {
  if (m_hTheme) {
    HRESULT hr = DrawThemeText(m_hTheme, hDC, part, state, text, -1, dwFlagsDT, (gray ? DTT_GRAYED : 0), rc);
    hr;
  } else {
  }
}

/*
        enum WidgetScheme
        {
        BUTTON,
                CLOCK,
                COMBOBOX,
                EDIT,
                EXPLORERBAR,
                GLOBALS,
                HEADER, // ListView ˆê—— ‚Ìã•”‚Ìƒwƒbƒ_
                LISTVIEW,
                MENU,
                MENUBAND,
                PAGE,
                PROGRESS,
                REBAR,
                SCROLLBAR,
                SPIN,
                STARTPANEL,
                STATUS,
                TAB,
                TASKBAND,
                TASKBAR,
                TOOLBAR,
                TOOLTIP,
                TRACKBAR,
                TRAYNOTIFY,
                TREEVIEW,
                WINDOW,
        };
*/
//==============================================================================
// Basic

COLORREF mew::theme::DotNetTabBkgndColor() {
  COLORREF clrBtnFace = ::GetSysColor(COLOR_BTNFACE);
  BYTE r = GetRValue(clrBtnFace);
  BYTE g = GetGValue(clrBtnFace);
  BYTE b = GetBValue(clrBtnFace);
  BYTE nMax = math::max(r, g, b);
  const BYTE magic = (nMax > (0xFF - 35)) ? (BYTE)(0xFF - nMax) : (BYTE)35;
  if (nMax == 0) {
    r = (BYTE)(r + magic);
    g = (BYTE)(g + magic);
    b = (BYTE)(b + magic);
  } else {
    r = (BYTE)(r + magic * r / nMax);
    g = (BYTE)(g + magic * g / nMax);
    b = (BYTE)(b + magic * b / nMax);
  }
  return RGB(r, g, b);
}

COLORREF mew::theme::DotNetInactiveTextColor() {
  COLORREF clrGrayText = ::GetSysColor(COLOR_GRAYTEXT);

  BYTE r = GetRValue(clrGrayText);
  BYTE g = GetGValue(clrGrayText);
  BYTE b = GetBValue(clrGrayText);
  BYTE nMax = math::max(r, g, b);
  const BYTE magic = 43;
  if (nMax != 0) {
    r = (BYTE)(r < magic ? r / 2 : r - magic * r / nMax);
    g = (BYTE)(g < magic ? g / 2 : g - magic * g / nMax);
    b = (BYTE)(b < magic ? b / 2 : b - magic * b / nMax);
  }
  return RGB(r, g, b);
}

//==============================================================================
// Tab

void mew::theme::TabDrawHeader(HDC hDC, RECT& rc) {
  DCHandle dc(hDC);
  rc.bottom -= 1;
  dc.FrameRect(&rc, ::GetSysColorBrush(COLOR_3DSHADOW));
  Pen pen(::GetSysColor(COLOR_3DHILIGHT));
  CPenHandle penOld = dc.SelectPen(pen);
  dc.MoveTo(rc.left + 1, rc.bottom - 2);
  dc.LineTo(rc.left + 1, rc.top + 1);
  dc.LineTo(rc.right - 2, rc.top + 1);
  dc.LineTo(rc.right - 2, rc.bottom - 1);
  dc.MoveTo(rc.left, rc.bottom);
  dc.LineTo(rc.right, rc.bottom);
  dc.SelectPen(penOld);
  rc.left += 2;
  rc.right -= 2;
  rc.top += 2;
  rc.bottom -= 1;
}

void mew::theme::TabDrawItem(HDC hDC, RECT bounds, PCWSTR text, DWORD status, HFONT hFontNormal, HFONT hFontBold,
                             COLORREF clrActiveTab, COLORREF clrActiveText, COLORREF clrInactiveText) {
  DCHandle dc(hDC);
  if (status & CHECKED) {
    bounds.right -= 1;
    bounds.bottom += 1;

    dc.FillSolidRect(&bounds, clrActiveTab);

    Pen penHilight(::GetSysColor(COLOR_BTNHIGHLIGHT));
    Pen penText(clrActiveText);
    dc.DrawLine(penHilight, bounds.left, bounds.top, bounds.left, bounds.bottom);
    dc.DrawLine(penText, bounds.right, bounds.top, bounds.right, bounds.bottom - 2);
  } else {
    Pen pen(::GetSysColor(COLOR_BTNSHADOW));
    if (false)  //(bHasBottomStyle)
      dc.DrawLine(pen, bounds.right - 1, bounds.top + 3, bounds.right - 1, bounds.bottom - 1);
    else
      dc.DrawLine(pen, bounds.right - 1, bounds.top + 2, bounds.right - 1, bounds.bottom - 2);
  }
  CFontHandle font;
  if (status & FOCUSED) {
    if (status & ENABLED)
      dc.SetTextColor(::GetSysColor(COLOR_BTNTEXT));
    else
      dc.SetTextColor(RGB(255, 0, 0));
    font = hFontBold;
  } else {
    if (status & ENABLED)
      dc.SetTextColor(clrInactiveText);
    else
      dc.SetTextColor(RGB(255, 0, 0));
    font = hFontNormal;
  }
  if (status & HOT) {
    LOGFONT info;
    font.GetLogFont(&info);
    info.lfUnderline = true;
    CFont fontUL;
    fontUL.CreateFontIndirect(&info);
    dc.DrawText(fontUL, text, &bounds, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
  } else {
    dc.DrawText(font, text, &bounds, DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
  }
}

//==============================================================================

namespace {
const int MENU_ICON_SPACE_W = 3;
const int MENU_ICON_SPACE_H = 3;
const int MENU_ITEM_PADDING = 4;
const int MENU_TEXT_INDENT = 4;
const int MENU_TEXT_SPACE = 4;

//==============================================================================

inline SIZE ImageList_GetIconSize(IImageList* pImageList) {
  SIZE sz = {16, 16};
  if (pImageList) pImageList->GetIconSize((int*)&sz.cx, (int*)&sz.cy);
  return sz;
}
inline COLORREF BlendSysColor(int a, int b, int factor) { return BlendRGB(::GetSysColor(a), ::GetSysColor(b), factor); }

//==============================================================================
// subroutine for menu

static void DrawMenuText(DCHandle dc, RECT& rc, PCTSTR text, COLORREF color) {
  dc.SetTextColor(color);
  int nTab = -1;
  int count = ::lstrlen(text);
  for (int i = 0; i < count; i++) {
    if (text[i] == _T('\t')) {
      nTab = i;
      break;
    }
  }
  dc.DrawText(text, nTab, &rc, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
  if (nTab != -1) dc.DrawText(&text[nTab + 1], -1, &rc, DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
}
static void DrawMenuMark(DCHandle dc, bool selected, bool checked, bool disabled, const RECT& rc) {
  if (!checked) return;
  const int nColor = (disabled ? COLOR_BTNSHADOW : (selected ? COLOR_MENU : COLOR_MENUTEXT));
  const int w = rc.right - rc.left;
  const int h = rc.bottom - rc.top;
  RECT rcSource = {0, 0, w, h};

  // set colors
  const COLORREF clrBlack = RGB(0, 0, 0);
  const COLORREF clrWhite = RGB(255, 255, 255);
  COLORREF clrTextOld = dc.SetTextColor(clrBlack);
  COLORREF clrBkOld = dc.SetBkColor(clrWhite);

  // create source image
  CDC dcSource;
  CBitmap bmp;
  bmp.CreateCompatibleBitmap(dc, w, h);
  dcSource.CreateCompatibleDC(dc);
  HBITMAP hBmpOld = dcSource.SelectBitmap(bmp);
  dcSource.FillRect(&rcSource, nColor);

  // create mask
  CDC dcMask;
  dcMask.CreateCompatibleDC(dc);
  CBitmap bmpMask;
  bmpMask.CreateBitmap(w, h, 1, 1, NULL);
  HBITMAP hBmpOld1 = dcMask.SelectBitmap(bmpMask);
  dcMask.DrawFrameControl(&rcSource, DFC_MENU, DFCS_MENUCHECK);

  // draw the checkmark transparently
  const DWORD ROP_DSno = 0x00BB0226L;
  const DWORD ROP_DSa = 0x008800C6L;
  const DWORD ROP_DSo = 0x00EE0086L;
  const DWORD ROP_DSna = 0x00220326L;

  // draw checkmark - special case black and white colors
  COLORREF clrCheck = ::GetSysColor(nColor);
  if (clrCheck == clrWhite) {
    dc.BitBlt(rc.left, rc.top, w, h, dcMask, 0, 0, ROP_DSno);
    dc.BitBlt(rc.left, rc.top, w, h, dcSource, 0, 0, ROP_DSa);
  } else {
    if (clrCheck != clrBlack) dcSource.BitBlt(0, 0, w, h, dcMask, 0, 0, ROP_DSna);
    dc.BitBlt(rc.left, rc.top, w, h, dcMask, 0, 0, ROP_DSa);
    dc.BitBlt(rc.left, rc.top, w, h, dcSource, 0, 0, ROP_DSo);
  }
  // restore all
  dc.SetTextColor(clrTextOld);
  dc.SetBkColor(clrBkOld);
  dcSource.SelectBitmap(hBmpOld);
  dcMask.SelectBitmap(hBmpOld1);
}
static void DrawMenuIcon(DCHandle dc, bool selected, bool checked, bool disabled, IImageList* pImageList,
                         IMAGELISTDRAWPARAMS& params) {
  ASSERT(pImageList);
  SIZE size = ImageList_GetIconSize(pImageList);
  // center bitmap in caller's rectangle
  RECT rc = {params.x, params.y, params.x + size.cx, params.y + size.cy};

  // paint background
  if (checked) {
    RECT rcFrame = rc;
    ::InflateRect(&rcFrame, MENU_ICON_SPACE_W, MENU_ICON_SPACE_H);
    dc.DrawRect(rcFrame, ::GetSysColor(COLOR_HIGHLIGHT), BlendSysColor(COLOR_MENU, COLOR_HIGHLIGHT, 25));
  }
  if (selected) params.fStyle = ILD_SELECTED;
  VERIFY_HRESULT(pImageList->Draw(&params));
}
static void DrawShadowIcon(DCHandle dc, HIMAGELIST hImageList, POINT pt, int nImage) {
  HICON hIcon = ImageList_ExtractIcon(NULL, hImageList, nImage);
  Brush brush(BlendSysColor(COLOR_3DFACE, COLOR_WINDOWTEXT, 35));
  SIZE sz = {0, 0};
  dc.DrawState(pt, sz, hIcon, DSS_MONO, brush);
  ::DestroyIcon(hIcon);
}

//==============================================================================
// subroutine for tab

//==============================================================================

static CFont theMenuFont;
}  // namespace

//==============================================================================

void mew::theme::HandleSystemSettingChange() {
  NONCLIENTMETRICS metrics = {sizeof(NONCLIENTMETRICS)};
  ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);
  theMenuFont.CreateFontIndirect(&metrics.lfMenuFont);
}

SIZE mew::theme::MenuMeasureItem(PCWSTR wcsText, IImageList* pImageList, int nImage) {
  if (!theMenuFont) {
    HandleSystemSettingChange();
  }

  if (str::empty(wcsText)) {
    SIZE sz = {0, ::GetSystemMetrics(SM_CYMENU) / 2};
    return sz;
  } else {
    DC dc;
    dc.CreateCompatibleDC();
    SIZE sz = dc.GetTextExtent(theMenuFont, wcsText);
    SIZE ret;
    SIZE iconSize = ImageList_GetIconSize(pImageList);
    ret.cx = sz.cx + iconSize.cx + MENU_ICON_SPACE_W * 2 + MENU_TEXT_INDENT + MENU_TEXT_SPACE;
    ret.cy = math::max<int>(sz.cy + MENU_ITEM_PADDING, iconSize.cy + MENU_ICON_SPACE_H * 2);
    return ret;
  }
}

void mew::theme::MenuDrawItem(HDC hDC, const RECT& rcBounds, DWORD dwStatus, PCWSTR wcsText, IImageList* pImageList,
                              int nImage) {
  DCHandle dc(hDC);

  SIZE iconSize = ImageList_GetIconSize(pImageList);

  const int iconw = iconSize.cx + MENU_ICON_SPACE_W * 2;
  const bool selected = (dwStatus & SELECTED) != 0;
  const bool disabled = (dwStatus & ENABLED) == 0;
  const bool checked = (dwStatus & CHECKED) != 0;

  RECT rcIcon = {rcBounds.left, rcBounds.top, rcBounds.left + iconw, rcBounds.bottom};
  RECT rcText = {rcBounds.left + iconw, rcBounds.top, rcBounds.right, rcBounds.bottom};
  int menuTextColorIndex;
  if (selected) {
    dc.FillSolidRect(&rcBounds, GetSysColor(COLOR_HIGHLIGHT));
    menuTextColorIndex = COLOR_HIGHLIGHTTEXT;
  } else {
    dc.FillSolidRect(&rcBounds, GetSysColor(COLOR_MENU));
    menuTextColorIndex = COLOR_MENUTEXT;
  }
  if (str::empty(wcsText)) {  // separator
    int y = (rcBounds.top + rcBounds.bottom) / 2;
    dc.MoveTo(rcBounds.left, y);
    dc.LineTo(rcBounds.right, y);
  } else {
    rcText.left += MENU_TEXT_INDENT;
    rcText.right -= MENU_TEXT_SPACE;
    dc.SetBkMode(TRANSPARENT);
    COLORREF color = GetSysColor(disabled ? COLOR_GRAYTEXT : menuTextColorIndex);
    DrawMenuText(dc, rcText, wcsText, color);
    if (nImage >= 0) {
      IMAGELISTDRAWPARAMS params = {sizeof(IMAGELISTDRAWPARAMS)};
      params.i = nImage;
      params.hdcDst = dc;
      params.x = rcIcon.left + MENU_ICON_SPACE_W;
      params.y = (rcIcon.top + rcIcon.bottom - iconSize.cy) / 2;
      params.rgbFg = CLR_DEFAULT;
      params.rgbBk = CLR_NONE;
      if (pImageList) DrawMenuIcon(dc, selected, checked, disabled, pImageList, params);
    } else {
      rcIcon.left += MENU_ICON_SPACE_W;
      rcIcon.right = rcIcon.left + iconSize.cx;
      rcIcon.top = (rcIcon.top + rcIcon.bottom - iconSize.cy) / 2;
      rcIcon.bottom = rcIcon.top + iconSize.cy;
      DrawMenuMark(dc, selected, checked, disabled, rcIcon);
    }
  }
}

namespace {
static bool IsLuna() {
#ifdef SPI_GETFLATMENU
  BOOL bRetVal = FALSE;
  BOOL bRet = ::SystemParametersInfo(SPI_GETFLATMENU, 0, &bRetVal, 0);
  return (bRet && bRetVal);
#else
  return false;
#endif
}
}  // namespace

bool mew::theme::MenuDrawButton(NMTBCUSTOMDRAW* draw, HFONT hFont, bool bShowKeyboardCues, bool bIsMenuDropped) {
  DCHandle dc(draw->nmcd.hdc);
  HFONT hFontOld = NULL;
  if (hFont != NULL) hFontOld = dc.SelectFont(hFont);

  WTL::CToolBarCtrl tb = draw->nmcd.hdr.hwndFrom;

  TBBUTTONINFO tbi = {sizeof(TBBUTTONINFO)};
  TCHAR szText[MAX_PATH] = {0};
  tbi.dwMask = TBIF_TEXT | TBIF_STYLE | TBIF_STATE | TBIF_IMAGE;
  tbi.pszText = szText;
  tbi.cchText = sizeof(szText) / sizeof(TCHAR);
  tb.GetButtonInfo(draw->nmcd.dwItemSpec, &tbi);

  if (::lstrlen(szText) == 0) tb.GetButtonText(draw->nmcd.dwItemSpec, szText);

  // Get state information
  UINT uItemState = draw->nmcd.uItemState;
  bool selected = (uItemState & ODS_SELECTED) != 0;
  bool hotlight = (uItemState & ODS_HOTLIGHT) != 0;
  bool checked = (tbi.fsState & TBSTATE_CHECKED) != 0;
  bool pressed = (tbi.fsState & TBSTATE_PRESSED) != 0;
  bool disabled = (tbi.fsState & TBSTATE_ENABLED) == 0;
  RECT rcItem = draw->nmcd.rc;

  //
  bool bLuna = IsLuna();

  if (bLuna) {
    // Draw highlight
    if (disabled) {
    } else if (selected || hotlight || pressed || checked || bIsMenuDropped) {
      dc.FillRect(&rcItem, COLOR_HIGHLIGHT);
    }
  } else {
    // Draw classic
    if (disabled) {
    } else {
      if (pressed || checked || bIsMenuDropped) {
        dc.DrawEdge(&rcItem, BDR_SUNKENOUTER, BF_RECT);
      } else if (hotlight) {
        dc.DrawEdge(&rcItem, BDR_RAISEDINNER, BF_RECT);
      }
    }
  }

  // Draw image
  UINT uTextFlags = DT_CENTER;
  if (tbi.iImage >= 0) {
    // Get ImageList
    HIMAGELIST hImageList = NULL;
    if (hotlight) hImageList = tb.GetHotImageList();
    if (disabled) hImageList = tb.GetDisabledImageList();
    if (hImageList == NULL) hImageList = tb.GetImageList();
    // Draw icon
    if (hImageList != NULL) {
      int cxIcon, cyIcon;
      ImageList_GetIconSize(hImageList, &cxIcon, &cyIcon);
      SIZE sizeButton = {rcItem.right - rcItem.left, rcItem.bottom - rcItem.top};
      SIZE sizePadding = {(sizeButton.cx - cxIcon) / 2, (sizeButton.cy - cyIcon) / 2};
      POINT point = {rcItem.left + 4, rcItem.top + sizePadding.cy};
      // Draw the image
      if (disabled) {
        // Draw disabled icon (shadow, really)
        DrawShadowIcon(dc, hImageList, point, tbi.iImage);
      } else if (hotlight || selected) {
        if (!selected) {
          // Draw selected icon shadow
          point.x++;
          point.y++;
          DrawShadowIcon(dc, hImageList, point, tbi.iImage);
          point.x -= 2;
          point.y -= 2;
        }
        // Finally draw the icon above
        ImageList_Draw(hImageList, tbi.iImage, dc, point.x, point.y, ILD_TRANSPARENT);
      } else {
        // Draw normal icon
        ImageList_DrawEx(hImageList, tbi.iImage, dc, point.x, point.y, 0, 0, CLR_NONE, CLR_NONE, ILD_TRANSPARENT);
      }

      uTextFlags = DT_LEFT;
      rcItem.left += cxIcon + 8;
    }
  }

  // Draw text

  if (::lstrlen(szText) > 0) {
    COLORREF clrText;
    if (bLuna) {
      if (disabled)
        clrText = GetSysColor(COLOR_GRAYTEXT);
      else if (hotlight || bIsMenuDropped)
        clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
      else
        clrText = GetSysColor(COLOR_BTNTEXT);
    } else {
      if (disabled)
        clrText = GetSysColor(COLOR_GRAYTEXT);
      else
        clrText = GetSysColor(COLOR_BTNTEXT);
      if (pressed || checked || bIsMenuDropped) ::OffsetRect(&rcItem, 1, 1);
    }

    uTextFlags |= DT_SINGLELINE | DT_VCENTER;
    if (!bShowKeyboardCues) uTextFlags |= DT_HIDEPREFIX;

    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(clrText);
    dc.DrawText(szText, -1, &rcItem, uTextFlags);
  }

  if (hFont != NULL) dc.SelectFont(hFontOld);
  return true;
}

//==============================================================================
