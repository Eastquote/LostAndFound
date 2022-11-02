#include "AsteroidField.h"

#include "GameWorld.h"
#include "SquirrelNoise5.h"
#include "Engine/Components/SpriteComponent.h"
#include "CameraManager.h"

Asteroid::Asteroid(std::optional<std::string> in_animName, Vec2f in_moveVec, float in_rotationSpeed, int32_t in_startFrame, int32_t in_drawOrder)
	: m_animName(in_animName.has_value() ? in_animName.value() : "")
	, m_moveDir(in_moveVec.Norm())
	, m_moveSpeed(in_moveVec.Len())
	, m_drawOrder(in_drawOrder)
	, m_startFrame(in_startFrame)
	, m_rotationSpeed(in_rotationSpeed)
{
}
void Asteroid::Initialize() {
	GameActor::Initialize();
	m_sprite = MakeSprite();
	m_sprite->SetPlayRate(0.0f);
	m_sprite->SetComponentDrawOrder(m_drawOrder);
	m_sprite->PlayAnim(m_animName, true, m_startFrame);
	m_sprite->SetRenderLayer("parallax");
}
Task<> Asteroid::ManageActor() {
	while(true) {
		// Update pos, ignoring camera motion/parallax
		SetWorldPos(GetWorldPos() + m_moveDir * (m_moveSpeed * DT()));
		// Update rotation
		SetWorldRot(GetWorldRot() + m_rotationSpeed * DT());
		co_await Suspend();
	}
}

void AsteroidField::Initialize() {
	GameActor::Initialize();
	// Create bounding box
	auto boundsScalar = 4.0f;
	auto cameraPos = GameWorld::Get()->GetCameraManager()->GetWorldPos();
	SetWorldPos(cameraPos);
	m_bounds = Box2f::FromCenter(GetWorldPos(), Vec2f{ 600.0f / boundsScalar, 500.f / boundsScalar });
	int32_t max_xLg = 2;
	int32_t max_lg = 6;
	int32_t max_med = 10;
	int32_t max_sm = 10;
	// Spawn BKG asteroids
	for(auto i = 0; i < max_sm; ++i) {
		auto asteroid = SpawnRandomAsteroid("AsteroidSm/AsteroidSm", 1, 1);
		m_asteroids.push_back(asteroid);
		m_bkgAsteroids.push_back(asteroid);
	}
	// Spawn MG asteroids
	for(auto i = 0; i < max_med; ++i) {
		auto asteroid = SpawnRandomAsteroid("AsteroidMed/AsteroidMed", 2, 2);
		m_asteroids.push_back(asteroid);
		m_mgAsteroids.push_back(asteroid);
	}
	// Spawn FG asteroids
	for(auto i = 0; i < max_xLg; ++i) {
		std::shared_ptr<Asteroid> asteroid = nullptr;
		asteroid = SpawnRandomAsteroid("AsteroidXlg/AsteroidXlg", 0, 3);
		m_asteroids.push_back(asteroid);
		m_fgAsteroids.push_back(asteroid);
	}
	for(auto i = 0; i < max_lg; ++i) {
		auto asteroid = SpawnRandomAsteroid("AsteroidLg/AsteroidLg", 1, 3);
		m_asteroids.push_back(asteroid);
		m_fgAsteroids.push_back(asteroid);
	}
}

std::shared_ptr<Asteroid> AsteroidField::SpawnRandomAsteroid(std::string in_animName, int32_t in_variantCount, int32_t in_drawOrder) {
	std::shared_ptr<Asteroid> result = nullptr;
	auto bounds = GetWorld()->GetCameraManager()->GetAsteroidBoundsWorld();
	float randXOffset = (Get1dNoiseZeroToOne(m_noiseIndex) * bounds.w);
	++m_noiseIndex;
	float randYOffset = (Get1dNoiseZeroToOne(m_noiseIndex) * bounds.h);
	++m_noiseIndex;
	float randRotation = (Get1dNoiseZeroToOne(m_noiseIndex) * 360.0f);
	++m_noiseIndex;
	Vec2f randMoveDir = Math::DegreesToVec((Get1dNoiseZeroToOne(m_noiseIndex) * 90.0f) + 135.0f);
	++m_noiseIndex;
	float randMoveSpeed = Get1dNoiseZeroToOne(m_noiseIndex) * 0.5f * 60.0f;
	++m_noiseIndex;
	float randRotationSpeed = ((Get1dNoiseZeroToOne(m_noiseIndex) * 1.0f) - 0.5f) * 60.0f;
	++m_noiseIndex;
	int32_t randFrame = 0;
	if(in_variantCount > 0) {
		randFrame = (int32_t)std::round(Get1dNoiseZeroToOne(m_noiseIndex) * (float)in_variantCount);
	}
	Vec2f moveVec = randMoveDir * randMoveSpeed;
	Transform randTransform = { Vec2f{randXOffset, randYOffset}, randRotation, Vec2f::One };
	result = Spawn<Asteroid>(AsShared<AsteroidField>(), Transform::Identity, in_animName, moveVec, randRotationSpeed, randFrame, in_drawOrder);
	return result;
}

void AsteroidField::ApplyParallax(Vec2f in_cameraDisp) {
	for(auto asteroid : m_fgAsteroids) {
		auto parallaxedDisp = in_cameraDisp * m_fgParallax;
		asteroid->SetWorldPos(asteroid->GetWorldPos() + parallaxedDisp);
	}
	for(auto asteroid : m_mgAsteroids) {
		auto parallaxedDisp = in_cameraDisp * m_mgParallax;
		asteroid->SetWorldPos(asteroid->GetWorldPos() + parallaxedDisp);
	}
	for(auto asteroid : m_bkgAsteroids) {
		auto parallaxedDisp = in_cameraDisp * m_bkgParallax;
		asteroid->SetWorldPos(asteroid->GetWorldPos() + parallaxedDisp);
	}
}

Task<> AsteroidField::ManageActor() {
	auto cameraManager = GetWorld()->GetCameraManager();
	auto cameraPos = Vec2f::Zero;
	while(true) {
		cameraPos = cameraManager->GetCameraWorldTransform().pos;
		SetWorldPos(cameraPos);

		auto bounds = cameraManager->GetAsteroidBoundsWorld();
		// "Wrap around" pos of any asteroids that have drifted out of bounds
		for(auto asteroid : m_asteroids) {
			auto asteroidPos = asteroid->GetWorldPos();
			auto leftEdge = bounds.GetLeft();
			auto rightEdge = bounds.GetRight();
			auto topEdge = bounds.GetTop();
			auto bottomEdge = bounds.GetBottom();
			if(asteroidPos.x < leftEdge) {
				asteroid->SetWorldPos(asteroidPos + Vec2f{ bounds.w, 0.0f });
			}
			else if(asteroidPos.x > rightEdge) {
				asteroid->SetWorldPos(asteroidPos - Vec2f{ bounds.w, 0.0f });
			}
			else if(asteroidPos.y < bottomEdge) {
				asteroid->SetWorldPos(asteroidPos + Vec2f{ 0.0f, bounds.h });
			}
			else if(asteroidPos.y > topEdge) {
				asteroid->SetWorldPos(asteroidPos - Vec2f{ 0.0f, bounds.h });
			}
		}
		auto lastCameraDisp = cameraManager->GetLastCameraDisp();
		//std::cout << "lastCameraDisp = " << lastCameraDisp.x << ", " << lastCameraDisp.y << std::endl;
		ApplyParallax(cameraManager->GetLastCameraDisp());
		co_await Suspend();
	}
}