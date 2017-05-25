// variant.cpp

#include "stdafx.h"
#include "private.h"
#include "message.hpp"
#include "io.hpp"
#include "object.hpp"

//==============================================================================

namespace
{
	static size_t find_tail(PCSTR str, size_t length) throw()
	{
		for(size_t i = length; i > 0; --i)
		{
			if(str[i-1] != '\0')
				return i;
		}
		return 0;
	}
	static bool isPrintable(PCSTR str, size_t length) throw()
	{
		WORD dst[16];
		if(!GetStringTypeExA(LOCALE_USER_DEFAULT, CT_CTYPE1, str, length, dst))
			return false;
		const WORD C1_PRINTABLE = C1_UPPER | C1_LOWER | C1_DIGIT | C1_SPACE | C1_PUNCT | C1_XDIGIT | C1_ALPHA;
		for(size_t i = 0; i < length; ++i)
		{
			if((dst[i] & C1_PRINTABLE) == 0)
				return false;
		}
		return true;
	}
}

namespace mew
{
	ToString<TypeCode>::ToString(TypeCode value) throw()
	{
		UINT32 code = value;
		if(code < 65536)
		{
			_stprintf(m_str, _T("#%d"), code);
		}
		else
		{
			while((code & 0xFF000000) == 0) { code <<= 8; }
			m_str[0] = (TCHAR)((code & 0xFF000000) >> 24);
			m_str[1] = (TCHAR)((code & 0x00FF0000) >> 16);
			m_str[2] = (TCHAR)((code & 0x0000FF00) >>  8);
			m_str[3] = (TCHAR)((code & 0x000000FF));
			m_str[4] = _T('\0');
		}
	}
	ToString<Guid>::ToString(const Guid& value) throw()
	{
		if(memcmp(((BYTE*)&value)+4, ((BYTE*)&GUID_Index)+4, 12) == 0)
		{
			m_str[0] = _T('#');
			str::itoa(m_str+1, (int)value.Data1); // as Index
		}
		else
		{
			PCSTR str =  (PCSTR)&value;
			size_t length = find_tail(str, sizeof(Guid));
			if(0 < length && isPrintable(str, length))
				str::convert(m_str, str, 40); // as string
			else
				str::guidtoa(m_str, value, true); // as GUID
		}
	}
}

//==============================================================================

#pragma warning( disable : 4702 ) // TRESPASS() に制御がわたらないため

#define MEW_CASE(exp, type_t)	case VariantType<type_t>::Code: { exp(type_t) } TRESPASS();

#define MEW_EXPAND_INTRINSIC(exp)				\
	MEW_CASE(exp, bool)							\
	MEW_CASE(exp, INT8)  MEW_CASE(exp, UINT8)	\
	MEW_CASE(exp, INT16) MEW_CASE(exp, UINT16)	\
	MEW_CASE(exp, INT32) MEW_CASE(exp, UINT32)	\
	MEW_CASE(exp, INT64) MEW_CASE(exp, UINT64)

#define MEW_EXPAND_STRUCT(exp)		\
	MEW_CASE(exp, Size)				\
	MEW_CASE(exp, Point)			\
	MEW_CASE(exp, Rect)				\
	MEW_CASE(exp, Color)

#define MEW_EXPAND_ALL(exp)		\
	MEW_EXPAND_INTRINSIC(exp)	\
	MEW_EXPAND_STRUCT(exp)

//==============================================================================

namespace
{
	void StringToPOD(PCWSTR s, bool&  result) throw()
	{
		result = s && (str::equals_nocase(s, _T("true")) || str::equals_nocase(s, _T("yes")));
	}
	void StringToPOD(PCWSTR s, INT8&  result) throw()	{ result = (INT8)str::atoi(s); }
	void StringToPOD(PCWSTR s, INT16& result) throw()	{ result = (INT16)str::atoi(s); }
	void StringToPOD(PCWSTR s, INT32& result) throw()	{ result = str::atoi(s); }
	void StringToPOD(PCWSTR s, INT64& result) throw()	{ result = str::atoi64(s); }
	void StringToPOD(PCWSTR s, UINT8&  result) throw()	{ result = (UINT8)str::atoi(s); }
	void StringToPOD(PCWSTR s, UINT16& result) throw()	{ result = (UINT16)str::atoi(s); }
	void StringToPOD(PCWSTR s, UINT32& result) throw()	{ result = str::atou(s); }
	void StringToPOD(PCWSTR s, UINT64& result) throw()	{ result = str::atoi64(s); }
	void StringToPOD(PCWSTR s, REAL32& result) throw()	{ result = (REAL32)str::atof(s); }
	void StringToPOD(PCWSTR s, REAL64& result) throw()	{ result = str::atof(s); }
	void StringToPOD(PCWSTR s, Size& result) throw()
	{
		_stscanf(s, _T("%d%d"), &result.cx, &result.cy);
	}
	void StringToPOD(PCWSTR s, Point& result) throw()
	{
		_stscanf(s, _T("%d%d"), &result.x, &result.y);
	}
	void StringToPOD(PCWSTR s, Color& result) throw()
	{
		int c[4] = { 0, 0, 0, 255 };
		_stscanf(s, _T("%d%d%d%d"), &c[0], &c[1], &c[2], &c[3]);
		result = Color((UINT8)c[0], (UINT8)c[1], (UINT8)c[2], (UINT8)c[3]);
	}
	void StringToPOD(PCWSTR s, Rect& result) throw()
	{
		_stscanf(s, _T("%d%d%d%d"), &result.left, &result.top, &result.right, &result.bottom);
	}
}

namespace
{
// warning C4244: 'from' から 'to' に変換しました。データが失われているかもしれません。
#pragma warning( disable : 4244 )

	template < typename To > inline static bool ObjectToPOD(IUnknown* obj, To& result) throw()
	{
		if(string str = cast(obj))
		{
			StringToPOD(str.str(), result);
			return true;
		}
		return false;
	}

	/// int/real
	template < typename To > inline static void ConvertPOD(TypeCode from, const variant::Union& var, To& result)
	{
#define TO_ARITHMETIC(type_t) { result = *(type_t*)var.buffer; return; }
		switch(from)
		{
		case TypeNull:
			result = 0;
			return;
		case TypeUnknown:
			if(ObjectToPOD<To>(var.unknown, result))
				return;
			break;
		MEW_EXPAND_INTRINSIC(TO_ARITHMETIC)
		}
		throw CastError(string::load(IDS_ERR_VARIANT_CONVERSION, from, TypeCode(VariantType<To>::Code)));
	}

	/// bool
	template <> inline static void ConvertPOD<bool>(TypeCode from, const variant::Union& var, bool& result) throw(...)
	{
#define NOT_ZERO(type_t)	{ result = (*(type_t*)var.buffer != 0); return; }
		switch(from)
		{
		case TypeNull: // Null ⇒ false
			result = false;
			return;
		case TypeUnknown: // 0 ⇒ false
			if(string str = cast(var.unknown))
			{
				StringToPOD(str.str(), result);
				return;
			}
			result = (var.unknown != 0);
			return;
		MEW_EXPAND_INTRINSIC(NOT_ZERO)
		}
		throw CastError(string::load(IDS_ERR_VARIANT_CONVERSION, from, TypeCode(VariantType<bool>::Code)));
	}

	/// Size
	inline static void ConvertPOD(TypeCode from, const variant::Union& var, Size& result) throw(...)
	{
		using TResult = Size;
		switch(from)
		{
		case TypeNull:
			result = TResult::Zero;
			return;
		case TypeSize:  // SizeとPointはメモリイメージが共通
		case TypePoint:
			memcpy(&result, var.buffer, sizeof(result));
			return;
		case TypeUnknown:
			if(ObjectToPOD<TResult>(var.unknown, result))
				return;
			break;
		}
		throw CastError(string::load(IDS_ERR_VARIANT_CONVERSION, from, TypeCode(VariantType<TResult>::Code)));
	}

	/// Point
	inline static void ConvertPOD(TypeCode from, const variant::Union& var, Point& result) throw(...)
	{
		using TResult = Point;
		switch(from)
		{
		case TypeNull:
			result = TResult::Zero;
			return;
		case TypeSize:	// SizeとPointはメモリイメージが共通
		case TypePoint:
			memcpy(&result, var.buffer, sizeof(result));
			return;
		case TypeUnknown:
			if(ObjectToPOD<TResult>(var.unknown, result))
				return;
			break;
		}
		throw CastError(string::load(IDS_ERR_VARIANT_CONVERSION, from, TypeCode(VariantType<TResult>::Code)));
	}

	/// Rect
	inline static void ConvertPOD(TypeCode from, const variant::Union& var, Rect& result) throw(...)
	{
		using TResult = Rect;
		switch(from)
		{
		case TypeNull:
			result = TResult::Zero;
			return;
		case TypeRect:
		{
			INT16* buf = (INT16*)var.buffer;
			result.assign(buf[0], buf[1], buf[2], buf[3]);
			return;
		}
		case TypeUnknown:
			if(ObjectToPOD<TResult>(var.unknown, result))
				return;
			break;
		}
		throw CastError(string::load(IDS_ERR_VARIANT_CONVERSION, from, TypeCode(VariantType<TResult>::Code)));
	}

	/// Color
	inline static void ConvertPOD(TypeCode from, const variant::Union& var, Color& result) throw(...)
	{
		using TResult = Color;
		switch(from)
		{
		case TypeNull:
			result = (Gdiplus::ARGB)TResult::Black;
			return;
		case TypeColor:
			memcpy(&result, var.buffer, sizeof(result));
			return;
		case TypeUnknown:
			if(ObjectToPOD<TResult>(var.unknown, result))
				return;
			break;
		}
		throw CastError(string::load(IDS_ERR_VARIANT_CONVERSION, from, TypeCode(VariantType<TResult>::Code)));
	}
#pragma warning( default : 4244 )
}

namespace mew
{
	void variant::ToPOD(TypeCode code, void* data, size_t size) const
	{
#define CONVERT_POD(type_t)	{ ASSERT(size == sizeof(type_t)); ConvertPOD(m_type, m_var, *(type_t*)data); return; }
		switch(code)
		{
		MEW_EXPAND_ALL(CONVERT_POD)
		default:
			switch(m_type)
			{
			case TypeUnknown:
				break;
			default:
				if(code == m_type)
				{
					ASSERT(size <= INTERNAL_BUFFER_SIZE);
					memcpy(data, m_var.buffer, size);
					return;
				}
			}
		}
		throw CastError(string::load(IDS_ERR_VARIANT_CONVERSION, m_type, code));
	}
	void variant::ToUnknown(REFINTF ppInterface) const
	{
		switch(m_type)
		{
		case TypeNull:
			*ppInterface.pp = 0;
			return;
		case TypeUnknown:
			if(m_var.unknown)
			{
				if(SUCCEEDED(m_var.unknown->QueryInterface(ppInterface.iid, (void**)ppInterface.pp)))
				{
					return;
				}
				else if(ppInterface.iid == __uuidof(IString))
				{
					ObjectToString((IString**)ppInterface.pp, m_var.unknown);
					return;
				}
			}
			*ppInterface.pp = 0;
			return;
		default: 
			if(ppInterface.iid == __uuidof(IString))
			{
#define TO_STRING(type_t)	{ CreateString((IString**)ppInterface.pp, ToString<type_t>(*(type_t*)m_var.buffer)); return; }
				switch(m_type)
				{
				MEW_EXPAND_INTRINSIC(TO_STRING)
				MEW_CASE(TO_STRING, Size)
				MEW_CASE(TO_STRING, Point)
				MEW_CASE(TO_STRING, Color)
				case TypeRect:
				{
					const INT16* s = (const INT16*)m_var.buffer;
					WCHAR buffer[160];
					_snwprintf(buffer, 160, L"%d %d %d %d", (int)s[0], (int)s[1], (int)s[2], (int)s[3]);
					CreateString((IString**)ppInterface.pp, buffer);
					return;
				}
				default:
					*ppInterface.pp = string::format(_T("internal [type=$1]"), m_type).detach();
					return;
				}
			}
		}
		throw CastError(string::load(IDS_ERR_VARIANT_CONVERSION, m_type, ppInterface.iid));
	}
	void variant::FromPOD(TypeCode code, const void* data, size_t size)
	{
		switch(code)
		{
		case TypeRect:
		{
			INT32* s = (INT32*)data;
			INT16* d = (INT16*)m_var.buffer;
			for(int i = 0; i < 4; ++i) { d[i] = (INT16)s[i]; }
			break;
		}
		default:
			if(size > INTERNAL_BUFFER_SIZE)
				throw ArgumentError(L"POD size is too large.");
			memcpy(m_var.buffer, data, size);
			break;
		}
		m_type = code;
	}
	void variant::FromVariant(const variant& rhs) throw()
	{
		switch(rhs.m_type)
		{
		case TypeUnknown:
			m_type = rhs.m_type;
			m_var.unknown = rhs.m_var.unknown;
			if(m_var.unknown)
				m_var.unknown->AddRef();
			break;
		default:
			memcpy(this, &rhs, sizeof(variant));
			break;
		}
	}
	MEW_API void variant::MakeEmpty() throw()
	{
		switch(m_type)
		{
		case TypeUnknown:
			if(m_var.unknown)
			{
				IUnknown* tmp = m_var.unknown;
				m_var.unknown = 0;
				tmp->Release();
			}
			break;
		default:
			break;
		}
	}
	void variant::load(IStream& stream)
	{
#define VAR_LOAD(type_t) { io::StreamReadExact(&stream, m_var.buffer, sizeof(type_t)); break; }
		MakeEmpty();
		try
		{
			stream >> m_type;
			switch(m_type)
			{
			case TypeNull:
				break;
			case TypeUnknown:
			{
				ref<IUnknown> unk;
				stream >> unk;
				if FAILED(unk.copyto(&m_var.unknown))
					m_type = TypeNull;
				break;
			}
			MEW_EXPAND_ALL(VAR_LOAD)
			default:
				io::StreamReadExact(&stream, m_var.buffer, INTERNAL_BUFFER_SIZE);
				break;
			}
		}
		catch(Error&)
		{
			m_type = TypeNull;
			throw;
		}
	}
	void variant::save(IStream& stream) const
	{
#define VAR_SAVE(type_t) { io::StreamWriteExact(&stream, m_var.buffer, sizeof(type_t)); break; }
		stream << m_type;
		switch(m_type)
		{
		case TypeNull:
			break;
		case TypeUnknown:
			io::StreamWriteObject(&stream, m_var.unknown);
			break;
		MEW_EXPAND_ALL(VAR_SAVE)
		default:
			io::StreamWriteExact(&stream, m_var.buffer, INTERNAL_BUFFER_SIZE);
			break;
		}
	}

	void variant::FromString(TypeCode typecode, string str)
	{
#define FROM_STRING(type_t) { StringToPOD(str.str(), *reinterpret_cast<type_t*>(m_var.buffer)); break; }
		switch(typecode)
		{
		case TypeNull:
			break;
		case TypeUnknown:
			objcpy(str, &m_var.unknown);
			break;
		MEW_EXPAND_INTRINSIC(FROM_STRING)
		MEW_CASE(FROM_STRING, Size)
		MEW_CASE(FROM_STRING, Point)
		MEW_CASE(FROM_STRING, Color)
		case TypeRect:
		{
			Rect r;
			StringToPOD(str.str(), r);
			INT32* d = (INT32*)m_var.buffer;
			d[0] = MAKELPARAM(r.left, r.top);
			d[1] = MAKELPARAM(r.right, r.bottom);
			break;
		}
		default:
			Zero();
			throw CastError(string::load(IDS_ERR_VARIANT_CONVERSION, str, typecode));
		}
		m_type = typecode;
#undef FROM_STRING
	}
}
