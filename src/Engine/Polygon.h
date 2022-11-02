#pragma once

#include <vector>
#include "Vec2.h"
#include "TasksConfig.h"

// Polygon join type (resizing)
enum class ePolygonType
{
	Chain,
	Outline,
	Solid,
};
enum class ePolygonJoinType
{
	Square,
	Miter,
	Round,
};
enum class ePolygonEndType
{
	Butt,
	Square,
	Round,
};

// Base polygon class
class Polygon
{
public:
	Polygon() {}
	Polygon(std::vector<Vec2f> in_verts, ePolygonType in_type = ePolygonType::Outline);

	// Accessors
	void SetType(ePolygonType in_type);
	ePolygonType GetType() const;
	void SetVerts(std::vector<Vec2f> in_verts);
	void SetVerts(std::vector<Vec2f>&& in_verts);
	const std::vector<Vec2f>& GetVerts() const
	{
		return m_verts;
	}

	// Orientation
	float SignedArea() const;
	bool IsCCW() const;
	void SetOrientation(bool in_ccw);

	// Transformations
	static std::vector<Polygon> Simplify(const std::vector<Polygon>& in_polys);
	static std::vector<Polygon> Resize(const std::vector<Polygon>& in_polys, float in_amount, ePolygonJoinType in_joinType = ePolygonJoinType::Square, ePolygonEndType in_endType = ePolygonEndType::Butt);
	static std::vector<Polygon> Triangulate(const std::vector<Polygon>& in_polys);

	// Index helper functions
	static size_t PrevIdx(size_t i, size_t n)
	{
		return i == 0 ? n - 1 : i - 1;
	}
	static size_t NextIdx(size_t i, size_t n)
	{
		return i == n - 1 ? 0 : i + 1;
	}

private:
	std::vector<Vec2f> m_verts;
	mutable std::optional<float> m_signedArea;
	ePolygonType m_type = ePolygonType::Outline;
};
