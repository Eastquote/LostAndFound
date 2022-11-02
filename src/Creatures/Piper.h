#pragma once

#include "Creature.h"

// Piper creature (spawns on proximity, flies up to y-match player, then rushes player)
class Piper : public Creature {
	DECLARE_SPAWNABLE_TYPE(Piper);
public:
	Piper(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name);
	virtual void Initialize() override;
	virtual void Update() override;

protected:
	virtual Task<> ManageAI() override;
	virtual void ExplodeAndDie() override;
	bool m_bIsMovingRight = true;

private:
	std::string m_floorDir = "Down";
};