#include "Projectiles/HomingMissile.h"

#include "Creature.h"
#include "Engine/EaseTo.h"
#include "Engine/Components/SpriteComponent.h"

void HomingMissile::Initialize() {
	Projectile::Initialize();
	m_bUpdatesOffScreen = true;
	m_bIgnoresWorld = true;
	SetDrawLayer(2);
}
void HomingMissile::Destroy() {
	if(GetTarget()) {
		GetTarget()->SetTargeted(false);
	}
	Projectile::Destroy();
}
void HomingMissile::OnTouchCreature(std::shared_ptr<Creature> in_creature, std::shared_ptr<SensorComponent> in_sensor) {
	if(in_creature != m_target) { //< Prevents missile from incidentally striking untargeted enemies 
		return;
	}
	Projectile::OnTouchCreature(in_creature, in_sensor);
}
Vec2f HomingMissile::ModifyMovement() {
	if(IsAlive(m_target)) {
		auto targetVec = (m_target->GetWorldPos() - GetWorldPos());
		auto targetDir = targetVec.Norm();
		auto angleConvergeTime = Math::MapRange(m_elapsedLifetime, m_hParams.m_angleConvergeLifetimeRange.m_min, m_hParams.m_angleConvergeLifetimeRange.m_max, 1.0f, 0.0f);
		auto angleBlendTime = Math::MapRange(targetVec.Len(), m_hParams.m_convergeDistRange.m_min, m_hParams.m_convergeDistRange.m_max, m_hParams.m_angleBlendTimeRange.m_min, m_hParams.m_angleBlendTimeRange.m_max) * angleConvergeTime;
		auto newAngle = EaseTo_Spring_Euler((float)Math::VecToDegrees(m_direction), (float)Math::VecToDegrees(targetDir), DT(), m_hParams.m_rotateVel, angleBlendTime);
		m_speed = EaseTo_Spring(m_speed, 15.0f * 60.0f, DT(), m_hParams.m_accelVel, m_hParams.m_accelDur);
		m_direction = Math::DegreesToVec(newAngle).Norm();
	}
	m_projectileSprite->SetWorldRot((float)Math::VecToDegrees(m_direction));
	return GetWorldPos() + (GetVel() * DT());
}