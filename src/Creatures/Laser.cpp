#include "Creatures/Laser.h"

#include "GameWorld.h"
#include "ParticleSpawner.h"
#include "Light.h"
#include "Projectile.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ColliderComponent.h"
#include "Algorithms.h"
#include <iostream>

std::shared_ptr<SpawnerDef<Laser>> Laser::s_spawnerDef = std::make_shared<SpawnerDef<Laser>>(
	1,			// maxAlive: maximum that can be alive at the same time
	-1.0f,		// spawnCooldown: -1 means "only spawn based on proximity (ignore time)"
	true,		// spawnOnEnter: true means "spawn as soon as player starts to enter enclosing room"
	-1.0f,		// triggerDistX: if m_spawnOnEnter is false,  spawn when player is WITHIN this X distance
	-1.0f,		// minRespawnDistX: once killed, only respawn when player is OUTSIDE this X distance (-1.0f = never respawn)
	false,		// spawnFacingPlayer: should they?
	std::array<float, 6>{ 54.8f, 1.0f, 8.2f, 2.0f, 2.0f, 34.0f }, // Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
	eTriggerTarget::None			// spawnOnTrigger: true means enemy will spawn when player touches a Trigger object with same name as this spawner's SpawnType
);

//-- LASER CREATURE CODE --//

Laser::Laser(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name)
	: Creature(in_instigator)
{
	m_initialCollisionDims = { 16.0f, 14.0f };
	m_bIsFacingRight = !in_direction;
	m_bIsMovingRight = !in_direction;
	m_name = in_name;
	m_rotation = in_rotation;
	m_direction = Math::DegreesToVec(
		(float)Math::VecToDegrees(in_direction ? Vec2f{ 1.0f, 0.0f } : Vec2f{ -1.0f, 0.0f }) + in_rotation
		);
	SetHealth(3);
	m_moveSpeed = 0.5f * 60.0f;
	m_hitPause = 0.0f;
}
void Laser::Initialize() {
	Creature::Initialize();
	SetDamageInfo(5);
	m_hitSensor->SetFiltering(CL_Enemy, CL_PlayerBullet | CL_PlayerBomb | CL_Player | CL_Door | CL_Enemy); //< Enables Laser-Laser touches
	m_bUpdatesOffScreen = true;
	// Check if this laser is a "primary" (i.e. it owns/updates the laser proper)
	auto id = m_name.substr(m_name.find("_") + 1, m_name.size());
	if(id == "00") {
		m_bPrimary = true;
		//printf("Found primary!\n");
	}

	// Sprite setup
	m_sprite->SetWorldRot(m_rotation);
	m_sprite->PlayAnim("Laser/Closed", true);
	m_sprite->SetComponentDrawOrder(1);

	// Light setup
	m_light = Spawn<Light>({}, "LightLarge/Core", "LightLarge/Penumbra");
	m_light->SetScale(0.25f);
	m_light->AttachToActor(AsShared<Actor>(), false);
}
void Laser::Update() {
	EraseInvalid(m_laserTargets);
	Creature::Update();
}
Task<> Laser::ManageAI() {
	auto creatureList = GetWorld()->GetEnclosingRoom(GetWorldPos())->creatures;
	for(auto creature : creatureList) {
		if(creature == AsShared<Creature>()) {
			//printf("	That's a mee!\n");
		}
		else {
			// Look for another Laser creature in the same group
			auto creatureName = creature->GetName();
			auto creatureGroup = creatureName.substr(0, creatureName.find("_"));
			auto myGroup = m_name.substr(0, m_name.find("_"));
			//std::cout << "m_name group = " << myGroup << "   creatureGroup = " << creatureGroup << std::endl;
			if(creatureGroup == myGroup) {
				if(std::dynamic_pointer_cast<Laser>(creature)) {
					std::weak_ptr<Laser> match = std::dynamic_pointer_cast<Laser>(creature);
					m_laserTargets.push_back(match);
				}
			}
		}
	}
	int32_t frameCount = 0;
	auto fireDelay = 10.4f;
	auto fireTimer = 0.0f;
	Transform bulletTransform = { Vec2f::Zero };
	auto world = GetWorld();
	auto targetDir = m_direction;
	auto sniper = true; //< Sets spoilsport bullets to target player (instead of using creature direction)
	auto rotateIncrement = 20.0f;
	int32_t prevTargetCount = 0;
	if(m_bPrimary && m_laserTargets.size()) {
		m_taskMgr.RunManaged(DrawBeam(m_laserTargets[0]));
	}
	else if(!m_bPrimary) {
		SetDrawLayer(1);
	}
	auto activityPeriod = 2.0f;
	auto inactivityPeriod = 2.0f;
	auto laserTimer = 0.0f;

	while(true) {
		laserTimer += DT();
		fireTimer -= DT();
		++frameCount;
		if(m_bPrimary && m_laserTargets.size()) {
			for(auto laser : m_laserTargets) {
				if(auto lock = laser.lock()) {
					auto primaryPos = GetWorldPos();
					auto laserLength = (primaryPos - lock->GetWorldPos()).Len();
					if(frameCount % 2 == 0) {
						//DrawDebugLine(GetWorldPos(), lock->GetWorldPos(), sf::Color::Cyan);
					}
					auto laserBox = Box2f::FromCenter(primaryPos, { 2.0f, 2.0f });
					auto playerBox = GetWorld()->GetPlayerWorldCollisionBox();
					auto laserDir = (lock->GetWorldPos() - primaryPos).Norm();
					m_sprite->SetWorldRot(laserDir.SignedAngleDeg());
					lock->GetSprite()->SetWorldRot(laserDir.SignedAngleDeg() + 180.0f);
					auto laserCheck = Math::SweepBoxAgainstBox(laserBox, laserDir * laserLength, playerBox);
					// Check if laser is hitting the player
					if(m_bActive && laserCheck.has_value() && laserCheck.value().m_dist < laserLength) {
						// Debug statements
						//printf("lasered!\n");
						//DrawDebugLine(primaryPos, primaryPos + (laserDir * laserCheck.value().m_dist), sf::Color::Red);
						//DrawDebugLine(GetWorldPos(), lock->GetWorldPos(), sf::Color::Red);

						GetWorld()->ZapPlayer(AsShared<Creature>(), primaryPos + (laserDir * laserCheck.value().m_dist));
					}
				}
			}
			// Toggle laser off periodically
			if(m_bActive && laserTimer >= activityPeriod) {
				m_taskMgr.RunManaged(DeactivateLaser());
				laserTimer = 0.0f;
			}
			// Toggle laser back on (uses a separate period)
			else if(!m_bActive && laserTimer >= inactivityPeriod) {
				m_taskMgr.RunManaged(ActivateLaser());
				laserTimer = 0.0f;
			}
		}
		else if(!m_laserTargets.size() && prevTargetCount > 0) {
			m_sprite->PlayAnim("Laser/Idle", true);
			//m_hitPause = 0.153f;
		}
		// "Spoilsport" firing-after-death behavior
		else if(!m_laserTargets.size() && !m_bIsFrozen && fireTimer <= 0.0f) {
			// TODO: change this into burst of multiple suicide bullet(s):
			bulletTransform.pos = GetWorldPos();
			if(sniper) { //< Target player directly
				targetDir = (world->GetPlayerWorldPos(true) - GetWorldPos()).Norm();
			}
			else { //< Fire in regular rotational pattern (deprecated)
				targetDir = targetDir.RotateDeg(rotateIncrement);
				m_sprite->SetWorldRot(targetDir.SignedAngleDeg());
			}
			auto bullet = Actor::Spawn<Projectile>(world, bulletTransform, g_creatureBulletDef, targetDir, m_bIsFacingRight ? 1 : -1);
			fireTimer = fireDelay;
			DeferredDestroy();
		}

		// Direction-changing behavior
		auto vel = m_direction * m_moveSpeed * m_moveSpeedModifier;
		if(m_bJustFlipped) {
			Move(GetWorldPos() + (vel * DT()), false, m_moveSpeed * DT(), AsShared<Creature>());
			m_bJustFlipped = false;
		}
		else if(!m_blockCardinals.Right() && !m_blockCardinals.Left()) {
			Move(GetWorldPos() + (vel * DT()), false, m_moveSpeed * DT(), AsShared<Creature>());
		}
		else {
			m_direction *= -1.0f;
			m_bJustFlipped = true;
		}

		prevTargetCount = (int32_t)m_laserTargets.size();

		co_await Suspend();
	}
}
Task<> Laser::DrawBeam(std::weak_ptr<Creature> in_target) {
	auto fullTileCount = 52; //< Maximum size -- game will crash if a laser needs to be longer than 52 tiles! TODO: make this less brittle
	auto prevTileCount = 0;

	// Targeting data
	auto tilePos = GetWorldPos();
	auto targetVec = Vec2f::Zero;
	auto targetDir = Vec2f::Zero;
	auto targetDist = targetVec.Len();
	int32_t targetTileCount = 0;

	// Helper lambda to update targeting data every tick
	auto UpdateTarget = [this, &targetVec, &targetDir, &targetDist, &targetTileCount, &tilePos](std::shared_ptr<Creature> in_creature) {
		tilePos = GetWorldPos();
		targetVec = in_creature->GetWorldPos() - GetWorldPos();
		targetDir = targetVec.Norm();
		targetDist = targetVec.Len();
		targetTileCount = (int32_t)(ceil(targetDist / 6.0f));
	};
	auto prevTargetVec = Vec2f::Zero;

	// Create beam sprites, set starting pos/rot/visibility for each
	if(m_laserTargets.size()) {
		auto target = in_target.lock();
		UpdateTarget(target);
		for(auto i = 0; i < fullTileCount; ++i) {
			auto pos = tilePos + (targetDir * float(i) * 6.0f);
			auto rot = targetDir.SignedAngleDeg() - 90.0f;

			// Make the laser sprites
			auto newSprite = MakeSprite(Transform::Identity);
			newSprite->PlayAnim("LaserBeam/Idle", true);
			newSprite->SetWorldPos(pos);
			newSprite->SetWorldRot(rot);
			newSprite->SetComponentDrawOrder(0);

			// Make the lights
			auto light = Spawn<Light>({}, "LightRect/Core", "LightRect/Penumbra");
			light->AttachToActor(AsShared<Actor>(), false);

			// Hide any tiles that overshoot desired laser length
			if(i >= targetTileCount) {
				newSprite->SetHidden(true);
			}
			m_laserBeamSprites.push_back(newSprite);
			m_laserBeamLights.push_back(light);
		}
	}
	else {
		co_return; //< Early-out if this laser doesn't have a target
	}

	// Main beam-length update loop
	while(true) {
		if(m_laserTargets.size()) {
			auto target = in_target.lock();
			UpdateTarget(target);

			auto targetPos = target->GetWorldPos();
			// Early-out if the paired Laser creatures haven't moved relative to each other
			if(targetVec != prevTargetVec) {
				// No sprites are created/destroyed in here -- just visibility toggled
				if(targetTileCount < prevTileCount) {
					for(auto i = prevTileCount; i >= targetTileCount; --i) {
						m_laserBeamSprites[i]->SetHidden(true);
						m_laserBeamLights[i]->SetHidden(true);
					}
				}
				else if(targetTileCount > prevTileCount) {
					for(auto i = prevTileCount; i < targetTileCount; ++i) {
						m_laserBeamSprites[i]->SetHidden(false);
						m_laserBeamLights[i]->SetHidden(false);
					}
				}

				// Re-position and -rotate visible tiles based on current target pos
				for(auto i = 0; i < targetTileCount; ++i) {
					m_laserBeamSprites[i]->SetWorldPos(tilePos + (targetDir * float(i) * 6.0f));
					m_laserBeamSprites[i]->SetWorldRot(targetDir.SignedAngleDeg() - 90.0f);
					m_laserBeamLights[i]->SetWorldPos(tilePos + (targetDir * float(i) * 6.0f));
					m_laserBeamLights[i]->SetWorldRot(targetDir.SignedAngleDeg() - 90.0f);
				}
			}
			prevTileCount = targetTileCount;
			prevTargetVec = targetVec;
			co_await Suspend();
		}
		else {
			co_return;
		}
	}
}
Task<> Laser::DeactivateLaser() {
	auto world = GetWorld();
	if(m_laserBeamSprites.size()) {
		for(auto light : m_laserBeamLights) {
			light->SetHidden(true);
		}
		for(auto sprite : m_laserBeamSprites) {
			sprite->SetPlayRate(8.0f);
			sprite->PlayAnim("LaserBeam/Fade", false);
		}
		// Hack timer to make sure all the other sprites are done
		m_bActive = false;
		co_await m_laserBeamSprites[0]->PlayAnim("LaserBeam/Fade", false);
	}
	co_return;
}
Task<> Laser::ActivateLaser() {
	auto world = GetWorld();
	if(m_laserBeamSprites.size()) {
		for(auto sprite : m_laserBeamSprites) {
			sprite->SetPlayRate(8.0f);
			sprite->PlayAnim("LaserBeam/Appear", false);
		}
		for(auto light : m_laserBeamLights) {
			light->SetHidden(false);
		}
		// Hack timer to make sure all the other sprites are done
		co_await m_laserBeamSprites[0]->PlayAnim("LaserBeam/Appear", false);
		m_bActive = true;
		for(auto sprite : m_laserBeamSprites) {
			sprite->SetPlayRate(1.0f);
			sprite->PlayAnim("LaserBeam/Idle", true);
		}
	}
	co_return;
}
Task<> Laser::KillLaser() {
	if(m_bActive && m_laserBeamSprites.size()) {
		for(auto sprite : m_laserBeamSprites) {
			sprite->SetPlayRate(8.0f);
			sprite->PlayAnim("LaserBeam/Fade", false);
		}
		co_await m_laserBeamSprites[0]->PlayAnim("LaserBeam/Fade", false);
		for(auto sprite : m_laserBeamSprites) {
			sprite->Destroy();
		}
		for(auto light : m_laserBeamLights) {
			light->Destroy();
		}
		m_laserBeamSprites.clear();
		m_laserBeamLights.clear();
		m_laserTargets.clear();
		m_bActive = false;
	}
	co_return;
}
void Laser::ClearTargets() {
	m_laserTargets.clear();
}
Task<> Laser::PreDeath() {
	if(GetHealth() <= 0) {
		if(m_laserTargets.size()) {
			auto primary = m_bPrimary ? AsShared<Laser>() : m_laserTargets[0].lock();
			co_await primary->KillLaser();
		}
	}
}
void Laser::TouchCallback(std::shared_ptr<GameActor> in_otherLaser) {
	if(std::dynamic_pointer_cast<Laser>(in_otherLaser)) {
		m_bIsFacingRight = !m_bIsFacingRight;
		m_bJustFlipped = true;
		m_direction *= -1.0f;
	}
}