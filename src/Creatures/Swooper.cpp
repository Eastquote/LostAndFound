#include "Creatures/Swooper.h"

#include "GameWorld.h"
#include "ParticleSpawnerDefs.h"
#include "Engine/Curve.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ColliderComponent.h"

std::shared_ptr<SpawnerDef<Swooper>> Swooper::s_spawnerDef = std::make_shared<SpawnerDef<Swooper>>(
	1,			// maxAlive: maximum that can be alive at the same time
	-1.0f,		// spawnCooldown: -1 means "only spawn based on proximity (ignore time)"
	true,		// spawnOnEnter: true means "spawn as soon as player starts to enter enclosing room"
	-1.0f,		// triggerDistX: if m_spawnOnEnter is false,  spawn when player is WITHIN this X distance
	255.0f,		// minRespawnDistX: once killed, only respawn when player is OUTSIDE this X distance
	false,		// spawnFacingPlayer: should they?
	std::array<float, 6>{ 16.0f, 12.0f, 32.0f, 34.0f, 4.0f, 4.0f }, // Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
	eTriggerTarget::None	// spawnOnTrigger: true means enemy will spawn when player touches a Trigger object with same name as this spawner's SpawnType
);

//-- SWOOPER CREATURE CODE --//

Swooper::Swooper(std::shared_ptr<CreatureSpawner> in_instigator, bool in_isMovingRight, float in_rotation, std::string in_name)
	: Creature(in_instigator)
{
	m_initialCollisionDims = { 24.0f, 16.0f };
	SetHealth(4);
	m_moveSpeed = 1.0f * 60.0f;
}
void Swooper::Initialize() {
	Creature::Initialize();
	SetDamageInfo(8);
	m_sprite->PlayAnim("Swooper/Normal", true);
}
Task<> Swooper::ManageAI() {
	auto dir = 1;
	auto flightCurve = Curve("data/curves/test.rtcurve");
	while(true) {
		// Suspend until the player is within chase range
		co_await WaitUntil([this] {
			return fabs(DistanceFromPlayer().x) < m_triggerDist;
			});
		if(DistanceFromPlayer().x < 0.0f) {
			m_bIsMovingRight = true;
			dir = 1;
		}
		else {
			m_bIsMovingRight = false;
			dir = -1;
		}
		auto bBelowPlayer = false;
		auto bNeverFloored = true;
		auto elapsedTime = 0.0f;
		auto startPos = GetWorldPos();
		auto playerStartPos = GetWorld()->GetPlayerWorldPos();
		auto playerPos = playerStartPos;
		m_bJustFlipped = false;
		if(startPos.y < playerPos.y) {
			bBelowPlayer = true;
		}
		// Swoop parabolically down towards player's knees until they jump
		while(playerPos.y - 15.0f <= GetWorldPos().y || bBelowPlayer) {
			auto moveDir = 1.0f;
			if(!m_bIsMovingRight) {
				moveDir = -1.0f;
			}
			auto speed = std::abs(m_moveSpeed * m_moveSpeedModifier) * moveDir;
			if(m_blockCardinals.Down()) {
				bNeverFloored = false;
			}
			playerPos = GetWorld()->GetPlayerWorldPos();
			auto newHeight = flightCurve.Eval(elapsedTime) * m_swoopHeight;

			if(elapsedTime <= flightCurve.GetTimeRange().m_max * (1.0f / m_moveSpeedModifier)) {
				// Move is broken into separate X- and Y- moves to enable "sliding" on blocking surfaces
				Move({ (GetWorldPos().x + speed * DT()), GetWorldPos().y }, false, abs(speed * DT()), AsShared<Creature>());
				if(bNeverFloored) {
					Move({ GetWorldPos().x, (startPos.y - m_swoopHeight) + newHeight });
				}
			}
			else if(elapsedTime > flightCurve.GetTimeRange().m_max * (1.0f / m_moveSpeedModifier) && !bBelowPlayer) {
				// Move is broken into separate X- and Y- moves to enable "sliding" on blocking surfaces
				Move({ (GetWorldPos().x + (speed * DT())), GetWorldPos().y }, false, abs(speed * DT()), AsShared<Creature>());
				auto yDelta = flightCurve.GetTangent(flightCurve.GetTimeRange().m_max * (1.0f / m_moveSpeedModifier)).y;
				Move({ GetWorldPos().x, GetWorldPos().y + yDelta }, false, abs(speed * DT()), AsShared<Creature>());
			}
			else {
				break;
			}
			if(m_blockCardinals.Left() || m_blockCardinals.Right() || m_bJustFlipped) { //< If it hit a wall
				break;
			}
			if(!bNeverFloored && !m_blockCardinals.Down()) {
				break;
			}
			co_await Suspend();
			elapsedTime += DT();
		}
		elapsedTime = 0.0f;

		// Begin upswing
		while(!m_blockCardinals.Up()) {
			auto moveDir = 1.0f;
			if(!m_bIsMovingRight) {
				moveDir = -1.0f;
			}
			auto speed = std::abs(m_moveSpeed * m_moveSpeedModifier) * moveDir;
			auto newHeight = flightCurve.Eval(flightCurve.GetTimeRange().m_max * (1.0f / m_moveSpeedModifier) - elapsedTime) * m_swoopHeight;
			if(m_bJustFlipped) {
				// Move is broken into separate X- and Y- moves to enable "sliding" on blocking surfaces
				Move({ (GetWorldPos().x + (speed * DT())), GetWorldPos().y }, false, std::fabs(speed * DT()), AsShared<Creature>());
				Move({ GetWorldPos().x, (GetWorldPos().y + newHeight) }, false, std::fabs(speed * DT()), AsShared<Creature>());
				m_bJustFlipped = false;
			}
			// Move is broken into separate X- and Y- moves to enable "sliding" on blocking surfaces
			Move({ (GetWorldPos().x + (speed * DT())), GetWorldPos().y }, false, std::fabs(speed * DT()), AsShared<Creature>());
			Move({ GetWorldPos().x, (GetWorldPos().y + newHeight) }, false, std::fabs(speed * DT()), AsShared<Creature>());
			if(m_blockCardinals.Left() || m_blockCardinals.Right()) { //< Change x-dir when it hits a wall
				if(!m_bJustFlipped) {
					FlipDir();
				}
			}
			co_await Suspend();
			elapsedTime += DT();
		}
		co_await WaitSeconds(1.0f);
		co_await Suspend();
	}
}
void Swooper::ExplodeAndDie() {
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
