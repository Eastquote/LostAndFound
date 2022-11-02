#include "ProjectileManager.h"
#include "AudioManager.h"

#include "GameWorld.h"
#include "Algorithms.h"

void ProjectileManager::Initialize() {
	Actor::Initialize();
}
void ProjectileManager::Update() {
	Actor::Update();
	for(auto projectile : m_projectiles) {
		if(!projectile->StillAlive() && !projectile->IsDestroyed()) {
			auto hitEffectAnimName = projectile->GetHitEffectAnimName();
			auto effect = Actor::Spawn<Effect>(GameWorld::Get(), { projectile->GetWorldPos() }, hitEffectAnimName);
			auto hitEffectSoundName = projectile->GetHitEffectSoundName();
			AudioManager::Get()->PlaySound(hitEffectSoundName);
			projectile->DeferredDestroy();
		}
	}
	EraseInvalid(m_projectiles);
	EraseInvalid(m_effects);
}
void ProjectileManager::RegisterProjectile(std::shared_ptr<Projectile> in_proj) {
	m_projectiles.push_back(in_proj);
}
void ProjectileManager::RegisterEffect(std::shared_ptr<Effect> in_effect) {
	m_effects.push_back(in_effect);
}
