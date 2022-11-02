#pragma once

#include "GameActor.h"
#include "Player.h"

// Class of hand-placed boxes that behave in-game as lava
// (i.e. reduce move and jump speed, periodically cause player damage)
class Lava : public GameActor {
public:
	Lava(Box2f in_volume);
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;
	void OnTouchPlayer(std::shared_ptr<Player> in_player);
	void OnUnTouchPlayer(std::shared_ptr<Player> in_player);
	
private:
	Vec2i m_lavaDims;
};
