#include "ParticleSpawner.h"
#include "Particle.h"
#include "GameWorld.h"
#include "Engine/MathRandom.h"
#include "Algorithms.h"
#include <iostream>

ParticleSpawner::ParticleSpawner(const ParticleSpawnerDef& in_def, std::optional<Vec2f> in_dirOverride, const std::string& in_palette)
	: m_particleSpawnerDef(std::make_shared<ParticleSpawnerDef>(in_def))
	, m_dirOverride(in_dirOverride)
	, m_palette(in_palette)
{
}
void ParticleSpawner::Initialize() {
	GameActor::Initialize();
	m_worldCollisionBox = Box2f::FromCenter(Vec2f::Zero, m_particleSpawnerDef->collisionBoxDims);

	if(m_particleSpawnerDef->sensorRadius) {
		Circle particleCollisionCircle = { m_particleSpawnerDef->sensorRadius };
		m_particleSensorShape.SetCircle(particleCollisionCircle);
	}
	else {
		Vec2f particleCollisionBox = m_worldCollisionBox.GetDims();
		m_particleSensorShape.SetBox(particleCollisionBox);
	}
	m_spawnerLifetime = Math::RandomFloat(m_particleSpawnerDef->spawnerLifetimeRange.x, m_particleSpawnerDef->spawnerLifetimeRange.y);
	//printf("Particle Spawner initialized!\n");
}
void ParticleSpawner::Destroy() {
	//printf("Particle Spawner destroyed!\n");
	GameActor::Destroy();
}
void ParticleSpawner::Update() {
	GameActor::Update();
	EraseInvalid(m_liveParticles);
}
void ParticleSpawner::MakeParticle() {
	ParticleDef newDef = {
		m_particleSpawnerDef->spawnPosOffset,
		m_particleSpawnerDef->animName,
		m_particleSpawnerDef->animStartFrame,
		m_particleSpawnerDef->animPlayrate,
		m_particleSpawnerDef->animFlipRotation,
		m_particleSpawnerDef->animFlipRotationFps,
		m_particleSpawnerDef->animBlink,
		m_particleSpawnerDef->effectAnimName,
		m_worldCollisionBox,
		m_particleSensorShape,
		m_particleSpawnerDef->collideWorld,
		m_particleSpawnerDef->bounce,
		Math::RandomFloat(m_particleSpawnerDef->particleLifetimeRange.x, m_particleSpawnerDef->particleLifetimeRange.y),
		Math::RandomFloat(m_particleSpawnerDef->directionRange.x, m_particleSpawnerDef->directionRange.y),
		Math::RandomFloat(m_particleSpawnerDef->speedRange.x, m_particleSpawnerDef->speedRange.y),
		Math::RandomFloat(m_particleSpawnerDef->rotationRange.x, m_particleSpawnerDef->rotationRange.y),
		Math::RandomFloat(m_particleSpawnerDef->rotationSpeedRange.x, m_particleSpawnerDef->rotationSpeedRange.y),
		Math::RandomFloat(m_particleSpawnerDef->scaleRange.x, m_particleSpawnerDef->scaleRange.y),
		m_particleSpawnerDef->gravity,
		m_particleSpawnerDef->category,
		m_particleSpawnerDef->mask,
		m_palette,
	};
	auto newParticle = Actor::Spawn<Particle>(GetWorld(), { GetWorldPos() }, newDef, m_dirOverride);
	m_liveParticles.push_back(newParticle);
}
Task<> ParticleSpawner::ManageActor() {
	auto elapsedLifetime = 0.0f;
	auto spawnTimer = 0.0f;
	auto spawnRate = 1.0f / Math::RandomFloat(m_spawnRange.x, m_spawnRange.y);
	if(m_burst) {
		spawnTimer = m_burstRate;
	}
	else {
		spawnTimer = spawnRate;
	}
	while(true) {
		if(m_liveParticles.size() < m_particlesMax) {
			if(m_burst) {
				if(spawnTimer >= m_burstRate) {
					auto burstAmount = Math::RandomInt(	m_particleSpawnerDef->burstParticleRange.x, 
														m_particleSpawnerDef->burstParticleRange.y);
					for(int i = 0; i < burstAmount; i++) {
						MakeParticle();
					}
					spawnTimer = 0.0f;
				}
			}
			else {
				if(spawnTimer >= spawnRate) {
					MakeParticle();
					auto spawnRate = 1.0f / Math::RandomFloat(m_spawnRange.x, m_spawnRange.y);
					spawnTimer = 0.0f;
				}
			}
		}
		co_await Suspend();
		elapsedLifetime += DT();
		spawnTimer += DT();
		if(elapsedLifetime > m_spawnerLifetime) {
			DeferredDestroy();
		}
	}
}
