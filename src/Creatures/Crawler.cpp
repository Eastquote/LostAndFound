#include "Creatures/Crawler.h"

#include "GameWorld.h"
#include "ParticleSpawnerDefs.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ColliderComponent.h"
#include <iostream>

std::shared_ptr<SpawnerDef<Crawler>> Crawler::s_spawnerDef = std::make_shared<SpawnerDef<Crawler>>(
	1,			// maxAlive: maximum that can be alive at the same time
	-1.0f,		// spawnCooldown: -1 means "only spawn based on proximity (ignore time)"
	true,		// spawnOnEnter: true means "spawn as soon as player starts to enter enclosing room"
	-1.0f,		// triggerDistX: if m_spawnOnEnter is false,  spawn when player is WITHIN this X distance
	255.0f,		// minRespawnDistX: once killed, only respawn when player is OUTSIDE this X distance
	false,		// spawnFacingPlayer: should they?
	std::array<float, 6>{ 40.0f, 52.0f, 8.0f, 0.0f, 0.0f, 2.0f }, // Nothing, SmEnergy, LgEnergy, Missile, SuperMissile, Grenade
	eTriggerTarget::None	// spawnOnTrigger: true means enemy will spawn when player touches a Trigger object with same name as this spawner's SpawnType
);

//-- CRAWLER CREATURE CODE --//

Crawler::Crawler(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name)
	: Creature(in_instigator)
{
	m_bIsFacingRight = !in_direction;
	m_bIsMovingRight = !in_direction;
	m_initialCollisionDims = { 16.0f, 16.0f };
	SetHealth(2);
}
void Crawler::Initialize() {
	Creature::Initialize();
	SetDamageInfo(8);

	// World collision box fixup
	m_worldCollisionBoxLocal = Box2f::FromCenter(Vec2f::Zero, { 14.0f, 14.0f }); //< shrink down the world collision now that everything else is set up

	// Behavior setup
	m_moveSpeed = 0.75f * 60.0f;
	auto moveDir = !m_bIsMovingRight ? -1.0f : 1.0f;
	m_moveSpeed = std::abs(m_moveSpeed) * moveDir;
}
void Crawler::ExplodeAndDie() {
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
void Crawler::ConfigSprites(std::string in_animName, std::string in_animNameLit,
	bool in_bFlipVert, bool in_bFlipHori, bool in_bFlipVertLit, bool in_bFlipHoriLit) {
		m_sprite->PlayAnim(in_animName, true);
		m_spriteLit->PlayAnim(in_animNameLit, true);
		m_sprite->SetFlipVert(in_bFlipVert);
		m_sprite->SetFlipHori(in_bFlipHori);
		m_spriteLit->SetFlipVert(in_bFlipVertLit);
		m_spriteLit->SetFlipHori(in_bFlipHoriLit);
};
std::string Crawler::GetFloorDir(bool in_bIsInitial) {
	std::string floorDir;
	auto normal = SpawnSurfaceCheck(false, false, false, false);
	if(normal == Vec2f::Up) {
		floorDir = "Down";
		if(in_bIsInitial) {
			Move(GetWorldPos() + Vec2f{ 0.0f, -m_blockCardinals.DownDist() }, true);
		}
		ConfigSprites("Crawler/Hori", "Crawler/HoriLit", false, false, false, false);
		m_sprite->PlayAnim("Crawler/Hori", true);
		m_spriteLit->PlayAnim("Crawler/HoriLit", true);
	}
	else if(normal == Vec2f::Right) {
		floorDir = "Left";
		if(in_bIsInitial) {
			Move(GetWorldPos() + Vec2f{ -m_blockCardinals.LeftDist(), 0.0f }, true);
		}
		ConfigSprites("Crawler/Vert", "Crawler/VertLit", false, true, false, true);
	}
	else if(normal == Vec2f::Down) {
		floorDir = "Up";
		if(in_bIsInitial) {
			Move(GetWorldPos() + Vec2f{ 0.0f, m_blockCardinals.UpDist() }, true);
		}
		ConfigSprites("Crawler/Hori", "Crawler/HoriLit", true, true, true, true);
	}
	else if(normal == Vec2f::Left) {
		floorDir = "Right";
		if(in_bIsInitial) {
			Move(GetWorldPos() + Vec2f{ m_blockCardinals.RightDist(), 0.0f }, true);
		}
		ConfigSprites("Crawler/Vert", "Crawler/VertLit", true, false, true, false);
	}
	return floorDir;
}

Vec2f Crawler::SetFloorDir(const std::string& in_dir) {
	const float speed = m_moveSpeed;
	if(in_dir == "Down") {
		bool flipHori = m_bIsFacingRight ? false : true;
		ConfigSprites("Crawler/Hori", "Crawler/HoriLit", false, flipHori, false, flipHori);
		return Vec2f{ speed, 0.0f };
	}
	else if(in_dir == "Left") {
		bool flipVert = m_bIsFacingRight ? true : false;
		ConfigSprites("Crawler/Vert", "Crawler/VertLit", flipVert, true, flipVert, true);
		return Vec2f{ 0.0f, -speed };
	}
	else if(in_dir == "Up") {
		bool flipHori = m_bIsFacingRight ? true : false;
		ConfigSprites("Crawler/Hori", "Crawler/HoriLit", true, flipHori, true, flipHori);
		return Vec2f{ -speed, 0.0f };
	}
	else if(in_dir == "Right") {
		bool flipVert = m_bIsFacingRight ? false : true;
		ConfigSprites("Crawler/Vert", "Crawler/VertLit", flipVert, false, flipVert, false);
		return Vec2f{ 0.0f, speed };
	}
	else {
		return Vec2f::Zero;
	}
}
Task<> Crawler::ManageAI() {
	m_floorDir = GetFloorDir(true);
	auto moveVec = SetFloorDir(m_floorDir);
	auto bJustTurned = false;
	auto lastFloor = "";
	bool isFloating;
	bool isStuckMidAir = false;
	auto fireDelay = 1.5f;
	auto fireTimer = fireDelay;
	bool sniper = false;
	auto world = GetWorld();
	Transform bulletTransform = { GetWorldPos(), 0.0f, Vec2f::One };
	std::shared_ptr<Projectile> bullet = {};
	//co_await WaitSeconds(1.0f / 60.0f);
	float dtCapped = 0.0f;
	while(true) {
		//std::cout << std::endl << "DT() = " << DT() << std::endl;
		dtCapped = DT() < .017 ? DT() : 1.0f / 60.0f;
		bulletTransform.pos = GetWorldPos();
		fireTimer -= DT();
		isFloating = !m_blockCardinals.Up() && !m_blockCardinals.Down() && !m_blockCardinals.Left() && !m_blockCardinals.Right();
		//std::cout << "isFloating = " << isFloating << std::endl;
		//std::cout << "moveVec = " << moveVec.x << ", " << moveVec.y << std::endl;
		//std::cout << "lastFloor = " << lastFloor << std::endl;
		//std::cout << "Up: " << m_blockCardinals.UpDist() << " | Down: " << m_blockCardinals.DownDist() << " | Left: " << m_blockCardinals.LeftDist() << " | Right: " << m_blockCardinals.RightDist() << std::endl;
		//std::cout << "isFloating = " << isFloating << std::endl; 
		//std::cout << "bJustTurned = " << bJustTurned << std::endl; 


		if(fireTimer <= 0.0f) {
			bulletTransform.pos = GetWorldPos();
			auto targetDir = moveVec;
			if(sniper) {
				targetDir = (world->GetPlayerWorldPos(true) - GetWorldPos()).Norm();
			}
			// Firing bullet behavior (turned off for now)
			//bullet = Actor::Spawn<Projectile>(world, bulletTransform, g_creatureBulletDef, targetDir, m_bIsFacingRight ? 1 : -1);
			fireTimer = fireDelay;
		}

		auto speed = m_moveSpeed * m_moveSpeedModifier;
		if(bJustTurned) {
			// Perform post-turn "fixup" moves
			Move(GetWorldPos() + moveVec * dtCapped, false, std::abs(speed * dtCapped), AsShared<Creature>()); //< First, scoot over to other side of corner (so new "floor" is below collision)
			auto normal = SpawnSurfaceCheck(false, false, false, true); //< Then, correct pos to sit flush with new floor (i.e. zero distance between collision and floor)
			isFloating = !m_blockCardinals.Up() && !m_blockCardinals.Down() && !m_blockCardinals.Left() && !m_blockCardinals.Right();
			// Debug statements
			//std::cout << "POST-TURN-FIXUP NUMBERS:" << std::endl;
			//std::cout << "Up: " << m_blockCardinals.UpDist() << " | Down: " << m_blockCardinals.DownDist() << " | Left: " << m_blockCardinals.LeftDist() << " | Right: " << m_blockCardinals.RightDist() << std::endl;
			//std::cout << "isFloating = " << isFloating << std::endl;
		}
		else if(m_bJustFlipped) {
			Move(GetWorldPos() + moveVec * dtCapped, false, std::abs(speed * dtCapped), AsShared<Creature>());
			m_bJustFlipped = false;
		}
		auto startDirWasRight = m_bIsMovingRight; //< Saving this to compare later

		// "Floating mid-air" cases
		if(isFloating && bJustTurned) { //< Just went mid-air (always happens on turns)
			Move(GetWorldPos() + (moveVec * dtCapped * 2.0f), false, std::abs(speed * dtCapped), AsShared<Creature>());
			bJustTurned = false;
		}
		else if(isFloating && !bJustTurned && !isStuckMidAir) { 
			// Still mid-air after a turn would have ended -- uh oh! back up to prep for GetFloorDir()
			Move(GetWorldPos() + (moveVec * dtCapped * -2.0f), false, std::abs(speed * dtCapped * 2.0f), AsShared<Creature>());
			isStuckMidAir = true;
		}
		else if(isStuckMidAir) { 
			// Find nearest floor and snap flush to it
			auto newFloor = GetFloorDir(true);
			if(newFloor != "") {
				moveVec = SetFloorDir(newFloor);
				lastFloor = "";
				isStuckMidAir = false;
			}
		}
		if(m_blockCardinals.Down() && lastFloor != "Down" && lastFloor != "Up") { //< 
			moveVec = Vec2f{ speed, 0.0f };
			Move(GetWorldPos() + moveVec * dtCapped, false, std::abs(speed * dtCapped), AsShared<Creature>());
			if(!m_blockCardinals.Down() && !bJustTurned) { //< If that move just walked you off a ledge
				if(m_bIsMovingRight) {
					moveVec = SetFloorDir("Left");
				}
				else {
					moveVec = SetFloorDir("Right");
				}
				lastFloor = "Down"; //< You're about to be on a new floor now so update lastFloor
				bJustTurned = true;
				continue;
			}
			else if(m_blockCardinals.Right() && !bJustTurned && startDirWasRight) {
				if(m_bJustFlipped) { 
					// This means it hit the room boundary during the Move() func and was flipped at that time
					moveVec = SetFloorDir("Down");
					continue;
				}
				moveVec = SetFloorDir("Right");
				lastFloor = "Down";
				bJustTurned = true;
				continue;
			}
			else if(m_blockCardinals.Left() && !bJustTurned && !startDirWasRight) {
				if(m_bJustFlipped) {
					// This means it hit the room boundary during the Move() func and was flipped at that time
					moveVec = SetFloorDir("Down");
					continue;
				}
				moveVec = SetFloorDir("Left");
				lastFloor = "Down";
				bJustTurned = true;
				continue;
			}
			bJustTurned = false;
		}
		else if(m_blockCardinals.Left() && lastFloor != "Left" && lastFloor != "Right") {
			moveVec = Vec2f{ 0.0f, -speed };
			Move(GetWorldPos() + moveVec * dtCapped, false, std::abs(speed * dtCapped), AsShared<Creature>());
			if(!m_blockCardinals.Left() && !bJustTurned) {
				if(m_bIsMovingRight) {
					moveVec = SetFloorDir("Up");
				}
				else {
					moveVec = SetFloorDir("Down");
				}
				lastFloor = "Left";
				bJustTurned = true;
				continue;
			}
			else if(m_blockCardinals.Down() && !bJustTurned && startDirWasRight) {
				if(startDirWasRight != m_bIsMovingRight) { 
					// This means it hit the room boundary during the Move() func and was flipped at that time
					moveVec = SetFloorDir("Left");
					continue;
				}
				moveVec = SetFloorDir("Down");
				lastFloor = "Left";
				bJustTurned = true;
				continue;
			}
			else if(m_blockCardinals.Up() && !bJustTurned && !startDirWasRight) {
				if(startDirWasRight != m_bIsMovingRight) { 
					// This means it hit the room boundary during the Move() func and was flipped at that time
					moveVec = SetFloorDir("Left");
					continue;
				}
				moveVec = SetFloorDir("Up");
				lastFloor = "Left";
				bJustTurned = true;
				continue;
			}
			bJustTurned = false;
		}
		else if(m_blockCardinals.Up() && lastFloor != "Up" && lastFloor != "Down") {
			moveVec = Vec2f{ -speed, 0.0f };
			Move(GetWorldPos() + moveVec * dtCapped, false, std::abs(speed * dtCapped), AsShared<Creature>());
			if(!m_blockCardinals.Up() && !bJustTurned) {
				if(m_bIsMovingRight) {
					moveVec = SetFloorDir("Right");
				}
				else {
					moveVec = SetFloorDir("Left");
				}
				lastFloor = "Up";
				bJustTurned = true;
				continue;
			}
			else if(m_blockCardinals.Left() && !bJustTurned && startDirWasRight) {
				if(m_bJustFlipped) { 
					// This means it hit the room boundary during the Move() func and was flipped at that time
					moveVec = SetFloorDir("Up");
					continue;
				}
				moveVec = SetFloorDir("Left");
				lastFloor = "Up";
				bJustTurned = true;
				continue;
			}
			else if(m_blockCardinals.Right() && !bJustTurned && !startDirWasRight) {
				if(m_bJustFlipped) { 
					// This means it hit the room boundary during the Move() func and was flipped at that time
					moveVec = SetFloorDir("Up");
					continue;
				}
				moveVec = SetFloorDir("Right");
				lastFloor = "Up";
				bJustTurned = true;
				continue;
			}
			bJustTurned = false;
		}
		else if(m_blockCardinals.Right() && lastFloor != "Right" && lastFloor != "Left") {
			moveVec = Vec2f{ 0.0f, speed };
			Move(GetWorldPos() + moveVec * dtCapped, false, std::abs(speed* dtCapped), AsShared<Creature>());
			if(!m_blockCardinals.Right() && !bJustTurned) {
				if(m_bIsMovingRight) {
					moveVec = SetFloorDir("Down");
				}
				else {
					moveVec = SetFloorDir("Up");
				}
				lastFloor = "Right";
				bJustTurned = true;
				continue;
			}
			else if(m_blockCardinals.Up() && !bJustTurned && startDirWasRight) {
				if(m_bJustFlipped) { 
					// This means it hit the room boundary during the Move() func and was flipped at that time
					moveVec = SetFloorDir("Right");
					continue;
				}
				moveVec = SetFloorDir("Up");
				lastFloor = "Right";
				bJustTurned = true;
				continue;
			}
			else if(m_blockCardinals.Down() && !bJustTurned && !startDirWasRight) {
				if(m_bJustFlipped) { 
					// This means it hit the room boundary during the Move() func and was flipped at that time
					moveVec = SetFloorDir("Right");
					continue;
				}
				moveVec = SetFloorDir("Down");
				lastFloor = "Right";
				bJustTurned = true;
				continue;
			}
			bJustTurned = false;
		}
		else if(!isFloating && !isStuckMidAir) {
			// Debug statements for floating-midair case
			printf("WARNING: this Crawler has no valid moves! Breaking movement loop now!\n");
			//std::cout << "lastFloor = " << lastFloor << std::endl;
			//std::cout << "bJustTurned = " << bJustTurned << std::endl;
			//std::cout << "m_blockCardinals.Up = " << m_blockCardinals.Up() << std::endl;
			//std::cout << "m_blockCardinals.Down = " << m_blockCardinals.Down() << std::endl;
			//std::cout << "m_blockCardinals.Left = " << m_blockCardinals.Left() << std::endl;
			//std::cout << "m_blockCardinals.Right = " << m_blockCardinals.Right() << std::endl;
			break;
		}
		co_await Suspend();
	}
}
