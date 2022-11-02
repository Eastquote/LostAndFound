#include "CameraManager.h"

#include "GameWorld.h"
#include "CreatureSpawner.h"
#include "GameLoop.h"
#include "Hud.h"
#include "PlayerStatus.h"
#include "SquirrelNoise5.h"
#include "Engine/GameWindow.h"
#include "Engine/MathGeometry.h"
#include "Engine/DebugDrawSystem.h"
#include "Engine/Components/SpriteComponent.h"
#include <algorithm>

namespace {
	const Vec2f k_sensorOffsetHigh = { 0.0f, 0.0f };
	const Vec2f k_sensorOffsetLow = { 0.0f, -11.0f };
}

//--- CAMERA MANAGER ---//

void CameraManager::Initialize() {
	GameActor::Initialize();
	SetUpdateStage(eUpdateStage::PostPhysics);
	m_prevPlayerPos = GetWorld()->GetPlayerWorldPos();
	SetCameraPos(m_prevPlayerPos);
}
void CameraManager::SetCameraPos(const Vec2f& in_pos, float in_angle) {
	//Vec2i newPos = { (int32_t)in_pos.x, (int32_t)in_pos.y };
	auto window = GetWindow();
	auto windowSize = GetRenderSize();
	auto roundedPos = Vec2f{ std::round(in_pos.x), std::round(in_pos.y) };
	SetWorldPos(roundedPos);
	m_lastCameraDisp = roundedPos - m_prevCameraPos;
	m_prevCameraPos = roundedPos;
	window->SetWorldView(	Box2d{ std::round(in_pos.x) - (double)windowSize.x / 2, std::round(in_pos.y) - (double)windowSize.y / 2,
							(double)windowSize.x, (double)windowSize.y }, -in_angle);
}
Vec2f CameraManager::ComputeCameraPos(const Vec2f& in_targetPlayerPos) {
	m_cameraBoundsLocal.y = m_bIsSensorHigh ? k_sensorOffsetHigh.y : k_sensorOffsetLow.y;
	auto world = GetWorld();
	auto targetRoom = world->GetEnclosingRoom(in_targetPlayerPos);
	auto currentRoom = world->GetEnclosingRoom(world->GetPlayerWorldPos());
	if(targetRoom != currentRoom) {
		ActivateRoom(targetRoom, false);
	}
	auto roomBounds = targetRoom->bounds;
	auto windowSize = GetRenderSize();
	const auto& targetPlayerPos = in_targetPlayerPos;
	auto targetCameraDelta = Vec2f::Zero;
	auto sensorBox = GetCameraBoundsWorld();;

	// Move camera if player is outside sensorBounds
	if(!Math::PointInBox(sensorBox, targetPlayerPos)) {
		if(targetPlayerPos.x > sensorBox.GetExtentsX().m_max) {
			targetCameraDelta.x = targetPlayerPos.x - sensorBox.GetExtentsX().m_max;
		}
		else if(targetPlayerPos.x < sensorBox.GetExtentsX().m_min) {
			targetCameraDelta.x = targetPlayerPos.x - sensorBox.GetExtentsX().m_min;
		}
		if(targetPlayerPos.y > sensorBox.GetExtentsY().m_max) {
			targetCameraDelta.y = targetPlayerPos.y - sensorBox.GetExtentsY().m_max;
		}
		else if(targetPlayerPos.y < sensorBox.GetExtentsY().m_min) {
			targetCameraDelta.y = targetPlayerPos.y - sensorBox.GetExtentsY().m_min;
		}
	}
	auto targetCameraPos = GetWorldPos() + targetCameraDelta;

	// Clamp camera position to whole-pixel increments
	Vec2f windowPosClamped = {
		std::clamp(	targetCameraPos.x, roomBounds.x + std::floor((float)windowSize.x / 2), 
					(roomBounds.x + roomBounds.w) - std::floor((float)windowSize.x / 2)),
		std::clamp(	targetCameraPos.y, roomBounds.y + std::floor((float)windowSize.y / 2), 
					(roomBounds.y + roomBounds.h) - std::floor((float)windowSize.y / 2))
	};
	return windowPosClamped;
}
Vec2f CameraManager::ComputeCameraShake(int32_t in_frameNumber, float in_distress) {
	auto distressMultiplier = in_distress * in_distress;
	auto noiseX = Get1dNoiseNegOneToOne(in_frameNumber, 0);
	auto noiseY = Get1dNoiseNegOneToOne(in_frameNumber, 1);
	return Vec2f{ noiseX, noiseY } * distressMultiplier;
}
float CameraManager::ComputeCameraRoll(int32_t in_frameNumber, float in_distress) {
	auto distressMultiplier = in_distress * in_distress;
	auto noiseZ = Get1dNoiseNegOneToOne(in_frameNumber, 2);
	return noiseZ * distressMultiplier;
}
void CameraManager::ActivateRoom(std::shared_ptr<Room> in_currentRoom, bool in_bActivateDoors) {
	auto rooms = GetWorld()->GetRooms();

	// Activate spawners in this room
	for(auto spawner : in_currentRoom->spawners) {
		spawner->SetActive(true);
	}

	// Check for room unlock status
	auto playerStatus = GetWorld()->GetPlayerStatus();
	if(!playerStatus->IsRoomUnlocked(in_currentRoom->Id)) {
		playerStatus->UnlockRoom(in_currentRoom->Id);
	}
	if(in_bActivateDoors){
		// Turn off all the doors in the game
		for(auto room : rooms) {
			for(auto door : room->doors) {
				door->SetActive(false);
			}
		}

		// Turn back ON the doors that touch this room
		for(auto door : in_currentRoom->doors) {
			door->SetActive(true);
		}
	}
}
void CameraManager::StartRoomTransition(std::shared_ptr<Door> in_door, bool in_dir, bool in_bIsVertical) {
	m_bIsTransitioningPositive = in_dir;
	m_transitionDoor = in_door;
	m_bRoomTransitioning = true;
	m_bIsVerticalDoor = in_bIsVertical;
}
Task<> CameraManager::ManageActor() {
	auto world = GetWorld();
	auto window = GetWindow();
	m_prevCameraPos = GetWorldPos();
	auto currentRoom = world->GetPlayerRoom();

	// Utility function to animate camera position
	auto Lerp = [](Vec2f in_startVal, Vec2f in_endVal, float in_elapsedNorm) {
		auto xVal = in_startVal.x + in_elapsedNorm * (in_endVal.x - in_startVal.x);
		auto yVal = in_startVal.y + in_elapsedNorm * (in_endVal.y - in_startVal.y);
		return Vec2f{ xVal, yVal };
	};
	auto windowSize = GetRenderSize();
	auto rooms = GetWorld()->GetRooms();
	ActivateRoom(currentRoom);
	auto frameNumber = 0;

	// Main loop
	while(true) {
		Vec2f playerPos = world->GetPlayerWorldPos();
		currentRoom = world->GetPlayerRoom();
		windowSize = GetRenderSize();
		if(m_bRoomTransitioning) {
			m_prevRoom = currentRoom;
			auto transitionDuration = 2.13f;
			auto elapsedTransitionTime = 0.0f;
			co_await WaitSeconds(0.3f);
			auto dir = m_bIsTransitioningPositive ? 1.0f : -1.0f;
			auto targetOffset = m_bIsVerticalDoor ? Vec2f{ 0.0f, 41.0f * dir } : Vec2f{ 41.0f * dir, 0.0f };
			auto targetPos = ComputeCameraPos(m_transitionDoor->GetWorldPos() + targetOffset);
			float windowFixupPosY = 0.0f;

			// Do the camera move from one room to another
			while(elapsedTransitionTime < transitionDuration) {
				elapsedTransitionTime += DT();
				if(!m_bIsVerticalDoor) {
					// Incrementally adjust camera y-pos until it aligns with vertical bounds of adjacent room
					windowFixupPosY = GetWorldPos().y;
					if(windowFixupPosY > targetPos.y) {
						windowFixupPosY -= (1.0f * 60.0f) * DT();
					}
					else if(windowFixupPosY < targetPos.y) {
						windowFixupPosY += (1.0f * 60.0f)* DT();
					}
				}
				auto windowPosInterp = Lerp(m_prevCameraPos, targetPos, elapsedTransitionTime * (1.0f / transitionDuration));
				auto yCameraPos = m_bIsVerticalDoor ? windowPosInterp.y : windowFixupPosY;
				auto newPos = Vec2f{ windowPosInterp.x, yCameraPos };
				SetCameraPos(newPos);
				co_await Suspend();
			}
			currentRoom = GetWorld()->GetPlayerRoom();
			if(m_prevRoom != currentRoom) { //< Clean up the previous room
				for(auto spawner : m_prevRoom->spawners) {
					spawner->SetActive(false);
					spawner->DestroyChildren();
				}
				GetWorld()->OnRoomChanged(m_prevRoom, currentRoom);
			}
			ActivateRoom(currentRoom);
			m_bRoomTransitioning = false;
		}

		// Adjust camera bounds' vertical position (reduces camera bounds mis-alignment in vert->horiz room transitions)
		for(auto door : currentRoom->doors) {
			// If rose above door's midpoint this frame
			if(m_prevPlayerPos.y < door->GetWorldPos().y && playerPos.y > door->GetWorldPos().y) {
				m_bIsSensorHigh = true;
			}
			else if(m_prevPlayerPos.y > door->GetWorldPos().y - 16.0f && playerPos.y < door->GetWorldPos().y - 16.0f) {
				m_bIsSensorHigh = false;
			}
		}
		m_distress = std::clamp(m_distress, 0.0f, 1.75f);
		if(!m_bManualOverride) {
			auto windowPosClamped = ComputeCameraPos(GetWorld()->GetPlayerWorldPos());
			auto windowShakeOffset = ComputeCameraShake(frameNumber, m_distress);
			auto windowRotationOffset = ComputeCameraRoll(frameNumber, m_distress);
			SetCameraPos(	Vec2f{ windowPosClamped.x, windowPosClamped.y } + windowShakeOffset, 
							windowRotationOffset);
			if(GameLoop::ShouldDisplayDebug()) { //< Display camera bounds on-screen
				DrawDebugBox(GetCameraBoundsWorld(), m_bIsSensorHigh ? sf::Color::Cyan : sf::Color::Magenta);
				DrawDebugPoint(playerPos, sf::Color::Blue);
			}
		}
		m_prevRoom = currentRoom;
		m_prevPlayerPos = playerPos;
		m_distress -= DT() * 2.5f;
		++frameNumber;
		co_await Suspend();
	}
 }