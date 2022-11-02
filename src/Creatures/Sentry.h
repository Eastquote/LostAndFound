#pragma once

#include "Creature.h"

// Sentry creature (patrols horizontally, reversing direction when it hits walls -- invincible)
class Sentry : public Creature {
	DECLARE_SPAWNABLE_TYPE(Sentry);
public:
	Sentry(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name);
	virtual void Initialize() override;
	virtual void FlipDir() override;

protected:
	virtual Task<> ManageAI() override;
};