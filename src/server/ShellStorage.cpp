// ShellStorage.cpp

#include "stdafx.h"
#include "main.hpp"
#include "object.hpp"
#include "storage.hpp"
#include "std/buffer.hpp"
#include <time.h>

const int DEFAULT_SETTING_EXPIRE = 7*24*60*60;

//==============================================================================

class ShellStorage : public Root< implements<IShellStorage, ISerializable> >
{
private:
	typedef BufferT<BYTE>	Buffer;
	struct Value
	{
		time_t	time;
		Buffer	buffer;

		friend inline IStream& operator >> (IStream& stream, Value& v) throw(...)
		{
			return stream >> v.time >> v.buffer;
		}

		friend inline IStream& operator << (IStream& stream, const Value& v) throw(...)
		{
			return stream << v.time << v.buffer;
		}
	};
	typedef io::StorageT<string, Value>	Storage;

	class ShellStream : public Root< implements<IStream, ISequentialStream>, mixin<io::StreamImpl, mew::DynamicLife> >
	{
	public:
		ShellStorage *	m_owner;
		const string		m_key;
		const bool			m_writable;
		Buffer&				m_buffer;
		size_t				m_pos;

		ShellStream(ShellStorage* owner, const string& key, Buffer& buffer, bool writable)
			: m_owner(owner), m_key(key), m_buffer(buffer), m_writable(writable)
		{
			m_pos = 0;
			m_buffer.reserve(200);
			if(writable)
				m_buffer.clear(); // truncate
		}
		void Dispose()
		{
			if(m_owner)
			{
				m_owner->OnStreamClose(this);
				m_owner = null;
			}
		}
		HRESULT __stdcall Read(void* data, ULONG size, ULONG* done)
		{
			size_t r = m_buffer.read(m_pos, (BYTE*)data, size);
			m_pos += r;
			if(done)
				*done = r;
			return (r > 0 ? S_OK : S_FALSE);
		}
		HRESULT __stdcall Write(const void* data, ULONG size, ULONG* done)
		{
			if(!m_writable)
				return STG_E_ACCESSDENIED;
			m_buffer.write(m_pos, (const BYTE*)data, size);
			m_pos += size;
			if(done)
				*done = size;
			return S_OK;
		}
		HRESULT __stdcall SetSize(ULARGE_INTEGER sz)
		{
			m_buffer.resize(sz.QuadPart);
			return S_OK;
		}

		virtual DWORD	GetAccessMode() const		{ return m_writable ? STGM_READWRITE : STGM_READ; }
		virtual UINT64	GetSize() const				{ return m_buffer.size(); }
		virtual UINT64	GetPosition() const			{ return m_pos; }
		virtual HRESULT	SeekTo(UINT64 where)
		{
			if(!m_writable && where > m_buffer.size())
				return E_FAIL;
			m_pos = (size_t)where;
			return S_OK;
		}
	};

private:
	Storage	m_storage;

public: // Object
	void __init__(IUnknown* arg)
	{
		if(arg)
		{
			Stream stream(__uuidof(io::Reader), arg);
			Deserialize(stream);
		}
	}
	void Dispose() throw()
	{
		m_storage.Dispose();
	}

public: // ISerializable
	REFCLSID get_Class() throw()		{ return __uuidof(this); }
	void Deserialize(IStream& stream)	{ stream >> m_storage; }

	void Serialize(IStream& stream)
	{
		Vacuum(theAvesta->GetProfileSint32(L"Misc", _T("SettingExpire"), DEFAULT_SETTING_EXPIRE));
		stream << m_storage;
	}

public: // IShellStorage
	static string RemoveLeaf(const string& path)
	{
		size_t len1 = path.length();
		TCHAR tmp[MAX_PATH];
		path.copyto(tmp, MAX_PATH);
		if(!PathRemoveFileSpec(tmp))
			return null;
		size_t len2 = str::length(tmp);
		if(len1 == len2)
			return null;
		return string(tmp, len2);
	}

	HRESULT QueryStream(IStream** ppStream, io::IEntry* pFolder, bool writable)
	{
		HRESULT hr;
		string name = pFolder->GetName(IEntry::PATH_OR_NAME);
		hr = OnStreamOpen(name, writable, ppStream);
		if(SUCCEEDED(hr) || writable)
			return hr;
		// read only の場合、見つからなければ親フォルダの設定を返す。
		while(name = RemoveLeaf(name))
		{
			hr = OnStreamOpen(name, writable, ppStream);
			switch(hr)
			{
			case STG_E_FILENOTFOUND:
			case STG_E_PATHNOTFOUND:
				continue;
			}
			break;
		}
		return hr;
	}

	struct IsDescendant
	{
		string	path;
		bool operator () (const string& key, const Value& value) const
		{
			bool ret = PathIsChild(path, key);
			if(ret)
				TRACE(L"$1 is deleted. ($2)", key, path);
			return ret;
		}
		static bool PathIsChild(const string& parent, const string& child)
		{
			size_t lenP = parent.length();
			size_t lenC = child.length();
			if(lenP == 0 || lenC <= lenP)
				return false;
			LPCWSTR strP = parent.str();
			LPCWSTR strC = child.str();
			if(str::compare_nocase(strP, strC, lenP) != 0)
				return false;
			return (strP[lenP-1] == '\\' || strC[lenP] == L'\\');
		}
	};

	HRESULT SyncDescendants(io::IEntry* pFolder)
	{
		string key = pFolder->GetName(IEntry::PATH_OR_NAME);
		IsDescendant op = { key };
		m_storage.DeleteIf(op);
		return S_OK;
	}

	struct IsExpired
	{
		time_t time;
		bool operator () (const string& key, const Value& value) const
		{
			bool ret = (value.time < time);
			if(ret)
				TRACE(L"$1 is deleted. ($2 < $3)", key, (UINT)value.time, (UINT)time);
			return ret;
		}
	};

private:
	void Vacuum(time_t expire)
	{
		time_t now = time(NULL);
		IsExpired op = { now - expire };
		m_storage.DeleteIf(op);
	}

	void OnStreamClose(ShellStream* stream)
	{
		TRACE(L"OnStreamClose($1, $2)", stream->m_key, (stream->m_writable ? L"w" : L"r"));
		m_storage.Unlock(stream->m_key);
	}
	HRESULT OnStreamOpen(const string& key, bool writable, IStream** ppStream)
	{
		Value* value;
		HRESULT hr = m_storage.Lock(key, writable, &value);
		TRACE(L"OnStreamOpen($1, $2) == $3", key, (writable ? L"w" : L"r"), (int)hr);
		if FAILED(hr)
			return hr;
		value->time = time(NULL);
		*ppStream = new ShellStream(this, key, value->buffer, writable);
		return S_OK;
	}
};

AVESTA_EXPORT( ShellStorage )
