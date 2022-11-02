#include "EditorMode.h"

#include <iostream>

#include "Engine/AssetCache.h"
#include "Engine/Game.h"
#include "Engine/InputSystem.h"
#include "Engine/Editor/ImguiCurveWidget.h"
#include "Engine/Shader.h"

Task<> EditorModeToggle::ManageActor()
{
	SetTimeStream(eTimeStream::Real);

	m_inputComp = MakeChild<InputComponent>();
	SetEditorToggleButtons({ eButton::Tilde });

	// spawn editor mode actor
	ObjectGuard<EditorMode> editorMode = Guard(Spawn<EditorMode>({}));

	while (true)
	{
		// disable editor
		// keep the same one spawned so that it can keep state
		editorMode->m_bEnabled = false;

		// wait for press
		co_await WaitUntil([this](){
			return m_editorToggleButton->JustPressed();
		});
		m_editorToggleButton->Consume();

		{
			// pause the game, if desired
			std::shared_ptr<Token> debugPauseToken;
			if (m_bShouldPauseWhileEditorOpen)
			{
				debugPauseToken = GameBase::Get()->ShouldPause.TakeToken(__FUNCTION__);
			}

			// TODO consume inputs to stop avoid debug time controls using them?

			// enable editor
			editorMode->m_bEnabled = true;

			// wait for press or editor close
			co_await WaitUntil([this, &editorMode](){
				return m_editorToggleButton->JustPressed() || !editorMode->m_bEnabled;
			});
			m_editorToggleButton->Consume();
		}
	}
}


Task<> EditorMode::ManageActor()
{
	SetTimeStream(eTimeStream::Real);

	Curve curve;
	std::string curvePath;

	bool bCurveEditorActive = false;

	while (true)
	{
		if (m_bEnabled)
		{
			if(ImGui::BeginMainMenuBar())
			{
				if (ImGui::Button("X"))
				{
					m_bEnabled = false;
				}

				if (ImGui::Button("Curve Editor"))
				{
					bCurveEditorActive = !bCurveEditorActive;
				}

				if (ImGui::Button("Reload Shaders"))
				{
					std::cout << "------------------------------\n";
					std::cout << "reloading shaders!\n";
					std::cout << "------------------------------\n";

					AssetCache<Shader>::Get()->ReloadAll();

					std::cout << "------------------------------\n";
					std::cout << "finished reloading shaders\n";
					std::cout << "------------------------------\n";
				}

				//if (ImGui::BeginMenu("Close Editor"))
				//{
				//	if(ImGui::MenuItem("Close"))
				//	{
				//		//Do something
				//	}
				//	ImGui::EndMenu();
				//}

				ImGui::EndMainMenuBar();
			}

			if (bCurveEditorActive)
			{
				ImGui::Begin("Curve Editor");
				{
					ImGui::CurveAssetEditor("Curve Asset Editor", curve, curvePath);
				}
				ImGui::End();
			}
		}

		co_await Suspend();
	}
}
