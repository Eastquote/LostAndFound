#pragma once

#include "Pickup.h"

static PickupDef g_pickupSmallEnergy = {
	ePickupSize::Small,
	"EnergyPickup/Small",
	PickupFuncs::OnSmallEnergyPickup,
};
static PickupDef g_pickupLargeEnergy = {
	ePickupSize::Small,
	"EnergyPickup/Large",
	PickupFuncs::OnLargeEnergyPickup,
};
static PickupDef g_pickupMissile = {
	ePickupSize::Small,
	"MissilePickup/Small",
	PickupFuncs::OnMissilePickup,
};
static PickupDef g_pickupSuperMissile = {
	ePickupSize::Small,
	"MissilePickup/Small", //< TODO: Create custom art for this feature if it ever actually gets implemented
	PickupFuncs::OnSuperMissilePickup,
};
static PickupDef g_pickupGrenade = {
	ePickupSize::Small,
	"GrenadePickup/Small",
	PickupFuncs::OnGrenadePickup,
};
static PickupDef g_pickupBall = {
	ePickupSize::Large,
	"ItemBall/Idle",
	PickupFuncs::OnBallPickup,
	0,
	std::pair<std::wstring, std::wstring> {L"FISH BOWL", L"Tap down to transform."}
};
static PickupDef g_pickupBomb = {
	ePickupSize::LargeShell,
	"PickupsLarge/Items",
	PickupFuncs::OnBombPickup,
	1,
};
static PickupDef g_pickupEnergyTank = {
	ePickupSize::Large,
	"ItemEnergyTank/Idle",
	PickupFuncs::OnEnergyTankPickup,
	0,
	std::pair<std::wstring, std::wstring> {L"EXTRA BATTERY", L"Max energy increased by 100"}
};
static PickupDef g_pickupMissileTank = {
	ePickupSize::Large,
	"ItemMissileTank/Idle",
	PickupFuncs::OnMissileTankPickup,
	0,
	std::pair<std::wstring, std::wstring> {L"MISSILE TANK", L"Max missile count increased by 5"}
};
static PickupDef g_pickupLongBeam = {
	ePickupSize::LargeShell,
	"PickupsLarge/Items",
	PickupFuncs::OnLongBeamPickup,
	4,
};
static PickupDef g_pickupIceBeam = {
	ePickupSize::LargeShell,
	"PickupsLarge/Items",
	PickupFuncs::OnIceBeamPickup,
	5,
};
static PickupDef g_pickupWaveBeam = {
	ePickupSize::LargeShell,
	"PickupsLarge/Items",
	PickupFuncs::OnWaveBeamPickup,
	6,
};
static PickupDef g_pickupGravityBoots = {
	ePickupSize::LargeShell,
	"PickupsLarge/Items",
	PickupFuncs::OnGravityBootsPickup,
	7,
};
static PickupDef g_pickupGrenadeTank = {
	ePickupSize::Large,
	"ItemGrenadeTank/Idle",
	PickupFuncs::OnGrenadeTankPickup,
	0,
	std::pair<std::wstring, std::wstring> {L"GRENADE", L"Max grenade count increased by 5. Hold to throw further."}
};
static PickupDef g_pickupGrapple = {
	ePickupSize::LargeShell,
	"PickupsLarge/Items",
	PickupFuncs::OnGrapplePickup,
	10,
};
static PickupDef g_pickupCharge = {
	ePickupSize::Large,
	"ItemCharge/Idle",
	PickupFuncs::OnChargePickup,
	0,
	std::pair<std::wstring, std::wstring> {L"CHARGE CELL", L"Hold fire button. Has many uses..."}
};
static PickupDef g_pickupWallJump = {
	ePickupSize::LargeShell,
	"PickupsLarge/Items",
	PickupFuncs::OnWallJumpPickup,
	7,
};
