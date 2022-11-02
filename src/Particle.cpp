#include "Particle.h"

#include "ParticleManager.h"
#include "GameWorld.h"
#include "Player.h"
#include "Creature.h"
#include "Engine/MathGeometry.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/TilesComponent.h"
#include "Engine/TileMap.h"
#include <iostream>

//--- PARTICLE CODE ---//

Particle::Particle(const ParticleDef& in_def, std::optional<Vec2f> in_dirOverride)
	: m_particleDef(std::make_shared<ParticleDef>(in_def))
	, m_dirOverride(in_dirOverride)
{
}
void Particle::Initialize() {
	GameActor::Initialize();

	m_worldCollisionBox = m_particleDef->worldCollisionBox;

	// Setup sensor
	auto particleSensor = MakeSensor(Transform::Identity, m_particleDef->sensorShape);
	particleSensor->SetFiltering(m_particleDef->category, m_particleDef->mask);
	particleSensor->SetTouchCallback([this](bool in_beginning, std::shared_ptr<SensorComponent> in_other) {
		if(in_beginning) {
			if(std::dynamic_pointer_cast<Player>(in_other->GetActor())) {
				OnTouchPlayer(std::dynamic_pointer_cast<Player>(in_other->GetActor()));
			}
			if(std::dynamic_pointer_cast<Creature>(in_other->GetActor())) {
				OnTouchCreature(std::dynamic_pointer_cast<Creature>(in_other->GetActor()));
			}
		}
		});

	// Setup sprite
	m_animSprite = MakeSprite(Transform::Identity);
	//m_animSprite->SetRenderLayer("hud");
	m_animSprite->PlayAnim(m_particleDef->animName, true, m_particleDef->animStartFrame);
	m_animSprite->SetPlayRate(m_particleDef->animPlayrate);
	m_animSprite->SetPalette(m_particleDef->palette);
	m_animSprite->SetWorldRot(m_particleDef->rotation);
	m_animSprite->SetWorldScale(Vec2f{ m_particleDef->scale, m_particleDef->scale });

	m_bounce = m_particleDef->bounce;

	// Register with manager
	auto particleMgr = GetWorld()->GetParticleManager();
	particleMgr->RegisterParticle(AsShared<Particle>());
}
Task<> Particle::ManageActor() {
	m_elapsedLifetime = 0.0f;
	//auto speed = m_particleDef->speed;
	TaskHandle<> animFlipAndBlinkTask = m_taskMgr.Run(AnimFlipAndBlink());
	auto dirOverride = m_dirOverride.value_or(Vec2f::Zero);
	while(true) {
		auto vel = GetVel();
		Move(Vec2f{ GetWorldPos().x + ((vel.x + dirOverride.x) * DT()), GetWorldPos().y + ((vel.y + dirOverride.y) * DT())}, m_particleDef->collideWorld);
		vel = GetVel();
		vel.y -= m_particleDef->gravity;
		m_particleDef->speed = vel.Len();
		m_particleDef->direction = vel.SignedAngleDeg();
		if(m_dirOverride.has_value()) {
			dirOverride.y -= m_particleDef->gravity;
		}
		if(auto rotSpeed = m_particleDef->rotationSpeed) {
			m_particleDef->rotation += (rotSpeed * 360 * DT());
			m_animSprite->SetWorldRot(m_particleDef->rotation);
		}
		if(m_elapsedLifetime > m_particleDef->lifetime && !IsDestroyed()) {
			DeferredDestroy();
		}
		co_await Suspend();
		m_elapsedLifetime += DT();
	}
}
Task<> Particle::AnimFlipAndBlink() {
	auto updateTimer = 0.0f;
	auto visibility = false;
	auto flipDir = m_particleDef->animFlipRotation;
	auto flipUpdateDelay = 1 / m_particleDef->animFlipRotationFps;
	auto flickerTimer = 1.0f;
	// "NES-style" rotation func: simply alternates horizontal and vertical flips to create illusion of 90-degree steps
	auto FlipUpdate = [frame = (int32_t)0, flipStates = Vec2i{ true , true }, this](int32_t in_flipDir) mutable {
		if(frame % 2 == 0) {
			m_animSprite->SetFlipHori(flipStates.x);
			flipStates.x = !flipStates.x;
		}
		else if(frame % 2 == 1) {
			m_animSprite->SetFlipVert(flipStates.y);
			flipStates.y = !flipStates.y;
		}
		frame += in_flipDir;
	};
	while(true) {
		if(flipDir) {
			if(updateTimer > flipUpdateDelay) {
				FlipUpdate(flipDir);
				updateTimer = 0.0f;
			}
		}

		// Flicker out if stationary
		if(m_particleDef->animBlink && m_particleDef->speed < 10.0f) {
			flickerTimer -= DT();
			if(flickerTimer <= 0.0f && updateTimer > 0.1f) {
				co_await GetWorld()->Fade(m_animSprite, nullptr, 1.0f, false);
				DeferredDestroy();
				// Blink behavior
				//SetHidden(visibility);
				//visibility = !visibility;
				//updateTimer = 0.0f;
			}
			if(flickerTimer <= -1.5f) {
				DeferredDestroy();
			}
		}
		co_await Suspend();
		updateTimer += DT();
	}
}
void Particle::Bounce(const Math::BoxSweepResults& in_sweepResults, float in_amount) {
	// Modifies m_direction and m_speed to reflect (eyyy) a bounce off the world geometry
	auto vel = GetVel();
	auto velDir = vel.SignedAngleDeg();
	auto normalUpVec = in_sweepResults.m_normal;
	auto normalRightVec = normalUpVec.RotateDeg(-90.0f);
	auto bounceVec = (normalRightVec * vel.Dot(normalRightVec) + (normalUpVec * vel.Dot(normalUpVec) * -1.0f));
	m_particleDef->direction = bounceVec.SignedAngleDeg();
	m_particleDef->speed = bounceVec.Len() * in_amount;
	if(m_particleDef->speed < 30.0f) { //< TODO: this assumes a 60hz refresh rate, breaks in slowmo
		m_particleDef->rotationSpeed *= in_amount;
		m_animSprite->SetPlayRate(m_particleDef->animPlayrate *= in_amount);
	}
}
Vec2f Particle::Move(const Vec2f& in_pos, bool collideWorld) {
	auto newPos = in_pos - GetWorldPos();
	std::optional<Math::BoxSweepResults> sweepResults;
	if(collideWorld) {
		sweepResults = Math::SweepBoxAgainstGrid(GetCollisionBoxWorld(), newPos,
			GetCollisionTilesComp()->GetGridToWorldTransform(),
			GetCollisionTilesComp()->GetTileLayer()->GetGridBoundingBox(),
			[this](Vec2i in_testPos) {
				return GetCollisionTilesComp()->GetTileLayer()->GetTile(in_testPos) > 0;
			});
	}
	if(sweepResults.has_value()) {
		if(m_bounce) {
			Bounce(sweepResults.value(), m_bounce);
			auto hitVec = sweepResults.value().m_normal * sweepResults.value().m_dist;
			SetWorldPos({ GetWorldPos().x + hitVec.x, GetWorldPos().y + hitVec.y });
		}
		else {
			DeferredDestroy();
		}
	}
	else {
		SetWorldPos(in_pos);
	}
	return newPos;
}
void Particle::OnTouchPlayer(std::shared_ptr<Player> in_player) {
	auto effect = Actor::Spawn<Effect>(GetWorld(), GetWorldTransform(), m_particleDef->effectAnimName);
	DeferredDestroy();
}
void Particle::OnTouchCreature(std::shared_ptr<Creature> in_creature) {
	auto effect = Actor::Spawn<Effect>(GetWorld(), GetWorldTransform(), m_particleDef->effectAnimName);
	DeferredDestroy();
}
