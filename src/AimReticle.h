#pragma once

#include "GameActor.h"

// Displays a reticle for indicating targets of the charged (homing) missile
class AimReticle : public GameActor {
public:
	AimReticle(std::shared_ptr<GameActor> in_target);
	virtual void Initialize() override;
	virtual void Destroy() override;
	virtual Task<> ManageActor() override;
	void RequestDestroy(); //< Tells the reticle to clean itself up gracefully over time (without visual pops)

private:
	void UnlinkTarget();
	Task<> TrackTarget();

	std::shared_ptr<SpriteComponent> m_sprite;
	std::weak_ptr<GameActor> m_target;
	bool m_bDestroyRequested = false;
};