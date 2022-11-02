#include "AimReticleManager.h"

#include "AimReticle.h"
#include "Algorithms.h"

void AimReticleManager::Update() {
	Actor::Update();
	EraseInvalid(m_aimReticles);
}
void AimReticleManager::ClearOldReticles() {
	for(auto aimReticle : m_aimReticles) {
		if(IsAlive(aimReticle)) {
			aimReticle->RequestDestroy();
		}
	}
}
void AimReticleManager::RegisterAimReticle(std::shared_ptr<AimReticle> in_aimReticle) {
	// Kick off the graceful destruction of any AimReticles that already existed (without visual pops)
	ClearOldReticles();
	// Add a new aimReticle to track the current target
	m_aimReticles.push_back(in_aimReticle);
}
