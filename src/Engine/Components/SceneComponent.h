#pragma once

#include "TasksConfig.h"
#include "Engine/Object.h"
#include "Engine/Time.h"
#include "Engine/Transform.h"

class Actor;

// Scene Component
class SceneComponent : public Object
{
public:
	using tAttachChildren = std::vector<std::shared_ptr<SceneComponent>>;

	// Object Interface
	void Initialize();
	void Destroy();

	// Actor
	void SetActor(std::shared_ptr<Actor> in_actor);
	std::shared_ptr<Actor> GetActor() const;

	// Time Stream (provided by Actor)
	float DT() const;
	float Time() const;

	// Attachment
	void SetAttachParent(std::shared_ptr<SceneComponent> in_attachParent, bool in_keepWorldTransform = true);
	std::shared_ptr<SceneComponent> GetAttachParent() const;
	const tAttachChildren& GetAttachChildren() const;

	// Relative Transform
	void SetRelativeTransform(const Transform& in_relativeTransform);
	const Transform& GetRelativeTransform() const;
	void SetRelativePos(const Vec2f& in_relativePos);
	const Vec2f& GetRelativePos() const;
	void SetRelativeRot(float in_relativeRot);
	const float GetRelativeRot() const;
	void SetRelativeScale(const Vec2f& in_relativeScale);
	const Vec2f& GetRelativeScale() const;

	// World Transform
	void SetWorldTransform(const Transform& in_worldTransform);
	const Transform& GetWorldTransform() const;
	void SetWorldPos(const Vec2f& in_worldPos);
	const Vec2f& GetWorldPos() const;
	void SetWorldRot(float in_worldRot);
	const float GetWorldRot() const;
	void SetWorldScale(const Vec2f& in_worldScale);
	const Vec2f& GetWorldScale() const;

protected:
	// Transform-Changed Event
	virtual void OnTransformChanged() {}

	void ClearTransformCache();
	std::weak_ptr<Actor> m_actor;
	std::weak_ptr<SceneComponent> m_attachParent;
	tAttachChildren m_attachChildren;
	Transform m_relativeTransform;
	mutable std::optional<Transform> m_worldTransform;
};
