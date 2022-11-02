#pragma once

#include "Projectile.h"

// "Wave" beam that oscillates on a sinewave as it moves in a cardinal direction
class WaveBeam : public Projectile {
public:
	using Projectile::Projectile;
	virtual void Initialize() override;
	virtual Vec2f ModifyMovement() override;

private:
	float k_wavePeriod = 0.2f;
	float m_waveAmplitude = 11.0f;
	float m_waveElapsedLifetime = 0.0f;
};
