#pragma once

#include "Particle.h"
#include "Effect.h"
#include <memory>
#include <vector>

// Dead simple particle manager -- keep registry of live particles, erases invalid (Destroyed) ones every tick
class ParticleManager : public Actor {
public:
	virtual void Initialize() override;
	virtual void Update() override;
	size_t GetParticleCount() const { return m_particles.size(); }
	void RegisterParticle(std::shared_ptr<Particle> in_particle);

private:
	std::vector<std::shared_ptr<Particle>> m_particles;
};
