#pragma once

#include "MenuItem.h"

// UI Item Functions
namespace uiFuncs {
	void OnResumeActivate(); // TODO: how do I let these call methods on the menuitem owner(i.e. pause menu) object?
	void OnMainMenuHighlight();
	void OnMainMenuUnhighlight();
	void OnFunFactsHighlight();
	void OnFunFactsUnhighlight();
	void OnNewGameActivate();
	void OnContinueGameActivate();
	void OnExitGameActivate();
	void OnConfirmActivate();
	void OnCancelActivate();
}

// Pause Menu Item Defs:
static MenuItemDef ui_ResumeItem = {
	L"Resume",
	nullptr,
	nullptr,
	uiFuncs::OnResumeActivate,
	sf::Color{ 146, 144, 255, 255 }, // purple
	sf::Color{ 255, 255, 255, 255 }, // white

};
static MenuItemDef ui_MainMenuItem = {
	L"Main Menu",
	uiFuncs::OnMainMenuHighlight,
	uiFuncs::OnMainMenuUnhighlight,
	nullptr,
	sf::Color{ 146, 144, 255, 255 }, // purple
	sf::Color{ 255, 255, 255, 255 }, // white
};
static MenuItemDef ui_FunFactsItem = {
	L"Fun Facts",
	uiFuncs::OnFunFactsHighlight,
	uiFuncs::OnFunFactsUnhighlight,
	nullptr,
	sf::Color{ 146, 144, 255, 255 }, // purple
	sf::Color{ 255, 255, 255, 255 }, // white
};

// Main Menu Item Defs:
static MenuItemDef ui_NewGameItem = {
	L"NEW GAME",
	nullptr,
	nullptr,
	uiFuncs::OnNewGameActivate,
	sf::Color{ 192, 203, 220, 255 }, // bluish
	sf::Color{ 251, 251, 251, 255 }, // white
};
static MenuItemDef ui_ContinueGameItem = {
	L"CONTINUE",
	nullptr,
	nullptr,
	uiFuncs::OnContinueGameActivate,
	sf::Color{ 192, 203, 220, 255 }, // bluish
	sf::Color{ 251, 251, 251, 255 }, // white
};
static MenuItemDef ui_ExitGameItem = {
	L"EXIT",
	nullptr,
	nullptr,
	uiFuncs::OnExitGameActivate,
	sf::Color{ 192, 203, 220, 255 }, // bluish
	sf::Color{ 251, 251, 251, 255 }, // white
};
// Confirm Menu Item Defs:
static MenuItemDef ui_ConfirmItem = {
	L"CONFIRM",
	nullptr,
	nullptr,
	uiFuncs::OnConfirmActivate,
	sf::Color{ 192, 203, 220, 255 }, // bluish
	sf::Color{ 251, 251, 251, 255 }, // white
};
static MenuItemDef ui_CancelItem = {
	L"CANCEL",
	nullptr,
	nullptr,
	uiFuncs::OnCancelActivate,
	sf::Color{ 192, 203, 220, 255 }, // bluish
	sf::Color{ 251, 251, 251, 255 }, // white
};
