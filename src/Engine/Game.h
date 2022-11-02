#pragma once

#include "Object.h"
#include "Engine/Guard.h"
#include "Engine/Vec2.h"
#include "TokenList.h"

#include <memory>
#include <vector>
#include <string>

class Actor;
class GameWindow;
class InputSystem;
class DebugDrawSystem;

// Game Base
class GameBase
{
public:
	GameBase(const Vec2i& in_windowDims, const Vec2i& in_renderDims, const std::wstring& in_title);
	~GameBase();

	virtual void Run();
	void Update();
	void Draw();

	static GameBase* Get();
	std::shared_ptr<GameWindow> GetWindow() const;
	InputSystem* GetInputSystem() const;

	// Time + pause management
	void SetFrameRateCap(int32_t in_frameRateCap);
	void SetTimeDilation(float in_timeDilation);
	float GetTimeDilation() const { return m_timeDilation; }
	TokenList<> ShouldPause;

	void SetPhysicsCallback(std::function<void()> in_func) { m_physicsCallback = in_func; } // settable (or "bindable") delegate pattern
	//void SetDebugDrawCallback(std::function<void()> in_func) { m_debugDrawCallback = in_func; } // settable (or "bindable") delegate pattern

protected:
	friend class Actor;
	void RegisterActor(std::shared_ptr<Actor> in_actor);

private:
	std::function<void()> m_physicsCallback;
	//std::function<void()> m_debugDrawCallback;
	std::shared_ptr<GameWindow> m_window;
	std::shared_ptr<DebugDrawSystem> m_debugDrawSystem;
	std::vector<std::shared_ptr<Actor>> m_actors;
	int32_t m_frameRateCap = 60;
	float m_timeDilation = 1.0;
	uint32_t m_updateId = 0;
};

// Game
template <typename tGameLoop>
class Game : public GameBase
{
public:
	Game()
		: GameBase(tGameLoop::GetWindowDims(), tGameLoop::GetRenderDims(), tGameLoop::GetWindowTitle())
	{
	}

	virtual void Run() override
	{
		// Create game loop
		auto root = Guard(Object::MakeRoot());
		auto gameLoop = Object::Make<tGameLoop>(root.Get());

		// Run game as normal
		GameBase::Run();
	}
};
