#include "LightElement.h"
#include "GameWorld.h"
#include "Engine/MathEasings.h"
#include "Engine/Components/SpriteComponent.h"

LightElement::LightElement(const std::string& in_anim)
	: m_animName(in_anim)
{
}
void LightElement::Initialize() {
	GameActor::Initialize();
	m_sprite = MakeSprite();
	m_sprite->PlayAnim(m_animName, true);
	m_sprite->SetRenderLayer("lights");
}
void LightElement::SetScale(float in_scale) {
	m_scaleTask = {};
	m_sprite->SetRelativeScale(Vec2f{ in_scale, in_scale });
}
void LightElement::SetHidden(bool in_hide) {
	m_sprite->SetHidden(in_hide);
}
TaskHandle<> LightElement::InterpScale(float in_targetScale, float in_duration) {
	// Interp this particular LightElement's uniform scale to the target value, over the specified duration
	m_scaleTask = m_taskMgr.Run(ManageInterpScale(in_targetScale, in_duration)); //< Dispatch task to TaskManager
	return m_scaleTask;
}
Task<> LightElement::ManageInterpScale(float in_targetScale, float in_duration) {
	std::shared_ptr<GameWorld> world = GetWorld();
	float startScale = m_sprite->GetRelativeScale().x;
	float scaleDelta = in_targetScale - startScale;
	float elapsedTime = 0.0f;
	float interpAlpha = 0.0f;
	while(elapsedTime < in_duration) {
		elapsedTime += DT();
		// Calculate this frame's location on a smooth, normalized interpolation curve
		interpAlpha = std::clamp(world->EaseAlpha(elapsedTime, in_duration, Math::EaseInOutSmootherstep), 0.0f, 1.0f);
		// Set new scale for this frame
		float newScale = startScale + (interpAlpha * scaleDelta);
		m_sprite->SetRelativeScale(Vec2f{ newScale, newScale });
		co_await Suspend();
	}
	// Make sure there's no floating point funny business
	m_sprite->SetRelativeScale(Vec2f{ in_targetScale, in_targetScale });
}
