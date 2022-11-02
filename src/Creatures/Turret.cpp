#include "Creatures/Turret.h"

#include "GameWorld.h"
#include "Projectile.h"
#include "ParticleSpawnerDefs.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ColliderComponent.h"

std::shared_ptr<SpawnerDef<Turret>> Turret::s_spawnerDef = std::make_shared<SpawnerDef<Turret>>(
	1,		  	// maxAlive: maximum that can be alive at the same time
	-1.0f,	  	// spawnCooldown: -1 means "only spawn based on proximity (ignore time)"
	true,	  	// spawnOnEnter: true means "spawn as soon as player starts to enter enclosing room"
	-1.0f,	  	// triggerDistX: if m_spawnOnEnter is false,  spawn when player is WITHIN this X distance
	-1.0f,	  	// minRespawnDistX: once killed, only respawn when player is OUTSIDE this X distance
	false,		// spawnFacingPlayer: should they?
	std::array<float, 6>{ 54.8f, 8.0f, 1.0f, 2.0f, 2.0f, 34.0f }, // dropOdds: Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
	eTriggerTarget::None	// spawnOnTrigger: true means enemy will spawn when player touches a Trigger object with same name as this spawner's SpawnType
);
std::shared_ptr<SpawnerDef<TurretStill>> TurretStill::s_spawnerDef = std::make_shared<SpawnerDef<TurretStill>>(
	1,		  	// maxAlive: maximum that can be alive at the same time
	-1.0f,	  	// spawnCooldown: -1 means "only spawn based on proximity (ignore time)"
	true,	  	// spawnOnEnter: true means "spawn as soon as player starts to enter enclosing room"
	-1.0f,	  	// triggerDistX: if m_spawnOnEnter is false,  spawn when player is WITHIN this X distance
	255.0f,	  	// minRespawnDistX: once killed, only respawn when player is OUTSIDE this X distance
	false,		// spawnFacingPlayer: should they?
	std::array<float, 6>{ 54.8f, 8.0f, 1.0f, 2.0f, 2.0f, 34.0f }, // dropOdds: Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
	eTriggerTarget::None	// spawnOnTrigger: true means enemy will spawn when player touches a Trigger object with same name as this spawner's SpawnType
);

//-- TURRET CREATURE CODE --//

Turret::Turret(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name)
	: Creature(in_instigator, in_direction)
{
}
void Turret::Initialize() {
	Creature::Initialize();
	SetDamageInfo(10);
	// World collision boxes setup based on surrounding geometry
	m_upVec = SpawnSurfaceCheck(); //< Determine which way is "up" (as turrets can "stick" to all four surface orientations)
	auto localCollsionOffset = m_upVec * -1.0f * 1.0f;
	auto localCollisionLedgeOffset = m_upVec * -1.0f * 1.0f;
	m_worldCollisionBoxLocal = Box2f::FromCenter(localCollsionOffset, { 12.0f, 12.0f });
	m_worldCollisionBoxLocalLedge = Box2f::FromCenter(localCollisionLedgeOffset, { 8.0f, 8.0f });

	// Actually move/rotate creature to be flush against nearest surface
	m_upVec = SpawnSurfaceCheck(false, false, false, true);
	m_direction = m_upVec.RotateDeg(-90); //< rotate to match
	m_sprite->SetWorldRot(m_upVec.SignedAngleDeg() - 90.0f);
	m_spriteLit->SetWorldRot(m_upVec.SignedAngleDeg() - 90.0f);

	// Touch sensors setup
	ClearDefaultSensor();
	AddCollisionBox({ 0.0, 0.0f }, { 16.0f, 16.0f }, "weakSpot");
	m_hitSensors.find("weakSpot")->second->SetFiltering(CL_Enemy, CL_PlayerBullet | CL_PlayerBomb | CL_Player);

	// Sprite setup
	m_sprite->SetPlayRate(1.0f);
	m_sprite->PlayAnim("Turret/Idle", true);
	m_spriteLit->PlayAnim("Turret/IdleLit", true);

	// Behavior setup
	SetHealth(2);
	SetArmor(0);
	m_moveSpeed = 0.5f * 60.0f;
	m_bUpdatesOffScreen = true;
}
Task<> Turret::ManageAI() {
	auto world = GetWorld();
	auto GetVel = [this] {return m_direction * m_moveSpeed; };
	CardinalUpdate(m_blockCardinals);

	// Set up offset to handle    left/right OR           up/down movement cases
	int64_t dirOffset = m_upVec == Vec2f::Up || m_upVec == Vec2f::Down ? 2 : 0;
	std::array<Vec2f, 4> dirs = {
		Vec2f::Up,
		Vec2f::Down,
		Vec2f::Right,
		Vec2f::Left
	};
	auto fireZoneDims = Vec2f{ 255.0f, 64.0f };
	if(dirOffset) {
		fireZoneDims = Vec2f{ fireZoneDims.y, fireZoneDims.x };
	}
	Box2f fireZone = Box2f::FromCenter(m_upVec * 128.0f, fireZoneDims);
	if(m_bIsSniper) {
		m_taskMgr.RunManaged(TryFireBullet(g_creatureBulletDef, std::nullopt, g_turretFireSniper));
	}
	else {
		m_taskMgr.RunManaged(TryFireBullet(g_creatureBulletDef, m_upVec, g_turretFire));
	}
	m_bCanFire = false;
	while(true) {
		std::array<bool, 4> cardinalsBlocked = {
			m_blockCardinals.Up(),
			m_blockCardinals.Down(),
			m_blockCardinals.Right(),
			m_blockCardinals.Left(),
		};

		// Walk back and forth
		if(!m_bIsStationary) {
			Move(GetWorldPos() + GetVel() * DT());
			auto dirSign = abs(m_direction.x) == 1.0f ? std::copysign(1.0f, m_direction.x) : std::copysign(1.0f, m_direction.y);
			if(dirSign == 1.0f && cardinalsBlocked[dirOffset] ||
				dirSign == -1.0f && cardinalsBlocked[dirOffset + (int64_t)1] || AtLedge(m_upVec, m_direction)) {
				m_direction *= -1.0f;
			}
		}

		// Fire when player is in zone of fire OR periodically
		auto fireZoneWorld = fireZone.TransformedBy(GetWorldTransform());
		//DrawDebugBox(fireZoneWorld, sf::Color::Green);
		if(Math::PointInBox(fireZoneWorld, world->GetPlayerWorldPos()) || !m_bTriggerable) {
			m_bCanFire = true;
			//DrawDebugBox(fireZoneWorld, sf::Color::Red);
		}
		else {
			m_bCanFire = false;
		}
		co_await Suspend();
	}
}
void Turret::ExplodeAndDie() {
	if(ShouldChunksplode()) {
		Actor::Spawn<ParticleSpawner>(GetWorld(), { GetWorldPos() },
			g_debrisSpawnerLg1, std::nullopt, "Base");
		Actor::Spawn<ParticleSpawner>(GetWorld(), { GetWorldPos() },
			g_emberSpawnerLg1, std::nullopt, "Base");
		Actor::Spawn<ParticleSpawner>(GetWorld(), { GetWorldPos() },
			g_emberSpawnerLg2, std::nullopt, "Base");
	}
	else {
		Actor::Spawn<ParticleSpawner>(GetWorld(), { GetWorldPos() },
			g_debrisSpawnerSm1, std::nullopt, "Base");
		Actor::Spawn<ParticleSpawner>(GetWorld(), { GetWorldPos() },
			g_emberSpawnerLg1, std::nullopt, "Base");
		Actor::Spawn<ParticleSpawner>(GetWorld(), { GetWorldPos() },
			g_emberSpawnerLg2, std::nullopt, "Base");
	}
	Creature::ExplodeAndDie();
}


//-- TURRETSTILL CREATURE CODE --//

TurretStill::TurretStill(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name)
	: Turret(in_instigator, in_direction, in_rotation, in_name)
{
}
void TurretStill::Initialize() {
	Turret::Initialize();
	m_bIsStationary = true;
	m_bIsSniper = false;
	m_bTriggerable = false;
}
