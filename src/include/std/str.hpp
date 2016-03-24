/// @file str.hpp
/// UNDOCUMENTED
#pragma once

#include <mbstring.h>

namespace mew
{
	/// 低レベル文字列処理.
	namespace str
	{
		/// 文字列が0または長さが0か否かを判別する.
		inline bool empty(const char*    str)	{ return !str || str[0] == '\0'; }
		inline bool empty(const wchar_t* str)	{ return !str || str[0] == L'\0'; }
		/// 文字列の長さを返す.
		inline size_t length(const char*    str)	{ return str ? strlen(str) : 0; }
		inline size_t length(const wchar_t* str)	{ return str ? wcslen(str) : 0; }
		/// 文字列を終端のヌル文字も含め、格納するのに必要なバイト数を返す.
		inline size_t size(const char*    str)		{ return sizeof(char)    * (length(str) + 1); }
		inline size_t size(const wchar_t* str)		{ return sizeof(wchar_t) * (length(str) + 1); }
		/// 
		inline size_t span(const char*    str, const char*    set)	{ return _mbsspn((const UCHAR*)str, (const UCHAR*)set); }
		inline size_t span(const wchar_t* str, const wchar_t* set)	{ return wcsspn(str, set); }
		//
		inline size_t cspan(const char*    str, const char*    set)	{ return _mbscspn((const UCHAR*)str, (const UCHAR*)set); }
		inline size_t cspan(const wchar_t* str, const wchar_t* set)	{ return wcscspn(str, set); }
		//
		inline const char*    inc(const char*  x)		{ return (const char*)_mbsinc((const UCHAR*)x); }
		inline       char*    inc(      char*  x)		{ return (char*)_mbsinc((UCHAR*)x); }
		inline const wchar_t* inc(const wchar_t* x)		{ return x+1; }
		inline       wchar_t* inc(      wchar_t* x)		{ return x+1; }
		/// 文字列の中で最初に出現する文字の位置を返す.
		inline const char*    find(const char*    str, char    c)	{ return (const char*)_mbschr((const UCHAR*)str, (UINT)c); }
		inline const wchar_t* find(const wchar_t* str, wchar_t c)
		{
#if _MANAGED
			// managedでは、なぜかwcschrでの検索が失敗する。原因不明。
			while(*str && *str != c) { str++; }
			return (*str == c) ? str : 0;
#else
			return wcschr(str, c);
#endif
		}
		inline       char*    find(      char*    str, char    c)	{ return (char*)_mbschr((UCHAR*)str, (UINT)c); }
		inline       wchar_t* find(      wchar_t* str, wchar_t c)
		{
#if _MANAGED
			// managedでは、なぜかwcschrでの検索が失敗する。原因不明。
			while(*str && *str != c) { str++; }
			return (*str == c) ? str : 0;
#else
			return wcschr(str, c);
#endif
		}
		/// 文字列中で最後に出現する文字の位置を返す.
		inline const char*    find_reverse(const char*    str, char    c)	{ return (const char*)_mbsrchr((const UCHAR*)str, (UINT)c); }
		inline const wchar_t* find_reverse(const wchar_t* str, wchar_t c)	{ return wcsrchr(str, c); }
		inline char*    find_reverse(char*     str, char  c)	{ return (char*)_mbsrchr((UCHAR*)str, (UINT)c); }
		inline wchar_t* find_reverse(wchar_t*  str, wchar_t c)	{ return wcsrchr(str, c); }
		/// 文字列の中で、マスクに含まれる文字が最初に見つかった位置を返す.
		inline char*  find_some_of(char* str, const char* mask)
		{
			char* found = str + _mbscspn((const UCHAR*)str, (const UCHAR*)mask);
			return *found == '\0' ? 0 : found;
		}
		inline wchar_t* find_some_of(wchar_t* str, const wchar_t* mask)
		{
			wchar_t* found = str + wcscspn(str, mask);
			return *found == L'\0' ? 0 : found;
		}
		inline const char*  find_some_of(const char*  str, const char*  mask)
		{
			const char* found = str + _mbscspn((const UCHAR*)str, (const UCHAR*)mask);
			return *found == '\0' ? 0 : found;
		}
		inline const wchar_t* find_some_of(const wchar_t* str, const wchar_t* mask)
		{
			const wchar_t* found = str + wcscspn(str, mask);
			return *found == L'\0' ? 0 : found;
		}
		/// 大文字小文字を区別して比較する.
		inline int compare(const char*    lhs, const char*  rhs)
		{
			if(!lhs) return empty(rhs) ? 0 :  1;
			if(!rhs) return empty(lhs) ? 0 : -1;
			return strcmp(lhs, rhs);
		}
		inline int compare(const wchar_t* lhs, const wchar_t* rhs)
		{
			if(!lhs) return empty(rhs) ? 0 :  1;
			if(!rhs) return empty(lhs) ? 0 : -1;
			return wcscmp(lhs, rhs);
		}
		inline int compare(const char*    lhs, const char*    rhs, size_t sz)
		{
			if(!lhs) return empty(rhs) ? 0 :  1;
			if(!rhs) return empty(lhs) ? 0 : -1;
			return strncmp(lhs, rhs, sz);
		}
		inline int compare(const wchar_t* lhs, const wchar_t* rhs, size_t sz)
		{
			if(!lhs) return empty(rhs) ? 0 :  1;
			if(!rhs) return empty(lhs) ? 0 : -1;
			return wcsncmp(lhs, rhs, sz);
		}
		/// 大文字小文字を区別しないで比較する.
		inline int compare_nocase(const char* lhs, const char* rhs)
		{
			if(!lhs) return empty(rhs) ? 0 :  1;
			if(!rhs) return empty(lhs) ? 0 : -1;
			return stricmp(lhs, rhs);
		}
		inline int compare_nocase(const wchar_t* lhs, const wchar_t* rhs)
		{
			if(!lhs) return empty(rhs) ? 0 :  1;
			if(!rhs) return empty(lhs) ? 0 : -1;
			return _wcsicmp(lhs, rhs);
		}
		inline int compare_nocase(const char*    lhs, const char*    rhs, size_t sz)
		{
			if(!lhs) return empty(rhs) ? 0 :  1;
			if(!rhs) return empty(lhs) ? 0 : -1;
			return strnicmp(lhs, rhs, sz);
		}
		inline int compare_nocase(const wchar_t* lhs, const wchar_t* rhs, size_t sz)
		{
			if(!lhs) return empty(rhs) ? 0 :  1;
			if(!rhs) return empty(lhs) ? 0 : -1;
			return _wcsnicmp(lhs, rhs, sz);
		}
		/// 等しいか否かを返す.
		inline bool equals(const char*    lhs, const char*    rhs)
		{
			if(!lhs) return empty(rhs);
			if(!rhs) return empty(lhs);
			return strcmp(lhs, rhs) == 0;
		}
		inline bool equals(const wchar_t* lhs, const wchar_t* rhs)
		{
			if(!lhs) return empty(rhs);
			if(!rhs) return empty(lhs);
			return wcscmp(lhs, rhs) == 0;
		}
		inline bool equals_nocase(const char*    lhs, const char*    rhs)
		{
			if(!lhs) return empty(rhs);
			if(!rhs) return empty(lhs);
			return stricmp(lhs, rhs) == 0;
		}
		inline bool equals_nocase(const wchar_t* lhs, const wchar_t* rhs)
		{
			if(!lhs) return empty(rhs);
			if(!rhs) return empty(lhs);
			return _wcsicmp(lhs, rhs) == 0;
		}
		/// 0でない場合、文字列の長さを0にする.
		inline char*    clear(char*    str)		{ if(str) str[0] =  '\0'; return str; }
		inline wchar_t* clear(wchar_t* str)		{ if(str) str[0] = L'\0'; return str; }
		/// コピーする.
		inline char*    copy(char*    dst, const char*    src, size_t sz)	{ return strncpy(dst, src, sz); }
		inline wchar_t* copy(wchar_t* dst, const wchar_t* src, size_t sz)	{ return wcsncpy(dst, src, sz); }
		inline char*    copy(char*    dst, const char*    src)				{ return src ? strcpy(dst, src) : clear(dst); }
		inline wchar_t* copy(wchar_t* dst, const wchar_t* src)				{ return src ? wcscpy(dst, src) : clear(dst); }
		/// 前に連結する.
		template < typename Ch > inline Ch* prepend(Ch* dst, const Ch* src)
		{
			size_t dstln = length(dst);
			size_t srcln = length(src);
			memmove(dst+srcln, dst, (dstln+1)*sizeof(Ch));
			memcpy(dst, src, srcln*sizeof(Ch));
			return dst;
		}
		/// 連結する.
		inline char*    append(char*    dst, const char*    src)	{ return strcat(dst, src); }
		inline wchar_t* append(wchar_t* dst, const wchar_t* src)	{ return wcscat(dst, src); }
		inline char*    append(char*    dst, const char*    src, size_t srclen)	{ return strncat(dst, src, srclen); }
		inline wchar_t* append(wchar_t* dst, const wchar_t* src, size_t srclen)	{ return wcsncat(dst, src, srclen); }
		/// マルチバイト文字列⇔ワイド文字列を変換する.
		inline void convert(char*    dst, const char*    src, size_t sz)	{ copy(dst, src, sz); }
		inline void convert(wchar_t* dst, const wchar_t* src, size_t sz)	{ copy(dst, src, sz); }
		inline void convert(wchar_t* dst, const char*    src, size_t sz)	{ MultiByteToWideChar(CP_ACP, 0, src, -1, dst, (int)sz); }
		inline void convert(char*    dst, const wchar_t* src, size_t sz)	{ WideCharToMultiByte(CP_ACP, 0, src, -1, dst, (int)sz, 0, 0); }
		/// 文字変換.
		inline char		tolower(char c)			{ return (__isascii(c)  && ::isupper(c) ) ? (char)::tolower(c) : c; }
		inline wchar_t	tolower(wchar_t c)		{ return (::iswascii(c) && ::iswupper(c)) ? ::towlower(c) : c; }
		inline char		toupper(char c)			{ return (__isascii(c)  && ::islower(c) ) ? (char)::toupper(c) : c; }
		inline wchar_t	toupper(wchar_t c)		{ return (::iswascii(c) && ::iswlower(c)) ? ::towupper(c) : c; }
		/// 文字列から整数に変換する.
		inline int atoi(const char*    str)	{ return str ? ::atoi(str) : 0; }
		inline int atoi(const wchar_t* str)	{ return str ? ::_wtoi(str) : 0; }
		inline int atou(const char*    str, int radix = 10)	{ return str ? ::strtoul(str, NULL, radix) : 0; }
		inline int atou(const wchar_t* str, int radix = 10)	{ return str ? ::wcstoul(str, NULL, radix) : 0; }
		inline __int64 atoi64(const char*   str)	{ return str ? ::_atoi64(str) : 0; }
		inline __int64 atoi64(const wchar_t* str)	{ return str ? ::_wtoi64(str) : 0; }
		/// 文字列から実数に変換する.
		inline double atof(const char*    str)	{ return str ? ::atof(str) : 0.0; }
		inline double atof(const wchar_t* str)	{ return str ? ::_wtof(str) : 0.0; }
		/// 整数から文字列に変換する.
		inline char*    itoa(char*    str, int value, int radix = 10)				{ return ::_itoa(value, str, radix); }
		inline wchar_t* itoa(wchar_t* str, int value, int radix = 10)				{ return ::_itow(value, str, radix); }
		inline char*    itoa(char*    str, unsigned int value, int radix = 10)		{ return ::_ultoa(value, str, radix); }
		inline wchar_t* itoa(wchar_t* str, unsigned int value, int radix = 10)		{ return ::_ultow(value, str, radix); }
		inline char*    itoa(char*    str, __int64 value, int radix = 10)			{ return ::_i64toa(value, str, radix); }
		inline wchar_t* itoa(wchar_t* str, __int64 value, int radix = 10)			{ return ::_i64tow(value, str, radix); }
		inline char*    itoa(char*    str, unsigned __int64 value, int radix = 10)	{ return ::_ui64toa(value, str, radix); }
		inline wchar_t* itoa(wchar_t* str, unsigned __int64 value, int radix = 10)	{ return ::_ui64tow(value, str, radix); }
		/// 実数から文字列に変換する.
		inline char*    ftoa(char*    str, double value, int digits = 6)	{ return ::_gcvt(value, digits, str); }
		inline wchar_t* ftoa(wchar_t* str, double value, int digits = 6)
		{
			char buffer[32];
			::_gcvt(value, digits, buffer);
			size_t i = 0;
			do { str[i] = buffer[i]; } while(buffer[i++] != '\0');
			return str;
		}
		///
		inline char* guidtoa(char* str, REFGUID value, bool brace = false) throw()
		{
			sprintf(str, brace ? "{%08X-%04X-%04X-%04X-%08X%04X}" : "%08X-%04X-%04X-%04X-%08X%04X",
				value.Data1, value.Data2, value.Data3,
				MAKEWORD (value.Data4[1], value.Data4[0]),
				MAKEDWORD(value.Data4[5], value.Data4[4], value.Data4[3], value.Data4[2]),
				MAKEWORD (value.Data4[7], value.Data4[6]));
			return str;
		}
		inline wchar_t* guidtoa(wchar_t* str, REFGUID value, bool brace = false) throw()
		{
			swprintf(str, brace ? L"{%08X-%04X-%04X-%04X-%08X%04X}" : L"%08X-%04X-%04X-%04X-%08X%04X",
				value.Data1, value.Data2, value.Data3,
				MAKEWORD (value.Data4[1], value.Data4[0]),
				MAKEDWORD(value.Data4[5], value.Data4[4], value.Data4[3], value.Data4[2]),
				MAKEWORD (value.Data4[7], value.Data4[6]));
			return str;
		}
		///
		inline bool atoguid(GUID& guid, PCSTR str)
		{
			UINT v1, v[10];
			if(sscanf(str, "{%8X-%4X-%4X-%2X%2X-%2X%2X%2X%2X%2X%2X}", &v1, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9]) != 11)
				return false;
			guid.Data1    = v1;
			guid.Data2    = (WORD)v[0];
			guid.Data3    = (WORD)v[1];
			guid.Data4[0] = (BYTE)v[2];
			guid.Data4[1] = (BYTE)v[3];
			guid.Data4[2] = (BYTE)v[4];
			guid.Data4[3] = (BYTE)v[5];
			guid.Data4[4] = (BYTE)v[6];
			guid.Data4[5] = (BYTE)v[7];
			guid.Data4[6] = (BYTE)v[8];
			guid.Data4[7] = (BYTE)v[9];
			return true;
		}
		inline bool atoguid(GUID& guid, PCWSTR str)
		{
			UINT v1, v[10];
			if(swscanf(str, L"{%8X-%4X-%4X-%2X%2X-%2X%2X%2X%2X%2X%2X}", &v1, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9] ) != 11)
				return false;
			guid.Data1    = v1;
			guid.Data2    = (WORD)v[0];
			guid.Data3    = (WORD)v[1];
			guid.Data4[0] = (BYTE)v[2];
			guid.Data4[1] = (BYTE)v[3];
			guid.Data4[2] = (BYTE)v[4];
			guid.Data4[3] = (BYTE)v[5];
			guid.Data4[4] = (BYTE)v[6];
			guid.Data4[5] = (BYTE)v[7];
			guid.Data4[6] = (BYTE)v[8];
			guid.Data4[7] = (BYTE)v[9];
			return true;
		}
		/// 相対パスを絶対パスに変換する.
		inline char*    fullpath(char*    dst, const char*    src, size_t sz)	{ return _fullpath(dst, src, sz); }
		inline wchar_t* fullpath(wchar_t* dst, const wchar_t* src, size_t sz)	{ return _wfullpath(dst, src, sz); }
	}
}
