#pragma once

#include "Creature.h"

// Turret:
class Turret : public Creature {
	DECLARE_SPAWNABLE_TYPE(Turret);
public:
	Turret(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name);
	virtual void Initialize() override;
	virtual void ExplodeAndDie() override;

protected:
	virtual Task<> ManageAI() override;
	bool m_bIsStationary = false;
	bool m_bIsSniper = false;
	bool m_bTriggerable = true;

private:
	Vec2f m_upVec = Vec2f::Up;
	Vec2f m_direction = Vec2f::Right;
	int32_t m_projectilesLive = 0;
	int32_t m_maxProjectiles = 0;
};
// TurretStill:
class TurretStill : public Turret {
	DECLARE_SPAWNABLE_TYPE(TurretStill);
public:
	TurretStill(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name);
	virtual void Initialize() override;
};