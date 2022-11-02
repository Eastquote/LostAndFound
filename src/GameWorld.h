#pragma once

#include "Engine/Actor.h"
#include "Engine/Box.h"
#include "GameActor.h"
#include <unordered_set>

class Player;
class Creature;
class ParticleManager;
class ProjectileManager;
class SensorManager;
class CameraManager;
class DropManager;
class TileObject;
class TilesComponent;
class TileMap;
class CreatureSpawner;
struct Room;
struct PlayerStatus;
class Door;
struct BeamStatus;
class Projectile;
class CollisionWorld;
class PauseMenu;
class Hud;
class ColliderComponent_TilesComponent;
class Trigger;
class AimReticleManager;

struct Room {
	Box2f bounds;
	std::vector<std::shared_ptr<CreatureSpawner>> spawners;
	std::vector<std::shared_ptr<Trigger>> triggers;
	std::vector<std::shared_ptr<Door>> doors;
	std::vector<std::shared_ptr<Room>> adjacentRooms;
	std::vector<std::shared_ptr<Creature>> creatures;
	int32_t Id;
};

// Makes game world exist and be usable -- loads and stores the current map, all spawned actors including Player, and all the managers
class GameWorld : public GameActor {
public:
	static std::shared_ptr<GameWorld> Get();
	GameWorld();
	GameWorld(bool in_newGame);
	~GameWorld();
	virtual void Initialize() override;
	virtual void Destroy() override;
	virtual Task<> ManageActor() override;
	virtual void Update() override;
	virtual bool ShouldUpdateWhilePaused() const override { return true; }

	// Loads map data, spawns/configures/stores all tiles, layers, actors, rooms -- the Big Dog of this class
	void LoadMap(const std::string& in_filename);

	// Actor management
	void AddActor(std::shared_ptr<GameActor> in_actor) { m_actors.insert(in_actor); }
	void RemoveActor(std::shared_ptr<GameActor> in_actor) { m_actors.erase(in_actor); }
	void DeferredDestroy(std::shared_ptr<GameActor> in_actor) { m_actorsToDestroy.push_back(in_actor); }
	
	// World getters
	const std::unordered_set<std::shared_ptr<GameActor>>& GetActors() const { return m_actors; }
	std::shared_ptr<TilesComponent> GetCollisionTilesComp() const { return m_collisionTilesComp; }
	std::shared_ptr<TilesComponent> GetWorldTilesComp() const { return m_worldTilesComp; }
	const std::vector<std::shared_ptr<TilesComponent>>& GetDrawTilesComp() const { return m_drawTilesComp; }
	std::shared_ptr<TileMap> GetTileMap() const { return m_tileMap; }
	std::shared_ptr<ProjectileManager> GetProjectileManager() const { return m_projectileManager; }
	std::shared_ptr<ParticleManager> GetParticleManager() const { return m_particleManager; }
	std::shared_ptr<SensorManager> GetSensorManager() const { return m_sensorManager; }
	std::shared_ptr<DropManager> GetDropManager() const { return m_dropManager; }
	std::shared_ptr<CameraManager> GetCameraManager() const { return m_cameraManager; }
	std::shared_ptr<AimReticleManager> GetAimReticleManager() const { return m_aimReticleManager; }
	std::shared_ptr<CollisionWorld> GetCollisionWorld() const { return m_collisionWorld; }
	std::shared_ptr<Hud> GetHud() const { return m_hud; }
	
	// Player getters/setters, utils
	std::shared_ptr<PlayerStatus> GetPlayerStatus() const { return m_playerStatus; }
	Vec2f GetPlayerWorldPos(bool shotTarget = false) const;
	void SetPlayerWorldPos(const Vec2f& in_pos);
	void ZapPlayer(std::shared_ptr<Creature> in_creature, const Vec2f& in_zapPos);
	void MovePlayer(const Vec2f& in_pos, bool in_bTeleport = false);
	int32_t GetPlayerHealth(bool returnPercentage = false) const;
	void ChangePlayerHealth(int32_t in_change);
	void EnablePlayerControls();
	void DisablePlayerControls();
	void AttachPlayerTo(std::shared_ptr<Actor> in_actor, bool in_keepWorldTransform = true);
	void DetachPlayer(bool in_keepWorldTransform = true);
	void SetPlayerGravityModifier(float in_gravityMod);
	
	// Room methods
	const std::vector<std::shared_ptr<Room>>& GetRooms() const { return m_rooms; }
	std::shared_ptr<Room> GetPlayerRoom() const;
	std::shared_ptr<Room> GetEnclosingRoom(const Vec2f& in_pos) const;
	std::optional<bool> RoomEnclosesCreatureWorldCollision(std::shared_ptr<Creature> in_creature) const;
	void OnRoomChanged(std::shared_ptr<Room> in_oldRoom, std::shared_ptr<Room> in_newRoom);
	
	WeakTaskHandle OnItemPickup(std::optional<std::pair<std::wstring, std::wstring>> in_itemTip);
	void OnPause();

	// Utility methods
	bool CreatureIsOffCamera(std::shared_ptr<Creature> in_creature) const;
	std::vector<std::shared_ptr<Creature>> GetOnCameraCreatures() const;
	bool ProjectileIsOffCamera(std::shared_ptr<Projectile> in_projectile) const;

	// Fades the opacity of any sprite or text component from 0->100 or vice-versa
	Task<> Fade(std::shared_ptr<SpriteComponent> in_sprite, std::shared_ptr<TextComponent> in_text = nullptr, 
				float in_duration = 1.0f, bool in_fadeToOpaque = false);
	
	// Generic easing helper -- feed it target/elapsed times and a curve func, it returns a normalized value
	float EaseAlpha(float in_elapsedTime, float in_targetDuration, std::function<float(float)> in_easingFunc);
	
	// Converts cameraspace position in_pos to a worldspace position
	Vec2f ViewToWorld(const Vec2f& in_pos);
	std::shared_ptr<Player> GetPlayer() const { return m_player; }
	int32_t GetPlayerMissiles(bool returnPercentage = false) const;
	int32_t GetPlayerSuperMissiles(bool returnPercentage = false) const;
	int32_t GetPlayerGrenades(bool returnPercentage) const;
	int32_t GetPlayerEnergyTankCount() const;
	Box2f GetPlayerWorldCollisionBox() const;
	bool IsPlayerGrounded() const;
	bool IsPlayerFish() const;
	
	// GameOver methods
	bool GetGameOverRequest() const { return m_bGameOverRequested; }
	void GameOverRequest() { m_bGameOverRequested = true; }

private:
	// Pauses game, plays a little jingle, then unpauses
	Task<> ItemPickupFanfare(std::optional<std::pair<std::wstring, std::wstring>> in_itemTip);
	void Cleanup();
	Task<> ToggleDebug();
	void ClearInvalidRoomActors(std::shared_ptr<Room> in_room);

	// Actor data
	std::unordered_set<std::shared_ptr<GameActor>> m_actors;
	std::vector<std::shared_ptr<GameActor>> m_actorsToDestroy;

	// Player data
	std::shared_ptr<Player> m_player;
	Vec2f m_lastPlayerPos = Vec2f::Zero;
	Vec2f m_lastPlayerTargetPos = Vec2f::Zero;
	std::shared_ptr<PlayerStatus> m_playerStatus;
	std::shared_ptr<PlayerStatus> m_lastPlayerStatus;

	// Map data
	std::shared_ptr<TileMap> m_tileMap;
	std::shared_ptr<TilesComponent> m_collisionTilesComp;
	std::shared_ptr<ColliderComponent_TilesComponent> m_colliderTilesComp;
	std::shared_ptr<TilesComponent> m_worldTilesComp;
	std::shared_ptr<TilesComponent> m_worldBkgTilesComp;
	std::vector<std::shared_ptr<TilesComponent>> m_drawTilesComp;
	std::shared_ptr<TilesComponent> m_worldTilesLitComp;
	std::vector<std::shared_ptr<TilesComponent>> m_drawTilesLitComp;
	std::shared_ptr<TilesComponent> m_worldBkgTilesLitComp;
	std::vector<std::shared_ptr<TilesComponent>> m_drawBkgTilesLitComp;
	std::vector<std::shared_ptr<Room>> m_rooms;

	// Managers
	std::shared_ptr<ProjectileManager> m_projectileManager;
	std::shared_ptr<ParticleManager> m_particleManager;
	std::shared_ptr<SensorManager> m_sensorManager;
	std::shared_ptr<CameraManager> m_cameraManager;
	std::shared_ptr<AimReticleManager> m_aimReticleManager;
	std::shared_ptr<DropManager> m_dropManager;
	std::shared_ptr<CollisionWorld> m_collisionWorld;
	std::shared_ptr<Hud> m_hud;
	std::shared_ptr<PauseMenu> m_pauseMenu;

	bool m_bGameOverRequested = false;
};
