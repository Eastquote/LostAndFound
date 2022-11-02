#pragma once

#include "Projectiles/WaveBeam.h"

// WaveBeam ProjectileDefs
static ProjectileDef g_playerBulletWaveDef = {
	{ 0.1f, 0.1f },
	3.0f,
	0.3f,
	4.0f * 60.0f,
	1,
	"ShotBeam/Idle",
	CL_PlayerBullet,
	CL_Enemy | CL_Armor | CL_Door | CL_Pickup | CL_EnemyBullet,
	DF_Player | DF_Beam | DF_NoWalls,
	"ShotBeamHitEffect/Idle",
	"Blank",
	eDamageType::PlayerWaveBeam,
	&SpawnProjectile<WaveBeam>
};
static ProjectileDef g_playerBulletLongWaveDef = {
	{ 0.1f, 0.1f },
	3.0f,
	1.3f,
	4.0f * 60.0f,
	1,
	"ShotBeam/Idle",
	CL_PlayerBullet,
	CL_Enemy | CL_Armor | CL_Door | CL_Pickup | CL_EnemyBullet,
	DF_Player | DF_Beam | DF_NoWalls,
	"ShotBeamHitEffect/Idle",
	"Blank",
	eDamageType::PlayerWaveBeam,
	&SpawnProjectile<WaveBeam>
};