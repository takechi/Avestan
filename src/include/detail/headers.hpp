// headers.hpp
#pragma once

//==============================================================================
// warning

#pragma warning(disable : 4100)  // warning C4100: 'argument' : 引数は関数の本体部で 1
                                 // 度も参照されません。
#pragma warning(disable : 4127)  // warning C4127: 条件式が定数です。
#pragma warning(disable : 4201)  // warning C4201: 非標準の拡張機能が使用されています :
                                 // 無名の構造体または共用体です。
#pragma warning(disable : 4290)  // warning C4290: C++ の例外の指定は無視されます。関数が
                                 // __declspec(nothrow) でないことのみ表示されます。

//==============================================================================
// system headers

#ifndef _WINDOWS_

/*
#define WINVER 0x0501

#if WINVER > 0x0500
#define _WIN32_WINDOWS 0x0501
#define _WIN32_WINNT 0x0501
#define _WIN32_IE 0x0600
#else
#define _WIN32_WINDOWS 0x0500
#define _WIN32_WINNT 0x0500
#define _WIN32_IE 0x0501
#endif
*/

#define _ATL_ALL_WARNINGS
#define _ATL_NO_OPENGL
#define _ATL_NO_OLD_NAMES
#define _WTL_NO_CSTRING        // CString ではなく、ATL::CStringT を使用するため。
#define _WTL_NO_WTYPES         // CSize, CPoint, CRect
                               // ではなく、ATLのクラスを使用するため。
#define _WTL_NO_UNION_CLASSES  // ATL にある同名のクラスを使用する。

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define _CRTDBG_MAP_ALLOC

#endif

//==============================================================================
// check compile options

/*
#if _WIN32_WINDOWS < 0x0500
#error _WIN32_WINDOWS must be 0x0500 or lator.
#endif
*/
#ifndef _MT
#error マルチスレッド設定でビルドする必要があります。
#endif

#ifndef _CPPUNWIND
#error C++例外処理を有効にする必要があります。
#endif

#ifndef _NATIVE_WCHAR_T_DEFINED
#error wchar_t を組み込み型として扱う必要があります。
#endif

//==============================================================================
// intrinsic

#include <windows.h>
/*
extern "C"
{
   long  __cdecl _InterlockedIncrement(long volatile *Addend);
   long  __cdecl _InterlockedDecrement(long volatile *Addend);
   long  __cdecl _InterlockedCompareExchange(long* volatile Dest, long Exchange,
long Comp); long  __cdecl _InterlockedExchange(long* volatile Target, long
Value); long  __cdecl _InterlockedExchangeAdd(long* volatile Addend, long
Value);
}
*/

#pragma intrinsic(_InterlockedCompareExchange)
#define InterlockedCompareExchange _InterlockedCompareExchange

#pragma intrinsic(_InterlockedExchange)
#define InterlockedExchange _InterlockedExchange

#pragma intrinsic(_InterlockedExchangeAdd)
#define InterlockedExchangeAdd _InterlockedExchangeAdd

#pragma intrinsic(_InterlockedIncrement)
#define InterlockedIncrement _InterlockedIncrement

#pragma intrinsic(_InterlockedDecrement)
#define InterlockedDecrement _InterlockedDecrement

//==============================================================================
// standard headers

#include <atlbase.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <crtdbg.h>
#include <malloc.h>

#include <cstdio>
#include <cstdlib>

//==============================================================================
// macro

#ifndef MAKEDWORD
#define MAKEDWORD(ch0, ch1, ch2, ch3) \
  ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
#endif

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif
#define GET_XY_LPARAM(lp) GET_X_LPARAM(lp), GET_Y_LPARAM(lp)

#ifdef UNICODE
const bool IS_UNICODE_CHARSET = true;
#else
const bool IS_UNICODE_CHARSET = false;
#endif

#define WIDEN2(x) L##x
#define WIDEN(x) WIDEN2(x)
#define __FILE_W__ WIDEN(__FILE__)
#define __FUNCTION_W__ WIDEN(__FUNCTION__)

using REAL32 = float;   ///< 32bit浮動小数.
using REAL64 = double;  ///< 64bit浮動小数.
