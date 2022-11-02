#pragma once

#include "GameWorld.h"
#include <cstdint>
#include <Engine/Vec2.h>
#include <set>
#include <iostream>
#include <algorithm>
#include <vector>
#define PLAYERSTATUS_VERSION 1

struct BeamStatus {
	bool iceBeam = false;
	bool waveBeam = false;
};

struct PlayerStatus {
public:
	void TryLoad();
	void Save();

	void SaveRecharge() {
		missiles = missileTanks * 5;
		energy = 99 + (energyTanks * 100);
		grenades = grenadeTanks * 5;
		isDirty = true;
	}
	void SetupNewSuit() { //< Invoked after player transitions from fish -> new suit (basically takes all your toys away)
		missileTanks = 0;
		missiles = 0;
		superMissileTanks = 0;
		superMissiles = 0;
		grenadeTanks = 0;
		grenades = 0;
		energyTanks = 0;
		energy = 30;
		ball = false;
		bomb = false;
		longBeam = true;
		beamStatus = { false, false };
		highJump = false;
		wallJump = false;
		charge = false;
		screwJump = false;
		unlockedObjs = {};
		hasNewSuit = true;
		primaryWeaponUnlocks = { 0 };
		secondaryWeaponUnlocks = {};
		currentPWeapon = 0;
		currentSWeapon = 0;
	}
	bool IsNewSuitUnlocked() const {
		return hasNewSuit;
	}
	void AddMissile() {
		missiles = std::clamp(missiles + 1, 0, missileTanks * 5);
		isDirty = true;
	}
	void AddSuperMissile() {
		superMissiles = std::clamp(superMissiles + 1, 0, superMissileTanks * 5);
		isDirty = true;
	}
	void AddGrenade() {
		grenades = std::clamp(grenades + 1, 0, grenadeTanks * 5);
		isDirty = true;
	}
	int32_t GetMissileCount() const {
		return missiles;
	}
	int32_t GetSuperMissileCount() const {
		return superMissiles;
	}
	int32_t GetGrenadeCount() const {
		return grenades;
	}
	bool TryRemoveMissile() {
		if(missiles > 0) {
			missiles -= 1;
			isDirty = true;
			return true;
		}
		else {
			return false;
		}
	}
	bool TryRemoveGrenade() {
		if(grenades > 0) {
			grenades -= 1;
			isDirty = true;
			return true;
		}
		else {
			return false;
		}
	}
	void AddMissileTank() {
		if(!IsMissileUnlocked()) {
			primaryWeaponUnlocks.push_back(1);
		}
		missileTanks += 1;
		missiles = std::clamp(missiles + 5, 0, missileTanks * 5);
		isDirty = true;
	}
	int32_t GetMissileTankCount() const {
		return missileTanks;
	}
	void AddSuperMissileTank() {
		superMissileTanks += 1;
		superMissiles = std::clamp(superMissiles + 5, 0, superMissileTanks * 5);
		isDirty = true;
	}
	int32_t GetSuperMissileTankCount() const {
		return superMissileTanks;
	}
	void AddGrenadeTank() {
		grenadeTanks += 1;
		grenades = std::clamp(grenades + 5, 0, grenadeTanks * 5);
		isDirty = true;
	}
	int32_t GetGrenadeTankCount() const {
		return grenadeTanks;
	}
	void AddEnergy(int32_t in_energy) {
		energy = std::clamp(energy + in_energy, 0, (energyTanks * 100) + 99);
		isDirty = true;
	}
	int32_t GetEnergy() const {
		return energy;
	}
	void Set1Energy() {
		energy = 1;
	}
	void AddEnergyTank() {
		energyTanks = std::clamp(energyTanks + 1, 0, 6);
		auto maxEnergy = (energyTanks * 100) + 99;
		energy = maxEnergy;
		isDirty = true;
	}
	int32_t GetEnergyTankCount() const {
		return energyTanks;
	}
	void UnlockIceBeam() {
		beamStatus.iceBeam = true;
		if(!IsIceBeamUnlocked()){
			primaryWeaponUnlocks.push_back(2);
			std::sort(primaryWeaponUnlocks.begin(), primaryWeaponUnlocks.end());
		}
		currentPWeapon = 2;
		isDirty = true;
	}
	bool IsIceBeamUnlocked() const {
		return std::find(primaryWeaponUnlocks.begin(), primaryWeaponUnlocks.end(), 2) != primaryWeaponUnlocks.end();
	}
	void UnlockWaveBeam() {
		beamStatus.waveBeam = true;
		if(!IsWaveBeamUnlocked()){
			primaryWeaponUnlocks.push_back(3);
			std::sort(primaryWeaponUnlocks.begin(), primaryWeaponUnlocks.end());
		}
		currentPWeapon = 3;
		isDirty = true;
	}
	bool IsWaveBeamUnlocked() const {
		return std::find(primaryWeaponUnlocks.begin(), primaryWeaponUnlocks.end(), 3) != primaryWeaponUnlocks.end();
	}
	void UnlockGrenade() {
		if(!IsGrenadeUnlocked()){
			secondaryWeaponUnlocks.push_back(0);
			std::sort(secondaryWeaponUnlocks.begin(), secondaryWeaponUnlocks.end());
		}
		AddGrenadeTank();
		isDirty = true;
	}
	bool IsGrenadeUnlocked() const {
		return std::find(secondaryWeaponUnlocks.begin(), secondaryWeaponUnlocks.end(), 0) != secondaryWeaponUnlocks.end();
	}
	void UnlockGrapple() {
		if(!IsGrappleUnlocked()){
			secondaryWeaponUnlocks.push_back(1);
			std::sort(secondaryWeaponUnlocks.begin(), secondaryWeaponUnlocks.end());
		}
		isDirty = true;
	}
	bool IsGrappleUnlocked() const {
		return std::find(secondaryWeaponUnlocks.begin(), secondaryWeaponUnlocks.end(), 1) != secondaryWeaponUnlocks.end();
	}
	bool IsBeamUnlocked() const {
		return std::find(primaryWeaponUnlocks.begin(), primaryWeaponUnlocks.end(), 0) != primaryWeaponUnlocks.end();
	}
	bool IsMissileUnlocked() const {
		return std::find(primaryWeaponUnlocks.begin(), primaryWeaponUnlocks.end(), 1) != primaryWeaponUnlocks.end();
	}
	void UnlockBall() {
		ball = true;
		isDirty = true;
	}
	bool IsBallUnlocked() const {
		return ball;
	}
	void UnlockBomb() {
		bomb = true;
		isDirty = true;
	}
	bool IsBombUnlocked() const {
		return bomb;
	}
	void UnlockLongBeam() {
		longBeam = true;
		isDirty = true;
	}
	bool IsLongBeamUnlocked() const {
		return longBeam;
	}
	void UnlockHighJump() {
		GameWorld::Get()->SetPlayerGravityModifier(0.66667f);
		highJump = true;
		isDirty = true;
	}
	bool IsHighJumpUnlocked() const {
		return highJump;
	}
	void UnlockWallJump() {
		wallJump = true;
		isDirty = true;
	}
	bool IsWallJumpUnlocked() const {
		return wallJump;
	}
	void UnlockCharge() {
		charge = true;
		isDirty = true;
	}
	bool IsChargeUnlocked() const {
		return charge;
	}

	bool IsBeamSelected() const {
		return currentPWeapon == 0;
	}
	bool IsMissileSelected() const {
		return currentPWeapon == 1;
	}
	bool IsIceBeamSelected() const {
		return currentPWeapon == 2;
	}
	bool IsWaveBeamSelected() const {
		return currentPWeapon == 3;
	}
	bool IsGrenadeSelected() const {
		return currentSWeapon == 0;
	}
	bool IsGrappleSelected() const {
		return currentSWeapon == 1;
	}

	void SetSpawnPos(Vec2f in_pos) {
		spawnPosX = in_pos.x;
		spawnPosY = in_pos.y;
		hasSpawnPos = true;
		isDirty = true;
	}
	void SetPrimary(int in_val) {
		if(std::find(primaryWeaponUnlocks.begin(), primaryWeaponUnlocks.end(), in_val) != primaryWeaponUnlocks.end()) {
			currentPWeapon = in_val;
		}
	}
	void SetSecondary(int in_val) {
		if(std::find(secondaryWeaponUnlocks.begin(), secondaryWeaponUnlocks.end(), in_val) != secondaryWeaponUnlocks.end()) {
			currentSWeapon = in_val;
		}
	}
	Vec2f GetSpawnPos() const {
		return Vec2f{ spawnPosX, spawnPosY };
	}
	int32_t GetSelectedPrimary() const {
		return currentPWeapon;
	}
	int32_t GetSelectedSecondary() const {
		return currentSWeapon;
	}
	bool IsSecondaryUnlocked() {
		return secondaryWeaponUnlocks.size() > 0;
	}
	void CyclePrimary() {
		for(auto i = currentPWeapon + 1; i < 5; i++) {
			if(std::find(primaryWeaponUnlocks.begin(), primaryWeaponUnlocks.end(), i) != primaryWeaponUnlocks.end()) {
				currentPWeapon = i;
				break;
			}
			else {
				currentPWeapon = 0;
			}
		}
		if(currentPWeapon == 1 && !GetMissileCount()) {
			CyclePrimary();
		}
	}
	void CycleSecondary() {
		if(IsSecondaryUnlocked()) {
			for(auto i = currentSWeapon + 1; i < 4; i++) {
				if(std::find(secondaryWeaponUnlocks.begin(), secondaryWeaponUnlocks.end(), i) != secondaryWeaponUnlocks.end()) {
					currentSWeapon = i;
					break;
				}
				else {
					currentSWeapon = 0;
				}
			}
		}
	}
	bool HasSpawnPos() const {
		return hasSpawnPos;
	}
	void UnlockObject(int32_t in_objId) {
		unlockedObjs.insert(in_objId);
		isDirty = true;
	}
	bool IsObjectUnlocked(int32_t in_objId) const {
		return unlockedObjs.find(in_objId) != unlockedObjs.end();
	}
	void UnlockCinematic(int32_t in_cinematicId) {
		if(!IsCinematicUnlocked(in_cinematicId)){
			unlockedCinematics.insert(in_cinematicId);
			isDirty = true;
		}
	}
	bool IsCinematicUnlocked(int32_t in_cinematicId) const {
		return unlockedCinematics.find(in_cinematicId) != unlockedCinematics.end();
	}
	void UnlockRoom(int32_t in_roomId) {
		unlockedRooms.insert(in_roomId);
		isDirty = true;
		// Debug statement
		//std::cout << "CAMERAMANAGER: Room #" << in_roomId << " unlocked!" << std::endl;
	}
	bool IsRoomUnlocked(int32_t in_roomId) const {
		return unlockedRooms.find(in_roomId) != unlockedRooms.end();
	}
	std::vector<int32_t> GetPWeaponUnlocks() { return primaryWeaponUnlocks; }
	std::vector<int32_t> GetSWeaponUnlocks() { return secondaryWeaponUnlocks; }

	void CompleteCinematic(int32_t in_cinematicId) {
		cinematicsCompleted.insert(in_cinematicId);
		isDirty = true;
	}
	bool IsCinematicComplete(int32_t in_cinematicId) {
		return cinematicsCompleted.find(in_cinematicId) != cinematicsCompleted.end();
	}

private:
	bool isDirty = false;
	int32_t version = PLAYERSTATUS_VERSION;
	int32_t missileTanks = 20;
	int32_t missiles = 99;
	int32_t superMissileTanks = 0;
	int32_t superMissiles = 0;
	int32_t grenadeTanks = 0;
	int32_t grenades = 0;
	int32_t energyTanks = 0;
	int32_t energy = 30;
	bool ball = false;
	bool bomb = false;
	bool longBeam = true;
	BeamStatus beamStatus = { false, false };
	bool highJump = true;
	bool wallJump = false;
	bool charge = false;
	bool screwJump = false; //< Not implemented!
	float spawnPosX = 0.0f;
	float spawnPosY = 0.0f;
	bool hasSpawnPos = false;
	bool hasNewSuit = false;
	std::set<int32_t> unlockedObjs;
	std::set<int32_t> unlockedRooms;
	std::set<int32_t> unlockedCinematics;
	std::set<int32_t> cinematicsCompleted;
	std::vector<int32_t> primaryWeaponUnlocks = { 0, 1 };
	std::vector<int32_t> secondaryWeaponUnlocks = {};
	int32_t currentPWeapon = 1;
	int32_t currentSWeapon = 0;
};
