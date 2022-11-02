#pragma once

#include "Projectile.h"

struct HomingParams {
	MinMaxf m_angleConvergeLifetimeRange = { 0.3f, 1.0f };
	MinMaxf m_convergeDistRange = { 8.0f, 60.0f };
	MinMaxf m_angleBlendTimeRange = { 0.0f, 0.15f };
	float m_convergeDist = 60.0f;
	float m_rotateVel = 100.0f;
	float m_accelVel = 0.0f;
	float m_accelDur = 0.55f;
};

// Charged homing missile subclass -- has a target and overridden movement behavior
class HomingMissile : public Projectile {
public:
	using Projectile::Projectile;
	virtual void Initialize() override;
	virtual void Destroy() override;
	// Launches forwards, then steers around & accelerates to target
	virtual Vec2f ModifyMovement() override;
	void SetTarget(std::shared_ptr<GameActor> in_target) { m_target = in_target; }
	std::shared_ptr<GameActor> GetTarget() { return m_target; }

protected:
	virtual void OnTouchCreature(std::shared_ptr<Creature> in_creature, std::shared_ptr<SensorComponent> in_sensor) override;

private:
	std::shared_ptr<GameActor> m_target;
	HomingParams m_hParams;
};
