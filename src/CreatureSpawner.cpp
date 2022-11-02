#include "CreatureSpawner.h"

#include "GameWorld.h"
#include "Creature.h"
#include "Algorithms.h"
#include <iostream>

//--- CREATURE BASE CLASS CODE ---//

CreatureSpawner::CreatureSpawner(std::shared_ptr<SpawnerDefBase> in_def, bool in_dir, float in_rotation, std::string in_name)
	: m_def(in_def)
	, m_bStartFlipped(in_dir)
	, m_rotation(in_rotation)
	, m_name(in_name)
{
}
void CreatureSpawner::Initialize() {
	GameActor::Initialize();
	m_bCanRespawn = m_def->minRespawnDistX >= 0.0f ? true : false;
}
Task<> CreatureSpawner::ManageActor() {
	auto elapsedTime = 0.0f;
	auto spawnFacingPlayer = (m_def->proximitySpawnDistX >= 0.0f);
	while(true) {
		if(m_bIsActive) {
			auto dist = DistanceFromPlayer();
			if(ShouldSpawn(dist, elapsedTime)) {
				if(m_def->spawnFacingPlayer) {
					m_bStartFlipped = dist.x < 0 ? false : true;
				}
				m_children.push_back(m_def->Spawn(	AsShared<Object>(), AsShared<CreatureSpawner>(), 
													m_bStartFlipped, m_rotation, m_name));
				if(m_currentAlive == m_def->maxAlive) {
					m_bHasSpawned = true;
				}
				elapsedTime = 0.0f;
			}
			elapsedTime += DT();
		}
		EraseInvalid(m_children);
		m_bWasActiveLastFrame = m_bIsActive;
		co_await Suspend();
	}
}
void CreatureSpawner::SetActive(bool in_state) { 
	m_bIsActive = in_state;
	if(!m_bWasActiveLastFrame) {
		m_bHasSpawned = false;
		m_justTriggered = false;
	}
}
bool CreatureSpawner::ShouldSpawn(const Vec2f& in_dist, float in_elapsedTime) {
	if(m_bCanRespawn || (!m_bCanRespawn && !m_bHasSpawned)) {
		if(m_bIsActive && m_currentAlive < m_def->maxAlive) {
			// Spawn heuristics
			bool playerJustEnteredRoom = (m_def->spawnOnEnter && !m_bWasActiveLastFrame);
			bool isTriggerable = m_def->spawnOnTrigger != eTriggerTarget::None;
			
			// Respawn heuristics
			bool satisfiesCooldown =	(m_def->spawnCooldown == -1.0f) || ((m_def->spawnCooldown >= 0.0f) && 
										(in_elapsedTime >= m_def->spawnCooldown));
			bool withinProximitySpawnRange = (std::abs(in_dist.x) < m_def->proximitySpawnDistX) && (in_dist.y < 0.0f);
			bool canProximitySpawn = !m_def->spawnOnEnter;
			bool withinRespawnMinRange = false; // (std::abs(in_dist.x) > m_def->minRespawnDistX); //< Disabled this check to prevent in-room respawning across the board
			
			// Non-triggerable spawn case (default)
			if(!isTriggerable && (playerJustEnteredRoom || !m_bHasSpawned)) {
				return true;
			}

			// Triggerable spawn case
			else if(isTriggerable && m_justTriggered) {
				m_justTriggered = false;
				return true;
			}

			// Respawn case (currently disabled -- see withinRespawnMinRange above)
			else if(satisfiesCooldown) {
				if(!m_def->spawnOnEnter && m_bCanRespawn && canProximitySpawn && withinProximitySpawnRange && withinRespawnMinRange) {
					return true;
				}
				else if(m_def->spawnOnEnter && m_bCanRespawn && !canProximitySpawn && withinRespawnMinRange) {
					return true;
				}
			}
		}
	}
	return false;
}
void CreatureSpawner::AwakenAll() {
	for(auto child : m_children) {
		child->Awaken();
	}
}
