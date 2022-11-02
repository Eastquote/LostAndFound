#pragma once

#include <memory>

// Forward declarations
class Shader;

namespace sf
{
	class RenderTexture;
	class Color;
}

#include "Vec2.h"
#include "Transform.h"
#include "Box.h"

// Render Texture
class RenderTexture
{
public:
	RenderTexture(const Vec2i& in_textureDims);
	~RenderTexture();

	void Clear(sf::Color in_color);
	void Draw(Transform in_transform=Transform{}, std::shared_ptr<RenderTexture> in_renderToTexture=nullptr, std::shared_ptr<Shader> in_shader=nullptr) const;

	void SetView(const Box2f& in_view, float in_angle = 0.0f);
	Box2f GetView() const;

	void SetSmooth(bool in_smooth);
	bool GetSmooth() const;

	Vec2u GetSize() const;

	Vec2f ViewToWorld(const Vec2f& in_windowPos) const;
	Vec2f WorldToView(const Vec2f& in_worldPos) const;

	sf::RenderTexture* GetSFMLRenderTexture() const;

private:
	Box2f m_view;
	std::shared_ptr<sf::RenderTexture> m_renderTexture;
};
