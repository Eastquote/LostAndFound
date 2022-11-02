#include "Creatures/Charger.h"

#include "GameWorld.h"
#include "ParticleSpawnerDefs.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ColliderComponent.h"

std::shared_ptr<SpawnerDef<Charger>> Charger::s_spawnerDef = std::make_shared<SpawnerDef<Charger>>(
	1,		  	// maxAlive: maximum that can be alive at the same time
	-1.0f,	  	// spawnCooldown: -1 means "only spawn based on proximity (ignore time)"
	true,	  	// spawnOnEnter: true means "spawn as soon as player starts to enter enclosing room"
	-1.0f,	  	// triggerDistX: if m_spawnOnEnter is false,  spawn when player is WITHIN this X distance
	255.0f,	  	// minRespawnDistX: once killed, only respawn when player is OUTSIDE this X distance
	false,		// spawnFacingPlayer: should they?
	std::array<float, 6>{ 54.8f, 1.0f, 8.2f, 2.0f, 2.0f, 34.0f }, // dropOdds: Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
	eTriggerTarget::None	// spawnOnTrigger: true means enemy will spawn when player touches a Trigger object with same name as this spawner's SpawnType
);

//-- CHARGER CREATURE CODE --//

Charger::Charger(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name)
	: Creature(in_instigator, in_direction)
{
	m_initialCollisionDims = { 32.0f, 8.0f };
	SetHealth(1);
	SetArmor(10);
	m_moveSpeed = 0.0f;
}
void Charger::Initialize() {
	Creature::Initialize();
	SetDamageInfo(6);
	m_bUpdatesOffScreen = true;

	// Touch sensors setup
	ClearDefaultSensor();
	AddCollisionBox({ 0.0, 2.0f }, { 16.0f, 8.0f }, "weakSpot");
	m_hitSensors.find("weakSpot")->second->SetFiltering(CL_Enemy, CL_PlayerBullet | CL_PlayerBomb | CL_Player);
	AddCollisionBox({ 0.0, 0.0f }, { 32.0f, 8.0f }, "body");
	m_hitSensors.find("body")->second->SetFiltering(CL_Enemy, CL_PlayerBullet | CL_PlayerBomb | CL_Player);

	SpawnSurfaceCheck(true, false, false, true);
	m_sprite->PlayAnim("Charger/IdleSleeping", true);
}
Task<> Charger::ManageAI() {
	// Starts on ground, awakened by proximity on ground nearby
	auto accelRate = 0.05f * 60.0f;
	auto maxSpeed = 1.5f * 60.0f;
	auto timeoutTimer = 3.0f;
	auto triggerRange = 130.0f;
	auto timeoutRadius = 150.0f;
	auto wakeupTrigger = Box2f::FromCenter(Vec2f{ 0.0f, 4.0f }, { triggerRange, 16.0f });
	while(true) {
		// Wait for player to trigger
		co_await WaitUntil([this, wakeupTrigger] { return CheckTrigger(wakeupTrigger); });
		co_await m_sprite->PlayAnim("Charger/Appear", false);

		// Charge at player
		while(true) {
			auto playerDist = DistanceToPlayer();
			auto playerDir = playerDist.x > 0.0f ? 1.0f : -1.0f;
			MinMaxf speedRange;
			speedRange.m_min = -maxSpeed;
			speedRange.m_max = maxSpeed;
			auto direction = Vec2f{ std::copysign(1.0f, m_moveSpeed), 0.0f };

			// Stop cold at ledges
			if(AtLedge(Vec2f::Up, direction) == 1.0f) {
				m_moveSpeed = 0.0f;
				speedRange.m_max = 0.0f;
			}
			else if(AtLedge(Vec2f::Up, direction) == -1.0f) {
				m_moveSpeed = 0.0f;
				speedRange.m_min = -0.0f;
			}
			m_moveSpeed = std::clamp(m_moveSpeed + (accelRate * playerDir), speedRange.m_min, speedRange.m_max);
			Move(GetWorldPos() + Vec2f{ m_moveSpeed * DT(), 0.0f });

			// Timeout if player stays away for a bit
			if(playerDist.Len() > timeoutRadius) {
				timeoutTimer -= DT();
				if(timeoutTimer <= 0.0f) {
					timeoutTimer = 5.0f;
					break;
				}
			}
			co_await Suspend();
		}

		// After timeout, decelerate smoothly
		while(abs(m_moveSpeed) > accelRate) {
			m_moveSpeed *= 0.9f;
			Move(GetWorldPos() + Vec2f{ m_moveSpeed * DT(), 0.0f });
			co_await Suspend();
		}
		m_moveSpeed = 0.0f;

		// Go to sleep
		m_sprite->PlayAnim("Charger/Disappear", false);
		co_await WaitSeconds(0.3f);
		m_sprite->PlayAnim("Charger/IdleSleeping", true);
		co_await Suspend();
	}
}
void Charger::ExplodeAndDie() {
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
