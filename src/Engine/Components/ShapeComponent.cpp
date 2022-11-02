#include "ShapeComponent.h"

#include "Engine/GameWindow.h"

#include <SFML/Graphics.hpp>

void ShapeComponent::SetPolygon(Polygon in_poly)
{
	if(m_poly.GetVerts().size() != in_poly.GetVerts().size())
	{
		m_shape.reset();
		m_lines.clear();
	}
	m_poly = in_poly;
}
const Polygon& ShapeComponent::GetPolygon() const
{
	return m_poly;
}
void ShapeComponent::SetDrawOutline(bool in_outline)
{
	if(m_outline != in_outline)
	{
		m_shape.reset();
		m_lines.clear();
		m_outline = in_outline;
	}
}
bool ShapeComponent::GetDrawOutline() const
{
	return m_outline;
}
void ShapeComponent::SetFill(bool in_fill)
{
	m_fill = in_fill;
}
bool ShapeComponent::GetFill() const
{
	return m_fill;
}
void ShapeComponent::OnTransformChanged()
{
	DrawComponent::OnTransformChanged();
	m_shape.reset();
	m_lines.clear();
}
void ShapeComponent::Draw()
{
	DrawComponent::Draw();

	// Get world transform
	auto sfmlRenderTarget = GetTargetSFMLRenderTexture();
	const auto& worldTransform = GetWorldTransform();

	// Rebuild shape (if necessary)
	const auto& verts = m_poly.GetVerts();
	auto numVerts = verts.size();
	if(!m_shape && m_poly.GetType() == ePolygonType::Solid && m_fill)
	{
		auto tris = Polygon::Triangulate({ m_poly });
		for(const auto& tri : tris)
		{
			auto v1 = worldTransform.TransformPoint(tri.GetVerts()[0]);
			auto v2 = worldTransform.TransformPoint(tri.GetVerts()[1]);
			auto v3 = worldTransform.TransformPoint(tri.GetVerts()[2]);

			sf::Vertex verts[3];
			verts[0].position = { v1.x, v1.y };
			verts[1].position = { v2.x, v2.y };
			verts[2].position = { v3.x, v3.y };
			verts[0].color = sf::Color::Black;
			verts[1].color = sf::Color::Black;
			verts[2].color = sf::Color::Black;
			sfmlRenderTarget->draw(&verts[0], 3, sf::PrimitiveType::Triangles);
		}
	}

	// Draw shape
	if(m_shape)
	{
		sfmlRenderTarget->draw(m_shape.value());
	}

	// Draw outlines (as needed)
	if(m_outline && numVerts > 1)
	{
		if(m_lines.size() == 0)
		{
			m_lines.reserve(numVerts + 1);
			for(auto v : verts)
			{
				auto pos = worldTransform.TransformPoint(v);
				m_lines.push_back(sf::Vertex({ pos.x, pos.y }, sf::Color::White));
			}
			if(m_poly.GetType() != ePolygonType::Chain)
			{
				auto pos = worldTransform.TransformPoint(verts[0]);
				m_lines.push_back(sf::Vertex({ pos.x, pos.y }, sf::Color::White));
			}
		}
		sfmlRenderTarget->draw(m_lines.data(), m_lines.size(), sf::LineStrip);
	}
}
