// stdafx.h
#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NON_CONFORMING_SWPRINTFS

#define MEW_API __declspec(dllexport)

//==============================================================================

#define _ATL_NO_COM
#define _ATL_NO_COM_SUPPORT
#define _ATL_NO_HOSTING

#include "mew.hpp"

#undef ATLASSERT
#define ATLASSERT	ASSERT

#ifndef _DEBUG
#	define m_bModal	true
#endif

#include <atlwin.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <gdiplus.h>

namespace WTL
{
#undef max
#undef min
	template < typename T > inline T max(T a, T b)	{ return a > b ? a : b; }
	template < typename T > inline T min(T a, T b)	{ return a < b ? a : b; }
}

#include <atlapp.h>
#include <atluser.h>
#include <atlctrls.h>
#include <atlctrlw.h>
#include <atlctrlx.h>
#include <atldlgs.h>

#include "std/vector.hpp"
#include "std/algorithm.hpp"
