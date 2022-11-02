#pragma once

#include "DamageInfo.h"
#include "Cardinals.h"
#include "GameActor.h"
#include "Engine/MathCore.h"
#include "Engine/Box.h"

class Creature;
class Projectile;
struct ProjectileDef;

// A base class that implements functionality common to both Player and Creatures
class Character : public GameActor {
public:
	virtual void Initialize() override;
	virtual void Destroy() override;

	// Tries to move Character to in_pos
	// (tests for collision using Sweep and sets player pos to wherever the sweep collided with world tiles)
	virtual Vec2f Move(	const Vec2f& in_pos, bool in_teleport = false, float in_sweepVal = 0.01f, 
						std::shared_ptr<Creature> in_creature = {}, bool in_breakStuff = false, 
						std::optional<Box2f> in_box = {});
	const Box2f& GetCollisionBoxLocal() const { return m_worldCollisionBoxLocal; };
	Box2f GetCollisionBoxWorld() const { return m_worldCollisionBoxLocal.TransformedBy(GetWorldTransform()); };
	const Cardinals& GetCardinals() const { return m_blockCardinals; }

	// Updates a given Cardinals struct to reflect in which directions it's currently blocked, and at what distance
	void CardinalUpdate(Cardinals& in_blockCardinals, float sweepVal = 0.01f, Vec2f offset = Vec2f::Zero, 
						std::optional<Box2f> in_collisionBox = {}, std::shared_ptr<Creature> in_creature = {}) const;

protected:
	std::shared_ptr<Projectile> SpawnProjectile(const ProjectileDef& in_def, const Transform& in_transform, 
												const Vec2f& in_dir, int32_t in_facingDir = 0, const float in_mag = 1.0f);
	
	Box2f m_worldCollisionBoxLocal = Box2f::FromBottomCenter(Vec2f{ 0.0f, -15.0f }, { 8.0f, 29.0f });
	Box2f m_worldCollisionBoxLocalLedge = Box2f::FromBottomCenter(Vec2f{ 0.0f, -15.0f }, { 8.0f, 29.0f });
	Cardinals m_blockCardinals;
};
