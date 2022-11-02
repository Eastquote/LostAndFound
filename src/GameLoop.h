#pragma once

#include "Engine/Actor.h"
#include "Engine/MathCore.h"
#include "SensorManager.h"

struct ButtonState;

// "Outer loop" of the whole game -- defines and governs transitions between Main Menu and Gameplay game states
class GameLoop : public Actor
{
public:
	static Vec2i GetRenderDims() { return { 336, 240 }; }
	static Vec2i GetWindowDims() { return Math::Ceil(Vec2f(GetRenderDims() * 2) * Vec2f{ GetPixelAspectRatio(), 1.0f }); }
	static std::wstring GetWindowTitle() { return L"Lost & Found"; }
	static bool ShouldDisplayDebug() { return s_bDisplayDebug; }

private:
	Task<> ReturnToFullSpeed(float in_totalDuration);
	virtual Task<> ManageActor() override;
	Task<bool> GameState_Menu();
	Task<> DestroyPlayerSuit(bool isFish = false);
	Task<> GameState_Gameplay(bool in_newGame);
	static float GetPixelAspectRatio() { return 8.0f/7.0f; }

	// Defines and executes intro cinematic in full
	Task<> IntroCinematicTask();

	// Listen for cinematic-triggering conditions to be met, then define/execute relevant cinematics in full
	Task<> CinematicTask(bool in_bCinematicsEnabled = true);

	static bool s_bDisplayDebug;
	bool m_bGameOver = false;
	bool m_bFirstBoot = true;
};
