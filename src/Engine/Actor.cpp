#include "Actor.h"

#include "Engine/AssetCache.h"
#include "Engine/Game.h"
#include "Engine/Components/SpriteComponent.h"
#include "Engine/Components/ShapeComponent.h"
#include "Engine/Components/TilesComponent.h"
#include "Engine/Components/TextComponent.h"
#include "Engine/Components/SensorComponent.h"
#include "Engine/PhysicsSystem.h"
#include "Engine/TileMap.h"
#include "Engine/Polygon.h"

//--- Actor ---//
Actor::Actor()
{
	m_rootSceneComp = std::make_shared<SceneComponent>(); // NOTE: Cannot be initialized until Actor::Initialize()
}
std::shared_ptr<SceneComponent> Actor::GetRootComponent() const
{
	return m_rootSceneComp;
}
void Actor::AttachToComponent(std::shared_ptr<SceneComponent> in_sceneComp, bool in_keepWorldTransform)
{
	m_rootSceneComp->SetAttachParent(in_sceneComp, in_keepWorldTransform);
}
void Actor::AttachToActor(std::shared_ptr<Actor> in_actor, bool in_keepWorldTransform)
{
	m_rootSceneComp->SetAttachParent(in_actor->m_rootSceneComp, in_keepWorldTransform);
}
void Actor::Detach(bool in_keepWorldTransform)
{
	m_rootSceneComp->SetAttachParent(nullptr, in_keepWorldTransform);
}
void Actor::SetWorldTransform(const Transform& in_worldTransform)
{
	m_rootSceneComp->SetWorldTransform(in_worldTransform);
}
const Transform& Actor::GetWorldTransform() const
{
	return m_rootSceneComp->GetWorldTransform();
}
void Actor::SetWorldPos(const Vec2f& in_worldPos)
{
	auto worldTransform = m_rootSceneComp->GetWorldTransform();
	worldTransform.pos = in_worldPos;
	m_rootSceneComp->SetWorldTransform(worldTransform);
}
const Vec2f& Actor::GetWorldPos() const
{
	const auto& worldTransform = m_rootSceneComp->GetWorldTransform();
	return worldTransform.pos;
}
void Actor::SetWorldRot(float in_worldRot)
{
	auto worldTransform = m_rootSceneComp->GetWorldTransform();
	worldTransform.rot = in_worldRot;
	m_rootSceneComp->SetWorldTransform(worldTransform);
}
float Actor::GetWorldRot() const
{
	const auto& worldTransform = m_rootSceneComp->GetWorldTransform();
	return worldTransform.rot;
}
void Actor::SetWorldScale(const Vec2f& in_worldScale)
{
	auto worldTransform = m_rootSceneComp->GetWorldTransform();
	worldTransform.scale = in_worldScale;
	m_rootSceneComp->SetWorldTransform(worldTransform);
}
const Vec2f& Actor::GetWorldScale() const
{
	const auto& worldTransform = m_rootSceneComp->GetWorldTransform();
	return worldTransform.scale;
}
void Actor::Initialize()
{
	Object::Initialize();

	// Initialize root scene component (if not already initialized)
	if(!m_rootSceneComp->IsInitialized())
	{
		m_rootSceneComp->SetOwner(AsShared());
		m_rootSceneComp->SetActor(AsShared<Actor>());
		m_rootSceneComp->Initialize();
	}

	GameBase::Get()->RegisterActor(AsShared<Actor>());
	m_taskMgr.RunManaged(ManageActor());
}
void Actor::Destroy()
{
	m_taskMgr.KillAllTasks();
	Object::Destroy();
}
float Actor::DT() const
{
	switch(m_timeStream)
	{
	case eTimeStream::Audio: return Time::AudioDT();
	case eTimeStream::Real: return Time::RealDT();
	default:
	case eTimeStream::Game: return Time::DT();
	}
}
float Actor::Time() const
{
	switch(m_timeStream)
	{
	case eTimeStream::Audio: return Time::AudioTime();
	case eTimeStream::Real: return Time::RealTime();
	default:
	case eTimeStream::Game: return Time::Time();
	}
}
void Actor::UpdateWithId(uint32_t in_updateId)
{
	// Only update if we haven't already updated this frame
	if(m_lastUpdateId != in_updateId)
	{
		m_lastUpdateId = in_updateId;
		Update();
	}
}
void Actor::Update()
{
	// Update tasks
	m_taskMgr.Update();

	// Update draw components
	m_drawComps.erase(std::remove_if(m_drawComps.begin(), m_drawComps.end(), [](const auto& in_comp) {
		return in_comp->IsDestroyed();
	}), m_drawComps.end());
	for(auto drawComp : m_drawComps)
	{
		if(IsAlive(drawComp))
		{
			drawComp->Update();
		}
	}
}
void Actor::Draw()
{
	if (m_bHidden) return;

	// Sort draw components
	auto drawCompList = m_drawComps;
	std::stable_sort(drawCompList.begin(), drawCompList.end(), [](auto lhs, auto rhs) { return lhs->GetDrawOrder() < rhs->GetDrawOrder(); });
	for(auto drawComp : drawCompList)
	{
		if(IsAlive(drawComp) && !drawComp->GetHidden())
		{
			drawComp->Draw();
		}
	}
}
void Actor::AddActorDependency(std::shared_ptr<Actor> in_actor)
{
	m_actorDeps.push_back(in_actor);
}
void Actor::RemoveActorDependency(std::shared_ptr<Actor> in_actor)
{
	m_actorDeps.erase(std::remove_if(m_actorDeps.begin(), m_actorDeps.end(), [&in_actor](const auto& in_other) {
		if(auto other = in_other.lock())
		{
			return in_actor == other;
		
		}
		return true;
	}), m_actorDeps.end());
}
std::shared_ptr<BodyComponent> Actor::MakeBody(bool in_destroyOldRoot)
{
	auto body = SpawnWithInit<BodyComponent>(m_rootSceneComp->GetWorldTransform(), [this, in_destroyOldRoot](auto in_comp) {
		in_comp->SetActor(AsShared<Actor>());
		if(in_destroyOldRoot)
		{
			auto attachChildren = m_rootSceneComp->GetAttachChildren();
			for(auto attachChild : attachChildren)
			{
				attachChild->SetAttachParent(in_comp);
			}
			m_rootSceneComp->Destroy(); // Destroy old root scene component
		}
		else
		{
			m_rootSceneComp->SetAttachParent(in_comp); // Re-parent old scene root component
		}
		m_rootSceneComp = in_comp;
	});
	return body;
}
std::shared_ptr<SpriteComponent> Actor::MakeSprite(const Transform& in_transform)
{
	auto sprite = SpawnWithInit<SpriteComponent>(in_transform, [this](auto in_comp) {
		in_comp->SetActor(AsShared<Actor>());
	});
	sprite->SetAttachParent(m_rootSceneComp, false);
	m_drawComps.push_back(sprite);
	return sprite;
}
std::shared_ptr<ShapeComponent> Actor::MakeShape(const Polygon& in_polygon, const Transform& in_transform)
{
	auto shape = SpawnWithInit<ShapeComponent>(in_transform, [this](auto in_comp) {
		in_comp->SetActor(AsShared<Actor>());
	});
	shape->SetAttachParent(m_rootSceneComp, false);
	shape->SetPolygon(in_polygon);
	m_drawComps.push_back(shape);
	return shape;
}
std::shared_ptr<TilesComponent> Actor::MakeTiles(std::shared_ptr<TileLayer> in_tileLayer, const Transform& in_transform)
{
	auto tiles = SpawnWithInit<TilesComponent>(in_transform, [this](auto in_comp) {
		in_comp->SetActor(AsShared<Actor>());
	});
	tiles->SetAttachParent(m_rootSceneComp, false);
	tiles->SetTileLayer(in_tileLayer);
	m_drawComps.push_back(tiles);
	return tiles;
}
std::shared_ptr<TextComponent> Actor::MakeText(const Transform& in_transform)
{
	auto text = SpawnWithInit<TextComponent>(in_transform, [this](auto in_comp) {
		in_comp->SetActor(AsShared<Actor>());
	});
	text->SetAttachParent(m_rootSceneComp, false);
	m_drawComps.push_back(text);
	return text;
}

