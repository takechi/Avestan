// buffer.hpp
#pragma once

#include "math.hpp"

namespace mew
{
	/// BufferT.
	template < class T, class TAlloc = ATL::CCRTAllocator >
	class BufferT : public CHeapPtr<T, TAlloc>
	{
		typedef CHeapPtr<T, TAlloc> super;
	private:
		size_t	m_length;
		size_t	m_capacity;

	public:
		BufferT() throw() : m_length(0), m_capacity(0) {}
		BufferT(const BufferT& rhs) throw() : m_length(0), m_capacity(0)
		{
			append(rhs, rhs.size());
		}
		BufferT& operator = (const BufferT& rhs) throw()
		{
			clear();
			append(rhs, rhs.size());
			return *this;
		}
	public:
		T* data() throw()				{ return m_pData; }
		const T* data() const throw()	{ return m_pData; }
		void push_back(T c) throw()
		{
			reserve(m_length + 1);
			m_pData[m_length] = c;
			++m_length;
		}
		void append(T c) throw()
		{
			push_back(c);
		}
		void append(const T* data, size_t len) throw()
		{
			write(m_length, data, len);
		}
		size_t read(size_t pos, T* data, size_t len)
		{
			if(pos >= m_length)
				return 0;
			if(pos + len >= m_length)
				len = m_length - pos;
			memcpy(data, m_pData + pos, len * sizeof(T));
			return len;
		}
		void write(size_t pos, const T* data, size_t len)
		{
			if(len == 0)
				return;
			if(m_length < pos + len)
			{
				size_t length = m_length;
				resize(pos + len);
				if(pos > length)
				{	// 末尾を越えた書き込みの場合、「穴」をゼロで埋める。
					memset(m_pData + length, 0, (pos - length) * sizeof(T));
				}
			}
			memcpy(m_pData + pos, data, len * sizeof(T));
		}
		bool empty() const throw()
		{
			return m_length == 0;
		}
		size_t size() const throw()
		{
			return m_length;
		}
		void resize(size_t sz) throw()
		{
			reserve(sz);
			m_length = sz;
		}
		void reserve(size_t capacity) throw()
		{
			if(m_capacity >= capacity)
				return;
			m_capacity = math::max(static_cast<size_t>(16), capacity, m_capacity*2);
			Reallocate(m_capacity);
		}
		void clear() throw()
		{
			m_length = 0;
		}

		friend inline IStream& operator >> (IStream& stream, BufferT& v) throw(...)
		{
			size_t size;
			stream >> size;
			v.resize(size);
			io::StreamReadExact(&stream, v, size * sizeof(T));
			return stream;
		}

		friend inline IStream& operator << (IStream& stream, const BufferT& v) throw(...)
		{
			stream << v.size();
			io::StreamWriteExact(&stream, v, v.size() * sizeof(T));
			return stream;
		}
	};

	/// StringBufferT.
	template < class T, class TAlloc = ATL::CCRTAllocator >
	class StringBufferT : public BufferT<T, TAlloc>
	{
		typedef BufferT<T, TAlloc> super;
	public:
		const T* str() const	{ return empty() ? 0 : data(); }
		using super::append;
		/// 末端のNULL文字は追加しない.
		void append(const T* str)
		{
			size_t length = str::length(str);
			size_t sz = size();
			resize(sz + length);
			for(size_t i = 0; i < length; ++i)
				data()[sz+i] = str[i];
		}
		void append_path(const T* path)
		{
			if(str::find(path, T(' ')))
			{	// スペースを含む場合
				append(T('"'));
				append(path);
				append(T('"'));
			}
			else
			{	// スペースを含まない場合
				append(path);
			}
		}
		void terminate()
		{
			push_back(T('\0'));
		}
		StringBufferT& operator << (const T* str)	{ append(str); return *this; }
		StringBufferT& operator << (T c)			{ append(c); return *this; }
	};
	typedef StringBufferT<TCHAR>	StringBuffer;
}
