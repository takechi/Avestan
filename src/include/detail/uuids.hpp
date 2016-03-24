// uuids.hpp
#pragma once

#include <functional>

typedef float	REAL32;
typedef double	REAL64;

namespace avesta
{
	[ uuid("93134159-3D5F-4022-91B0-DEA784069020") ] struct CLSID_bool;
	[ uuid("315A3C00-4B0A-4F5B-AB0D-1443B92F2390") ] struct CLSID_INT8;
	[ uuid("790DBE84-FD1B-4F7D-9018-490924D692DA") ] struct CLSID_UINT8;
	[ uuid("BD9E766D-A8EC-4E68-8282-0B085AF4E3A6") ] struct CLSID_INT16;
	[ uuid("17CD079B-5388-4C1D-B009-08CD376A3441") ] struct CLSID_UINT16;
	[ uuid("F67C96B8-B53C-4F99-B25A-9E115A0BC2FD") ] struct CLSID_INT32;
	[ uuid("8266E366-B0C8-4140-9178-280775BC2E02") ] struct CLSID_UINT32;
	[ uuid("67931C49-2E1C-4A5F-A6F2-281073322F40") ] struct CLSID_INT64;
	[ uuid("0A85E065-71E2-49CA-9E06-8D812C34DE21") ] struct CLSID_UINT64;
	[ uuid("76268420-0122-4B3B-B0F5-9B504DDED6FE") ] struct CLSID_REAL32;
	[ uuid("B318A6E6-FD5B-4B97-A0E0-A6443342049E") ] struct CLSID_REAL64;
	[ uuid("5F2BDE7A-9665-4060-B23E-33BD9D2A7EC0") ] struct CLSID_SIZE;
	[ uuid("CFEBF29E-3DDB-4562-B687-D9F52A0D8B9C") ] struct CLSID_POINT;
	[ uuid("6373E583-D3E8-44D5-A706-146A9858CC8C") ] struct CLSID_RECT;

#define PP_EXPAND_NUMERIC(fn)			\
	fn(INT8)	fn(UINT8)				\
	fn(INT16)	fn(UINT16)				\
	fn(INT32)	fn(UINT32)				\
	fn(INT64)	fn(UINT64)				\
	fn(REAL32)	fn(REAL64)

#define PP_EXPAND_INTRINSIC(fn)			\
	fn(SIZE)	fn(POINT)	fn(RECT)	\
	fn(bool)	PP_EXPAND_NUMERIC(fn)

	template < typename T > struct type2clsid { typedef T Result; };
#define PP_TYPE2CLSID(n)	\
	template <> struct type2clsid<n> { typedef CLSID_##n Result; };
	PP_EXPAND_INTRINSIC( PP_TYPE2CLSID )
#undef PP_TYPE2CLSID
}

#define uuidof(TYPE)	__uuidof(avesta::type2clsid<TYPE>::Result)

template <> struct std::less<GUID> : std::binary_function<GUID, GUID, bool>
{
	bool operator() (REFGUID lhs, REFGUID rhs) const
	{
		return memcmp(&lhs, &rhs, sizeof(GUID)) < 0;
	}
};
