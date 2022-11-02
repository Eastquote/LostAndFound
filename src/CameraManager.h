#pragma once

#include "GameActor.h"
#include "Door.h"

struct Room;
class AsteroidField;

// Manages game camera movement, including room transitions and camera shake
class CameraManager : public GameActor {
public:
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;
	Box2f GetCameraBoundsWorld() { return m_cameraBoundsLocal.TransformedBy(GetWorldTransform()); }
	Box2f GetAsteroidBoundsWorld() { return m_asteroidBoundsLocal.TransformedBy(GetWorldTransform()); }
	const Transform& GetCameraWorldTransform() const { return GetWorldTransform(); }
	void StartRoomTransition(std::shared_ptr<Door> in_door, bool in_dir, bool in_bIsVertical = false);
	void AddDistress(float in_distressIncrease) { m_distress += in_distressIncrease; }
	void SetDistress(float in_distressVal) { m_distress = in_distressVal; }
	void SetOverridden(bool in_bOverridden = true) { m_bManualOverride = in_bOverridden; }
	void SetCameraPos(const Vec2f& in_pos, float in_angle = 0.0f);
	Vec2f GetLastCameraDisp() { return m_lastCameraDisp; }

private:
	void ActivateRoom(std::shared_ptr<Room> in_currentRoom, bool in_bActivateDoors = true);
	Vec2f ComputeCameraPos(const Vec2f& in_targetPlayerPos);
	Vec2f ComputeCameraShake(int32_t in_frameNumber, float in_distress);
	float ComputeCameraRoll(int32_t in_frameNumber, float in_distress);

	bool m_bIsVerticalDoor = false;
	bool m_bManualOverride = false;
	bool m_bIsSensorHigh = false;
	Vec2f m_prevPlayerPos = Vec2f::Zero;
	std::shared_ptr<Room> m_prevRoom;
	Vec2f m_prevCameraPos = Vec2f::Zero;
	Vec2f m_lastCameraDisp = Vec2f::Zero;
	Box2f m_cameraBoundsLocal = Box2f::FromCenter({ 0.0f, -11.0f }, { 32.0f, 32.0f });
	Box2f m_asteroidBoundsLocal = Box2f::FromCenter({ 0.0f, 0.0f }, { 600.0f, 500.0f });
	bool m_bRoomTransitioning = false;
	std::shared_ptr<Door> m_transitionDoor;
	bool m_bIsTransitioningPositive = true;
	float m_distress = 0.0f;
};
