#pragma once

#include "Engine/Transform.h"
#include "Engine/Box.h"
#include "Engine/Object.h"
#include "Engine/Delegate.h"
#include "Engine/Components/SceneComponent.h"
#include "Task.h"
#include "TaskManager.h"

// Forward declarations
class b2World;
class b2Body;

class FixtureComponent : public Object
{
public:
	void Initialize();
	void Destroy();
};

class JointComponent : public Object
{
public:
	void Initialize();
	void Destroy();
};

enum class eBodyType
{
	Static,
	Dynamic,
	Kinematic,
};

class BodyComponent : public SceneComponent
{
public:
	BodyComponent(eBodyType in_bodyType = eBodyType::Static);

	void Initialize();
	void Destroy();

	// TODO: setters for position/velocity, as drivers for kinematic bodies, and as a 'move request' otherwise

private:
	friend class PhysicsSystem;
	void Update();
	void UpdateScale();

	b2Body* m_body = nullptr;
	eBodyType m_bodyType = eBodyType::Static;
	Vec2f m_cachedScale = Vec2f::One;
};

struct PhysicsStepParams
{
	float maxSubStepTime = 1.0f / 120.0f;
	int32_t maxSubSteps = 6;
	int32_t velIters = 6;
	int32_t posIters = 2;
};

class PhysicsSystem
{
public:
	static PhysicsSystem* Get();

	PhysicsSystem();
	~PhysicsSystem();

	void Update();

	// Setters
	void SetGravity(const Vec2f& in_gravity);
	const Vec2f& GetGravity() const;

	// Simulation
	using tOnStepFn = std::function<void(int32_t, int32_t)>;
	tListenerPtr BindOnStepFn(tOnStepFn in_onStepFn);
	void SetStepParams(const PhysicsStepParams& in_stepParams);

	// std::vector<QueryResult> BoxQuery(pos, dims)
	// std::vector<QueryResult> CircleQuery(pos, radius)
	// std::vector<TraceResult> Trace(start, end, maxResults)

protected:
	friend class ShapeComponent;
	friend class JointComponent;
	friend class BodyComponent;
	b2World* GetWorld() const;
	void AddBody(std::shared_ptr<BodyComponent> in_bodyComp);

private:
	TaskManager m_taskMgr;
	Task<> SimulateTask();
	std::vector<std::shared_ptr<BodyComponent>> m_bodies;
	Delegate<tOnStepFn> m_onStepDelegate;
	PhysicsStepParams m_stepParams;
	b2World* m_world = nullptr;
	Vec2f m_gravity = Vec2f::Zero;
};
