// CommandList.hpp
#pragma once

class CommandList : public Root< implements<ICommand>, mixin<StaticLife> >
{
public:
	class CommandEntry : public Root< implements<ITreeItem, ICommand> >
	{
	protected:
		ref<IEntry>	m_entry;
		string		m_Text;
		size_t		m_index;

	public:
		CommandEntry(IEntry* entry) : m_entry(entry), m_index(0) { ASSERT(m_entry); }
		void Dispose()
		{
			m_entry.clear();
		}
		string get_Name()
		{
			if(!m_Text)
			{
				string path_or_name = m_entry->GetName(IEntry::PATH_OR_NAME);
				if(m_index < 9)
					m_Text = string::format(_T("&$1: $2"), m_index+1, path_or_name);
				else if(m_index == 9)
					m_Text = string::format(_T("&0: $1"), path_or_name);
				else
					m_Text = path_or_name;
			}
			return m_Text;
		}
		ref<ICommand> get_Command()				{ return this; }
		int		get_Image()						{ return -2; }
		void	set_Name(string value)			{ TRESPASS(); }
		void	set_Command(ICommand* value)	{ TRESPASS(); }
		void	set_Image(int value)			{ TRESPASS(); }
		bool	HasChildren()					{ return false; }
		size_t	GetChildCount()					{ return 0; }
		HRESULT GetChild(REFINTF ppChild, size_t index)	{ return E_INVALIDARG; }
		UINT32	OnUpdate()						{ return 1; }
		UINT32	QueryState(IUnknown* owner)		{ return ENABLED; }
		IEntry*	GetEntry() const				{ return m_entry; }
		void SetIndex(size_t index)
		{
			m_index = index;
			m_Text.clear();
		}
	};

protected:
	UINT32	m_TimeStamp;
	using Entries = std::deque< ref<CommandEntry> >;
	Entries	m_queue;

public:
	CommandList() : m_TimeStamp(1)	{}
	UINT32 GetTimeStamp() const	{ return m_TimeStamp; }
	void clear()
	{
		m_queue.clear();
	}
	size_t get_Count() const
	{
		return m_queue.size();
	}
	HRESULT GetAt(REFINTF ppChild, size_t index) const
	{
		if(index >= m_queue.size())
			return E_INVALIDARG;
		return m_queue[index]->QueryInterface(ppChild);
	}
	void Renumbering()
	{
		for(size_t i = 0; i < m_queue.size(); ++i)
		{
			m_queue[i]->SetIndex(i);
		}
	}

public: // command
	string get_Description()
	{
		if(m_queue.empty())
			return null;
		else
			return m_queue.front()->Description;
	}
	UINT32 QueryState(IUnknown* owner)
	{
		if(m_queue.empty())
			return 0;
		else
			return m_queue.front()->QueryState(owner);
	}
	void Invoke()
	{
		if(!m_queue.empty())
			m_queue.front()->Invoke();
	}
};

class CommandTreeItem : public Root< implements<ITreeItem, IEditableTreeItem> >
{
private:
	CommandList&		m_List;
	string				m_Text;
	int					m_Image;

public:
	CommandTreeItem(CommandList& mru) : m_List(mru)
	{
		m_Image = I_IMAGENONE;
	}
	void Dispose() throw()
	{
		m_Text.clear();
	}

	string    get_Name()				{ return m_Text; }
	ref<ICommand> get_Command()				{ return &m_List; }
	int	      get_Image()				{ return m_Image; }
	void set_Name(string value)			{ m_Text = value; }
	void set_Command(ICommand* value)	{ ASSERT(0); }
	void set_Image(int value)			{ m_Image = value; }

	bool HasChildren()		{ return true; }
	size_t GetChildCount()	{ return m_List.get_Count(); }
	HRESULT GetChild(REFINTF ppChild, size_t index)	{ return m_List.GetAt(ppChild, index); }
	UINT32 OnUpdate()	{ return m_List.GetTimeStamp(); }
	void AddChild(ITreeItem* child)		{ return; }
	bool RemoveChild(ITreeItem* child)	{ return false; }
};

//==============================================================================

class MRUList : public CommandList
{
private:
	class MRUEntry : public CommandEntry
	{
	private:
		MRUList&	m_owner;
	public:
		MRUEntry(MRUList& mru, IEntry* entry) : CommandEntry(entry), m_owner(mru)	{}
		void Invoke()
		{
			ref<IUnknown> addref(OID);
			m_owner.Remove(this); // この瞬間、参照数がゼロになる可能性がある
			theAvesta->OpenFolder(m_entry, NaviOpen);
		}
		string get_Description()
		{
			string path_or_name = m_entry->GetName(IEntry::PATH_OR_NAME);
			return string::load(IDS_MRU_OPEN, path_or_name);
		}
	};

public:
	static const size_t MAX_ENTRY_COUNT = 10;
	void Push(IEntry* entry)
	{
		if(!entry)
			return;
		++m_TimeStamp;
		for(Entries::iterator i = m_queue.begin(); i != m_queue.end(); ++i)
		{
			ref<CommandEntry> e = *i;
			if(e->GetEntry()->Equals(entry, IEntry::PATH))
			{
				m_queue.erase(i);
				m_queue.push_front(e);
				Renumbering();
				return;
			}
		}
		// new entry
		m_queue.push_front(ref<MRUEntry>::from(new MRUEntry(*this, entry)));
		if(m_queue.size() > MAX_ENTRY_COUNT)
			m_queue.resize(MAX_ENTRY_COUNT);
		Renumbering();
	}
	ref<IEntry> Pop()
	{
		if(m_queue.empty())
			return null;
		ref<CommandEntry> entry = m_queue.front();
		m_queue.pop_front();
		++m_TimeStamp;
		return entry->GetEntry();
	}
	void Remove(MRUEntry* entry)
	{
		for(Entries::iterator i = m_queue.begin(); i != m_queue.end(); ++i)
		{
			if(*i == entry)
			{
				m_queue.erase(i);
				Renumbering();
				++m_TimeStamp;
				return;
			}
		}
	}
	string get_Description()
	{
		return _T("最後に閉じられたフォルダを開きます");
	}

	friend inline IStream& operator << (IStream& stream, const MRUList& v) throw(...)
	{
		stream << v.m_queue.size();
		for(Entries::const_iterator i = v.m_queue.begin(); i != v.m_queue.end(); ++i)
		{
			stream << (*i)->GetEntry();
		}
		return stream;
	}

	friend inline IStream& operator >> (IStream& stream, MRUList& v) throw(...)
	{
		size_t size;
		stream >> size;
		for(size_t i = 0; i < size; ++i)
		{
			ref<IEntry> entry;
			stream >> entry;
			v.m_queue.push_back(ref<MRUEntry>::from(new MRUEntry(v, entry)));
		}
		return stream;
	}
};
