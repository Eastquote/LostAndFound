#include "Pickup.h"

#include "GameActor.h"
#include "GameWorld.h"
#include "GameEnums.h"
#include "Player.h"
#include "PlayerStatus.h"
#include "Projectile.h"
#include "Bomb.h"
#include "CreatureSpawner.h"
#include "AudioManager.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ColliderComponent.h"
#include "Engine/Game.h"

namespace PickupFuncs {
	std::function<void()> OnEnergyPickup(int32_t in_payload) {
		return[in_payload] {
			AudioManager::Get()->PlaySound("EnergyPickup");
			GameWorld::Get()->GetPlayerStatus()->AddEnergy(in_payload);
		};
	}
	void OnSmallEnergyPickup() {
		AudioManager::Get()->PlaySound("EnergyPickup", 0.0f, 0.5f);
		GameWorld::Get()->GetPlayerStatus()->AddEnergy(5);
	}
	void OnLargeEnergyPickup() {
		AudioManager::Get()->PlaySound("EnergyPickupLg", 0.0f, 0.5f);
		GameWorld::Get()->GetPlayerStatus()->AddEnergy(20);
	}
	void OnMissilePickup() {
		AudioManager::Get()->PlaySound("MissilePickup", 0.0f, 0.5f);
		GameWorld::Get()->GetPlayerStatus()->AddMissile();
	}
	void OnSuperMissilePickup() {
		GameWorld::Get()->GetPlayerStatus()->AddSuperMissile();
	}
	void OnGrenadePickup() {
		AudioManager::Get()->PlaySound("MissilePickup", 0.0f, 0.5f);
		GameWorld::Get()->GetPlayerStatus()->AddGrenade();
	}
	void OnBallPickup() {
		GameWorld::Get()->GetPlayerStatus()->UnlockBall();
	}
	void OnBombPickup() {
		GameWorld::Get()->GetPlayerStatus()->UnlockBomb();
	}
	void OnEnergyTankPickup() {
		GameWorld::Get()->GetPlayerStatus()->AddEnergyTank();
	}
	void OnMissileTankPickup() {
		GameWorld::Get()->GetPlayerStatus()->AddMissileTank();
	}
	void OnLongBeamPickup() {
		GameWorld::Get()->GetPlayerStatus()->UnlockLongBeam();
	}
	void OnIceBeamPickup() {
		GameWorld::Get()->GetPlayerStatus()->UnlockIceBeam();
	}
	void OnWaveBeamPickup() {
		GameWorld::Get()->GetPlayerStatus()->UnlockWaveBeam();
	}
	void OnGravityBootsPickup() {
		GameWorld::Get()->GetPlayerStatus()->UnlockHighJump();
	}
	void OnGrenadeTankPickup() {
		GameWorld::Get()->GetPlayerStatus()->UnlockGrenade();
	}
	void OnGrapplePickup() {
		GameWorld::Get()->GetPlayerStatus()->UnlockGrapple();
	}
	void OnChargePickup() {
		GameWorld::Get()->GetPlayerStatus()->UnlockCharge();
	}
	void OnWallJumpPickup() {
		GameWorld::Get()->GetPlayerStatus()->UnlockWallJump();
	}
}

//--- PICKUP CODE ---//

void Pickup::Initialize() {
	GameActor::Initialize();
	m_sprite = MakeSprite(Transform::Identity);
	m_sprite->SetRenderLayer("hud");
	if(m_pickupDef->size == ePickupSize::Small) {
		m_radius = 5.0f;
		m_lifetime = 6.33f;
		m_bHasShell = false;
		m_bItemFanfare = false;
		SetupAnim(m_pickupDef->animName);
	}
	else if(m_pickupDef->size == ePickupSize::Large) {
		m_radius = 7.0f;
		m_lifetime = -1.0f;
		m_bHasShell = false;
		m_bItemFanfare = true;
		SetupAnim(m_pickupDef->animName, m_pickupDef->singleFrame.value(), 1.0f);
	}
	else if(m_pickupDef->size == ePickupSize::LargeShell) {
		m_radius = 7.0f;
		m_lifetime = -1.0f;
		m_bHasShell = true;
		m_bItemFanfare = true;
		SetupAnim(m_pickupDef->animName, m_pickupDef->singleFrame.value(), 0.0f);
	}
	if(m_bHasShell) {
		SetupShell();
	}
	if(m_instigator) {
		m_instigator->ChangeAliveCount(1);
	}
	if(IsPickedUp()) {
		DeferredDestroy();
	}
	Circle projectileCollisionCircle = { m_radius };
	SensorShape projectileSensorShape;
	projectileSensorShape.SetCircle(projectileCollisionCircle);
	auto projectileSensor = MakeSensor(Transform::Identity, projectileSensorShape);
	projectileSensor->SetFiltering(CL_Pickup, CL_Player | CL_PlayerBullet | CL_PlayerBomb);
	projectileSensor->SetTouchCallback([this](bool in_beginning, std::shared_ptr<SensorComponent> in_other) {
		if(in_beginning) {
			if(std::dynamic_pointer_cast<Player>(in_other->GetActor())) {
				OnTouchPlayer(std::dynamic_pointer_cast<Player>(in_other->GetActor()));
			}
			else if(m_bHasShell) {
				if(std::dynamic_pointer_cast<Projectile>(in_other->GetActor())) {
					Pickup::OnTouchWeapon();
				}
				else if(std::dynamic_pointer_cast<Bomb>(in_other->GetActor())) {
					Pickup::OnTouchWeapon();
				}
			}
		}
	});
}
void Pickup::OnTouchPlayer(std::shared_ptr<Player> in_other) {
	if(m_bHasShell && in_other->GetDashingCharged()) {
		m_bShellJustHit = true;
	}
	else if(!m_bHasShell) {
		PickUp();
	}
}
void Pickup::OnTouchWeapon() {
	m_bShellJustHit = true;
}
void Pickup::Destroy() {
	if(m_instigator) {
		m_instigator->ChangeAliveCount(-1);
	}
	GameActor::Destroy();
}
Task<> Pickup::ManageActor() {
	auto elapsedTime = 0.0f;
	while(true) {
		if(m_bHasShell && m_bShellJustHit) {

			// Play exploding-sphere anim & clear the collider
			m_sprite->PlayAnim("ItemSphere/Sphere", false, 1);
			m_sprite->SetPlayRate(1.0);
			m_collider->Destroy();
			co_await WaitSeconds(0.066667f);

			// Play proper item anim
			SetupAnim(m_pickupDef->animName, m_pickupDef->singleFrame.value(), 0.0f);
			m_bHasShell = false;
			m_bShellJustHit = false;
		}
		if(GetLifetime() != -1.0f && elapsedTime > GetLifetime()) {
			DeferredDestroy();
		}
		co_await Suspend();
		elapsedTime += DT();
	}
}
bool Pickup::IsPickedUp() const {
	if(m_objId == 0) {
		return false;
	}
	auto playerStatus = GameWorld::Get()->GetPlayerStatus();
	return playerStatus->IsObjectUnlocked(m_objId);
}
void Pickup::PickUp() {
	if(m_bItemFanfare) {
		if(m_pickupDef->itemTip.has_value()){
			GetWorld()->OnItemPickup(m_pickupDef->itemTip.value());
		}
		else {
			GetWorld()->OnItemPickup(std::nullopt);
		}
	}
	m_pickupDef->pickupFunc();
	if(m_objId) {
		GetWorld()->GetPlayerStatus()->UnlockObject(m_objId);
	}
	DeferredDestroy();
}
void Pickup::SetupAnim(std::string in_animName, int32_t in_startFrame, float in_playRate, const std::string& in_palette) {
	m_palette = in_palette;
	m_sprite->PlayAnim(in_animName, true, in_startFrame);
	m_sprite->SetPlayRate(in_playRate);
}
void Pickup::SetupShell() {
	m_sprite->PlayAnim("ItemSphere/Sphere", false, 0);
	m_sprite->SetPlayRate(0.0f);
	m_collider = MakeCollider_Box(AsShared(), Transform::Identity, GetWorld()->GetCollisionWorld(), Box2f::FromCenter(Vec2f::Zero, {14.0f, 14.0f}));
}
