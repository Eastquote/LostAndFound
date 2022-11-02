#pragma once

#include "Projectiles/HomingMissile.h"

// HomingMissile ProjectileDefs
static ProjectileDef g_playerHomingMissileDef = {
	{ 1.0f, 1.0f },
	0.5f,
	10.0f,
	3.5f * 60.0f,
	8,
	"SuperMissile/Normal",
	CL_PlayerBullet,
	CL_Enemy | CL_Armor,
	DF_Player | DF_Explodes | DF_TargetOnly | DF_Charged,
	"Explosion2/Explosion",
	"Blank",
	eDamageType::PlayerHomingMissile,
	&SpawnProjectile<HomingMissile>
};

static ProjectileDef g_bossHomingMissileDef = {
	{ 1.0f, 1.0f },
	13.0f,
	10.0f,
	2.5f * 60.0f,
	100,
	"ChargeEffect/Charged",
	CL_EnemyBullet,
	CL_Player,
	DF_Enemy | DF_Explodes | DF_TargetOnly | DF_Charged | DF_SuitBreaking,
	"Explosion2/Explosion",
	"Blank",
	eDamageType::Boss1Bullet,
	&SpawnProjectile<HomingMissile>
};