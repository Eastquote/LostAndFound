#pragma once

#include "Creature.h"

// Charger creature (low-lying creature that rushes at player's legs when triggered)
class Charger : public Creature {
	DECLARE_SPAWNABLE_TYPE(Charger);
public:
	Charger(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name);
	virtual void Initialize() override;
	virtual void ExplodeAndDie() override;

protected:
	virtual Task<> ManageAI() override;
};
