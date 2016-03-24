// CommandProvider.hpp
#pragma once

#include "main.hpp"

template < class TBase >
class __declspec(novtable) CommandProvider : public TBase
{
protected:
	ref<ICommands>	m_commands;

public:
	HRESULT InvokeCommand(string name)
	{
		HRESULT hr;
		ref<ICommand> command;
		if FAILED(hr = m_commands->Find(&command, name))
			return hr;
		command->Invoke();
		return S_OK;
	}

public:
	template < class M >
	void HandleEvent(ISignal* source, int code, HRESULT (M::*fn)(message), message msg = null)
	{
		source->Connect(code, function(this, fn), msg);
	}
	template < class T, class M >
	void HandleEvent(ISignal* source, int code, T* sink, HRESULT (M::*fn)(message), message msg = null)
	{
		source->Connect(code, function(sink, fn), msg);
	}
	void HandleEvent(ISignal* source, int code, IWindow* window, message msg = null)
	{
		source->Connect(code, function(window, &IWindow::Send), msg);
	}

public: // SetHandler thunk
	template < class M >
	void CommandProcess(const string& command, HRESULT (M::*fn)(message), message msg = null)
	{
		CommandProcess(command, function(this, fn), msg);
	}
	template < class T, class M >
	void CommandProcess(const string& command, const T& p, HRESULT (M::*fn)(message), message msg = null)
	{
		if(p)
			CommandProcess(command, function(p, fn), msg);
		else
			DisableCommand(command);
	}
	void CommandProcess(const string& command, IWindow* window, message msg = null)
	{
		if(window)
			CommandProcess(command, function(window, &IWindow::Send), msg);
		else
			DisableCommand(command);
	}
	void CommandProcess(const string& command, function fn, message msg = null)
	{
		m_commands->SetHandler(command, fn, msg);
	}

public: // SetObserver thunk
	template < class M >
	void CommandObserve(const string& command, HRESULT (M::*fn)(message), message msg = null)
	{
		CommandObserve(command, function(this, fn), msg);
	}
	template < class T, class M >
	void CommandObserve(const string& command, const T& p, HRESULT (M::*fn)(message), message msg = null)
	{
		CommandObserve(command, function(p, fn), msg);
	}
	void CommandObserve(const string& command, IWindow* window, message msg = null)
	{
		CommandObserve(command, function(window, &IWindow::Send), msg);
	}
	void CommandObserve(const string& command, function fn, message msg = null)
	{
		m_commands->SetObserver(command, fn, msg);
	}

public: // SetHandler and Observer
	template < class H1, class H2 >
	void CommandHandler(const string& command, const H1& h1, message process, const H2& h2, message observe)
	{
		if(h1)
		{
			CommandProcess(command, h1, process);
			CommandObserve(command, h2, observe);
		}
		else
		{
			DisableCommand(command);
		}
	}
	void CommandToFocus(const string& command, message msg)
	{
		CommandProcess(command, function(this, &Main::ForwardToCurrent), msg);
		CommandObserve(command, function(this, &Main::EnableIfAnyFolder));
	}
	void CommandToSelect(const string& command, message msg)
	{
		CommandProcess(command, function(this, &Main::ForwardToCurrent), msg);
		CommandObserve(command, function(this, &Main::EnableIfAnySelection));
	}
	void CommandToCheck(const string& command, message msg)
	{
		CommandProcess(command, function(this, &Main::ForwardToCurrent), msg);
		CommandObserve(command, function(this, &Main::EnableIfCheckBoxEnabled));
	}
	void DisableCommand(const string& command)
	{
		CommandObserve(command, &Main::DisableCommandHandler);
	}
	HRESULT DisableCommandHandler(message msg)
	{
		msg["state"] = 0;
		return S_OK;
	}

public: // etc.
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
		command->Connect(EventInvoke, function(this, &Main::DeprecatedHandler), msg);
		return S_OK;
	}
	*/
	void CommandWindowControl(const string& name, IWindow* window, ICommands* commands)
	{
		string cmdShow  = string::format(L"$1.Show(toggle)", name);
		string cmdFocus = string::format(L"$1.Focus", name);
		commands->Add(cmdShow , string::load(IDS_DESC_SHOW , name));
		commands->Add(cmdFocus, string::load(IDS_DESC_FOCUS, name));
		if(window)
		{
			// Show
			CommandProcess(cmdShow, window, CommandShow);
			message m;
			m["window"] = window;
			CommandObserve(cmdShow, &Main::ObserveShow, m);
			// Focus
			CommandProcess(cmdFocus, window, &IWindow::Focus);
		}
		else
		{
			// Show
			DisableCommand(cmdShow);
			// Focus
			DisableCommand(cmdFocus);
		}
	}

	void RegisterForms(Templates& templates, ICommands* commands)
	{
		for(Templates::const_iterator i = m_forms.begin(); i != m_forms.end(); ++i)
		{
			if(!i->window || !i->id)
				continue;
			CommandWindowControl(i->id, i->window, commands);
		}
	}
};
