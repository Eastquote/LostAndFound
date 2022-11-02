#include "TitleChar.h"
#include "GameWorld.h"
#include "Engine/EaseTo.h"
#include "Engine/Components/SpriteComponent.h"
#include <iostream>

TitleChar::TitleChar(Vec2f in_spawnTargetPos, std::string in_animName, int32_t in_startFrame, bool in_bSingleFrame, Vec2f in_spawnOffset, float in_speed, int32_t in_drawOrder)
	: m_spawnTargetPos(in_spawnTargetPos)
	, m_animName(in_animName)
	, m_startFrame(in_startFrame)
	, m_singleFrame(in_bSingleFrame)
	, m_spawnOffset(in_spawnOffset)
	, m_speed(in_speed)
	, m_drawOrder(in_drawOrder)
{
}
void TitleChar::Initialize() {
	GameActor::Initialize();

	m_sprite = MakeSprite();
	m_sprite->SetRelativePos(m_spawnOffset);
	m_sprite->SetRenderLayer("hud");
	m_sprite->SetComponentDrawOrder(m_drawOrder);
	m_spriteShadow = MakeSprite();
	if(m_singleFrame) {
		m_sprite->SetPlayRate(0.0f);
		m_spriteShadow->SetPlayRate(0.0f);
	}
	m_sprite->PlayAnim(m_animName, true, m_startFrame);
	auto shadowAnimName = m_animName + "Shadow";
	m_spriteShadow->PlayAnim(shadowAnimName, true, m_startFrame);
	m_spriteShadow->SetRelativePos(m_spawnOffset + m_shadowOffset);
	m_spriteShadow->SetRenderLayer("hud");
	m_spriteShadow->SetComponentDrawOrder(m_drawOrder - 25);
	m_direction = (-m_spawnOffset.Norm());
}
Task<> TitleChar::ManageActor() {
	// Start fade-in
	m_taskMgr.RunManaged(GetWorld()->Fade(m_sprite, nullptr, 1.0, true));
	m_taskMgr.RunManaged(GetWorld()->Fade(m_spriteShadow, nullptr, 1.0, true));

	const auto gravity = 0.092f * 60.0f;
	auto vel = GetVel();
	auto maxSpeed = 1.0f * 60.0f;
	auto minSpeed = 0.05f * 60.0f;
	while(true) {
		vel = GetVel();
		m_sprite->SetRelativePos(m_sprite->GetRelativePos() + (GetVel() * DT()));
		m_spriteShadow->SetRelativePos(m_sprite->GetRelativePos() + m_shadowOffset);
		auto gravityEffect = (m_sprite->GetRelativePos() * -1.0f).Norm() * gravity;
		vel += gravityEffect;
		m_speed = std::min(vel.Len(), maxSpeed);
		m_direction = vel.Norm();

		// Throttle down the max speed at aphelion
		if(m_speed < 2.0f && maxSpeed > minSpeed) {
			maxSpeed = std::max(maxSpeed * 0.9f, minSpeed);
		}
		co_await Suspend();
	}
}