#pragma once

#include "GameActor.h"

class Player;
class CreatureSpawner;
class ColliderComponent_Box;

namespace PickupFuncs {
	void OnSmallEnergyPickup();
	void OnLargeEnergyPickup();
	void OnMissilePickup();
	void OnSuperMissilePickup();
	void OnGrenadePickup();
	void OnBallPickup();
	void OnBombPickup();
	void OnEnergyTankPickup();
	void OnMissileTankPickup();
	void OnLongBeamPickup();
	void OnIceBeamPickup();
	void OnWaveBeamPickup();
	void OnGravityBootsPickup();
	void OnGrenadeTankPickup();
	void OnGrapplePickup();
	void OnChargePickup();
	void OnWallJumpPickup();
	std::function<void()> OnEnergyPickup(int32_t in_payload);
}

enum class ePickupSize {
	Small,
	Large,
	LargeShell,
};

struct PickupDef {
	ePickupSize size = ePickupSize::Small;
	std::string animName = "Util/Blank";
	std::function<void()> pickupFunc;
	std::optional<int32_t> singleFrame;
	std::optional<std::pair<std::wstring, std::wstring>> itemTip;
};

// Item that the player can pick up to change their PlayerStatus data (e.g. unlock an ability, or increase health or stock of ammo)
class Pickup : public GameActor {
public:
	Pickup() {}
	Pickup(int32_t in_objId, const PickupDef& in_pickupDef)
		: m_objId(in_objId)
		, m_pickupDef(std::make_shared<PickupDef>(in_pickupDef))
	{
	}
	Pickup(std::shared_ptr<CreatureSpawner> in_instigator, int32_t in_objId, const PickupDef& in_pickupDef)
		: m_instigator(in_instigator)
		, m_objId(in_objId)
		, m_pickupDef(std::make_shared<PickupDef>(in_pickupDef))
	{
	}
	virtual void Initialize() override;
	virtual void Destroy() override;
	virtual Task<> ManageActor() override;
	virtual float GetLifetime() const { return m_lifetime; }
	bool IsPickedUp() const;
	void PickUp();
	void SetupAnim(std::string in_animName, int32_t in_startFrame = 0, float in_playRate = 1.0f, const std::string& in_palette = "Base");
	void SetupShell();
	virtual void OnTouchPlayer(std::shared_ptr<Player> in_other);
	void OnTouchWeapon();

protected:
	std::shared_ptr<SpriteComponent> m_sprite;
	std::string m_palette = "Base";
	float m_radius = 7.0f;
	float m_lifetime = 6.33f;
	int32_t m_objId = 0;
	bool m_bItemFanfare = false;
	bool m_bHasShell = false;
	bool m_bShellJustHit = false;
	std::shared_ptr<ColliderComponent_Box> m_collider;
	std::shared_ptr<PickupDef> m_pickupDef;

private:
	std::shared_ptr<SensorComponent> m_hitSensor;
	std::shared_ptr<CreatureSpawner> m_instigator;
};
