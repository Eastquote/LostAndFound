#pragma once

#include "Creature.h"

// Blobber creature (spawns a clump of lil dudes that grow + swarm-hop at player when triggered)
class Blobber : public Creature {
	DECLARE_SPAWNABLE_TYPE(Blobber);
public:
	Blobber(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name);
	virtual void Initialize() override;
	virtual void ExplodeAndDie() override;

protected:
	virtual Task<> ManageAI() override;

private:
	int32_t m_spawnIndex = 0;
};