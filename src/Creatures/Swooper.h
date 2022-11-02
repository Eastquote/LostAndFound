#pragma once

#include "Creature.h"

// Swooper creature (waits on ceiling til triggered, then swoops parabolically down towards player)
class Swooper : public Creature {
	DECLARE_SPAWNABLE_TYPE(Swooper);
public:
	Swooper(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name);
	virtual void Initialize() override;

protected:
	virtual Task<> ManageAI() override;
	virtual void ExplodeAndDie() override;

private:
	float m_triggerDist = 5.0f * 16.0f; //< 5 tiles, basically
	float m_swoopHeight = 156.0f;
};