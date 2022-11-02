#pragma once
#include <math.h>
#include "GameActor.h"
#include "GameEnums.h"

class Particle;

struct ParticleSpawnerDef {
	Vec2f spawnPosOffset = Vec2f::Zero;
	std::string animName = "Effects/PinkBox";
	uint32_t animStartFrame = 0;
	float animPlayrate = 1.0;
	int32_t animFlipRotation = 0;
	float animFlipRotationFps = 20;
	bool animBlink = false;
	std::string effectAnimName = "Explosion2/Explosion";
	Vec2f collisionBoxDims = { 8.0f, 8.0f };
	float sensorRadius = 4.0f;
	bool collideWorld = false;
	float bounce = 0.0f;
	uint32_t category = 0;
	uint32_t mask = 0;
	Vec2f spawnerLifetimeRange = { 0.0f, 0.1f };
	Vec2f particleLifetimeRange = { 0.0f, 2.0f };
	Vec2f directionRange = { 0.0f, 360.0f };
	Vec2f speedRange = { -1.0f, 1.0f };
	Vec2f rotationRange = { 0.0f, 360.0f };
	Vec2f rotationSpeedRange = { 0.0f, 0.0f };
	Vec2f scaleRange = { 1.0f, 1.0f };
	float gravity = 0.0f;
	float gravityDir = 0.0f; // 0-360
	uint32_t particlesMax = 10;
	Vec2f spawnRange = { 0.1f, 10.0f }; // particles/second
	bool burst = true;
	Vec2u burstParticleRange = { 0, 5 }; // particles/burst
	float burstRate = 1.0f; // bursts/second
};

// Store (range-randomized) spawn settings for indiv. particles, and spawn 'em!
class ParticleSpawner : public GameActor {
public:
	ParticleSpawner(const ParticleSpawnerDef& in_def, std::optional<Vec2f> in_dirOverride, const std::string& in_palette = "Base");
	virtual void Initialize() override;
	virtual void Destroy() override;
	virtual Task<> ManageActor() override;
	virtual void Update() override;

private:
	void MakeParticle();
	std::shared_ptr<ParticleSpawnerDef> m_particleSpawnerDef;
	std::vector<std::shared_ptr<Particle>> m_liveParticles;
	float m_spawnerLifetime = 1.0f;
	SensorShape m_particleSensorShape;

	Vec2f m_spawnPosOffset = Vec2f::Zero;
	Box2f m_worldCollisionBox = Box2f::FromCenter(Vec2f::Zero, { 8.0f, 8.0f });
	uint32_t m_particlesMax = 10;
	Vec2f m_spawnRange = { 1.0f, 10.0f };
	bool m_burst = true;
	Vec2u m_burstParticleRange = { 1, 5 };
	float m_burstRate = 1.0f;
	std::optional<Vec2f> m_dirOverride;
	std::string m_palette = "Base";
};