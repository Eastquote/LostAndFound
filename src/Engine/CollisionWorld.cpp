#include "CollisionWorld.h"
#include "Engine/Components/ColliderComponent.h"



std::optional<Math::BoxSweepResults> CollisionWorld::SweepBox(Box2f in_boxToSweep, Vec2f in_sweepVec) const
{
	//ClearInvalidColliders();

	std::optional<Math::BoxSweepResults> firstHit{};

	for (std::weak_ptr<ColliderComponent> weakColl : m_colliders)
	{
		if (auto coll = weakColl.lock())
		{
			std::optional<Math::BoxSweepResults> res = coll->SweepBoxAgainstThis(in_boxToSweep, in_sweepVec);

			if (res && (!firstHit || res->m_dist < firstHit->m_dist))
			{
				firstHit = res;
			}
		}
	}

	return firstHit;
}

//void CollisionWorld::ClearInvalidColliders() mutable
//{
//	for (auto iter = m_colliders.begin(); iter != m_colliders.end(); ) 
//	{
//		if (iter) 
//		{
//			iter = m_colliders.erase(iter);
//		} 
//		else 
//		{
//			iter++;
//		}
//	}
//}

void CollisionWorld::AddCollider(ColliderComponent* in_coll)
{
	m_colliders.insert(in_coll->AsShared<ColliderComponent>());
}

void CollisionWorld::RemoveCollider(ColliderComponent* in_coll)
{
	m_colliders.erase(in_coll->AsShared<ColliderComponent>());
}

void CollisionWorld::OnColliderBoundingBoxChanged(ColliderComponent* in_coll)
{
	RemoveCollider(in_coll);
	AddCollider(in_coll);
}
