#include "TilesComponent.h"

#include "Engine/Game.h"
#include "Engine/GameWindow.h"
#include "Engine/TileMap.h"
#include "Engine/MathCore.h"
#include "Engine/RenderTexture.h"

#include <SFML/Graphics.hpp>

//--- TilesComponent ---//
void TilesComponent::Draw()
{
	DrawComponent::Draw();

	if(!m_tileLayer)
	{
		return;
	}

	// Create sprite
	const auto& worldTransform = GetWorldTransform();
	sf::Sprite sprite;
	sprite.setPosition(std::round(worldTransform.pos.x), std::round(worldTransform.pos.y));
	sprite.setRotation(worldTransform.rot);
	sprite.setScale(worldTransform.scale.x, -worldTransform.scale.y);

	// Draw the tile layer
	const auto& gridDims = m_tileLayer->GetGridDims();
	const auto& tileDims = m_tileLayer->GetTileDims();
	sprite.setOrigin(0.0f, (float)tileDims.y);

	if(m_tileLayer && m_tileLayer->GetLayerType() == eTileLayerType::Tile)
	{
		auto gridDims = m_tileLayer->GetGridDims();
		auto worldView = GetTargetRenderTexture()->GetView();
		auto upperLeft = WorldPosToGridPos(worldView.GetMin());
		auto lowerRight = WorldPosToGridPos(worldView.GetMax());
		MinMaxi colRange{ upperLeft.x, lowerRight.x };
		MinMaxi rowRange{ upperLeft.y, lowerRight.y };
		auto renderTarget = GetTargetSFMLRenderTexture();

		// iterate through tilesets
		size_t tilesetIdx = 0;
		for(auto tileSet : m_tileLayer->GetTileSets())
		{
			MinMaxi tileIdRange = m_tileLayer->GetTileIdRanges()[tilesetIdx];
			sprite.setTexture(tileSet->GetTexture()->GetSFMLTexture());
			const auto& tileBoxes = tileSet->GetTiles();
			const auto& tiles = m_tileLayer->GetTiles();
			for(auto row = rowRange.m_min; row <= rowRange.m_max; ++row)
			{
				for(auto col = colRange.m_min; col <= colRange.m_max; ++col)
				{
					// Set tile sub-texture
					Vec2i tileGridPos{ col, row };
					auto tileIdx = m_tileLayer->GridPosToTileIdx(tileGridPos); // Index in the tiles array
					auto tile = tiles[tileIdx]; // Which tile to draw
					if(!tileIdRange.Contains_InclExcl(tile))
					{
						continue;
					}
					auto rect = Box2i(tileBoxes[(size_t)tile - (size_t)tileIdRange.m_min]);
					sprite.setTextureRect({ rect.x, rect.y, rect.w, rect.h });

					// Set tile position
					Vec2f tilePos = { (float)(tileGridPos.x * tileDims.x), (float)(tileGridPos.y * tileDims.y) };
					tilePos = worldTransform.TransformPoint(tilePos);
					sprite.setPosition(tilePos.x, tilePos.y);

					// Draw the tile
					renderTarget->draw(sprite);
				}
			}
			++tilesetIdx;
		}
	}
}
void TilesComponent::SetTileLayer(std::shared_ptr<TileLayer> in_tileLayer)
{
	m_tileLayer = in_tileLayer;
}

std::shared_ptr<TileLayer> TilesComponent::GetTileLayer() const
{
	return m_tileLayer;
}

Transform TilesComponent::GetGridToWorldTransform() const
{
	Transform tm = GetWorldTransform();
	tm.scale *= m_tileLayer->GetTileDims();
	return tm;
}

Vec2i TilesComponent::WorldPosToGridPos(Vec2f in_worldPos) const
{
	Vec2f gridSpacePos_Float = GetGridToWorldTransform().InvTransformPoint(in_worldPos);
	return Math::Floor(gridSpacePos_Float);
}

Box2f TilesComponent::GridPosToWorldBox(Vec2i in_gridPos) const
{
	return Box2f::FromBottomLeft(in_gridPos, {1, 1}).TransformedBy(GetGridToWorldTransform());
}
