#include "Creatures/Pirate.h"

#include "SquirrelNoise5.h"
#include "Projectile.h"
#include "ParticleSpawnerDefs.h"
#include "GameWorld.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ColliderComponent.h"

std::shared_ptr<SpawnerDef<Pirate>> Pirate::s_spawnerDef = std::make_shared<SpawnerDef<Pirate>>(
	1,		  	// maxAlive: maximum that can be alive at the same time
	5.0f,	  	// spawnCooldown: -1 means "only spawn based on proximity (ignore time)"
	false,	  	// spawnOnEnter: true means "spawn as soon as player starts to enter enclosing room"
	-1.0f,	  	// triggerDistX: if m_spawnOnEnter is false,  spawn when player is WITHIN this X distance
	-1.0f,	  	// minRespawnDistX: once killed, only respawn when player is OUTSIDE this X distance
	false,		// spawnFacingPlayer: should they?
	std::array<float, 6>{ 54.8f, 8.0f, 1.0f, 2.0f, 2.0f, 34.0f }, // dropOdds: Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
	eTriggerTarget::Pirate	// spawnOnTrigger: a non-None value means an enemy will spawn when player touches a Trigger object with same name as this spawner's SpawnType
);

// [CHECK] is this a useful thing?
//#define DECLARESPAWNABLETYPE(typename, maxAlive, spawnCooldown, spawnOnEnter, triggerDistX, minRespawnDistX, spawnFacingPlayer, dropOdds, spawnOnTrigger) \
//std::shared_ptr<SpawnerDef<typename>> typename::s_spawnerDef = std::make_shared<SpawnerDef<typename>>(\
//maxAlive,		  	/* maxAlive: maximum that can be alive at the same time*/\
//spawnCooldown,	  	/* spawnCooldown: -1 means "only spawn based on proximity (ignore time)"*/\
//spawnOnEnter,	  	/* spawnOnEnter: true means "spawn as soon as player starts to enter enclosing room"*/\
//triggerDistX,	  	/* triggerDistX: if m_spawnOnEnter is false,  spawn when player is WITHIN this X distance*/\
//minRespawnDistX,	  	/* minRespawnDistX: once killed, only respawn when player is OUTSIDE this X distance*/\
//spawnFacingPlayer,		/* spawnFacingPlayer: should they?*/\
//dropOdds, /* dropOdds: Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade*/\
//spawnOnTrigger	/* spawnOnTrigger: a non-None value means an enemy will spawn when player touches a Trigger object with same name as this spawner's SpawnType*/\
//)

//-- PIRATE CREATURE CODE --//

Pirate::Pirate(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name)
	: Creature(in_instigator, in_direction)
{
	m_initialCollisionDims = { 14.0f, 28.0f };
	SetHealth(15);
	SetArmor(15);
	m_moveSpeed = 1.0f * 60.0f;
}
void Pirate::Initialize() {
	Creature::Initialize();
	SetDamageInfo(3);
	m_bUpdatesOffScreen = true;

	// Touch sensors setup
	ClearDefaultSensor();
	AddCollisionBox({ 0.0, 0.0f }, { 18.0f, 28.0f }, "weakSpot");
	m_hitSensors.find("weakSpot")->second->SetFiltering(CL_None, CL_PlayerBullet | CL_PlayerBomb | CL_Player);

	// Behavior setup
	SpawnSurfaceCheck(true, false, false, true);
}
Task<> Pirate::ManageAI() {
	auto dropGravity = .212f * 60.0f;
	auto maxDropSpeed = 4.0f * 60.0f;
	auto dropSpeed = 0.0f;
	auto maxSpeed = 2.7f * 60.0f;
	auto timeoutTimer = 3.0f;
	auto fireRange = 120.0f;
	auto activeRange = 200.0f;
	auto timeoutRadius = 150.0f;
	auto dir = m_bIsFacingRight ? 1.0f : -1.0f;
	auto playerDir = GetPlayerDir();
	if(playerDir == -1.0f) {
		FlipDir();
	}
	m_sprite->SetFlipHori(!m_bIsFacingRight);
	m_sprite->SetPlayRate(3.0f);

	// Teleport in
	co_await m_sprite->PlayAnim("Pirate/Teleport", false);
	m_sprite->SetPlayRate(2.0f);
	m_sprite->PlayAnim("Pirate/Idle", true);
	m_hitSensors.find("weakSpot")->second->SetFiltering(CL_Enemy, CL_PlayerBullet | CL_PlayerBomb | CL_Player);
	auto xTargetDir = m_bIsFacingRight ? 1.0f : -1.0f;
	auto PointAtPlayer = [this]() {
		auto dir = m_bIsFacingRight ? 1.0f : -1.0f;
		auto distToPlayer = DistanceToPlayer();
		auto playerIsBehindYou = std::copysignf(1.0f, distToPlayer.x) != dir;
		if(playerIsBehindYou) {
			FlipDir();
			m_sprite->SetFlipHori(!m_bIsFacingRight);
		}
		return m_bIsFacingRight ? 1.0f : -1.0f;
	};
	auto bulletSpawnOffset = Vec2f{ -6.0f, 8.0f };
	auto randomStep = 0;

	// Set up behavioral heuristics
	bool bPlayerInYRange = false;
	bool bPlayerBelow = false;
	bool bPlayerAbove = false;
	bool bPlayerInXRange = false;
	bool bPlayerOutsideActiveRange = false;
	auto UpdateHeuristics = [	this, &bPlayerInXRange, &bPlayerInYRange, &bPlayerAbove, &bPlayerBelow, 
								&bPlayerOutsideActiveRange, &dir, fireRange, activeRange]() {
		auto distToPlayer = DistanceToPlayer();
		bPlayerInYRange = distToPlayer.y < 20.0f && distToPlayer.y > -12.0f;
		bPlayerBelow = distToPlayer.y <= -12.0f;
		bPlayerAbove = distToPlayer.y >= 20.0f;
		bPlayerInXRange = abs(distToPlayer.x) < fireRange;
		bPlayerOutsideActiveRange = distToPlayer.Len() > activeRange;
		dir = m_bIsFacingRight ? 1.0f : -1.0f;
	};
	auto walking = false;

	// Inner behavior loop
	while(true) {
		UpdateHeuristics();

		// If player is within your Y-range, set xDirection
		if(bPlayerInYRange || bPlayerAbove) {
			auto prevXTargetDir = xTargetDir;
			//printf("Player in Y range!\n");
			xTargetDir = PointAtPlayer();
			if(xTargetDir != prevXTargetDir) {
				m_sprite->PlayAnim("Pirate/Idle", true);
				walking = false;
				co_await WaitSeconds(0.5f);
			}
		}

		// If player is far on X or Y, try to close in
		auto movePos = GetWorldPos() + Vec2f{ abs(m_moveSpeed) * DT() * xTargetDir, 0.0f };
		if(!bPlayerInXRange || !bPlayerInYRange) {
			if(!walking) {
				m_sprite->PlayAnim("Pirate/Run", true);
				walking = true;
			}
			Move(movePos);
		}
		CardinalUpdate(m_blockCardinals);

		// If at wall
		if((m_blockCardinals.Left() || m_blockCardinals.Right()) && m_blockCardinals.Down()) {
			// Jump (high)

			m_sprite->PlayAnim("Pirate/Jump", true);
			co_await Hop(5.0f * 60.0f, 3.0f * 60.0f);
			m_sprite->PlayAnim("Pirate/Idle", true);
		}

		// If at ledge:
		else if(AtLedge(Vec2f::Up, Vec2f{ xTargetDir, 0.0f })) {
			if(bPlayerBelow) {
				m_sprite->PlayAnim("Pirate/Run", true);
				co_await TryWalkDistance(20.0f * xTargetDir);
				m_sprite->PlayAnim("Pirate/Idle", true);
			}
			else if(bPlayerInYRange) {
				// Proper forward hop
				m_sprite->PlayAnim("Pirate/Jump", true);
				co_await Hop(2.0f * 60.0f, 3.0f * 60.0f);
				m_sprite->PlayAnim("Pirate/Idle", true);
			}
			else {
				// Higher forward hop
				m_sprite->PlayAnim("Pirate/Jump", true);
				co_await Hop(4.0f * 60.0f, 3.0f * 60.0f);
				m_sprite->PlayAnim("Pirate/Idle", true);
			}
		}
		if(!m_blockCardinals.Down()) {
			m_sprite->PlayAnim("Pirate/Jump", true);
			co_await TryFall(maxDropSpeed, dropGravity);
			m_sprite->PlayAnim("Pirate/Idle", true);
		}

		// If player is in firing zone, fire
		if(bPlayerInXRange && bPlayerInYRange) {
			walking = false;
			m_sprite->PlayAnim("Pirate/Idle", true);
			auto seed = (uint32_t)std::round(GetWorldPos().x + GetWorldPos().y);
			// TODO: add check for world geo obstructions!
			auto randomVal = Get1dNoiseZeroToOne(randomStep, seed);
			auto stateCount = 4;
			if(randomVal < 1.0f - (1.0f / stateCount)) {
				// Fire one shot
				co_await TryFireBullet(g_creatureBulletDefFaster, Vec2f{ xTargetDir, 0.0f }, g_pirateFireOne);
			}
			else if(randomVal < 1.0f - (2.0f / stateCount)) {
				// Fire two shots
				co_await TryFireBullet(g_creatureBulletDefFaster, Vec2f{ xTargetDir, 0.0f }, g_pirateFireTwo);
			}
			else if(randomVal < 1.0f - (3.0f / stateCount)) {
				// TODO: show Charge effect antic here
				
				// TODO: Fire special Charge shot here
				co_await TryFireBullet(g_creatureBulletDefFaster, Vec2f{ xTargetDir, 0.0f }, g_pirateFireThree);
			}
			else {
				m_sprite->PlayAnim("Pirate/Run", true);
				co_await TryWalkDistance(22.0f * xTargetDir);
				m_sprite->PlayAnim("Pirate/Idle", true);
			}
			co_await WaitSeconds(0.25f);
			++randomStep;
		}
		++randomStep;
		co_await Suspend();
	}
}
void Pirate::ExplodeAndDie() {
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
