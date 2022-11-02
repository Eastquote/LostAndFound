#pragma once

#include <memory>
#include <list>

struct Listener {};
using tListenerPtr = std::shared_ptr<Listener>;

template <typename tFunc>
class Delegate
{
public:
	struct _Listener : public Listener
	{
		_Listener(const tFunc& in_func)
			: func(in_func)
		{
		}
		tFunc func;
	};

	tListenerPtr Add(tFunc in_func)
	{
		auto ret = std::make_shared<_Listener>(in_func);
		m_listeners.push_back(ret);
		return std::static_pointer_cast<Listener>(ret);
	}
	template <typename ...tEventArgs>
	void Broadcast(tEventArgs... in_args)
	{
		for(auto i = m_listeners.begin(); i != m_listeners.end(); )
		{
			auto listener = i->lock();
			if(listener.get())
			{
				listener->func(std::forward<tEventArgs>(in_args)...);
				++i;
			}
			else
				m_listeners.erase(i++);
		}
	}

private:
	using tWeakListenerPtr = std::weak_ptr<_Listener>;
	using tListeners = std::list<tWeakListenerPtr>;
	tListeners m_listeners;
};
