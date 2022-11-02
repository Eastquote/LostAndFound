#pragma once

#include "Engine/Components/DrawComponent.h"
#include "Engine/Polygon.h"

#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/Vertex.hpp>

// Shape Component
class ShapeComponent : public DrawComponent
{
public:
	virtual void Draw() override;

	void SetPolygon(Polygon in_poly);
	const Polygon& GetPolygon() const;
	void SetDrawOutline(bool in_outline);
	bool GetDrawOutline() const;
	void SetFill(bool in_fill);
	bool GetFill() const;

protected:
	virtual void OnTransformChanged() override;

	std::optional<sf::ConvexShape> m_shape;
	std::vector<sf::Vertex> m_lines;
	Polygon m_poly;
	bool m_outline = true;
	bool m_fill = true;
};
