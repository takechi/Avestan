// messenger.cpp

#include "stdafx.h"
#include "private.h"
#include "signal.hpp"
#include "std/map.hpp"

namespace mew {

class Messenger : public Root< implements<IMessenger, ISignal, IDisposable> >
{
private:
	/// 関数と引数のセット.
	class Closure
	{
	public:
		message		msg;

	private:
		function	m_function;

	public:
		Closure(function fn, const message& msg) : m_function(fn), msg(msg)
		{
			ASSERT(m_function);
		}
		HRESULT fire(const message& msg)
		{
			if(empty())
				return S_FALSE;
			HRESULT hr = m_function(msg);
			if FAILED(hr)
				clear();
			return hr;
		}
		bool equals(const function& p) const throw()	{ return m_function == p; }
		bool equals(IUnknown* p) const throw()			{ return objcmp(p, m_function.target); }
		void clear() throw()							{ m_function.clear(); msg.clear(); }
		bool empty() const throw()						{ return !m_function; }
	};
	typedef std::multimap<EventCode, Closure>	Map;
	typedef Map::iterator						iterator;
	typedef std::pair<iterator, iterator>		range_type;
	struct value_type : public std::pair<EventCode, Closure>
	{
		typedef std::pair<EventCode, Closure> super;
		value_type(const function& fn, EventCode code, const message& msg) : super(code, Closure(fn, msg)) {}
	};

	CriticalSection	m_cs;
	Map	m_map;
	volatile size_t	m_depth;	// 呼び出し深さがゼロの場合のみ安全に削除できるため、呼び出し深さを記録する
	volatile bool	m_removed;	// 多少なりとも削除チェック回数を減らすためのフラグ

public:
	class Invoker : public Root< implements<IMessage, ISerializable> >
	{
	private:
		Messenger *		m_msgr;
		const range_type	m_range;
		message				m_orig, m_args;

		iterator begin() const throw()	{ return m_range.first; }
		iterator end() const throw()	{ return m_range.second; }

		class EnumVariantTwo : public Root< implements<IEnumVariant> >
		{
		private:
			ref<IEnumVariant>	m_first, m_second;
		public:
			EnumVariantTwo(IEnumVariant* first, IEnumVariant* second) : m_first(first), m_second(second)
			{
				ASSERT(m_first);
				ASSERT(m_second);
			}
			bool Next(GUID* key, variant* var)
			{
				if(m_first->Next(key, var))
					return true;
				return m_second->Next(key, var);
			}
			void Reset()
			{
				m_first->Reset();
				m_second->Reset();
			}
		};

	public:
		Invoker(Messenger* msgr, range_type range) : m_msgr(msgr), m_range(range)	{}
		void Dispose() throw()
		{
			m_orig.clear();
			m_args.clear();
			m_msgr.clear();
		}
		HRESULT Send(message msg)
		{
			struct MessageLock
			{
				message&	m_msg;
				MessageLock(message& msgref, const message& msg) : m_msg(msgref)
				{
					m_msg = msg;
				}
				~MessageLock()
				{
					m_msg.clear();
				}
			};
			struct InvokeLock
			{
				Messenger *	m_msgr;
				message&		m_msg;
				InvokeLock(Messenger * msgr, message& msgref, const message& msg) : m_msgr(msgr), m_msg(msgref)
				{
					ASSERT(m_msgr);
					m_msgr->BeginInvoke();
					m_msg = msg;
				}
				~InvokeLock()
				{
					m_msg.clear();
					m_msgr->EndInvoke();
				}
			};

			try
			{
				InvokeLock invoke(m_msgr, m_args, msg);
				for(iterator i = begin(); i != end(); ++i)
				{
					if(i->second.empty())
						continue;
					MessageLock lock(m_orig, i->second.msg);
					i->second.fire(this);
				}
			}
			catch(Error&)
			{
				throw;
			}
			catch(...)
			{	// FIXME:
				ASSERT(!"segfault on Invoker.Send(msg)");
			}
			return S_OK;
		}

	public: // ISerializable
		REFCLSID get_Class() throw() { return GUID_NULL; }
		void Serialize(IStream& stream)
		{
			throw exceptions::LogicError(L"Invoker does not support serialize.", E_NOTIMPL);
		}

	public: // IMesasge
		const variant& Get(const Guid& key) throw()
		{
			if(m_orig)
			{
				const variant& var = m_orig->Get(key);
				if(!var.empty())
					return var;
			}
			if(m_args)
			{
                return m_args->Get(key);
			}
			return variant::null;
		}
		void Set(const Guid& key, const variant& var) throw()
		{
			if(m_args)
				m_args->Set(key, var);
		}
		ref<IEnumVariant> Enumerate() throw()
		{
			ref<IEnumVariant> orig, args;
			if(m_orig) { orig = m_orig->Enumerate(); }
			if(m_args) { args = m_args->Enumerate(); }
			if(!orig)     return args;
			else if(args) return ref<IEnumVariant>(new EnumVariantTwo(orig, args));
			else          return orig;
		}
	};

public:
	void BeginInvoke() throw()
	{
		m_cs.Lock();
		++m_depth;
	}
	void EndInvoke() throw()
	{
		--m_depth;
		if(m_depth == 0 && m_removed)
		{
			m_removed = false;
			for(iterator i = m_map.begin(); i != m_map.end();)
			{
				if(i->second.empty())
					i = m_map.erase(i);
				else
					++i;
			}
		}
		m_cs.Unlock();
	}
	void __init__(IUnknown* arg)
	{
		ASSERT(!arg);
		m_depth = 0;
		m_removed = false;
	}
	void Dispose() throw()
	{
		try
		{
			if(m_depth == 0)
				m_map.clear();
			else
			{
				for(Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
				{
					i->second.clear();
				}
			}
		}
		catch(...)
		{	// FIXME:
			ASSERT(!"segfault on Messenger.Dispose(msg)");
		}
	}

public: // ISignal
	HRESULT Connect(EventCode code, function fn, message msg = null) throw()
	{
		if(!fn)
			return E_INVALIDARG;
		iterator i = Find(code, fn);
		if(i == m_map.end())
		{	// new fn
			m_map.insert(value_type(fn, code, msg));
		}
		else
		{	// existing fn
			TRACE(_T("Connect : replace ($1 ⇒ $2)"), i->second.msg, msg);
			i->second.msg = msg;
		}
		return S_OK;
	}
	size_t Disconnect(EventCode code, function fn, IUnknown* obj = null) throw()
	{
		AutoLock lock(m_cs);
		if(!fn)
		{
			if(code == 0)
				return RemoveFunc(obj);
			else if(!obj)
				return RemoveCode(code);
			else
				return RemoveBoth(code, obj);
		}
		else
		{
			if(code == 0)
				return RemoveFunc(fn);
			else
				return RemoveBoth(code, fn);
		}
	}

public: // IMessenger
	function Invoke(EventCode code) throw()
	{
		range_type range = m_map.equal_range(code);
		if(range.first == range.second)
			return null;
		return function(objnew<Invoker>(this, range), &Invoker::Send);
	}

private:
	iterator Find(EventCode code, function fn) throw()
	{
		range_type range = m_map.equal_range(code);
		for(iterator i = range.first; i != range.second; ++i)
		{
			if(i->second.equals(fn))
			{
				return i;
			}
		}
		return m_map.end();
	}
	void RemoveAll() throw()
	{
		AutoLock lock(m_cs);
		if(m_depth == 0)
		{
			m_map.clear();
		}
		else
		{
			for(iterator i = m_map.begin(); i != m_map.end(); ++i)
			{
				i->second.clear();
			}
			m_removed = true;
		}
	}
	template < class T >
	size_t RemoveRange(range_type range, const T& obj) throw()
	{
		size_t num = 0;
		if(m_depth == 0)
		{
			for(iterator i = range.first; i != range.second;)
			{
				if(i->second.equals(obj))
				{
					i = m_map.erase(i);
					++num;
				}
				else
					++i;
			}
		}
		else
		{
			for(iterator i = range.first; i != range.second; ++i)
			{
				if(i->second.equals(obj))
				{
					i->second.clear();
					++num;
				}
			}
			m_removed = (m_removed || num != 0);
		}
		return num;
	}
	template < class T >
	size_t RemoveFunc(const T& obj) throw()
	{
		return RemoveRange(range_type(m_map.begin(), m_map.end()), obj);
	}
	template < class T >
	size_t RemoveBoth(EventCode code, const T& obj)
	{
		return RemoveRange(m_map.equal_range(code), obj);
	}
	size_t RemoveCode(EventCode code) throw()
	{
		if(m_depth == 0)
		{
			return m_map.erase(code);
		}
		else
		{
			size_t num = 0;
			range_type range = m_map.equal_range(code);
			for(iterator i = range.first; i != range.second; ++i)
			{
				i->second.clear();
				++num;
			}
			m_removed = (m_removed || num != 0);
			return num;
		}
	}
};

} 

AVESTA_EXPORT( Messenger )
