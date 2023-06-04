// keymap.cpp

#include "stdafx.h"
#include "widgets.hpp"
#include "object.hpp"
#include "std/array_map.hpp"

namespace mew {
namespace ui {

class KeymapTable : public Root<implements<IKeymapTable, IKeymap> > {
 private:  // variables
  using Binds = mew::array_map<DWORD, ref<ICommand> >;
  Binds m_Binds;

  inline static DWORD MakeKey(WORD mod, WORD vkey) { return (DWORD)mod << 16 | (DWORD)vkey; }

 public:  // Object
  void __init__(IUnknown* arg) { ASSERT(!arg); }
  void Dispose() throw() { m_Binds.clear(); }

 public:  // IKeymap
  HRESULT OnKeyDown(IWindow* window, UINT16 modifiers, UINT8 vkey) {
    ref<ICommand> command;
    HRESULT hr;
    if FAILED (hr = GetBind(modifiers, vkey, &command)) {
      return hr;
    }
    command->Invoke();
    return S_OK;
  }

 public:  // IKeymapTable
  size_t get_Count() { return m_Binds.size(); }
  HRESULT GetBind(size_t index, UINT16* modifiers, UINT8* vkey, REFINTF pp) {
    if (index >= m_Binds.size()) {
      return E_INVALIDARG;
    }
    Binds::const_iterator i = m_Binds.at(index);
    if (modifiers) {
      *modifiers = HIWORD(i->first);
    }
    if (vkey) {
      *vkey = (UINT8)LOWORD(i->first);
    }
    return (pp.pp) ? i->second->QueryInterface(pp) : S_OK;
  }
  HRESULT GetBind(UINT16 modifiers, UINT8 vkey, REFINTF ppCommand) {
    DWORD key = MakeKey(modifiers, vkey);
    Binds::const_iterator i = m_Binds.find(key);
    if (i == m_Binds.end()) {
      return E_FAIL;
    }
    return i->second.copyto(ppCommand);
  }
  HRESULT SetBind(UINT16 modifiers, UINT8 vkey, ICommand* pCommand) {
    DWORD key = MakeKey(modifiers, vkey);
    if (!pCommand) {
      return m_Binds.erase(key) > 0 ? S_OK : E_FAIL;
    } else {
      m_Binds[key] = pCommand;
      return S_OK;
    }
  }
};

AVESTA_EXPORT(KeymapTable)

}  // namespace ui
}  // namespace mew
