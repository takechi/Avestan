/// @file basic.hpp
/// doxygenのためのOSインタフェースの再定義.
#pragma once

//==============================================================================

#ifndef __IUnknown_INTERFACE_DEFINED__
#define __IUnknown_INTERFACE_DEFINED__

/// IUnknown.
__interface IUnknown {
  /// Returns pointers to supported interfaces.
  HRESULT __stdcall QueryInterface(REFINTF ppInterface);
  /// Increments reference count.
  ULONG __stdcall AddRef();
  /// Decrements reference count.
  ULONG __stdcall Release();
};

#endif  // __IUnknown_INTERFACE_DEFINED__

  //==============================================================================

#ifndef __IEnumUnknown_INTERFACE_DEFINED__
#define __IEnumUnknown_INTERFACE_DEFINED__

/// IEnumUnknown. from objidl.h
__interface __declspec(uuid("00000100-0000-0000-C000-000000000046")) IEnumUnknown : IUnknown {
  HRESULT __stdcall Next(ULONG count, IUnknown * *output, ULONG * fetched);
  HRESULT __stdcall Skip(ULONG count);
  HRESULT __stdcall Reset();
  HRESULT __stdcall Clone(IEnumUnknown * *ppEnum);
};

#endif  // __IEnumUnknown_INTERFACE_DEFINED__

  //==============================================================================

#ifndef __ISequentialStream_INTERFACE_DEFINED__
#define __ISequentialStream_INTERFACE_DEFINED__

/// ISequentialStream.
__interface __declspec(uuid("0C733A30-2A1C-11CE-ADE5-00AA0044773D")) ISequentialStream : IUnknown {
  HRESULT __stdcall Read(void* pv, ULONG cb, ULONG* pcbRead);
  HRESULT __stdcall Write(const void* pv, ULONG cb, ULONG* pcbWritten);
};

#endif  // __ISequentialStream_INTERFACE_DEFINED__

  //==============================================================================

#ifndef __IStream_INTERFACE_DEFINED__
#define __IStream_INTERFACE_DEFINED__

/// IStream.
__interface __declspec(uuid("0000000c-0000-0000-C000-000000000046")) IStream : ISequentialStream {
  HRESULT __stdcall Seek(LARGE_INTEGER move, DWORD dwOrigin, ULARGE_INTEGER * newpos);
  HRESULT __stdcall SetSize(ULARGE_INTEGER size);
  HRESULT __stdcall CopyTo(IStream * pStream, ULARGE_INTEGER cb, ULARGE_INTEGER * pcbRead, ULARGE_INTEGER * pcbWritten);
  HRESULT __stdcall Commit(DWORD grfCommitFlags);
  HRESULT __stdcall Revert();
  HRESULT __stdcall LockRegion(ULARGE_INTEGER offset, ULARGE_INTEGER cb, DWORD dwLockType);
  HRESULT __stdcall UnlockRegion(ULARGE_INTEGER offset, ULARGE_INTEGER cb, DWORD dwLockType);
  HRESULT __stdcall Stat(STATSTG * pStatStg, DWORD grfStatFlag);
  HRESULT __stdcall Clone(IStream * *ppStream);
};

#endif  // __IStream_INTERFACE_DEFINED__

  //==============================================================================

#ifndef __IOleWindow_INTERFACE_DEFINED__
#define __IOleWindow_INTERFACE_DEFINED__

/// IOleWindow. from oleidl.h
__interface __declspec(uuid("00000114-0000-0000-C000-000000000046")) IOleWindow : IUnknown {
  HRESULT __stdcall GetWindow(HWND * phwnd);
  HRESULT __stdcall ContextSensitiveHelp(BOOL fEnterMode);
};

#endif  // __IOleWindow_INTERFACE_DEFINED__

  //==============================================================================

#ifndef __IImageList_INTERFACE_DEFINED__
#define __IImageList_INTERFACE_DEFINED__

/// IImageList. from commoncontrols.h
__interface IImageList : IUnknown {
  HRESULT __stdcall Add(HBITMAP hbmImage, HBITMAP hbmMask, int* pi);
  HRESULT __stdcall ReplaceIcon(int i, HICON hicon, int* pi);
  HRESULT __stdcall SetOverlayImage(int iImage, int iOverlay);
  HRESULT __stdcall Replace(int i, HBITMAP hbmImage, HBITMAP hbmMask);
  HRESULT __stdcall AddMasked(HBITMAP hbmImage, COLORREF crMask, int* pi);
  HRESULT __stdcall Draw(IMAGELISTDRAWPARAMS * pimldp);
  HRESULT __stdcall Remove(int i);
  HRESULT __stdcall GetIcon(int i, UINT flags, HICON* picon);
  HRESULT __stdcall GetImageInfo(int i, IMAGEINFO* pImageInfo);
  HRESULT __stdcall Copy(int iDst, IUnknown* punkSrc, int iSrc, UINT uFlags);
  HRESULT __stdcall Merge(int i1, IUnknown* punk2, int i2, int dx, int dy, REFIID riid, PVOID* ppv);
  HRESULT __stdcall Clone(REFIID riid, PVOID * ppv);
  HRESULT __stdcall GetImageRect(int i, RECT* prc);
  HRESULT __stdcall GetIconSize(int* cx, int* cy);
  HRESULT __stdcall SetIconSize(int cx, int cy);
  HRESULT __stdcall GetImageCount(int* pi);
  HRESULT __stdcall SetImageCount(UINT uNewCount);
  HRESULT __stdcall SetBkColor(COLORREF clrBk, COLORREF * pclr);
  HRESULT __stdcall GetBkColor(COLORREF * pclr);
  HRESULT __stdcall BeginDrag(int iTrack, int dxHotspot, int dyHotspot);
  HRESULT __stdcall EndDrag();
  HRESULT __stdcall DragEnter(HWND hwndLock, int x, int y);
  HRESULT __stdcall DragLeave(HWND hwndLock);
  HRESULT __stdcall DragMove(int x, int y);
  HRESULT __stdcall SetDragCursorImage(IUnknown * punk, int iDrag, int dxHotspot, int dyHotspot);
  HRESULT __stdcall DragShowNolock(BOOL fShow);
  HRESULT __stdcall GetDragImage(POINT * ppt, POINT * pptHotspot, REFIID riid, PVOID * ppv);
  HRESULT __stdcall GetItemFlags(int i, DWORD* dwFlags);
  HRESULT __stdcall GetOverlayImage(int iOverlay, int* piIndex);
};

#endif  // __IImageList_INTERFACE_DEFINED__

  //==============================================================================

#ifndef __IDataObject_INTERFACE_DEFINED__
#define __IDataObject_INTERFACE_DEFINED__

using DATADIR = enum tagDATADIR { DATADIR_GET = 1, DATADIR_SET = 2 };

/// IDataObject.
__interface __declspec(uuid("0000010e-0000-0000-C000-000000000046") IDataObject : IUnknown
{
  HRESULT __stdcall GetData(FORMATETC * pFormatEtc, STGMEDIUM * pMedium);
  HRESULT __stdcall GetDataHere(FORMATETC * pFormatEtc, STGMEDIUM * pMedium);
  HRESULT __stdcall QueryGetData(FORMATETC * pFormatEtc);
  HRESULT __stdcall GetCanonicalFormatEtc(FORMATETC * pFormatEtcIn, FORMATETC * pFormatEtcOut);
  HRESULT __stdcall SetData(FORMATETC * pFormatEtc, STGMEDIUM * pMedium, BOOL fRelease);
  HRESULT __stdcall EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC * *ppenumFormatEtc);
  HRESULT __stdcall DAdvise(FORMATETC * pFormatEtc, DWORD advf, IAdviseSink * pAdvSink, DWORD * pdwConnection);
  HRESULT __stdcall DUnadvise(DWORD dwConnection);
  HRESULT __stdcall EnumDAdvise(IEnumSTATDATA * *ppEnumAdvise);
};

#endif  // __IDataObject_INTERFACE_DEFINED__

//==============================================================================

#ifndef __IDropSource_INTERFACE_DEFINED__
#define __IDropSource_INTERFACE_DEFINED__

/// IDropSource.
__interface __declspec(uuid("00000121-0000-0000-C000-000000000046") IDropSource : IUnknown
{
  HRESULT __stdcall QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
  HRESULT __stdcall GiveFeedback(DWORD dwEffect);
};

#endif  // __IDropSource_INTERFACE_DEFINED__

//==============================================================================

#ifndef __IDropTarget_INTERFACE_DEFINED__
#define __IDropTarget_INTERFACE_DEFINED__

/// IDropTarget.
__interface __declspec(uuid("00000122-0000-0000-C000-000000000046") IDropTarget : IUnknown
{
  HRESULT __stdcall DragEnter(IDataObject * pDataObj, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
  HRESULT __stdcall DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
  HRESULT __stdcall DragLeave();
  HRESULT __stdcall Drop(IDataObject * pDataObj, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect);
};

#endif  // __IDropTarget_INTERFACE_DEFINED__

//==============================================================================
