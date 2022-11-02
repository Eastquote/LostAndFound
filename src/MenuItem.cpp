#include "MenuItem.h"

#include "GameWorld.h"
#include "Engine/Components/TextComponent.h"
#include "Engine/Font.h"
#include <codecvt>
#include <locale>

namespace uiFuncs {
	void OnResumeActivate() {
		// play unpause sound
		//GameWorld::Get()->UnPause();
	}
	void OnMainMenuHighlight() {
		// set confirm widget to active
	}
	void OnMainMenuUnhighlight() {
		// set confirm widget to inactive
	}
	void OnFunFactsHighlight() {
		// set fun facts widget to active
	}
	void OnFunFactsUnhighlight() {
		// set fun facts widget to inactive
	}
	void OnNewGameActivate() {
		// play confirm sound
	}
	void OnContinueGameActivate() {
		// play confirm sound
	}
	void OnExitGameActivate() {
		std::exit(EXIT_SUCCESS);
	}
	void OnConfirmActivate() {
	}
	void OnCancelActivate() {
	}
}

//--- MENU ITEMS ---//

void MenuItem::Initialize() {
	GameActor::Initialize();
	SetDrawLayer(5);
	// Setup text
	m_textComp = MakeText(Transform::Identity);
	m_textComp->SetRenderLayer("hud");
	m_textComp->SetFont("Pixica-Bold");
	m_textComp->GetFont()->SetSmooth(false);
	m_textComp->SetFontSizePixels(16);
	m_textComp->SetText(m_def->text);
	m_textComp->SetAlignment({ 0.5f, 0.0f });
	m_textComp->SetColor(m_def->idleColor.value());
	m_textComp->SetComponentDrawOrder(10);
	auto text = L"**DEFAULT**";
}
Task<> MenuItem::ManageActor() {
	SetTimeStream(eTimeStream::Real);
	auto basePos = m_textComp->GetWorldPos();
	auto indentPos = m_textComp->GetWorldPos() + Vec2f{ -0.0f, 0.0f }; //< Zeroed out for now
	auto prevHighlighted = false;
	while(true) {
		if(prevHighlighted != m_highlighted) {
			if(m_highlighted) {
				m_textComp->SetWorldPos(indentPos);
				m_textComp->SetColor(m_def->highlightColor.value());
			}
			else {
				m_textComp->SetWorldPos(basePos);
				m_textComp->SetColor(m_def->idleColor.value());
			}
		}
		prevHighlighted = m_highlighted;
		co_await Suspend();
	}
}
Box2f MenuItem::GetBounds() const {
	return m_textComp->GetBounds();
}
void MenuItem::TryHighlight() {
	if(m_def->highlightFunc) {
		m_def->highlightFunc();
	}
}
void MenuItem::TryUnhighlight() {
	if(m_def->unhighlightFunc) {
		m_def->unhighlightFunc();
	}
}
void MenuItem::TryActivate() {
	if(m_def->activateFunc) {
		m_def->activateFunc();
	}
}
