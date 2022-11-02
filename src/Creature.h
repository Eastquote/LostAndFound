#pragma once

#include "Character.h"
#include "CreatureSpawner.h"
#include "GameEnums.h"
#include "TokenList.h"
#include <array>

#define DECLARE_SPAWNABLE_TYPE(typename) \
public: \
	static std::shared_ptr<SpawnerDef<typename>> GetSpawnerDef() { return s_spawnerDef; } \
private: \
	static std::shared_ptr<SpawnerDef<typename>> s_spawnerDef

template<typename T> struct SpawnerDef;

class CreatureSpawner;
class Light;
struct DamageInfo;
struct ProjectileDef;
struct FireDef;

// Creature Base Class
class Creature : public Character {
public:
	Creature(std::shared_ptr<CreatureSpawner> in_spawner);
	Creature(std::shared_ptr<CreatureSpawner> in_spawner, bool in_bisMovingRight);
	virtual void Initialize() override;
	virtual void Destroy() override;
	virtual void Update() override;
	virtual Task<> ManageActor() override;
	virtual void FlipDir();

	int32_t GetPayload() const { return GetDamageInfo().m_payload; }
	int32_t GetHealth() const { return m_health; }
	int32_t GetArmor() const { return m_armor; }
	std::string GetName() const { return  m_name; }
	void SetHit(bool in_bool) { m_bHit = in_bool; }
	void ToggleFrozen(bool in_charged = false);
	void AddChill();
	bool CheckFrozen() const { return m_bIsFrozen; }
	void SetLastDamageType(uint32_t in_damageType) { m_lastDamageType = in_damageType; }
	const std::array<float, 6>& GetDropOdds() const { return m_spawner->GetDropOdds(); }
	std::shared_ptr<CreatureSpawner> GetSpawner() { return m_spawner; }
	float GetPlayerDir();
	void Awaken() { m_bAwake = true; }
	int32_t GetLiveProjectileCount() const { return m_liveProjectiles; };
	void ChangeProjectileCount(int32_t in_change);
	std::shared_ptr<SpriteComponent> GetSprite() const { return m_sprite; }

	// Handles damage event effects (change health, effects, sounds) and returns HitResponse type
	eHitResponse HandleDamage(DamageInfo in_dmgInfo, std::shared_ptr<SensorComponent> in_sensor);

protected:
	virtual void ChangeHealth(int32_t in_change, bool hitArmor = false);
	Task<> PlayFireAnim(std::string in_fireAnim, std::string in_fireAnimLit, std::string in_resumeAnim, std::string in_resumeAnimLit);
	virtual std::optional<DamageInfo> ModifyDamage(DamageInfo in_dmgInfo, std::shared_ptr<SensorComponent> in_sensor, bool in_hitArmor);
	virtual void SetupComponents();

	// Only used by subclasses (Projectiles reach into Creatures to throw all the basic touch levers)
	virtual void TouchCallback(std::shared_ptr<GameActor> in_other);

	virtual void UntouchCallback();
	virtual void ExplodeAndDie();
	bool CheckTrigger(Box2f in_trigger);
	virtual Task<> ManageAI();
	virtual Vec2f GetCollisionSize() const;
	void SetHitSensorBox(Vec2f in_dims);
	void AddCollisionBox(Vec2f in_relativePos, Vec2f in_dims, std::string in_name);
	std::shared_ptr<SensorComponent> GetCollisionBox(std::string name);

	// Returns the surface normal of the nearest valid cardinal surface within 255.0f pixels
	// (if in_bMoveToFlush is true, also moves creature to be flush against that surface)
	Vec2f SpawnSurfaceCheck(bool in_bDownOnly = false, bool in_bUpOnly = false, bool in_bSidesOnly = false, bool in_bMoveToFlush = false);

	Task<> TryWalkDistance(float in_distance);
	float AtLedge(Vec2f in_upDir = Vec2f::Up, Vec2f in_moveDir = Vec2f::Right);
	virtual Task<> CheckHealth();
	virtual Task<> PreDeath();
	void SetHealth(int32_t in_val) { m_health = in_val; }
	void SetArmor(int32_t in_val) { m_armor = in_val; }
	Task<> Freeze();
	bool ShouldChunksplode() const;
	void StartPhasedWindow(float in_duration = 1.0f);
	float SetDir(float in_dir = 1.0f);
	Task<> Hop(float in_launchSpeedY, float in_maxFallSpeedY, float* in_timer = nullptr, float in_gravity = 0.212f * 60.0f);
	Task<> TryFall(float in_maxFallSpeedY, float in_gravity);
	virtual Task<> TryFireBullet(const ProjectileDef& in_projectileDef, std::optional<Vec2f> in_vec, const FireDef& in_fireDef);
	void ClearDefaultSensor();

	std::shared_ptr<SensorComponent> m_hitSensor;
	std::map<std::string, std::shared_ptr<SensorComponent>> m_hitSensors;
	std::shared_ptr<SpriteComponent> m_sprite;
	std::shared_ptr<SpriteComponent> m_spriteLit;
	std::shared_ptr<CreatureSpawner> m_spawner;
	uint32_t m_lastDamageType = eDamageFlag::DF_None;
	Vec2f m_initialCollisionDims = { 16.0f, 16.0f };
	bool m_bHit = false;
	bool m_bIsMovingRight = true;
	bool m_bIsFacingRight = true;
	float m_moveSpeed = 0.5f * 60.0f;
	float m_moveSpeedModifier = 1.0f;
	bool m_bJustFlipped = false;
	bool m_bCanFreeze = true;
	bool m_bIsFrozen = false;
	float m_freezeDuration = 4.31667f;
	std::string m_name;
	float m_rotation = 0.0f;
	float m_hitPause = 0.153f;
	bool m_bJustPhased = false;
	float m_phaseDuration = 0.0f;
	bool m_bAwake = false;
	int32_t m_spawnIndex = 0;
	int32_t m_noiseIndex = 0;
	int32_t m_liveProjectiles = 0;
	bool m_bCanFire = true;
	bool m_bCanSpawnPickup = true;
	std::shared_ptr<Light> m_light;

private:
	Task<> m_checkHealthTask;
	Task<> m_manageAITask;
	TokenList<> m_updateAITokens;
	bool ShouldUpdateAI() const { return m_updateAITokens; }
	int32_t m_armor = 0;
	int32_t m_health = 1;
};
