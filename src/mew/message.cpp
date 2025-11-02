// message.cpp

#include "stdafx.h"
#include "private.h"
#include "message.hpp"
#include "io.hpp"
#include "std/map.hpp"

//==============================================================================

namespace mew {

/// メッセージ.
class __declspec(uuid("50E61566-66FC-448C-8760-7B6B09511C5D")) Message;

class Message : public Root<implements<IMessage, ISerializable> > {
 private:
  using VariantAlloc = std::allocator<std::pair<const Guid, variant> >;
  using VariantMap = std::map<Guid, variant, std::less<Guid>, VariantAlloc>;

 private:
  VariantMap m_vars;

  class EnumVariant : public Root<implements<IEnumVariant> > {
   private:
    Message* m_owner;
    VariantMap::const_iterator m_iter;

   public:
    EnumVariant(Message* owner) : m_owner(owner) { Reset(); }
    bool Next(GUID* key, variant* var) {
      if (m_iter == m_owner->m_vars.end()) {
        return false;
      }
      if (key) {
        *key = m_iter->first;
      }
      if (var) {
        *var = m_iter->second;
      }
      ++m_iter;
      return true;
    }
    void Reset() { m_iter = m_owner->m_vars.begin(); }
  };

 public:  // Object
  Message() {}
  Message(INT32 code) {
    if (code != 0) {
      m_vars[GUID_Verb] = code;
    }
  }
  Message(VariantMap& vars) { m_vars.swap(vars); }
  void __init__(IUnknown* arg) {
    if (arg) {
      Stream stream(__uuidof(io::Reader), arg);
      Deserialize(stream);
    }
  }
  void Dispose() noexcept { m_vars.clear(); }

 public:  // ISerializable
  REFCLSID get_Class() noexcept { return __uuidof(this); }
  void Deserialize(IStream& stream) { stream >> m_vars; }
  void Serialize(IStream& stream) { stream << m_vars; }

 public:  // IMessage
  const variant& Get(const Guid& key) noexcept {
    VariantMap::const_iterator i = m_vars.find(key);
    if (i == m_vars.end()) {
      return variant::null;
    } else {
      return i->second;
    }
  }
  void Set(const Guid& key, const variant& var) noexcept {
    if (var.empty()) {
      m_vars.erase(key);
    } else {
      m_vars[key] = var;
    }
  }
  ref<IEnumVariant> Enumerate() noexcept {
    if (m_vars.empty()) {
      return null;
    } else {
      return objnew<EnumVariant>(this);
    }
  }
};

void CreateMessage(IMessage** ppMessage, EventCode code, IUnknown* arg) {
  ASSERT(ppMessage);
  *ppMessage = new Message(code);
}

AVESTA_EXPORT(Message)

}  // namespace mew

//==============================================================================

HRESULT mew::LoadPersistMessage(IPersistMessage* obj, const message& msg, const char* key) {
  if (!obj) {
    return E_POINTER;
  }
  message data = msg[key];
  if (!data) {
    return E_FAIL;
  }
  try {
    obj->LoadFromMessage(data);
    return S_OK;
  } catch (exceptions::Error& e) {
    return e.Code;
  }
}
HRESULT mew::SavePersistMessage(IPersistMessage* obj, message& msg, const char* key) {
  if (!obj) {
    return E_POINTER;
  }
  try {
    msg[key] = obj->SaveToMessage();
    return S_OK;
  } catch (exceptions::Error& e) {
    return e.Code;
  }
}
