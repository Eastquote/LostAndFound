#include "Projectile.h"

#include "GameEnums.h"
#include "GameWorld.h"
#include "Player.h"
#include "PlayerStatus.h"
#include "Door.h"
#include "Creature.h"
#include "ProjectileManager.h"
#include "AudioManager.h"
#include "Effect.h"
#include "DestroyedTile.h"
#include "ParticleSpawnerDefs.h"
#include "Engine/MathGeometry.h"
#include "Engine/EaseTo.h"
#include "Engine/Components/TilesComponent.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/TileMap.h"
#include "Engine/DebugDrawSystem.h"
#include "Engine/MathEasings.h"
#include "Engine/MathRandom.h"
#include "Engine/MathDynamics.h"
#include <iostream>
#include <math.h>

Projectile::Projectile(const ProjectileDef& in_def, const Vec2f& in_dir, int32_t in_facingDir, const float in_mag)
	: m_worldCollisionBox(Box2f::FromCenter(Vec2f::Zero, in_def.collisionBoxDims))
	, m_speed(in_def.speed * in_mag)
	, m_direction(in_dir)
	, m_facingDir(in_facingDir)
	//, m_mag(in_mag)
	, m_def(in_def)
{
}

//--- PROJECTILE CODE ---//

void Projectile::Initialize() {
	GameActor::Initialize();
	m_bUpdatesOffScreen = false;
	m_spawnPos = GetWorldPos();
	m_worldCollisionBox = Box2f::FromCenter(Vec2f::Zero, { m_def.sensorRadius, m_def.sensorRadius });
	
	// Setup damageInfo
	SetDamageInfo(m_def.payload, m_def.damageFlags);

	// Setup sensor
	Circle projectileCollisionCircle = { m_def.sensorRadius };
	SensorShape projectileSensorShape;
	projectileSensorShape.SetCircle(projectileCollisionCircle);
	auto projectileSensor = MakeSensor(Transform::Identity, projectileSensorShape);
	projectileSensor->SetFiltering(m_def.category, m_def.mask);
	projectileSensor->SetTouchCallback([this](bool in_beginning, std::shared_ptr<SensorComponent> in_other) {
		if(in_beginning) {
			if(std::dynamic_pointer_cast<Creature>(in_other->GetActor())) {
				OnTouchCreature(std::dynamic_pointer_cast<Creature>(in_other->GetActor()), in_other);
			}
			if(std::dynamic_pointer_cast<Player>(in_other->GetActor())) {
				OnTouchPlayer(std::dynamic_pointer_cast<Player>(in_other->GetActor()), in_other);
			}
			if(std::dynamic_pointer_cast<Projectile>(in_other->GetActor())) {
				OnTouchProjectile(std::dynamic_pointer_cast<Projectile>(in_other->GetActor()), in_other);
			}
			if(std::dynamic_pointer_cast<Door>(in_other->GetActor())) {
				OnTouchDoor(std::dynamic_pointer_cast<Door>(in_other->GetActor()), in_other);
			}
		}
	});

	// Setup sprite
	m_projectileSprite = MakeSprite(Transform::Identity);
	m_projectileSprite->SetRenderLayer("hud");
	m_projectileSprite->PlayAnim(m_def.animName, true);
	m_projectileSprite->SetWorldRot(float(Math::VecToDegrees(m_direction)));

	// Setup orientation
	if(m_direction.x < 0.0f) {
		m_projectileSprite->SetFlipVert(true);
	}

	// Register
	GetWorld()->GetProjectileManager()->RegisterProjectile(AsShared<Projectile>());
}
void Projectile::Destroy() {
	GameActor::Destroy();
}
void Projectile::DeferredDestroy() {
	if(std::dynamic_pointer_cast<Creature>(m_instigator)) {
		std::dynamic_pointer_cast<Creature>(m_instigator)->ChangeProjectileCount(-1);
	}
	GameActor::DeferredDestroy();
}
Task<> Projectile::ManageActor() {
	while(true) {
		if(!m_bIsExploding && (m_elapsedLifetime >= m_def.lifetime || m_bExplosionTriggered)) {
			co_await ExplodeTask();
		}
		auto newPos = ModifyMovement();
		Move(newPos);
		m_projectileSprite->SetWorldRot(m_projectileSprite->GetWorldRot() + m_rotationSpeed * 360.0f * DT());
		co_await Suspend();
	}
}
Vec2f Projectile::ModifyMovement() {
	// Overridden in child classes
	return GetWorldPos() + (GetVel() * DT());
}
Vec2f Projectile::Move(Vec2f in_pos) {
	auto dmgInfo = GetDamageInfo();
	auto newPos = in_pos - GetWorldPos();
	auto bOpensDoors = (m_def.mask & CL_Door) != 0;
	auto penetratesWalls = dmgInfo.m_damageFlags & DF_NoWalls;
	auto isCharged = dmgInfo.m_damageFlags & DF_Charged;
	int32_t tile = 0;
	auto sweepResults = Math::SweepBoxAgainstGrid(GetCollisionBoxWorld(), newPos,
		GetCollisionTilesComp()->GetGridToWorldTransform(),
		GetCollisionTilesComp()->GetTileLayer()->GetGridBoundingBox(),
		[this, tile, bOpensDoors](Vec2i in_testPos) mutable {
			tile = GetCollisionTilesComp()->GetTileLayer()->GetTile(in_testPos);
			return bOpensDoors ? (tile == 237) : (tile > 0);
		});
	if(sweepResults && !m_bIgnoresWorld) { //< Filter out homing missiles here
		auto newVec = sweepResults->m_sweptBox.GetBottomLeft() - GetCollisionBoxWorld().GetBottomLeft();
		auto tileBoundaryPos = GetWorldPos() + newVec + (newVec.Norm() * ((m_def.sensorRadius / 2) - 0.01f));
		if(m_bounce) {
			Bounce(sweepResults.value(), m_bounce);
		}
		else if(TryDestroyTiles(tileBoundaryPos, -sweepResults->m_normal, penetratesWalls, isCharged ? 1 : 0)) {
			auto effect = Actor::Spawn<Effect>(GetWorld(), { tileBoundaryPos }, m_def.hitEffectAnimName);
			AudioManager::Get()->PlaySound(m_def.hitEffectSoundName);
			DeferredDestroy();
		}
		else {
			SetWorldPos(GetWorldPos() + newPos);
		}
	}
	// Destroy if off-camera
	else if(GetWorld()->ProjectileIsOffCamera(AsShared<Projectile>()) && !m_bUpdatesOffScreen) {
		DeferredDestroy();
	}
	else {
		SetWorldPos(GetWorldPos() + newPos);
	}
	m_elapsedLifetime += DT();
	// Debug statements
	//std::cout << "Projectile mag = " << newPos.Len() << std::endl;
	//auto worldTiles = GetWorld()->GetWorldTilesComp();
	//auto worldLayer = worldTiles->GetTileLayer();
	//auto actorGridPos = worldTiles->WorldPosToGridPos(GetWorldPos() + newPos);
	//DrawDebugBox(worldTiles->GridPosToWorldBox(actorGridPos), sf::Color::Cyan);
	return newPos;
}
void Projectile::Bounce(const Math::BoxSweepResults& in_sweepResults, float in_amount) {
	// Modifies m_direction and m_speed to reflect (eyyy) a bounce off the world geometry
	auto vel = GetVel();
	auto normalUpVec = in_sweepResults.m_normal;
	auto normalRightVec = normalUpVec.RotateDeg(-90.0f);
	auto bounceVec = (normalRightVec * vel.Dot(normalRightVec) + (normalUpVec * vel.Dot(normalUpVec) * -1));
	m_direction = bounceVec.Norm();
	m_speed = bounceVec.Len() * in_amount;
	if(m_speed < 20.0f) {
		m_rotationSpeed *= in_amount;
	}
	if(m_elapsedLifetime > m_def.lifetime * 0.7f && !m_finalCountdown) {
		m_projectileSprite->SetPlayRate(m_animPlayrate *= 6.0f);
		m_finalCountdown = true;
	}
}
void Projectile::MakeAoeSensor(float in_radius) {
	Circle projectileCollisionCircle = { in_radius };
	SensorShape projectileSensorShape;
	projectileSensorShape.SetCircle(projectileCollisionCircle);
	auto projectileSensor = MakeSensor(Transform::Identity, projectileSensorShape);
	projectileSensor->SetFiltering(m_def.category, CL_Enemy | CL_Player | CL_Door | CL_Pickup);
	projectileSensor->SetTouchCallback([this](bool in_beginning, std::shared_ptr<SensorComponent> in_other) {
		auto alreadyHit = std::find(m_actorsAlreadyHit.begin(), m_actorsAlreadyHit.end(), 
									in_other->GetActor()) != m_actorsAlreadyHit.end();
		if(!alreadyHit && m_bIsExploding && in_beginning) {
			if(std::dynamic_pointer_cast<Creature>(in_other->GetActor())) {
				OnTouchCreature(std::dynamic_pointer_cast<Creature>(in_other->GetActor()), in_other);
			}
			if(std::dynamic_pointer_cast<Player>(in_other->GetActor())) {
				OnTouchPlayer(std::dynamic_pointer_cast<Player>(in_other->GetActor()), in_other);
			}
			if(std::dynamic_pointer_cast<Door>(in_other->GetActor())) {
				OnTouchDoor(std::dynamic_pointer_cast<Door>(in_other->GetActor()), in_other);
			}
		}
	});
}
void Projectile::Detonate() {
	m_taskMgr.RunManaged(ExplodeTask());
}
Task<> Projectile::ExplodeTask() {
	m_bIsExploding = true;
	auto effect = Actor::Spawn<Effect>(GetWorld(), GetWorldTransform(), m_def.hitEffectAnimName);
	Actor::Spawn<ParticleSpawner>(GetWorld(), { GetWorldPos() },
		g_debrisSpawnerSm1, std::nullopt, "Base");
	Actor::Spawn<ParticleSpawner>(GetWorld(), { GetWorldPos() },
		g_emberSpawnerLg1, std::nullopt, "Base");
	Actor::Spawn<ParticleSpawner>(GetWorld(), { GetWorldPos() },
		g_emberSpawnerLg2, std::nullopt, "Base");
	m_projectileSprite->PlayAnim("Util/Blank", true);
	m_speed = 0.0f;
	MakeAoeSensor(16.0f);
	AudioManager::Get()->PlaySound("Explosion");
	TryDestroyTiles(GetWorldPos(), Vec2f{ 1.0f, 0.0f }, false, 2);
	auto elapsedTime = 0.0f;
	while(elapsedTime < m_explosionTime) {
		co_await Suspend();
		elapsedTime += DT();
	}
	DeferredDestroy();
}
void Projectile::DestroyBullet() {
	auto effect = Actor::Spawn<Effect>(GetWorld(), GetWorldTransform(), m_def.hitEffectAnimName);
	DeferredDestroy();
}
void Projectile::OnTouchCreature(std::shared_ptr<Creature> in_creature, std::shared_ptr<SensorComponent> in_sensor) {
	// Eliminate friendly fire between enemies
	if(std::dynamic_pointer_cast<Creature>(m_instigator) && in_creature->GetDamageInfo().m_damageFlags & DF_Enemy) {
		return;
	}
	auto dmgInfo = GetDamageInfo();
	auto hitResponse = in_creature->HandleDamage(dmgInfo, in_sensor);
	if((GetDamageInfo().m_damageFlags & DF_Explodes) && !m_bIsExploding) {
		Detonate();
	}
	else if(hitResponse != eHitResponse::Ignore && !(dmgInfo.m_damageFlags & DF_Penetrates) && !m_bIsExploding) {
		DestroyBullet();
	}
}
void Projectile::OnTouchPlayer(std::shared_ptr<Player> in_player, std::shared_ptr<SensorComponent> in_sensor) {
	auto dmgInfo = GetDamageInfo();

	// Player bomb case (deprecated)
	if(dmgInfo.m_damageFlags & DF_Explodes & DF_Player) {
		m_actorsAlreadyHit.push_back(in_player);
		return;
	}

	// Enemy bomb case -- explode on impact
	else if(dmgInfo.m_damageFlags & DF_Explodes & DF_Enemy) {
		Detonate();
	}
	else {
		DestroyBullet();
	}
}
void Projectile::OnTouchDoor(std::shared_ptr<Door> in_door, std::shared_ptr<SensorComponent> in_sensor) {
	if(!in_door->IsDoorOpen()) {
		DestroyBullet();
	}
}
void Projectile::OnTouchProjectile(std::shared_ptr<Projectile> in_projectile, std::shared_ptr<SensorComponent> in_sensor) {
	if(IsPlayerBullet() || (IsEnemyBullet() && in_projectile->IsPlayerBullet())) {
		if(GetDamageInfo().m_damageFlags & DF_Explodes) {
			Detonate();
		}
		else {
			DestroyBullet();
		}
	}
}
