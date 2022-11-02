#pragma once

#include "GameActor.h"
#include "GameWorld.h"
#include "Character.h"
#include "GameEnums.h"
#include "Engine/MathGeometry.h"

class Player;
class Creature;
class Door;
class Projectile;
class HomingMissile;
class WaveBeam;
class Grenade;

using tProjectileSpawnFn = std::function<std::shared_ptr<Projectile>(	std::shared_ptr<Character>, const Transform&, 
																		const ProjectileDef&, const Vec2f&, int32_t, const float)>;

template <typename T>
std::shared_ptr<T> SpawnProjectile(	std::shared_ptr<Character> in_instigator, const Transform& in_transform, 
									const ProjectileDef& in_def, const Vec2f& in_dir, int32_t in_facingDir, const float in_mag) {
	auto projectile = Actor::Spawn<T>(in_instigator->GetWorld(), in_transform, in_def, in_dir, in_facingDir, in_mag);
	projectile->SetInstigator(in_instigator);
	return projectile;
}

struct ProjectileDef {
	Vec2f collisionBoxDims = { 8.0f, 8.0f };
	float sensorRadius = 8.0f;
	float lifetime = 1.0f;
	float speed = 1.0f;
	int32_t payload = 1;
	std::string animName = "Util/Blank";
	uint32_t category = 0;
	uint32_t mask = 0;
	uint32_t damageFlags = 0;
	std::string hitEffectAnimName = "Util/Blank";
	std::string hitEffectSoundName = "Blank";
	eDamageType damageType = eDamageType::PlayerBeam; //< Deprecated -- use damageFlags instead
	tProjectileSpawnFn spawnFunc = &SpawnProjectile<Projectile>;
	std::shared_ptr<Projectile> Spawn(	std::shared_ptr<Character> in_instigator, const Transform& in_transform, 
										const Vec2f& in_dir, int32_t in_facingDir = 0, const float in_mag = 1.0f) const {
		return spawnFunc(in_instigator, in_transform, *this, in_dir, in_facingDir, in_mag);
	}
};

struct FireDef {
	int32_t maxLive = 1;
	float period = 1.0f;
	Vec2f posOffset = Vec2f::Zero;
	int32_t maxToFire = 1;
	bool bIsHoming = false;
	std::string fireAnim;
	std::string fireAnimLit;
	std::string resumeAnim;
	std::string resumeAnimLit;
};

// Projectile base class for all characters' weapons -- handles movement, sensing/collisions, damage info, and 
// explosion behavior as needed
class Projectile : public GameActor {
public:
	Projectile(const ProjectileDef& in_def, const Vec2f& in_dir, int32_t in_facingDir = 0, const float in_mag = 1.0f);
	virtual void Initialize() override;
	virtual void Destroy() override;
	virtual void DeferredDestroy() override;
	Task<> ManageActor();
	Box2f GetCollisionBoxLocal() const { return m_worldCollisionBox; };
	Box2f GetCollisionBoxWorld() const { return m_worldCollisionBox.TransformedBy(GetWorldTransform()); };
	virtual Vec2f ModifyMovement();

	Vec2f Move(Vec2f in_pos);
	void Bounce(const Math::BoxSweepResults& in_sweepResults, float in_amount);
	void MakeAoeSensor(float in_radius);
	void SetInstigator(std::shared_ptr<Character> in_character) { m_instigator = in_character; }
	Vec2f GetVel() { return m_direction * m_speed; }
	std::string GetHitEffectAnimName() { return m_def.hitEffectAnimName; }
	std::string GetHitEffectSoundName() { return m_def.hitEffectSoundName; }
	bool StillAlive() { return (m_elapsedLifetime <= m_def.lifetime); }
	void Detonate();
	bool IsPlayerBullet() { return (m_category & CL_PlayerBullet) != 0; }
	bool IsEnemyBullet() { return (m_category & CL_EnemyBullet) != 0; }

protected:
	Task<> ExplodeTask();
	void DestroyBullet();
	virtual void OnTouchCreature(std::shared_ptr<Creature> in_creature, std::shared_ptr<SensorComponent> in_sensor);
	virtual void OnTouchPlayer(std::shared_ptr<Player> in_player, std::shared_ptr<SensorComponent> in_sensor);
	void OnTouchDoor(std::shared_ptr<Door> in_door, std::shared_ptr<SensorComponent> in_sensor);
	void OnTouchProjectile(std::shared_ptr<Projectile> in_projectile, std::shared_ptr<SensorComponent> in_sensor);
	
	std::shared_ptr<Character> m_instigator;
	Vec2f m_spawnPos = Vec2f::Zero;
	// Sprite + anim data
	std::shared_ptr<SpriteComponent> m_projectileSprite;
	std::string m_animName = "Util/Blank";
	std::string m_hitEffectAnimName = "Util/Blank";
	std::string m_hitEffectSoundName = "Blank";
	float m_animPlayrate = 1.0f;
	// Collision/sensor + damage data
	Box2f m_worldCollisionBox = Box2f::FromCenter(Vec2f::Zero, { 8.0f, 8.0f });
	float m_sensorRadius = 8.0f;
	const ProjectileDef m_def;
	uint32_t m_category = 0;
	uint32_t m_mask = 0;
	// Physics data
	float m_speed = 1.0f * 60.0f; //< Pixels/sec
	float m_maxSpeed = 1.0f * 60.0f;
	Vec2f m_direction = { 1.0f, 0.0f };
	float m_rotationSpeed = 0.0f;
	int32_t m_facingDir = 0;
	float m_waveDir = 1.0f;
	float m_elapsedLifetime = 0.0f;
	//float m_mag = 1.0f * 60.0f;
	float m_bounce = 0.0f;
	float m_gravity = 0.05f * 60.0f;
	bool m_bIgnoresWorld = false;
	bool m_finalCountdown = false;
	// Explosion data
	bool m_bIsExploding = false;
	float m_explosionTime = .38f;
	bool m_bExplosionTriggered = false;
	std::vector<std::shared_ptr<GameActor>> m_actorsAlreadyHit;
};

//--- PROJECILE DEFS ---//

static ProjectileDef g_creatureBulletDef = {
	{ 0.5f, 0.5f },
	6.0f,
	3.0f,
	1.0f * 60.0f,
	5,
	"ShotBeam/Idle",
	CL_EnemyBullet,
	CL_Player | CL_Pickup,
	DF_Enemy | DF_Beam,
	"ShotBeamHitEffect/Idle",
	"WallHit2",
	eDamageType::EnemyBullet
};
static ProjectileDef g_creatureBulletDefFaster = {
	{ 0.5f, 0.5f },
	6.0f,
	3.0f,
	1.5f * 60.0f,
	5,
	"ShotBeam/Idle",
	CL_EnemyBullet,
	CL_Player | CL_Pickup,
	DF_Enemy | DF_Beam,
	"ShotBeamHitEffect/Idle",
	"CreatureHit",
	eDamageType::EnemyBullet
};

static ProjectileDef g_playerBulletDef = {
	{ 1.0f, 1.0f },
	0.5f,
	0.3f,
	4.0f * 60.0f,
	1,
	"ShotBeam/Idle",
	CL_PlayerBullet,
	CL_Enemy | CL_Armor | CL_Door | CL_Pickup,
	DF_Player | DF_Beam,
	"ShotBeamHitEffect/Idle",
	"WallHit",
	eDamageType::PlayerBeam
};
static ProjectileDef g_playerBulletChargedDef = {
	{ 1.0f, 1.0f },
	6.0f,
	0.25f,
	5.75f * 60.0f,
	5,
	"BeamCharged/Shot",
	CL_PlayerBullet,
	CL_Enemy | CL_Armor | CL_Door | CL_Pickup | CL_EnemyBullet,
	DF_Player | DF_Beam | DF_Charged | DF_Penetrates,
	"Explosion2/Explosion",
	"Explosion",
	eDamageType::PlayerChargedBeam,
};
static ProjectileDef g_playerBulletIceDef = {
	{ 1.0f, 1.0f },
	0.5f,
	0.3f,
	8.0f * 60.0f,
	1,
	"Effects/IceBeamPlayer",
	CL_PlayerBullet,
	CL_Enemy | CL_Armor | CL_Door | CL_Pickup | CL_EnemyBullet,
	DF_Player | DF_Beam | DF_Ice,
	"ShotBeamHitEffect/Idle",
	"CreatureHit",
	eDamageType::PlayerIceBeam
};
static ProjectileDef g_playerBulletIceChargedDef = {
	{ 1.0f, 1.0f },
	6.0f,
	1.35f,
	4.75f * 60.0f,
	1,
	"BeamCharged/Shot",
	CL_PlayerBullet,
	CL_Enemy | CL_Armor | CL_Door | CL_Pickup,
	DF_Player | DF_Beam | DF_Charged | DF_Ice,
	"ShotBeamHitEffect/Idle",
	"CreatureHit",
	eDamageType::PlayerChargedIceBeam
};
static ProjectileDef g_playerBulletLongDef = {
	{ 0.1f, 0.1f },
	0.5f,
	1.3f,
	4.0f * 60.0f,
	1,
	"ShotBeam/Idle",
	CL_PlayerBullet,
	CL_Enemy | CL_Armor | CL_Door | CL_Pickup | CL_EnemyBullet,
	DF_Player | DF_Beam,
	"ShotBeamHitEffect/Idle",
	"WallHit2",
	eDamageType::PlayerBeam
};
static ProjectileDef g_playerBulletLongIceDef = {
	{ 0.1f, 0.1f },
	0.5f,
	1.3f,
	4.0f * 60.0f,
	1,
	"Effects/IceBeamPlayer",
	CL_PlayerBullet,
	CL_Enemy | CL_Armor | CL_Door | CL_Pickup | CL_EnemyBullet,
	DF_Player | DF_Beam | DF_Ice,
	"ShotBeamHitEffect/Idle",
	"CreatureHit",
	eDamageType::PlayerIceBeam
};
static ProjectileDef g_playerMissileDef = {
	{ 1.0f, 1.0f },
	0.5f,
	1.3f,
	4.0f * 60.0f,
	5,
	"ShotMissile/Idle",
	CL_PlayerBullet,
	CL_Enemy | CL_Armor | CL_Door | CL_Pickup,
	DF_Player | DF_Explodes,
	"Explosion2/Explosion",
	"Explosion",
	eDamageType::PlayerMissile
};
static ProjectileDef g_enemyBulletDef1 = {
	{ 0.1f, 0.1f },
	0.5f,
	0.4f,
	2.0f * 60.0f,
	1,
	"ShotBeam/Idle",
	CL_EnemyBullet,
	CL_Player,
	DF_Enemy | DF_Beam,
	"ShotBeamHitEffect/Idle",
	"WallHit2",
	eDamageType::EnemyBullet
};

//--- FIRE DEFS ---//

static FireDef g_pirateFireOne{
	1,
	0.5f,
	Vec2f{ -6.0f, 8.0f },
	1
};
static FireDef g_pirateFireTwo{
	2,
	0.25f,
	Vec2f{ -6.0f, 8.0f },
	2
};
static FireDef g_pirateFireThree{
	3,
	0.25f,
	Vec2f{ -6.0f, 8.0f },
	3
};
static FireDef g_bossHomingFire{
	1,
	4.0f,
	Vec2f::Zero,
	0,
	true
};
static FireDef g_turretFireSniper{
	1,
	2.0f,
	Vec2f::Zero,
	0,
	false,
	"Turret/Fire",
	"Turret/FireLit",
	"Turret/Idle",
	"Turret/IdleLit"
};
static FireDef g_turretFire{
	2,
	2.0f,
	Vec2f::Zero,
	0,
	false,
	"Turret/Fire",
	"Turret/FireLit",
	"Turret/Idle",
	"Turret/IdleLit"
};
