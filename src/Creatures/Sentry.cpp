#include "Creatures/Sentry.h"

#include "GameWorld.h"
#include "ParticleSpawner.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ColliderComponent.h"

std::shared_ptr<SpawnerDef<Sentry>> Sentry::s_spawnerDef = std::make_shared<SpawnerDef<Sentry>>(
	1,			// maxAlive: maximum that can be alive at the same time
	-1.0f,		// spawnCooldown: -1 means "only spawn based on proximity (ignore time)"
	true,		// spawnOnEnter: true means "spawn as soon as player starts to enter enclosing room"
	-1.0f,		// triggerDistX: if m_spawnOnEnter is false,  spawn when player is WITHIN this X distance
	255.0f,		// minRespawnDistX: once killed, only respawn when player is OUTSIDE this X distance
	false,		// spawnFacingPlayer: should they?
	std::array<float, 6>{ 26.0f, 32.0f, 8.0f, 32.0f, 2.0f, 2.0f }, // Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
	eTriggerTarget::None	// spawnOnTrigger: true means enemy will spawn when player touches a Trigger object with same name as this spawner's SpawnType
);

//-- SENTRY CREATURE CODE --//

Sentry::Sentry(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name)
	: Creature(in_instigator)
{
	m_bIsFacingRight = !in_direction;
	m_bIsMovingRight = !in_direction;
	m_initialCollisionDims = { 16.0f, 8.0f };
	SetHealth(-1);
	m_moveSpeed = 0.5 * 60.0f;
}
void Sentry::Initialize() {
	Creature::Initialize();
	SetDamageInfo(8);
	m_sprite->PlayAnim("Sentry/Normal", true);
}
Task<> Sentry::ManageAI() {
	while(true) {
		// If touching wall at start of frame, flip direction
		bool isTouchingWall = m_blockCardinals.Right() || m_blockCardinals.Left();
		if(isTouchingWall) {
			FlipDir();
		}

		// Move one frame's distance in the current direction
		Move(GetWorldPos() + Vec2f{ m_moveSpeed * DT(), 0.0f }, false, std::abs(m_moveSpeed * m_moveSpeedModifier) * DT(), AsShared<Creature>());
		co_await Suspend();
	}
}
void Sentry::FlipDir() {
	Creature::FlipDir();
	m_sprite->SetFlipHori(!m_bIsFacingRight);
	float moveDir = m_bIsFacingRight ? 1.0f : -1.0f;
	m_moveSpeed = std::abs(m_moveSpeed) * moveDir;
}