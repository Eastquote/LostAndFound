#include "Effect.h"

#include "ProjectileManager.h"
#include "GameWorld.h"
#include "Engine/Components/SpriteComponent.h"

//--- EFFECT ---//

Effect::Effect(std::string in_animName, std::optional<std::shared_ptr<Actor>> in_attachTo)
	: m_animName(std::move(in_animName))
{
	if(in_attachTo.has_value()) {
		AttachToActor(in_attachTo.value());
	}
}
void Effect::Initialize() {
	GameActor::Initialize();

	// Setup sprite
	m_effectSprite = MakeSprite(Transform::Identity);
	m_effectSprite->SetRenderLayer("hud");

	// Piggyback on ProjectileManager's functionality to handle object registration
	auto projMgr = GetWorld()->GetProjectileManager();
	projMgr->RegisterEffect(AsShared<Effect>());
}
Task<> Effect::ManageActor() {
	co_await m_effectSprite->PlayAnim(m_animName, false);
	DeferredDestroy();
}