#pragma once

#include "GameActor.h"
#include "GameEnums.h"
#include <array>

class CreatureSpawner;
class Creature;

struct SpawnerDefBase {
	SpawnerDefBase(int32_t in_maxAlive, float in_spawnCooldown, bool in_spawnOnEnter, float in_proximitySpawnDistX, float in_minRespawnDistX, bool in_spawnFacingPlayer, const std::array<float, 6>& in_dropOdds, eTriggerTarget in_spawnOnTrigger)
		: maxAlive(in_maxAlive)
		, spawnCooldown(in_spawnCooldown)
		, spawnOnEnter(in_spawnOnEnter)
		, proximitySpawnDistX(in_proximitySpawnDistX)
		, minRespawnDistX(in_minRespawnDistX)
		, spawnFacingPlayer(in_spawnFacingPlayer)
		, dropOdds(in_dropOdds)
		, spawnOnTrigger(in_spawnOnTrigger)
	{
	}
	virtual ~SpawnerDefBase() {}
	virtual std::shared_ptr<Creature> Spawn(std::shared_ptr<Object> in_owner, std::shared_ptr<CreatureSpawner> in_spawner, bool in_direction, float in_rotation, std::string in_name) const = 0;
	int32_t maxAlive = 1;
	float spawnCooldown = -1.0f;		//< -1 means "only spawn based on proximity (ignore time)"
	bool spawnOnEnter = true;			//< True means "spawn as soon as player starts to enter enclosing room"
	float proximitySpawnDistX = 80.0f;	//< If m_spawnOnEnter is false, spawn when player is WITHIN this X distance
	float minRespawnDistX = 255.0f;		//< Once killed, only respawn when player is OUTSIDE this X distance (-1.0 means "can't respawn")
	bool spawnFacingPlayer = false;
	std::array<float, 6> dropOdds = { 0.0f , 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }; //< Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
	eTriggerTarget spawnOnTrigger = eTriggerTarget::None;	//< If != None, only spawn when player touches a Trigger object with same eTriggerTarget as this value
};

// All in-game enemies are created by these CreatureSpawners -- 
// they are hand-placed in the map and their spawn behavior is determined by their SpawnerDef
class CreatureSpawner : public GameActor {
public:
	CreatureSpawner(std::shared_ptr<SpawnerDefBase> in_def, bool in_dir = false, float in_rotation = 0.0f, std::string in_name = "");
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;

	// Toggles spawner's ability to spawn at all
	// (typically activated on all spawners within a room when the room is entered, deactivated on exit)
	void SetActive(bool in_state);

	// Evaluates all the heuristics to determine if creature should spawn now
	bool ShouldSpawn(const Vec2f& in_dist, float in_elapsedTime);

	// Wakes up all creatures who are children of this spawner (most relevant with spawners with multiple children e.g. Blobber)
	void AwakenAll();

	eTriggerTarget IsTriggerable() const { return m_def->spawnOnTrigger; }
	void TriggerSpawn() { m_justTriggered = true; }
	bool GetActive() const { return m_bIsActive; }
	int32_t GetAliveCount() const { return m_currentAlive; }
	const std::string& GetName() const { return m_name; }
	const std::array<float, 6>& GetDropOdds() const { return m_def->dropOdds; }
	void ChangeAliveCount(int32_t in_change) { m_currentAlive += in_change; }

private:
	std::shared_ptr<SpawnerDefBase> m_def;
	bool m_bWasActiveLastFrame = false;
	bool m_bIsActive = false;
	int32_t m_currentAlive = 0;
	bool m_bHasSpawned = false;
	bool m_bCanRespawn = true;
	bool m_bStartFlipped = false;
	std::string m_name;
	float m_rotation = 0.0f;
	std::vector<std::shared_ptr<Creature>> m_children;
	bool m_justTriggered = false;
};

template<typename T>
struct SpawnerDef : public SpawnerDefBase { // the Abstract Factory's concrete templated child class
	using SpawnerDefBase::SpawnerDefBase;
	virtual std::shared_ptr<Creature> Spawn(std::shared_ptr<Object> in_owner, std::shared_ptr<CreatureSpawner> in_spawner, 
											bool in_direction, float in_rotation, std::string in_name) const override {
		auto creature = Actor::Spawn<T>(in_owner, { in_spawner->GetWorldPos() }, in_spawner, in_direction, in_rotation, in_name);
		GameWorld::Get()->GetEnclosingRoom(in_spawner->GetWorldPos())->creatures.push_back(creature);
		return creature;
	}
};