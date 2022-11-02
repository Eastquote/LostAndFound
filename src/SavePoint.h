#pragma once

#include "GameActor.h"

class SensorComponent;
class Player;

// Simple in-world sensor tagged as CL_SavePoint. 
// Player has code to enable a TrySave() call when upInput is pressed while touching this
class SavePoint : public GameActor {
public:
	virtual void Initialize() override;

private:
	std::shared_ptr<SensorComponent> m_playerSensor;
};
