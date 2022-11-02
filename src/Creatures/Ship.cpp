#include "Ship.h"

#include "GameWorld.h"
#include "AudioManager.h"
#include "Projectiles/HomingMissile.h"
#include "Projectiles/HomingMissileDefs.h"
#include "ParticleSpawnerDefs.h"
#include "CameraManager.h"
#include "PlayerStatus.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ColliderComponent.h"
#include "Engine/EaseTo.h"
#include "FunctionGuard.h"

std::shared_ptr<SpawnerDef<Ship>> Ship::s_spawnerDef = std::make_shared<SpawnerDef<Ship>>(
	1,		  	// maxAlive: maximum that can be alive at the same time
	5.0f,	  	// spawnCooldown: -1 means "only spawn based on proximity (ignore time)"
	true,	  	// spawnOnEnter: true means "spawn as soon as player starts to enter enclosing room"
	-1.0f,	  	// triggerDistX: if m_spawnOnEnter is false,  spawn when player is WITHIN this X distance
	-1.0f,	  	// minRespawnDistX: once killed, only respawn when player is OUTSIDE this X distance
	false,		// spawnFacingPlayer: should they?
	std::array<float, 6>{ 54.8f, 8.0f, 1.0f, 2.0f, 2.0f, 34.0f }, // dropOdds: Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
	eTriggerTarget::None	// spawnOnTrigger: if !None, enemy will spawn when player touches a Trigger object w/ same name as this spawner's SpawnType
);

//-- SHIP CREATURE CODE --//

Ship::Ship(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name)
	: Creature(in_instigator)
{
	m_initialCollisionDims = { 126.0f, 48.0f };
	m_worldCollisionBoxLocalFrozen1 = Box2f::FromTopCenter(Vec2f{ 0.0f, -3.0f }, { 96.0f, 48.0f });
	m_worldCollisionBoxLocalFrozen2 = Box2f::FromTopCenter(Vec2f{ 0.0f, -16.0f }, { 128.0f, 16.0f });
	SetHealth(10000); // "effectively infinite" is the goal here
	SetArmor(15);
	m_moveSpeed = 1.0f;
	m_bUpdatesOffScreen = true;
}
void Ship::Initialize() {
	Creature::Initialize();
	m_bUpdatesOffScreen = true;
	SetDamageInfo(0);
	// Touch sensors setup
	ClearDefaultSensor();
	AddCollisionBox({ 0.0, -24.0f }, { 128.0f, 48.0f }, "weakSpot");
	m_hitSensors.find("weakSpot")->second->SetFiltering(CL_None, CL_PlayerBullet | CL_PlayerBomb | CL_Player);

	//TODO: Use these boxes when fight #2 is being implemented:
	//AddCollisionBox({ 0.0, -24.0f }, { 96.0f, 48.0f }, "hurtBox");
	//m_hitSensors.find("hurtBox")->second->SetFiltering(CL_Enemy, CL_Player);
	//AddCollisionBox({ 14.0, 0.0f }, { 10.0f, 28.0f }, "shield");
	//m_hitSensors.find("shield")->second->SetFiltering(CL_Armor, CL_PlayerBullet | CL_PlayerBomb | CL_Player);

	// Sprite setup
	SetDrawLayer(10);
	m_sprite->PlayAnim("Ship/Idle", true);

	m_doorSprite = MakeSprite(Transform::Identity);
	m_doorSprite->SetPlayRate(1.0f);
	m_doorSprite->PlayAnim("ShipDoor/Closed", true);

	m_blightSprite = MakeSprite(Transform::Identity);
	m_blightSprite->PlayAnim("Util/Blank", true);
	
	m_jetSprite1 = MakeSprite(Transform::Identity);
	m_jetSprite1->SetRenderLayer("hud");
	m_jetSprite1->PlayAnim("JetFire/Idle", true);
	m_jetSprite1->SetPlayRate(0.5f);
	m_jetSprite1->SetWorldPos(GetWorldPos() + Vec2f{ -35.0f, -51.0f });
	m_jetSprite1->SetComponentDrawOrder(1010);
	
	m_jetSprite2 = MakeSprite(Transform::Identity);
	m_jetSprite2->SetRenderLayer("hud");
	m_jetSprite2->PlayAnim("JetFire/Idle", true);
	m_jetSprite2->SetPlayRate(0.5f);
	m_jetSprite2->SetWorldPos(GetWorldPos() + Vec2f{ 35.0f, -51.0f });
	m_jetSprite2->SetComponentDrawOrder(1010);

	printf("Ship spawned!\n");
}
Task<> Ship::ManageAI() {
	std::shared_ptr<ColliderComponent_Box> collider;
	std::shared_ptr<ColliderComponent_Box> collider2;
	auto bJustSolidified = true;
	auto bEvil = false;
	auto bFighting = false;
	auto bPatrolling = false;
	auto bFightingTransition = false;
	auto spawnLocation = GetWorldPos();
	auto hideLocation = spawnLocation + Vec2f{ 0.0f, 560.0f };
	auto fightLocation = spawnLocation + Vec2f{ 0.0f, -160.0f };
	auto world = GetWorld();
	auto audioMgr = AudioManager::Get();

	// Wait until cinema 2 is over, then turn evil
	co_await WaitUntil([this, bEvil]() {
		return !bEvil && GetWorld()->GetPlayerStatus()->IsCinematicComplete(2);
	});
	AddCollisionBox({ 0.0, -24.0f }, { 96.0f, 48.0f }, "hurtBox");
	m_hitSensors.find("hurtBox")->second->SetFiltering(CL_Enemy, CL_Player);
	m_blightSprite->PlayAnim("Ship/IdleBlight", true);
	m_blightSprite->SetComponentDrawOrder(1020);
	SetDrawLayer(-2);
	SetWorldPos(hideLocation);
	bEvil = true;

	// Wait for player to trigger
	co_await WaitUntil([this, fightLocation](){
		return (GetWorld()->GetPlayerWorldPos() - fightLocation).Len() < m_triggerDist;
	});

	// Fly down dramatically to the stage of the bossfight
	co_await audioMgr->FadeMusic(2.5f, 0.0f);
	audioMgr->PlayMusic("Countdown", 0.0f, 0.00f);
	co_await audioMgr->FadeMusic(2.5f, 0.4f);
	SetSolid(false);
	auto shipVel = Vec2f::Down * 10.0f;
	auto shipPos = GetWorldPos();
	while(shipPos != fightLocation) {
		auto newPos = EaseTo_Spring(shipPos, fightLocation, DT(), shipVel, 2.0f);
		Move(newPos);
		shipPos = GetWorldPos();
		if((shipPos - fightLocation).Len() < 1.0f) {
			SetWorldPos(fightLocation);
			break;
		}
		co_await Suspend();
	}
	bEvil = false;
	bFightingTransition = true;

	// Prep and start tasks for fight
	GetWorld()->GetCameraManager()->SetDistress(3.0f);
	co_await WaitSeconds(2.0f);
	auto bulletSpawnOffset = Vec2f{ 0.0f, 0.0f };
	auto fireTask = TryFireBullet(g_bossHomingMissileDef, Vec2f::Up, g_bossHomingFire).CancelIf([world] {
		return world->IsPlayerFish();
		});
	m_taskMgr.RunManaged(std::move(fireTask));
	m_hitSensors.find("weakSpot")->second->SetFiltering(CL_Enemy, CL_PlayerBullet | CL_PlayerBomb | CL_Player);
	bFightingTransition = false;
	bFighting = true;

	// Inner bossfight loop
	while(bFighting) {
		auto playerDir = DistanceToPlayer().Norm();
		Move(GetWorldPos() + Vec2f{ playerDir.x * 10.0f, 0.0f });
		if(GetWorld()->IsPlayerFish()) {
			bFighting = false;
			bPatrolling = true;
			break;
		}
		// TODO: Add the following phases:
		// try to line up vertically with player, then smash down (must run left/right)
			// heat-up: shockwave comes out laterally, must jump over
		// slide along ground (must jump over or get to ledge)
			// heat-up: faster slide speed
		// fire targeted "shotgun" blast of many bullets (must take cover behind terrain)
			// heat-up: zip quickly to other angle, fire again (must adjust cover position)
		
		co_await Suspend();
	}

	// Fly away once player's suit is destroyed
	co_await WaitSeconds(1.0f);
	shipVel = Vec2f::Zero;
	shipPos = GetWorldPos();
	auto retreatLocation = Vec2f{ 5216.0f, GetWorldPos().y + 560.0f };
	while(shipPos != retreatLocation) {
		auto newPos = EaseTo_Spring(shipPos, retreatLocation, DT(), shipVel, 10.5f);
		Move(newPos, true);
		shipPos = GetWorldPos();
		if((shipPos - retreatLocation).Len() < 1.0f) {
			SetWorldPos(retreatLocation);
			break;
		}
		co_await Suspend();
	}
}
void Ship::SetSolid(bool in_state) {
	m_bIsSolid = in_state;
	if(m_bIsSolid) {
		m_collider = MakeCollider_Box(	AsShared(), Transform::Identity, 
										GetWorld()->GetCollisionWorld(), m_worldCollisionBoxLocalFrozen1);
		m_collider2 = MakeCollider_Box(	AsShared(), Transform::Identity,
										GetWorld()->GetCollisionWorld(), m_worldCollisionBoxLocalFrozen2);
	}
	else {
		m_collider = {};
		m_collider2 = {};
	}
}
Task<> Ship::OpenDoor() {
	co_await m_doorSprite->PlayAnim("ShipDoor/Opening", false);
	m_doorSprite->PlayAnim("ShipDoor/Open", true);
}
Task<> Ship::CloseDoor() {
	co_await m_doorSprite->PlayAnim("ShipDoor/Closing", false);
	m_doorSprite->PlayAnim("ShipDoor/Closed", true);
}
void Ship::ExplodeAndDie() {
	if(ShouldChunksplode()) {
		auto icePaletteCheck = CheckFrozen() ? "Ice" : "Base";
		Actor::Spawn<ParticleSpawner>(	GetWorld(), { GetWorldPos() + Vec2f{ -4.0f, 4.0f } }, 
										g_dropperDeathExplosionDef0, Vec2f{ -1.72f, 3.0f }, icePaletteCheck);
		Actor::Spawn<ParticleSpawner>(	GetWorld(), { GetWorldPos() + Vec2f{ 4.0f, 4.0f } },  
										g_dropperDeathExplosionDef1, Vec2f{ 1.72f, 3.0f }, icePaletteCheck);
		Actor::Spawn<ParticleSpawner>(	GetWorld(), { GetWorldPos() + Vec2f{ -4.0f, -4.0f } },
										g_dropperDeathExplosionDef2, Vec2f{ -1.72f, 1.75f }, icePaletteCheck);
		Actor::Spawn<ParticleSpawner>(	GetWorld(), { GetWorldPos() + Vec2f{ 4.0f, -4.0f } }, 
										g_dropperDeathExplosionDef3, Vec2f{ 1.72f, 1.75f }, icePaletteCheck);
	}
	Creature::ExplodeAndDie();
}
