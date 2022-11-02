#include "ColliderComponent.h"

#include "Engine/Components/TilesComponent.h"
#include "Engine/TileMap.h"

void ColliderComponent::Initialize()
{
	Super::Initialize();

	if (auto world = m_world.lock()) {
		world->AddCollider(this);
	}
}

void ColliderComponent::Destroy()
{
	if (auto world = m_world.lock()) {
		world->RemoveCollider(this);
	}

	Super::Destroy();
}

Box2f ColliderComponent_TilesComponent::GetBoundingBoxLocal() const
{
	return Box2f(m_tilesComp->GetTileLayer()->GetGridBoundingBox());
}

std::optional<Math::BoxSweepResults> ColliderComponent_TilesComponent::SweepBoxAgainstThis(Box2f in_boxToSweep, Vec2f in_sweepVec) const
{

	return Math::SweepBoxAgainstGrid(in_boxToSweep, in_sweepVec,
		m_tilesComp->GetGridToWorldTransform(),
		m_tilesComp->GetTileLayer()->GetGridBoundingBox(),
		[this](Vec2i in_testPos) {
			return m_tilesComp->GetTileLayer()->GetTile(in_testPos) > 0;
		}
	);
}
