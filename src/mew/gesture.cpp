// gesture.cpp

#include "stdafx.h"
#include "widgets.hpp"
#include "object.hpp"
#include "std/array_map.hpp"

using namespace mew;
using namespace mew::ui;

namespace mew { namespace ui {

class GestureTable : public Root< implements<IGestureTable, IGesture> >
{
private:
	struct GestureKeyRef
	{
		UINT16	modifiers;
		size_t	size;
		const Gesture* 	gestures;

		GestureKeyRef(UINT16 mods, size_t len, const Gesture* ges) : modifiers(mods), size(len), gestures(ges)
		{
		}
		size_t length() const	{ return size; }
	};

	struct GestureKey
	{
		UINT16	modifiers;
		std::vector<Gesture>	gestures;

		GestureKey(UINT16 mods, size_t len, const Gesture* ges) : modifiers(mods), gestures(ges, ges+len)
		{
		}
		size_t length() const	{ return gestures.size(); }
		bool accept(size_t length, const Gesture ges[]) const
		{
			if(length > gestures.size())
				return false;
			for(size_t i = 0; i < length; ++i)
			{
				if(gestures[i] != ges[i])
					return false;
			}
			return (length == gestures.size() || (length < gestures.size() && gestures[length] > GestureButtonX2));
		}
	};

	struct GestureKeyLess
	{
		typedef GestureKey	first_argument_type;

		template < typename LHS, typename RHS >
		bool operator () (const LHS& lhs, const RHS& rhs) const
		{
			if(lhs.length() != rhs.length())
				return lhs.length() < rhs.length();
			if(lhs.modifiers != rhs.modifiers)
				return lhs.modifiers < rhs.modifiers;
			for(size_t i = 0; i < lhs.length(); ++i)
			{
				if(lhs.gestures[i] != rhs.gestures[i])
					return lhs.gestures[i] < rhs.gestures[i];
			}
			return false;
		}
	};

private:
	typedef mew::array_map< GestureKey, ref<ICommand>, GestureKeyLess > GestureMap;
	GestureMap	m_GestureMap;

public:
	void __init__(IUnknown* arg)
	{
	}
	void Dispose()
	{
		m_GestureMap.clear();
	}

public: // IGesture
	HRESULT OnGestureAccept(HWND hWnd, Point ptScreen, size_t length, const Gesture gesture[])
	{
		for(GestureMap::const_iterator i = m_GestureMap.begin(); i != m_GestureMap.end(); ++i)
		{
			if(i->first.accept(length, gesture))
				return S_OK;
		}
		return E_FAIL;
	}
	HRESULT OnGestureUpdate(UINT16 modifiers, size_t length, const Gesture gesture[])
	{
		return S_OK;
	}
	HRESULT OnGestureFinish(UINT16 modifiers, size_t length, const Gesture gesture[])
	{
		ref<ICommand> command;
		if SUCCEEDED(GetGesture(modifiers, length, gesture, &command))
		{
			command->Invoke();
			return S_OK;
		}
		else
			return E_FAIL;
	}

public: // IGestureTable
	size_t get_Count()
	{
		return m_GestureMap.size();
	}
	HRESULT GetGesture(size_t index, REFINTF ppCommand)
	{
		if(index >= m_GestureMap.size())
			return E_INVALIDARG;
		return m_GestureMap.at(index)->second.copyto(ppCommand);
	}
	HRESULT GetGesture(UINT16 modifiers, size_t length,	const Gesture gesture[], REFINTF ppCommand)
	{
		GestureMap::const_iterator i = m_GestureMap.find(GestureKeyRef(modifiers, length, gesture));
		if(i == m_GestureMap.end())
			return E_FAIL;
		else
			return i->second.copyto(ppCommand);
	}
	HRESULT SetGesture(UINT16 modifiers, size_t length,	const Gesture gesture[], ICommand* pCommand)
	{
		GestureKey key(modifiers, length, gesture);
		if(!pCommand)
			m_GestureMap.erase(key);
		else
			m_GestureMap[key] = pCommand;
		return S_OK;
	}
};

} }

AVESTA_EXPORT( GestureTable )
