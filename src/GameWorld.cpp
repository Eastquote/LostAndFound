#include "GameWorld.h"

#include "GameActor.h"
#include "Player.h"
#include "PlayerStatus.h"
#include "Creature.h"
#include "Creatures/Blobber.h"
#include "Creatures/Charger.h"
#include "Creatures/Crawler.h"
#include "Creatures/Cruiser.h"
#include "Creatures/Dropper.h"
#include "Creatures/Laser.h"
#include "Creatures/Monopod.h"
#include "Creatures/Piper.h"
#include "Creatures/Pirate.h"
#include "Creatures/Rammer.h"
#include "Creatures/Sentry.h"
#include "Creatures/Ship.h"
#include "Creatures/Swooper.h"
#include "Creatures/Turret.h"
#include "CreatureSpawner.h"
#include "ProjectileManager.h"
#include "ParticleManager.h"
#include "AimReticleManager.h"
#include "SensorManager.h"
#include "GameLoop.h"
#include "SensorManager.h"
#include "CameraManager.h"
#include "DropManager.h"
#include "AudioManager.h"
#include "PickupDefs.h"
#include "Door.h"
#include "Suit.h"
#include "Trigger.h"
#include "Lava.h"
#include "SavePoint.h"
#include "PauseMenu.h"
#include "Hud.h"
#include "Algorithms.h"
#include "Engine/Game.h"
#include "Engine/AssetCache.h"
#include "Engine/Components/TilesComponent.h"
#include "Engine/Components/TextComponent.h"
#include "Engine/Font.h"
#include "TextDefs.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/CollisionWorld.h"
#include "Engine/Components/ColliderComponent.h"
#include "Engine/MathGeometry.h"
#include "Engine/DebugDrawSystem.h"
#include "Engine/TileMap.h"
#include "Engine/MathEasings.h"

static GameWorld* s_gameWorld = {};
std::shared_ptr<GameWorld> GameWorld::Get() {
	SQUID_RUNTIME_CHECK(IsAlive(s_gameWorld), "tried to access invalid GameWorld");
	return std::static_pointer_cast<GameWorld>(s_gameWorld->AsShared());
}
GameWorld::GameWorld()
	: m_playerStatus(std::make_shared<PlayerStatus>())
	, m_lastPlayerStatus(std::make_shared<PlayerStatus>())
{
	SQUID_RUNTIME_CHECK(!IsAlive(s_gameWorld), "tried to construct GameWorld while another is alive");
	s_gameWorld = this;
	m_playerStatus->TryLoad();
	*m_lastPlayerStatus = *m_playerStatus;
}
GameWorld::GameWorld(bool in_newGame)
	: m_playerStatus(std::make_shared<PlayerStatus>())
	, m_lastPlayerStatus(std::make_shared<PlayerStatus>())
{
	SQUID_RUNTIME_CHECK(!IsAlive(s_gameWorld), "tried to construct GameWorld while another is alive");
	s_gameWorld = this;
	if(!in_newGame) {
		m_playerStatus->TryLoad();
	}
	*m_lastPlayerStatus = *m_playerStatus;
}
GameWorld::~GameWorld() {
	Cleanup();
}
void GameWorld::Initialize() {
	Actor::Initialize();
	SetTimeStream(eTimeStream::Real);
	SetUpdateStage(eUpdateStage::Final);
	m_bGameOverRequested = false;
	// Spawn managers
	m_projectileManager = Spawn<ProjectileManager>({});
	m_sensorManager = Spawn<SensorManager>({});
	m_particleManager = Spawn<ParticleManager>({});
	m_aimReticleManager = Spawn<AimReticleManager>({});
	m_dropManager = Spawn<DropManager>({});

	m_collisionWorld = std::make_shared<CollisionWorld>();
}
void GameWorld::Destroy() {
	Actor::Destroy();
	SQUID_RUNTIME_CHECK(s_gameWorld == this, "tried to destroy GameWorld when it wasn't the singleton instance");
	Cleanup(); // This must happen AFTER the world cleans up its child Actors
}
Task<> GameWorld::ManageActor() {
	auto toggleDebugTask = m_taskMgr.Run(ToggleDebug());
	auto criticalPlayerHealth = false;
	auto healthBeepTimer = 0.0f;
	while(true) {
		auto playerHealth = GetPlayerStatus()->GetEnergy();
		if(playerHealth < 30) {
			criticalPlayerHealth = true;
		}
		else if(playerHealth > 50) {
			criticalPlayerHealth = false;
		}
		// Beep while health is critically low (disabled for now)
		//if(criticalPlayerHealth){
		//	healthBeepTimer += DT();
		//	if(healthBeepTimer >= 0.267f && !m_pauseMenu) {
		//		AudioManager::Get()->PlaySound("CriticalHealthBeep");
		//		healthBeepTimer = 0.0f;
		//	}
		//}
		if(GameLoop::ShouldDisplayDebug()) {
			for(auto room : m_rooms) {
				DrawDebugBox(room->bounds, sf::Color::Yellow);
			}
		}
		co_await Suspend();
	}
}
void GameWorld::Update() {
	Actor::Update();
	AudioManager::Get()->Update();
	m_lastPlayerPos = GetPlayerWorldPos();
	m_lastPlayerTargetPos = GetPlayerWorldPos(true);

	// DeferredDestroy() requested actors
	for(std::shared_ptr<Actor> actor : m_actorsToDestroy)
	{
		if(IsAlive(actor))
		{
			actor->Destroy();
		}
	}
	m_actorsToDestroy.clear();
	GetPlayerStatus()->Save();
	ClearInvalidRoomActors(GetPlayerRoom());
}
void GameWorld::LoadMap(const std::string& in_filename) {
	// Load map data (tiles, objects, layers -- all of it)
	m_drawTilesComp.clear();
	m_tileMap = AssetCache<TileMap>::Get()->LoadAsset(in_filename);

	Transform tilesTM = { {0.0f, 0.0f } }; //< global tile position offset (unused for now)

	// Setup collision layer
	auto collisionLayer = m_tileMap->GetLayer("TileCollision");
	auto collisionLayerCopy = std::make_shared<TileLayer>(*collisionLayer);
	m_collisionTilesComp = MakeTiles(collisionLayerCopy, tilesTM);
	m_collisionTilesComp->SetRenderLayer("hud");
	m_colliderTilesComp = MakeCollider_TilesComp(tilesTM, m_collisionWorld, m_collisionTilesComp);

	// Setup worldTiles layer
	auto worldTilesLayer = m_tileMap->GetLayer("World");
	auto worldTiles = MakeTiles(worldTilesLayer, tilesTM);
	m_worldTilesComp = worldTiles;
	m_drawTilesComp.push_back(worldTiles);
	worldTiles->SetRenderLayer("fgTiles");

	// Setup worldBkgTiles layer
	auto worldBkgTilesLayer = m_tileMap->GetLayer("WorldBkg");
	auto worldBkgTiles = MakeTiles(worldBkgTilesLayer, tilesTM);
	m_worldBkgTilesComp = worldBkgTiles;
	m_drawTilesComp.push_back(worldBkgTiles);
	worldBkgTiles->SetRenderLayer("bkgTiles");

	// Setup overlapTiles layer
	auto overlapTilesLayer = m_tileMap->GetLayer("WorldOverlap");
	auto overlapTiles = MakeTiles(overlapTilesLayer, tilesTM);
	overlapTiles->GetActor()->SetDrawLayer(1);
	m_drawTilesComp.push_back(overlapTiles);
	overlapTiles->SetRenderLayer("fgTiles");

	// Setup worldBkgTilesLit layer
	auto worldBkgTilesLitLayer = std::make_shared<TileLayer>(*worldBkgTilesLayer);
	auto litBkgTileSets = worldBkgTilesLitLayer->GetTileSets();
	auto found2 = std::find_if(litBkgTileSets.begin(), litBkgTileSets.end(), [](const std::shared_ptr<TileSet>& in_tileSet) {
		return in_tileSet->GetName() == "GraveyardOrbitTiles";
		});
	if(found2 != litBkgTileSets.end())
	{
		auto fn = found2->get()->GetFilename();
		// Add "_lit" between filename and file extension
		fn = fn.substr(0, fn.size() - 4) + "_lit.tsx";
		// Replace albedo tileset with lit version in the LitLayer
		auto tileSetLit = AssetCache<TileSet>::Get()->LoadAsset(fn);
		worldBkgTilesLitLayer->ReplaceTileSet(*found2, tileSetLit);
	}
	auto worldBkgTilesLit = MakeTiles(worldBkgTilesLitLayer, tilesTM);
	m_worldBkgTilesLitComp = worldBkgTilesLit;
	m_drawTilesLitComp.push_back(worldBkgTilesLit);
	worldBkgTilesLit->SetRenderLayer("bkgLightMask");

	// Setup worldTilesLit layer
	auto worldTilesLitLayer = std::make_shared<TileLayer>(*worldTilesLayer);

	// TODO: pack this into a lambda that takes a layer:
	auto litTileSets = worldTilesLitLayer->GetTileSets();
	auto found = std::find_if(litTileSets.begin(), litTileSets.end(), [](const std::shared_ptr<TileSet>& in_tileSet) {
		return in_tileSet->GetName() == "GraveyardOrbitTiles";
	});
	if(found != litTileSets.end())
	{
		auto fn = found->get()->GetFilename();
		fn = fn.substr(0, fn.size() - 4) + "_lit.tsx";
		auto tileSetLit = AssetCache<TileSet>::Get()->LoadAsset(fn);
		worldTilesLitLayer->ReplaceTileSet(*found, tileSetLit);
	}
	auto worldTilesLit = MakeTiles(worldTilesLitLayer, tilesTM);
	m_worldTilesLitComp = worldTilesLit;
	m_drawTilesLitComp.push_back(worldTilesLit);
	worldTilesLit->SetRenderLayer("fgLightMask");

	// Create tile layer draw components
	for(auto tileLayer : m_tileMap->GetLayers()) {
		bool noDraw = tileLayer->GetName().find("_nodraw") != std::string::npos;
		if(tileLayer->GetLayerType() == eTileLayerType::Tile && tileLayer->GetName() != "TileCollision" && 
			tileLayer->GetName() != "WorldOverlap" && tileLayer->GetName() != "World" && 
			tileLayer->GetName() != "WorldBkg" && !noDraw)
			{
			auto tiles = MakeTiles(tileLayer, tilesTM);
			m_drawTilesComp.push_back(tiles);
			tiles->SetRenderLayer("fgGameplay");
		}
	}
	// Spawn things & create/populate rooms
	auto spawnerObjects = m_tileMap->GetObjectsByType("Spawner");
	auto roomObjs = m_tileMap->GetObjectsByType("Room");
	auto roomNumber = 1;
	for(auto roomObj : roomObjs) {
		auto room = std::make_shared<Room>();
		room->Id = std::stoi(roomObj->GetName());
		auto roomBox = roomObj->GetBox();
		roomBox.w = roomBox.w - roomBox.x; // HACK: remove once Tim fixes the GetBox() func!
		roomBox.h = roomBox.h - roomBox.y; // HACK: remove once Tim fixes the GetBox() func!
		roomBox.y = roomBox.y - roomBox.h; // HACK: remove once Tim fixes the GetBox() func!
		room->bounds = roomBox;
		m_rooms.push_back(room);
		roomNumber++;
	}


	auto lavaObjs = m_tileMap->GetObjectsByType("Lava");
	auto lavaNumber = 1;
	for(auto lavaObj : lavaObjs) {
		auto lavaBox = lavaObj->GetBox();
		lavaBox.w = lavaBox.w - lavaBox.x; // HACK: remove once Tim fixes the GetBox() func!
		lavaBox.h = lavaBox.h - lavaBox.y; // HACK: remove once Tim fixes the GetBox() func!
		lavaBox.y = lavaBox.y - lavaBox.h; // HACK: remove once Tim fixes the GetBox() func!
		//std::cout << "lavaBox #" << lavaNumber << ":  x=" << lavaBox.x << "  y=" << lavaBox.y << "  w=" << lavaBox.w << "  h=" << lavaBox.h << std::endl;
		Actor::Spawn<Lava>(AsShared<GameWorld>(), { lavaBox.GetCenter() }, lavaBox);
		lavaNumber++;
	}

	// Setup vectors for actor storage
	std::vector<std::shared_ptr<CreatureSpawner>> spawners;
	std::vector<std::shared_ptr<TileObject>> lasers;
	std::vector<std::shared_ptr<Door>> doors;
	std::vector<std::shared_ptr<Suit>> suits;
	std::vector<std::shared_ptr<Trigger>> triggers;

	// Spawn all the things
	for(std::shared_ptr<TileObject> spawnerObject : spawnerObjects) {
		Transform spawnTM = tilesTM * Transform{ spawnerObject->GetPos() };
		if(std::optional<std::string> spawnType = spawnerObject->GetPropertyAs<std::string>("SpawnType")) {

			// Player spawn
			if(spawnType == "player") {
				auto playerSpawnPos = spawnerObject->GetCentroid();
				if(m_playerStatus->HasSpawnPos()) {
					playerSpawnPos = m_playerStatus->GetSpawnPos();
				}
				m_player = Actor::Spawn<Player>(AsShared<GameWorld>(), { playerSpawnPos });
			}

			// Creature spawns
			if(spawnType == "ship") {
				auto spawnPos = spawnerObject->GetCentroid();
				auto direction = spawnerObject->GetFlipHori();
				auto rotation = spawnerObject->GetRotation();
				auto name = spawnerObject->GetName();
				spawners.push_back(	Actor::Spawn<CreatureSpawner>(AsShared<GameWorld>(), { spawnPos }, 
									Ship::GetSpawnerDef(), direction, rotation, name));
			}
			if(spawnType == "pirate") {
				auto spawnPos = spawnerObject->GetCentroid() + Vec2f{ 0.0f, 0.0f };
				spawners.push_back(Actor::Spawn<CreatureSpawner>(AsShared<GameWorld>(), { spawnPos }, Pirate::GetSpawnerDef()));
			}
			if(spawnType == "turret") {
				auto spawnPos = spawnerObject->GetCentroid() + Vec2f{ 0.0f, 0.0f };
				spawners.push_back(Actor::Spawn<CreatureSpawner>(AsShared<GameWorld>(), { spawnPos }, Turret::GetSpawnerDef()));
			}
			if(spawnType == "turretstill") {
				auto spawnPos = spawnerObject->GetCentroid() + Vec2f{ 0.0f, 0.0f };
				spawners.push_back(	Actor::Spawn<CreatureSpawner>(AsShared<GameWorld>(), { spawnPos }, 
									TurretStill::GetSpawnerDef()));
			}
			if(spawnType == "blobber") {
				auto spawnPos = spawnerObject->GetCentroid() + Vec2f{ 0.0f, 0.0f };
				spawners.push_back(Actor::Spawn<CreatureSpawner>(AsShared<GameWorld>(), { spawnPos }, Blobber::GetSpawnerDef()));
			}
			if(spawnType == "rammer") {
				auto spawnPos = spawnerObject->GetCentroid() + Vec2f{ 0.0f, 0.0f };
				auto spawnDirection = spawnerObject->GetFlipHori();
				spawners.push_back(	Actor::Spawn<CreatureSpawner>(AsShared<GameWorld>(), { spawnPos }, 
									Rammer::GetSpawnerDef(), spawnDirection));
			}
			if(spawnType == "charger") {
				auto spawnPos = spawnerObject->GetCentroid() + Vec2f{ 0.0f, 0.0f };
				spawners.push_back(Actor::Spawn<CreatureSpawner>(AsShared<GameWorld>(), { spawnPos }, Charger::GetSpawnerDef()));
			}
			if(spawnType == "monopod") {
				auto monopodSpawnPos = spawnerObject->GetCentroid() + Vec2f{ 0.0f, -4.0f };
				spawners.push_back(Actor::Spawn<CreatureSpawner>(AsShared<GameWorld>(), { monopodSpawnPos },
									Monopod::GetSpawnerDef()));
			}
			if(spawnType == "dropper") {
				auto dropperSpawnPos = spawnerObject->GetCentroid() + Vec2f{ 0.0f, -4.0f };
				spawners.push_back(	Actor::Spawn<CreatureSpawner>(AsShared<GameWorld>(), { dropperSpawnPos },
									Dropper::GetSpawnerDef()));
			}
			if(spawnType == "sentry") {
				auto sentrySpawnPos = spawnerObject->GetCentroid();
				auto sentryDirection = spawnerObject->GetFlipHori();
				spawners.push_back(	Actor::Spawn<CreatureSpawner>(AsShared<GameWorld>(), { sentrySpawnPos },
									Sentry::GetSpawnerDef(), sentryDirection));
			}
			if(spawnType == "swooper") {
				auto swooperSpawnPos = spawnerObject->GetCentroid();
				spawners.push_back(	Actor::Spawn<CreatureSpawner>(AsShared<GameWorld>(), { swooperSpawnPos },
									Swooper::GetSpawnerDef()));
			}
			if(spawnType == "crawler") {
				auto crawlerSpawnPos = spawnerObject->GetCentroid();
				auto crawlerRotation = spawnerObject->GetRotation();
				auto crawlerDirection = spawnerObject->GetFlipHori();
				spawners.push_back(	Actor::Spawn<CreatureSpawner>(AsShared<GameWorld>(), { crawlerSpawnPos },
									Crawler::GetSpawnerDef(), crawlerDirection));
			}
			if(spawnType == "piper") {
				auto piperSpawnPos = spawnerObject->GetCentroid();
				spawners.push_back(	Actor::Spawn<CreatureSpawner>(AsShared<GameWorld>(), { piperSpawnPos },
									Piper::GetSpawnerDef()));
			}
			if(spawnType == "cruiser") {
				auto cruiserSpawnPos = spawnerObject->GetCentroid();
				auto cruiserDirection = spawnerObject->GetFlipHori();
				spawners.push_back(	Actor::Spawn<CreatureSpawner>(AsShared<GameWorld>(), { cruiserSpawnPos }, 
									Cruiser::GetSpawnerDef(), cruiserDirection));
			}
			if(spawnType == "laser") {
				auto spawnPos = spawnerObject->GetCentroid();
				auto direction = spawnerObject->GetFlipHori();
				auto rotation = spawnerObject->GetRotation();
				auto name = spawnerObject->GetName();
				spawners.push_back(	Actor::Spawn<CreatureSpawner>(AsShared<GameWorld>(), { spawnPos }, 
									Laser::GetSpawnerDef(), direction, rotation, name));
			}

			// World object spawns
			if(spawnType == "door") {
				auto doorColor = spawnerObject->GetPropertyAs<std::string>("DoorType");
				auto doorIsVertical = spawnerObject->GetPropertyAs<std::string>("isVertical");
				auto doorIsVerticalValue = doorIsVertical.value() == "false" ? false : true;
				auto doorColorValue = (doorColor.value_or("blue") == "blue") ? eDoorColor::Blue : eDoorColor::Red;
				if(doorColor == "") {
					doorColorValue = eDoorColor::None;
				}
				auto doorOffset = Vec2f{ 0.0f, -48.0f };
				auto doorSpawnPos = spawnerObject->GetCentroid();
				auto doorId = spawnerObject->GetObjectId();
				doors.push_back(Actor::Spawn<Door>(	AsShared<GameWorld>(), { doorSpawnPos + doorOffset }, doorColorValue, doorId, 
													doorIsVerticalValue));
			}
			if(spawnType == "suit") {
				auto spawnPos = spawnerObject->GetCentroid() + Vec2f{ 0.0f, 0.0f };
				suits.push_back(Actor::Spawn<Suit>(AsShared<GameWorld>(), { spawnPos }));
			}
			if(spawnType == "trigger") {
				auto tTargetString = spawnerObject->GetName();
				eTriggerTarget tTarget;
				if(tTargetString == "pirate") {
					tTarget = eTriggerTarget::Pirate;
				}
				else if(tTargetString == "boss") {
					tTarget = eTriggerTarget::Boss;
				}
				auto triggerBox = spawnerObject->GetBox();
				auto triggerID = spawnerObject->GetObjectId();
				triggerBox.w = triggerBox.w - triggerBox.x; // HACK: remove once Tim fixes the GetBox() func!
				triggerBox.h = triggerBox.h - triggerBox.y; // HACK: remove once Tim fixes the GetBox() func!
				triggerBox.y = triggerBox.y - triggerBox.h; // HACK: remove once Tim fixes the GetBox() func!
				triggers.push_back(Actor::Spawn<Trigger>(	AsShared<GameWorld>(), { triggerBox.GetCenter() }, tTarget, 
															triggerBox, triggerID));
			}

			// Item spawns
			if(spawnType == "item_ball") {
				if(!m_playerStatus->IsBallUnlocked()) {
					auto spawnPos = spawnerObject->GetCentroid();
					auto objId = spawnerObject->GetObjectId();
					Actor::Spawn<Pickup>(AsShared<GameWorld>(), { spawnPos }, objId, g_pickupBall);
				}
			}
			if(spawnType == "item_bomb") {
				if(!m_playerStatus->IsBombUnlocked()) {
					auto spawnPos = spawnerObject->GetCentroid();
					auto objId = spawnerObject->GetObjectId();
					Actor::Spawn<Pickup>(AsShared<GameWorld>(), { spawnPos }, objId, g_pickupBomb);
				}
			}
			if(spawnType == "item_energyTank") {
				auto spawnPos = spawnerObject->GetCentroid();
				auto objId = spawnerObject->GetObjectId();
				Actor::Spawn<Pickup>(AsShared<GameWorld>(), { spawnPos }, objId, g_pickupEnergyTank);
			}
			if(spawnType == "item_missileTank") {
				auto spawnPos = spawnerObject->GetCentroid();
				auto objId = spawnerObject->GetObjectId();
				Actor::Spawn<Pickup>(AsShared<GameWorld>(), { spawnPos }, objId, g_pickupMissileTank);
			}
			if(spawnType == "item_grenadeTank") {
				auto spawnPos = spawnerObject->GetCentroid();
				auto objId = spawnerObject->GetObjectId();
				Actor::Spawn<Pickup>(AsShared<GameWorld>(), { spawnPos }, objId, g_pickupGrenadeTank);
			}
			if(spawnType == "item_longBeam") {
				if(!m_playerStatus->IsLongBeamUnlocked()) {
					auto spawnPos = spawnerObject->GetCentroid();
					auto objId = spawnerObject->GetObjectId();
					Actor::Spawn<Pickup>(AsShared<GameWorld>(), { spawnPos }, objId, g_pickupLongBeam);
				}
			}
			if(spawnType == "item_iceBeam") {
				if(!m_playerStatus->IsIceBeamUnlocked()) {
					auto spawnPos = spawnerObject->GetCentroid();
					auto objId = spawnerObject->GetObjectId();
					Actor::Spawn<Pickup>(AsShared<GameWorld>(), { spawnPos }, objId, g_pickupIceBeam);
				}
			}
			if(spawnType == "item_waveBeam") {
				if(!m_playerStatus->IsWaveBeamUnlocked()) {
					auto spawnPos = spawnerObject->GetCentroid();
					auto objId = spawnerObject->GetObjectId();
					Actor::Spawn<Pickup>(AsShared<GameWorld>(), { spawnPos }, objId, g_pickupWaveBeam);
				}
			}
			if(spawnType == "item_gravityBoots") {
				if(!m_playerStatus->IsHighJumpUnlocked()) {
					auto spawnPos = spawnerObject->GetCentroid();
					auto objId = spawnerObject->GetObjectId();
					Actor::Spawn<Pickup>(AsShared<GameWorld>(), { spawnPos }, objId, g_pickupGravityBoots);
				}
			}
			if(spawnType == "item_grapple") {
				if(!m_playerStatus->IsHighJumpUnlocked()) {
					auto spawnPos = spawnerObject->GetCentroid();
					auto objId = spawnerObject->GetObjectId();
					Actor::Spawn<Pickup>(AsShared<GameWorld>(), { spawnPos }, objId, g_pickupGrapple);
				}
			}
			if(spawnType == "item_wallJump") {
				if(!m_playerStatus->IsWallJumpUnlocked()) {
					auto spawnPos = spawnerObject->GetCentroid();
					auto objId = spawnerObject->GetObjectId();
					Actor::Spawn<Pickup>(AsShared<GameWorld>(), { spawnPos }, objId, g_pickupWallJump);
				}
			}
			if(spawnType == "item_charge") {
				if(!m_playerStatus->IsChargeUnlocked()) {
					auto spawnPos = spawnerObject->GetCentroid();
					auto objId = spawnerObject->GetObjectId();
					Actor::Spawn<Pickup>(AsShared<GameWorld>(), { spawnPos }, objId, g_pickupCharge);
				}
			}
			if(spawnType == "savePoint") {
				auto spawnPos = spawnerObject->GetCentroid();
				Actor::Spawn<SavePoint>(AsShared<GameWorld>(), { spawnPos });
			}
		}
	}

	// Assign all spawned things to their overlapping and/or enclosing rooms
	auto AssignToEnclosingRoom = [this](std::shared_ptr<CreatureSpawner> in_creatureSpawner) {
		for(auto room : m_rooms) {
			if(Math::PointInBox(room->bounds, in_creatureSpawner->GetWorldPos())) {
				room->spawners.push_back(in_creatureSpawner);
			}
		}
	};
	for(auto spawner : spawners) {
		AssignToEnclosingRoom(spawner);
	}
	for(auto room : m_rooms) {
		for(auto door : doors) {
			if(Math::OverlapBoxes(room->bounds, door->GetBounds())) {
				room->doors.push_back(door);
			}
		}
		for(auto trigger : triggers) {
			if(Math::OverlapBoxes(room->bounds, trigger->GetBounds())) {
				room->triggers.push_back(trigger);
			}
		}
	}
	m_hud = Spawn<Hud>({});
	m_cameraManager = Spawn<CameraManager>({});
}
Vec2f GameWorld::GetPlayerWorldPos(bool shotTarget) const {
	if(shotTarget) {
		return m_player ? m_player->GetTargetPos() : m_lastPlayerTargetPos;
	}
	return m_player ? m_player->GetWorldPos() : m_lastPlayerPos;
}
void GameWorld::SetPlayerWorldPos(const Vec2f& in_pos) {
	m_player->SetWorldPos(in_pos);
}
void GameWorld::ZapPlayer(std::shared_ptr<Creature> in_creature, const Vec2f& in_zapPos) {
	m_player->LaserTouch(in_creature, in_zapPos);
}
void GameWorld::MovePlayer(const Vec2f& in_pos, bool in_bTeleport) {
	m_player->Move(in_pos, in_bTeleport);
}
int32_t GameWorld::GetPlayerHealth(bool returnPercentage) const {
	auto playerStatus = GetPlayerStatus();
	auto currentHealth = playerStatus->GetEnergy();
	if(returnPercentage) {
		auto maxHealth = (playerStatus->GetEnergyTankCount() * 100) + 99;
		return (currentHealth / maxHealth) * 100;
	}
	else {
		return currentHealth;
	}
}
void GameWorld::ChangePlayerHealth(int32_t in_change) {
	auto playerStatus = GetPlayerStatus();
	playerStatus->AddEnergy(in_change);

	// If player is losing health
	if(in_change < 0.0f) {
		// Camera shake contribution
		auto distressChange = Math::MapRange(abs(float(in_change)), 0.0f, 10.0f, 0.0f, 2.0f, true);
		GetCameraManager()->AddDistress(distressChange);
	}
}
void GameWorld::EnablePlayerControls() {
	m_player->SetControlsEnabled(true);
}
void GameWorld::DisablePlayerControls() {
	m_player->SetControlsEnabled(false);
}
void GameWorld::AttachPlayerTo(std::shared_ptr<Actor> in_actor, bool in_keepWorldTransform) {
	m_player->AttachToActor(in_actor, in_keepWorldTransform);
}
void GameWorld::DetachPlayer(bool in_keepWorldTransform) {
	m_player->Detach(in_keepWorldTransform);
}
void GameWorld::SetPlayerGravityModifier(float in_gravityMod) {
	m_player->SetGravityModifier(in_gravityMod);
}
std::shared_ptr<Room> GameWorld::GetPlayerRoom() const {
	std::shared_ptr<Room> playerRoom;
	auto playerPos = GetPlayerWorldPos();
	for(auto room : m_rooms) {
		if(Math::PointInBox(room->bounds, playerPos)) {
			playerRoom = room;
			break;
		}
	}
	if(playerRoom) {
		return playerRoom;
	}
	//printf("ERROR: Player isn't in a room!");
	return nullptr;
}
std::shared_ptr<Room> GameWorld::GetEnclosingRoom(const Vec2f& in_pos) const {
	std::shared_ptr<Room> actorRoom;
	for(auto room : m_rooms) {
		if(Math::PointInBox(room->bounds, in_pos)) {
			actorRoom = room;
			break;
		}
	}
	if(actorRoom) {
		return actorRoom;
	}
	else {
		printf("ERROR: Actor isn't in a room!");
	}
	return nullptr;
}
std::optional<bool> GameWorld::RoomEnclosesCreatureWorldCollision(std::shared_ptr<Creature> in_creature) const {
	std::shared_ptr<Room> spawnerRoom;
	for(auto room : m_rooms) {
		if(std::find(room->spawners.begin(), room->spawners.end(), in_creature->GetSpawner()) != room->spawners.end()) {
			spawnerRoom = room;
			break;
		}
	}
	return Math::BoxContainsBox(spawnerRoom->bounds, in_creature->GetCollisionBoxWorld());
 }
void GameWorld::OnRoomChanged(std::shared_ptr<Room> in_oldRoom, std::shared_ptr<Room> in_newRoom) {
	ClearInvalidRoomActors(in_oldRoom);
}
WeakTaskHandle GameWorld::OnItemPickup(std::optional<std::pair<std::wstring, std::wstring>> in_itemTip) {
	if(!GameLoop::ShouldDisplayDebug()) {
		return m_taskMgr.RunManaged(ItemPickupFanfare(in_itemTip)); //< RunManaged will survive the collapse of this stack frame
	}
	return {};
}
void GameWorld::OnPause() {
	m_pauseMenu = Actor::Spawn<PauseMenu>({});
}
bool GameWorld::CreatureIsOffCamera(std::shared_ptr<Creature> in_creature) const {
	auto worldView = GetWindow()->GetWorldView();
	return Math::OverlapBoxes(worldView, in_creature->GetCollisionBoxWorld()) ? false : true;
 }
std::vector<std::shared_ptr<Creature>> GameWorld::GetOnCameraCreatures() const
{
	std::vector<std::shared_ptr<Creature>> onCameraCreatures;
	for(auto creature : GetPlayerRoom()->creatures) {
		if(!CreatureIsOffCamera(creature)) {
			onCameraCreatures.push_back(creature);
		}
	}
	return onCameraCreatures;
}
bool GameWorld::ProjectileIsOffCamera(std::shared_ptr<Projectile> in_projectile) const {
	auto worldView = GetWindow()->GetWorldView();
	return Math::OverlapBoxes(worldView, in_projectile->GetCollisionBoxWorld()) ? false : true;
}
Task<> GameWorld::Fade(	std::shared_ptr<SpriteComponent> in_sprite, std::shared_ptr<TextComponent> in_text, float in_duration, 
							bool in_fadeToOpaque) {
	auto world = GetWorld();
	auto fadeTotalDuration = in_duration;
	auto elapsedTime = 0.0f;
	auto currentColor = sf::Color::White;
	if(in_sprite) {
		currentColor = in_sprite->GetColor();
	}
	else if(in_text) {
		currentColor = in_text->GetColor();
	}
	currentColor = sf::Color((uint8_t)currentColor.r, (uint8_t)currentColor.g, (uint8_t)currentColor.b, (uint8_t)255);
	int32_t targetAlpha = in_fadeToOpaque ? 255 : 0;

	// Early-out if duration is 0.0f (i.e. instantaneous)
	if(fadeTotalDuration <= 0.0f) {
		if(in_text) {
			in_text->SetColor(sf::Color(currentColor.r, currentColor.g, currentColor.b, targetAlpha));
		}
		else {
			in_sprite->SetColor(currentColor.r, currentColor.g, currentColor.b, targetAlpha);
		}
		co_return;
	}
	while(true) {
		elapsedTime += DT();
		auto fadeAlpha = int32_t(world->EaseAlpha(elapsedTime, fadeTotalDuration, Math::EaseInOutSmoothstep) * 255.0f);
		if(!in_fadeToOpaque) {
			fadeAlpha = 255 - fadeAlpha;
		}
		if(in_text) {
			in_text->SetColor(	sf::Color((uint8_t)currentColor.r, (uint8_t)currentColor.g, (uint8_t)currentColor.b, 
								(uint8_t)fadeAlpha));
		}
		else {
			in_sprite->SetColor((uint8_t)currentColor.r, (uint8_t)currentColor.g, (uint8_t)currentColor.b, (uint8_t)fadeAlpha);
		}
		if(elapsedTime >= fadeTotalDuration) {
			if(in_text) {
				in_text->SetColor(	sf::Color((uint8_t)currentColor.r, (uint8_t)currentColor.g, (uint8_t)currentColor.b, 
									(uint8_t)targetAlpha));
			}
			else {
				in_sprite->SetColor((uint8_t)currentColor.r, (uint8_t)currentColor.g, (uint8_t)currentColor.b, (uint8_t)targetAlpha);
			}
			co_return;
		}
		co_await Suspend();
	}
}
float GameWorld::EaseAlpha(float in_elapsedTime, float in_targetDuration, std::function<float(float)> in_easingFunc) {
	auto elapsedTimeNorm = (in_elapsedTime - 0.0f) / (in_targetDuration - 0.0f);
	auto alpha = in_easingFunc(elapsedTimeNorm);
	return alpha;
};
Vec2f GameWorld::ViewToWorld(const Vec2f& in_pos) {
	auto window = GetWindow();
	return window->GetWorldView().GetTopLeft() + Vec2f{ in_pos.x, -in_pos.y };
}
int32_t GameWorld::GetPlayerMissiles(bool returnPercentage) const {
	auto playerStatus = GetPlayerStatus();
	auto currentMissiles = playerStatus->GetMissileCount();
	if(returnPercentage) {
		auto maxMissiles = playerStatus->GetMissileTankCount() * 5;
		if(maxMissiles) {
			auto percentageMissiles = (currentMissiles / maxMissiles) * 100;
			return percentageMissiles;
		}
		else {
			return 0;
		}
	}
	else {
		return currentMissiles;
	}
}
int32_t GameWorld::GetPlayerSuperMissiles(bool returnPercentage) const {
	auto playerStatus = GetPlayerStatus();
	auto currentMissiles = playerStatus->GetSuperMissileCount();
	if(returnPercentage) {
		auto maxMissiles = playerStatus->GetSuperMissileTankCount() * 5;
		if(maxMissiles) {
			auto percentageMissiles = (currentMissiles / maxMissiles) * 100;
			return percentageMissiles;
		}
		else {
			return 0;
		}
	}
	else {
		return currentMissiles;
	}
}
int32_t GameWorld::GetPlayerGrenades(bool returnPercentage) const {
	auto playerStatus = GetPlayerStatus();
	auto currentBombs = playerStatus->GetGrenadeCount();
	if(returnPercentage) {
		auto maxBombs = playerStatus->GetGrenadeTankCount() * 5;
		if(maxBombs) {
			auto percentageBombs = (currentBombs / maxBombs) * 100;
			return percentageBombs;
		}
		else {
			return 0;
		}
	}
	else {
		return currentBombs;
	}
}
int32_t GameWorld::GetPlayerEnergyTankCount() const {
	return m_playerStatus ? m_playerStatus->GetEnergyTankCount() : 0;
}
Box2f GameWorld::GetPlayerWorldCollisionBox() const {
	return m_player->GetCollisionBoxWorld();
}
bool GameWorld::IsPlayerGrounded() const {
	return m_player->IsGrounded();
}
bool GameWorld::IsPlayerFish() const {
	return m_player->IsFish();
}
Task<> GameWorld::ItemPickupFanfare(std::optional<std::pair<std::wstring, std::wstring>> in_itemTip) {
	auto isPausedToken = GameBase::Get()->ShouldPause.TakeToken(__FUNCTION__);
	auto itemTip = in_itemTip.has_value() ? in_itemTip.value() : std::pair<std::wstring, std::wstring>{ L"Heading", L"Body Text" };
	auto textHeading = MakeAndConfigText(g_pickupInfoHeading, itemTip.first, Vec2f{0.0f, 164.0f});
	auto textBody = MakeAndConfigText(g_pickupInfoBody, itemTip.second, Vec2f{ 0.0f, 178.0f });

	co_await AudioManager::Get()->FadeMusic(0.5f, 0.0f);
	AudioManager::Get()->PlayMusic("GetItemJingle", 0.0f, 0.2f, false);
	co_await WaitSeconds(3.5f, [] {return Time::RealTime(); });
	AudioManager::Get()->ResumePrevMusic();
	co_await AudioManager::Get()->FadeMusic(1.0f, 0.05f, false);
	textHeading->Destroy();
	textBody->Destroy();
}
void GameWorld::Cleanup() {
	if(s_gameWorld == this) {
		s_gameWorld = {};
	}
}
Task<> GameWorld::ToggleDebug() {
	while(true) {
		if(m_collisionTilesComp) {
			if(GameLoop::ShouldDisplayDebug()) {
				m_collisionTilesComp->SetHidden(false);
			}
			else {
				m_collisionTilesComp->SetHidden(true);
			}
		}
		co_await Suspend();
	}
}
void GameWorld::ClearInvalidRoomActors(std::shared_ptr<Room> in_room) {
	if(in_room) {
		auto& playerRoomCreatures = in_room->creatures;
		if(playerRoomCreatures.size() > 0) {
			EraseInvalid(in_room->creatures);
		}
	}
}
