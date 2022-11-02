#pragma once

#include "GameActor.h"

struct MenuItemDef {
	const wchar_t* text = L"**DEFAULT**";
	std::function<void()> highlightFunc = {};
	std::function<void()> unhighlightFunc = {};
	std::function<void()> activateFunc = {};
	std::optional<sf::Color> idleColor;
	std::optional<sf::Color> highlightColor;
};

// Widget for menu items (e.g. "New Game" on the main menu) that handles text appearance, highlight behavior, and function dispatch
class MenuItem : public GameActor {
public:
	MenuItem(const MenuItemDef& in_def, int32_t in_index)
		: m_def(std::make_shared<MenuItemDef>(in_def))
		, m_index(in_index)
	{
	}
	virtual bool ShouldUpdateWhilePaused() const override { return true; }
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;
	const int32_t GetIndex() { return m_index; }
	void SetIndex(int32_t in_index) { m_index = in_index; }
	bool MatchIndex(int32_t in_index) { return m_index == in_index; }
	void SetActive(bool in_state) { m_highlighted = in_state; }
	Box2f GetBounds() const;
	void TryHighlight();
	void TryUnhighlight();
	void TryActivate();

private:
	std::shared_ptr<MenuItemDef> m_def = {};
	int32_t m_index = 0;
	std::shared_ptr<TextComponent> m_textComp = {};
	bool m_highlighted = false;
};