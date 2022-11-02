#include "Creatures/Monopod.h"

#include "GameWorld.h"
#include "Projectiles/Grenade.h"
#include "Projectiles/GrenadeDefs.h"
#include "Effect.h"
#include "ParticleSpawnerDefs.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ColliderComponent.h"

std::shared_ptr<SpawnerDef<Monopod>> Monopod::s_spawnerDef = std::make_shared<SpawnerDef<Monopod>>(
	1,		  	// maxAlive: maximum that can be alive at the same time
	-1.0f,	  	// spawnCooldown: -1 means "only spawn based on proximity (ignore time)"
	true,	  	// spawnOnEnter: true means "spawn as soon as player starts to enter enclosing room"
	-1.0f,	  	// triggerDistX: if m_spawnOnEnter is false,  spawn when player is WITHIN this X distance
	255.0f,	  	// minRespawnDistX: once killed, only respawn when player is OUTSIDE this X distance
	false,		// spawnFacingPlayer: should they?
	std::array<float, 6>{ 54.8f, 1.0f, 8.2f, 2.0f, 2.0f, 34.0f }, // dropOdds: Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
	eTriggerTarget::None	// spawnOnTrigger: true means enemy will spawn when player touches a Trigger object with same name as this spawner's SpawnType
);

//-- MONOPOD CREATURE CODE --//

Monopod::Monopod(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name)
	: Creature(in_instigator, in_direction)
{
	m_initialCollisionDims = { 16.0f, 16.0f };
	SetHealth(5);
	SetArmor(10);
	m_moveSpeed = 0.5f * 60.0f;
}
void Monopod::Initialize() {
	Creature::Initialize();
	SetDamageInfo(6);
	m_bUpdatesOffScreen = true;
	m_worldCollisionBoxLocalLegs = Box2f::FromCenter(Vec2f{ 2.0f, -8.0f }, { 4.0f, 14.0f });

	// Touch sensors setup
	ClearDefaultSensor();
	AddCollisionBox({ -4.0, 8.0f }, { 8.0f, 12.0f }, "headLeft");
	AddCollisionBox({ 4.0, 8.0f }, { 8.0f, 12.0f }, "headRight");
	for(auto sensor : m_hitSensors) { //< Set both "heads" to the same filtering
		sensor.second->SetFiltering(CL_Armor, CL_PlayerBullet | CL_PlayerBomb | CL_Player | CL_EnemyBullet);
	}
	AddCollisionBox({ 0, 8.0f }, { 8.0f, 16.0f }, "headCenter");
	m_hitSensors.find("headCenter")->second->SetFiltering(CL_World, CL_PlayerBullet | CL_PlayerBomb | CL_Player);
	AddCollisionBox({ 0, -8.0f }, { 6.0f, 14.0f }, "legs");
	m_hitSensors.find("legs")->second->SetFiltering(CL_World, CL_PlayerBullet | CL_PlayerBomb | CL_Player);

	// Sprite setup
	m_sprite->PlayAnim("Monopod/BodyIdleStanding", true);
	m_spriteLegs = MakeSprite(Transform::Identity);
	m_spriteLegs->PlayAnim("Util/Blank", true);
}
void Monopod::ChangeHealth(int32_t in_change, bool hitArmor) {
	auto health = GetHealth();
	if(hitArmor) {
		auto armor = GetArmor();
		SetArmor(armor + in_change >= 0 ? armor += in_change : 0);
	}
	else if(health != -1) {
		SetHealth(health + in_change >= 0 ? health += in_change : 0);
	}
	if(health <= 0) {
		SetHealth(1);
		m_bJustDestroyedLeg = true;
	}
}
Task<> Monopod::ManageAI() {
	auto dropGravity = .212f * 60.0f;
	auto maxDropSpeed = 3.0f * 60.0f;
	auto dropSpeed = 0.0f;
	auto headBox = GetCollisionBoxWorld();
	auto legBox = m_worldCollisionBoxLocalLegs.TransformedBy(GetWorldTransform());
	auto hasLeg = false;
	auto bulletPeriod = 0.7f;
	auto bulletTimer = bulletPeriod;
	auto world = GetWorld();
	auto bulletDir = 1.0f;
	auto triggered = false;
	auto playerDist = GetWorld()->GetPlayerWorldPos() - GetWorldPos();

	auto wakeupTrigger = Box2f::FromCenter(Vec2f{ 0.0f, 24.0f }, { 128.0f, 48.0f });

	SpawnSurfaceCheck(true, false, false, true);
	Move(GetWorldPos() + Vec2f{ 0.0f, -m_worldCollisionBoxLocalLegs.GetDims().y }, true);
	Cardinals ledgeCheckCardinals;
	Cardinals legCardinals;
	// Starts on ground, awakened by proximity on ground nearby
	co_await WaitUntil([this, wakeupTrigger] { return CheckTrigger(wakeupTrigger); });

	auto startY = GetWorldPos().y;
	auto dir = Vec2f{ 0.0f, 1.0f };
	auto speed = 3.0f * 60.0f;
	auto cardinalSweep = 5.0f * 60.0f;

	// Hop upwards...
	while(true) {
		legBox = m_worldCollisionBoxLocalLegs.TransformedBy(GetWorldTransform());
		CardinalUpdate(legCardinals, cardinalSweep * DT(), Vec2f::Zero, legBox);
		auto vel = dir * speed;
		Move(GetWorldPos() + vel * DT());
		if(GetWorldPos().y - startY > 31.0f) {
			// Pop leg out once at desired height,
			m_spriteLegs->PlayAnim("Monopod/LegRunning", true);
			m_hitSensors.find("legs")->second->SetFiltering(CL_Enemy, CL_PlayerBullet | CL_PlayerBomb | CL_Player);
			break;
		}
		co_await Suspend();
	}
	CardinalUpdate(m_blockCardinals, 0.01f, Vec2f::Zero, m_worldCollisionBoxLocalLegs);

	// ...then land
	while(true) {
		auto vel = dir * speed;
		Move(GetWorldPos() + vel * DT(), false, 0.01f, nullptr, false, m_worldCollisionBoxLocalLegs);
		if(m_blockCardinals.Down()) {
			break;
		}
		speed -= dropGravity;
		co_await Suspend();
	}
	auto walkDir = Vec2f{ GetWorld()->GetPlayerWorldPos() - GetWorldPos() }.x >= 0.0f ? 1.0f : -1.0f;
	while(true) {
		// Walk around, lobbing grenades in either direction from time to time
		bulletTimer -= DT();
		legBox = m_worldCollisionBoxLocalLegs;

		// Turn around at ledges
		CardinalUpdate(ledgeCheckCardinals, 0.1f, { legBox.w * walkDir, 0.0f }, legBox);
		if(m_blockCardinals.Right() || m_blockCardinals.Left() || !ledgeCheckCardinals.Down()) {
			walkDir *= -1;
		}
		else {
			CardinalUpdate(ledgeCheckCardinals, 0.1f, Vec2f::Zero, legBox);
			if(ledgeCheckCardinals.Right() || ledgeCheckCardinals.Left()) {
				walkDir *= -1;
			}
		}
		Move(GetWorldPos() + Vec2f{ (m_moveSpeed * DT() * walkDir), 0.0f });

		// Grenade-lobbing behavior
		if(bulletTimer <= 0.0f) {
			std::vector<Vec2f> bulletDirs = { Vec2f{bulletDir, 1.0f}.Norm() };
			Transform bulletTransform = { GetWorldPos() + Vec2f{0.0, 14.0f}, 0.0f, Vec2f::One };
			for(auto dir : bulletDirs) {
				auto grenade = SpawnProjectile(g_enemyGrenadeDef, bulletTransform, dir, (int)dir.x, 1.0f);
			}
			bulletTimer = bulletPeriod;
			bulletDir *= -1;
		}

		// Check leg-destruction thresholds
		if(GetHealth() < 3 || GetArmor() == 0) {
			m_bJustDestroyedLeg = true;
		}

		// Destroy leg
		if(m_bJustDestroyedLeg) {
			SetHealth(3);
			m_spriteLegs->PlayAnim("Util/Blank", true);
			auto effect = Actor::Spawn<Effect>(GetWorld(), { GetWorldPos() + Vec2f{0.0f, -8.0f} }, "Explosion2/Explosion");
			m_hitSensors.find("legs")->second->SetFiltering(CL_World, CL_None);
			hasLeg = false;
			break;
		}
		co_await Suspend();
	}

	// Fall to ground after leg is destroyed
	while(!m_blockCardinals.Down()) {
		dropSpeed += dropGravity * m_moveSpeedModifier;
		if(dropSpeed < maxDropSpeed * m_moveSpeedModifier) {
			Move(GetWorldPos() + Vec2f{ m_moveSpeed * DT() * walkDir, -dropSpeed * DT() });
		}
		else {
			Move(GetWorldPos() + Vec2f{ m_moveSpeed * DT() * walkDir, -maxDropSpeed * DT() * m_moveSpeedModifier });
		}
		co_await Suspend();
	}
	m_sprite->PlayAnim("MonoPod/BodyIdleOpen", true);

	// Fire upward arc of deathbullets
	dir = Vec2f{ 1.0f, 0.0f }.RotateDeg(50.0f);
	for(auto i = 0; i < 4; ++i) {
		dir = dir.RotateDeg(15.0f);
		Transform bulletTransform = { GetWorldPos() + Vec2f{0.0, 6.0f}, 0.0f, Vec2f::One };
		auto grenade = SpawnProjectile(g_enemyGrenadeDef, bulletTransform, dir, (int)dir.x, 1.0f);
	}
	ExplodeAndDie();
}
void Monopod::ExplodeAndDie() {
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