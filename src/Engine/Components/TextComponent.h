#pragma once

#include "Engine/Components/DrawComponent.h"

#include <SFML/Graphics/Text.hpp>
#include "Engine/Box.h"
class Font;

struct TextDef {
	std::string font = "PixicaMicro-Regular";
	int32_t size = 16;
	Vec2f alignment = { 0.5f, 0.0f };
	sf::Color color = sf::Color::White; // purplish
};

// Text Component
class TextComponent : public DrawComponent
{
public:
	virtual void Draw() override;

	const Box2f GetBounds() const;
	void SetFont(std::shared_ptr<Font> in_font);
	void SetFont(const std::string& in_fontName);
	std::shared_ptr<Font> GetFont() const;
	void SetText(const std::wstring& in_textStr);
	const std::wstring& GetText() const;
	void SetFontSizePixels(int32_t in_pixelSize);
	void SetAlignment(Vec2f in_alignment);
	void SetColor(sf::Color in_color) { m_color = in_color; }
	sf::Color GetColor() { return m_color; }
	void SetLineSpacingFactor(float in_factor) { m_lineSpacingFactor = in_factor; }

protected:
	virtual void OnTransformChanged() override;

	std::shared_ptr<Font> m_font;
	sf::Color m_color = sf::Color::White;
	sf::Text m_text;
	std::wstring m_textStr;
	int32_t m_pixelSize = 12;
	float m_lineSpacingFactor = 1.0f;
	Vec2f m_alignment = Vec2f::Zero;
	bool m_needsRefresh = true;
};
