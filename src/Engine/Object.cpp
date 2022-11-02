#include "Object.h"

void Object::Initialize()
{
	m_state = eObjectState::Initialized;
	SQUID_RUNTIME_CHECK(m_owner.get(), "Objects must have an owner set before Initialize() is called");
}
void Object::Destroy()
{
	SQUID_RUNTIME_CHECK(!IsDestroyed(), "Object::Destroy() called twice on the same object");
	std::shared_ptr<Object> self = AsShared();
	SetOwner(nullptr);
	m_state = eObjectState::Destroyed;
	while(m_children.size())
	{
		std::shared_ptr<Object> obj = m_children.back();
		if(!obj->IsDestroyed())
			obj->Destroy();
		SQUID_RUNTIME_CHECK(obj->IsDestroyed(), "Child object not destroyed (perhaps it's class is missing an Object::Destroy() call?)");
	}
}
void Object::DestroyChildren()
{
	eObjectState tmpState = m_state;
	m_state = eObjectState::Destroyed; // Temporarily mark us a destroyed so we don't destroy ourselves in the case of self-ownership
	while(m_children.size())
	{
		std::shared_ptr<Object> obj = m_children.back();
		if(!obj->IsDestroyed())
			obj->Destroy();
		SQUID_RUNTIME_CHECK(obj->IsDestroyed(), "Child object not destroyed (perhaps its class is missing an Object::Destroy() call?)");
	}
	m_state = tmpState;
}
void Object::SetOwner(std::shared_ptr<Object> in_owner)
{
	if(in_owner != m_owner && (!in_owner.get() || !in_owner->IsDestroyed()))
	{
		if(m_owner.get())
			m_owner->RemoveChild(m_childIdx);
		m_owner = in_owner;
		if(m_owner.get())
			m_childIdx = m_owner->AddChild(AsShared());
	}
}
int32_t Object::AddChild(std::shared_ptr<Object> in_child)
{
	in_child->m_childIdx = (int32_t)m_children.size();
	m_children.push_back(in_child);
	return in_child->m_childIdx;
}
void Object::RemoveChild(int32_t in_childIdx)
{
	m_children[in_childIdx] = m_children.back();
	m_children[in_childIdx]->m_childIdx = in_childIdx;
	m_children.pop_back();
}
