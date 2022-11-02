#include "SensorManager.h"

#include "GameLoop.h"
#include "Algorithms.h"
#include "Engine/DebugDrawSystem.h"
#include "Engine/Game.h"
#include "Engine/Components/SensorComponent.h"

//--- SENSOR MANAGER CODE ---//

void SensorManager::Initialize() {
	Actor::Initialize();
	GameBase::Get()->SetPhysicsCallback([this]() {
		Update();
	});
}
void SensorManager::Destroy() {
	GameBase::Get()->SetPhysicsCallback(nullptr);
	Actor::Destroy();
}
void SensorManager::Update() {
	tTouches newTouches;
	for(const auto& sensorA : m_sensors) {

		// Check for touches
		for(const auto& sensorB : m_sensors) {
			if(sensorA == sensorB) {
				continue;
			}
			if(sensorA->IsTouching(sensorB)) {

				// Verify this is a NEW touch -- if not, ignore
				auto IsDuplicate = [this, sensorA, sensorB](const tTouches& in_sensorVector) {
					for(const auto& pair : in_sensorVector) {
						if((pair.first == sensorA && pair.second == sensorB) ||
							(pair.first == sensorB && pair.second == sensorA)) {
							return true;
						}
					}
					return false;
				};
				if(!IsDuplicate(newTouches) && !IsDuplicate(m_currentTouches)) {
					auto newPair = std::make_pair(sensorA, sensorB);
					newTouches.push_back(newPair);
					//printf("new touch!\n");
				}
			}
		}

		// Display debug shapes
		if(GameLoop::ShouldDisplayDebug()) {
			if(sensorA->GetShape().GetShapeType() == eSensorShapeType::Circle) {
				auto circle = sensorA->GetShape().GetCircle();
				DrawDebugCircle(Vec2f::Zero, circle.radius, sf::Color::White, sensorA->GetWorldTransform(), (-1.0f));
			}
			if(sensorA->GetShape().GetShapeType() == eSensorShapeType::Box) {
				Box2f box = Box2f::FromCenter(Vec2f::Zero, sensorA->GetShape().GetBox());
				DrawDebugBox(box, sf::Color::Yellow, sensorA->GetWorldTransform(), (-1.0f));
			}
		}
	}

	// Dispatch new touch callbacks
	for(const auto& sensorPair : newTouches) {
		auto& sensorA = sensorPair.first;
		auto& sensorB = sensorPair.second;
		if(IsAlive(sensorA)) {
			sensorA->CallTouchCallback(true, sensorB);
		}
		if(IsAlive(sensorB)) {
			sensorB->CallTouchCallback(true, sensorA);
		}
	}

	// Identify + remove touches that are no longer overlapping
	tTouches untouches;
	auto CheckPairTouch = [](const auto& pair) {
		return !pair.first->IsTouching(pair.second);
	};
	std::copy_if(m_currentTouches.begin(), m_currentTouches.end(), std::back_inserter(untouches), CheckPairTouch);
	m_currentTouches.erase(std::remove_if(m_currentTouches.begin(), m_currentTouches.end(), CheckPairTouch), m_currentTouches.end());

	// Dispatch untouch callbacks
	for(const auto sensorPair : untouches) {
		auto& sensorA = sensorPair.first;
		auto& sensorB = sensorPair.second;
		if(IsAlive(sensorA)) {
			sensorA->CallTouchCallback(false, sensorB);
		}
		if(IsAlive(sensorB)) {
			sensorB->CallTouchCallback(false, sensorA);
		}
	}

	// Add new touches that are overlapping this frame
	std::move(newTouches.begin(), newTouches.end(), std::back_inserter(m_currentTouches));

	// Remove any invalid sensors
	EraseInvalid(m_sensors);
}
void SensorManager::RegisterSensor(std::shared_ptr<SensorComponent> in_comp) {
	m_sensors.push_back(in_comp);
}
