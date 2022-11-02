#pragma once

#include "GameActor.h"

// Single element of a Light actor, used to express either the core or penumbra elements of a Light
class LightElement : public GameActor {
public:
	LightElement(const std::string& in_anim);
	virtual void Initialize() override;
	void SetScale(float in_scale);
	void SetHidden(bool in_hide = true);

	// Interpolate LightElement scale over specified duration -- called without co-await ("fire and forget")
	TaskHandle<> InterpScale(float in_targetScale = 1.0f, float in_duration = 1.0f);

private:
	// Implementation of scale interpolation behavior (always run as a managed Task by InterpScale())
	Task<> ManageInterpScale(float in_targetScale, float in_duration);

	TaskHandle<> m_scaleTask;
	std::shared_ptr<SpriteComponent> m_sprite;
	std::string m_animName;
	bool m_bHoriOnly = false;
};
