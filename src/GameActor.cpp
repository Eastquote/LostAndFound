#include "GameActor.h"

#include "GameWorld.h"
#include "DestroyedTile.h"
#include "SensorManager.h"
#include "Player.h"
#include "Engine/Components/SensorComponent.h"
#include "Engine/Components/ColliderComponent.h"
#include "Engine/Components/TilesComponent.h"
#include "Engine/Components/TextComponent.h"
#include "Engine/Font.h"
#include "Engine/TileMap.h"
#include "Engine/CollisionWorld.h"
#include <iostream>

//--- GAME ACTOR CODE ---//

void GameActor::Initialize() {
	Actor::Initialize();
	m_world = GameWorld::Get();
	SQUID_RUNTIME_CHECK(IsAlive(m_world), "tried to construct GameActor with invalid world");
	m_world->AddActor(std::static_pointer_cast<GameActor>(AsShared()));
	m_damageInfo.m_instigator = AsShared<GameActor>();
}
void GameActor::Destroy() {
	m_world->RemoveActor(std::static_pointer_cast<GameActor>(AsShared()));
	Actor::Destroy();
}
void GameActor::DeferredDestroy() {
	m_world->DeferredDestroy(std::static_pointer_cast<GameActor>(AsShared()));
}
std::shared_ptr<SensorComponent> GameActor::MakeSensor(const Transform& in_transform, const SensorShape& in_shape)
{
	auto sensor = SpawnWithInit<SensorComponent>(in_transform, [this, in_shape](auto in_comp) {
		in_comp->SetActor(AsShared<Actor>());
		in_comp->SetShape(in_shape);
		});
	sensor->SetAttachParent(GetRootComponent(), false);
	auto sensorMgr = GetWorld()->GetSensorManager();
	sensorMgr->RegisterSensor(sensor);
	return sensor;
}
std::shared_ptr<ColliderComponent_Box> GameActor::MakeCollider_Box(	std::shared_ptr<Object> in_owner, 
																		const Transform& in_transform,
																		std::shared_ptr<CollisionWorld> in_world, 
																		Box2f in_boxLocal)
{
	auto colliderBoxComp = SpawnWithInit<ColliderComponent_Box>(in_owner, in_transform, [this](auto in_comp) {
		in_comp->SetActor(AsShared<Actor>());
		}, 
		in_world, in_boxLocal
	);
	colliderBoxComp->SetAttachParent(GetRootComponent(), false);
	return colliderBoxComp;
}
std::shared_ptr<ColliderComponent_TilesComponent> GameActor::MakeCollider_TilesComp(	const Transform& in_transform, 
																						std::shared_ptr<CollisionWorld> in_world,
																						std::shared_ptr<TilesComponent> in_tilesComp)
{
	auto colliderTilesComp = SpawnWithInit<ColliderComponent_TilesComponent>(in_transform, [this](auto in_comp) {
		in_comp->SetActor(AsShared<Actor>());
		}, 
		in_world, in_tilesComp
	);
	colliderTilesComp->SetAttachParent(GetRootComponent(), false);
	return colliderTilesComp;
}
std::shared_ptr<TilesComponent> GameActor::GetCollisionTilesComp() const {
	return GetWorld()->GetCollisionTilesComp();
}
std::shared_ptr<GameWorld> GameActor::GetWorld() const {
	return m_world;
}
Vec2f GameActor::DistanceFromPlayer() const {
	auto playerPos = GetWorld()->GetPlayerWorldPos();
	return GetWorldPos() - playerPos;
}
Vec2f GameActor::DistanceToPlayer() const {
	auto playerPos = GetWorld()->GetPlayerWorldPos();
	return playerPos - GetWorldPos();
}
DamageInfo GameActor::GetDamageInfo() const {
	DamageInfo result = m_damageInfo;
	result.m_damagePos = GetWorldPos();
	return result;
}
bool GameActor::TryDestroyTiles(const Vec2f& in_pos, const Vec2f& in_dir, bool in_bPenetratesWalls, int32_t in_aoe) {
	auto collisionTiles = GetWorld()->GetCollisionTilesComp();
	auto collisionLayer = collisionTiles->GetTileLayer();
	auto worldTiles = GetWorld()->GetWorldTilesComp();
	auto worldLayer = worldTiles->GetTileLayer();
	auto actorGridPos = worldTiles->WorldPosToGridPos(in_pos);
	auto gridSize = 16;
	Vec2i dirCardinal = { int32_t(abs(in_dir.x) < abs(in_dir.y) ? 0 : copysign(1.0, in_dir.x)),
						  int32_t(abs(in_dir.y) < abs(in_dir.x) ? 0 : copysign(1.0, in_dir.y)) };
	if(in_dir.x == in_dir.y) {
		dirCardinal = { (int32_t)copysign(1.0, in_dir.x) , (int32_t)copysign(1.0, in_dir.y) };
	}
	std::vector<Vec2i> offsets = { {0, 0}, dirCardinal };
	if(in_aoe) {
		// Add cardinal tiles to destruction Area Of Effect (AOE)
		offsets = { {0, 1}, {1, 0}, {0, -1}, {-1, 0}, {0, 0} }; //< Cardinal tiles + origin tile
		if(in_aoe > 1) {
			// Increase AOE by adding diagonal tiles as well
			std::vector<Vec2i> diagonals = { {1, 1}, {-1, 1}, {-1, -1}, {1, -1} }; //< Diagonal tiles
			for(auto coord : diagonals) {
				offsets.push_back(coord);
			}
			}
		for(auto& offset : offsets) {
			offset += dirCardinal;
		}
	}
	auto result = false;
	for(int i = 0; i < offsets.size(); i++) {
		auto offsetGridPos = actorGridPos + offsets[i];
		auto currentTileId = worldLayer->GetTile(offsetGridPos);

		// If hitsite is on a destructible tile
		if(currentTileId == 54 || currentTileId == 270) { //< The crumbly brick tiles
			worldLayer->SetTile(offsetGridPos, 0); //< 0 is the first "blank" ID in the GO tileset
			collisionLayer->SetTile(offsetGridPos, 0);
			Transform destroyedTileTransform = { (offsetGridPos * gridSize) + Vec2i{gridSize / 2, gridSize / 2}, 0.0f, Vec2f::One };
			Actor::Spawn<DestroyedTile>(GetWorld(), destroyedTileTransform, currentTileId);
			result = true;
		}

		// If hitsite is inside a non-destructible block
		else if(currentTileId != 54 && !in_bPenetratesWalls) {
			result = true;
		}
	}
	return result;
}
void GameActor::SetDamageInfo(int32_t in_payload, uint32_t in_dmgFlag) {
	m_damageInfo.m_payload = in_payload;
	if(in_dmgFlag) {
		m_damageInfo.m_damageFlags = in_dmgFlag;
	}
}
std::shared_ptr<TextComponent> GameActor::MakeAndConfigText(TextDef& in_def, std::wstring in_text, Vec2f in_posOffset,
	bool in_hidden, int32_t in_drawOrder, std::string in_renderLayer){
	// TODO: Get this working! Strange crashes.
	std::shared_ptr<TextComponent> textComp = MakeText(Transform::Identity);
	auto worldView = GetWindow()->GetWorldView();
	textComp->SetRenderLayer(in_renderLayer);
	textComp->SetFont(in_def.font);
	textComp->GetFont()->SetSmooth(false);
	textComp->SetFontSizePixels(in_def.size);
	textComp->SetText(in_text);
	textComp->SetAlignment(in_def.alignment);
	textComp->SetColor(in_def.color);
	textComp->SetHidden(in_hidden);
	textComp->SetComponentDrawOrder(in_drawOrder);
	textComp->SetWorldPos(GameWorld::Get()->ViewToWorld({ (worldView.w / 2.0f) + in_posOffset.x, in_posOffset.y }));
	return textComp;
}