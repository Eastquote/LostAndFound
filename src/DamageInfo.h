#pragma once

#include "Engine/Actor.h"

class GameActor;

// Used to store and pass around relevant damage info between GameActors during damage-handling
struct DamageInfo {
	std::shared_ptr<GameActor> m_instigator;
	int32_t m_payload = 1;
	uint32_t m_damageFlags = 0;
	Vec2f m_damagePos = Vec2f::Zero;
};
