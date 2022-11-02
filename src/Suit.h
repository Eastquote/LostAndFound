#pragma once

#include "GameActor.h"

class Player;

// Empty-spacesuit prop placed in the world -- if Player touches it while in Fish state, triggers Player's 
// suit-entering transition and self-destructs
class Suit : public GameActor {
public:
	Suit() {};
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;

private:
	void OnTouchPlayer(std::shared_ptr<Player> in_player, std::shared_ptr<SensorComponent> in_sensor);
	std::shared_ptr<SpriteComponent> m_sprite;
	std::shared_ptr<SpriteComponent> m_fgSprite;
	std::shared_ptr<SensorComponent> m_playerSensor;
	bool m_bActive = false;
};
