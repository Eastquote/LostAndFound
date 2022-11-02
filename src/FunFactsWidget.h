#pragma once

#include "GameActor.h"
#include <map>

// UI Pause Menu element that displays room checkboxes and browsable text "fun facts" (not currently in use)
class FunFactsWidget : public GameActor {
public:
	virtual void Initialize() override;
	void CreateJibit();
	void UpdateJibits();
	void ChangeIndex(bool in_rightwards);
	virtual bool ShouldUpdateWhilePaused() const override { return true; }

private:
	int32_t m_index = 0;
	std::vector<int32_t> m_unlockedIds;
	std::shared_ptr<TextComponent> m_headingText0;
	std::shared_ptr<TextComponent> m_headingText1;
	std::shared_ptr<TextComponent> m_factText;
	std::shared_ptr<SpriteComponent> m_bkgSprite;
	std::vector<std::shared_ptr<SpriteComponent>> m_jibitSprites;
};