#pragma once

#include "Engine/Actor.h"
#include <vector>
#include <memory>

class SensorComponent;

// Stores SensorComponent pointers & current touches, determines when new touches and untouches occur, 
// and dispatches the relevant callbacks
class SensorManager : public Actor
{
public:
	virtual void Initialize() override;
	virtual void Destroy() override;
	void Update();
	void RegisterSensor(std::shared_ptr<SensorComponent> in_comp);

private:
	using tTouch = std::pair<std::shared_ptr<SensorComponent>, std::shared_ptr<SensorComponent>>;
	using tTouches = std::vector<tTouch>;
	tTouches m_currentTouches;
	std::vector<std::shared_ptr<SensorComponent>> m_sensors;
};