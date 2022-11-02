#include "SceneComponent.h"

#include "Engine/Actor.h"

void SceneComponent::Initialize()
{
	Object::Initialize();
	SQUID_RUNTIME_CHECK(IsAlive(m_actor), "SceneComponents require their actor to be set prior to initialization");
}
void SceneComponent::Destroy()
{
	SetAttachParent(nullptr, false); // Detach from parent (clears transform cache)
	for(auto attachChild : m_attachChildren)
	{
		attachChild->m_attachParent.reset(); // Raw parent unset (we know transform cache was already cleared in SetAttachParent())
	}
	m_attachChildren.clear(); // Clear attach-children array
	m_actor.reset(); // Clear actor reference
	Object::Destroy();
}
void SceneComponent::SetActor(std::shared_ptr<Actor> in_actor)
{
	m_actor = in_actor;
}
std::shared_ptr<Actor> SceneComponent::GetActor() const
{
	return m_actor.lock();
}
float SceneComponent::DT() const
{
	return GetActor()->DT();
}
float SceneComponent::Time() const
{
	return GetActor()->Time();
}
void SceneComponent::SetAttachParent(std::shared_ptr<SceneComponent> in_attachParent, bool in_keepWorldTransform)
{
	if(GetAttachParent() != in_attachParent)
	{
		Transform oldTransform = in_keepWorldTransform ? GetWorldTransform() : Transform::Identity;
		m_attachParent = in_attachParent;
		if(auto attachParent = GetAttachParent())
		{
			attachParent->m_attachChildren.push_back(AsShared<SceneComponent>());
		}
		ClearTransformCache();
		if(in_keepWorldTransform)
		{
			SetWorldTransform(oldTransform);
		}
	}
}
std::shared_ptr<SceneComponent> SceneComponent::GetAttachParent() const
{
	return m_attachParent.lock();
}
const SceneComponent::tAttachChildren& SceneComponent::GetAttachChildren() const
{
	return m_attachChildren;
}
void SceneComponent::ClearTransformCache()
{
	// Recursively clear the cache on all attach children (clearing any dead or with incorrect attach parent)
	for(size_t idx = 0; idx < m_attachChildren.size(); ++idx)
	{
		const auto& attachChild = m_attachChildren[idx];
		bool shouldRemoveChild = !IsAlive(attachChild);
		if(!shouldRemoveChild)
		{
			auto attachParent = attachChild->GetAttachParent();
			shouldRemoveChild = !IsAlive(attachParent) || attachParent.get() != this;
		}
		if(shouldRemoveChild)
		{
			m_attachChildren[idx--] = m_attachChildren.back();
			m_attachChildren.pop_back();
		}
		else
		{
			attachChild->ClearTransformCache();
		}
	}

	// Clear our own world transform
	m_worldTransform.reset();
	OnTransformChanged();
}
void SceneComponent::SetRelativeTransform(const Transform& in_relativeTransform)
{
	m_relativeTransform = in_relativeTransform;
	ClearTransformCache();
}
const Transform& SceneComponent::GetRelativeTransform() const
{
	return m_relativeTransform;
}
void SceneComponent::SetRelativePos(const Vec2f& in_relativePos)
{
	m_relativeTransform.pos = in_relativePos;
	ClearTransformCache();
}
const Vec2f& SceneComponent::GetRelativePos() const
{
	return m_relativeTransform.pos;
}
void SceneComponent::SetRelativeRot(float in_relativeRot)
{
	m_relativeTransform.rot = in_relativeRot;
	ClearTransformCache();
}
const float SceneComponent::GetRelativeRot() const
{
	return m_relativeTransform.rot;
}
void SceneComponent::SetRelativeScale(const Vec2f& in_relativeScale)
{
	m_relativeTransform.scale = in_relativeScale;
	ClearTransformCache();
}
const Vec2f& SceneComponent::GetRelativeScale() const
{
	return m_relativeTransform.scale;
}
void SceneComponent::SetWorldTransform(const Transform& in_worldTransform)
{
	// Calculate and set the world transform
	auto attachParent = GetAttachParent();
	if(IsAlive(attachParent))
	{
		const auto& parentWorldTransform = attachParent->GetWorldTransform();
		SetRelativeTransform(in_worldTransform * parentWorldTransform.Inverse());
		m_worldTransform = in_worldTransform;
	}
	else
	{
		SetRelativeTransform(in_worldTransform);
	}
}
const Transform& SceneComponent::GetWorldTransform() const
{
	// Calculate and cache final transform
	auto attachParent = GetAttachParent();
	if(IsAlive(attachParent))
	{
		if(!m_worldTransform)
		{
			m_worldTransform = GetRelativeTransform() * attachParent->GetWorldTransform();
		}
		return m_worldTransform.value();
	}
	return m_relativeTransform;
}
void SceneComponent::SetWorldPos(const Vec2f& in_worldPos)
{
	const auto& worldTransform = GetWorldTransform();
	SetWorldTransform({ in_worldPos, worldTransform.rot, worldTransform.scale });
}
const Vec2f& SceneComponent::GetWorldPos() const
{
	return GetWorldTransform().pos;
}
void SceneComponent::SetWorldRot(float in_worldRot)
{
	const auto& worldTransform = GetWorldTransform();
	SetWorldTransform({ worldTransform.pos, in_worldRot, worldTransform.scale });
}
const float SceneComponent::GetWorldRot() const
{
	return GetWorldTransform().rot;
}
void SceneComponent::SetWorldScale(const Vec2f& in_worldScale)
{
	const auto& worldTransform = GetWorldTransform();
	SetWorldTransform({ worldTransform.pos, worldTransform.rot, in_worldScale });
}
const Vec2f& SceneComponent::GetWorldScale() const
{
	return GetWorldTransform().scale;
}
