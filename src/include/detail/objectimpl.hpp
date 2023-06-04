// objectimpl.hpp
#pragma once

namespace mew {
namespace detail {

inline HRESULT InterfaceAssign(IUnknown* p, void** pp) {
  *pp = p;
  p->AddRef();
  return S_OK;
}

//==============================================================================

template <class TInherits, class TSupports>
class __declspec(novtable) Interface : public meta::Inherits<TInherits> {
 public:
  using __primary__ = typename TInherits::Head;  ///< OIDに対応するインタフェース.
  IUnknown* get_OID() throw() { return static_cast<IUnknown*>(static_cast<__primary__*>(this)); }
  __declspec(property(get = get_OID)) IUnknown* OID;

 public:  // IUnknown
  HRESULT __stdcall QueryInterface(REFIID iid, void** pp) throw() { return InternalQueryInterface<TSupports>(iid, pp); }
  ULONG __stdcall AddRef() throw() = 0;
  ULONG __stdcall Release() throw() = 0;

 private:
  template <typename Interface>
  Interface* InternalGetInterface() {
    STATIC_ASSERT(SUPERSUBCLASS(IUnknown, Interface));
    using Derived = typename meta::MostDerived<TInherits, Interface>::Result;
    return static_cast<Interface*>(static_cast<Derived*>(this));
  }
  template <typename TList>
  HRESULT __stdcall InternalQueryInterface(REFIID iid, void** pp) throw() {
    if (iid == __uuidof(TList::Head)) {
      return InterfaceAssign(InternalGetInterface<TList::Head>(), pp);
    } else {
      return InternalQueryInterface<TList::Tail>(iid, pp);
    }
  }
  template <>
  HRESULT __stdcall InternalQueryInterface<meta::Void>(REFIID iid, void** pp) throw() {
    if (iid == __uuidof(IUnknown)) {
      return InterfaceAssign(OID, pp);
    } else {
      *pp = null;
      return E_NOINTERFACE;
    }
  }
};

//==============================================================================

template <class TBase, class TSupports, typename TMixin>
class Object2 {
  using TInherits = typename meta::MostDerivedOnly<TSupports>::Result;
  using mixin = typename TMixin::template Result1<Interface<TInherits, TSupports> >::Result;

 public:
  class __declspec(novtable) Result : public TBase, public mixin {
    using TLeft = TBase;
    using TRight = mixin;

   public:
    using __inherits__ = meta::Union<typename TLeft::__inherits__, TInherits>;
    using __supports__ = meta::Union<typename TLeft::__supports__, TSupports>;

    // IUnknownの実装はすべてTLeftへ転送する。
    using __primary__ = typename TLeft::__primary__;
    HRESULT __stdcall QueryInterface(REFIID iid, void** pp) throw() {
      HRESULT hr = TLeft::QueryInterface(iid, pp);
      if (hr != E_NOINTERFACE) {
        return hr;
      } else {
        return TRight::QueryInterface(iid, pp);
      }
    }
    ULONG __stdcall AddRef() throw() { return TLeft::AddRef(); }
    ULONG __stdcall Release() throw() { return TLeft::Release(); }
    IUnknown* get_OID() throw() { return TLeft::get_OID(); }
    __declspec(property(get = get_OID)) IUnknown* OID;
  };
};

template <class TSupports, typename TMixin>
class Object2<meta::Void, TSupports, TMixin> {
  using TInherits = typename meta::MostDerivedOnly<TSupports>::Result;
  using mixin = typename TMixin::template Result1<Interface<TInherits, TSupports> >::Result;

 public:
  // これがルートクラス.
  class __declspec(novtable) Result : public mixin {
   public:
    using __inherits__ = TInherits;
    using __supports__ = TSupports;
    HRESULT __stdcall QueryInterface(REFIID iid, void** pp) throw() {
      if (!pp) {return E_INVALIDARG;}
      return __super::QueryInterface(iid, pp);
    }
    HRESULT __stdcall QueryInterface(REFINTF pp) throw() { return QueryInterface(pp.iid, pp.pp); }

    DEBUG_ONLY(~Result() { mew::UnregisterInstance(OID); })
  };
};

template <class TBase, typename TMixin>
class Object2<TBase, meta::Void, TMixin> {
 public:
  using Result = typename TMixin::template Result1<TBase>::Result;
};

template <typename TMixin>
class Object2<meta::Void, meta::Void, TMixin> {
 public:
  using Result = typename TMixin::template Result1<meta::Void>::Result;
};

//==============================================================================

template <class TBase, typename TImplements, typename TMixin>
class Object1 {
  using Sorted = typename meta::DerivedToFront<typename TImplements::Result>::Result;
  using Avail = typename meta::Subtract<Sorted, typename TBase::__supports__>::Result;

 public:
  using Result = typename Object2<TBase, Avail, TMixin>::Result;
};

template <class TBase, typename TMixin>
class Object1<TBase, implements<>, TMixin> {
 public:
  using Result = typename Object2<TBase, meta::Void, TMixin>::Result;
};

template <typename TImplements, typename TMixin>
class Object1<meta::Void, TImplements, TMixin> {
  using Avail = typename meta::DerivedToFront<typename TImplements::Result>::Result;

 public:
  using Result = typename Object2<meta::Void, Avail, TMixin>::Result;
};

template <typename TMixin>
class Object1<meta::Void, implements<>, TMixin> {
 public:
  using Result = typename Object2<meta::Void, meta::Void, TMixin>::Result;
};
}  // namespace detail
}  // namespace mew
