// dragdrop.cpp

#include "stdafx.h"
#include <atlcoll.h>
#include "private.h"
#include "io.hpp"
#include "shell.hpp"

using namespace mew::io;

#ifdef _DEBUG
#define TRACE_FORMAT(fmt, cf) \
  if (fmt->cfFormat == cf)    \
    TRACE(L#cf);              \
  else
#define TRACE_FORMATS(fmt)          \
  TRACE_FORMAT(fmt, CF_TEXT)        \
  TRACE_FORMAT(fmt, CF_UNICODETEXT) \
  TRACE_FORMAT(fmt, CF_INETURLA)    \
  TRACE_FORMAT(fmt, CF_INETURLW)    \
  TRACE_FORMAT(fmt, CF_SHELLIDLIST) \
  TRACE_FORMAT(fmt, CF_HDROP)       \
  TRACE(_T("UnknownFormat $1"), fmt->cfFormat);
#else
#define TRACE_FORMATS(fmt)
#endif

namespace {
template <class T>
class GlobalSrc {
 private:
  HGLOBAL m_hGlobal;
  T* m_p;

 public:
  GlobalSrc(HGLOBAL hGlobal) : m_hGlobal(hGlobal) {
    ASSERT(m_hGlobal);
    m_p = (T*)::GlobalLock(m_hGlobal);
  }
  ~GlobalSrc() {
    ASSERT(m_hGlobal);
    ::GlobalUnlock(m_hGlobal);
  }
  operator T*() { return m_p; }

 private:
  GlobalSrc(const GlobalSrc&);
  GlobalSrc& operator=(const GlobalSrc&);
};

template <class T>
class GlobalDst {
 private:
  HGLOBAL m_hGlobal;
  T* m_p;

 public:
  GlobalDst(size_t size) {
    m_hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, size);
    m_p = (T*)::GlobalLock(m_hGlobal);
  }
  ~GlobalDst() { ASSERT(!m_hGlobal); }
  operator T*() { return m_p; }
  HGLOBAL Detach() {
    HGLOBAL tmp = m_hGlobal;
    if (m_hGlobal) {
      ::GlobalUnlock(m_hGlobal);
      m_hGlobal = nullptr;
    }
    return tmp;
  }

 private:
  GlobalDst(const GlobalDst&);
  GlobalDst& operator=(const GlobalDst&);
};

// const CLIPFORMAT CF_TEXT;
// const CLIPFORMAT CF_UNICODETEXT;
// const CLIPFORMAT CF_HDROP;
const CLIPFORMAT CF_INETURLA = (CLIPFORMAT)::RegisterClipboardFormat(CFSTR_INETURLA);
const CLIPFORMAT CF_INETURLW = (CLIPFORMAT)::RegisterClipboardFormat(CFSTR_INETURLW);
const CLIPFORMAT CF_SHELLIDLIST = (CLIPFORMAT)::RegisterClipboardFormat(CFSTR_SHELLIDLIST);
#ifdef UNICODE
const CLIPFORMAT CF_INETURL = CF_INETURLW;
#else
const CLIPFORMAT CF_INETURL = CF_INETURLA;
#endif

// tymed の変換は行うが、データ形式の変換は行わない
static HRESULT DuplicateStgMedium(const STGMEDIUM* src, STGMEDIUM* dst, const FORMATETC* fmt, DWORD tymed = 0) {
  HRESULT hr;
  if (tymed == 0 || (tymed & src->tymed) != 0) {
    tymed = src->tymed;
  }
  memset(dst, 0, sizeof(STGMEDIUM));
  if (src->tymed == tymed) {
    switch (src->tymed) {
      case TYMED_NULL:
        break;
      case TYMED_GDI:
      case TYMED_MFPICT:
      case TYMED_ENHMF:
      case TYMED_HGLOBAL:
      case TYMED_FILE:
      case TYMED_ISTREAM:
        // どれもポインタなので、代入後のバイナリ値は等しい。とりあえず hGlobal として受け取っておく
        dst->hGlobal = (HGLOBAL)OleDuplicateData(src->hGlobal, fmt->cfFormat, 0);
        if (!dst->hGlobal) return E_FAIL;
        break;
      case TYMED_ISTORAGE:
      default:
        return OLE_E_CANTCONVERT;
    }
  } else {
    switch (src->tymed) {
      case TYMED_HGLOBAL:
        switch (tymed) {
          case TYMED_ISTREAM:  // hGlobal to IStream
            if FAILED (hr = CreateStreamOnHGlobal(src->hGlobal, false, &dst->pstm)) return hr;
            break;
          default:
            return OLE_E_CANTCONVERT;
        }
        break;
      case TYMED_ISTREAM:
        switch (tymed) {
          case TYMED_HGLOBAL:  // IStream to hGlobal
          {
            mew::Stream stream;
            dst->hGlobal = StreamCreateOnHGlobal(&stream, 0, false);
            LARGE_INTEGER zero = {0, 0};
            src->pstm->Seek(zero, STREAM_SEEK_SET, NULL);
            const int bufsize = 1024;
            BYTE buffer[bufsize];
            ULONG done;
            while (SUCCEEDED(src->pstm->Read(buffer, bufsize, &done)) && done > 0) {
              stream.write(buffer, done);
            }
            break;
          }
          default:
            return OLE_E_CANTCONVERT;
        }
        break;
      default:
        return OLE_E_CANTCONVERT;
    }
  }
  if ((dst->pUnkForRelease = src->pUnkForRelease) != nullptr) {
    dst->pUnkForRelease->AddRef();
  }
  dst->tymed = tymed;
  return S_OK;
}

// 分割しないと循環参照が生じるのかも？
// 一緒にしてしまうと、アプリケーション終了時に固まることがある。
class DropSource : public mew::Root<mew::implements<IDropSource>, mew::mixin<mew::StaticLife> > {
 public:  // IDropSource
  STDMETHODIMP QueryContinueDrag(BOOL bEscapePressed, DWORD mk) {
    // 「２つ以上のマウスボタンが押されていたら」をもっとスマートに判定できないか？
    if (bEscapePressed || ((mk & MK_LBUTTON) && (mk & (MK_MBUTTON | MK_RBUTTON))) ||
        ((mk & MK_MBUTTON) && (mk & (MK_LBUTTON | MK_RBUTTON))) || ((mk & MK_RBUTTON) && (mk & (MK_LBUTTON | MK_MBUTTON)))) {
      return DRAGDROP_S_CANCEL;
    }
    if (!(mk & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON))) {
      return DRAGDROP_S_DROP;
    }
    return S_OK;
  }
  STDMETHODIMP GiveFeedback(DWORD dwEffect) { return DRAGDROP_S_USEDEFAULTCURSORS; }
};
}  // namespace

namespace mew {
namespace io {

//==============================================================================

class DragSource : public Root<implements<IDragSource, IDataObject> >  // IAsyncOperation
{
 private:
  class EnumFORMATETC : public Root<implements<IEnumFORMATETC> > {
   private:
    DragSource* m_owner;
    size_t m_iter;

   public:
    EnumFORMATETC(DragSource* owner, size_t index = 0) : m_owner(owner), m_iter(0) {}

    STDMETHOD(Next)(ULONG n, LPFORMATETC fmts, ULONG FAR* done) {
      ULONG dn = 0;
      HRESULT hr = m_owner->QueryFormats(m_iter, n, fmts, &dn);
      m_iter += dn;
      if (done) {
        *done = dn;
      }
      return hr;
    }
    STDMETHOD(Skip)(ULONG n) {
      m_iter += n;
      return S_OK;
    }
    STDMETHOD(Reset)(void) {
      m_iter = 0;
      return S_OK;
    }
    STDMETHOD(Clone)(IEnumFORMATETC** ppObject) {
      if (!ppObject) {
        return E_POINTER;
      }
      *ppObject = new EnumFORMATETC(m_owner, m_iter);
      return S_OK;
    }
  };

  class DataElement {
   private:
    FORMATETC m_format;
    STGMEDIUM m_medium;

   public:
    DataElement() { m_medium.tymed = TYMED_NULL; }
    ~DataElement() {
      if (m_medium.tymed != TYMED_NULL) {
        ::ReleaseStgMedium(&m_medium);
      }
    }
    const FORMATETC& GetFormat() const { return m_format; }
    bool IsCompatible(const FORMATETC* fmt) const {
      if (m_format.cfFormat == CF_TEXT || m_format.cfFormat == CF_UNICODETEXT) {
        return fmt->cfFormat == CF_TEXT || fmt->cfFormat == CF_UNICODETEXT;
      } else if (m_format.cfFormat == CF_HDROP || m_format.cfFormat == CF_SHELLIDLIST) {
        return fmt->cfFormat == CF_HDROP || fmt->cfFormat == CF_SHELLIDLIST;
      } else if (m_format.cfFormat == CF_INETURLA || m_format.cfFormat == CF_INETURLW) {
        return fmt->cfFormat == CF_INETURLA || fmt->cfFormat == CF_INETURLW;
      } else
        return false;
    }
    bool CanConvert(const FORMATETC* fmt) const {
      if (m_format.cfFormat == CF_TEXT || m_format.cfFormat == CF_UNICODETEXT) {
        return fmt->cfFormat == CF_TEXT || fmt->cfFormat == CF_UNICODETEXT;
      } else if (m_format.cfFormat == CF_SHELLIDLIST) {
        return fmt->cfFormat == CF_SHELLIDLIST || fmt->cfFormat == CF_HDROP || fmt->cfFormat == CF_TEXT ||
               fmt->cfFormat == CF_UNICODETEXT;
      } else if (m_format.cfFormat == CF_INETURLA || m_format.cfFormat == CF_INETURLW) {
        return fmt->cfFormat == CF_INETURLA || fmt->cfFormat == CF_INETURLW || fmt->cfFormat == CF_TEXT ||
               fmt->cfFormat == CF_UNICODETEXT;
      } else
        return false;
    }
    HGLOBAL ConvertCopy(CLIPFORMAT fmt) const {
      if (m_medium.tymed != TYMED_HGLOBAL) {
        return nullptr;
      }
      //
      ASSERT(m_format.cfFormat != fmt);
      if (m_format.cfFormat == CF_TEXT || m_format.cfFormat == CF_INETURLA) {
        if (fmt == CF_UNICODETEXT || fmt == CF_INETURLW) {  // A => W
          GlobalSrc<CHAR> src(m_medium.hGlobal);
          size_t srcln = strlen(src);
          size_t cch = srcln + 1;
          GlobalDst<WCHAR> dst(cch * sizeof(WCHAR));
          size_t dstln = ::MultiByteToWideChar(CP_ACP, 0, src, srcln, dst, cch);
          dst[dstln] = L'\0';
          return dst.Detach();
        }
      } else if (m_format.cfFormat == CF_UNICODETEXT || m_format.cfFormat == CF_INETURLW) {
        if (fmt == CF_TEXT || fmt == CF_INETURLA) {
          GlobalSrc<WCHAR> src(m_medium.hGlobal);
          size_t srcln = wcslen(src);
          size_t cch = (srcln + 1) * 2;
          GlobalDst<CHAR> dst(cch * sizeof(CHAR));
          size_t dstln = ::WideCharToMultiByte(CP_ACP, 0, src, srcln, dst, cch, 0, 0);
          dst[dstln] = '\0';
          return dst.Detach();
        }
      } else if (m_format.cfFormat == CF_SHELLIDLIST) {
        if (fmt == CF_HDROP) {
          GlobalSrc<CIDA> src(m_medium.hGlobal);
          return afx::CIDAToHDROP(src);
        } else if (fmt == CF_TEXT) {
          GlobalSrc<CIDA> src(m_medium.hGlobal);
          return afx::CIDAToTextA(src);
        } else if (fmt == CF_UNICODETEXT) {
          GlobalSrc<CIDA> src(m_medium.hGlobal);
          return afx::CIDAToTextW(src);
        }
      }
      return nullptr;
    }
    HRESULT CopyTo(FORMATETC* fmt, STGMEDIUM* medium) {
      if (fmt->cfFormat == m_format.cfFormat) {
        return DuplicateStgMedium(&m_medium, medium, &m_format, fmt->tymed);
      } else if (fmt->tymed & TYMED_HGLOBAL) {
        if (HGLOBAL hGlobal = ConvertCopy(fmt->cfFormat)) {
          TRACE(_T("ConvertCopy succeeded!!"));
          medium->hGlobal = hGlobal;
          if ((medium->pUnkForRelease = m_medium.pUnkForRelease) != null) {
            medium->pUnkForRelease->AddRef();
          }
          medium->tymed = TYMED_HGLOBAL;
          return S_OK;
        }
        return DV_E_FORMATETC;
      } else if (fmt->tymed & TYMED_ISTREAM) {
        if (HGLOBAL hGlobal = ConvertCopy(fmt->cfFormat)) {
          TRACE(_T("ConvertCopy succeeded!!"));
          CreateStreamOnHGlobal(hGlobal, true, &medium->pstm);
          if ((medium->pUnkForRelease = m_medium.pUnkForRelease) != null) {
            medium->pUnkForRelease->AddRef();
          }
          medium->tymed = TYMED_ISTREAM;
          return S_OK;
        }
        return DV_E_FORMATETC;
      } else {
        return DV_E_TYMED;
      }
    }
    HRESULT SetTo(const FORMATETC* srcfmt, const STGMEDIUM* src, BOOL bRelease) {
      m_format = *srcfmt;
      if (bRelease) {
        m_medium = *src;
        return S_OK;
      } else {
        return DuplicateStgMedium(src, &m_medium, srcfmt);
      }
    }
  };

 private:
  ref<IDragSourceHelper> m_helper;
  CAutoPtrArray<DataElement> m_data;
  DropSource m_dropsrc;

 public:
  void __init__(IUnknown* arg) {}
  void Dispose() throw() {
    m_helper.clear();
    m_data.RemoveAll();
  }

 public:  // IDragSource
  DropEffect DoDragDrop(DWORD dwSupportsEffect) {
    TRACE(_T("DoDragDrop Begin"));
    DWORD dwResult = 0;
    ::DoDragDrop(this, &m_dropsrc, dwSupportsEffect, &dwResult);
    TRACE(_T("DoDragDrop End"));
    return (DropEffect)dwResult;
  }
  DropEffect DoDragDrop(DWORD dwSupportsEffect, HWND hWndDrag, POINT ptCursor) {
    if (!m_helper) {
      CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, IID_IDragSourceHelper, (void**)&m_helper);
    }
    if (m_helper) {
      m_helper->InitializeFromWindow(hWndDrag, &ptCursor, this);
    }
    return DoDragDrop(dwSupportsEffect);
  }
  DropEffect DoDragDrop(DWORD dwSupportsEffect, HBITMAP hBitmap, COLORREF colorkey, POINT ptHotspot) {
    BITMAP info;
    ::GetObject(hBitmap, sizeof(info), &info);
    SHDRAGIMAGE image;
    image.sizeDragImage.cx = info.bmWidth;
    image.sizeDragImage.cy = info.bmHeight;
    image.ptOffset = ptHotspot;
    image.hbmpDragImage = hBitmap;
    image.crColorKey = colorkey;
    if (!m_helper) {
      CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, IID_IDragSourceHelper, (void**)&m_helper);
    }
    if (m_helper) {
      m_helper->InitializeFromBitmap(&image, this);
    }
    return DoDragDrop(dwSupportsEffect);
  }
  HRESULT AddData(CLIPFORMAT cfFormat, const void* data, size_t size) {
    if (!data || size == 0) {
      return E_INVALIDARG;
    }
    HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, size);
    memcpy(::GlobalLock(hGlobal), data, size);
    ::GlobalUnlock(hGlobal);
    HRESULT hr = AddGlobalData(cfFormat, hGlobal);
    if FAILED (hr) {
      ::GlobalFree(hGlobal);
    }
    return hr;
  }
  HRESULT AddGlobalData(CLIPFORMAT cfFormat, HGLOBAL hGlobal) {
    if (!hGlobal) {
      return E_INVALIDARG;
    }
    FORMATETC fmt;
    fmt.cfFormat = cfFormat;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.ptd = NULL;
    fmt.tymed = TYMED_HGLOBAL;
    STGMEDIUM medium;
    medium.tymed = TYMED_HGLOBAL;
    medium.pUnkForRelease = NULL;
    medium.hGlobal = hGlobal;
    return SetData(&fmt, &medium, true);
  }
  HRESULT AddIDList(LPCITEMIDLIST pIDList) {
    HGLOBAL hGlobal = afx::CIDAFromSingleIDList(pIDList);
    HRESULT hr = AddGlobalData(CF_SHELLIDLIST, hGlobal);
    if FAILED (hr) {
      ::GlobalFree(hGlobal);
      return hr;
    }
    AddGlobalData(CF_HDROP, afx::CIDAToHDROP(GlobalSrc<CIDA>(hGlobal)));
    return S_OK;
  }
  HRESULT AddIDList(const CIDA* pCIDA) { return AddData(CF_SHELLIDLIST, pCIDA, afx::CIDAGetSize(pCIDA)); }
  HRESULT AddText(string text) {
    PCTSTR chars = text.str();
    size_t size = str::size(chars);
    return AddData(CF_UNICODETEXT, chars, size);
  }
  HRESULT AddURL(string url) {
    PCTSTR chars = url.str();
    size_t size = str::size(chars);
    return AddData(CF_INETURL, chars, size);
  }

 public:  // IDataObject
  int FindFormat(const FORMATETC* fmt) const {
    int count = m_data.GetCount();
    for (int i = 0; i < count; ++i) {
      if (m_data[i]->IsCompatible(fmt)) {
        return i;
      }
    }
    for (int i = 0; i < count; ++i) {
      if (m_data[i]->CanConvert(fmt)) {
        return i;
      }
    }
    return -1;
  }
  HRESULT QueryFormats(size_t from, size_t num, FORMATETC fmts[], ULONG FAR* done) {
    size_t i = 0;
    for (; i < num; ++i) {
      size_t index = from + i;
      if (index >= m_data.GetCount()) {
        break;
      }
      fmts[i] = m_data[index]->GetFormat();
    }
    if (done) {
      *done = i;
    }
    return i == num ? S_OK : S_FALSE;
  }

  STDMETHOD(GetData)(FORMATETC* fmt, STGMEDIUM* medium) {
    TRACE(L"GetData");
    if (!fmt || !medium) {
      return E_POINTER;
    }
    if (!(DVASPECT_CONTENT & fmt->dwAspect)) {
      return DV_E_DVASPECT;
    }
    TRACE_FORMATS(fmt);
    int index = FindFormat(fmt);
    if (index < 0) {
      return DV_E_FORMATETC;
    }
    return m_data[index]->CopyTo(fmt, medium);
  }
  STDMETHOD(QueryGetData)(FORMATETC* fmt) {
    TRACE(L"QueryGetData");
    if (!fmt) {
      return E_POINTER;
    }
    if (!(DVASPECT_CONTENT & fmt->dwAspect)) {
      return DV_E_DVASPECT;
    }
    TRACE_FORMATS(fmt);
    return FindFormat(fmt) >= 0;
  }
  STDMETHOD(SetData)(FORMATETC* fmt, STGMEDIUM* medium, BOOL bRelease) {
    if (!fmt || !medium) {
      return E_POINTER;
    }
    TRACE_FORMATS(fmt);
    CAutoPtr<DataElement> data(new DataElement());
    HRESULT hr = data->SetTo(fmt, medium, bRelease);
    if FAILED (hr) {
      return hr;
    }
    m_data.Add(data);
    return S_OK;
  }
  STDMETHOD(EnumFormatEtc)(DWORD dwDirection, IEnumFORMATETC** ppEnumFormatEtc) {
    if (!ppEnumFormatEtc) {
      return E_POINTER;
    }
    switch (dwDirection) {
      case DATADIR_GET:
        TRACE(L"EnumFormatEtc");
        *ppEnumFormatEtc = new EnumFORMATETC(this);
        return S_OK;
      default:
        *ppEnumFormatEtc = null;
        ATLTRACENOTIMPL(_T("DragSource::EnumFormatEtc(dwDirection)"));
    }
  }
  STDMETHOD(GetDataHere)(FORMATETC* /* pFormatEtc */, STGMEDIUM* /* medium */) {
    ATLTRACENOTIMPL(_T("DragSource::GetDataHere"));
  }
  STDMETHOD(GetCanonicalFormatEtc)(FORMATETC* /* pFormatEtcIn */, FORMATETC* /* pFormatEtcOut */) {
    ATLTRACENOTIMPL(_T("DragSource::GetCanonicalFormatEtc"));
  }
  STDMETHOD(DAdvise)(FORMATETC* pFormatEtc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection) {
    ATLTRACENOTIMPL(_T("DragSource::DAdvise\n"));
  }
  STDMETHOD(DUnadvise)(DWORD dwConnection) { ATLTRACENOTIMPL(_T("DragSource::DUnadvise\n")); }
  STDMETHOD(EnumDAdvise)(IEnumSTATDATA** ppenumAdvise) { ATLTRACENOTIMPL(_T("DragSource::EnumDAdvise\n")); }
};

}  // namespace io
}  // namespace mew

//==============================================================================

AVESTA_EXPORT(DragSource)
