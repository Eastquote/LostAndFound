#pragma once
#include "GameActor.h"

// A simple class for displaying drifting asteroids in the game's background.
class Asteroid : public GameActor {
public:
	Asteroid(std::optional<std::string> in_animName, Vec2f in_moveVec = Vec2f::Zero, float in_rotationSpeed = 0.0f, int32_t in_startFrame = 0, int32_t in_drawOrder = 0);
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;

	std::string m_animName = "";
	std::shared_ptr<SpriteComponent> m_sprite;
	Vec2f m_moveDir = Vec2f::Zero;
	float m_moveSpeed = 0.0f;
	float m_rotationSpeed = 0.0f;
	int32_t m_startFrame = 0;
	int32_t m_drawOrder = 0;
};

// Manages the positions, sorting, and movement/rotation of all the Asteroid objects in the background
class AsteroidField : public GameActor {
public:
	AsteroidField() {};
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;
	void ApplyParallax(Vec2f in_cameraDisp);

private:
	std::shared_ptr<Asteroid> SpawnRandomAsteroid(std::string in_animName, int32_t in_variantCount, int32_t in_drawOrder);
	int32_t m_noiseIndex = 0;
	Box2f m_bounds = Box2f::FromCenter(Vec2f::Zero, Vec2f::Zero);
	std::vector<std::shared_ptr<Asteroid>> m_asteroids;
	std::vector<std::shared_ptr<Asteroid>> m_fgAsteroids;
	std::vector<std::shared_ptr<Asteroid>> m_mgAsteroids;
	std::vector<std::shared_ptr<Asteroid>> m_bkgAsteroids;
	float m_fgParallax = 0.1f;
	float m_mgParallax = 0.5f;
	float m_bkgParallax = 1.0f;
};