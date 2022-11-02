#pragma once

#include "Engine/MathGeometry.h"

#include <set>

/*
CollisionSystem: 

A simple wrapper around MathGeometry sweep/overlap functions that lets you sweep and query overlaps against multiple things at once.


TODO:
- broadphase acceleration structure
- shape queries (query colliders that overlap a shape)
- support some sort of collision mask system
- circle support
*/

class ColliderComponent;

///////////////////////////////////////////////////////////////////////////////////////////
// CollisionWorld: 

class CollisionWorld : public std::enable_shared_from_this<CollisionWorld>
{
public:	
	// queries
	std::optional<Math::BoxSweepResults> SweepBox(Box2f in_boxToSweep, Vec2f in_sweepVec) const;

	// in the future, colliders will need to call this whenever their bounding boxes change, in order to 
	void OnColliderBoundingBoxChanged(ColliderComponent* in_coll);

private:
	friend class ColliderComponent;

	std::set<std::weak_ptr<ColliderComponent>, std::owner_less<std::weak_ptr<ColliderComponent>>> m_colliders;

	//void ClearInvalidColliders() mutable;
	void AddCollider(ColliderComponent* in_coll);
	void RemoveCollider(ColliderComponent* in_coll);
};

