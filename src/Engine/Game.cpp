#include "Game.h"

#include "Task.h"
#include "Actor.h"

#include "GameWindow.h"
#include "Engine/PhysicsSystem.h"
#include "Engine/DebugDrawSystem.h"

#include <SFML/System.hpp>

//--- Time Linkage ---//
extern float g_gameDt;
extern float g_audioDt;
extern float g_realDt;
extern float g_gameTime;
extern float g_audioTime;
extern float g_realTime;

//--- GameBase ---//
static GameBase* s_game = {};
GameBase* GameBase::Get()
{
	return s_game;
}
GameBase::GameBase(const Vec2i& in_windowDims, const Vec2i& in_renderDims, const std::wstring& in_title)
{
	s_game = this; // Set Game singleton
	m_window = std::make_shared<GameWindow>(in_windowDims, in_renderDims, in_title);
	m_debugDrawSystem = std::make_shared<DebugDrawSystem>();
}
GameBase::~GameBase()
{
	s_game = {};
}
void GameBase::SetFrameRateCap(int32_t in_frameRateCap)
{
	m_frameRateCap = in_frameRateCap;	
}
void GameBase::SetTimeDilation(float in_timeDilation)
{
	m_timeDilation = in_timeDilation;
}
void GameBase::Run()
{
	sf::Clock clock;

	// Time getter
	auto GetElapsedTime = [&clock] { return (float)clock.getElapsedTime().asSeconds(); };

	SetTimeDilation(1.0f);

	// Main game loop
	g_realTime = GetElapsedTime();
	while(m_window->IsOpen())
	{
		// Update time
		auto newRealTime = GetElapsedTime();
		g_realDt = newRealTime - g_realTime;
		g_realTime = newRealTime;
		g_audioDt = !ShouldPause ? g_realDt : 0.0f; // Audio + game delta-time is 0 when paused
		g_audioTime += g_audioDt;
		float gameDt = std::min(g_audioDt, 2.0f / 60.0f); //< Clamp DT() to last no more than 2 frames (at 60hz)
		g_gameDt = gameDt * m_timeDilation;
		g_gameTime += g_gameDt;
		
		// Handle window events
		m_window->Update();

		// Update the game
		Update();

		// Draw the frame
		Draw();

		// Cap FPS
		if(m_frameRateCap > 0)
		{
			float frameDur = 1.0f / m_frameRateCap;
			while(GetElapsedTime() - g_realTime < frameDur)
			{
				// Spin-lock
			}
		}

		// Low FPS simulation
		//std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 20));
	}
}
void GameBase::Update()
{
	// Actor update function
	++m_updateId; // Increment update ID
	auto UpdateActor = [this](const auto& actor, eUpdateStage stage, bool isPaused)
	{
		// Early-out if actor has already updated or wants to update in a later stage
		if(actor->m_lastUpdateId == m_updateId || actor->GetUpdateStage() > stage)
		{
			return;
		}

		// Update actor if game isn't paused (or it can update while paused)
		if(!isPaused || actor->ShouldUpdateWhilePaused())
		{
			// Update dependencies
			bool allDepsUpdated = true;
			auto& deps = actor->GetActorDependencies();
			if(deps.size())
			{
				for(auto& depWeak : deps)
				{
					if(auto dep = depWeak.lock())
					{
						SQUID_RUNTIME_CHECK(!isPaused || dep->ShouldUpdateWhilePaused(), "Actor updates while paused, but dependency does not");
						SQUID_RUNTIME_CHECK(dep->GetUpdateStage() <= actor->GetUpdateStage(), "Dependency's update stage is later than actor's update stage");
						dep->UpdateWithId(m_updateId);
					}
				}
				deps.erase(std::remove_if(deps.begin(), deps.end(), [](const auto& in_actor) {
					return !IsAlive(in_actor);
				}), deps.end());
			}

			// Update actor
			actor->UpdateWithId(m_updateId);
		}
	};

	// Update actors in stages
	auto isPaused = ShouldPause;
	// TODO: unroll this into each phase so physics can be called between PrePhysics and PostPhysics
	//for(int32_t stage = 0; stage <= (int32_t)eUpdateStage::Final; ++stage)
	//{
	//	size_t writeIdx = 0;
	//	for(size_t readIdx = 0; readIdx < m_actors.size(); ++readIdx)
	//	{
	//		auto actor = m_actors[readIdx];
	//		if(IsAlive(actor))
	//		{
	//			// Update actor if this is its stage, or if it is from an earlier stage
	//			UpdateActor(actor, (eUpdateStage)stage, isPaused);
	//			if(writeIdx != readIdx)
	//			{
	//				m_actors[writeIdx] = std::move(m_actors[readIdx]);
	//			}
	//			++writeIdx;
	//		}
	//	}
	//	m_actors.resize(writeIdx); // Erase dead actors
	//}

	auto updateStage = [this, UpdateActor, isPaused](eUpdateStage in_stage) {
		size_t writeIdx = 0;
		for(size_t readIdx = 0; readIdx < m_actors.size(); ++readIdx)
		{
			auto actor = m_actors[readIdx];
			if(IsAlive(actor))
			{
				// Update actor if this is its stage, or if it is from an earlier stage
				UpdateActor(actor, (eUpdateStage)in_stage, isPaused);
				if(writeIdx != readIdx)
				{
					m_actors[writeIdx] = std::move(m_actors[readIdx]);
				}
				++writeIdx;
			}
		}
		m_actors.resize(writeIdx); // Erase dead actors
	};

	updateStage(eUpdateStage::Initial);
	updateStage(eUpdateStage::PrePhysics);
	// Update physics
	if(m_physicsCallback)
	{
		m_physicsCallback();
	}
	else {
		PhysicsSystem::Get()->Update();
	}
	updateStage(eUpdateStage::PostPhysics);
	updateStage(eUpdateStage::Final);
	//m_debugDrawCallback();
}
void GameBase::Draw()
{
	// Clear frame buffer
	m_window->Clear();

	// Draw all actors
	auto actorDrawList = m_actors;
	std::stable_sort(actorDrawList.begin(), actorDrawList.end(), [](auto lhs, auto rhs) { return lhs->GetDrawLayer() < rhs->GetDrawLayer(); });
	for(auto actor : actorDrawList)
	{
		if(IsAlive(actor))
		{
			actor->Draw();
		}
	}

	m_window->ApplyPostProcessing();

	// Draw debug lines/shapes
	m_debugDrawSystem->UpdateAndDraw();

	// Present the frame for display
	m_window->Swap();
}
std::shared_ptr<GameWindow> GameBase::GetWindow() const
{
	return m_window;
}
InputSystem* GameBase::GetInputSystem() const
{
	return GetWindow()->GetInputSystem();
}
void GameBase::RegisterActor(std::shared_ptr<Actor> in_actor)
{
	m_actors.push_back(in_actor);
}