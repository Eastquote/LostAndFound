#pragma once

#include "GameActor.h"

// Simple class to draw one-sprite hand-animated effects on screen
class Effect : public GameActor {
public:
	Effect(std::string in_animName = "Util/Blank", std::optional<std::shared_ptr<Actor>> in_attachTo = std::nullopt);
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;

private:
	std::shared_ptr<SpriteComponent> m_effectSprite;
	std::string m_animName;
};