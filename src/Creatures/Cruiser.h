#pragma once

#include "Creature.h"

// Cruiser creature (flies horizontally while sine-oscillating vertically, reverses on wall touches)
class Cruiser : public Creature {
	DECLARE_SPAWNABLE_TYPE(Cruiser);
public:
	Cruiser(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name);
	virtual void Initialize() override;

protected:
	virtual Task<> ManageAI() override;
	virtual void ExplodeAndDie() override;
};