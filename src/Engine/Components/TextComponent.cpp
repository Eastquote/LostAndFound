#include "TextComponent.h"

#include "Engine/AssetCache.h"
#include "Engine/Font.h"

#include "Engine/GameWindow.h"

#include <SFML/Graphics.hpp>

void TextComponent::SetFont(std::shared_ptr<Font> in_font)
{
	m_font = in_font;
	m_needsRefresh = true;
}
void TextComponent::SetFont(const std::string& in_fontName)
{
	if(auto font = AssetCache<Font>::Get()->LoadAsset(std::string() + "data/fonts/" + in_fontName + ".ttf"))
	{
		if(font != m_font)
		{
			SetFont(font);
		}
	}
}
std::shared_ptr<Font> TextComponent::GetFont() const
{
	return m_font;
}
void TextComponent::SetText(const std::wstring& in_textStr)
{
	m_textStr = in_textStr;
	m_needsRefresh = true;
}
const std::wstring& TextComponent::GetText() const
{
	return m_textStr;
}
void TextComponent::SetFontSizePixels(int32_t in_pixelSize)
{
	m_pixelSize = in_pixelSize;
	m_needsRefresh = true;
}
void TextComponent::SetAlignment(Vec2f in_alignment)
{
	m_alignment = in_alignment;
}
const Box2f TextComponent::GetBounds() const
{
	auto rect = m_text.getGlobalBounds();
	auto result = Box2f::FromTopLeft(Vec2f{ rect.left, rect.top }, Vec2f{ rect.width, rect.height });
	return result;
}
void TextComponent::OnTransformChanged()
{
	DrawComponent::OnTransformChanged();
	m_needsRefresh = true;
}
void TextComponent::Draw()
{
	DrawComponent::Draw();

	if(m_needsRefresh)
	{
		if(m_font)
		{
			m_text.setFont(m_font->GetSFMLFont());
		}
		m_text.setString(m_textStr);
		m_text.setFillColor(m_color); // TODO: set this up to be a member variable w/ setter/getter
		m_text.setOutlineColor(sf::Color::Black);
		m_text.setOutlineThickness(0);
		m_text.setCharacterSize(m_pixelSize);
		m_text.setLineSpacing(m_lineSpacingFactor);
		m_font->SetSmooth(false);
		const sf::Texture& texture = m_font->GetSFMLFont().getTexture(m_pixelSize);
		const_cast<sf::Texture*>(&texture)->setSmooth(false);
		m_needsRefresh = false;
	}

	// Set origin based on alignment, and remove any padding in the top left (sfml likes to add ~10px at the top)
	const auto bounds = m_text.getLocalBounds();
	m_text.setOrigin(bounds.width * m_alignment.x + bounds.left, bounds.height * m_alignment.y + bounds.top);

	// Get world transform
	const auto& worldTransform = GetWorldTransform();
	m_text.setPosition(worldTransform.pos.x, worldTransform.pos.y);
	m_text.setRotation(worldTransform.rot);
	m_text.setScale(worldTransform.scale.x, -worldTransform.scale.y);

	// Draw text
	if(m_textStr.size())
	{
		auto sfmlRenderTarget = GetTargetSFMLRenderTexture();
		sfmlRenderTarget->draw(m_text);
	}
}
