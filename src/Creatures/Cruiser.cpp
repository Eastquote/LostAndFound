#include "Creatures/Cruiser.h"

#include "GameWorld.h"
#include "ParticleSpawnerDefs.h"
#include "Engine/MathRandom.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ColliderComponent.h"

std::shared_ptr<SpawnerDef<Cruiser>> Cruiser::s_spawnerDef = std::make_shared<SpawnerDef<Cruiser>>(
	1,			// maxAlive: maximum that can be alive at the same time
	-1.0f,		// spawnCooldown: -1 means "only spawn based on proximity (ignore time)"
	true,		// spawnOnEnter: true means "spawn as soon as player starts to enter enclosing room"
	-1.0f,		// triggerDistX: if m_spawnOnEnter is false,  spawn when player is WITHIN this X distance
	255.0f,		// minRespawnDistX: once killed, only respawn when player is OUTSIDE this X distance
	false,		// spawnFacingPlayer: should they?
	std::array<float, 6>{ 2.0f, 24.0f, 24.0f, 24.0f, 24.0f, 4.0f }, // Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
	eTriggerTarget::None	// spawnOnTrigger: true means enemy will spawn when player touches a Trigger object with same name as this spawner's SpawnType
);

//-- CRUISER CREATURE CODE --//

Cruiser::Cruiser(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name)
	: Creature(in_instigator)
{
	m_initialCollisionDims = { 16.0f, 8.0f };
	m_bIsFacingRight = !in_direction;
	m_bIsMovingRight = !in_direction;
	SetHealth(5);
	m_moveSpeed = 1.0f * 60.0f;
}
void Cruiser::Initialize() {
	Creature::Initialize();
	SetDamageInfo(8);
	m_sprite->PlayAnim("Cruiser/Closed", true);
}
void Cruiser::ExplodeAndDie() {
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
Task<> Cruiser::ManageAI() {
	int32_t period = 82; //< In frames
	auto timer = 0.0f;
	auto yImpulse = 0.0f;
	auto gravity = 0.1f * 60.0f;
	auto startY = GetWorldPos().y;
	auto yDir = Math::RandomInt(0, 1) ? 1.0f : -1.0f;
	auto yScale = 1.0f;
	auto moveDir = 1.0f;
	auto animPlayed = false;
	while(true) {
		auto adjustedPeriod = period * (1.0f / m_moveSpeedModifier);

		// Calculate y-scale at top of arc
		if(DistanceFromPlayer().y * -yDir >= 0.0f) {
			yScale = Math::RandomFloat(0.8f, 1.0f);
		}
		else {
			yScale = Math::RandomFloat(0.2f, 0.35f);
		}

		yImpulse = 0.0f;
		timer = 0.0f;
		animPlayed = false;

		// Loop until DOWNSWING is over
		while(timer <= adjustedPeriod / 60.0f) {
			moveDir = 1.0f;
			if(!m_bIsFacingRight) {
				moveDir = -1.0f;
			}
			auto speed = std::abs(m_moveSpeed * m_moveSpeedModifier) * moveDir;
			m_sprite->SetFlipHori(!m_bIsFacingRight);
			if(m_bJustFlipped) {
				Move(GetWorldPos() + Vec2f{ speed * DT(), 0.0f }, false, 0.5f, AsShared<Creature>());
				Move(GetWorldPos() + Vec2f{ 0.0f, yImpulse }, false, 0.5f, AsShared<Creature>());
				m_bJustFlipped = false;
			}
			else if(!m_blockCardinals.Right() && !m_blockCardinals.Left()) {
				if(timer <= (adjustedPeriod / 60.0f) / 2.0f) {
					yImpulse += gravity * DT() * yScale * yDir * m_moveSpeedModifier;
				}
				else {
					yImpulse += gravity * DT() * yScale * -yDir * m_moveSpeedModifier;
				}
				Move(GetWorldPos() + Vec2f{ speed * DT(), 0.0f }, false, 0.5f, AsShared<Creature>());
				Move(GetWorldPos() + Vec2f{ 0.0f, yImpulse }, false, 0.5f, AsShared<Creature>());
			}
			else {
				FlipDir();
			}

			// Handle animation state
			if(!animPlayed && timer > (adjustedPeriod / 70.0f) / 2.0f) {
				m_sprite->PlayAnim("Cruiser/Open", false, 0);
				animPlayed = true;
			}
			timer += DT();
			co_await Suspend();
		}
		yDir *= -1;
	}
}
