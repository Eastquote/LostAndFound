#pragma once

#include "GameActor.h"
#include "GameEnums.h"
#include "Engine/Components/SpriteComponent.h"

class Creature;
class Player;
class Door;
struct DamageInfo;

// A stationary bomb that damages enemies AND breaks cardinal-adjacent destructible world tiles
class Bomb : public GameActor {
public:
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;
	void TryDestroyTiles();

private:
	void MakeAoeSensor();
	void OnTouchCreature(std::shared_ptr<Creature> in_other);
	void OnTouchPlayer(std::shared_ptr<Player> in_other);
	void OnTouchDoor(std::shared_ptr<Door> in_other);

	bool m_bIsExploding = false;
	float m_fuseTime = 0.8f;
	float m_explosionTime = 0.38f;
	std::shared_ptr<SpriteComponent> m_sprite;
	std::vector<std::shared_ptr<GameActor>> m_actorsAlreadyHit;
};
