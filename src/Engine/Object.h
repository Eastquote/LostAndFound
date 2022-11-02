#pragma once

#include <memory>
#include <cassert>
#include <vector>
#include <functional>

#include "TasksConfig.h"

enum class eObjectState
{
	Uninitialized,
	Initialized,
	Destroyed,
	Deleted,
};
class Object : public std::enable_shared_from_this<Object>
{
public:
	virtual ~Object() { m_state = eObjectState::Deleted; }
	virtual void Initialize();
	virtual void Destroy();
	void DestroyChildren();
	bool IsInitialized() { return m_state >= eObjectState::Initialized; }
	bool IsDestroyed() { return m_state >= eObjectState::Destroyed; }
	eObjectState GetState() { return m_state; }
	void SetOwner(std::shared_ptr<Object> in_owner);
	std::shared_ptr<Object> GetOwner() const { return m_owner; }

	static std::shared_ptr<Object> MakeRoot()
	{
		auto obj = std::make_shared<Object>();
		obj->SetOwner(obj);
		obj->Initialize();
		return obj;
	}

	template <typename tObj, typename... Args>
	static std::shared_ptr<tObj> Make(const std::shared_ptr<Object>& in_owner, Args... args)
	{
		auto obj = std::make_shared<tObj>(std::forward<Args>(args)...);
		obj->SetOwner(in_owner);
		obj->Initialize();
		return obj;
	}

	template <typename tObj, typename... Args>
	std::shared_ptr<tObj> MakeChild(Args... args)
	{
		return Make<tObj>(AsShared(), std::forward<Args>(args)...);
	}

	template <typename tObj, typename... Args>
	static std::shared_ptr<tObj> MakeWithInit(const std::shared_ptr<Object>& in_owner, std::function<void(std::shared_ptr<tObj>)> in_initFunc, Args... args)
	{
		auto obj = std::make_shared<tObj>(std::forward<Args>(args)...);
		obj->SetOwner(in_owner);
		if(in_initFunc)
			in_initFunc(obj);
		obj->Initialize();
		return obj;
	}

	template <typename tObj, typename... Args>
	std::shared_ptr<tObj> MakeChildWithInit(std::function<void(std::shared_ptr<tObj>)> in_initFunc, Args... args)
	{
		return Make<tObj>(AsShared(), std::move(in_initFunc), std::forward<Args>(args)...);
	}

	template <typename T = Object>
	std::shared_ptr<T> AsShared()
	{
		return shared_from_base<T>();
	}

protected:
	template <typename tDerived>
	std::shared_ptr<tDerived> shared_from_base()
	{
		return std::static_pointer_cast<tDerived>(shared_from_this());
	}

private:
	int32_t AddChild(std::shared_ptr<Object> in_child);
	void RemoveChild(int32_t in_childIdx);

	eObjectState m_state = eObjectState::Uninitialized;
	std::shared_ptr<Object> m_owner;
	using tChildren = std::vector<std::shared_ptr<Object> >;
	tChildren m_children;
	int32_t m_childIdx = -1;
};

template <typename tObj>
class ObjectGuard
{
public:
	ObjectGuard() {}
	ObjectGuard(std::shared_ptr<tObj> in_obj)
		: m_obj(in_obj)
	{
	}
	~ObjectGuard()
	{
		Reset();
	}
	ObjectGuard(const ObjectGuard<tObj>& in_other) = delete;
	ObjectGuard<tObj>& operator=(const ObjectGuard<tObj>& in_other) = delete;
	ObjectGuard(ObjectGuard<tObj>&& in_other)
		: m_obj(std::move(in_other.m_obj))
	{
		SQUID_RUNTIME_CHECK(this != &in_other, "Cannot move object to itself");
		in_other.m_obj.reset();
	}
	ObjectGuard<tObj>& operator=(ObjectGuard<tObj>&& in_other)
	{
		SQUID_RUNTIME_CHECK(this != &in_other, "Cannot move object to itself");
		m_obj = std::move(in_other.m_obj);
		in_other.m_obj.reset();
		return *this;
	}
	void Set(std::shared_ptr<tObj> in_obj)
	{
		SQUID_RUNTIME_CHECK(m_obj->IsInitialized(), "Object guard cannot be assigned an uninitialized object");
		m_obj = in_obj;
	}
	std::shared_ptr<tObj> Get() const
	{
		return m_obj;
	}
	tObj* operator->() const
	{
		return m_obj.get();
	}
	void Forget()
	{
		m_obj = nullptr;
	}
	void Reset()
	{
		if(m_obj.get())
		{
			if(!m_obj->IsDestroyed())
			{
				m_obj->Destroy();
			}
			m_obj = nullptr;
		}
	}

private:
	std::shared_ptr<tObj> m_obj;
};

class ObjectMultiGuard
{
public:
	ObjectMultiGuard() {}
	~ObjectMultiGuard()
	{
		Reset();
	}
	ObjectMultiGuard(const ObjectMultiGuard& in_other) = delete;
	ObjectMultiGuard& operator=(const ObjectMultiGuard& in_other) = delete;
	ObjectMultiGuard(ObjectMultiGuard&& in_other)
		: m_objects(std::move(in_other.m_objects))
	{
		SQUID_RUNTIME_CHECK(this != &in_other, "Cannot move object to itself");
		in_other.m_objects.clear();
	}
	ObjectMultiGuard& operator=(ObjectMultiGuard&& in_other)
	{
		SQUID_RUNTIME_CHECK(this != &in_other, "Cannot move object to itself");
		m_objects = std::move(in_other.m_objects);
		in_other.m_objects.clear();
	}
	template <typename T>
	std::shared_ptr<T> Guard(std::shared_ptr<T> in_obj)
	{
		m_objects.push_back(in_obj);
		return in_obj;
	}
	void Forget()
	{
		m_objects.clear();
	}
	void Reset()
	{
		for(const auto& obj : m_objects)
		{
			if(!obj->IsDestroyed())
			{
				obj->Destroy();
			}
		}
		m_objects.clear();
	}

private:
	std::vector<std::shared_ptr<Object>> m_objects;
};

template <typename tObj>
static auto MakeObjectGuard(std::shared_ptr<tObj> in_obj)
{
	return std::move(ObjectGuard<tObj>(in_obj));
}

static bool IsAlive(Object* in_obj)
{
	return in_obj && !in_obj->IsDestroyed();
}

template <typename tObj>
static bool IsAlive(std::shared_ptr<tObj> in_obj)
{
	return in_obj && !in_obj->IsDestroyed();
}

template <typename tObj>
static bool IsAlive(std::weak_ptr<tObj> in_obj)
{
	if(auto obj = in_obj.lock())
	{
		return !obj->IsDestroyed();
	}
	return false;
}
