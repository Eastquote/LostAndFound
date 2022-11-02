#include "Projectiles/Grenade.h"
#include "Engine/MathRandom.h"

void Grenade::Initialize() {
	Projectile::Initialize();
	m_gravity = 0.05f * 60.0f;
	m_maxSpeed = m_def.speed; //< For grenades, we use the def's speed value as a max speed
	//m_speed = m_mag; //< Then we set m_speed to whatever magnitude came in on the spawn call
	m_bounce = 0.5f; //< Retains this much magnitude after each Bounce() call
	m_rotationSpeed = Math::RandomInt(0, 1) == 1 ? 2.5f * 60.0f : -2.5f * 60.0f;
	m_bUpdatesOffScreen = true;
}
Vec2f Grenade::ModifyMovement() {
	Vec2f newPos = Vec2f::Zero;
	auto vel = GetVel();
	vel.y = std::clamp(vel.y - m_gravity, -m_maxSpeed, m_maxSpeed);
	m_speed = vel.Len();
	m_direction = vel.Norm();
	newPos = GetWorldPos() + (GetVel() * DT());
	return newPos;
}