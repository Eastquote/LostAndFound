#include "Trigger.h"

#include "Engine/DebugDrawSystem.h"
#include "GameEnums.h"
#include "GameWorld.h"
#include "GameLoop.h"
#include "Player.h"
#include "PlayerStatus.h"
#include "CreatureSpawner.h"

//--- TRIGGER CODE ---//

Trigger::Trigger(eTriggerTarget in_tTarget, Box2f in_tBox, int32_t in_objId)
	: m_tTarget(in_tTarget)
	, m_objId(in_objId)
	, m_triggerBox(in_tBox)
{
}
void Trigger::Initialize() {
	GameActor::Initialize();
	auto playerStatus = GetWorld()->GetPlayerStatus();
	auto unlocked = playerStatus->IsObjectUnlocked(m_objId);
	if(unlocked) {
		m_bIsActive = true;
	}
	auto pos = GetWorldPos();

	// Set up sensors
	SensorShape playerSensorShape;
	playerSensorShape.SetBox({ m_triggerBox.GetDims() });
	auto playerSensor = MakeSensor(Transform::Identity, playerSensorShape);
	playerSensor->SetFiltering(CL_Trigger, CL_Player);
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
	m_triggerBox.SetCenter_Move(GetWorldPos());
}
Task<> Trigger::ManageActor() {
	auto debugDrawTask = m_taskMgr.RunManaged(DrawTriggerVolume());

	// Gather target spawners
	auto enclosingRoom = GetWorld()->GetEnclosingRoom(GetWorldPos());
	auto localSpawners = enclosingRoom->spawners;
	for(auto spawner : localSpawners) {
		if(spawner->IsTriggerable() == m_tTarget) {
			m_targetSpawners.push_back(spawner);
		}
	}
	while(true) {
		co_await Suspend();
	}
}
void Trigger::OnTouchPlayer(std::shared_ptr<Player> in_player) {
	for(auto spawner : m_targetSpawners) {
		spawner->TriggerSpawn();
	}
	m_bPlayerOnTrigger = true;
}
void Trigger::OnUnTouchPlayer(std::shared_ptr<Player> in_player) {
	m_bPlayerOnTrigger = false;
}
bool Trigger::TriggerClear() {
	return (!m_bPlayerOnTrigger);
}
Task<> Trigger::DrawTriggerVolume() {
	while(true) {
		if(GameLoop::ShouldDisplayDebug()) {
			if(TriggerClear()) {
				DrawDebugBox(m_triggerBox, sf::Color::Green);
			}
			else if(!TriggerClear()) {
				DrawDebugBox(m_triggerBox, sf::Color::Red);
			}
		}
		co_await Suspend();
	}
}
