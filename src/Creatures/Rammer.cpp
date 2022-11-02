#include "Creatures/Rammer.h"

#include "ParticleSpawnerDefs.h"
#include "GameWorld.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ColliderComponent.h"

std::shared_ptr<SpawnerDef<Rammer>> Rammer::s_spawnerDef = std::make_shared<SpawnerDef<Rammer>>(
	1,		  	// maxAlive: maximum that can be alive at the same time
	-1.0f,	  	// spawnCooldown: -1 means "only spawn based on proximity (ignore time)"
	true,	  	// spawnOnEnter: true means "spawn as soon as player starts to enter enclosing room"
	-1.0f,	  	// triggerDistX: if m_spawnOnEnter is false,  spawn when player is WITHIN this X distance
	255.0f,	  	// minRespawnDistX: once killed, only respawn when player is OUTSIDE this X distance
	false,		// spawnFacingPlayer: should they?
	std::array<float, 6>{ 54.8f, 1.0f, 8.2f, 2.0f, 2.0f, 34.0f }, // dropOdds: Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
	eTriggerTarget::None	// spawnOnTrigger: true means enemy will spawn when player touches a Trigger object with same name as this spawner's SpawnType
);

//-- RAMMER CREATURE CODE --//

Rammer::Rammer(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name)
	: Creature(in_instigator, in_direction)
{
	m_initialCollisionDims = { 38.0f, 28.0f };
	m_bIsFacingRight = !in_direction;
	m_bIsMovingRight = !in_direction;
	SetHealth(3);
	SetArmor(15);
	m_moveSpeed = 0.0f;
}
void Rammer::Initialize() {
	Creature::Initialize();
	SetDamageInfo(6);
	m_bUpdatesOffScreen = true;

	// Touch sensors setup
	ClearDefaultSensor();
	AddCollisionBox({ 0.0, 0.0f }, { 18.0f, 28.0f }, "weakSpot");
	m_hitSensors.find("weakSpot")->second->SetFiltering(CL_Enemy, CL_PlayerBullet | CL_PlayerBomb | CL_Player);
	AddCollisionBox({ 14.0, 0.0f }, { 10.0f, 28.0f }, "shield");
	m_hitSensors.find("shield")->second->SetFiltering(CL_Armor, CL_PlayerBullet | CL_PlayerBomb | CL_Player);

	// Sprite setup
	m_sprite->PlayAnim("Rammer/IdleSleep", true);
	m_sprite->SetFlipHori(!m_bIsFacingRight);

	SpawnSurfaceCheck(true, false, false, true);
}

Task<> Rammer::ManageAI() {
	// Starts on ground, awakened by proximity on ground nearby
	auto accelRate = 0.05f * 60.0f;
	auto maxSpeed = 2.7f * 60.0f;
	auto timeoutTimer = 3.0f;
	auto triggerRange = 130.f;
	auto timeoutRadius = 150.0f;
	auto awake = false;
	auto dir = m_bIsFacingRight ? 1.0f : -1.0f;
	auto triggerOffset = 22.5f;
	auto wakeupTrigger = Box2f::FromCenter(Vec2f{ (triggerRange / 2) * dir, -5.0f }, { triggerRange, 16.0f });
	while(true) {
		// Wait for player to trigger (only triggers if creature is FACING player)
		co_await WaitUntil([this, wakeupTrigger] { return CheckTrigger(wakeupTrigger); });
		awake = true;

		// Get ready to ram
		while(awake) {
			auto dir = m_bIsFacingRight ? 1.0f : -1.0f;
			auto playerDir = GetPlayerDir();
			if(dir != playerDir) {
				FlipDir();
			}
			m_sprite->SetFlipHori(!m_bIsFacingRight);
			m_hitSensors.find("shield")->second->SetWorldPos({ GetWorldPos().x + (14.0f * dir), GetWorldPos().y });

			// Wakeup anim
			m_sprite->PlayAnim("Rammer/IdleAlert", false);
			co_await WaitSeconds(0.75f);

			m_sprite->PlayAnim("Rammer/Ram", true);
			MinMaxf speedRange;
			speedRange.m_min = -maxSpeed;
			speedRange.m_max = maxSpeed;

			// It's rammin' time!
			while(true) {
				// Accelerate towards player
				auto animSpeed = Math::MapRange(abs(m_moveSpeed), 0.0f, maxSpeed, 0.1f, 1.0f);
				m_sprite->SetPlayRate(animSpeed);
				m_moveSpeed = std::clamp(m_moveSpeed + (accelRate * dir), speedRange.m_min, speedRange.m_max);
				auto direction = Vec2f{ m_moveSpeed * DT(), 0.0f };
				auto directionNorm = direction.Norm();
				Move(GetWorldPos() + direction);

				// If it hits a wall/ledge
				if(m_blockCardinals.Right() || m_blockCardinals.Left() || AtLedge(Vec2f::Up, directionNorm)) {
					// Stop moving
					if(AtLedge(Vec2f::Up, directionNorm) == 1.0f) {
						speedRange.m_max = 0.0f;
					}
					else if(AtLedge(Vec2f::Up, directionNorm) == -1.0f) {
						speedRange.m_min = 0.0f;
					}
					m_moveSpeed = 0.0f;
					m_sprite->SetPlayRate(1.0f);

					// Take a beat
					m_sprite->PlayAnim("Rammer/IdleAlert", true);
					co_await WaitSeconds(1.0f);

					// Flip around
					FlipDir();
					dir = m_bIsFacingRight ? 1.0f : -1.0f;
					m_sprite->SetFlipHori(!m_bIsFacingRight);
					m_hitSensors.find("shield")->second->SetWorldPos({ GetWorldPos().x + (14.0f * dir), GetWorldPos().y });
					wakeupTrigger = Box2f::FromCenter(Vec2f{ (triggerRange / 2) * dir, -5.0f }, { triggerRange, 16.0f });

					// If not retriggered, go back to sleep
					if(!CheckTrigger(wakeupTrigger)) {
						m_sprite->SetPlayRate(0.0f);
						m_sprite->PlayAnim("Rammer/IdleAlert", true, 0);
						co_await WaitSeconds(1.0f);
						m_sprite->SetPlayRate(1.0f);
						m_sprite->PlayAnim("Rammer/IdleSleep", true);
						awake = false;
						break;
					}

					// Otherwise, start ramming again at the top of the while(awake) loop
					else {
						break;
					}
				}
				co_await Suspend();
			}
			co_await Suspend();
		}
		co_await Suspend();
	}
}
void Rammer::ExplodeAndDie() {
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
