#include "Engine/Editor/DebugTimeControls.h"

#include "Engine/Game.h"
#include "Engine/InputSystem.h"

Task<> DebugTimeControls::ManageActor()
{
	SetTimeStream(eTimeStream::Real);

	m_inputComp = MakeChild<InputComponent>();
	SetPauseButtons({ eButton::RBracket });
	SetSlomoButtons({ eButton::LBracket });

	co_await ManageDebugPause();
}

Task<> DebugTimeControls::ManageDebugPause()
{
	//co_await RunYuzuTaskFSMSimple(DebugTimeMode_Unpaused());
	co_return;
}

void DebugTimeControls::SetDebugPause(bool bDebugPaused)
{
	if (bDebugPaused)
	{
		if (!m_debugPauseToken)
		{
			m_debugPauseToken = GameBase::Get()->ShouldPause.TakeToken(__FUNCTION__);
		}
		GameBase::Get()->ShouldPause.AddToken(m_debugPauseToken);
	}
	else
	{
		GameBase::Get()->ShouldPause.RemoveToken(m_debugPauseToken);
	}
}

//Task<TaskStatePtr> DebugTimeControls::DebugTimeMode_Paused()
//{
//	const bool bPrevPaused = (bool)GameBase::Get()->ShouldPause;
//	SetDebugPause(true);
//	auto SG = MakeFnGuard( [&]() { SetDebugPause(bPrevPaused); } );
//
//	co_await Suspend();
//	co_await WaitUntil([this]() { return m_debugPauseButton->IsPressed() || m_debugSlomoButton->JustPressed(); });
//
//	if (m_debugSlomoButton->JustPressed())
//	{
//		co_return DebugTimeMode_Slomo();
//	}
//
//	const bool bReleasedQuickly = co_await WaitUntil([this]() { return !m_debugPauseButton->IsPressed(); })
//		.CancelIf([StartTime=Time::RealTime()](){ return Time::RealTime() - StartTime > 0.5f; });
//
//	if (bReleasedQuickly)
//	{
//		co_return DebugTimeMode_FrameStep();
//	}
//	else
//	{
//		co_return DebugTimeMode_Unpaused();
//	}
//}
//
//Task<TaskStatePtr> DebugTimeControls::DebugTimeMode_FrameStep()
//{
//	SetDebugPause(false);
//	co_await Suspend();
//	SetDebugPause(true);
//
//	co_return DebugTimeMode_Paused();
//}
//
//// each subsequent press of dpad left advances to a slower speed, then cycles back to normal when it reaches the end of the list
//Task<TaskStatePtr> DebugTimeControls::DebugTimeMode_Slomo()
//{
//	const float PrevDilation = GameBase::Get()->GetTimeDilation();
//	auto SG = MakeFnGuard([&](){ GameBase::Get()->SetTimeDilation(PrevDilation); });
//
//	int32_t dilationIdx = 0;
//	const std::vector<float> dilations = {
//		0.5f,
//		0.2f,
//		0.05f,
//	};
//
//	while (true)
//	{
//		if (dilationIdx >= dilations.size()) break;
//
//		GameBase::Get()->SetTimeDilation(dilations[dilationIdx]);
//
//		co_await Suspend();
//		co_await WaitUntil([this]() { return m_debugPauseButton->JustPressed() || m_debugSlomoButton->JustPressed(); });
//
//		if (m_debugPauseButton->JustPressed())
//		{
//			co_return DebugTimeMode_Paused();
//		}
//
//		const bool bReleasedQuickly = co_await WaitUntil([this]() { return !m_debugSlomoButton->IsPressed(); })
//			.CancelIf([StartTime=Time::RealTime()](){ return Time::RealTime() - StartTime > 0.5f; });
//
//		if (!bReleasedQuickly)
//		{
//			co_return DebugTimeMode_Unpaused();
//		}
//
//		dilationIdx++;
//	}
//
//	co_return DebugTimeMode_Unpaused();
//}
//
//Task<TaskStatePtr> DebugTimeControls::DebugTimeMode_Unpaused()
//{
//	SetDebugPause(false);
//
//	co_await Suspend();
//	co_await WaitUntil([this](){ return m_debugPauseButton->JustPressed() || m_debugSlomoButton->JustPressed(); });
//
//	if (m_debugPauseButton->JustPressed())
//	{
//		co_return DebugTimeMode_Paused();
//	}
//	else
//	{
//		co_return DebugTimeMode_Slomo();
//	}
//}
