#pragma once

#include "GameActor.h"

class Projectile;
class Player;
class Bomb;
class Creature;
class SensorComponent;

enum class eDoorColor {
	None = 0, //< Spawns in a permanently-open state
	Blue = 1, //< Opens temporaritly when hit with any player projectile
	Red = 2, //< Opens permanently when hit with five explosive player projectiles (e.g. missiles or grenades)
};

// Hand-placed actor that enables or prevents player passage from room to room
class Door : public GameActor {
public:
	Door(eDoorColor in_color, int32_t in_objId, bool in_bIsVertical = false);
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;

	void OnTouchPlayerBullet(std::shared_ptr<Projectile> in_projectile);
	void OnTouchPlayerBomb(std::shared_ptr<Bomb> in_bomb);
	void OnTouchPlayer(std::shared_ptr<Player> in_player);
	void OnUnTouchPlayer(std::shared_ptr<Player> in_player);
	void OnTouchCreature(std::shared_ptr<Creature> in_creature);
	void SetDoorTileBlocking(bool in_bIsBlocking);
	bool IsDoorOpen() const { return m_bIsOpen; }

	const Box2f& GetBounds() const { return m_doorVolume; }
	bool GetActive() const { return m_bIsActive; }
	void SetActive(bool in_state) { m_bIsActive = in_state; }

private:
	bool DoorwayClear();
	void FlashRedDoor();
	Task<> FlashRedDoorTask();
	// During a room transition, move player into or out of doorway depending on which arg is supplied
	Task<> TransitionPlayerPosition(bool in_bIsEntering);

	// Debug tool to visualize door bounds
	Task<> DrawDoorVolume();

	// Door sprites
	std::shared_ptr<SpriteComponent> m_doorSpriteLeft;
	std::shared_ptr<SpriteComponent> m_doorSpriteRight;
	std::shared_ptr<SpriteComponent> m_doorSpriteCenter;
	std::shared_ptr<SpriteComponent> m_doorSpriteCenterLit;

	std::shared_ptr<Projectile> m_projectileThatHit;
	std::shared_ptr<SensorComponent> m_playerSensor;
	eDoorColor m_color = eDoorColor::Blue;
	Box2f m_doorVolume = Box2f::FromCenter(Vec2f{ 0.0f, 0.0f }, { 62.0f, 48.0f });
	std::vector<Vec2f> m_tileLocations;
	bool m_bDoorJustActivated = false;
	bool m_bPlayerInDoorway = false;
	bool m_bTransitionInProgress = false;
	bool m_bIsOpen = false;
	bool m_bIsActive = false;
	bool m_bIsVertical = false;
	int32_t m_creaturesInDoorway = 0;
	int32_t m_explosiveHealth = 5;
	int32_t m_objId = 0;
};