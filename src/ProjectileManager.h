#pragma once

#include "Projectile.h"
#include "Effect.h"
#include <memory>
#include <vector>

// Simple projectile registry and cleaner -- spawns hit effects, erases invalid (destroyed) entries each tick
class ProjectileManager : public Actor {
public:
	virtual void Initialize() override;
	virtual void Update() override;

	size_t GetProjectileCount() { return m_projectiles.size(); }

	void RegisterProjectile(std::shared_ptr<Projectile> in_proj);
	void RegisterEffect(std::shared_ptr<Effect> in_effect);

private:
	std::vector<std::shared_ptr<Projectile>> m_projectiles;
	std::vector<std::shared_ptr<Effect>> m_effects;
};
