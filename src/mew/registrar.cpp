// registrar.cpp

#include <unordered_map>

#include "stdafx.h"
#include "private.h"

inline bool operator<(REFGUID lhs, REFGUID rhs) { return memcmp(&lhs, &rhs, sizeof(GUID)) < 0; }

//==============================================================================

namespace {

struct CLSIDHash {
  size_t operator()(const CLSID& clsid) const {
    const auto* var = reinterpret_cast<const size_t*>(&clsid);
    STATIC_ASSERT(sizeof(CLSID) == sizeof(size_t) * 2);
    return var[0] ^ var[1];
  };
};

using Registrar = std::unordered_map<CLSID, mew::FactoryProc, CLSIDHash>;
// 他のグローバル変数の初期化ルーチンから呼ばれるても良いように、
// 関数ないスタティックにして構築順序を制御する必要がある。
Registrar& GetRegistrar() {
  static Registrar theRegistrar;
  return theRegistrar;
}
}  // namespace

namespace mew {
MEW_API void CreateInstance(REFCLSID clsid, REFINTF ppInterface, IUnknown* arg) throw(...) {
  // まず、モジュールクラスマップから検索し、見つからなければCOMクラスを作成する.
  HRESULT hr;
  Registrar::const_iterator i = GetRegistrar().find(clsid);
  if (i != GetRegistrar().end()) {  // ファクトリ関数が見つかった
    i->second(ppInterface, arg);
    ASSERT(*ppInterface.pp);
    return;
  } else if (SUCCEEDED(hr = ::CoCreateInstance(clsid, null, CLSCTX_ALL, ppInterface.iid,
                                               ppInterface.pp))) {  // COMクラスの作成に成功
    TRACE(L"info: CoCreateInstance($1)", clsid);
    ASSERT(*ppInterface.pp);
    return;
  } else {  // クラスが見つからない
    throw mew::exceptions::ClassError(string::load(IDS_ERR_INVALIDCLSID, clsid), hr);
  }
}
MEW_API void RegisterFactory(REFCLSID clsid, FactoryProc factory) noexcept {
  ASSERT(GetRegistrar()[clsid] == null && "作成可能クラスの二重登録");
  GetRegistrar()[clsid] = factory;
}
}  // namespace mew
