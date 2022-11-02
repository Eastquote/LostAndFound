#pragma once

#include "Engine/Actor.h"

class AimReticle;

// Manages aim reticles for the charged (homing) missile to ensure there's only one active at a time
class AimReticleManager : public Actor {
public:
	virtual void Update() override;
	void ClearOldReticles();
	void RegisterAimReticle(std::shared_ptr<AimReticle> in_aimReticle);

private:
	std::vector<std::shared_ptr<AimReticle>> m_aimReticles;
};
