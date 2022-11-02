#include "Door.h"

#include "GameWorld.h"
#include "GameEnums.h"
#include "Projectile.h"
#include "Player.h"
#include "PlayerStatus.h"
#include "Bomb.h"
#include "Task.h"
#include "Creature.h"
#include "AudioManager.h"
#include "CameraManager.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/TilesComponent.h"
#include "Engine/TileMap.h"
#include "Engine/DebugDrawSystem.h"
#include "Engine/MathGeometry.h"
#include <iostream>

static std::string ColorToAnim(eDoorColor color, bool isClosing) {
	switch(color) {
	default:
	case eDoorColor::None:
		return isClosing ? "Util/Blank" : "Util/Blank";
	case eDoorColor::Blue:
		return isClosing ? "Door/BlueClosing" : "Door/Blue";
	case eDoorColor::Red:
		return isClosing ? "Door/RedClosing" : "Door/Red";
	}
}

//--- DOOR CODE ---//

Door::Door(eDoorColor in_color, int32_t in_objId, bool in_bIsVertical)
	: m_color(in_color)
	, m_objId(in_objId)
	, m_bIsVertical(in_bIsVertical)
{
}
void Door::Initialize() {
	GameActor::Initialize();

	// Set up sprites
	SetDrawLayer(-2);
	auto openAnim = ColorToAnim(m_color, false);
	m_doorSpriteLeft = MakeSprite(Transform::Identity);
	m_doorSpriteLeft->SetRelativePos({ -20.0f, 0.0f });
	m_doorSpriteLeft->PlayAnim(openAnim, false, m_bIsOpen ? 12 : 0);
	m_doorSpriteLeft->SetPlayRate(0.0f);
	m_doorSpriteLeft->SetFlipHori(true);
	//m_doorSpriteLeft->SetComponentDrawOrder(9);

	m_doorSpriteRight = MakeSprite(Transform::Identity);
	m_doorSpriteRight->SetRelativePos({ 20.0f, 0.0f });
	m_doorSpriteRight->PlayAnim(openAnim, false, m_bIsOpen ? 12 : 0);
	m_doorSpriteRight->SetPlayRate(0.0f);
	//m_doorSpriteRight->SetComponentDrawOrder(9);

	m_doorSpriteCenter = MakeSprite(Transform::Identity);
	m_doorSpriteCenter->PlayAnim("DoorCenter/Blue", false);
	if(m_color == eDoorColor::Red) {
		m_doorSpriteCenter->PlayAnim("DoorCenter/Red", false);
	}
	m_doorSpriteCenter->SetRenderLayer("fgTiles");
	//m_doorSpriteCenter->SetComponentDrawOrder(10);
	m_doorSpriteCenterLit = MakeSprite(Transform::Identity);
	m_doorSpriteCenterLit->PlayAnim("DoorCenter/BlueLit", false);
	m_doorSpriteCenterLit->SetRenderLayer("fgLightMask");

	// Set up blocking tiles
	auto pos = GetWorldPos();
	auto startingPos = pos + Vec2f{ -24.0f, -24.0f };
	float gridSize = 16.0f;
	for(auto i = 0; i < 4; i++) {
		for(auto j = 0; j < 3; j++) {
			auto tilePos = startingPos + Vec2f{ (i * gridSize), (j * gridSize), };
			m_tileLocations.push_back(tilePos);
		}
	}
	SetDoorTileBlocking(!m_bIsOpen);

	// Set up sensors
	SensorShape playerBulletSensorShape;
	playerBulletSensorShape.SetBox({ 48.0f, 48.0f });
	auto playerBulletSensor = MakeSensor(Transform::Identity, playerBulletSensorShape);
	playerBulletSensor->SetFiltering(CL_Door, CL_PlayerBullet | CL_PlayerBomb);
	playerBulletSensor->SetTouchCallback([this](bool in_beginning, std::shared_ptr<SensorComponent> in_other) {
		if(in_beginning && !m_bIsOpen) {
			if(std::dynamic_pointer_cast<Projectile>(in_other->GetActor())) {
				OnTouchPlayerBullet(std::dynamic_pointer_cast<Projectile>(in_other->GetActor()));
			}
			else if(std::dynamic_pointer_cast<Bomb>(in_other->GetActor())) {
				OnTouchPlayerBomb(std::dynamic_pointer_cast<Bomb>(in_other->GetActor()));
			}
		}
	});
	SensorShape playerSensorShape;
	playerSensorShape.SetBox({ 48.0f, 48.0f });
	auto playerSensor = MakeSensor(Transform::Identity, playerSensorShape);
	playerSensor->SetFiltering(CL_Door, CL_Player | CL_Enemy);
	playerSensor->SetTouchCallback([this](bool in_beginning, std::shared_ptr<SensorComponent> in_other) {
		if(in_beginning) {
			if(std::dynamic_pointer_cast<Player>(in_other->GetActor())) {
				OnTouchPlayer(std::dynamic_pointer_cast<Player>(in_other->GetActor()));
			}
		}
		else if(!in_beginning) {
			if(std::dynamic_pointer_cast<Player>(in_other->GetActor())) {
				OnUnTouchPlayer(std::dynamic_pointer_cast<Player>(in_other->GetActor()));
			}
		}
	});

	// Set up behavior
	auto playerStatus = GetWorld()->GetPlayerStatus();
	auto unlocked = playerStatus->IsObjectUnlocked(m_objId);
	if((m_color == eDoorColor::Red && unlocked) || m_color == eDoorColor::None) {
		m_bIsOpen = true;
		m_bIsActive = true;
		SetDoorTileBlocking(false);
		m_doorSpriteRight->SetHidden(true);
		m_doorSpriteLeft->SetHidden(true);
	}
	m_doorVolume.SetCenter_Move(GetWorldPos());
}
Task<> Door::TransitionPlayerPosition(bool in_bIsEntering) {
	auto world = GetWorld();
	auto doorPos = GetWorldPos();
	auto playerPos = world->GetPlayerWorldPos();
	auto isTransitioningPositive = m_bIsVertical ? playerPos.y < doorPos.y : playerPos.x < doorPos.x;
	auto dir = isTransitioningPositive ? 1.0f : -1.0f;

	// If entering doorway, targetOffset is right at edge of room boundary; otherwise, it's just beyond exit side of doorway
	auto targetOffset = in_bIsEntering ? -1.0f : 41.0f;
	auto targetPos = (m_bIsVertical ? doorPos.y : doorPos.x) + (targetOffset * dir);
	auto relevantPosAxis = m_bIsVertical ? playerPos.y : playerPos.x;
	auto transitionSpeed = 1.0f * 60.0f; //< Pixels/sec

	// Move player through doorway until they their target pos
	while(isTransitioningPositive ? (relevantPosAxis < targetPos) : (relevantPosAxis > targetPos)) {
		auto targetOffset = m_bIsVertical ? Vec2f{ 0.0f, transitionSpeed * dir * DT() } : Vec2f{ transitionSpeed * dir * DT(), 0.0f };
		world->SetPlayerWorldPos(playerPos + targetOffset);
		playerPos = world->GetPlayerWorldPos();
		relevantPosAxis = m_bIsVertical ? playerPos.y : playerPos.x;
		co_await Suspend();
	}
}
Task<> Door::ManageActor() {
	//auto debugDrawTask = m_taskMgr.Run(DrawDoorVolume());
	auto openAnim = ColorToAnim(m_color, false);
	auto closeAnim = ColorToAnim(m_color, true);
	auto world = GetWorld();
	auto doorPos = GetWorldPos();
	auto doorTimer = 0.0f;
	auto doorOpenTime = 2.85f;
	auto explosiveHit = false;
	auto explosiveHealthLastFrame = m_explosiveHealth;
	while(true) {
		explosiveHit = explosiveHealthLastFrame > m_explosiveHealth;
		if(m_bIsActive) {
			auto playerPos = world->GetPlayerWorldPos();
			auto isTransitioningPositive = m_bIsVertical ? playerPos.y < doorPos.y : playerPos.x < doorPos.x;

			// Flash door
			if(explosiveHit) {
				m_doorSpriteRight->SetPlayRate(1.0f);
				m_doorSpriteLeft->SetPlayRate(1.0f);
				m_doorSpriteLeft->PlayAnim("Door/RedFlash", false);
				co_await m_doorSpriteRight->PlayAnim("Door/RedFlash", false);
			}

			// Open door
			if(m_bDoorJustActivated && !m_bIsOpen) {
				AudioManager::Get()->PlaySound("Door");
				m_doorSpriteRight->SetPlayRate(1.0f);
				m_doorSpriteLeft->SetPlayRate(1.0f);
				m_doorSpriteLeft->PlayAnim("Door/BlueFlash", false);
				co_await m_doorSpriteRight->PlayAnim("Door/BlueFlash", false);
				m_doorSpriteRight->PlayAnim(openAnim, false);
				m_doorSpriteLeft->PlayAnim(openAnim, false);
				SetDoorTileBlocking(false);
				m_bDoorJustActivated = false;
				m_bIsOpen = true;
			}
			if(m_bIsOpen && m_color == eDoorColor::Blue) {

				// Wait for player to enter
				if(doorTimer <= doorOpenTime && !m_bPlayerInDoorway && !m_bTransitionInProgress){
					doorTimer += DT();
				}

				// If player enters doorway while open, start moving them through
				else if(doorTimer <= doorOpenTime && m_bPlayerInDoorway && !m_bTransitionInProgress) {
					m_bTransitionInProgress = true;
				}

				// If open time expires before player enters, close doors
				else if(doorTimer > doorOpenTime && !m_bPlayerInDoorway && !m_bTransitionInProgress) {
					SetDoorTileBlocking(true);
					m_doorSpriteRight->PlayAnim(closeAnim, false);
					m_doorSpriteLeft->PlayAnim(closeAnim, false);
					AudioManager::Get()->PlaySound("Door");
					co_await WaitSeconds(0.3f);
					m_bIsOpen = false;
					m_bTransitionInProgress = false;
					doorTimer = 0.0f;
				}
				if(m_bTransitionInProgress) {
					// Move player into doorway
					co_await TransitionPlayerPosition(true);

					// Close doors
					m_doorSpriteRight->PlayAnim(closeAnim, false);
					m_doorSpriteLeft->PlayAnim(closeAnim, false);
					AudioManager::Get()->PlaySound("Door");

					// Move camera into next room
					world->GetCameraManager()->StartRoomTransition(AsShared<Door>(), isTransitioningPositive);
					co_await WaitSeconds(2.13f);

					// Open doors again
					m_doorSpriteRight->PlayAnim(openAnim, false);
					m_doorSpriteLeft->PlayAnim(openAnim, false);
					AudioManager::Get()->PlaySound("Door");

					// Move player out other side of doorway
					co_await TransitionPlayerPosition(false);

					// Reactivate blocking physics on door
					SetDoorTileBlocking(true);

					// Close doors
					m_doorSpriteRight->PlayAnim(closeAnim, false);
					m_doorSpriteLeft->PlayAnim(closeAnim, false);
					AudioManager::Get()->PlaySound("Door");
					co_await WaitSeconds(0.3f);
					m_bIsOpen = false;
					m_bTransitionInProgress = false;
					doorTimer = 0.0f;
				}
			}

			// For doors that are permanently open
			else if(m_bIsOpen && m_color == eDoorColor::Red || m_color == eDoorColor::None) {
				auto playerStatus = GetWorld()->GetPlayerStatus();
				playerStatus->UnlockObject(m_objId);
				co_await WaitUntil([this]() { return m_bPlayerInDoorway; });
				// Move player into doorway
				co_await TransitionPlayerPosition(true);
				// Move camera into next room
				world->GetCameraManager()->StartRoomTransition(AsShared<Door>(), isTransitioningPositive, m_bIsVertical);
				co_await WaitSeconds(2.13f);
				// Move player out other side of doorway
				co_await TransitionPlayerPosition(false);
			}
		}
		explosiveHealthLastFrame = m_explosiveHealth;
		co_await Suspend();
	}
}
void Door::OnTouchPlayerBullet(std::shared_ptr<Projectile> in_projectile) {
	//printf("DOOR: touched bullet!\n");
	auto damageFlags = in_projectile->GetDamageInfo().m_damageFlags;
	auto playerExplosiveHit = (damageFlags & (DF_Player & DF_Explodes)) == (DF_Player & DF_Explodes);
	if(m_color == eDoorColor::Blue) {
		m_bDoorJustActivated = true;
	}

	// Grenades and missiles can open red doors if you have enough to "kill" the door
	else if(m_color == eDoorColor::Red && playerExplosiveHit) {
		m_explosiveHealth -= 1;
		//FlashRedDoor();
		if(m_explosiveHealth <= 0) {
			m_bDoorJustActivated = true;
		}
	}
}
void Door::FlashRedDoor() {
	auto task = m_taskMgr.RunManaged(FlashRedDoorTask());
}
Task<> Door::FlashRedDoorTask() {
	m_doorSpriteLeft->SetPlayRate(1.0f);
	m_doorSpriteRight->SetPlayRate(1.0f);
	m_doorSpriteLeft->PlayAnim("Door/RedFlash", false);
	co_await m_doorSpriteRight->PlayAnim("Door/RedFlash", false);
	m_doorSpriteLeft->SetPlayRate(0.0f);
	m_doorSpriteRight->SetPlayRate(0.0f);
	m_doorSpriteLeft->PlayAnim("Door/Red", false);
	m_doorSpriteRight->PlayAnim("Door/Red", false);
}
void Door::OnTouchPlayer(std::shared_ptr<Player> in_player) {
	//printf("DOOR::Door Touched Player!");
	if(m_bIsOpen) {
		in_player->SetDoorTransition(true);
		m_bPlayerInDoorway = true;
	}
}
void Door::OnTouchCreature(std::shared_ptr<Creature> in_creature) {
	//printf("DOOR::Door Touched Creature!");
	m_creaturesInDoorway += 1;
}
void Door::OnTouchPlayerBomb(std::shared_ptr<Bomb> in_bomb) {
	//printf("DOOR::Door Touched Bomb!");
	if(m_color == eDoorColor::Blue) {
		m_bDoorJustActivated = true;
	}
}
void Door::OnUnTouchPlayer(std::shared_ptr<Player> in_player) {
	//printf("DOOR::      Door UNTouched Player!");
	in_player->SetDoorTransition(false);
	m_bPlayerInDoorway = false;
}
void Door::SetDoorTileBlocking(bool in_bIsBlocking) {
	auto tileIdx = in_bIsBlocking ? 2 : 0;
	auto collisionTiles = GetWorld()->GetCollisionTilesComp();
	auto collisionLayer = collisionTiles->GetTileLayer();
	for(const auto& pos : m_tileLocations) {
		auto gridPos = collisionTiles->WorldPosToGridPos(pos);
		collisionLayer->SetTile(gridPos, tileIdx);
	}
}
bool Door::DoorwayClear() {
	return (!m_bPlayerInDoorway && m_creaturesInDoorway == 0);
}
Task<> Door::DrawDoorVolume() {
	while(true) {
		if(DoorwayClear()) {
			DrawDebugBox(m_doorVolume, sf::Color::Green);
		}
		else if(!DoorwayClear()) {
			DrawDebugBox(m_doorVolume, sf::Color::Red);
		}
		co_await Suspend();
	}
}
