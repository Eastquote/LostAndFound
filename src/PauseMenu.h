#pragma once
#include "GameActor.h"

class MenuItem;
class ConfirmWidget;
class FactWidget;
class InputComponent;
struct Token;

// Menu that pauses gameplay; overlays a dark quad and various MenuItem widgets
class PauseMenu : public GameActor {
public:
	virtual bool ShouldUpdateWhilePaused() const override { return true; }
	virtual void Initialize() override;
	virtual Task<> ManageActor() override;

protected:
	std::shared_ptr<InputComponent> m_inputComp;

private:
	void SetPause(bool bPaused);

	std::shared_ptr<Token> m_pauseToken;
	std::shared_ptr<SpriteComponent> m_bkgQuad;
	std::shared_ptr<TextComponent> m_headingTextComp;
	std::vector<std::shared_ptr<MenuItem>> m_menuItems;
	int32_t m_index = 0;
	std::shared_ptr<ConfirmWidget> m_confirmWidget;
};
