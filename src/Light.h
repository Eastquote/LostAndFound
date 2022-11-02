#pragma once

#include "LightElement.h"

// Light actor with scalable core and penumbra LightElements that illuminate game world according to shader logic
class Light : public GameActor {
public:
	Light(std::string in_animNameCore, std::string in_animNamePenumbra);
	virtual void Initialize() override;
	void SetScale(float in_scale);
	void SetHidden(bool in_hide = true);

	// Interpolate LightElement scale over specified duration -- called without co-await ("fire and forget")
	TaskHandle<> InterpScale(float in_targetScale = 1.0f, float in_duration = 1.0f);

private:
	// Dispatches InterpScale params to managed, co_await-ed m_taskMgr.Run() calls
	Task<> ManageInterpScale(float in_targetScale, float in_duration);

	TaskHandle<> m_scaleTask;
	std::shared_ptr<LightElement> m_core;
	std::shared_ptr<LightElement> m_penumbra;
	std::string m_coreAnimName;
	std::string m_penumbraAnimName;
};
