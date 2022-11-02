#include "Polygon.h"

#include "poly2tri.h"
#include "clipper.hpp"

//--- Helpers ---//
ClipperLib::Path VertsToPath(const std::vector<Vec2f>& in_verts)
{
	ClipperLib::Path path;
	path.reserve(in_verts.size());
	for(const auto& v : in_verts)
	{
		path.push_back({ (int)((double)v.x * 1024.0), (int)((double)v.y * 1024.0) });
	}
	return path;
}
std::vector<p2t::Point> PathToPoints(const ClipperLib::Path& in_path)
{
	std::vector<p2t::Point> verts;
	verts.reserve(in_path.size());
	for(const auto& p : in_path)
	{
		verts.push_back({ ((double)p.X / 1024.0), ((double)p.Y / 1024.0) });
	}
	return verts;
}
std::vector<Vec2f> PathToVerts(const ClipperLib::Path& in_path)
{
	std::vector<Vec2f> verts;
	verts.reserve(in_path.size());
	for(const auto& p : in_path)
	{
		verts.push_back({ (float)((double)p.X / 1024.0), (float)((double)p.Y / 1024.0) });
	}
	return verts;
}
std::vector<Polygon> TrisToPolygons(const std::vector<p2t::Triangle*>& in_tris)
{
	std::vector<Polygon> polys;
	polys.reserve(in_tris.size());
	for(const auto& tri : in_tris)
	{
		auto* p0 = tri->GetPoint(0);
		auto* p1 = tri->GetPoint(1);
		auto* p2 = tri->GetPoint(2);
		std::vector<Vec2f> verts = {
			{ (float)(p0->x), (float)(p0->y) },
			{ (float)(p1->x), (float)(p1->y) },
			{ (float)(p2->x), (float)(p2->y) },
		};
		polys.push_back({ verts, ePolygonType::Solid });
	}
	return polys;
}

//--- Polygon ---//
Polygon::Polygon(std::vector<Vec2f> in_verts, ePolygonType in_type)
	: m_verts(std::move(in_verts))
	, m_type(in_type)
{
}
void Polygon::SetVerts(std::vector<Vec2f> in_verts)
{
	m_signedArea.reset();
	m_verts = std::move(in_verts);
}
void Polygon::SetVerts(std::vector<Vec2f>&& in_verts)
{
	m_signedArea.reset();
	m_verts = std::move(in_verts);
}
float Polygon::SignedArea() const
{
	if(m_signedArea)
	{
		return m_signedArea.value();
	}

	// formula from http://geomalgorithms.com example code
	// see also https://stackoverflow.com/a/717367
	float area = 0.0;
	size_t n = m_verts.size();
	for(size_t idx = 0; idx < n; ++idx)
	{
		const auto& vert = m_verts[idx];
		const auto& nextVert = m_verts[NextIdx(idx, n)];
		const auto& prevVert = m_verts[PrevIdx(idx, n)];
		area += vert.x * (nextVert.y - prevVert.y);
	}
	area *= 0.5f;
	m_signedArea = area;
	return area;
}
bool Polygon::IsCCW() const
{
	return SignedArea() >= 0.0f;
}
void Polygon::SetOrientation(bool in_ccw)
{
	if(in_ccw != IsCCW())
	{
		std::reverse(m_verts.begin(), m_verts.end());
	}
}
void Polygon::SetType(ePolygonType in_type)
{
	m_type = in_type;
}
ePolygonType Polygon::GetType() const
{
	return m_type;
}
std::vector<Polygon> Polygon::Simplify(const std::vector<Polygon>& in_polys)
{
	std::vector<Polygon> polygons;
	ClipperLib::Paths paths;
	paths.reserve(in_polys.size());
	for(const auto& poly : in_polys)
	{
		if(poly.GetType() == ePolygonType::Solid)
		{
			auto path = VertsToPath(poly.GetVerts());
			ClipperLib::Paths simplePaths;
			SimplifyPolygon(path, simplePaths, ClipperLib::pftEvenOdd);
			paths.insert(paths.end(), simplePaths.begin(), simplePaths.end());
		}
		else
		{
			polygons.push_back(poly);
		}
	}
	SimplifyPolygons(paths, ClipperLib::pftNonZero);
	for(const auto& path : paths)
	{
		polygons.push_back({ PathToVerts(path), ePolygonType::Solid });
	}
	return polygons;
}
static ClipperLib::JoinType s_joinTypeLut[] = {
  ClipperLib::jtSquare,
  ClipperLib::jtMiter,
  ClipperLib::jtRound,
};
std::vector<Polygon> Polygon::Resize(const std::vector<Polygon>& in_polys, float in_amount, ePolygonJoinType in_joinType, ePolygonEndType in_endType)
{
	std::vector<Polygon> polygons;
	ClipperLib::ClipperOffset offset(2.0, 256.0f);
	for(const auto& poly : in_polys)
	{
		ClipperLib::Path path = VertsToPath(poly.GetVerts());
		ClipperLib::EndType endType	= (poly.GetType() == ePolygonType::Outline) ? ClipperLib::etClosedLine
									: (poly.GetType() == ePolygonType::Solid) ? ClipperLib::etClosedPolygon
									: (in_endType == ePolygonEndType::Butt) ? ClipperLib::etOpenButt
									: (in_endType == ePolygonEndType::Square) ? ClipperLib::etOpenSquare
									: ClipperLib::etOpenRound;
		offset.AddPath(path, s_joinTypeLut[(int32_t)in_joinType], endType);
	}
	ClipperLib::Paths offsetPaths;
	offset.Execute(offsetPaths, (int32_t)((double)in_amount * 1024.0));
	for(const auto& offPath : offsetPaths)
	{
		polygons.push_back({ PathToVerts(offPath), ePolygonType::Solid });
	}
	return polygons;
}
std::vector<Polygon> Polygon::Triangulate(const std::vector<Polygon>& in_polys)
{
	// Condition input polygons
	std::vector<Polygon> polygons;
	ClipperLib::Paths fullPaths;
	std::vector<std::vector<p2t::Point>> holePoints;
	for(const auto& poly : in_polys)
	{
		if(poly.GetType() != ePolygonType::Solid)
		{
			continue;
		}

		ClipperLib::Path path = VertsToPath(poly.GetVerts());
		ClipperLib::Paths simplePaths;
		SimplifyPolygon(path, simplePaths);
		if(Orientation(path))
		{
			fullPaths.insert(fullPaths.begin(), simplePaths.begin(), simplePaths.end());
		}
		else
		{
			holePoints.push_back(PathToPoints(path));
		}
	}
	
	// Convert holes to p2t representation
	std::vector<std::vector<p2t::Point*>> holes;
	for(auto& hp : holePoints)
	{
		std::vector<p2t::Point*> hole;
		for(auto& p : hp)
		{
			hole.push_back(&p);
		}
		holes.push_back(hole);
	}
	
	// Perform CDT
	for(const auto& path : fullPaths)
	{
		auto points = PathToPoints(path);
		std::vector<p2t::Point*> pointPtrs;
		for(auto& p : points)
		{
			pointPtrs.push_back(&p);
		}
		p2t::CDT cdt(pointPtrs);
		for(const auto& hole : holes)
		{
			cdt.AddHole(hole);
		}
		cdt.Triangulate();
		auto triPolys = TrisToPolygons(cdt.GetTriangles());
		polygons.insert(polygons.end(), triPolys.begin(), triPolys.end());
	}
	return polygons;
}
