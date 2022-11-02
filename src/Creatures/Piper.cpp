#include "Creatures/Piper.h"

#include "GameWorld.h"
#include "ParticleSpawnerDefs.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ColliderComponent.h"
#include <iostream>

std::shared_ptr<SpawnerDef<Piper>> Piper::s_spawnerDef = std::make_shared<SpawnerDef<Piper>>(
	1,			// maxAlive: maximum that can be alive at the same time
	0.0f,		// spawnCooldown: -1 means "only spawn based on proximity (ignore time)"
	false,		// spawnOnEnter: true means "spawn as soon as player starts to enter enclosing room"
	80.0f,		// triggerDistX: if m_spawnOnEnter is false,  spawn when player is WITHIN this X distance
	0.0f,		// minRespawnDistX: once killed, only respawn when player is OUTSIDE this X distance
	true,		// spawnFacingPlayer: should they?
	std::array<float, 6>{ 2.0f, 24.0f, 24.0f, 24.0f, 24.0f, 4.0f }, // Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
	eTriggerTarget::None	// spawnOnTrigger: true means enemy will spawn when player touches a Trigger object with same name as this spawner's SpawnType
);

//-- PIPER CREATURE CODE --//

Piper::Piper(std::shared_ptr<CreatureSpawner> in_instigator, bool in_isMovingRight, float in_rotation, std::string in_name)
	: Creature(in_instigator)
	, m_bIsMovingRight(!in_isMovingRight)
{
	m_initialCollisionDims = { 8.0f , 12.0f };
	SetHealth(1);
}
void Piper::Initialize() {
	Creature::Initialize();
	SetDamageInfo(8);

	// Player-centric orientation setup
	auto playerPos = GetWorld()->GetPlayerWorldPos();
	m_bIsMovingRight = playerPos.x >= GetWorldPos().x;
	auto dir = m_bIsMovingRight ? 1.0f : -1.0f;
	m_moveSpeed = 1.5f * 60.0f * dir;

	// Sprite setup
	m_sprite->PlayAnim("Piper/Normal", true);
	m_sprite->SetFlipHori(!m_bIsMovingRight);
}
void Piper::Update() {
	if(GetWorld()->CreatureIsOffCamera(AsShared<Creature>())) {
		DeferredDestroy();
	}
	else {
		Creature::Update();
	}
}
Task<> Piper::ManageAI() {
	auto moveVec = Vec2f::Zero;
	auto dist = Vec2f::Zero;
	auto pos = Vec2f::Zero;
	auto startPos = GetWorldPos();
	auto isAbovePlayer = false;
	while(true) {
		auto speed = m_moveSpeed * m_moveSpeedModifier;
		// Fly upwards until level with player
		auto speedDT = (abs(speed) * DT() * 1.25f);
		std::cout << "speedDT = " << speedDT << std::endl;
		while(GetWorldPos().y < (startPos.y + 16.0f)) {
			SetWorldPos({ GetWorldPos().x, GetWorldPos().y + (abs(speed) * DT() * 1.25f) });
			co_await Suspend();
		}
		auto playerPos = GetWorld()->GetPlayerWorldPos();
		if(playerPos.y - GetWorldPos().y <= -5.0f) {
			isAbovePlayer = true;
		}
		if(!isAbovePlayer) {
			SetWorldPos({ GetWorldPos().x, GetWorldPos().y + (abs(speed) * DT() * 1.25f) });
		}
		// Rush horizontally towards player
		if(isAbovePlayer) {
			co_await WaitSeconds(0.1f * (1.0f / m_moveSpeedModifier));
			while(true) {
				playerPos = GetWorld()->GetPlayerWorldPos();
				SetWorldPos({ GetWorldPos().x + speed * DT(), GetWorldPos().y });
				co_await Suspend();
			}
		}
		co_await Suspend();
	}
}
void Piper::ExplodeAndDie() {
	if(ShouldChunksplode()) {
		auto icePaletteCheck = CheckFrozen() ? "Ice" : "Base";
		Actor::Spawn<ParticleSpawner>(GetWorld(), { GetWorldPos() + Vec2f{ -4.0f, 4.0f } },
			g_piperDeathExplosionDef0, Vec2f{ -1.72f, 3.0f }, icePaletteCheck);
		Actor::Spawn<ParticleSpawner>(GetWorld(), { GetWorldPos() + Vec2f{ 4.0f, 4.0f } },
			g_piperDeathExplosionDef1, Vec2f{ 1.72f, 3.0f }, icePaletteCheck);
		Actor::Spawn<ParticleSpawner>(GetWorld(), { GetWorldPos() + Vec2f{ -4.0f, -4.0f } },
			g_piperDeathExplosionDef2, Vec2f{ -1.72f, 1.75f }, icePaletteCheck);
		Actor::Spawn<ParticleSpawner>(GetWorld(), { GetWorldPos() + Vec2f{ 4.0f, -4.0f } },
			g_piperDeathExplosionDef3, Vec2f{ 1.72f, 1.75f }, icePaletteCheck);
	}
	Creature::ExplodeAndDie();
}
