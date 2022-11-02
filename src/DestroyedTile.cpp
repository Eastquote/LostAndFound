#include "DestroyedTile.h"

#include "GameWorld.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/TilesComponent.h"
#include "Engine/TileMap.h"
#include "Engine/DebugDrawSystem.h"

//--- DESTROYED TILE ---//

DestroyedTile::DestroyedTile(int32_t in_tileId) 
	: m_tileId(in_tileId)
{
}
void DestroyedTile::Initialize() {
	GameActor::Initialize();
	m_sprite = MakeSprite(Transform::Identity);
}
Task<> DestroyedTile::ManageActor() {
	while(true) {
		//DrawDebugPoint(GetWorldPos());
		auto collisionTiles = GetWorld()->GetCollisionTilesComp();
		auto collisionLayer = collisionTiles->GetTileLayer();
		auto worldTiles = GetWorld()->GetWorldTilesComp();
		auto worldLayer = worldTiles->GetTileLayer();
		auto tileGridPos = worldTiles->WorldPosToGridPos(GetWorldPos());

		// Dissolve brick tile art
		m_sprite->PlayAnim("DestructTiles/BlueBrickDissolve", false);

		// Delay (this is the period when the location of the destroyed tile is passable by the player)
		co_await WaitSeconds(5.45f);

		// Return brick tile art
		m_sprite->PlayAnim("DestructTiles/BlueBrickReturn", false);
		co_await WaitSeconds(0.08333f);

		// Set the visual and collision tiles back to initial id's
		worldLayer->SetTile(tileGridPos, m_tileId);
		collisionLayer->SetTile(tileGridPos, 237); // 237 is the blocking tile (red diagonal lines)
		DeferredDestroy();
	}
}
