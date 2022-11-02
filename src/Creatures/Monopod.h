#pragma once

#include "Creature.h"

// Monopod creature (armored creature with destroyable leg)
class Monopod : public Creature {
	DECLARE_SPAWNABLE_TYPE(Monopod);
public:
	Monopod(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name);
	virtual void Initialize() override;
	virtual void ChangeHealth(int32_t in_change, bool hitArmor = false) override;

protected:
	virtual Task<> ManageAI() override;
	virtual void ExplodeAndDie() override;

private:
	Box2f m_worldCollisionBoxLocalLegs = Box2f::FromCenter(Vec2f{ 0.0f, -8.0f }, { 4.0f, 16.0f });
	std::shared_ptr<SpriteComponent> m_spriteLegs;
	bool m_bJustDestroyedLeg = false;
};
