// command.cpp

#include "stdafx.h"
#include "private.h"
#include "application.hpp"
#include "std/map.hpp"
#include "signal.hpp"

namespace mew {
// template <> struct Event<EventInvoke>
template <>
struct Event<EventQueryState> {
  static void event(message& msg, ICommand* from, IUnknown* owner) {
    msg["from"] = from;
    msg["owner"] = owner;
  }
};
}  // namespace mew

namespace {
//==============================================================================

class Command : public mew::Root<mew::implements<mew::ICommand, mew::ISignal>, mew::mixin<mew::SignalImpl, mew::DynamicLife> > {
 private:  // variables
  mew::string m_Name;
  mew::string m_Description;

 public:  // Object
  Command(mew::string name, mew::string description) : m_Name(name), m_Description(description) {
    m_msgr.create(__uuidof(mew::Messenger));
  }
  void Dispose() noexcept { m_msgr.dispose(); }

 public:  // ICommand
  mew::string get_Description() { return m_Description; }
  UINT32 QueryState(IUnknown* owner) {
    mew::message reply = InvokeEvent<mew::EventQueryState>(this, owner);
    return reply["state"] | (UINT32)(mew::ENABLED);
  }
  void Invoke() {
    TRACE(L"info: Command.Invoke[name=$1, desc=$2]", m_Name, m_Description);
    InvokeEvent<mew::EventInvoke>(static_cast<ICommand*>(this));
  }

 public:  // ISignal
  HRESULT Connect(mew::EventCode code, mew::function fn, mew::message msg = mew::null) noexcept {
    switch (code) {
      case mew::EventInvoke:
      case mew::EventQueryState:
        return __super::Connect(code, fn, msg);
      default:
        ASSERT(!"warning: Unsupported event code.");
        return E_INVALIDARG;
    }
  }

  struct Compare {
    bool operator()(const mew::string& lhs, const mew::string& rhs) const { return lhs.compare_nocase(rhs) < 0; }
    bool operator()(const Command* lhs, const Command* rhs) const { return operator()(lhs->m_Name, rhs->m_Name); }
    bool operator()(const mew::string& lhs, const Command* rhs) const { return operator()(lhs, rhs->m_Name); }
    bool operator()(const Command* lhs, const mew::string& rhs) const { return operator()(lhs->m_Name, rhs); }
  };
};

//==============================================================================

class CompoundCommand : public mew::Root<mew::implements<mew::ICommand> > {
 private:  // variables
  mew::string m_Name;
  mew::array<mew::ICommand> m_Commands;

 public:  // Object
  CompoundCommand(mew::string name) : m_Name(name) {}

 public:  // ICommand
  mew::string get_Description() { return m_Name; }
  UINT32 QueryState(IUnknown* owner) {
    UINT32 check = 0;
    for (mew::array<mew::ICommand>::iterator i = m_Commands.begin(); i != m_Commands.end(); ++i) {
      UINT32 state = (*i)->QueryState(owner);
      if (!(state & mew::ENABLED)) {
        return check;
      }
      if (state & mew::CHECKED) {
        check = mew::CHECKED;
      }
    }
    return mew::ENABLED | check;
  }
  void Invoke() {
    TRACE(_T("info: CompoundCommand.Invoke[name=$1]"), m_Name);
    m_Commands.foreach (this, &CompoundCommand::InvoleCommand);
  }
  void AddCommand(ICommand* command) {
    ASSERT(command);
    m_Commands.push_back(command);
  }

 private:
  bool InvoleCommand(ICommand* command) {
    command->Invoke();
    afx::PumpMessage();
    Sleep(1);  // とりあえずスリープを入れて、2週回してみる。
    afx::PumpMessage();
    return true;
  }
};
}  // namespace

//==============================================================================

namespace mew {

class Commands : public Root<implements<ICommands> > {
 private:
  using Map = std::map<string, ref<ICommand>, Command::Compare>;
  Map m_map;

 public:  // Object
  void __init__(IUnknown* arg) { ASSERT(!arg); }
  void Dispose() noexcept { m_map.clear(); }

 public:  // ICommands
  HRESULT Add(string name, ICommand* command) {
    if (m_map.find(name) != m_map.end()) {
      TRACE(name);
      ASSERT(!"同じ名前のコマンドを登録しようとしました");
      return E_FAIL;
    }
    m_map[name] = command;
    return S_OK;
  }
  HRESULT Add(string name, string description) {
    if (m_map.find(name) != m_map.end()) {
      TRACE(name);
      ASSERT(!"同じ名前のコマンドを登録しようとしました");
      return E_FAIL;
    }
    m_map[name] = objnew<Command>(name, description);
    return S_OK;
  }
  HRESULT Alias(string alias, string name) {
    if (!alias) {
      return E_POINTER;
    }
    Map::iterator i = m_map.find(name);
    if (i == m_map.end()) {
      return E_INVALIDARG;
    }
    m_map[alias] = i->second;
    return S_OK;
  }
  HRESULT Find(REFINTF ppInterface, string name) {
    Map::iterator i = m_map.find(name);
    if (i == m_map.end()) {
      ref<CompoundCommand> cmd;

      const PCTSTR SEPARATOR = _T(";");
      const PCTSTR TRIM =
          _T(";")
          _T(" \t\r\n");
      StringSplit token(name.str(), SEPARATOR, TRIM);

      while (string nm = token.next()) {
        Map::iterator j = m_map.find(nm);
        if (j == m_map.end()) {  // ひとつでも見つからないコマンドがあれば失敗
          TRACE(name);
          ASSERT(!"command not found");
          *ppInterface.pp = null;
          return E_FAIL;
        }
        if (!cmd) cmd.attach(new CompoundCommand(name));
        cmd->AddCommand(j->second);
      }
      if (!cmd) {
        return E_INVALIDARG;
      }
      // TODO: 複合コマンドを登録する。しかし、ここで勝手に登録すると、解放されなくなってしまう。
      // 複合コマンド専用にAddRef()しない配列を用意して、弱参照として扱うべきか？
      // Add(name);
      return cmd->QueryInterface(ppInterface);
    } else {
      return i->second->QueryInterface(ppInterface);
    }
  }
  HRESULT Remove(string name) { return m_map.erase(name) > 0 ? S_OK : E_FAIL; }
  HRESULT SetHandler(string name, function fn, message msg) {
    HRESULT hr;
    ref<ISignal> src;
    if FAILED (hr = Find(&src, name)) {
      return hr;
    }
    return src->Connect(EventInvoke, fn, msg);
  }
  HRESULT SetObserver(string name, function fn, message msg) {
    HRESULT hr;
    ref<ISignal> src;
    if FAILED (hr = Find(&src, name)) {
      return hr;
    }
    return src->Connect(EventQueryState, fn, msg);
  }
};

AVESTA_EXPORT(Commands)

}  // namespace mew

//==============================================================================
