/// @file object.hpp
/// IUnknown実装ヘルパ.
#pragma once

#include "error.hpp"
#include "mew.hpp"

namespace mew {
//==============================================================================
// implements.

template <class T0 = meta::Void, class T1 = meta::Void, class T2 = meta::Void, class T3 = meta::Void, class T4 = meta::Void,
          class T5 = meta::Void, class T6 = meta::Void, class T7 = meta::Void, class T8 = meta::Void, class T9 = meta::Void>
struct implements {
  using Result = meta::Typelist<T0, typename implements<T1, T2, T3, T4, T5, T6, T7, T8, T9>::Result>;
};

template <>
struct implements<PP_CSV0(10, meta::Void)> {
  using Result = meta::Void;
};

//==============================================================================
// mixin.

template <template <typename> class T0 = meta::Void1, template <typename> class T1 = meta::Void1,
          template <typename> class T2 = meta::Void1, template <typename> class T3 = meta::Void1,
          template <typename> class T4 = meta::Void1, template <typename> class T5 = meta::Void1,
          template <typename> class T6 = meta::Void1, template <typename> class T7 = meta::Void1,
          template <typename> class T8 = meta::Void1, template <typename> class T9 = meta::Void1>
struct mixin {
  template <typename TBase>
  struct Result1 {
    using Result = T0<typename mixin<T1, T2, T3, T4, T5, T6, T7, T8, T9>::template Result1<TBase>::Result>;
  };
};

template <>
struct mixin<PP_CSV0(10, meta::Void1)> {
  template <typename TBase>
  struct Result1 {
    using Result = TBase;
  };
};
}  // namespace mew

#include "detail/objectimpl.hpp"

namespace mew {
//==============================================================================
// 生存期間管理.

/// スタック上に作られるオブジェクトの生存期間管理.
template <class TBase>
class __declspec(novtable) StaticLife : public TBase {
 public:
  ULONG __stdcall AddRef() noexcept { return 1; }
  ULONG __stdcall Release() noexcept { return 1; }
};

/// ヒープ上に作られるオブジェクトの生存期間管理.
template <class TBase>
class __declspec(novtable) DynamicLife : public TBase {
 protected:
  volatile LONG m_refcount;
  DynamicLife() noexcept : m_refcount(1) {}
  virtual ~DynamicLife() noexcept {
    DEBUG_ONLY(if (m_refcount != 0) { __debugbreak(); });
  }

 private:  // non-copyable
  DynamicLife(const DynamicLife&);
  DynamicLife& operator=(const DynamicLife&);

 public:
  ULONG __stdcall AddRef() noexcept {
    DEBUG_ONLY(if (m_refcount <= 0) { __debugbreak(); });
    return ::InterlockedIncrement(&m_refcount);
  }
  ULONG __stdcall Release() noexcept {
    DEBUG_ONLY(if (m_refcount <= 0) { __debugbreak(); });
    ULONG refcount = ::InterlockedDecrement(&m_refcount);
    if (refcount == 0) {
      Dispose();
      __free__();
    }
    return refcount;
  }

 public:  // overridable
  template <typename T>
  static T* __new__(IUnknown* arg) {
    return new T();
  }
  void __init__(IUnknown* arg) {}
  virtual void Dispose() noexcept {}
  virtual void __free__() noexcept { delete this; }
};

//==============================================================================
// IUnknown実装ベース.

template <typename TBase, typename TImplements = implements<>, typename TMixin = mixin<> >
class __declspec(novtable) Object : public detail::Object1<TBase, TImplements, TMixin>::Result {
  // typedef meta::Typelist __inherits__; // C++言語的に継承しているインタフェース.
  // typedef meta::Typelist __supports__; // COMオブジェクトとしてサポートするインタフェース.
  // typedef ... __primary__; // 最も「左側」のインタフェース.

  // IUnknown* OID; // COMアイデンティティとしてのIUnknownポインタ.

  // HRESULT __stdcall QueryInterface(REFIID iid, void** pp) noexcept;
  // HRESULT __stdcall QueryInterface(REFINTF pp) noexcept;
  // ULONG __stdcall AddRef() noexcept;
  // ULONG __stdcall Release() noexcept;
};

template <typename TImplements, typename TMixin = mixin<DynamicLife> >
class __declspec(novtable) Root : public detail::Object1<meta::Void, TImplements, TMixin>::Result {};

//==============================================================================

template <class T, const char* Name = (const char*)0>
class DefaultFactory {
 public:
  DefaultFactory() { mew::RegisterFactory(__uuidof(T), &CreateInstance); }
  static void CreateInstance(REFINTF pp, IUnknown* arg) throw(...) {
    T* obj = T::template __new__<T>(arg);
    VERIFY_HRESULT(obj->QueryInterface(pp.iid, pp.pp));
    try {
      obj->__init__(arg);
      DEBUG_ONLY(mew::RegisterInstance(obj->OID, Name););
      obj->Release();
    } catch (...) {
      obj->Release();
      static_cast<IUnknown*>(*pp.pp)->Release();
      *pp.pp = null;
      throw;
    }
  }
};

template <class T, void (*fnCreateInstance)(REFINTF, IUnknown*)>
class FunctionFactory {
 public:
  FunctionFactory() { mew::RegisterFactory(__uuidof(T), fnCreateInstance); }
};

#ifdef _DEBUG
#define AVESTA_EXPORT(Name)             \
  char creatable_name_##Name[] = #Name; \
  static mew::DefaultFactory<Name, creatable_name_##Name> creatable_##Name;
#else
#define AVESTA_EXPORT(Name) static mew::DefaultFactory<Name> creatable_##Name;
#endif

#define AVESTA_EXPORT_FUNC(Name) static mew::FunctionFactory<Name, Create##Name> creatable_##Name;
}  // namespace mew
