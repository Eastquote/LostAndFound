#pragma once
#include "GameActor.h"
#include "GameEnums.h"

class Projectile;
class Player;
class Bomb;
class Creature;
class CreatureSpawner;
class SensorComponent;

// Simple rectangular trigger sensor, to be placed in the world and used to spawn enemies based on Player location
class Trigger : public GameActor {
public:
	Trigger(eTriggerTarget in_tTarget, Box2f in_tBox, int32_t in_objId);
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;
	void OnTouchPlayer(std::shared_ptr<Player> in_player);
	void OnUnTouchPlayer(std::shared_ptr<Player> in_player);
	Box2f GetBounds() const { return m_triggerBox; }
	bool GetActive() const { return m_bIsActive; }
	void SetActive(bool in_state) { m_bIsActive = in_state; }
	bool TriggerClear();

private:
	Task<> DrawTriggerVolume();

	bool m_bIsActive = true;
	bool m_bTriggerJustActivated = false;
	bool m_bPlayerOnTrigger = false;
	Box2f m_triggerBox = Box2f::FromCenter(Vec2f{ 0.0f, 0.0f }, { 62.0f, 48.0f });
	eTriggerTarget m_tTarget = eTriggerTarget::Pirate;
	std::shared_ptr<SensorComponent> m_playerSensor;
	int32_t m_objId = 0;
	std::vector<std::shared_ptr<CreatureSpawner>> m_targetSpawners;
};
