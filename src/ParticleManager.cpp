#include "ParticleManager.h"

#include "Algorithms.h"

//--- PARTICLE MANAGER CODE ---//

void ParticleManager::Initialize() {
	Actor::Initialize();
}
void ParticleManager::Update() {
	Actor::Update();
	EraseInvalid(m_particles);
}
void ParticleManager::RegisterParticle(std::shared_ptr<Particle> in_particle) {
	m_particles.push_back(in_particle);
}