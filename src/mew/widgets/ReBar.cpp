// ReBar.cpp

#include "stdafx.h"
#include "../private.h"
#include "WindowImpl.h"

#include "impl/WTLControls.hpp"
#include "theme.hpp"

struct ReBarBandInfo : REBARBANDINFO {
  ReBarBandInfo(DWORD rbbim) {
    memset(this, 0, sizeof(REBARBANDINFO));
    cbSize = sizeof(REBARBANDINFO);
    fMask = rbbim;
  }

  __declspec(property(get = get_Visible, put = put_Visible)) bool Visible;
  bool get_Visible() const { return (fStyle & RBBS_HIDDEN) == 0; }
  void put_Visible(bool value) {
    if (value)
      fStyle &= ~RBBS_HIDDEN;
    else
      fStyle |= RBBS_HIDDEN;
  }

  __declspec(property(get = get_Break)) bool Break;
  bool get_Break() const { return (fStyle & RBBS_BREAK) != 0; }

  __declspec(property(get = get_Locked, put = set_Locked)) bool Locked;
  bool get_Locked() const { return (fStyle & RBBS_NOGRIPPER) != 0; }
  void set_Locked(bool value) {
    if (value) {  // locked, without grip
      fStyle &= ~RBBS_GRIPPERALWAYS;
      fStyle |= RBBS_NOGRIPPER;
    } else {  // unlocked, with grip
      fStyle |= RBBS_GRIPPERALWAYS;
      fStyle &= ~RBBS_NOGRIPPER;
    }
  }
};

template <class TBase>
class __declspec(novtable) WallPaperImpl : public TBase {
 protected:
  string m_WallPaperFile;
  UINT32 m_WallPaperAlign;
  CAutoPtr<Gdiplus::Image> m_BackgroundImage;

 protected:
  WallPaperImpl() : m_WallPaperAlign(DirMaskWE | DirMaskNS) {}

 public:  // IWallPaper
  string get_WallPaperFile() { return m_WallPaperFile; }
  UINT32 get_WallPaperAlign() { return m_WallPaperAlign; }
  void set_WallPaperFile(string value) {
    if (m_WallPaperFile == value) return;
    if (!value) {
      m_WallPaperFile.clear();
      m_BackgroundImage.Free();
    } else {
      CAutoPtr<Gdiplus::Image> image(Gdiplus::Image::FromFile(value.str()));
      if (!image || image->GetLastStatus() != Gdiplus::Ok) {
        TRACE(L"error: WallPaperImpl.背景画像が読み込めない");
        return;
      }
      m_WallPaperFile = value;
      m_BackgroundImage = image;
    }
    Update();
  }
  void set_WallPaperAlign(UINT32 value) {
    if (m_WallPaperAlign == value) return;
    m_WallPaperAlign = value;
    Update();
  }

 private:
  static int GetHorzPos(UINT32 align, const Rect& rc, const Size& sz) {
    switch (align & DirMaskWE) {
      case 0:  // center
        return (rc.left + rc.right - sz.w) / 2;
      case DirWest:
        return rc.left;
      case DirEast:
        return rc.right - sz.w;
      default:  // tile
        return 0;
    }
  }
  static int GetVertPos(UINT32 align, const Rect& rc, const Size& sz) {
    switch (align & DirMaskNS) {
      case 0:  // center
        return (rc.top + rc.bottom - sz.h) / 2;
        break;
      case DirNorth:
        return rc.top;
        break;
      case DirSouth:
        return rc.bottom - sz.h;
        break;
      default:  // tile
        return 0;
    }
  }

 public:  // operation for implement
  void DrawBackground(HDC hDC, const Rect& rc, COLORREF clrBkgnd) const {
    CDCHandle dc(hDC);
    if (m_BackgroundImage) {
      Size sz(m_BackgroundImage->GetWidth(), m_BackgroundImage->GetHeight());
      Point pt(GetHorzPos(m_WallPaperAlign, rc, sz), GetVertPos(m_WallPaperAlign, rc, sz));
      Gdiplus::Graphics g(dc);
      if ((m_WallPaperAlign & DirMaskNS) == (DirMaskNS)) {
        if ((m_WallPaperAlign & DirMaskWE) == (DirMaskWE)) {  // tile both
          for (int x = rc.left; x < rc.right; x += sz.w)
            for (int y = rc.top; y < rc.bottom; y += sz.h) g.DrawImage(m_BackgroundImage, x, y);
        } else {  // tile vert
          dc.FillSolidRect(&rc, clrBkgnd);
          for (int y = rc.top; y < rc.bottom; y += sz.h) g.DrawImage(m_BackgroundImage, pt.x, y);
        }
      } else if ((m_WallPaperAlign & DirMaskWE) == (DirMaskWE)) {  // tile horz
        dc.FillSolidRect(&rc, clrBkgnd);
        for (int x = rc.left; x < rc.right; x += sz.w) g.DrawImage(m_BackgroundImage, x, pt.y);
      } else {  // non-tiling
        dc.FillSolidRect(&rc, clrBkgnd);
        g.DrawImage(m_BackgroundImage, pt.x, pt.y);
      }
    } else {
      dc.FillSolidRect(&rc, clrBkgnd);
    }
  }
};

namespace mew {
namespace ui {

class ReBar
    : public WallPaperImpl<WindowImpl<CWindowImplEx<ReBar, WTLEX::CReBar<CReBarCtrlT<WindowImplBase> > >,
                                      implements<IList, IWindow, ISignal, IDisposable, IWallPaper, IPersistMessage> > > {
 private:
  static const int BAND_WIDTH_MIN = 0;
  static const int BAND_HEIGHT_MIN = 0;
  bool m_locked;

 public:
  bool SupportsEvent(EventCode code) const throw() {
    if (__super::SupportsEvent(code)) return true;
    switch (code) {
        // case EventItemFocus:
        //   return true;
      default:
        return false;
    }
  }
  void DoCreate(CWindowEx parent) {
    m_locked = false;
    RECT rc = {0, 0, 40, 40};
    __super::DoCreate(
        parent, &rc, DirNorth,
        WS_CONTROL | CCS_NOPARENTALIGN | CCS_NOMOVEY | CCS_NODIVIDER | RBS_DBLCLKTOGGLE | RBS_VARHEIGHT | RBS_BANDBORDERS);
    REBARINFO band = {sizeof(REBARINFO)};
    SetBarInfo(&band);
    SetFont(AtlGetDefaultGuiFont());
  }
  void set_Dock(Direction value) {
    DWORD style = 0;
    switch (value) {
      case DirNone:
      case DirCenter:
      case DirNorth:
      case DirSouth:
        style = 0;
        break;
      case DirWest:
      case DirEast:
        style = CCS_VERT;
        break;
      default:
        TRESPASS();
    }
    ModifyStyle(CCS_VERT, style);
    __super::set_Dock(value);
  }
  static BOOL CALLBACK InvalidateProc(HWND hWnd, LPARAM) {
    ::InvalidateRect(hWnd, NULL, true);
    return true;
  }
  void Update(bool sync = false) {
    Invalidate();
    EnumChildWindows(m_hWnd, InvalidateProc, 0);
    __super::Update(sync);
  }

 public:  // msg map
  DECLARE_WND_SUPERCLASS(_T("mew.ui.ReBar"), __super::GetWndClassName());

  BEGIN_MSG_MAP_(HandleWindowMessage)
  MSG_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
  MSG_HANDLER(WM_SIZE, OnSize)
  MSG_HANDLER(WM_CONTEXTMENU, OnContextMenu)
  MSG_HANDLER(WM_UPDATEUISTATE, OnUpdateUIState)
  MESSAGE_HANDLER(WM_PARENTNOTIFY, OnParentNotify)
  REFLECTED_NOTIFY_CODE_HANDLER(RBN_HEIGHTCHANGE, OnHeightChange)
  CHAIN_MSG_MAP_TO(__super::HandleWindowMessage)
  END_MSG_MAP()

  HRESULT Send(message msg) {
    if (!m_hWnd) return E_FAIL;
    return __super::Send(msg);
  }
  HRESULT OnChildResizeDefault(message msg) {
    ref<IWindow> from = msg["from"];
    if (!from) return E_FAIL;
    int index = FindBand(from->Handle);
    if (index >= 0) {
      Size size = msg["size"];
      ReBarBandInfo band(RBBIM_CHILDSIZE | RBBIM_IDEALSIZE);
      band.cxIdeal = size.w;
      band.cxMinChild = math::max(size.w, BAND_WIDTH_MIN);
      band.cyMinChild = math::max(size.h, BAND_HEIGHT_MIN);
      SetBandInfo(index, &band);
      UpdateParent();
    }
    return S_OK;
  }
  LRESULT OnParentNotify(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    UINT uMsg = LOWORD(wParam);
    switch (uMsg) {
      case WM_CREATE:
      case WM_DESTROY:
        if (::GetParent((HWND)lParam) == m_hWnd) HandleChildCreate(uMsg == WM_CREATE, (HWND)lParam);
        break;
    }
    bHandled = false;
    return 0;
  }
  void HandleChildCreate(bool create, CWindowEx w) {
    ref<IWindow> p;
    if FAILED (QueryInterfaceInWindow(w, &p)) return;
    if (create) {
      p->Connect(EventResizeDefault, function(this, &ReBar::OnChildResizeDefault));
      Size size = p->DefaultSize;
      ReBarBandInfo band(RBBIM_STYLE | RBBIM_CHILD | RBBIM_IDEALSIZE | RBBIM_CHILDSIZE | RBBIM_SIZE);
      band.fStyle = RBBS_GRIPPERALWAYS | RBBS_BREAK | (m_locked ? RBBS_NOGRIPPER : 0);
      band.hwndChild = w;
      band.cxIdeal = size.w;
      band.cxMinChild = 0;
      band.cyMinChild = math::max(size.h, BAND_HEIGHT_MIN);
      band.cx = math::max(size.w, BAND_WIDTH_MIN);
      InsertBand(-1, &band);
    } else {
      p->Disconnect(0, 0, OID);
      int index = FindBand(w);
      if (index >= 0) DeleteBand(index);
    }
  }
  LRESULT OnHeightChange(int, LPNMHDR pnmh, BOOL&) {
    UpdateParent();
    return 0;
  }
  bool OnUpdateUIState(UINT, WPARAM, LPARAM, LRESULT&) {
    bool change = false;
    ReBarBandInfo band(RBBIM_CHILD | RBBIM_STYLE);
    const int count = GetBandCount();
    for (int i = 0; i < count; ++i) {
      if (!GetBandInfo(i, &band)) continue;
      bool visible = afx::GetVisible(band.hwndChild);
      if (visible != band.Visible) {
        change = true;
        ShowBand(i, visible);
      }
    }
    if (change) UpdateParent();
    return true;
  }
  bool OnEraseBkgnd(UINT, WPARAM wParam, LPARAM, LRESULT& lResult) {
    if (!m_BackgroundImage) return false;
    __super::DrawBackground((HDC)wParam, ClientArea, ::GetSysColor(COLOR_3DFACE));
    lResult = 1;
    return true;
  }
  bool OnSize(UINT, WPARAM, LPARAM, LRESULT&) {
    Update();
    return false;
  }
  bool OnContextMenu(UINT, WPARAM, LPARAM lParam, LRESULT&) {
    int count = GetBandCount();
    if (count == 0) return true;
    Point ptScreen(GET_XY_LPARAM(lParam));
    if (ptScreen.x < 0 || ptScreen.y < 0) {  // by menu-key
      RECT rc;
      GetWindowRect(&rc);
      ptScreen.assign(rc.left, rc.bottom);
    }
    CMenu popup;
    popup.CreatePopupMenu();
    for (int i = 0; i < count; ++i) {
      ReBarBandInfo band(RBBIM_STYLE | RBBIM_CHILD);
      GetBandInfo(i, &band);
      TCHAR name[MAX_PATH];
      ::GetWindowText(band.hwndChild, name, MAX_PATH);
      popup.AppendMenu(MF_STRING | (band.Visible ? MF_CHECKED : 0), i + 1, name);
    }
    const UINT ID_LOCKED = 1000;
    if (count > 0) popup.AppendMenu(MF_SEPARATOR);
    popup.AppendMenu(MF_STRING | (m_locked ? MF_CHECKED : 0), ID_LOCKED, _T("ツールバーを固定する(&B)"));
    int cmd = popup.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RETURNCMD, ptScreen.x, ptScreen.y, m_hWnd);
    switch (cmd) {
      case ID_LOCKED:
        m_locked = !m_locked;
        LockBands(m_locked);
        break;
      default:
        if (0 < cmd && cmd <= count) {
          int i = cmd - 1;
          ReBarBandInfo band(RBBIM_STYLE | RBBIM_CHILD);
          GetBandInfo(i, &band);
          afx::SetVisible(band.hwndChild, !band.Visible);
        }
        break;
    }
    return true;
  }

 public:  // IList
  size_t get_Count() { return GetBandCount(); }
  HRESULT GetAt(REFINTF ppObj, size_t index) {
    ReBarBandInfo band(RBBIM_CHILD);
    if (!GetBandInfo(index, &band)) return E_INVALIDARG;
    return QueryInterfaceInWindow(band.hwndChild, ppObj);
  }
  HRESULT GetContents(REFINTF pp, Status status) {
    if (pp.iid == __uuidof(IEnumUnknown)) {
      ReBarBandInfo band(RBBIM_STYLE | RBBIM_CHILD);
      using EnumWindow = Enumerator<IWindow>;
      ref<EnumWindow> e;
      e.attach(new EnumWindow());
      for (int i = 0; GetBandInfo(i, &band); i++) {
        switch (status) {
          case StatusNone:
            break;
          case SELECTED:
            if (!band.Visible) continue;
            break;
          case UNSELECTED:
            if (band.Visible) continue;
            break;
          default:
            TRESPASS();
        }
        ref<IWindow> w;
        if SUCCEEDED (QueryInterfaceInWindow(band.hwndChild, &w)) e->push_back(w);
      }
      return e.copyto(pp);
    }
    TRESPASS_DBG(return E_NOTIMPL);
  }
  HRESULT GetStatus(IndexOr<IUnknown> item, DWORD* status, size_t* index = null) { TRESPASS_DBG(return E_NOTIMPL); }
  HRESULT SetStatus(IndexOr<IUnknown> item, Status status = SELECTED, bool unique = false) { TRESPASS_DBG(return E_NOTIMPL); }

 public:  // IPersistMessage
  void LoadFromMessage(const message& msg) {
    m_locked = msg["fixed"];
    if (message archive = msg["bands"]) {
      // すでに追加されたバンドを並び替えるよりも、新しく追加したほうが楽なので。
      using Bands = std::vector<ReBarBandInfo>;
      Bands bands;
      ReBarBandInfo band(RBBIM_SIZE | RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE);
      for (int i = 0; GetBandInfo(i, &band); i++) {
        bands.push_back(band);
      }
      size_t size = bands.size();
      for (size_t i = 0; i < size; i++) {
        DeleteBand(0);
        if (message msg_ = archive[i]) {
          string name = msg_["name"];
          if (!name) break;
          UINT32 width = msg_["width"] | BAND_WIDTH_MIN;
          bool linebreak = msg_["linebreak"] | true;
          bool visible = msg_["visible"] | true;
          for (size_t j = 0; j < size; j++) {
            TCHAR wndName[MAX_PATH];
            ::GetWindowText(bands[j].hwndChild, wndName, MAX_PATH);
            if (name == wndName) {
              bands[j].cx = width;
              bands[j].fStyle = RBBS_GRIPPERALWAYS | (linebreak ? RBBS_BREAK : 0) | (visible ? 0 : RBBS_HIDDEN);
              if (i != j) std::swap(bands[i], bands[j]);
              break;
            }
          }
        }
      }
      ASSERT(GetBandCount() == 0);
      for (size_t i = 0; i < size; ++i) {
        InsertBand(i, &bands[i]);
        ref<IWindow> w;
        if SUCCEEDED (QueryInterfaceInWindow(bands[i].hwndChild, &w)) w->Visible = bands[i].Visible;
      }
      if (m_locked) {  // バンドがひとつしかない場合にRBBS_NOGRIPPERを設定すると、自動的にRBBS_GRIPPERALWAYSも解除されてしまう。
        // これを防ぐため、InsertBand()とSetBandInfo(~RBBS_NOGRIPPER)を別々に行う。
        LockBands(m_locked);
      }
    }
    this->WallPaperFile = msg["bkfile"] | string();
    ;
    this->WallPaperAlign = msg["bkalign"] | (DirMaskNS | DirMaskWE);
  }
  message SaveToMessage() {
    message msg;
    msg["fixed"] = m_locked;
    message archive;
    ReBarBandInfo band(RBBIM_SIZE | RBBIM_STYLE | RBBIM_CHILD);
    for (int i = 0; GetBandInfo(i, &band); i++) {
      TCHAR name[MAX_PATH];
      ::GetWindowText(band.hwndChild, name, MAX_PATH);
      message msg_;
      msg_["name"] = name;
      msg_["width"] = (UINT32)band.cx;
      msg_["linebreak"] = band.Break;
      msg_["visible"] = band.Visible;
      archive[i] = msg_;
    }
    msg["bands"] = archive;
    msg["bkfile"] = this->WallPaperFile;
    msg["bkalign"] = this->WallPaperAlign;
    return msg;
  }

 private:
  void LockBands(bool locked) {
    SetRedraw(false);
    // スタイルの変更だけでは、左側の隙間を調整してくれない。
    // そのため、いったん削除し、その後で追加しなおす。
    int count = GetBandCount();
    using Bands = std::vector<ReBarBandInfo>;
    Bands bands;
    for (int i = 0; i < count; i++) {
      ReBarBandInfo band(RBBIM_SIZE | RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE);
      GetBandInfo(0, &band);
      bands.push_back(band);
      DeleteBand(0);
    }
    //
    ASSERT(GetBandCount() == 0);
    for (int i = 0; i < count; i++) {
      // Bandを追加 or 削除すると、ウィンドウの表示も勝手に表示される。
      // いったん表示状態で追加し、その後、ウィンドウを再び隠すことで対処する。
      ReBarBandInfo& band = bands[i];
      bool visible = band.Visible;
      band.Visible = true;
      band.Locked = locked;
      InsertBand(i, &band);
      if (!visible) afx::SetVisible(band.hwndChild, false);
    }
    SetRedraw(true);
  }
};

}  // namespace ui
}  // namespace mew

AVESTA_EXPORT(ReBar)
