#pragma once

#include "Creature.h"

// Ship creature (becomes the boss)
class Ship : public Creature {
	DECLARE_SPAWNABLE_TYPE(Ship);
public:
	Ship(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name);
	virtual void Initialize() override;
	virtual void ExplodeAndDie() override;

	// Set ship to behave as blocking world geometry
	void SetSolid(bool in_state);

	Task<> OpenDoor();
	Task<> CloseDoor();

protected:
	virtual Task<> ManageAI() override;

private:
	std::shared_ptr<SpriteComponent> m_doorSprite;
	std::shared_ptr<SpriteComponent> m_blightSprite;
	std::shared_ptr<SpriteComponent> m_jetSprite1;
	std::shared_ptr<SpriteComponent> m_jetSprite2;

	std::shared_ptr<ColliderComponent_Box> m_collider;
	std::shared_ptr<ColliderComponent_Box> m_collider2;
	Box2f m_worldCollisionBoxLocalFrozen1 = Box2f::FromTopCenter(Vec2f{ 0.0f, 0.0f }, { 96.0f, 48.0f });
	Box2f m_worldCollisionBoxLocalFrozen2 = Box2f::FromTopCenter(Vec2f{ 0.0f, 16.0f }, { 128.0f, 16.0f });

	float m_triggerDist = 80.0f;
	Vec2f m_direction = Vec2f::Down;
	bool m_bIsSolid = false;
};
