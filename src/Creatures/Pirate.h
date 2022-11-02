#pragma once

#include "Creature.h"

// Pirate creature (teleports in, seeks out player on foot, fires projectiles)
class Pirate : public Creature {
	DECLARE_SPAWNABLE_TYPE(Pirate);
public:
	Pirate(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name);
	virtual void Initialize() override;
	virtual void ExplodeAndDie() override;

protected:
	virtual Task<> ManageAI() override final;
};