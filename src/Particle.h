#pragma once

#include "GameActor.h"
#include "Engine/MathGeometry.h"
#include <math.h>

class Effect;
class Player;
class Creature;

struct ParticleDef {
	Vec2f spawnPosOffset = Vec2f::Zero;
	std::string animName = "Effects/PinkBox";
	uint32_t animStartFrame = 0;
	float animPlayrate = 1.0f;
	int32_t animFlipRotation = 0;
	float animFlipRotationFps = 20;
	bool animBlink = false;
	std::string effectAnimName = "Explosion2/Explosion";
	Box2f worldCollisionBox;
	SensorShape sensorShape;
	bool collideWorld = false;
	float bounce = 0.0f;
	float lifetime = 1.0f;
	float direction = 90.0f;
	float speed = 1.0f * 60.0f;
	float rotation = 0.0f;
	float rotationSpeed = 0.0f;
	float scale = 1.0f;
	float gravity = 0.5f;
	uint32_t category = 0;
	uint32_t mask = 0;
	std::string palette = "Base";
};

// Simple effect particle -- moves according to spawn params, dies when it hits something (if set that way) or lifetime expires
class Particle : public GameActor {
public:
	Particle(const ParticleDef& in_def, std::optional<Vec2f> in_dirOverride);
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;
	// Stripped-down move function
	Vec2f Move(const Vec2f& in_pos, bool collideWorld = false);
	Vec2f GetVel() { return Math::DegreesToVec(m_particleDef->direction) * m_particleDef->speed; }
	Box2f GetCollisionBoxWorld() const { return m_worldCollisionBox.TransformedBy(GetWorldTransform()); };

private:
	void OnTouchPlayer(std::shared_ptr<Player> in_player);
	void OnTouchCreature(std::shared_ptr<Creature> in_creature);
	// "NES-style" flip-based rotation, plus 30hz flicker
	Task<> AnimFlipAndBlink();

	void Bounce(const Math::BoxSweepResults& in_sweepResults, float in_amount);

	std::shared_ptr<ParticleDef> m_particleDef;
	std::shared_ptr<SpriteComponent> m_animSprite;
	Box2f m_worldCollisionBox = Box2f::FromCenter(Vec2f::Zero, { 8.0f, 8.0f });
	std::optional<Vec2f> m_dirOverride;
	float m_bounce = 0.0f;
	float m_elapsedLifetime = 0.0f;
};
