#pragma once

#include "Engine/Actor.h"
#include "DamageInfo.h"
#include "Engine/Components/SensorComponent.h"

struct SensorShape;
class GameWorld;
class CollisionWorld;
class ColliderComponent_Box;
class ColliderComponent_TilesComponent;
class TilesComponent;
struct TextDef;

// Base Actor subclass for this game -- adds helpers for collider/sensor setup, targeting, getting distance, and destroying tiles
class GameActor : public Actor {
public:
	virtual void Initialize() override;
	virtual void Destroy() override;
	virtual void DeferredDestroy();

	// Component helper functions
	std::shared_ptr<SensorComponent> MakeSensor(const Transform& in_transform, const SensorShape& in_shape);
	std::shared_ptr<ColliderComponent_Box> MakeCollider_Box(std::shared_ptr<Object> in_owner, const Transform& in_transform, 
															std::shared_ptr<CollisionWorld> in_world, Box2f in_boxLocal);
	std::shared_ptr<ColliderComponent_TilesComponent> MakeCollider_TilesComp(	const Transform& in_transform, 
																				std::shared_ptr<CollisionWorld> in_world, 
																				std::shared_ptr<TilesComponent> in_tilesComp);
	
	std::shared_ptr<TilesComponent> GetCollisionTilesComp() const;
	std::shared_ptr<GameWorld> GetWorld() const;
	Vec2f DistanceFromPlayer() const;
	Vec2f DistanceToPlayer() const;
	DamageInfo GetDamageInfo() const;
	void SetTargeted(bool in_setting) { m_bIsTargeted = in_setting; }
	bool GetTargeted() const { return m_bIsTargeted; }

	// Destroys tiles within a shape (used for destroyable environment features)
	bool TryDestroyTiles(const Vec2f& in_pos, const Vec2f& in_dir, bool in_bPenetratesWalls, int32_t in_aoe = 0);

protected:
	void SetDamageInfo(int32_t in_payload, uint32_t in_dmgFlag = 0);
	std::shared_ptr<TextComponent> MakeAndConfigText(TextDef& in_def, std::wstring in_text = L"", Vec2f in_posOffset = Vec2f::Zero,
		bool in_hidden = false, int32_t in_drawOrder = 10, std::string in_renderLayer = "hud");

	bool m_bUpdatesOffScreen = true;

private:
	std::shared_ptr<GameWorld> m_world;
	DamageInfo m_damageInfo;
	bool m_bIsTargeted = false;
};
