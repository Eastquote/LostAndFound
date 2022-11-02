#include "Creatures/Dropper.h"

#include "GameWorld.h"
#include "ParticleSpawnerDefs.h"
#include "Projectile.h"
#include "Projectiles/Grenade.h"
#include "Projectiles/GrenadeDefs.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ColliderComponent.h"

std::shared_ptr<SpawnerDef<Dropper>> Dropper::s_spawnerDef = std::make_shared<SpawnerDef<Dropper>>(
	1,			// maxAlive: maximum that can be alive at the same time
	-1.0f,		// spawnCooldown: -1 means "only spawn based on proximity (ignore time)"
	true,		// spawnOnEnter: true means "spawn as soon as player starts to enter enclosing room"
	-1.0f,		// triggerDistX: if m_spawnOnEnter is false,  spawn when player is WITHIN this X distance
	255.0f,		// minRespawnDistX: once killed, only respawn when player is OUTSIDE this X distance
	false,		// spawnFacingPlayer: should they?
	std::array<float, 6>{ 54.8f, 8.0f, 1.2f, 34.0f, 2.0f, 2.0f }, // Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
	eTriggerTarget::None	// spawnOnTrigger: true means enemy will spawn when player touches a Trigger object with same name as this spawner's SpawnType
);

//-- DROPPER CREATURE CODE --//

Dropper::Dropper(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name)
	: Creature(in_instigator)
{
	m_initialCollisionDims = { 8.0f, 24.0f };
	SetHealth(2);
}
void Dropper::Initialize() {
	Creature::Initialize();
	SetDamageInfo(8);
	m_sprite->PlayAnim("Dropper/Idle", true);
	m_spriteLit->PlayAnim("Dropper/IdleLit", true);
}
Task<> Dropper::ManageAI() {
	auto dropGravity = .112f * 60.0f;
	auto maxDropSpeed = 3.0f * 60.0f;
	auto dropSpeed = 0.0f;
	auto xSpeed = 1.0f * 60.0f;
	CardinalUpdate(m_blockCardinals, 255.0f);
	auto floorDist = m_blockCardinals.DownDist();

	// Suspend until the player is within chase range
	co_await WaitUntil([this, floorDist] {
		auto playerDist = DistanceFromPlayer();
		//std::cout << "fabs(playerDist.x) = " << fabs(playerDist.x) << " | " << (fabs(playerDist.x) < m_triggerDist) <<
		//	(playerDist.y > 0.0f) <<
		//	(playerDist.y < floorDist) << std::endl;
		return fabs(playerDist.x) < m_triggerDist &&
			playerDist.y > 0.0f &&
			playerDist.y < floorDist;
		});
	CardinalUpdate(m_blockCardinals);

	// Fall and veer towards player until touching ground
	auto dir = Vec2f{ 0.0f, -1.0f };
	auto rotateVel = 3.0f * 60.0f;
	while(!m_blockCardinals.Down()) {
		auto rotation = 0.0f;
		auto targetVec = (GetWorld()->GetPlayerWorldPos() - GetWorldPos());
		auto targetDir = targetVec.Norm();
		dropSpeed += dropGravity * m_moveSpeedModifier;
		if(DistanceFromPlayer().x < 0.0f) {
			Move(GetWorldPos() + Vec2f{ xSpeed * DT() * m_moveSpeedModifier, 0.0f });
			rotation += rotateVel * DT();
		}
		else if(DistanceFromPlayer().x >= 0.0f) {
			Move(GetWorldPos() + Vec2f{ -xSpeed * DT() * m_moveSpeedModifier, 0.0f });
			rotation -= rotateVel * DT();
		}
		if(dropSpeed < maxDropSpeed * m_moveSpeedModifier) {
			Move(GetWorldPos() + Vec2f{ 0.0f, -dropSpeed * DT() });
		}
		else {
			Move(GetWorldPos() + Vec2f{ 0.0f, -maxDropSpeed * DT() * m_moveSpeedModifier });
		}
		m_sprite->SetWorldRot(GetWorldRot() + rotation);
		co_await Suspend();
	}
	m_sprite->SetWorldRot(0.0f);

	// Drill into ground while getting ready to explode
	auto timer = 1.0667f * float(1.0f / m_moveSpeedModifier);
	auto i = 0;
	auto fired1 = false;
	auto fired2 = false;
	while(timer > 0.0f) {
		timer -= DT();
		std::array<Vec2f, 4> wiggleDirs = { Vec2f{2.0f, 0.0f} * 60.0f, Vec2f{0.0f, -0.8f} * 60.0f, Vec2f{-2.0f, 0.0f} * 60.0f, Vec2f{0.0f, -0.8f} * 60.0f };
		Move(GetWorldPos() + wiggleDirs[i] * DT(), true);
		++i;
		if(i > wiggleDirs.size() - 1) {
			i = 0;
		}

		// Fire debris bullets!
		if(timer < 0.85f && !fired1) {
			auto dir = Vec2f{ 1.0f, 0.0f }.RotateDeg(65.0f);
			for(auto i = 0; i < 2; ++i) {
				dir = dir.RotateDeg(15.0f);
				Transform bulletTransform = { GetWorldPos() + Vec2f{0.0, -1.0f}, 0.0f, Vec2f::One };
				auto grenade = SpawnProjectile(g_enemyGrenadeDef, bulletTransform, dir, (int)dir.x, 0.8f);
			}
			fired1 = true;
		}
		if(timer < 0.35f && !fired2) {
			auto dir = Vec2f{ 1.0f, 0.0f }.RotateDeg(50.0f);
			for(auto i = 0; i < 2; ++i) {
				dir = dir.RotateDeg(30.0f);
				Transform bulletTransform = { GetWorldPos() + Vec2f{0.0, 10.0f}, 0.0f, Vec2f::One };
				auto grenade = SpawnProjectile(g_enemyGrenadeDef, bulletTransform, dir, (int)dir.x, 0.8f);
			}
			fired2 = true;
			m_bCanSpawnPickup = false;
		}
		co_await Suspend();
	}

	// Wait for any inbound missiles to hit
	co_await WaitUntil([this] {
		return !GetTargeted();
		});

	ExplodeAndDie();
}
void Dropper::ExplodeAndDie() {
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
