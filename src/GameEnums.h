#pragma once

#include <inttypes.h>

// Repository of all the enums used by the non-engine systems in this game

enum class eDrawLayer
{
	Background = 0,
	Player = 200,
	Projectiles = 400,
};
enum eCollisionLayer : uint32_t
{						
	CL_None,
	CL_Player =			(1 << 0),
	CL_Enemy =			(1 << 1),
	CL_PlayerBullet =	(1 << 2),
	CL_EnemyBullet =	(1 << 3),
	CL_Pickup =			(1 << 4),
	CL_World =			(1 << 5),
	CL_Door =			(1 << 6),
	CL_PlayerBomb =     (1 << 7),
	CL_Lava =			(1 << 8),
	CL_SavePoint =		(1 << 9),
	CL_Armor =			(1 << 10),
	CL_Trigger =		(1 << 11),
};
enum class ePickupType
{
	SmallEnergy = 0,
	LargeEnergy = 1,
	Missile = 2,
	Bomb = 3,
};
enum eDamageFlag : uint32_t
{
	DF_None,
	DF_Beam =		(1 << 0),
	DF_Ice =		(1 << 1),
	DF_Charged =	(1 << 2),
	DF_TargetOnly = (1 << 3),
	DF_NoWalls =	(1 << 4),
	DF_Dash =		(1 << 5),
	DF_Explodes	=	(1 << 6),
	DF_Penetrates = (1 << 7),
	DF_Enemy =		(1 << 8),
	DF_Player =		(1 << 9),
	DF_NoKnockback =(1 << 10),
	DF_SuitBreaking =(1 << 11),
};
enum class eDamageType //< Deprecated, do not use
{
	None,
	PlayerBeam = 0,
	PlayerMissile = 1,
	PlayerHomingMissile = 2,
	PlayerBomb = 3,
	PlayerIceBeam = 4,
	PlayerWaveBeam = 5,
	EnemyBullet = 6,
	EnemyBomb = 7,
	Enemy = 8,
	Lava = 9,
	PlayerChargedBeam = 10,
	PlayerChargedIceBeam = 11,
	Boss1Bullet = 12,
};
enum class eHitResponse
{
	None,
	Hit = 1,
	Blocked = 2,
	Ignore = 3
};
enum class eTriggerTarget {
	None,
	Pirate = 1,
	Boss = 2,
};
enum eDir {
	Up,
	Down,
	Left,
	Right,
};
