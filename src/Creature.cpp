#include "Creature.h"

#include "GameWorld.h"
#include "GameLoop.h"
#include "CreatureSpawner.h"
#include "Player.h"
#include "PlayerStatus.h"
#include "Projectile.h"
#include "Projectiles/HomingMissile.h"
#include "ParticleSpawner.h"
#include "ParticleSpawnerDefs.h"
#include "ParticleManager.h"
#include "Effect.h"
#include "DropManager.h"
#include "AudioManager.h"
#include "Light.h"
#include "CameraManager.h"
#include "FunctionGuard.h"
#include "Algorithms.h"
#include "SquirrelNoise5.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ColliderComponent.h"
#include "Engine/MathRandom.h"
#include "Engine/MathGeometry.h"
#include "Engine/Curve.h"
#include "Engine/EaseTo.h"
#include "Engine/DebugDrawSystem.h"

//-- CREATURE BASE CLASS CODE --//

Creature::Creature(std::shared_ptr<CreatureSpawner> in_spawner)
	: m_spawner(in_spawner)
{
}
Creature::Creature(std::shared_ptr<CreatureSpawner> in_spawner, bool in_bisMovingRight)
	: m_spawner(in_spawner)
{
}
void Creature::Initialize() {
	Character::Initialize();
	m_spawner->ChangeAliveCount(1);
	SetupComponents();
	m_manageAITask = ManageAI();
	m_checkHealthTask = CheckHealth();
}
void Creature::Update() {
	++m_noiseIndex;
	if(!GetWorld()->CreatureIsOffCamera(AsShared<Creature>()) || m_bUpdatesOffScreen) {
		Character::Update();
		if(ShouldUpdateAI()) {
			m_manageAITask.Resume(); //< We KNOW this will be resumed exactly once per frame (that's how often Update() is called)
		}
		m_checkHealthTask.Resume();
	}
}
void Creature::Destroy() {
	m_checkHealthTask = nullptr;
	m_manageAITask = nullptr;
	m_spawner->ChangeAliveCount(-1);
	Character::Destroy();
}


Task<> Creature::ManageActor() {
	auto checkHealthTask = m_taskMgr.Run(CheckHealth());
	auto aiTask = ManageAI(); //< Injecting this in here is how we (effectively) make "ManageActor" polymorphic
	auto isInvincible = m_health == -1;
	auto basePlayRate = m_sprite->GetPlayRate();
	auto phaseDuration = m_phaseDuration;
	auto frame = 0;
	while(true) {
		if(!isInvincible) {
			if(m_health <= 0) {
				co_await PreDeath();
				ExplodeAndDie();
				co_return; //< This is the end of this character -- cleans up, is gone
			}
		}
		if(m_bJustPhased && m_phaseDuration == 0.0f) {
			phaseDuration = m_phaseDuration;
			m_bJustPhased = false;
		}
		if(m_phaseDuration > 0.0f) {
			if((frame % 2) == 0) {
				m_sprite->SetHidden(true);
			}
			else {
				m_sprite->SetHidden(false);
			}
			m_phaseDuration -= DT();
			frame += 1;
		}
		else {
			m_phaseDuration = 0.0f;
			m_sprite->SetHidden(false);
			frame = 0;
		}
		if(m_moveSpeedModifier < 1.0f) {
			if(m_moveSpeedModifier < 0.33f) {
				ToggleFrozen();
			}
			else {
				m_sprite->SetPalette("Chill");
				m_sprite->SetPlayRate(m_moveSpeedModifier);
				m_moveSpeedModifier += 0.001f;
				if(m_moveSpeedModifier >= 0.9f) {
					m_moveSpeedModifier = 1.0f;
					m_sprite->SetPlayRate(basePlayRate);
					m_sprite->SetPalette("Base");
				}
			}
		}
		if(m_bHit) {
			auto lastPalette = m_sprite->GetPalette();
			if(!m_bIsFrozen && !isInvincible) {
				m_sprite->SetPalette("Ice");
				co_await WaitSeconds(0.033f);
				m_sprite->SetPalette(lastPalette);
				m_bHit = false;
				if(!m_armor && m_hitPause) {
					co_await WaitSeconds(m_hitPause).CancelIf([this] { return m_bHit; });
				}
			}
			else if(m_bIsFrozen) {
				auto prevPlayRate = m_sprite->GetPlayRate();
				m_sprite->SetPlayRate(0.0);
				m_sprite->SetPalette("Ice");

				// If frozen while overlapping player, don't make world collider until AFTER overlap ends
				while(Math::IntersectBoxes(GetCollisionBoxWorld(), GetWorld()->GetPlayerWorldCollisionBox()).has_value()) {
					//printf("Frozen Creature / Player overlap!\n");
					co_await Suspend();
				}
				auto collider = MakeCollider_Box(AsShared(), Transform::Identity, GetWorld()->GetCollisionWorld(), GetCollisionBoxLocal());
				co_await WaitSeconds(m_freezeDuration).CancelIf([this] { return !m_bIsFrozen; }); //< 343 frames / 60 fps

				// Flash to warn it's about to wake up
				auto timer = 0.0f;
				auto frameCounter = 0;
				auto lastPalette = "Ice";
				while(timer < 1.5f && m_bIsFrozen) {
					if(frameCounter % 5 == 0) {
						auto newPalette = (lastPalette == "Ice") ? "Base" : "Ice";
						m_sprite->SetPalette(newPalette);
						lastPalette = newPalette;
					}
					co_await Suspend();
					timer += DT();
					frameCounter++;
					//printf("frameCounter = %d\n", frameCounter);
				}

				// Wake up!
				collider->Destroy();
				m_bHit = false;
				m_sprite->SetPlayRate(basePlayRate);
				m_sprite->SetPalette("Base");
				m_moveSpeedModifier = 1.0f;
				m_bIsFrozen = false;
			}
		}

		// Resume normal AI behavior
		auto updateAIToken = m_updateAITokens.TakeToken(__FUNCTION__);
		co_await Suspend();
	}
	co_await WaitForever();
}


void Creature::ChangeHealth(int32_t in_change, bool hitArmor) {
	auto health = GetHealth();
	if(hitArmor) {
		auto armor = GetArmor();
		SetArmor(armor + in_change >= 0 ? armor += in_change : 0);
	}
	else if(health != -1) {
		SetHealth(health + in_change >= 0 ? health += in_change : 0);
	}
}
void Creature::SetupComponents() {
	SetHitSensorBox(m_initialCollisionDims);
	m_worldCollisionBoxLocal = Box2f::FromCenter(Vec2f::Zero, m_initialCollisionDims);
	m_worldCollisionBoxLocalLedge = Box2f::FromCenter(Vec2f{ 0.0f, 0.0f }, { 8.0f, 8.0f });

	m_hitSensor->SetFiltering(CL_Enemy, CL_PlayerBullet | CL_PlayerBomb | CL_Player | CL_Door);
	m_hitSensor->SetTouchCallback([this](bool in_beginning, std::shared_ptr<SensorComponent> in_other) {
		if(in_beginning && std::dynamic_pointer_cast<GameActor>(in_other->GetActor())) {
			TouchCallback(std::dynamic_pointer_cast<GameActor>(in_other->GetActor()));
		}
		else {
			UntouchCallback();
		}
		});
	SetDamageInfo(1, DF_Enemy);
	m_sprite = MakeSprite(Transform::Identity);
	m_spriteLit = MakeSprite();
	m_spriteLit->SetRenderLayer("lights");
	m_bUpdatesOffScreen = false;
}
void Creature::SetHitSensorBox(Vec2f in_dims) {
	auto sensorMgr = GetWorld()->GetSensorManager();
	ClearDefaultSensor();
	Vec2f creatureCollisionBox{ in_dims };
	SensorShape creatureSensorShape;
	creatureSensorShape.SetBox(creatureCollisionBox);
	m_hitSensor = MakeSensor(Transform::Identity, creatureSensorShape);
}
eHitResponse Creature::HandleDamage(DamageInfo in_dmgInfo, std::shared_ptr<SensorComponent> in_sensor) {
	std::optional<DamageInfo> damage = {};
	// If they're armored, hit the armor instead of health
	if((in_sensor->GetFiltering().x & eCollisionLayer::CL_Armor)) {
		if(m_armor) {
			damage = ModifyDamage(in_dmgInfo, in_sensor, true);
			if((in_dmgInfo.m_damageFlags & DF_Ice)) {
				if((in_dmgInfo.m_damageFlags & DF_Charged)) {
					ToggleFrozen();
				}
				else {
					AddChill();
				}
			}
			if(damage.has_value()) {
				SetHit(true);
				AudioManager::Get()->PlaySound("CreatureHit"); //< TODO: play a CreatureBlock sound instead
				ChangeHealth(-damage.value().m_payload, true);
				return eHitResponse::Blocked;
			}
		}
		else {
			return eHitResponse::Ignore;
		}
	}
	else if (!(in_sensor->GetFiltering().x & eCollisionLayer::CL_Armor)) {
		damage = ModifyDamage(in_dmgInfo, in_sensor, false);
		if(damage.has_value()) {
			SetHit(true);
			AudioManager::Get()->PlaySound("CreatureHit2");
			ChangeHealth(-damage.value().m_payload, false);
			SetLastDamageType(damage->m_damageFlags);
			if((in_dmgInfo.m_damageFlags & DF_Ice)) {
				if((in_dmgInfo.m_damageFlags & DF_Charged)) {
					ToggleFrozen();
				}
				else {
					AddChill();
				}
			}
			return eHitResponse::Hit;
		}
	}
	return eHitResponse::Ignore;
}
std::optional<DamageInfo> Creature::ModifyDamage(DamageInfo in_dmgInfo, std::shared_ptr<SensorComponent> in_sensor, bool in_hitArmor) {
	auto out_dmgInfo = in_dmgInfo;
	if(m_phaseDuration > 0.0f) { // Ignore if creature is "phased" (visible but untouchable)
		return std::nullopt;
	}
	else if(in_hitArmor) {
		if((in_dmgInfo.m_damageFlags & DF_Dash) || 
			((in_dmgInfo.m_damageFlags & DF_Beam) && (in_dmgInfo.m_damageFlags & DF_Charged))) {
			out_dmgInfo.m_payload = 1;
			StartPhasedWindow(1.0f);
		}
		else if(in_dmgInfo.m_damageFlags & DF_Ice) {
			out_dmgInfo.m_payload = 0;
		}
		else if(!(in_dmgInfo.m_damageFlags & DF_Charged)) {
			out_dmgInfo.m_payload = 1; //< Hit by normal, not-charged projectile
		}
		else if(in_dmgInfo.m_payload > 1) {
			out_dmgInfo.m_payload = 2; //< Hit by charged projectile with greater-than-one payload
		}
		else {
			out_dmgInfo.m_payload = 0;
		}
	}
	else if(((in_dmgInfo.m_damageFlags & DF_Explodes) || 
		(in_dmgInfo.m_damageFlags & DF_Dash)) && m_bIsFrozen) { //< Give bonus damage to missile/dash hits on frozen creatures
		out_dmgInfo.m_payload += 2;
	}
	return out_dmgInfo; //< Returns modified data, but also handles unmodified case
}
void Creature::ToggleFrozen(bool in_charged) {
	m_freezeDuration = in_charged ? 8.31667f : 4.31667f;
	m_bIsFrozen = !m_bIsFrozen; 
}
void Creature::AddChill() {
	m_moveSpeedModifier = std::clamp(m_moveSpeedModifier - 0.33f, 0.25f, 1.0f);
}

Task<> Creature::Freeze() {
	while(m_bIsFrozen) {
		co_await WaitSeconds(3.0f);
	}
}
Task<> Creature::ManageAI() {
	// Overridden in child classes
	co_return;
}
Task<> Creature::PreDeath() {
	// Overridden in child classes
	co_return;
}
Vec2f Creature::GetCollisionSize() const {
	return { 16.0f, 16.0f };
}
void Creature::AddCollisionBox(Vec2f in_relativePos, Vec2f in_dims, std::string in_name) {
	Vec2f creatureCollisionBox{ in_dims };
	SensorShape creatureSensorShape;
	creatureSensorShape.SetBox(creatureCollisionBox);
	m_hitSensors.insert({ in_name, MakeSensor({ in_relativePos }, creatureSensorShape) });
}
std::shared_ptr<SensorComponent>  Creature::GetCollisionBox(std::string name) {
	return m_hitSensors.find(name)->second;
}
Task<> Creature::CheckHealth() {
	if(m_health == -1) {
		co_return;
	}
	while (true) {
		if (m_health <= 0) {
			break;
		}
		co_await Suspend();
	}
}
void Creature::ExplodeAndDie() {
	if(((m_lastDamageType & DF_Beam) || (m_lastDamageType & DF_Dash)) && !(m_lastDamageType & DF_Ice) || (m_lastDamageType & DF_Explodes)) {
		auto effect = Actor::Spawn<Effect>(GetWorld(), GetWorldTransform(), "Explosion2/Explosion");
		AudioManager::Get()->PlaySound("Explosion");
	}
	if(m_bCanSpawnPickup && GetWorld()->GetDropManager()->TrySpawning(AsShared<Creature>())) {
		DeferredDestroy();
	}
	else {
		DeferredDestroy();
	}	
}
void Creature::FlipDir() {
	m_bIsMovingRight = !m_bIsMovingRight;
	m_bIsFacingRight = !m_bIsFacingRight;
	m_moveSpeed *= -1.0f;
	m_bJustFlipped = true;
}
float Creature::SetDir(float in_dir) {
	auto dir = 1.0f;
	auto prevSpeed = m_moveSpeed;
	if(in_dir == 0.0f) {
		// Roll random dir if user supplies "0.0f" as in_dir
		dir = std::round(Get1dNoiseZeroToOne(m_noiseIndex, m_spawnIndex)) == 1 ? 1.0f : -1.0f;
	}
	else {
		dir = in_dir > 0.0f ? 1.0f : -1.0f;
	}
	m_moveSpeed = std::abs(m_moveSpeed) * dir;
	if(m_moveSpeed != prevSpeed) {
		m_bIsMovingRight = !m_bIsMovingRight;
		m_bIsFacingRight = !m_bIsFacingRight;
		m_bJustFlipped = true;
	}
	return dir;
}
bool Creature::CheckTrigger(Box2f in_trigger){
	auto playerPos = GetWorld()->GetPlayerWorldPos();
	auto triggerWorld = in_trigger.TransformedBy(GetWorldTransform());
	//DrawDebugBox(triggerWorld, sf::Color::Green);
	return Math::PointInBox(triggerWorld, playerPos);
}
Vec2f Creature::SpawnSurfaceCheck(bool in_bDownOnly, bool in_bUpOnly, bool in_bSidesOnly, bool in_bMoveToFlush) {
	Vec2f resultNormal = Vec2f::Up;
	CardinalUpdate(m_blockCardinals, 255.0f);
	if(in_bDownOnly) {
		resultNormal = Vec2f::Up;
	}
	else if(in_bUpOnly) {
		resultNormal = Vec2f::Down;
	}
	else if(in_bSidesOnly) {
		auto leftDist = m_blockCardinals.cardinals[eDir::Left];
		auto rightDist = m_blockCardinals.cardinals[eDir::Right];
		if(leftDist < rightDist) {
			resultNormal = Vec2f::Right;
		}
		else { //< Do this when distances are equal, as well
			resultNormal = Vec2f::Left;
		}
	}
	else { //< Select the smallest of the four cardinals, return normal of surface it hits
		auto minCardinal = m_blockCardinals.Min();
		if(minCardinal == eDir::Up) {
			resultNormal = Vec2f::Down;
		}
		else if(minCardinal == eDir::Down) {
			resultNormal = Vec2f::Up;
		}
		else if(minCardinal == eDir::Left) {
			resultNormal = Vec2f::Right;
		}
		else {
			resultNormal = Vec2f::Left;
		}
	}
	// Debug draw cardinal distances
	//DrawDebugPoint(GetWorldPos() + Vec2f{ 0.0f, m_blockCardinals.UpDist() });
	//DrawDebugPoint(GetWorldPos() + Vec2f{ 0.0f, -m_blockCardinals.DownDist() });
	//DrawDebugPoint(GetWorldPos() + Vec2f{ m_blockCardinals.RightDist(), 0.0f });
	//DrawDebugPoint(GetWorldPos() + Vec2f{ -m_blockCardinals.LeftDist(), 0.0f });
	if(in_bMoveToFlush) {
		if(resultNormal == Vec2f::Down) {
			Move(GetWorldPos() + Vec2f{ 0.0f, m_blockCardinals.UpDist() }, true);
		}
		else if(resultNormal == Vec2f::Up) {
			Move(GetWorldPos() + Vec2f{ 0.0f, -m_blockCardinals.DownDist() }, true);
		}
		else if(resultNormal == Vec2f::Right) {
			Move(GetWorldPos() + Vec2f{ -m_blockCardinals.LeftDist(), 0.0f }, true);
		}
		else if(resultNormal == Vec2f::Left) {
			Move(GetWorldPos() + Vec2f{ m_blockCardinals.RightDist(), 0.0f }, true);
		}
	}
	return resultNormal;
}
void Creature::TouchCallback(std::shared_ptr<GameActor> in_other) {
	// Debug statements
	//printf("Creature touch callback!\n");
	//std::cout << "m_lastDamage type = " << int(m_lastDamageType) << std::endl;
}
void Creature::UntouchCallback(){
	//printf("Creature UNtouch callback!\n");
}
float Creature::AtLedge(Vec2f in_upDir, Vec2f in_moveDir) {
	Cardinals ledgeCheckCardinals;
	auto offset = Vec2f::Zero;
	bool vertical;
	Box2f ledgeBox;

	// Offset ledgeBox properly based on direction
	if(abs(in_moveDir.x) == 1.0f) { //< Check if it's moving horizontally
		ledgeBox = Box2f::FromCenter(Vec2f::Zero, { 8.0f, GetCollisionBoxLocal().h });
		offset = Vec2f{ (GetCollisionBoxLocal().GetRight() + (ledgeBox.w / 2)) * in_moveDir.x, 0.0f };
		vertical = false;
	}
	else if(abs(in_moveDir.y) == 1.0f) {
		ledgeBox = Box2f::FromCenter(Vec2f::Zero, { m_worldCollisionBoxLocal.w, 8.0f });
		offset = Vec2f{ 0.0f, (GetCollisionBoxLocal().GetTop() + (ledgeBox.h / 2)) * in_moveDir.y };
		vertical = true;
	}
	else {
		printf("AtLedge() ERROR: in_moveDir wasn't a unit vector!");
		return 0;
	}

	CardinalUpdate(ledgeCheckCardinals, 8.0f, { offset }, ledgeBox);
	
	// Return correct ledge-check results based on direction & orientation...
	if(vertical) {
		if(in_upDir == Vec2f::Right && !ledgeCheckCardinals.Left() ||
			in_upDir == Vec2f::Left && !ledgeCheckCardinals.Right()) {
			return in_moveDir.y;
		}
	}
	else {
		if(in_upDir == Vec2f::Up && !ledgeCheckCardinals.Down() ||
			in_upDir == Vec2f::Down && !ledgeCheckCardinals.Up()) {
			return in_moveDir.x;
		}
	}
	// ...or, failing that, return nothing
	return 0.0f;
}
bool Creature::ShouldChunksplode() const {
	return ((m_lastDamageType & DF_Explodes) || (m_lastDamageType & DF_Dash)) 
		   || m_bIsFrozen;
}
void Creature::StartPhasedWindow(float in_duration) {
	m_bJustPhased = true;
	m_phaseDuration = in_duration;
}
float Creature::GetPlayerDir() {
	auto playerDist = DistanceToPlayer();
	return playerDist.x > 0.0f ? 1.0f : -1.0f;
}
void Creature::ChangeProjectileCount(int32_t in_change) {
	m_liveProjectiles = m_liveProjectiles + in_change < 0 ? 0 : m_liveProjectiles + in_change;
}
Task<> Creature::PlayFireAnim(std::string in_fireAnim, std::string in_fireAnimLit, std::string in_resumeAnim, std::string in_resumeAnimLit) {
	if(in_fireAnim.size()) {
		auto resumeAnimGuard = MakeFnGuard([this, in_resumeAnim, in_resumeAnimLit] {
			m_sprite->PlayAnim(in_resumeAnim, true);
			m_spriteLit->PlayAnim(in_resumeAnimLit, true);
		});
		co_await WaitForAll({
			m_sprite->PlayAnim(in_fireAnim, false),
			m_spriteLit->PlayAnim(in_fireAnimLit, false),
		});
	}
}
Task<> Creature::TryFireBullet(	const ProjectileDef& in_projectileDef, std::optional<Vec2f> in_vec, const FireDef& in_fireDef)
{
	auto maxLive = in_fireDef.maxLive;
	auto fireTimer = 0.0f;
	auto world = GetWorld();
	auto isSniper = in_vec.has_value() ? false : true;
	auto bulletsFired = 0;
	auto fireAnimAvail = false;
	TaskHandle<> fireAnim;
	while(true) {
		if(m_bCanFire && m_liveProjectiles < maxLive && fireTimer <= 0.0f) {
			fireAnim = m_taskMgr.Run(PlayFireAnim(	in_fireDef.fireAnim, in_fireDef.fireAnimLit, 
													in_fireDef.resumeAnim, in_fireDef.resumeAnimLit));
			auto targetDir = isSniper ? (world->GetPlayerWorldPos(true) - GetWorldPos()).Norm() : in_vec.value();
			Transform bulletTransform = { GetWorldPos() + (in_fireDef.posOffset * Vec2f{targetDir.x, 1.0f}), 0.0f, Vec2f::One };
			auto bullet = SpawnProjectile(in_projectileDef, bulletTransform, targetDir);
			if(in_fireDef.bIsHoming) {
				if(auto homingMissile = std::dynamic_pointer_cast<HomingMissile>(bullet)) {
					SQUID_RUNTIME_CHECK(homingMissile != nullptr, "MissileDef must return a child class of HomingMissile");
					homingMissile->SetTarget(GetWorld()->GetPlayer());
				}
			}
			ChangeProjectileCount(1);
			++bulletsFired;
			if(in_fireDef.maxToFire && bulletsFired >= in_fireDef.maxToFire) {
				break;
			}
			fireTimer = in_fireDef.period;
		}
		fireTimer -= DT();
		co_await Suspend();
	}
}
Task<> Creature::TryWalkDistance(float in_distance) {
	//printf("Trying to walk fwd!\n");
	auto dir = std::copysignf(1.0f, in_distance);
	auto moveDuration = (abs(in_distance) / abs(m_moveSpeed * DT())) / 60.0f;
	auto elapsedTime = 0.0f;
	while(elapsedTime < moveDuration) {
		Move(GetWorldPos() + Vec2f{ dir * abs(m_moveSpeed * DT()), 0.0f });
		if(!m_blockCardinals.Down()) {
			break;
		}
		elapsedTime += DT();
		co_await Suspend();
	}
	co_return;
}
Task<> Creature::TryFall(float in_maxFallSpeedY, float in_gravity) {
	//printf("Trying to fall!\n");
	auto fallSpeed = 0.0f;
	auto prevPos = GetWorldPos();
	while(!m_blockCardinals.Down()) {
		fallSpeed = std::clamp(fallSpeed + in_gravity, 0.0f, in_maxFallSpeedY);
		Move(GetWorldPos() + Vec2f{ 0.0f, -fallSpeed * DT() });
		if(prevPos == GetWorldPos()) {
			break;
		}
		prevPos = GetWorldPos();
		co_await Suspend();
	}
}
Task<> Creature::Hop(float in_launchSpeedY, float in_maxFallSpeedY, float* in_timer, float in_gravity ) {
	auto ySpeed = in_launchSpeedY;
	auto duration = 0.0f;
	while(true) {
		duration += DT();
		auto dir = std::copysign(1.0f, m_moveSpeed);
		auto absMoveSpeed = abs(m_moveSpeed);
		// Diagonal motion is broken into separate X- and Y-axis moves to enable "sliding" on blocking surfaces
		Move({(GetWorldPos().x + (abs(m_moveSpeed * DT()) + (in_launchSpeedY * DT() / 4.0f)) * dir), GetWorldPos().y});
		Move({GetWorldPos().x, GetWorldPos().y + ySpeed * DT() });
		if(m_blockCardinals.Up()) {
			ySpeed = 0.0f;
		}
		if(m_blockCardinals.Down()) {
			break;
		}
		ySpeed -= in_gravity;
		co_await Suspend();
	}
	if(in_timer) {
		in_timer = &duration;
	}
	co_return;
}
void Creature::ClearDefaultSensor() {
	if(IsAlive(m_hitSensor)) {
		m_hitSensor->Destroy();
		//printf("Removed default sensor!\n");
	}
}
