#include "Lava.h"

#include "Engine/DebugDrawSystem.h"

//--- LAVA CODE ---//

Lava::Lava(Box2f in_volume)
	: m_lavaDims(in_volume.GetDims())
{
}
void Lava::Initialize() {
	GameActor::Initialize();

	// Setup sensor
	SensorShape playerSensorShape;
	playerSensorShape.SetBox(m_lavaDims);
	auto playerSensor = MakeSensor(Transform::Identity, playerSensorShape);
	playerSensor->SetFiltering(CL_Lava, CL_Player);
	playerSensor->SetTouchCallback([this](bool in_beginning, std::shared_ptr<SensorComponent> in_other) {
		if(in_beginning) {
			if(auto other = std::dynamic_pointer_cast<Player>(in_other->GetActor())) {
				OnTouchPlayer(other);
			}
		}
		else if(auto other = std::dynamic_pointer_cast<Player>(in_other->GetActor())) {
			OnUnTouchPlayer(other);
		}
	});

	// Setup DamageInfo
	SetDamageInfo(1, DF_Enemy | DF_NoKnockback);
}
Task<> Lava::ManageActor() {
	while(true) {
		auto box = Box2f::FromCenter(GetWorldPos(), m_lavaDims);
		co_await Suspend();
	}
}
void Lava::OnTouchPlayer(std::shared_ptr<Player> in_player) {
	//printf("LAVA: Touched Player!\n");
}
void Lava::OnUnTouchPlayer(std::shared_ptr<Player> in_player) {
	//printf("LAVA: UNTouched Player!\n"); 
}