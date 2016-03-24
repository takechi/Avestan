// objectimpl.hpp
#pragma once

namespace mew
{
	namespace detail
	{
		using namespace meta;

		inline HRESULT InterfaceAssign(IUnknown* p, void** pp)
		{
			*pp = p;
			p->AddRef();
			return S_OK;
		}

		//==============================================================================

		template < class TInherits, class TSupports >
		class __declspec(novtable) Interface : public Inherits<TInherits>
		{
		public:
			typedef typename TInherits::Head __primary__; ///< OIDに対応するインタフェース.
			IUnknown* get_OID() throw()	{ return static_cast<IUnknown*>(static_cast<__primary__*>(this)); }
			__declspec(property(get=get_OID)) IUnknown* OID;

		public: // IUnknown
			HRESULT __stdcall QueryInterface(REFIID iid, void** pp) throw()
			{
				return InternalQueryInterface<TSupports>(iid, pp);
			}
			ULONG __stdcall AddRef() throw() = 0;
			ULONG __stdcall Release() throw() = 0;

		private:
			template < typename Interface > Interface* InternalGetInterface()
			{
				STATIC_ASSERT( SUPERSUBCLASS(IUnknown, Interface) );
				typedef typename MostDerived<TInherits, Interface>::Result Derived;
				return static_cast<Interface*>(static_cast<Derived*>(this));
			}
			template < typename TList >
			HRESULT __stdcall InternalQueryInterface(REFIID iid, void** pp) throw()
			{
				if(iid == __uuidof(TList::Head))
					return InterfaceAssign(InternalGetInterface<TList::Head>(), pp);
				else
					return InternalQueryInterface<TList::Tail>(iid, pp);
			}
			template <>
			HRESULT __stdcall InternalQueryInterface<Void>(REFIID iid, void** pp) throw()
			{
				if(iid == __uuidof(IUnknown))
					return InterfaceAssign(OID, pp);
				else
				{
					*pp = null;
					return E_NOINTERFACE;
				}
			}
		};

		//==============================================================================

		template < class TBase, class TSupports, typename TMixin >
		class Object2
		{
			typedef typename MostDerivedOnly<TSupports>::Result	TInherits;
			typedef typename TMixin::Result1< Interface<TInherits, TSupports> >::Result	mixin;
		public:
			class __declspec(novtable) Result : public TBase, public mixin
			{
				typedef TBase	TLeft;
				typedef mixin	TRight;
			public:
				typedef Union<typename TLeft::__inherits__, TInherits>	__inherits__;
				typedef Union<typename TLeft::__supports__, TSupports>	__supports__;

				// IUnknownの実装はすべてTLeftへ転送する。
				typedef typename TLeft::__primary__	__primary__;
				HRESULT __stdcall QueryInterface(REFIID iid, void** pp) throw()
				{
					HRESULT hr = TLeft::QueryInterface(iid, pp);
					if(hr != E_NOINTERFACE)
						return hr;
					else
						return TRight::QueryInterface(iid, pp);
				}
				ULONG __stdcall AddRef() throw()	{ return TLeft::AddRef(); }
				ULONG __stdcall Release() throw()	{ return TLeft::Release(); }
				IUnknown* get_OID() throw()			{ return TLeft::get_OID(); }
				__declspec(property(get=get_OID)) IUnknown* OID;
			};
		};

		template < class TSupports, typename TMixin >
		class Object2<Void, TSupports, TMixin>
		{
			typedef typename MostDerivedOnly<TSupports>::Result	TInherits;
			typedef typename TMixin::Result1< Interface<TInherits, TSupports> >::Result	mixin;
		public:
			// これがルートクラス.
			class __declspec(novtable) Result : public mixin
			{
			public:
				typedef TInherits	__inherits__;
				typedef TSupports	__supports__;
				HRESULT __stdcall QueryInterface(REFIID iid, void** pp) throw()
				{
					if(!pp)
						return E_INVALIDARG;
					return __super::QueryInterface(iid, pp);
				}
				HRESULT __stdcall QueryInterface(REFINTF pp) throw()
				{
					return QueryInterface(pp.iid, pp.pp);
				}

				DEBUG_ONLY( ~Result() { mew::UnregisterInstance(OID); } )
			};
		};

		template < class TBase, typename TMixin >
		class Object2<TBase, Void, TMixin>
		{
		public:
			typedef typename TMixin::Result1<TBase>::Result Result;
		};

		template < typename TMixin >
		class Object2<Void, Void, TMixin>
		{
		public:
			typedef typename TMixin::Result1<Void>::Result Result;
		};

		//==============================================================================

		template < class TBase, typename TImplements, typename TMixin >
		class Object1
		{
			typedef typename meta::DerivedToFront<typename TImplements::Result>::Result	Sorted;
			typedef typename Subtract<Sorted, typename TBase::__supports__>::Result		Avail;
		public:
			typedef typename Object2<TBase, Avail, TMixin>::Result Result;
		};

		template < class TBase, typename TMixin >
		class Object1<TBase, implements<>, TMixin>
		{
		public:
			typedef typename Object2<TBase, Void, TMixin>::Result Result;
		};

		template < typename TImplements, typename TMixin >
		class Object1<Void, TImplements, TMixin>
		{
			typedef typename meta::DerivedToFront<typename TImplements::Result>::Result	Avail;
		public:
			typedef typename Object2<Void, Avail, TMixin>::Result Result;
		};

		template < typename TMixin >
		class Object1<Void, implements<>, TMixin>
		{
		public:
			typedef typename Object2<Void, Void, TMixin>::Result Result;
		};
	}
}
