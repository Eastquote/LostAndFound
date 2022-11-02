#include "Projectiles/WaveBeam.h"

#include "GameWorld.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/MathEasings.h"
#include "Engine/MathRandom.h"
#include <iostream>

void WaveBeam::Initialize() {
	Projectile::Initialize();
	m_projectileSprite->SetFlipVert(false);
	m_facingDir = Math::RandomInt(0, 1) == 1 ? 1 : -1;
}
Vec2f WaveBeam::ModifyMovement() {
	auto prevPos = GetWorldPos();
	auto newPos = GetWorldPos() + GetVel();

	auto currOscillation = float((GetWorld()->EaseAlpha(m_waveElapsedLifetime, k_wavePeriod, Math::EaseInOutSin) - 0.5) * m_waveAmplitude * m_waveDir * m_facingDir);
	if(m_direction.x) {
		newPos.y += currOscillation;
	}
	else if(m_direction.y) {
		newPos.x += currOscillation;
	}
	m_waveElapsedLifetime += DT();
	if(m_waveElapsedLifetime >= k_wavePeriod) {
		m_waveDir *= -1;
		m_waveElapsedLifetime = 0.0f;
	}
	auto rotation = (newPos - prevPos).SignedAngleDeg();
	m_projectileSprite->SetWorldRot(rotation);

	return newPos;
}