#pragma once

#include "Creature.h"

// Rammer (rushes FAST along ground towards player if triggered)
class Rammer : public Creature {
	DECLARE_SPAWNABLE_TYPE(Rammer);
public:
	Rammer(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name);
	virtual void Initialize() override;
	virtual void ExplodeAndDie() override;

protected:
	virtual Task<> ManageAI() override;
};