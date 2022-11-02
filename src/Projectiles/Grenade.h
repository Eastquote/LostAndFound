#pragma once

#include "Projectile.h"

// Grenade projectile subclass which is affected by gravity and bounces on world geometry
class Grenade : public Projectile {
public:
	using Projectile::Projectile;
	virtual void Initialize() override;
	virtual Vec2f ModifyMovement() override;
};
