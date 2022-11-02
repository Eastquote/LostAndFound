#include "SavePoint.h"

#include "GameEnums.h"
#include "Player.h"

void SavePoint::Initialize() {
	GameActor::Initialize();
	SensorShape playerSensorShape;
	playerSensorShape.SetBox({ 40.0f, 32.0f });
	m_playerSensor = MakeSensor(Transform::Identity, playerSensorShape);
	m_playerSensor->SetFiltering(CL_SavePoint, CL_Player);
}
