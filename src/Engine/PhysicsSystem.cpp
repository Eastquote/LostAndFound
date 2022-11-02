#include "PhysicsSystem.h"

#include "Game.h"

#include "box2d/box2d.h"

//--- BodyComponent ---//
BodyComponent::BodyComponent(eBodyType in_bodyType)
: m_bodyType(in_bodyType)
{
}
void BodyComponent::Initialize()
{
	Object::Initialize();

	// Create body
	auto physWorld = PhysicsSystem::Get()->GetWorld();
	b2BodyDef bodyDef;
	const auto& worldTransform = GetWorldTransform();
	bodyDef.position.Set(worldTransform.pos.x, worldTransform.pos.y);
	bodyDef.angle = worldTransform.rot;
	m_cachedScale = worldTransform.scale;
	bodyDef.type = m_bodyType == eBodyType::Dynamic ? b2_dynamicBody
				 : m_bodyType == eBodyType::Kinematic ? b2_kinematicBody
				 : b2_staticBody;
	m_body = physWorld->CreateBody(&bodyDef);

	b2PolygonShape dynamicBox;
	dynamicBox.SetAsBox(1.0f, 1.0f);
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &dynamicBox;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.3f;
	m_body->CreateFixture(&fixtureDef);

	PhysicsSystem::Get()->AddBody(AsShared<BodyComponent>());
}
void BodyComponent::Destroy()
{
	// Destroy body
	auto physWorld = PhysicsSystem::Get()->GetWorld();
	physWorld->DestroyBody(m_body);
	m_body = nullptr;

	Object::Destroy();
}
void BodyComponent::Update()
{
	const auto& pos = m_body->GetPosition();
	float rot = (float)(m_body->GetAngle() / M_PI * 180.0f);
	SetRelativeTransform({ Vec2f{ pos.x, pos.y }, rot, GetRelativeScale() });
	auto worldScale = GetWorldScale();
	if(worldScale != m_cachedScale)
	{
		m_cachedScale = worldScale;
		UpdateScale();
	}
}
void BodyComponent::UpdateScale()
{
	// Resize all fixtures
	//...
}

//--- PhysicsSystem ---//
PhysicsSystem* PhysicsSystem::Get()
{
	static PhysicsSystem s_physSys;
	return &s_physSys;
}
PhysicsSystem::PhysicsSystem()
{
	// Create world
	m_world = new b2World({ 0.0f, 10.0f });
	m_world->SetAutoClearForces(false);
	m_taskMgr.RunManaged(SimulateTask());
}
PhysicsSystem::~PhysicsSystem()
{
	// Delete world
	delete m_world;
	m_world = nullptr;
}
void PhysicsSystem::Update()
{
	m_taskMgr.Update();
}
b2World* PhysicsSystem::GetWorld() const
{
	return m_world;
}
void PhysicsSystem::AddBody(std::shared_ptr<BodyComponent> in_bodyComp)
{
	m_bodies.push_back(in_bodyComp);
}
tListenerPtr PhysicsSystem::BindOnStepFn(tOnStepFn in_onStepFn)
{
	return m_onStepDelegate.Add(in_onStepFn);
}
void PhysicsSystem::SetStepParams(const PhysicsStepParams& in_stepParams)
{
	m_stepParams = in_stepParams;
}
Task<> PhysicsSystem::SimulateTask()
{
	TASK_NAME(__FUNCTION__);

	while(true)
	{
		// Simulate movement
		auto dt = Time::DT(); // Get game-time DT
		int32_t numSteps = (int32_t)std::round(dt / m_stepParams.maxSubStepTime);
		numSteps = std::min(numSteps, m_stepParams.maxSubSteps);
		float stepDur = (float)(dt / numSteps);
		for(auto i = 0; i < numSteps; ++i)
		{
			m_onStepDelegate.Broadcast(i, numSteps);
			m_world->Step(stepDur, m_stepParams.velIters, m_stepParams.posIters);
		}
		m_world->ClearForces();

		// Collision callbacks
		// ...

		// Update all bodies
		for(auto body : m_bodies)
		{
			if(IsAlive(body))
			{
				body->Update();
			}
		}

		// Suspend until next frame
		co_await Suspend();
	}
}
