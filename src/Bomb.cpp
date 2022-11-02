#include "Bomb.h"

#include "GameEnums.h"
#include "Creature.h"
#include "Player.h"
#include "Door.h"
#include "AudioManager.h"
#include "GameWorld.h"
#include "DestroyedTile.h"
#include "Character.h"
#include "Engine/Components/TilesComponent.h"
#include "Engine/TileMap.h"

void Bomb::Initialize() {
	GameActor::Initialize();
	m_sprite = MakeSprite(Transform::Identity);
	m_sprite->PlayAnim("Bomb/Bomb", true);
	SetDamageInfo(5, DF_Explodes | DF_Player);
}
Task<> Bomb::ManageActor() {
	while(true) {
		co_await WaitSeconds(m_fuseTime);
		m_sprite->PlayAnim("Explosion2/Explosion", false);
		MakeAoeSensor();
		m_bIsExploding = true;
		AudioManager::Get()->PlaySound("Explosion");
		TryDestroyTiles();
		auto elapsedTime = 0.0f;
		while(elapsedTime < m_explosionTime) {
			co_await Suspend();
			elapsedTime += DT();
		}
		DeferredDestroy();
	}
}
void Bomb::TryDestroyTiles() {
	auto collisionTiles = GetWorld()->GetCollisionTilesComp();
	auto collisionLayer = collisionTiles->GetTileLayer();
	auto worldTiles = GetWorld()->GetWorldTilesComp();
	auto worldLayer = worldTiles->GetTileLayer();
	auto bombGridPos = worldTiles->WorldPosToGridPos(GetWorldPos());
	auto gridSize = 16;
	std::vector<Vec2i> offsets = { {0, 1}, {1, 0}, {0, -1}, {-1, 0}, {0, 0} };
	for(int i = 0; i < 5; i++) {
		auto offsetGridPos = bombGridPos + offsets[i];
		auto currentTileId = worldLayer->GetTile(offsetGridPos);

		// Destroy relevant tiles
		if(currentTileId == 54 || currentTileId == 279) { //< 54 and 279 are the breakable brick tiles
			worldLayer->SetTile(offsetGridPos, 8); //< 8 is the first blank ID in the tileset (0 is a brick tile)
			collisionLayer->SetTile(offsetGridPos, 0);
			Transform destroyedTileTransform = { (offsetGridPos * gridSize) + Vec2i{gridSize / 2, gridSize / 2}, 0.0f, Vec2f::One };
			Actor::Spawn<DestroyedTile>(GetWorld(), destroyedTileTransform, currentTileId);
		}
	}
}
void Bomb::MakeAoeSensor() {
	Circle projectileCollisionCircle = { 16.0f };
	SensorShape projectileSensorShape;
	projectileSensorShape.SetCircle(projectileCollisionCircle);
	auto projectileSensor = MakeSensor(Transform::Identity, projectileSensorShape);
	projectileSensor->SetFiltering(CL_PlayerBomb, CL_Enemy | CL_Player | CL_Door | CL_Pickup);
	projectileSensor->SetTouchCallback([this](bool in_beginning, std::shared_ptr<SensorComponent> in_other) {
		auto alreadyHit = std::find(m_actorsAlreadyHit.begin(), m_actorsAlreadyHit.end(), 
									in_other->GetActor()) != m_actorsAlreadyHit.end();
		if(!alreadyHit && m_bIsExploding && in_beginning) {
			if(std::dynamic_pointer_cast<Creature>(in_other->GetActor())) {
				OnTouchCreature(std::dynamic_pointer_cast<Creature>(in_other->GetActor()));
			}
			if(std::dynamic_pointer_cast<Player>(in_other->GetActor())) {
				OnTouchPlayer(std::dynamic_pointer_cast<Player>(in_other->GetActor()));
			}
			if(std::dynamic_pointer_cast<Door>(in_other->GetActor())) {
				OnTouchDoor(std::dynamic_pointer_cast<Door>(in_other->GetActor()));
			}
		}
	});
}

// Ensuring each actor "hit" by bomb AOE is only hit once
void Bomb::OnTouchCreature(std::shared_ptr<Creature> in_other) {
	m_actorsAlreadyHit.push_back(in_other);
}
void Bomb::OnTouchPlayer(std::shared_ptr<Player> in_other) {
	m_actorsAlreadyHit.push_back(in_other);
}
void Bomb::OnTouchDoor(std::shared_ptr<Door> in_other) {
	m_actorsAlreadyHit.push_back(in_other);
}
