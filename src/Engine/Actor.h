#pragma once

#include "TaskManager.h"

#include "Engine/Object.h"
#include "Engine/Texture.h"
#include "Engine/Time.h"
#include "Engine/Transform.h"
#include "Engine/Vec2.h"

class SceneComponent;
class BodyComponent;
class DrawComponent;
class SpriteComponent;
class ShapeComponent;
class TilesComponent;
class TextComponent;
class TileLayer;
class Polygon;

enum class eUpdateStage
{
	Initial, // Start of frame
	PrePhysics, // Before simulating physics
	PostPhysics, // After simulating physics
	Final, // End of frame (just before drawing)
};

class Actor : public Object
{
public:
	Actor();

	// Actor + Object events
	virtual void Initialize() override;
	virtual void Destroy() override;
	virtual void Update();
	virtual void Draw();
	virtual bool ShouldUpdateWhilePaused() const { return false; }
	
	// Time
	void SetTimeStream(eTimeStream in_timeStream) { m_timeStream = in_timeStream; }
	eTimeStream GetTimeStream() const { return m_timeStream; }
	float DT() const;
	float Time() const;

	// Attachment
	std::shared_ptr<SceneComponent> GetRootComponent() const;
	void AttachToComponent(std::shared_ptr<SceneComponent> in_sceneComp, bool in_keepWorldTransform = true);
	void AttachToActor(std::shared_ptr<Actor> in_actor, bool in_keepWorldTransform = true);
	void Detach(bool in_keepWorldTransform = true);

	// Relative Transform
	void SetRelativeTransform(const Transform& in_relativeTransform);
	const Transform& GetRelativeTransform() const;
	void SetRelativePos(const Vec2f& in_relativePos);
	const Vec2f& GetRelativePos() const;
	void SetRelativeRot(float in_relativeRot);
	const float GetRelativeRot() const;
	void SetRelativeScale(const Vec2f& in_relativeScale);
	const Vec2f& GetRelativeScale() const;

	// World Transform
	void SetWorldTransform(const Transform& in_worldTransform);
	const Transform& GetWorldTransform() const;
	void SetWorldPos(const Vec2f& in_worldPos);
	const Vec2f& GetWorldPos() const;
	void SetWorldRot(float in_worldRot);
	float GetWorldRot() const;
	void SetWorldScale(const Vec2f& in_worldScale);
	const Vec2f& GetWorldScale() const;

	// Update staging/ordering
	void SetUpdateStage(eUpdateStage in_updateStage) { m_updateStage = in_updateStage; }
	eUpdateStage GetUpdateStage() const { return m_updateStage; }
	void AddActorDependency(std::shared_ptr<Actor> in_actor);
	void RemoveActorDependency(std::shared_ptr<Actor> in_actor);

	// Draw ordering
	template <typename tDrawLayer>
	void SetDrawLayer(tDrawLayer in_drawLayer) { m_drawLayer = (int32_t)in_drawLayer; }
	template <typename tDrawLayer = int32_t>
	tDrawLayer GetDrawLayer() const { return (tDrawLayer)m_drawLayer; }

	// Hiding
	void SetHidden(bool in_bHidden) { m_bHidden = in_bHidden; }
	bool GetHidden() const { return m_bHidden; }

	// Spawn functions
	template <typename tActor, typename... Args>
	static std::shared_ptr<tActor> Spawn(std::shared_ptr<Object> in_owner, const Transform& in_worldTransform, Args... args)
	{
		auto actor = Object::MakeWithInit<tActor>(in_owner, [&in_worldTransform](auto actor) {
			actor->SetWorldTransform(in_worldTransform);
		}, std::forward<Args>(args)...);
		return actor;
	}
	template <typename tActor, typename... Args>
	std::shared_ptr<tActor> Spawn(const Transform& in_worldTransform, Args... args)
	{
		return Actor::Spawn<tActor>(AsShared(), in_worldTransform, std::forward<Args>(args)...);
	}
	template <typename tActor, typename... Args>
	static std::shared_ptr<tActor> SpawnWithInit(std::shared_ptr<Object> in_owner, const Transform& in_worldTransform, std::function<void(std::shared_ptr<tActor>)> in_initFunc, Args... args)
	{
		auto actor = Object::MakeWithInit<tActor>(in_owner, [&in_worldTransform, &in_initFunc](auto actor) {
			actor->SetWorldTransform(in_worldTransform);
			in_initFunc(actor);
		}, std::forward<Args>(args)...);
		return actor;
	}
	template <typename tActor, typename... Args>
	std::shared_ptr<tActor> SpawnWithInit(const Transform& in_worldTransform, std::function<void(std::shared_ptr<tActor>)> in_initFunc, Args... args)
	{
		return Actor::SpawnWithInit<tActor>(AsShared(), in_worldTransform, std::move(in_initFunc), std::forward<Args>(args)...);
	}

	// Component factories
	std::shared_ptr<BodyComponent> MakeBody(bool in_destroyOldRoot = false);
	std::shared_ptr<SpriteComponent> MakeSprite(const Transform& in_transform = Transform::Identity);
	std::shared_ptr<ShapeComponent> MakeShape(const Polygon& in_polygon, const Transform& in_transform = Transform::Identity);
	std::shared_ptr<TilesComponent> MakeTiles(std::shared_ptr<TileLayer> in_tileLayer, const Transform& in_transform = Transform::Identity);
	std::shared_ptr<TextComponent> MakeText(const Transform& in_transform = Transform::Identity);

protected:
	TaskManager m_taskMgr;
	virtual Task<> ManageActor() { co_return; }

private:
	friend class GameBase;
	std::vector<std::weak_ptr<Actor>>& GetActorDependencies() { return m_actorDeps; }
	void UpdateWithId(uint32_t in_updateId);
	void SetRootSceneComponent(std::shared_ptr<SceneComponent> in_rootSceneComp);

	// Time stream
	eTimeStream m_timeStream = eTimeStream::Game;

	// Update settings
	eUpdateStage m_updateStage = eUpdateStage::PrePhysics;
	std::vector<std::weak_ptr<Actor>> m_actorDeps;
	uint32_t m_lastUpdateId = 0;

	// Draw settings
	std::shared_ptr<SceneComponent> m_rootSceneComp;
	bool m_bHidden = false;
	int32_t m_drawLayer = 0;
	std::vector<std::shared_ptr<DrawComponent>> m_drawComps;
};
