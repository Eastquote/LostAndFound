#pragma once

#include "Projectiles/Grenade.h"

// Grenade ProjectileDefs
static ProjectileDef g_playerGrenadeDef = {
	{ 2.0f, 2.0f },
	6.0f,
	3.0f,
	4.7f * 60.0f,
	4,
	"Grenade/GrenadeA",
	CL_PlayerBullet,
	CL_Player | CL_Armor | CL_Enemy | CL_Door | CL_Pickup,
	DF_Player | DF_Explodes,
	"Explosion2/Explosion",
	"Blank",
	eDamageType::PlayerBomb,
	&SpawnProjectile<Grenade>
};
static ProjectileDef g_enemyGrenadeDef = {
	{ 2.0f, 2.0f },
	6.0f,
	1.4f,
	2.0f * 60.0f,
	4,
	"Grenade/GrenadeA",
	CL_EnemyBullet,
	CL_Player | CL_Door | CL_Pickup | CL_PlayerBullet,
	DF_Enemy | DF_Explodes,
	"Explosion2/Explosion",
	"Blank",
	eDamageType::EnemyBomb,
	&SpawnProjectile<Grenade>
};
static ProjectileDef g_playerGrenadeChargedDef = {
	{ 2.0f, 2.0f },
	6.0f,
	3.0f,
	1.0f * 60.0f,
	7,
	"Grenade/GrenadeA",
	CL_PlayerBullet,
	CL_Enemy | CL_Door | CL_Pickup,
	DF_Player | DF_Explodes | DF_Charged,
	"Explosion2/Explosion",
	"Blank",
	eDamageType::PlayerBomb,
	& SpawnProjectile<Grenade>
};