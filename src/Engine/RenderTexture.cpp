#include "RenderTexture.h"

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Color.hpp>

#include "Engine/GameWindow.h"
#include "Engine/Shader.h"

RenderTexture::RenderTexture(const Vec2i& in_textureSize)
{
	m_renderTexture = std::make_shared<sf::RenderTexture>();

	sf::ContextSettings settings{};
	m_renderTexture->create(in_textureSize.x, in_textureSize.y, settings);

	SetView(Box2f::FromBottomLeft(Vec2f::Zero, in_textureSize));	
}

RenderTexture::~RenderTexture()
{
}

void RenderTexture::Clear(sf::Color in_color)
{
	m_renderTexture->clear(in_color);
}

void RenderTexture::Draw(Transform in_transform, std::shared_ptr<RenderTexture> in_renderToTexture, std::shared_ptr<Shader> in_shader) const
{
	m_renderTexture->display();

	sf::Sprite sprite(m_renderTexture->getTexture());
	sprite.setOrigin(0.0f, (float)m_renderTexture->getSize().y);
	sprite.setPosition(in_transform.pos.x, in_transform.pos.y);
	sprite.setRotation(in_transform.rot);
	sprite.setScale(in_transform.scale.x, -in_transform.scale.y);

	sf::RenderStates renderStates = sf::RenderStates::Default;
	if (in_shader)
	{
		// Gotta use sf::Shader::CurrentTexture rather than binding our own texture the normal way because otherwise we get "An internal OpenGL call failed in Texture.cpp(769)" error spam
		// since we're technically drawing a sprite containing our own texture, this is probably more what we want anyway
		in_shader->GetSFMLShader().setUniform("texture", sf::Shader::CurrentTexture);
		renderStates = sf::RenderStates(&in_shader->GetSFMLShader());
	}

	if (in_renderToTexture)
	{
		in_renderToTexture->GetSFMLRenderTexture()->draw(sprite, renderStates);
	}
	else
	{
		// NOTE: this is different from DrawComponents. 
		// if no target RenderTexture is passed, this draws directly to the window, rather than to the default game render texture. 
		// we need this to be able to draw our game view to the window. maybe it'd be better to make draw-to-window an explicit option though for consistency...
		GetWindow()->GetSFMLWindow()->draw(sprite, renderStates);
	}
}

void RenderTexture::SetView(const Box2f& in_view, float in_angle)
{
	m_view = in_view;

	sf::FloatRect rect(in_view.x, in_view.y, in_view.w, in_view.h);

	if (!GetSmooth())
	{
		// if we're in pixel mode, floating point geometry can shimmer when we move the view if the view pos isn't snapped like this
		rect.left = Math::Floor(rect.left);
		rect.top = Math::Floor(rect.top);
	}

	sf::View view(rect);

	// invert y to make the y axis point up. we also invert the y scale of sfml graphics primitives (sprite, text, etc) in their respective component Draw() methods 
	view.setSize(view.getSize().x, -view.getSize().y);
	view.setRotation(in_angle);
	m_renderTexture->setView(view);
}

Box2f RenderTexture::GetView() const
{
	return m_view;
}

void RenderTexture::SetSmooth(bool in_smooth)
{
	m_renderTexture->setSmooth(in_smooth);
}

bool RenderTexture::GetSmooth() const
{
	return m_renderTexture->isSmooth();
}

Vec2u RenderTexture::GetSize() const
{
	auto size = m_renderTexture->getSize();
	return { size.x, size.y };
}

sf::RenderTexture* RenderTexture::GetSFMLRenderTexture() const
{
	return m_renderTexture.get();
}

// copied from sf::RenderTarget::MapPixelToCoords, but changed to use float coordinates the whole way
Vec2f RenderTexture::ViewToWorld(const Vec2f& in_windowPos) const
{
	sf::View view = m_renderTexture->getView();

	// First, convert from viewport coordinates to homogeneous coordinates
	Vec2f normalized;
	sf::IntRect viewport = m_renderTexture->getViewport(view);
	normalized.x = -1.f + 2.f * (in_windowPos.x - viewport.left) / viewport.width;
	normalized.y = 1.f - 2.f * (in_windowPos.y - viewport.top) / viewport.height;

	// Then transform by the inverse of the view matrix
	auto ret = view.getInverseTransform().transformPoint({ normalized.x, normalized.y });
	return { ret.x, ret.y };
}

// copied from sf::RenderTarget::MapCoordsToPixel, but changed to use float coordinates the whole way
Vec2f RenderTexture::WorldToView(const Vec2f& in_worldPos) const
{
	sf::View view = m_renderTexture->getView();

	// First, transform the point by the view matrix
	sf::Vector2f normalized = view.getTransform().transformPoint({ in_worldPos.x, in_worldPos.y });

	// Then convert to viewport coordinates
	Vec2f pixel;
	sf::IntRect viewport = m_renderTexture->getViewport(view);
	pixel.x = (normalized.x + 1.f) / 2.f * viewport.width + viewport.left;
	pixel.y = (-normalized.y + 1.f) / 2.f * viewport.height + viewport.top;

	return pixel;
}