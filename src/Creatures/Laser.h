#pragma once

#include "Creature.h"

// Laser creature (groups of these guys shoot lasers between themselves)
class Laser : public Creature {
	DECLARE_SPAWNABLE_TYPE(Laser);
public:
	Laser(std::shared_ptr<CreatureSpawner> in_instigator, bool in_direction, float in_rotation, std::string in_name);
	virtual void Initialize() override;
	virtual void Update() override;

protected:
	virtual Task<> ManageAI() override;
	virtual void TouchCallback(std::shared_ptr<GameActor> in_other) override;
	Task<> DrawBeam(std::weak_ptr<Creature> in_target);
	Task<> KillLaser();
	void ClearTargets();
	virtual Task<> PreDeath() override;
	virtual Vec2f GetCollisionSize() const override {
		return { 10.0f, 6.0f };
	}
	Task<> DeactivateLaser();
	Task<> ActivateLaser();

private:
	std::vector<std::weak_ptr<Laser>> m_laserTargets;
	std::vector<std::shared_ptr<SpriteComponent>> m_laserBeamSprites;
	std::vector<std::shared_ptr<Light>> m_laserBeamLights;
	bool m_bPrimary = false;
	Vec2f m_direction = { 1.0f, 0.0f };
	bool m_bActive = true;
};