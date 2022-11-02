#pragma once

#include "Engine/CollisionWorld.h"
#include "Engine/MathGeometry.h"
#include "Engine/Components/SceneComponent.h"

/*
ColliderComponent: 

Represents a shape within a CollisionWorld, which can then be queried/swept against

*/


///////////////////////////////////////////////////////////////////////////////////////////
// ColliderComponent: abstract collider base class with a Transform but no defined shape

class ColliderComponent : public SceneComponent
{
	using Super = SceneComponent;

public:
	ColliderComponent(std::shared_ptr<CollisionWorld> in_world)
		: m_world(in_world)
	{}

	virtual void Initialize() override;
	virtual void Destroy() override;

	virtual Box2f GetBoundingBoxLocal() const = 0;
	virtual std::optional<Math::BoxSweepResults> SweepBoxAgainstThis(Box2f in_boxToSweep, Vec2f in_sweepVec) const = 0;

	Box2f GetBoundingBoxWorld() const { return GetBoundingBoxLocal().TransformedBy(GetWorldTransform()); }

	std::shared_ptr<CollisionWorld> GetWorld() const { return m_world.lock(); }

protected:
	virtual void OnTransformChanged() override {
		if (auto world = GetWorld()) {
			world->OnColliderBoundingBoxChanged(this);
		}
	}

private:
	std::weak_ptr<CollisionWorld> m_world;
};


///////////////////////////////////////////////////////////////////////////////////////////
// ColliderComponent_Box: a box collider

class ColliderComponent_Box : public ColliderComponent
{
public:
	ColliderComponent_Box(std::shared_ptr<CollisionWorld> in_world, Box2f in_boxLocal)
		: ColliderComponent(in_world)
		, m_boxLocal(in_boxLocal)
	{}


	virtual Box2f GetBoundingBoxLocal() const override { return GetBoxLocal(); }

	virtual std::optional<Math::BoxSweepResults> SweepBoxAgainstThis(Box2f in_boxToSweep, Vec2f in_sweepVec) const override { 
		return Math::SweepBoxAgainstBox(in_boxToSweep, in_sweepVec, GetBoxWorld());
	}


	Box2f GetBoxLocal() const { return m_boxLocal; }
	Box2f GetBoxWorld() const { return m_boxLocal.TransformedBy(GetWorldTransform()); }

	void SetBoxLocal(Box2f in_boxLocal) { 
		m_boxLocal = in_boxLocal;

		if (auto world = GetWorld()) {
			world->OnColliderBoundingBoxChanged(this);
		}
	}


private:
	Box2f m_boxLocal;
};  


///////////////////////////////////////////////////////////////////////////////////////////
// Collider_TilesComponent: wrapper around TilesComponent that acts as a Collider

class TilesComponent;

class ColliderComponent_TilesComponent : public ColliderComponent
{
public:
	ColliderComponent_TilesComponent(std::shared_ptr<CollisionWorld> in_world, std::shared_ptr<TilesComponent> in_tilesComp)
		: ColliderComponent(in_world)
		, m_tilesComp(in_tilesComp)
	{}


	virtual Box2f GetBoundingBoxLocal() const override;

	virtual std::optional<Math::BoxSweepResults> SweepBoxAgainstThis(Box2f in_boxToSweep, Vec2f in_sweepVec) const override;

	std::shared_ptr<TilesComponent> GetTilesComp() const { return m_tilesComp; }

private:
	std::shared_ptr<TilesComponent> m_tilesComp;
};
