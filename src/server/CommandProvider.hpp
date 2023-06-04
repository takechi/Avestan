// CommandProvider.hpp
#pragma once

#include "main.hpp"

template <class TBase>
class __declspec(novtable) CommandProvider : public TBase {
 protected:
  mew::ref<mew::ICommands> m_commands;

 public:
  HRESULT InvokeCommand(mew::string name) {
    HRESULT hr;
    mew::ref<mew::ICommand> command;
    if FAILED (hr = m_commands->Find(&command, name)) {
      return hr;
    }
    command->Invoke();
    return S_OK;
  }

 public:
  template <class M>
  void HandleEvent(mew::ISignal* source, int code, HRESULT (M::*fn)(mew::message), mew::message msg = mew::null) {
    source->Connect(code, mew::function(this, fn), msg);
  }
  template <class T, class M>
  void HandleEvent(mew::ISignal* source, int code, T* sink, HRESULT (M::*fn)(mew::message), mew::message msg = mew::null) {
    source->Connect(code, mew::function(sink, fn), msg);
  }
  void HandleEvent(mew::ISignal* source, int code, mew::ui::IWindow* window, mew::message msg = mew::null) {
    source->Connect(code, mew::function(window, &mew::ui::IWindow::Send), msg);
  }

 public:  // SetHandler thunk
  template <class M>
  void CommandProcess(const mew::string& command, HRESULT (M::*fn)(mew::message), mew::message msg = mew::null) {
    CommandProcess(command, mew::function(this, fn), msg);
  }
  template <class T, class M>
  void CommandProcess(const mew::string& command, const T& p, HRESULT (M::*fn)(mew::message), mew::message msg = mew::null) {
    if (p){
      CommandProcess(command, mew::function(p, fn), msg);
    }else{
      DisableCommand(command);
      }
  }
  void CommandProcess(const mew::string& command, mew::ui::IWindow* window, mew::message msg = mew::null) {
    if (window) {
      CommandProcess(command, mew::function(window, &mew::ui::IWindow::Send), msg);
    } else {
      DisableCommand(command);
    }
  }
  void CommandProcess(const mew::string& command, mew::function fn, mew::message msg = mew::null) {
    m_commands->SetHandler(command, fn, msg);
  }

 public:  // SetObserver thunk
  template <class M>
  void CommandObserve(const mew::string& command, HRESULT (M::*fn)(mew::message), mew::message msg = mew::null) {
    CommandObserve(command, mew::function(this, fn), msg);
  }
  template <class T, class M>
  void CommandObserve(const mew::string& command, const T& p, HRESULT (M::*fn)(mew::message), mew::message msg = mew::null) {
    CommandObserve(command, mew::function(p, fn), msg);
  }
  void CommandObserve(const mew::string& command, mew::ui::IWindow* window, mew::message msg = mew::null) {
    CommandObserve(command, mew::function(window, &mew::ui::IWindow::Send), msg);
  }
  void CommandObserve(const mew::string& command, mew::function fn, mew::message msg = mew::null) {
    m_commands->SetObserver(command, fn, msg);
  }

 public:  // SetHandler and Observer
  template <class H1, class H2>
  void CommandHandler(const mew::string& command, const H1& h1, mew::message process, const H2& h2, mew::message observe) {
    if (h1) {
      CommandProcess(command, h1, process);
      CommandObserve(command, h2, observe);
    } else {
      DisableCommand(command);
    }
  }
  void CommandToFocus(const mew::string& command, mew::message msg) {
    CommandProcess(command, mew::function(this, &Main::ForwardToCurrent), msg);
    CommandObserve(command, mew::function(this, &Main::EnableIfAnyFolder));
  }
  void CommandToSelect(const mew::string& command, mew::message msg) {
    CommandProcess(command, mew::function(this, &Main::ForwardToCurrent), msg);
    CommandObserve(command, mew::function(this, &Main::EnableIfAnySelection));
  }
  void CommandToCheck(const mew::string& command, mew::message msg) {
    CommandProcess(command, mew::function(this, &Main::ForwardToCurrent), msg);
    CommandObserve(command, mew::function(this, &Main::EnableIfCheckBoxEnabled));
  }
  void DisableCommand(const mew::string& command) { CommandObserve(command, &Main::DisableCommandHandler); }
  HRESULT DisableCommandHandler(mew::message msg) {
    msg["state"] = 0;
    return S_OK;
  }

 public:  // etc.
  /*
  HRESULT DeprecatedHandler(message msg)
  {
          string name = msg["name"];
          if(!name)
          {
                  ASSERT(0);
                  return E_INVALIDARG;
          }
          Notify(NotifyWarning, string::load(IDS_WARN_DEPRECATED, name));
          return S_OK;
  }
  HRESULT Deprecated(const string& name)
  {
          HRESULT hr;
          ref<ISignal> command;
          if FAILED(hr = m_commands->Find(&command, name))
                  return hr;
          message msg;
          msg["name"] = name;
          command->Connect(EventInvoke, function(this,
  &Main::DeprecatedHandler), msg); return S_OK;
  }
  */
  void CommandWindowControl(const mew::string& name, mew::ui::IWindow* window, mew::ICommands* commands) {
    mew::string cmdShow = mew::string::format(L"$1.Show(toggle)", name);
    mew::string cmdFocus = mew::string::format(L"$1.Focus", name);
    commands->Add(cmdShow, mew::string::load(IDS_DESC_SHOW, name));
    commands->Add(cmdFocus, mew::string::load(IDS_DESC_FOCUS, name));
    if (window) {
      // Show
      CommandProcess(cmdShow, window, mew::ui::CommandShow);
      mew::message m;
      m["window"] = window;
      CommandObserve(cmdShow, &Main::ObserveShow, m);
      // Focus
      CommandProcess(cmdFocus, window, &mew::ui::IWindow::Focus);
    } else {
      // Show
      DisableCommand(cmdShow);
      // Focus
      DisableCommand(cmdFocus);
    }
  }

  void RegisterForms(Templates& templates, mew::ICommands* commands) {
    for (Templates::const_iterator i = m_forms.begin(); i != m_forms.end(); ++i) {
      if (!i->window || !i->id) {
        continue;
      }
      CommandWindowControl(i->id, i->window, commands);
    }
  }
};
