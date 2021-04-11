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

class Command : public Root<implements<ICommand, ISignal>, mixin<SignalImpl, DynamicLife> > {
 private:  // variables
  string m_Name;
  string m_Description;

 public:  // Object
  Command(string name, string description) : m_Name(name), m_Description(description) { m_msgr.create(__uuidof(Messenger)); }
  void Dispose() throw() { m_msgr.dispose(); }

 public:  // ICommand
  string get_Description() { return m_Description; }
  UINT32 QueryState(IUnknown* owner) {
    message reply = InvokeEvent<EventQueryState>(this, owner);
    return reply["state"] | (UINT32)(ENABLED);
  }
  void Invoke() {
    TRACE(L"info: Command.Invoke[name=$1, desc=$2]", m_Name, m_Description);
    InvokeEvent<EventInvoke>(static_cast<ICommand*>(this));
  }

 public:  // ISignal
  HRESULT Connect(EventCode code, function fn, message msg = null) throw() {
    switch (code) {
      case EventInvoke:
      case EventQueryState:
        return __super::Connect(code, fn, msg);
      default:
        ASSERT(!"warning: Unsupported event code.");
        return E_INVALIDARG;
    }
  }

  struct Compare {
    bool operator()(const string& lhs, const string& rhs) const { return lhs.compare_nocase(rhs) < 0; }
    bool operator()(const Command* lhs, const Command* rhs) const { return operator()(lhs->m_Name, rhs->m_Name); }
    bool operator()(const string& lhs, const Command* rhs) const { return operator()(lhs, rhs->m_Name); }
    bool operator()(const Command* lhs, const string& rhs) const { return operator()(lhs->m_Name, rhs); }
  };
};

//==============================================================================

class CompoundCommand : public Root<implements<ICommand> > {
 private:  // variables
  string m_Name;
  array<ICommand> m_Commands;

 public:  // Object
  CompoundCommand(string name) : m_Name(name) {}

 public:  // ICommand
  string get_Description() { return m_Name; }
  UINT32 QueryState(IUnknown* owner) {
    UINT32 check = 0;
    for (array<ICommand>::iterator i = m_Commands.begin(); i != m_Commands.end(); ++i) {
      UINT32 state = (*i)->QueryState(owner);
      if (!(state & ENABLED)) return check;
      if (state & CHECKED) check = CHECKED;
    }
    return ENABLED | check;
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
  void Dispose() throw() { m_map.clear(); }

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
    if (!alias) return E_POINTER;
    Map::iterator i = m_map.find(name);
    if (i == m_map.end()) return E_INVALIDARG;
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
      if (!cmd) return E_INVALIDARG;
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
    if FAILED (hr = Find(&src, name)) return hr;
    return src->Connect(EventInvoke, fn, msg);
  }
  HRESULT SetObserver(string name, function fn, message msg) {
    HRESULT hr;
    ref<ISignal> src;
    if FAILED (hr = Find(&src, name)) return hr;
    return src->Connect(EventQueryState, fn, msg);
  }
};

}  // namespace mew

//==============================================================================

AVESTA_EXPORT(Commands)
