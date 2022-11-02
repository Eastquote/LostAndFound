#include "Light.h"

Light::Light(std::string in_animNameCore, std::string in_animNamePenumbra)
	: m_coreAnimName(in_animNameCore)
	, m_penumbraAnimName(in_animNamePenumbra)
{
}
void Light::Initialize() {
	GameActor::Initialize();

	// Setup LightElements
	m_core = Spawn<LightElement>({}, m_coreAnimName);
	m_penumbra = Spawn<LightElement>({}, m_penumbraAnimName);
	m_core->AttachToActor(AsShared<Actor>(), false);
	m_penumbra->AttachToActor(AsShared<Actor>(), false);
	m_core->SetDrawLayer(10);
	m_penumbra->SetDrawLayer(0);
}
void Light::SetScale(float in_scale) {
	m_scaleTask = {};
	m_core->SetScale(in_scale);
	m_penumbra->SetScale(in_scale);
}
void Light::SetHidden(bool in_hide) {
	m_core->SetHidden(in_hide);
	m_penumbra->SetHidden(in_hide);
}
TaskHandle<> Light::InterpScale(float in_targetScale, float in_duration) {
	// Interpolate entire Light actor to target scale over the specified duration (in seconds)
	m_scaleTask = m_taskMgr.Run(ManageInterpScale(in_targetScale, in_duration)); //< Dispatch task to TaskManager
	return m_scaleTask;
}
Task<> Light::ManageInterpScale(float in_targetScale, float in_duration) {
	co_await WaitForAll({ //< Guarantees that both LightElements are done scaling before proceeding
		m_core->InterpScale(in_targetScale, in_duration), //< The light's bright center
		m_penumbra->InterpScale(in_targetScale, in_duration), //< The dim "falloff" halo around the center
	});
}
