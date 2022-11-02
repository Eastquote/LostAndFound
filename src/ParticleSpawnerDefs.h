#pragma once

#include "ParticleSpawner.h"

static ParticleSpawnerDef g_playerDeathExplosionDef = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",		// animName
	0,							// animStartFrame
	0.123f,						// animPlayrate
	0,							// animFlipRotation
	60,							// animFlipRotationFps
	true,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 0.1f, 0.1f },				// collisionBoxDims
	0.1f,						// sensorRadius
	true,						// collideWorld
	0.5f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 6.0f, 6.0f },				// particlelifetimeRange
	{ 0.0f, 360.0f },			// directionRange
	{ 120.0f, 620.0f },		    // speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ -2.5f, 2.5f },             // rotationSpeedRange
	{ 0.25f, 1.0f },             // scaleRange
	0.1f * 60.0f,						// gravity
	270.0f,						// gravityDir
	15,							// particlesMax
	{ 8.0f, 15.0f },			// spawnRange
	true,						// burst
	{ 4, 4 },					// burstParticleRange
	1.0f,						// burstRate
};

static ParticleSpawnerDef g_crawlerDeathExplosionDef0 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	0,							// animStartFrame
	0.0f,						// animPlayrate
	-1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};

static ParticleSpawnerDef g_crawlerDeathExplosionDef1 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	1,							// animStartFrame
	0.0f,						// animPlayrate
	1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};

static ParticleSpawnerDef g_crawlerDeathExplosionDef2 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	2,							// animStartFrame
	0.0f,						// animPlayrate
	-1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};
static ParticleSpawnerDef g_crawlerDeathExplosionDef3 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	3,							// animStartFrame
	0.0f,						// animPlayrate
	1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};
static ParticleSpawnerDef g_dropperDeathExplosionDef0 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	0,							// animStartFrame
	0.0f,						// animPlayrate
	-1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};

static ParticleSpawnerDef g_dropperDeathExplosionDef1 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	1,							// animStartFrame
	0.0f,						// animPlayrate
	1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};

static ParticleSpawnerDef g_dropperDeathExplosionDef2 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	2,							// animStartFrame
	0.0f,						// animPlayrate
	-1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};
static ParticleSpawnerDef g_dropperDeathExplosionDef3 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	3,							// animStartFrame
	0.0f,						// animPlayrate
	1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};
// Swooper:
static ParticleSpawnerDef g_swooperDeathExplosionDef0 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	0,							// animStartFrame
	0.0f,						// animPlayrate
	-1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};

static ParticleSpawnerDef g_swooperDeathExplosionDef1 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	1,							// animStartFrame
	0.0f,						// animPlayrate
	1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};

static ParticleSpawnerDef g_swooperDeathExplosionDef2 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	2,							// animStartFrame
	0.0f,						// animPlayrate
	-1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};
static ParticleSpawnerDef g_swooperDeathExplosionDef3 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	3,							// animStartFrame
	0.0f,						// animPlayrate
	1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};
// Cruiser:
static ParticleSpawnerDef g_cruiserDeathExplosionDef0 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	0,							// animStartFrame
	0.0f,						// animPlayrate
	-1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};

static ParticleSpawnerDef g_cruiserDeathExplosionDef1 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	1,							// animStartFrame
	0.0f,						// animPlayrate
	1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};

static ParticleSpawnerDef g_cruiserDeathExplosionDef2 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	2,							// animStartFrame
	0.0f,						// animPlayrate
	-1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};
static ParticleSpawnerDef g_cruiserDeathExplosionDef3 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	3,							// animStartFrame
	0.0f,						// animPlayrate
	1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};
// Piper:
static ParticleSpawnerDef g_piperDeathExplosionDef0 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	0,							// animStartFrame
	0.0f,						// animPlayrate
	-1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};

static ParticleSpawnerDef g_piperDeathExplosionDef1 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	1,							// animStartFrame
	0.0f,						// animPlayrate
	1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};

static ParticleSpawnerDef g_piperDeathExplosionDef2 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	2,							// animStartFrame
	0.0f,						// animPlayrate
	-1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};
static ParticleSpawnerDef g_piperDeathExplosionDef3 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGeneric",	// animName
	3,							// animStartFrame
	0.0f,						// animPlayrate
	1,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 8.0f, 8.0f },				// collisionBoxDims
	4.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.4f, 0.4f },				// particlelifetimeRange
	{ 75.0f, 105.0f },			// directionRange
	{ 15.0f, 15.0f },			// speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.2f,						// gravity
	270.0f,						// gravityDir
	1,							// particlesMax
	{ 1.0f, 10.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};
static ParticleSpawnerDef g_debrisSpawnerLg1 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGenericDark",	// animName
	0,							// animStartFrame
	0.123f,						// animPlayrate
	0,							// animFlipRotation
	60,							// animFlipRotationFps
	true,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 0.1f, 0.1f },				// collisionBoxDims
	0.1f,						// sensorRadius
	true,						// collideWorld
	0.5f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 6.0f, 6.0f },				// particlelifetimeRange
	{ 0.0f, 360.0f },			// directionRange
	{ 120.0f, 380.0f },		    // speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ -2.5f, 2.5f },             // rotationSpeedRange
	{ 0.25f, 1.0f },             // scaleRange
	0.1f * 60.0f,						// gravity
	270.0f,						// gravityDir
	15,							// particlesMax
	{ 0.0f, 0.0f },			// spawnRange
	true,						// burst
	{ 7, 7 },					// burstParticleRange
	1.0f,						// burstRate
};
static ParticleSpawnerDef g_debrisSpawnerSm1 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGenericDark",	// animName
	0,							// animStartFrame
	0.123f,						// animPlayrate
	0,							// animFlipRotation
	60,							// animFlipRotationFps
	true,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 0.1f, 0.1f },				// collisionBoxDims
	0.1f,						// sensorRadius
	true,						// collideWorld
	0.4f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 6.0f, 6.0f },				// particlelifetimeRange
	{ 0.0f, 360.0f },			// directionRange
	{ 40.0f, 100.0f },		    // speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ -2.5f, 2.5f },             // rotationSpeedRange
	{ 0.25f, 1.0f },             // scaleRange
	0.1f * 60.0f,						// gravity
	270.0f,						// gravityDir
	15,							// particlesMax
	{ 0.0f, 0.0f },			// spawnRange
	true,						// burst
	{ 4, 4 },					// burstParticleRange
	1.0f,						// burstRate
};
static ParticleSpawnerDef g_emberSpawnerLg1 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisEmberLg",	// animName
	0,							// animStartFrame
	0.166f,						// animPlayrate
	0,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 2.0f, 2.0f },				// collisionBoxDims
	2.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.8f, 1.0f },				// particlelifetimeRange
	{ 45.0f, 135.0f },			// directionRange
	{ 60.0f, 120.0f },			    // speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.1f * 60.0f,				// gravity
	270.0f,						// gravityDir
	15,							// particlesMax
	{ 8.0f, 15.0f },			// spawnRange
	true,						// burst
	{ 4, 4 },					// burstParticleRange
	1.0f,						// burstRate
};
static ParticleSpawnerDef g_emberSpawnerLg2 = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisEmberLg",	// animName
	0,							// animStartFrame
	0.333f,						// animPlayrate
	0,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 2.0f, 2.0f },				// collisionBoxDims
	2.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.35f, 0.5f },				// particlelifetimeRange
	{ 65.0f, 115.0f },			// directionRange
	{ 120.0f, 210.0f },			    // speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.1f * 60.0f,				// gravity
	270.0f,						// gravityDir
	15,							// particlesMax
	{ 8.0f, 15.0f },			// spawnRange
	true,						// burst
	{ 2, 3 },					// burstParticleRange
	1.0f,						// burstRate
};
static ParticleSpawnerDef g_emberSpawnerCharged = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisEmberLg",	// animName
	0,							// animStartFrame
	0.166f * 4.0f,						// animPlayrate
	0,							// animFlipRotation
	60,							// animFlipRotationFps
	false,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 2.0f, 2.0f },				// collisionBoxDims
	2.0f,						// sensorRadius
	false,						// collideWorld
	0.0f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 0.225f, 0.25f },				// particlelifetimeRange
	{ 0.0f, 360.0f },			// directionRange
	{ 120.0f, 220.0f },			    // speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 0.0f, 0.0f },             // rotationSpeedRange
	{ 0.25f, 0.6f },             // scaleRange
	0.0f,				// gravity
	270.0f,						// gravityDir
	15,							// particlesMax
	{ 8.0f, 15.0f },			// spawnRange
	true,						// burst
	{ 8, 8 },					// burstParticleRange
	1.0f,						// burstRate
};
static ParticleSpawnerDef g_testSpawner = {
	Vec2f::Zero,				// spawnPosOffset
	"DebrisNew/DebrisGenericDark",	// animName
	0,							// animStartFrame
	0.123f,						// animPlayrate
	0,							// animFlipRotation
	60,							// animFlipRotationFps
	true,						// animBlink
	"Explosion2/Explosion",		// effectAnimName
	{ 0.1f, 0.1f },				// collisionBoxDims
	0.1f,						// sensorRadius
	true,						// collideWorld
	0.5f,						// bounce
	CL_None,					// category
	CL_None,					// mask
	{ 0.1f, 0.1f },				// spawnerLifetimeRange
	{ 6.0f, 6.0f },				// particlelifetimeRange
	{ 90.0f, 90.0f },			// directionRange
	{ 320.0f, 320.0f },		    // speedRange
	{ 0.0f, 360.0f },		    // rotationRange
	{ 2.5f, 2.5f },             // rotationSpeedRange
	{ 1.0f, 1.0f },             // scaleRange
	0.1f * 60.0f,						// gravity
	270.0f,						// gravityDir
	15,							// particlesMax
	{ 8.0f, 15.0f },			// spawnRange
	true,						// burst
	{ 1, 1 },					// burstParticleRange
	1.0f,						// burstRate
};