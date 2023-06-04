// imagelist.cpp

#include "stdafx.h"
#include "drawing.hpp"
#include "private.h"

class __declspec(uuid("7C476BA2-02B1-48f4-8048-B24619DDC058")) ImageList;

using namespace mew::drawing;

//==============================================================================

namespace {
void CopyBits(void *dst, int dststride, const void *src, int srcstride, size_t bpp, size_t width, size_t height) {
  ASSERT(dst);
  ASSERT(src);
  ASSERT(width * bpp <= (size_t)mew::math::abs(srcstride));
  ASSERT(width * bpp <= (size_t)mew::math::abs(dststride));

  if (dststride >= 0 && dststride == srcstride) {
    memcpy(dst, src, srcstride * height);
  } else {
    BYTE *d = (BYTE *)dst;
    BYTE *s = (BYTE *)src;
    for (size_t i = 0; i < height; ++i) {
      memcpy(d, s, bpp * width);
      d += dststride;
      s += srcstride;
    }
  }
}
}  // namespace

class ImageList : public mew::Root<mew::implements<mew::drawing::IImageList2, IImageList> > {
 private:
  WTL::CImageList m_normal;
  WTL::CImageList m_disabled;
  WTL::CImageList m_hot;

 public:
  static HIMAGELIST LoadImageList(Gdiplus::Bitmap *image) {
    int w = image->GetWidth();
    int h = image->GetHeight();

    HIMAGELIST imagelist = ImageList_Create(h, h, ILC_COLOR32, w / h, 20);
    ImageList_SetBkColor(imagelist, CLR_NONE);

    if (image->GetPixelFormat() == PixelFormat32bppARGB) {
      Gdiplus::Rect rcLock(0, 0, w, h);
      Gdiplus::BitmapData bits;
      if (image->LockBits(&rcLock, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bits) == Gdiplus::Ok) {
        // TODO: ビットマップのコピーを減らすべし。
        // 現在、GDI+ Bitmap → DIBSection → ImageList内バッファ と複数回のコピーが発生している。
        // DIBSection をスキップできれば、コピーを減らせる。そのためには以下のどちらかの方法がある。
        // ・GDI+ Bitmapの内部データを直接参照するDIBitmapを作成する。
        // ・ImageList内のバッファに直接書き込む。
        const int bpp = 4;  // sizeof(pixel for PixelFormat32bppARGB)
        WTL::CBitmap bitmap;
        BITMAPINFO info = {0};
        info.bmiHeader.biSize = sizeof(info.bmiHeader);
        info.bmiHeader.biWidth = w;
        info.bmiHeader.biHeight = h;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        BYTE *dst = nullptr;
        int dststride = ((w * bpp + 3) & ~3);
        bitmap.CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, (void **)&dst, nullptr, 0);
        CopyBits(dst + dststride * (h - 1), -dststride, bits.Scan0, bits.Stride, bpp, w, h);
        image->UnlockBits(&bits);
        ImageList_Add(imagelist, bitmap, nullptr);
        return imagelist;
      }
    }

    // アルファなしビットマップ
    WTL::CBitmap bitmap;
    if (image->GetHBITMAP(SysColor(COLOR_3DFACE), &bitmap.m_hBitmap) != Gdiplus::Ok) {
      return nullptr;
    }
    ImageList_Add(imagelist, bitmap, nullptr);
    return imagelist;
  }
  static HIMAGELIST LoadImageList(PCWSTR filename) {
    CAutoPtr<Gdiplus::Bitmap> image(new Gdiplus::Bitmap(filename));
    if (!image || image->GetLastStatus() != Gdiplus::Ok) {
      return nullptr;
    }
    return LoadImageList(image);
  }
  static HIMAGELIST LoadImageList(IStream *stream) {
    CAutoPtr<Gdiplus::Bitmap> image(new Gdiplus::Bitmap(stream));
    if (!image || image->GetLastStatus() != Gdiplus::Ok) {
      return nullptr;
    }
    return LoadImageList(image);
  }
  static HIMAGELIST LoadImageListOfPostfix(PCWSTR path, PCWSTR postfix, PCWSTR extension) {
    WCHAR buffer[MAX_PATH];
    mew::str::copy(buffer, path);
    ::PathRemoveExtension(buffer);
    mew::str::append(buffer, postfix);
    mew::str::append(buffer, extension);
    return LoadImageList(buffer);
  }
  ImageList(HIMAGELIST hNormal = nullptr, HIMAGELIST hDisabled = nullptr, HIMAGELIST hHot = nullptr)
      : m_normal(hNormal), m_disabled(hDisabled), m_hot(hHot) {}
  void __init__(IUnknown *arg) {
    if (mew::string filename = mew::cast(arg)) {
      PCWSTR path = filename.str();
      m_normal = LoadImageList(path);
      if (!m_normal) {
        throw mew::exceptions::IOError(mew::string::load(IDS_ERR_IMAGELIST, filename), STG_E_FILENOTFOUND);
      }
      PCWSTR extension = ::PathFindExtension(path);
      m_disabled = LoadImageListOfPostfix(path, L"-disable", extension);
      m_hot = LoadImageListOfPostfix(path, L"-hot", extension);
    } else if (arg) {
      mew::ref<IStream> stream(__uuidof(mew::io::Reader), arg);
      m_normal = LoadImageList(stream);
      if (!m_normal) {
        throw mew::exceptions::IOError(mew::string::load(IDS_ERR_IMAGELIST, L"[stream]"), STG_E_FILENOTFOUND);
      }
      // m_disabled = null;
      // m_hot = null;
    } else {
      // empty imagelist
    }
  }
  void Dispose() throw() {
    if (m_normal) {
      m_normal.Destroy();
    }
    if (m_disabled) {
      m_disabled.Destroy();
    }
    if (m_hot) {
      m_hot.Destroy();
    }
  }

  inline static HRESULT call(int *pResult, int result) {
    if (pResult) *pResult = result;
    return result >= 0 ? S_OK : AtlHresultFromLastError();
  }
  inline static HRESULT call(COLORREF *pResult, COLORREF result) {
    if (pResult) *pResult = result;
    return S_OK;
  }
  inline static HRESULT call(HICON *pResult, HICON result) {
    if (pResult) *pResult = result;
    return result ? S_OK : AtlHresultFromLastError();
  }
  inline static HRESULT call(BOOL result) {
    ASSERT(result);
    return result ? S_OK : AtlHresultFromLastError();
  }
  static HRESULT FromHIMAGELIST(HIMAGELIST hNormal, HIMAGELIST hDisabled, HIMAGELIST hHot, REFIID iid, void **ppv) {
    if (!ppv) {return E_POINTER;}
    return mew::objnew<ImageList>(hNormal, hDisabled, hHot)->QueryInterface(iid, ppv);
  }

 public:  // IImageList
  HRESULT __stdcall Add(HBITMAP hbmImage, HBITMAP hbmMask, int *pi) { return call(pi, m_normal.Add(hbmImage, hbmMask)); }
  HRESULT __stdcall ReplaceIcon(int i, HICON hicon, int *pi) { return call(pi, m_normal.ReplaceIcon(i, hicon)); }
  HRESULT __stdcall SetOverlayImage(int iImage, int iOverlay) { return call(m_normal.SetOverlayImage(iImage, iOverlay)); }
  HRESULT __stdcall Replace(int i, HBITMAP hbmImage, HBITMAP hbmMask) { return call(m_normal.Replace(i, hbmImage, hbmMask)); }
  HRESULT __stdcall AddMasked(HBITMAP hbmImage, COLORREF crMask, int *pi) {
    return call(pi, ImageList_AddMasked(m_normal, hbmImage, crMask));
  }
  HRESULT __stdcall Draw(IMAGELISTDRAWPARAMS *pimldp) {
    if (!pimldp) {return E_POINTER;}
    IMAGELISTDRAWPARAMS params = *pimldp;
    params.himl = m_normal;
    return call(ImageList_DrawIndirect(&params));
  }
  HRESULT __stdcall Remove(int i) { return call(m_normal.Remove(i)); }
  HRESULT __stdcall GetIcon(int i, UINT flags, HICON *picon) { return call(picon, m_normal.GetIcon(i, flags)); }
  HRESULT __stdcall GetImageInfo(int i, IMAGEINFO *pImageInfo) { return call(m_normal.GetImageInfo(i, pImageInfo)); }
  HRESULT __stdcall Copy(int iDst, IUnknown *punkSrc, int iSrc, UINT uFlags) {
    return E_NOTIMPL;
    // return call(ImageList_Copy(m_normal, iDst, punkSrc, iSrc, uFlags));
  }
  HRESULT __stdcall Merge(int i1, IUnknown *punk2, int i2, int dx, int dy, REFIID iid, PVOID *ppv) {
    return E_NOTIMPL;
    // return call(ImageList_Merge(m_normal, i1, punk2, i2, dx, dy, iid, ppv));
  }
  HRESULT __stdcall Clone(REFIID iid, PVOID *ppv) {
    return FromHIMAGELIST(m_normal.Duplicate(), m_disabled.Duplicate(), m_hot.Duplicate(), iid, ppv);
  }
  HRESULT __stdcall GetImageRect(int i, RECT *prc) {
    IMAGEINFO info;
    HRESULT hr = GetImageInfo(i, &info);
    if FAILED (hr) {return hr;}
    if (prc) {*prc = info.rcImage;}
    return S_OK;
  }
  HRESULT __stdcall GetIconSize(int *cx, int *cy) { return call(ImageList_GetIconSize(m_normal, cx, cy)); }
  HRESULT __stdcall SetIconSize(int cx, int cy) {
    if (m_normal) {return call(m_normal.SetIconSize(cx, cy));}
    m_normal.Create(cx, cy, ILC_COLOR32, 10, 10);
    return S_OK;
  }
  HRESULT __stdcall GetImageCount(int *pi) { return call(pi, ImageList_GetImageCount(m_normal)); }
  HRESULT __stdcall SetImageCount(UINT uNewCount) { return call(ImageList_SetImageCount(m_normal, uNewCount)); }
  HRESULT __stdcall SetBkColor(COLORREF clrBk, COLORREF *pclr) { return call(pclr, ImageList_SetBkColor(m_normal, clrBk)); }
  HRESULT __stdcall GetBkColor(COLORREF *pclr) { return call(ImageList_GetBkColor(m_normal)); }
  HRESULT __stdcall BeginDrag(int iTrack, int dxHotspot, int dyHotspot) {
    return call(ImageList_BeginDrag(m_normal, iTrack, dxHotspot, dyHotspot));
  }
  HRESULT __stdcall EndDrag(void) {
    ImageList_EndDrag();
    return S_OK;
  }
  HRESULT __stdcall DragEnter(HWND hwndLock, int x, int y) { return call(ImageList_DragEnter(hwndLock, x, y)); }
  HRESULT __stdcall DragLeave(HWND hwndLock) { return call(ImageList_DragLeave(hwndLock)); }
  HRESULT __stdcall DragMove(int x, int y) { return call(ImageList_DragMove(x, y)); }
  HRESULT __stdcall SetDragCursorImage(IUnknown *punk, int iDrag, int dxHotspot, int dyHotspot) {
    return call(ImageList_SetDragCursorImage(m_normal, iDrag, dxHotspot, dyHotspot));
  }
  HRESULT __stdcall DragShowNolock(BOOL fShow) { return call(ImageList_DragShowNolock(fShow)); }
  HRESULT __stdcall GetDragImage(POINT *ppt, POINT *pptHotspot, REFIID iid, PVOID *ppv) {
    return FromHIMAGELIST(ImageList_GetDragImage(ppt, pptHotspot), nullptr, nullptr, iid, ppv);
  }
  HRESULT __stdcall GetItemFlags(int i, DWORD *dwFlags) { return E_NOTIMPL; }
  HRESULT __stdcall GetOverlayImage(int iOverlay, int *piIndex) { return E_NOTIMPL; }

 public:
  HIMAGELIST get_Normal() throw() { return m_normal; }
  HIMAGELIST get_Disabled() throw() { return m_disabled; }
  HIMAGELIST get_Hot() throw() { return m_hot; }
};

//==============================================================================

AVESTA_EXPORT(ImageList)

/*
void CreateImageListInstance(REFINTF ppInterface, IUnknown* arg) throw(...)
{
        ref<IStream> stream;
        ::CreateStreamOnHGlobal(::GlobalAlloc(GMEM_MOVEABLE, 256), TRUE, &stream);
        HIMAGELIST hImageList = ::ImageList_Create(1, 1, ILC_COLOR24 | ILC_MASK, 10, 20);
        ASSERT(hImageList);
        VERIFY_HRESULT( ImageList_WriteEx(hImageList, ILP_NORMAL, stream) );
        ::ImageList_Destroy(hImageList);
        LARGE_INTEGER zero = { 0 };
        VERIFY_HRESULT( stream->Seek(zero, STREAM_SEEK_SET, null) );
        VERIFY_HRESULT( ImageList_ReadEx(ILP_NORMAL, stream, ppInterface.iid, ppInterface.pp) );
        stream.clear();
}

AVESTA_EXPORT_FUNC( ImageList )
*/