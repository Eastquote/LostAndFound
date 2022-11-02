#pragma once

#include <functional>

#include "TasksConfig.h" // for std::optional
#include "MathCore.h"
#include "Polygon.h"
#include "MinMax.h"
#include "Box.h"
#include "Transform.h"
#include "Engine/DebugDrawSystem.h"

/*
MathGeometry: assorted geometry queries for points, lines/rays/segments, and polygons
- closest points between primitives
- distance between primitives
- boolean overlap checks
- etc

These functions should be enough to write many simple games without needing a full physics engine. However, they are generally unoptimized, so they shouldn't be used for anything too heavy (a few queries per frame per character is fine, but doing actual physics engine stuff will get slow fast).
*/

namespace Math
{
	/////////////////////////////////////////////////////////////////////////////
	// geometry primitives

	struct Line
	{
		Line(Vec2f in_p0, Vec2f in_dir) : m_p0(in_p0), m_normDir(in_dir.Norm()) {}

		Vec2f m_p0 = Vec2f::Zero;
		Vec2f m_normDir = Vec2f::Zero;

		Vec2f GetDir() const { return m_normDir; }
		Vec2f GetPointFromT(float in_t) const { return m_p0 + m_normDir * in_t; }
		float ProjectPointToLine_GetT(Vec2f in_pt) const { return (in_pt - m_p0).Dot(m_normDir); }
	};

	using Ray = Line;

	struct Segment
	{
		Vec2f m_p0 = Vec2f::Zero;
		Vec2f m_p1 = Vec2f::Zero;

		Vec2f GetPointFromT(float in_t) const { return m_p0 + (m_p1 - m_p0) * in_t; }
		Vec2f GetDir() const { return (m_p1 - m_p0).Norm(); }
		float GetLength() const { return (m_p1 - m_p0).Len(); }

		MinMaxf GetTRange() const { return {0, 1}; }

		float ProjectPointToLine_GetT(Vec2f in_pt) const { return (in_pt - m_p0).Dot(GetDir()) / GetLength(); }
	};

	struct Circle
	{
		Vec2f m_p0 = Vec2f::Zero;
		float m_r = 0.0f;

		bool Contains(Vec2f in_pt) const { return in_pt.Dist(m_p0) <= m_r; }
	};


	/////////////////////////////////////////////////////////////////////////////
	// misc utility

	inline Vec2f Project(Vec2f in_vec, Vec2f in_onto) {
		in_onto = in_onto.Norm();

		return in_onto * in_vec.Dot(in_onto);
	}

	/////////////////////////////////////////////////////////////////////////////
	// bounding boxes and bounding circles

	inline Box2f CalculateBoundingBox(Circle in_c) {
		return Box2f::FromCenter(in_c.m_p0, Vec2f::One * in_c.m_r * 2.0f);
	}

	inline Circle CalculateBoundingCircle(Box2f in_b) {
		return Circle{ in_b.GetCenter(), in_b.GetDims().Len() * 0.5f };
	}

	Box2f CalculateBoundingBox(const Polygon& in_poly);

	Circle ComputeCircleFromPoints(Vec2f in_p0, Vec2f in_p1);
	Circle ComputeCircleFromPoints(Vec2f in_p0, Vec2f in_p1, Vec2f in_p2);

	Circle CalculateBoundingCircle(const Polygon& in_poly);
	Circle CalculateBoundingCircle_Approx(const Polygon& in_poly);


	/////////////////////////////////////////////////////////////////////////////
	// boolean containment queries

	// treats all polygons as closed/solid, regardless of in_poly.type
	bool PointInPolygon(const Polygon& in_poly, Vec2f in_pt);

	inline bool PointInBox(Box2f in_box, Vec2f in_pt) {
		return in_box.GetExtentsX().Contains_Incl(in_pt.x) && in_box.GetExtentsY().Contains_Incl(in_pt.y);
	}

	inline bool BoxContainsBox(Box2f in_biggerBox, Box2f in_smallerBox) {
		auto outerBoxIsWider = in_biggerBox.GetDims().x > in_smallerBox.GetDims().x;
		auto outerBoxIsTaller = in_biggerBox.GetDims().y > in_smallerBox.GetDims().y;
		if(outerBoxIsWider && outerBoxIsTaller) {
			auto innerBoxCenter = in_smallerBox.GetCenter();
			// shrink down the outer box by the width/height of the inner box, so we can do a simple PointInBox() check:
			auto outerBoxContractedDims = Vec2f{ (in_biggerBox.GetDims().x - in_smallerBox.GetDims().x), (in_biggerBox.GetDims().y - in_smallerBox.GetDims().y) };
			in_biggerBox.SetDims_CenterAnchor(outerBoxContractedDims);
			//DrawDebugBox(in_biggerBox, sf::Color::Yellow); 
			auto xExtents = in_biggerBox.GetExtentsX();
			auto yExtents = in_biggerBox.GetExtentsY();
			//DrawDebugPoint({ xExtents.m_min, yExtents.m_min }, sf::Color::Red);
			//DrawDebugPoint({ xExtents.m_max, yExtents.m_max }, sf::Color::Red);
			//DrawDebugPoint(innerBoxCenter, sf::Color::Red);
			return PointInBox(in_biggerBox, innerBoxCenter);
		}
		else {
			return false;
		}
	}

	/////////////////////////////////////////////////////////////////////////////
	// closest point queries

	inline Vec2f ClosestPointOnLine(Line in_line, Vec2f in_pt) {
		return in_line.m_p0 + in_line.m_normDir * (in_pt - in_line.m_p0).Dot(in_line.m_normDir);
	}

	inline Vec2f ClosestPointOnSegment(Segment in_line, Vec2f in_pt, float* out_segmentT=nullptr) {
		const Vec2f dir = in_line.GetDir();
		const float distAlongLine = (in_pt - in_line.m_p0).Dot(dir);
		const float t = Math::ClampUnit(distAlongLine / in_line.GetLength());
		
		if (out_segmentT) *out_segmentT = t;
		
		return in_line.GetPointFromT(t);
	}

	// if pt is inside, returns pt
	inline Vec2f ClosestPointOnBox(Box2f in_box, Vec2f in_pt) {
		return {
			in_box.GetExtentsX().Clamp(in_pt.x),
			in_box.GetExtentsY().Clamp(in_pt.y)
		};
	}

	// if pt is inside, returns closest point on the border
	inline Vec2f ClosestPointOnBoxBorder(Box2f in_box, Vec2f in_pt) {
		return {
			in_box.GetExtentsX().ClosestEdge(in_pt.x),
			in_box.GetExtentsY().ClosestEdge(in_pt.y)
		};
	}

	// by default, treats the polygon differently based on its ePolygonType type
	// this can be overridden by passing in_overridePolyType
	Vec2f ClosestPointOnPolygon(const Polygon& in_poly, Vec2f in_pt, std::optional<ePolygonType> in_overridePolyType={},
		int32_t* out_edgeIdx=nullptr, float* out_edgeT=nullptr);


	/////////////////////////////////////////////////////////////////////////////
	// signed distance queries

	// treats all polygons as solid (rather than as polylines, which have no interior, which makes signed dist make less sense)
	inline float SignedDistance(const Polygon& in_poly, Vec2f in_pt) {
		const Vec2f closestPtOnBorder = ClosestPointOnPolygon(in_poly, in_pt, ePolygonType::Outline);
		const bool bInside = PointInPolygon(in_poly, in_pt);
		return closestPtOnBorder.Dist(in_pt) * (bInside ? -1 : 1);
	}

	inline float SignedDistance(Box2f in_box, Vec2f in_pt) {
		const bool bInside = PointInBox(in_box, in_pt);
		return ClosestPointOnBoxBorder(in_box, in_pt).Dist(in_pt) * (bInside ? -1 : 1);
	}

	inline float SignedDistance(Circle in_circle, Vec2f in_pt) {
		return in_circle.m_p0.Dist(in_pt) - in_circle.m_r;
	}

	/////////////////////////////////////////////////////////////////////////////
	// box-box intersection

	// returns the overlapping region
	inline std::optional<Box2f> IntersectBoxes(Box2f in_box0, Box2f in_box1)
	{
		std::optional<MinMaxf> xIntersect = in_box0.GetExtentsX().Intersect(in_box1.GetExtentsX());
		if (!xIntersect) return {};

		std::optional<MinMaxf> yIntersect = in_box0.GetExtentsY().Intersect(in_box1.GetExtentsY());
		if (!yIntersect) return {};

		return Box2f::FromAxisExtents(*xIntersect, *yIntersect);
	}


	/////////////////////////////////////////////////////////////////////////////
	// polygon overlap query

	// requires convex polygons
	// treats both polygons as solid
	// if they overlap, returns the separating vector (the vector by which you'd have to move poly1 to it stop overlapping)
	std::optional<Vec2f> OverlapConvexPolygons(const Polygon& in_poly0, const Polygon& in_poly1);

	// if they overlap, returns the separating vector (the vector by which you'd have to move box1 to it stop overlapping)
	inline std::optional<Vec2f> OverlapBoxes(Box2f in_box0, Box2f in_box1)
	{
		const std::optional<Box2f> intersection = IntersectBoxes(in_box0, in_box1);
		if (!intersection) return {};

		const Vec2f iDims = intersection->GetDims();

		const Vec2f axis = iDims.x > iDims.y ? Vec2f{ 1.f, 0.f } : Vec2f{ 0.f, 1.f };
		return iDims * axis * (in_box1.GetCenter().Dot(axis) > in_box0.GetCenter().Dot(axis) ? 1.f : -1.f);
	}

	inline std::optional<Vec2f> OverlapConvexPolygonAndBox(const Polygon& in_poly0, Box2f in_box0)
	{
		return OverlapConvexPolygons(in_poly0, in_box0.ToPolygon());
	}

	// if in_poly0 is concave, the boolean overlap check will still correct, but the returned vector will not be guaranteed to fully separate the shapes
	inline std::optional<Vec2f> OverlapConvexPolygonAndCircle(const Polygon& in_poly0, Circle in_circle)
	{
		const Vec2f closest = ClosestPointOnPolygon(in_poly0, in_circle.m_p0, ePolygonType::Outline);
		const float dist = closest.Dist(in_circle.m_p0);
		
		const bool bInside = PointInPolygon(in_poly0, in_circle.m_p0);

		if (dist > in_circle.m_r && !bInside) return {};
		
		if (bInside)
		{
			return (closest - in_circle.m_p0).Trunc(in_circle.m_r + dist);
		}
		else
		{
			return (in_circle.m_p0 - closest).Trunc(in_circle.m_r - dist);
		}
	}


	/////////////////////////////////////////////////////////////////////////////
	// intersections between shape/line/ray/segment

	std::optional<Vec2f> IntersectSegments(Segment in_seg0, Segment in_seg1,
		float* out_t0 = nullptr, float* out_t1 = nullptr);

	std::optional<Vec2f> IntersectSegmentAndLine(Segment in_seg, Line in_line,
		float* out_t0 = nullptr, float* out_t1 = nullptr);

	std::optional<Vec2f> IntersectSegmentAndRay(Segment in_seg, Line in_line,
		float* out_t0 = nullptr, float* out_t1 = nullptr);

	// returns a std::vector containing all intersection points
	std::vector<Vec2f> IntersectLineAndPolygon(Line in_line, const Polygon& in_poly);
  
	std::optional<Vec2f> IntersectLines(Line in_line0, Line in_line1);

	
	/////////////////////////////////////////////////////////////////////////////
	// raycasts and shape sweeps
	// In general, these prefer to ignore collisions that are exactly/barely touching, since such collisions can make physics objects get stuck on corners and edges

	struct RaycastResults
	{
		RaycastResults(Vec2f in_pos, float in_dist, Vec2f in_normal) : m_pos(in_pos), m_dist(in_dist), m_normal(in_normal){}

		Vec2f m_pos;
		float m_dist;
		Vec2f m_normal;
	};

	struct BoxSweepResults
	{
		BoxSweepResults(Box2f in_sweptBox, float in_dist, Vec2f in_normal) : m_sweptBox(in_sweptBox), m_dist(in_dist), m_normal(in_normal){} 

		Box2f m_sweptBox;
		float m_dist;
		Vec2f m_normal;
	};

	std::optional<RaycastResults> CastRayAgainstPolygon(Ray in_ray, const Polygon& in_poly);

	std::optional<RaycastResults> CastRayAgainstBox(Ray in_ray, Box2f in_box);
	
	std::optional<RaycastResults> CastRayAgainstGrid(Ray in_ray, float in_maxDist, Transform in_gridToWorldTM, Box2i in_gridDimsBox, std::function<bool(Vec2i)> in_gridCellIsSolidFunc);

	std::optional<BoxSweepResults> SweepBoxAgainstBox(Box2f in_boxToSweep, Vec2f in_sweepVec, Box2f in_boxToSweepAgainst);

	std::optional<BoxSweepResults> SweepBoxAgainstGrid(Box2f in_box, Vec2f in_sweepVec, Transform in_gridToWorldTM, Box2i in_gridDimsBox, std::function<bool(Vec2i)> in_gridCellIsSolidFunc);



	/////////////////////////////////////////////////////////////////////////////
	// closest points on 2 segments

	// TODO
	//// returns a pair of { the closest point on seg0 to seg1, and the closest point on seg1 to seg0 }
	//std::pair<Vec2f, Vec2f> ClosestPointsOnSegments(Segment in_seg0, Segment in_seg1)
	//{
	//	const float t0_numerator = (in_seg1.m_p0 - in_seg0.m_p0).Cross(in_seg1.GetDir());
	//	const float t0_denom = in_seg0.GetDir().Cross(in_seg1.GetDir());

	//	// check if they're parallel 
	//	if (t0_denom == 0.0f)
	//	{
	//		// get the t values on seg0 for the endpoints of seg1
	//		const MinMaxf seg0Range = in_seg0.GetTRange();
	//		MinMaxf seg1Range = { in_seg0.ProjectPointToLine_GetT(in_seg1.m_p0), in_seg0.ProjectPointToLine_GetT(in_seg1.m_p1) };
	//		seg1Range.EnforceAscending();

	//		// check if seg1's t values intersect [0, 1]
	//		const std::optional<MinMaxf> tIntersection = seg0Range.Intersection(seg1Range);
	//		if (tIntersection)
	//		{
	//			return in_seg0.GetPointFromT(tIntersection->Mid());
	//		}
	//		else
	//		{
	//		}
	//	}

	//	// lines intersect -> calculate the t values for the intersection on each segment, and make sure they actually lie on the segments

	//	const float t0 = t0_numerator / t0_denom;
	//	if (!in_seg0.GetTRange().Contains_Incl(t0)) return {};

	//	const Vec2f lineIntersection = in_seg0.GetPointFromT(t0);

	//	const float t1 = in_seg1.ProjectPointToLine_GetT(lineIntersection);
	//	if (!in_seg1.GetTRange().Contains_Incl(t1)) return {};

	//	return lineIntersection;
	//}
};
