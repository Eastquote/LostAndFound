#pragma once

#include "Engine/Components/TextComponent.h"
#include "Engine/Font.h"

TextDef g_pickupInfoHeading = {
	"Pixica-Bold",
	16,
	{ 0.5f, 0.0f },
	sf::Color{ 146, 144, 255, 255 } // purplish
};

TextDef g_pickupInfoBody = {
	"PixicaMicro-Regular",
	16,
	{ 0.5f, 0.0f },
	sf::Color::White
};