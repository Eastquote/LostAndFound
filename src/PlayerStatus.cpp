#include "PlayerStatus.h"

#include <fstream>

//--- PLAYER STATUS CODE ---//

void PlayerStatus::TryLoad() {
	std::ifstream file{ "save.bin", std::ios_base::in | std::ios_base::binary };
	if(file.is_open()) {
		auto readPod = [&file](auto& data) {
			file.read(reinterpret_cast<char*>(&data), sizeof(decltype(data)));
		};
		auto readSet = [&file, readPod](auto& data) {
			int32_t size = 0;
			readPod(size);
			while(size-- > 0) {
				using setType = std::decay<decltype(data)>::type;
				setType::value_type item;
				readPod(item);
				data.insert(item);
			}
		};
		auto readVector = [&file, readPod](auto& data) {
			int32_t size = 0;
			readPod(size);
			while(size-- > 0) {
				using setType = std::decay<decltype(data)>::type;
				setType::value_type item;
				readPod(item);
				if(std::find(data.begin(), data.end(), item) == data.end()) {
					data.emplace_back(item);
				}
			}
		};
		int32_t fileVersion = 0;
		readPod(fileVersion);
		if(fileVersion > PLAYERSTATUS_VERSION) {
			return;
		}
		readPod(missileTanks);
		readPod(missiles);
		readPod(superMissileTanks);
		readPod(superMissiles);
		readPod(grenadeTanks);
		readPod(grenades);
		readPod(energyTanks);
		readPod(energy);
		readPod(ball);
		readPod(bomb);
		readPod(longBeam);
		readPod(beamStatus);
		readPod(highJump);
		readPod(wallJump);
		readPod(charge);
		readPod(screwJump);
		readPod(spawnPosX);
		readPod(spawnPosY);
		readPod(hasSpawnPos);
		readPod(hasNewSuit);
		// non-POD data starts here:
		readSet(unlockedObjs);
		readSet(unlockedRooms);
		readSet(unlockedCinematics);
		readSet(cinematicsCompleted);
		readVector(primaryWeaponUnlocks);
		readVector(secondaryWeaponUnlocks);
	}
	if(energy <= 0) {
		energy = 30;
	}
}
void PlayerStatus::Save() {
	if(isDirty) {
		std::ofstream file{ "save.bin", std::ios_base::out | std::ios_base::binary };
		auto writePod = [&file](auto data) {
			file.write(reinterpret_cast<const char*>(&data), sizeof(decltype(data)));
		};
		auto writeSet = [&file, writePod](auto data) {
			int32_t size = (int32_t)data.size();
			writePod(size);
			for(auto item : data) {
				writePod(item);
			}
		};
		writePod(version);
		writePod(missileTanks);
		writePod(missiles);
		writePod(superMissileTanks);
		writePod(superMissiles);
		writePod(grenadeTanks);
		writePod(grenades);
		writePod(energyTanks);
		writePod(energy);
		writePod(ball);
		writePod(bomb);
		writePod(longBeam);
		writePod(beamStatus);
		writePod(highJump);
		writePod(wallJump);
		writePod(charge);
		writePod(screwJump);
		writePod(spawnPosX);
		writePod(spawnPosY);
		writePod(hasSpawnPos);
		writePod(hasNewSuit);
		// non-POD data starts here:
		writeSet(unlockedObjs);
		writeSet(unlockedRooms);
		writeSet(unlockedCinematics);
		writeSet(cinematicsCompleted);
		writeSet(primaryWeaponUnlocks);
		writeSet(secondaryWeaponUnlocks);
		isDirty = false;
	}
}
