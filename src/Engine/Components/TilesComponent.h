#pragma once

#include "Engine/Components/DrawComponent.h"
#include "Engine/Box.h"

class TileLayer;

// Tiles Component
class TilesComponent : public DrawComponent
{
public:
	virtual void Draw() override;

	void SetTileLayer(std::shared_ptr<TileLayer> in_tileLayer);
	std::shared_ptr<TileLayer> GetTileLayer() const;

	Transform GetGridToWorldTransform() const;
	Vec2i WorldPosToGridPos(Vec2f in_worldPos) const;
	Box2f GridPosToWorldBox(Vec2i in_gridPos) const;

protected:
	std::shared_ptr<TileLayer> m_tileLayer;
};
