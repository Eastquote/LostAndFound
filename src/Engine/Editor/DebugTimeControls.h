#pragma once

#include "Engine/Actor.h"

#include "TaskFSM.h"
#include "Engine/InputSystem.h"

/* 
enables some time controls that are useful for debugging:
 - to use, just spawn one of these actors
 - press the pause button to pause the game
   - when paused, press the pause button again to step forward a single
   - hold the pause button to unpause
 - press the slomo button to cycle through increasing slomo amounts
   - hold the slomo button, or cycle all the way through, to return to full speed
 - use SetPauseButtons() and SetSlomoButtons() to change the default keybinds 
*/

// debug pause/framestep/slomo controls
class DebugTimeControls : public Actor
{
public:
	virtual bool ShouldUpdateWhilePaused() const override { return true; }

	void SetPauseButtons(const std::vector<eButton>& in_inputs) {
		m_debugPauseButton = m_inputComp->Button<ButtonStateWithHistory>("DebugPause", in_inputs, false);
		m_debugPauseButton->SetRealTime(true);
	}
	void SetSlomoButtons(const std::vector<eButton>& in_inputs) {
		m_debugSlomoButton = m_inputComp->Button<ButtonStateWithHistory>("DebugSlomo", in_inputs, false);
		m_debugSlomoButton->SetRealTime(true);
	}

private:
	virtual Task<> ManageActor() override;

	Task<> ManageDebugPause();
	//Task<TaskStatePtr> DebugTimeMode_Paused();
	//Task<TaskStatePtr> DebugTimeMode_FrameStep();
	//Task<TaskStatePtr> DebugTimeMode_Slomo();
	//Task<TaskStatePtr> DebugTimeMode_Unpaused();
	void SetDebugPause(bool bDebugPaused);

	std::shared_ptr<Token> m_debugPauseToken;
	std::shared_ptr<InputComponent> m_inputComp;
	std::shared_ptr<ButtonStateWithHistory> m_debugPauseButton;
	std::shared_ptr<ButtonStateWithHistory> m_debugSlomoButton;
};

