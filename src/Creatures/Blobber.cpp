#include "Creatures/Blobber.h"

#include "GameWorld.h"
#include "ParticleSpawnerDefs.h"
#include "SquirrelNoise5.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ColliderComponent.h"
#include <iostream>

std::shared_ptr<SpawnerDef<Blobber>> Blobber::s_spawnerDef = std::make_shared<SpawnerDef<Blobber>>(
	10,		  	// maxAlive: maximum that can be alive at the same time
	-1.0f,	  	// spawnCooldown: -1 means "only spawn based on proximity (ignore time)"
	true,	  	// spawnOnEnter: true means "spawn as soon as player starts to enter enclosing room"
	-1.0f,	  	// triggerDistX: if m_spawnOnEnter is false,  spawn when player is WITHIN this X distance
	255.0f,	  	// minRespawnDistX: once killed, only respawn when player is OUTSIDE this X distance
	false,		// spawnFacingPlayer: should they?
	std::array<float, 6>{ 54.8f, 8.0f, 1.0f, 2.0f, 2.0f, 34.0f }, // dropOdds: Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
	eTriggerTarget::None			// spawnOnTrigger: true means enemy will spawn when player touches a Trigger object with same name as this spawner's SpawnType
);

//-- BLOBBER CREATURE CODE --//

Blobber::Blobber(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name)
	: Creature(in_instigator, in_direction)
{
	m_spawnIndex = GetSpawner()->GetAliveCount(); // Supplies unique-within-this-spawngroup index to Noise funcs
	m_initialCollisionDims = { 10.0f, 10.0f };
	SetHealth(1);
	SetArmor(0);
	m_moveSpeed = 1.0f * 60.0f;
}
void Blobber::Initialize() {
	Creature::Initialize();
	SetDamageInfo(5);
	m_bUpdatesOffScreen = true;

	// Set up touch sensors
	ClearDefaultSensor();
	AddCollisionBox({ 0.0, 0.0f }, { 14.0f, 14.0f }, "weakSpot");
	m_hitSensors.find("weakSpot")->second->SetFiltering(CL_Enemy, CL_PlayerBullet | CL_PlayerBomb | CL_Player);

	// Set up sprite
	m_sprite->PlayAnim("Blobber/IdleSmall", true);

	// Set position
	SetWorldPos(GetWorldPos() + Vec2f{ 0, Get1dNoiseNegOneToOne(m_spawnIndex) });
	SpawnSurfaceCheck(true, false, false, true);
}
Task<> Blobber::ManageAI() {
	auto currentCreatureCount = GetWorld()->GetEnclosingRoom(GetWorldPos())->creatures.size();
	auto dropGravity = .212f * 60.0f;
	auto maxDropSpeed = 3.0f * 60.0f;
	auto dropSpeed = 0.0f;

	// Starts on ground, awakened by proximity on ground nearby
	auto timeoutTimer = 3.0f;
	auto triggerRange = 190.0f;
	auto timeoutRadius = 100.0f;
	auto hopTimer = 0.0f;
	float* hopTimerPtr = &hopTimer;
	auto dir = SetDir(0.0f); //< Roll random direction
	auto triggerOffset = 0.0f;
	auto wakeupTrigger = Box2f::FromBottomCenter(Vec2f{ 0.0f, -6.0f }, { triggerRange, 33.0f });
	auto noiseIndex = 0;
	auto anchorPos = GetWorldPos();
	auto JumpSelect = [this, &noiseIndex, currentCreatureCount](int32_t in_min = 1, int32_t in_max = 3) {
		auto randFloat = Get1dNoiseZeroToOne(noiseIndex, (uint32_t)currentCreatureCount);
		auto randInt = std::round((randFloat * (in_max - in_min)) + in_min);
		++noiseIndex;
		auto result = randInt + (randFloat * 0.25f);
		return result;
	};
	auto PointAtAnchor = [this](Vec2f in_anchorPos) {
		auto xDist = in_anchorPos.x - GetWorldPos().x;
		if(abs(xDist) < 8.0f) {
			SetDir(0.0f); //< Randomize dir if they're right on top of it, to keep from getting "pinned" on wall
			return;
		}
		auto dir = (in_anchorPos.x - GetWorldPos().x) > 0.0f ? 1.0f : -1.0f;
		SetDir(dir);
	};

	// Randomize start time
	co_await WaitSeconds(Get1dNoiseZeroToOne(m_spawnIndex, m_spawnIndex) * 2.0f);
	while(true) {
		m_sprite->PlayAnim("Blobber/IdleSmall", true);

		// Wait for player to trigger
		auto lastJumpMag = 0.0f;
		while(!m_bAwake) {
			//DrawDebugPoint(anchorPos, sf::Color::Red);
			auto jumpMag = JumpSelect(1, 2) * 60.0f; //< Calculate based on PREVIOUS jump (0.0f the first time through)
			auto recoveryTime = lastJumpMag * 0.7f * DT();
			co_await Hop(jumpMag, maxDropSpeed, hopTimerPtr);
			lastJumpMag = jumpMag;
			if(CheckTrigger(wakeupTrigger)) {
				m_bAwake = true;
				GetSpawner()->AwakenAll();
				break;
			}
			co_await WaitSeconds(recoveryTime);
			PointAtAnchor(anchorPos);
			m_sprite->SetFlipHori(!m_bIsFacingRight);
			co_await Suspend();
		}
		co_await m_sprite->PlayAnim("Blobber/TransitionUp", false);
		m_sprite->PlayAnim("Blobber/IdleLarge", true);
		co_await WaitSeconds(Get1dNoiseZeroToOne(m_spawnIndex, m_spawnIndex) * 1.0f);
		while(m_bAwake) {
			auto playerDist = DistanceToPlayer();
			auto playerDistFloat = playerDist.Len();
			SetDir(GetPlayerDir());
			m_sprite->SetFlipHori(!m_bIsFacingRight);

			// Jump bracketing
			auto jumpMin = 1;
			auto jumpMax = 4;
			if(playerDist.y > 32.0f) { //< Force high jumps when player is high up
				jumpMin = 3;
				jumpMax = 4;
			}
			
			// Possible additional behavior case (disabled for now)
			//if(playerDist.y < -6.0f){ //< Force small jumps when player is below blobs
			//	jumpMin = 1;
			//	jumpMax = 3;
			//}

			auto jumpMag = JumpSelect(jumpMin, jumpMax) * 60.0f;
			auto recoveryTime = lastJumpMag * DT() * 0.20f;
			co_await WaitSeconds(recoveryTime);
			hopTimer = recoveryTime;

			// Jump at player
			m_sprite->PlayAnim("Blobber/JumpLarge", false);
			co_await Hop(jumpMag, maxDropSpeed, hopTimerPtr);
			co_await m_sprite->PlayAnim("Blobber/LandLarge", false);
			m_sprite->PlayAnim("Blobber/IdleLarge", true);

			// Timeout logic
			if(abs(playerDistFloat) > timeoutRadius) {
				timeoutTimer -= hopTimer;
			}
			else {
				timeoutTimer = 3.0f;
			}
			if(timeoutTimer <= 0.0f) {
				m_bAwake = false;
			}
			lastJumpMag = jumpMag;
			co_await Suspend();
		}
		co_await m_sprite->PlayAnim("Blobber/TransitionDown", false);
		anchorPos = GetWorldPos();
		co_await Suspend();
	}
}
void Blobber::ExplodeAndDie() {
	GetSpawner()->AwakenAll(); // Wake up the whole squad -- for REVENGE!
	Actor::Spawn<ParticleSpawner>(GetWorld(), { GetWorldPos() },
		g_debrisSpawnerSm1, std::nullopt, "Base");
	Actor::Spawn<ParticleSpawner>(GetWorld(), { GetWorldPos() },
		g_emberSpawnerLg1, std::nullopt, "Base");
	Actor::Spawn<ParticleSpawner>(GetWorld(), { GetWorldPos() },
		g_emberSpawnerLg2, std::nullopt, "Base");
	Creature::ExplodeAndDie();
}
