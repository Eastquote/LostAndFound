#pragma once

#include "GameActor.h"

class TitleChar : public GameActor {
public:
	TitleChar(Vec2f in_spawnTargetPos, std::string in_animName, int32_t in_startFrame, bool in_bSingleFrame, Vec2f in_spawnOffsetPos, float in_translateDuration, int32_t in_drawOrder);
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;

private:
	Vec2f GetVel() {
		return m_direction.Norm() * m_speed;
	}
	Vec2f UpdateSign();
	std::string m_animName;
	Vec2f m_spawnTargetPos = Vec2i::Zero;
	Vec2f m_spawnOffset = Vec2i::Zero;
	int32_t m_startFrame = 0;
	float m_speed = 0.0f * 60.0f;
	Vec2f m_direction = Vec2f::Up;
	bool m_singleFrame = true;
	float m_translateDuration = 0.0f;
	int32_t m_drawOrder;
	Vec2f m_shadowOffset = Vec2f{ 3.0, -2.0 };
	std::shared_ptr<SpriteComponent> m_sprite;
	std::shared_ptr<SpriteComponent> m_spriteShadow;
};