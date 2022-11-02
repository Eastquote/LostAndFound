#include "AimReticle.h"

#include "GameWorld.h"
#include "AimReticleManager.h"
#include "Engine/Components/SpriteComponent.h"

AimReticle::AimReticle(std::shared_ptr<GameActor> in_target)
	: m_target(in_target)
{
}
void AimReticle::Initialize() {
	GameActor::Initialize();
	m_sprite = MakeSprite();
	m_sprite->SetRenderLayer("hud");
	auto aimReticleMgr = GetWorld()->GetAimReticleManager();
	aimReticleMgr->RegisterAimReticle(AsShared<AimReticle>());
	auto target = m_target.lock();
	SQUID_RUNTIME_CHECK(IsAlive(target), "AimReticle target must still be alive at initialization time");
	if(target){
		SetWorldPos(target->GetWorldPos());
		target->SetTargeted(true);
	}
}
void AimReticle::Destroy() {
	UnlinkTarget();
	GameActor::Destroy();
}
void AimReticle::RequestDestroy() {
	m_bDestroyRequested = true;
}
void AimReticle::UnlinkTarget() {
	// n.b. UnlinkTarget() *doesn't* unset the m_target var within this reticle
	// (This allows the reticle sprite to keep moving with the target while it's exiting)
	auto target = m_target.lock();
	if(IsAlive(target)) {
		target->SetTargeted(false);
	}
}
Task<> AimReticle::TrackTarget() {
	auto world = GetWorld();
	while(auto target = m_target.lock()) {
		SetWorldPos(target->GetWorldPos());
		co_await Suspend();
	}
}
Task<> AimReticle::ManageActor() {
	// Start tracking the target's position, and play the appear anim
	auto trackTargetTask = m_taskMgr.Run(TrackTarget());
	co_await m_sprite->PlayAnim("AimReticle/Appear", false);
	m_sprite->PlayAnim("AimReticle/Idle", true);

	// Wait until we should stop targeting
	co_await WaitUntil([this]{
		auto target = m_target.lock();
		return !GetWorld()->GetPlayer() || !IsAlive(target) || m_bDestroyRequested;
	});

	// Give the sprite a graceful exit
	UnlinkTarget();
	m_sprite->SetPlayRate(2.0f);
	co_await m_sprite->PlayAnim("AimReticle/Disappear", false);
	DeferredDestroy();
}
