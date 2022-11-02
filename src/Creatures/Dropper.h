#pragma once

#include "Creature.h"

// Dropper creature (drops from ceiling towards player when triggered, lobs grenades as it digs into ground)
class Dropper : public Creature {
	DECLARE_SPAWNABLE_TYPE(Dropper);
public:
	Dropper(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name);
	virtual void Initialize() override;

protected:
	virtual Task<> ManageAI() override;
	virtual void ExplodeAndDie() override;

private:
	float m_triggerDist = 40.0f;
};