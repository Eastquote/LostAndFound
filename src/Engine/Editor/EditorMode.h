#pragma once

#include "Engine/Actor.h"

#include "Task.h"
#include "Engine/InputSystem.h"

/* 
EditorMode: an actor that manages and draws various in-engine editor tools:
- curve asset editor


EditorModeToggle: a simple actor that binds a key (default ~) to enable/disable an EditorMode instance and pause the game
*/

class EditorModeToggle : public Actor
{
public:
	virtual bool ShouldUpdateWhilePaused() const override { return true; }

	void SetEditorToggleButtons(const std::vector<eButton>& in_inputs) {
		m_editorToggleButton = m_inputComp->Button<ButtonStateWithHistory>("Toggle Editor", in_inputs, false);
		m_editorToggleButton->SetRealTime(true);
	}

	void SetShouldPauseWhileEditorOpen(bool in_bPause) { m_bShouldPauseWhileEditorOpen = in_bPause; }

private:
	virtual Task<> ManageActor() override;

	bool m_bShouldPauseWhileEditorOpen = true;
	std::shared_ptr<InputComponent> m_inputComp;

	std::shared_ptr<ButtonStateWithHistory> m_editorToggleButton;
};

class EditorMode : public Actor
{
public:
	virtual bool ShouldUpdateWhilePaused() const override { return true; }

	bool m_bEnabled = false;

private:
	virtual Task<> ManageActor() override;
};

