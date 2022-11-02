#include "DropManager.h"
#include "Creature.h"
#include "GameWorld.h"
#include "Engine/MathRandom.h"
#include "PlayerStatus.h"
#include "PickupDefs.h"
#include <numeric>

//--- DROP MANAGER CODE ---//

bool DropManager::TrySpawning(std::shared_ptr<Creature> in_instigator) {
	auto dropRoll = Math::RandomFloat(0.0f, 102.0f);
	auto dropOdds = in_instigator->GetDropOdds();
	auto dropOddsModified = dropOdds;
	auto world = GameWorld::Get();

	// Check if player's items are full
	auto playerMissilesFull = (world->GetPlayerMissiles(true) == 100);
	auto playerSuperMissilesFull = (world->GetPlayerSuperMissiles(true) == 100);
	auto playerGrenadesFull = (world->GetPlayerGrenades(true) == 100);
	auto playerStatus = world->GetPlayerStatus();

	// Check player health to see if they're critical or not
	auto playerHealth = world->GetPlayerHealth();
	auto bCriticalHealth = false;
	if(playerHealth < 30) {
		bCriticalHealth = true;
	}

	// n.b. The threshold to EXIT criticalHealth state is intentionally higher than the threshold to ENTER it
	else if(playerHealth > 50) {
		bCriticalHealth = false;
	}
	
	if(bCriticalHealth) {
		// This state overrides the player's normal dropOdds and only allows for health pickups to spawn
		// (~fifty-fifty on whether you get a sm or lg health pellet)
		auto criticalHealthDropOdds = std::array<float, 6>{ 0.0f, 50.0f, 50.0f, 0.0f, 0.0f, 0.0f,};
		float accumulator = 0;
		for(int32_t i = 0; i < criticalHealthDropOdds.size(); i++) {
			accumulator += criticalHealthDropOdds[i];
			if(dropRoll > accumulator || i == 0) {
				continue;
			}
			else {
				Spawn(i, in_instigator);
				return true;
			}
		}
	}
	else {
		// Figure out which items are either full or haven't been unlocked yet
		std::vector<int32_t> fullOrLockedItems;
		if(world->GetPlayerHealth(true) == 100) {
			fullOrLockedItems.push_back(1);
			fullOrLockedItems.push_back(2);
			dropOddsModified[1] = 0.0f;
			dropOddsModified[2] = 0.0f;
		}
		if(playerMissilesFull || playerStatus->GetMissileTankCount() == 0) {
			fullOrLockedItems.push_back(3);
			dropOddsModified[3] = 0.0f;
		}
		if(playerSuperMissilesFull || playerStatus->GetSuperMissileTankCount() == 0) {
			fullOrLockedItems.push_back(4);
			dropOddsModified[4] = 0.0f;
		}
		if(playerGrenadesFull || playerStatus->GetGrenadeTankCount() == 0) {
			fullOrLockedItems.push_back(5);
			dropOddsModified[5] = 0.0f;
		}

		// Calculate the total number of possible values (defaults to 102 but likely less because of locked/full items being zeroed)
		auto totalOdds = std::accumulate(std::begin(dropOddsModified), std::end(dropOddsModified), 0.0f, std::plus<float>());

		// Iterate through fullOrLockedItems, proportionately distributing the full/locked items' 
		// drop probability into the dropOddsModified items that are still eligible to drop
		for(auto ineligibleItem : fullOrLockedItems) {
			for(auto i = 0; i < 4; i++) {
				if(i == ineligibleItem) {
					continue;
				}
				else {
					auto proportionalOdds = dropOddsModified[i] / totalOdds; //< Calculate this item's proportion of the total odds
					dropOddsModified[i] += dropOdds[ineligibleItem] * proportionalOdds;
				}
			}
		}

		// Iterate through the dropOddsModified to see which of the six items' value ranges the dropRoll value lands in
		// (spawn if it's a hit!)
		float accumulator = 0;
		for(int32_t i = 0; i < dropOddsModified.size(); i++) {
			accumulator += dropOddsModified[i];
			if(dropRoll > accumulator) {
				continue;
			}
			else {
				Spawn(i, in_instigator);
				return true;
			}
		}
	}
	return false;
}
std::shared_ptr<Pickup> DropManager::Spawn(int32_t in_dropItemIndex, std::shared_ptr<Creature> in_instigator) {
	if(in_dropItemIndex == 0) {
		return nullptr;
	}
	else if(in_dropItemIndex == 1) {
		return Actor::Spawn<Pickup>(in_instigator->GetSpawner(), 
									{ in_instigator->GetWorldPos() }, in_instigator->GetSpawner(), 0, g_pickupSmallEnergy);
	}
	else if(in_dropItemIndex == 2) {
		return Actor::Spawn<Pickup>(in_instigator->GetSpawner(), 
									{ in_instigator->GetWorldPos() }, in_instigator->GetSpawner(), 0, g_pickupLargeEnergy);
	}
	else if(in_dropItemIndex == 3) {
		return Actor::Spawn<Pickup>(in_instigator->GetSpawner(), 
									{ in_instigator->GetWorldPos() }, in_instigator->GetSpawner(), 0, g_pickupMissile);
	}
	else if(in_dropItemIndex == 4) {
		return Actor::Spawn<Pickup>(in_instigator->GetSpawner(), 
									{ in_instigator->GetWorldPos() }, in_instigator->GetSpawner(), 0, g_pickupSuperMissile);
	}
	else if(in_dropItemIndex == 5) {
		return Actor::Spawn<Pickup>(in_instigator->GetSpawner(), 
									{ in_instigator->GetWorldPos() }, in_instigator->GetSpawner(), 0, g_pickupGrenade);
	}
	return nullptr;
}
