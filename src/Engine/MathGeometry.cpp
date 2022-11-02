#include "MathGeometry.h"

#include "DebugDrawSystem.h"

/////////////////////////////////////////////////////////////////////////////
// bounding boxes and bounding circles

Box2f Math::CalculateBoundingBox(const Polygon& in_poly)
{
	const auto& verts = in_poly.GetVerts();
	if (verts.size() == 0) return Box2f{ 0, 0, 0, 0 };

	Box2f box = Box2f::FromCenter(verts[0], Vec2f::Zero);

	for (int i = 1; i < verts.size(); i++)
	{
		box.ExpandToInclude(verts[i]);
	}

	return box;
}

Math::Circle Math::CalculateBoundingCircle_Approx(const Polygon& in_poly)
{
	// approx bounding circle algorithm from Ritter90, as described in Ericson

	const auto& verts = in_poly.GetVerts();
	if (verts.size() == 0) return Circle{ Vec2f::Zero, 0.0f };

	// get extreme points on each axis
	const Vec2f v0 = verts[0];
	Vec2f minX = v0, maxX = v0, minY = v0, maxY = v0;
	for (int i = 0; i < verts.size(); i++)
	{
		const Vec2f v = verts[i];
		if (v.x < minX.x) minX = v;
		if (v.x > maxX.x) maxX = v;
		if (v.y < minY.y) minY = v;
		if (v.y > maxY.y) maxY = v;
	}

	const float distX = minX.Dist(maxX);
	const float distY = minY.Dist(maxY);

	// init circle based on the axis extremes pair that's more distant (x min/max or y min/max)
	Circle c{};
	if (distX > distY)
	{
		c = Circle{ (maxX + minX) * 0.5f, distX * 0.5f };
	}
	else
	{
		c = Circle{ (maxY + minY) * 0.5f, distY * 0.5f };
	}

	// enlarge+shift circle to include all other points
	for (Vec2f v : verts)
	{
		const float dist = v.Dist(c.m_p0);
		if (dist > c.m_r)
		{
			// grow by half the distance
			const float newR = (c.m_r + dist) * 0.5f;
			// and shift the center to cover the other half
			const float centerAdjustDist = newR - c.m_r;

			c.m_r = newR;
			c.m_p0 += (v - c.m_p0) / dist * centerAdjustDist;
		}
	}

	return c;
}

Math::Circle Math::ComputeCircleFromPoints(Vec2f in_p0, Vec2f in_p1)
{
	return Circle{ (in_p1 + in_p0) * 0.5f, in_p1.Dist(in_p0) * 0.5f + SMALL_NUMBER };
}

Math::Circle Math::ComputeCircleFromPoints(Vec2f in_p0, Vec2f in_p1, Vec2f in_p2)
{
	// based on Capens01 https://www.flipcode.com/archives/Smallest_Enclosing_Spheres.shtml
	// or https://stackoverflow.com/a/34326390

	const Vec2f a = in_p1 - in_p0;
	const Vec2f b = in_p2 - in_p0;

	const auto Len2 = [](Vec2f in_v) { return in_v.Dot(in_v); };
	const auto DoubleCross = [](Vec2f in_v, Vec2f in_orthoTo) {
		// equivalent of ((in_orthoTo x in_v) x in_orthoTo), where x is 3d cross product
		float cross = in_orthoTo.Cross(in_v);
		return Vec2f{ -in_orthoTo.y * cross, in_orthoTo.x * cross };
	};

	Vec2f o = DoubleCross(b, a) * Len2(b) + DoubleCross(a, b) * Len2(a);
	o /= 2.0f * Sqr(a.Cross(b));

	return Circle{ in_p0 + o, o.Len() };
}

static Math::Circle WelzlComputeBoundingCircle_Internal(const std::vector<Vec2f>& in_verts, uint32_t in_numVerts, std::vector<Vec2f>& in_sos, uint32_t in_numSos)
{
	// based on Ericson RTCD example code
	// in_sos being a reference is intentional and works with this algorithm, but means that we have to manually track in_numSos separately and do some manual manipulation to stop recursion steps from "seeing" supports they shouldn't? 

	// if no input points, the recursion has bottomed out. Now compute an
	// exact sphere based on points in set of support (zero through four points)
	if (in_numVerts == 0) 
	{
		switch (in_numSos) 
		{
		case 0: 
			return Math::Circle{Vec2f::Zero, -1.0f};
		case 1: 
			return Math::Circle{in_sos[0], SMALL_NUMBER};
		case 2: 
			return Math::ComputeCircleFromPoints(in_sos[0], in_sos[1]);
		case 3: 
			return Math::ComputeCircleFromPoints(in_sos[0], in_sos[1], in_sos[2]);
		default:
			SQUID_RUNTIME_ERROR("unexpected number of support points");
		}
	}

	// pop a point at random (TODO make it actually random)
	const Vec2f selectedVert = in_verts[in_numVerts - 1];
	in_numVerts--;

	// recursively compute the smallest bounding sphere of the remaining points
	// beware of stack overflow from this call on very large numbers of points 
	Math::Circle smallestCircle = WelzlComputeBoundingCircle_Internal(in_verts, in_numVerts, in_sos, in_numSos);

	// if the selected point lies inside this sphere, it is indeed the smallest
	if(smallestCircle.Contains(selectedVert)) return smallestCircle;

	// otherwise, update set of support to additionally contain the new point
	in_sos.resize(in_numSos + 1);
	in_sos[in_numSos] = selectedVert;
	in_numSos++;

	// recursively compute the smallest sphere of remaining points with new s.o.s.
	return WelzlComputeBoundingCircle_Internal(in_verts, in_numVerts, in_sos, in_numSos);
}

Math::Circle Math::CalculateBoundingCircle(const Polygon& in_poly)
{
	const auto& verts = in_poly.GetVerts();
	std::vector<Vec2f> sos;
	sos.reserve(verts.size());
	return WelzlComputeBoundingCircle_Internal(verts, (uint32_t)verts.size(), sos, 0);
}


/////////////////////////////////////////////////////////////////////////////
// boolean containment queries

bool Math::PointInPolygon(const Polygon& in_poly, Vec2f in_pt)
{
	// line test - test a horizontal ray through in_pt against each edge. iff the number of intersections is odd, the point is inside

	int intersections = 0;

	const std::vector<Vec2f>& verts = in_poly.GetVerts();
	for (int i = 0; i < verts.size(); i++)
	{
		const Vec2f p0 = verts[i];
		const Vec2f p1 = verts[Polygon::NextIdx(i, verts.size())];


		if (p1.y != p0.y)
		{
			// t on the edge where a horizontal line through in_pt intersects
			const float t = (in_pt.y - p0.y) / (p1.y - p0.y);

			// use open interval on one side to avoid double-counting lines that go through vertices
			if (t >= 0 && t < 1)
			{

				// ray is pointing to the right
				const float x = Math::Lerp(p0.x, p1.x, t);
				if (x > in_pt.x)
				{
					intersections++;
				}
			}
		}
	}

	return intersections % 2 == 1;
}


/////////////////////////////////////////////////////////////////////////////
// closest point queries

Vec2f Math::ClosestPointOnPolygon(const Polygon& in_poly, Vec2f in_pt, std::optional<ePolygonType> in_overridePolyType/*={}*/, int32_t* out_edgeIdx/*=nullptr*/, float* out_edgeT/*=nullptr*/)
{
	// simple algorithm that checks containment then gets the closest point among all the edges
	// not the most efficient, especially in physics sim contexts, but simple and supports concave polys

	float minDist = BIG_NUMBER;
	Vec2f closestPt = Vec2f::Zero;

	const ePolygonType polyType = in_overridePolyType.value_or(in_poly.GetType());

	if (polyType == ePolygonType::Solid)
	{
		if (PointInPolygon(in_poly, in_pt))
		{
			return in_pt;
		}
	}

	// chain polygons don't wrap around, so there's no edge between the first and last points
	const bool bWrap = polyType == ePolygonType::Solid || polyType == ePolygonType::Outline;

	const std::vector<Vec2f>& verts = in_poly.GetVerts();
	for (int i = 0; i < (bWrap ? verts.size() : verts.size() - 1); i++)
	{
		const Vec2f p0 = verts[i];
		const Vec2f p1 = verts[Polygon::NextIdx(i, verts.size())];

		float edgeT = -1;
		const Vec2f closestOnEdge = ClosestPointOnSegment(Segment{ p0, p1 }, in_pt, &edgeT);
		const float distToEdge = closestOnEdge.Dist(in_pt);

		if (distToEdge < minDist)
		{
			minDist = distToEdge;
			closestPt = closestOnEdge;

			if (out_edgeIdx) *out_edgeIdx = i;
			if (out_edgeT) *out_edgeT = edgeT;
		}
	}

	return closestPt;
}


/////////////////////////////////////////////////////////////////////////////
// polygon overlap query

std::optional<Vec2f> Math::OverlapConvexPolygons(const Polygon& in_poly0, const Polygon& in_poly1)
{
	// simple N^2 algorithm testing each face normal of each poly as a separating axis

	// the best separating vector is the one where the shapes interpenetrate the least
	// (if there's a vector where they don't interpenetrate at all, then they're not overlapping)
	float minPenetrationDist = INFINITY;
	Vec2f bestSeparatingVector = Vec2f::Zero;

	for (int edgesPolyIdx = 0; edgesPolyIdx <= 1; edgesPolyIdx++)
	{
		// do this twice - first test the points of poly1 against the edges of poly0, then vice versa

		const std::vector<Vec2f>& vs0 = (edgesPolyIdx == 0 ? in_poly0 : in_poly1).GetVerts();
		const std::vector<Vec2f>& vs1 = (edgesPolyIdx == 0 ? in_poly1 : in_poly0).GetVerts();
		const bool bEdgesCCW = (edgesPolyIdx == 0 ? in_poly0 : in_poly1).IsCCW();

		// for each edge in vs0...
		for (int i = 0; i < vs0.size(); i++)
		{
			// get the edge normal
			const Vec2f p0 = vs0[i];
			const Vec2f p1 = vs0[Polygon::NextIdx(i, vs0.size())];

			const Vec2f edgeDir = (p1 - p0).Norm();
			Vec2f edgeNormal = { edgeDir.y, -edgeDir.x }; // CW from edge, assuming poly is CCW
			if (!bEdgesCCW) edgeNormal *= -1; // if poly is actually CW, invert normal

			const Line normalLine{ p0, edgeNormal };

			// get the dist to each point in vs1 along this 
			// minSeparationDist: signed distance from the edge to the point. positive means the point is outside the edge's polygon
			float minSeparationDist = BIG_NUMBER;
			for (const Vec2f pt : vs1)
			{
				const float separationDist = normalLine.ProjectPointToLine_GetT(pt);
				minSeparationDist = Min(minSeparationDist, separationDist);
			}

			// if all points of vs1 lie on the outside of this edge of vs0, then the polygons are non-overlapping
			if (minSeparationDist > 0.0f)
			{
				return {};
			}
			else
			{
				// the polygons overlap on this axis. if they overlap less than on previously checked axes, then this axis is our new best candidate
				const float penetrationDist = -minSeparationDist;
				if (penetrationDist < minPenetrationDist)
				{
					minPenetrationDist = penetrationDist;
					bestSeparatingVector = edgeNormal * penetrationDist * (edgesPolyIdx == 0 ? 1.f : -1.f);
				}
			}
		}
	}

	// if we got here, we've checked all edge normals and they overlap on all of them, so the polygons must overlap
	return bestSeparatingVector;
}


/////////////////////////////////////////////////////////////////////////////
// line/ray/segment-primitive intersections

// internal helper for IntersectSegments and IntersectSegmentAndLine
// takes an override for the range of t accepted on in_seg1, allowing it to be treated as a ray/line
static std::optional<Vec2f> IntersectSegments_Internal(Math::Segment in_seg0, Math::Segment in_seg1, MinMaxf in_seg1TRange, float* out_t0 /*= nullptr*/, float* out_t1 /*= nullptr*/)
{
	// 2D specialization of Goldman, Graphics Gems p304
	// ref https://stackoverflow.com/a/565282

	const float dist0_numerator = (in_seg1.m_p0 - in_seg0.m_p0).Cross(in_seg1.GetDir());
	const float dist0_denom = in_seg0.GetDir().Cross(in_seg1.GetDir());

	// check if they're parallel 
	if (dist0_denom == 0.0f)
	{
		if (dist0_numerator == 0.0f)
		{
			// collinear

			// get the t values on seg0 for the endpoints of seg1
			const MinMaxf seg0Range = in_seg0.GetTRange();
			MinMaxf seg1Range = { in_seg0.ProjectPointToLine_GetT(in_seg1.m_p0), in_seg0.ProjectPointToLine_GetT(in_seg1.m_p1) };
			seg1Range.EnforceAscending();

			// check if seg1's t values intersect [0, 1]
			const std::optional<MinMaxf> tIntersection = seg0Range.Intersect(seg1Range);
			if (!tIntersection) return {};

			return in_seg0.GetPointFromT(tIntersection->Mid());
		}
		else
		{
			// non-intersecting
			return {};
		}
	}

	// lines intersect -> calculate the t values for the intersection on each segment, and make sure they actually lie on the segments

	const float dist0 = dist0_numerator / dist0_denom;
	const float t0 = dist0 / in_seg0.GetLength();
	if (!in_seg0.GetTRange().Contains_Incl(t0)) return {};

	const Vec2f lineIntersection = in_seg0.m_p0 + in_seg0.GetDir() * dist0;

	const float t1 = in_seg1.ProjectPointToLine_GetT(lineIntersection);
	if (!in_seg1TRange.Contains_Incl(t1)) return {};

	if (out_t0) *out_t0 = t0;
	if (out_t1) *out_t1 = t1;

	return lineIntersection;
}

std::optional<Vec2f> Math::IntersectSegments(Segment in_seg0, Segment in_seg1, float* out_t0 /*= nullptr*/, float* out_t1 /*= nullptr*/)
{
	return IntersectSegments_Internal(in_seg0, in_seg1, { 0, 1 }, out_t0, out_t1);
}

std::optional<Vec2f> Math::IntersectSegmentAndLine(Segment in_seg, Line in_line, float* out_t0 /*= nullptr*/, float* out_t1 /*= nullptr*/)
{
	return IntersectSegments_Internal(in_seg, Segment{ in_line.m_p0, in_line.m_p0 + in_line.m_normDir }, { -INFINITY, INFINITY }, out_t0, out_t1);
}

std::optional<Vec2f> Math::IntersectSegmentAndRay(Segment in_seg, Line in_line, float* out_t0 /*= nullptr*/, float* out_t1 /*= nullptr*/)
{
	return IntersectSegments_Internal(in_seg, Segment{ in_line.m_p0, in_line.m_p0 + in_line.m_normDir }, { 0.0f, INFINITY }, out_t0, out_t1);
}

std::vector<Vec2f> Math::IntersectLineAndPolygon(Line in_line, const Polygon& in_poly)
{
	if (PointInPolygon(in_poly, in_line.m_p0)) return { in_line.m_p0 };

	std::vector<Vec2f> ret;

	const auto& verts = in_poly.GetVerts();
	for (int i = 0; i < verts.size(); i++)
	{
		// get the edge normal
		const Vec2f p0 = verts[i];
		const Vec2f p1 = verts[Polygon::NextIdx(i, verts.size())];

		std::optional<Vec2f> intersection = IntersectSegmentAndLine(Segment{ p0, p1 }, in_line);

		if (intersection)
		{
			ret.push_back(*intersection);
		}
	}

	return ret;
}


/////////////////////////////////////////////////////////////////////////////
// raycasts and shape sweeps

std::optional<Math::RaycastResults> Math::CastRayAgainstPolygon(Ray in_ray, const Polygon& in_poly)
{
	// if ray starts inside, return immediately
	if (PointInPolygon(in_poly, in_ray.m_p0)) return RaycastResults(in_ray.m_p0, 0.0f, Vec2f::Zero);

	// intersect against each edge segment and return the hit with least t
	float minT = INFINITY;
	Vec2f firstIntersectionNormal = Vec2f::Zero;
	std::optional<Vec2f> firstIntersection;

	const auto& verts = in_poly.GetVerts();
	for (int i = 0; i < verts.size(); i++)
	{
		// get the edge normal
		const Vec2f p0 = verts[i];
		const Vec2f p1 = verts[Polygon::NextIdx(i, verts.size())];

		float rayIntersectT = -1;
		std::optional<Vec2f> intersection = IntersectSegmentAndRay(Segment{ p0, p1 }, in_ray, nullptr, &rayIntersectT);

		if (rayIntersectT >= 0.0f)
		{
			if (rayIntersectT < minT)
			{
				minT = rayIntersectT;
				firstIntersection = intersection;

				// if the polygon is CCW, then the edge normal will be the edge vector rotated 90 degrees
				// (we know we're coming from outside since we checked containment at the start)
				const Vec2f segmentDir = p1 - p0;
				Vec2f segmentNormal = { segmentDir.y, -segmentDir.x };

				// if the polygon isn't CCW, the normal will be inverted too
				if (!in_poly.IsCCW()) { segmentNormal = -segmentNormal; }

				firstIntersectionNormal = segmentNormal.Norm();
			}
		}
	}

	if (firstIntersection)
	{
		return RaycastResults(*firstIntersection, minT, firstIntersectionNormal);
	}

	return {};
}

std::optional<Math::RaycastResults> Math::CastRayAgainstBox(Ray in_ray, Box2f in_box)
// TODO: BROKEN, do not use -- should check for containment properly in the x- and y-aligned cases!
{
	// basic slab test (intersect the ray against the box's 6 faces and make sure that it enters before it exits)

	// each component is the intersection t for the ray and one of the faces
	const Vec2f tBoxMin = (in_box.GetMin() - in_ray.m_p0) / in_ray.GetDir();
	const Vec2f tBoxMax = (in_box.GetMax() - in_ray.m_p0) / in_ray.GetDir();

	// calculate min/max t of the segment and the box's face planes on each axis
	const MinMaxf tX = in_ray.GetDir().x == 0 ? MinMaxf{-INFINITY, INFINITY} : MinMaxf{ Min(tBoxMin.x, tBoxMax.x), Max(tBoxMin.x, tBoxMax.x) };
	const MinMaxf tY = in_ray.GetDir().y == 0 ? MinMaxf{-INFINITY, INFINITY} : MinMaxf{ Min(tBoxMin.y, tBoxMax.y), Max(tBoxMin.y, tBoxMax.y) };

	// t where the ray theoretically would enter and exit the box
	const float tEntry = Max3(tX.m_min, tY.m_min, 0.0f);
	const float tExit = Min(tX.m_max, tY.m_max);

	// ray hits box iff it enters before it exits
	// consider exact hits to be misses, so resolving a point onto the edge of the box counts it as no longer inside
	const bool bHit = tEntry < tExit;
	if (!bHit) return {};

	// return
	return RaycastResults(
		in_ray.GetPointFromT(tEntry), 
		tEntry, 
		tX.m_min > tY.m_min ? Vec2f{-Sign(in_ray.GetDir().x), 0} : Vec2f{0, -Sign(in_ray.GetDir().y)});
}

std::optional<Math::BoxSweepResults> Math::SweepBoxAgainstBox(Box2f in_boxToSweep, Vec2f in_sweepVec, Box2f in_boxToSweepAgainst)
{
	// check initial intersection
	{
		// for collision sweep purposes (to avoid getting stuck), count boxes that're exactly touching as nonintersecting at the start
		const auto Overlaps_Conservative = [](MinMaxf in_a, MinMaxf in_b) {
			return in_a.m_max > in_b.m_min && in_a.m_min < in_b.m_max;
		};

		if (   Overlaps_Conservative(in_boxToSweep.GetExtentsX(), in_boxToSweepAgainst.GetExtentsX())
			&& Overlaps_Conservative(in_boxToSweep.GetExtentsY(), in_boxToSweepAgainst.GetExtentsY()))
		{
			return BoxSweepResults(in_boxToSweep, 0.0f, Vec2f::Zero);
		}
	}

	// helper function that does a sweep in the specified axis as if the boxes were infinite in the other axis
	// returns unset optional on miss or on initial intersection
	// returns the displacement if there's a hit
	// call with swapped components to check y
	const auto SweepInAxis = [=](int32_t in_comp) -> std::optional<Vec2f> {
		// not moving in this axis
		if (in_sweepVec[in_comp] == 0.0f) return {};

		const bool bPositiveDir = in_sweepVec[in_comp] > 0;
		const float startComp = (bPositiveDir ? in_boxToSweep.GetMax() : in_boxToSweep.GetMin())[in_comp];
		const float endComp = (bPositiveDir ? in_boxToSweepAgainst.GetMin() : in_boxToSweepAgainst.GetMax())[in_comp];
		
		const float delta = endComp - startComp;

		// moving the away from collision in this axis 
		// (or initially overlapping in this axis, in which case the collision will happen on the other axis since we already checked for initial overlaps between the boxes)
		if (Sign(in_sweepVec[in_comp]) != Sign(delta) && delta != 0.0f) return {};

		const int32_t otherComp = in_comp == 0 ? 1 : 0;
		const float otherDelta = delta / in_sweepVec[in_comp] * in_sweepVec[otherComp];

		// swept displacement
		Vec2f deltaVec{};
		deltaVec[in_comp] = delta;
		deltaVec[otherComp] = otherDelta;

		return deltaVec;
	};

	// do a sweep in each axis
	std::optional<Vec2f> dispX = SweepInAxis(0);
	std::optional<Vec2f> dispY = SweepInAxis(1);

	// neither axis hits (eg moving away)
	if (!dispX && !dispY) return {};

	// figure out which axis is the real one
	int32_t hitAxis = -1;
	if (dispX && dispY)
	{
		// hits in both axes -> take the second (the first one will be beyond the box bounds in the other axis)
		hitAxis = dispX->Len() > dispY->Len() ? 0 : 1;
	}
	else
	{
		// hit in one axis -> just use that one
		hitAxis = dispX ? 0 : 1;
	}

	// get the hit displacement
	const Vec2f deltaVec = (hitAxis == 0 ? dispX : dispY).value();

	// the hit is beyond max dist
	if (deltaVec.Len() > in_sweepVec.Len()) return {};

	// calculate the swept box
	Box2f swept = in_boxToSweep;
	swept.SetCenter_Move(swept.GetCenter() + deltaVec);

	// verify that it actually hits (since the single-axis sweeps ignore the other axis)
	if (!IntersectBoxes(swept, in_boxToSweepAgainst)) return {};

	// if we got here, we have a hit!
	// calculate hit normal and return
	Vec2f hitNormal = Vec2f::Zero;
	hitNormal[hitAxis] = -Sign(deltaVec[hitAxis]);

	return BoxSweepResults(swept, deltaVec.Len(), hitNormal);
}

// casts a ray in grid space (grid cells are axis-aligned and 1x1 units in size)
// the template argument and in_bIsSolidFuncUsesFlooredGridPos arg are because normally users passing their own grid query functions will want floored Vec2i grid coordinates, but functions like SweepBoxAgainstGrid use this internally and need the exact Vec2f hit coords for their grid query functions
template<typename GridIsSolidFuncVecT>
static std::optional<Vec2f> CastRayGridSpace(Math::Ray in_ray, float in_maxDist, Transform in_gridToWorldTM, Box2i in_gridBoundsBox, std::function<bool(Vec2<GridIsSolidFuncVecT>)> in_gridCellIsSolidFunc, bool in_bIsSolidFuncUsesFlooredGridPos, Vec2f& out_hitNormal) {
	const auto StepAlongX = [](Math::Ray in_ray) -> Vec2f {
		if (in_ray.GetDir().x == 0.0f) return { BIG_NUMBER, BIG_NUMBER }; // return something far away so we won't use it

		const float x = in_ray.m_p0.x;
		const float y = in_ray.m_p0.y;

		const float dx = in_ray.GetDir().x > 0 ? std::floor(x + 1) - x : std::ceil(x - 1) - x;
		const float dy = dx * (in_ray.GetDir().y / in_ray.GetDir().x);
		
		Vec2f ret = in_ray.m_p0 + Vec2f{dx, dy};
		SQUID_RUNTIME_CHECK(ret.x == Math::Round(ret.x), "Float should always contain an integer here");
		//ret.x = Math::Round(ret.x);
		return ret;
	};

	const auto FloorToGridPos = [](Vec2f in_vec) {
		return Vec2i{(int32_t)std::floor(in_vec.x), (int32_t)std::floor(in_vec.y)};
	};

	const Box2f gridBoundsBoxFloat{(float)in_gridBoundsBox.x, (float)in_gridBoundsBox.y, (float)in_gridBoundsBox.w, (float)in_gridBoundsBox.h};

	//DrawDebugBox(gridBoundsBoxFloat.TransformedBy(in_gridToWorldTM), sf::Color::Green);

	bool bWasEverInGrid = in_gridBoundsBox.Contains_InclExcl(FloorToGridPos(in_ray.m_p0));

	// handle initial condition
	if (!bWasEverInGrid)
	{
		// if we're starting completely outside the grid, start by cast against the overall grid bounds 
		std::optional<Math::RaycastResults> hit = Math::CastRayAgainstBox(in_ray, gridBoundsBoxFloat);

		// if our ray will never intersect the grid, return no hit
		if (!hit || hit->m_dist > in_maxDist) return {};

		// if our ray will eventually hit the grid, "fast forward" to right before it hits (start before the hit to avoid the edge case)
		in_ray.m_p0 = hit->m_pos - in_ray.GetDir();
		
		//DrawDebugPoint(in_gridToWorldTM.TransformPoint(in_ray.m_p0), sf::Color::Cyan);
	}
	else
	{
		// helper function for checking if in_val is almost integral (allowing for float error)
		const auto IsOnGridline = [](float in_val) {
			return std::abs(in_val - Math::Round(in_val)) <= 0.0f;// KINDA_SMALL_NUMBER;
		};

		bool bFoundHit = false;
		Vec2f totalHitNormal = Vec2f::Zero;

		// handle the case where the initial point is exactly on a gridline on one or both axes
		// basically, along each axis, we want to check the 1 or 2 cells that we're currently in or moving into
		for (int mainAxisIdx = 0; mainAxisIdx <= 1; mainAxisIdx++)
		{
			// mainAxisIdx is the axis we're sweeping along, and otherAxisIdx is the perpendicular one
			const int otherAxisIdx = 1 - mainAxisIdx;
			const float dirInAxis = Math::Sign(in_ray.GetDir()[mainAxisIdx]);

			// if we're not moving in this axis, don't do any collision along it (to avoid getting stuck on corners)
			if (dirInAxis == 0) continue;

			// distance by which to adjust points that are exactly on gridlines 
			const float adjustDist = 0.01f;

			// p0 is the cell we're inside, or the one we're moving into if we're moving perp to a gridline
			Vec2f p0 = in_ray.m_p0;
			if (IsOnGridline(p0[mainAxisIdx]))
			{
				p0[mainAxisIdx] += dirInAxis * adjustDist;
			}
			// if we're moving along a gridline, p1 is the other cell we're potentially colliding with
			Vec2f p1 = p0;
			if (IsOnGridline(p0[otherAxisIdx]))
			{
				p0[otherAxisIdx] += adjustDist;
				p1[otherAxisIdx] -= adjustDist;
			}

			if (in_bIsSolidFuncUsesFlooredGridPos)
			{
				// calculate grid-snapped positions
				const Vec2i p0i = FloorToGridPos(p0);
				const Vec2i p1i = FloorToGridPos(p1);

				// if we're inside/hitting a wall on this axis, log the collision
				if (in_gridCellIsSolidFunc(p0i) && (p1i == p0i || in_gridCellIsSolidFunc(p1i)))
				{
					Vec2f hitNormal = Vec2f::Zero;
					hitNormal[mainAxisIdx] = -dirInAxis;

					totalHitNormal += hitNormal;
					bFoundHit = true;
				}
			}
			else
			{
				// if we're inside/hitting a wall on this axis, log the collision
				if (in_gridCellIsSolidFunc(p0) && (p1 == p0 || in_gridCellIsSolidFunc(p1)))
				{
					Vec2f hitNormal = Vec2f::Zero;
					hitNormal[mainAxisIdx] = -dirInAxis;

					totalHitNormal += hitNormal;
					bFoundHit = true;
				}
			}
		}

		// if we found any collisions, return
		if (bFoundHit)
		{
			out_hitNormal = totalHitNormal.Norm();
			//DrawDebugLine(in_ray.m_p0, in_ray.m_p0 + out_hitNormal * 1.5f, sf::Color::White, in_gridToWorldTM);

			return in_ray.m_p0;
		}
	}

	// casting loop
	while (true)
	{
		const Vec2f steppedX = StepAlongX(in_ray);
		Vec2f steppedY = StepAlongX({ {in_ray.m_p0.y, in_ray.m_p0.x}, {in_ray.GetDir().y, in_ray.GetDir().x} });
		steppedY = { steppedY.y, steppedY.x };

		const bool bSteppingInX = steppedX.Dist(in_ray.m_p0) < steppedY.Dist(in_ray.m_p0);
		const Vec2f stepped = bSteppingInX ? steppedX : steppedY;
		const float stepDist = stepped.Dist(in_ray.m_p0);

		// if this step would be beyond our max step distance, return no hit
		if (stepDist > in_maxDist)
		{
			return {};
		}

		// convert to grid coordinates
		// this is more than just flooring because our points are on cell boundaries, and when traveling in the negative direction. simply flooring will give us the next cell over
		Vec2<GridIsSolidFuncVecT> steppedGridPos{ 
			(GridIsSolidFuncVecT)(stepped.x) + (bSteppingInX && in_ray.GetDir().x < 0 ? -1 : 0), 
			(GridIsSolidFuncVecT)(stepped.y) + (!bSteppingInX && in_ray.GetDir().y < 0 ? -1 : 0)
		};

		// when GridIsSolidFuncVecT is int32_t, the cast will truncate, so we have to calculate this floored version separately
		Vec2<int32_t> flooredSteppedGridPos{ 
			(int32_t)std::floor(stepped.x) + (bSteppingInX && in_ray.GetDir().x < 0 ? -1 : 0), 
			(int32_t)std::floor(stepped.y) + (!bSteppingInX && in_ray.GetDir().y < 0 ? -1 : 0)
		};

		if (in_bIsSolidFuncUsesFlooredGridPos)
		{
			steppedGridPos = flooredSteppedGridPos;
		}

		// if stepped is inside the grid...
		if (in_gridBoundsBox.Contains_InclExcl(flooredSteppedGridPos))
		{
			// debug draw
			//DrawDebugLine(in_gridToWorldTM.TransformPoint(in_ray.m_p0), in_gridToWorldTM.TransformPoint(stepped));
			//DrawDebugPoint(in_gridToWorldTM.TransformPoint(stepped), sf::Color::Magenta);
			//const Box2f cellBox = Box2f::FromBottomLeft( steppedGridPos, {1.0f, 1.0f} ).TransformedBy(in_gridToWorldTM);
			//DrawDebugLine(cellBox.GetCenter(), in_gridToWorldTM.TransformPoint(stepped), sf::Color::Magenta);

			bWasEverInGrid = true;

			// if this grid cell is occupied, return the hit
			if (in_gridCellIsSolidFunc(steppedGridPos))
			{
				const Vec2f dir = in_ray.GetDir();
				out_hitNormal = bSteppingInX ? Vec2f{ -Math::Sign(dir.x), 0.0f } : Vec2f{ 0.0f, -Math::Sign(dir.y) };
				return stepped;
			}
		}
		else
		{
			// if we're not in the grid, but we were before, return no hit (since convexity means we'll never enter it again)
			if (bWasEverInGrid)
			{
				return {};
			}
		}

		// move our ray origin forward to the stepped position before continuing
		in_ray.m_p0 = stepped;
		in_maxDist -= stepDist;
	}
};

template<typename GridIsSolidFuncVecT>
static std::optional<Math::RaycastResults> IntersectRayAndGrid_Internal(Math::Ray in_ray, float in_maxDist, Transform in_gridToWorldTM, Box2i in_gridDimsBox, std::function<bool(Vec2<GridIsSolidFuncVecT>)> in_gridCellIsSolidFunc, bool in_bIsSolidFuncUsesFlooredGridPos)
{
	// rotated grids are 99% supported, but doesn't work with our float precision fixup hacks at the bottom of this function
	SQUID_RUNTIME_CHECK(in_gridToWorldTM.rot == 0.0f, "sweeping box against rotated grid is not supported");

	// transform ray and maxDist to grid coordinates
	const Math::Ray ray_gridCoords( in_gridToWorldTM.InvTransformPoint(in_ray.m_p0), in_gridToWorldTM.InvTransformVector(in_ray.GetDir()) );
	const float maxDist_gridCoords = in_gridToWorldTM.InvTransformVector(in_ray.GetDir() * in_maxDist).Len();


	// do the raycast in grid space
	Vec2f hitNormal_grid = Vec2f::Zero;
	const std::optional<Vec2f> hitPos = CastRayGridSpace<GridIsSolidFuncVecT>(ray_gridCoords, maxDist_gridCoords, in_gridToWorldTM, in_gridDimsBox, in_gridCellIsSolidFunc, in_bIsSolidFuncUsesFlooredGridPos, hitNormal_grid);
	if (!hitPos) return {};


	// transform normal back to world space 
	const Vec2f hitNormal_world = in_gridToWorldTM.TransformVector(hitNormal_grid).Norm();

	// HACK: transforming to/from grid space causes loss of precision. normally this is tiny enough to not be noticeable, but it's important to avoid drift when the collision is at distance 0 to avoid interpenetration
	if (*hitPos == ray_gridCoords.m_p0)
	{
		return Math::RaycastResults(in_ray.m_p0, 0.0f, hitNormal_world);
	}


	// transform hit pos and dist back to world space
	Vec2f hitPos_world = in_gridToWorldTM.TransformPoint(*hitPos);

	//HACK: because of precision loss in the grid conversion, our collision might be slightly off, so do some fixup
	{
		// FIXME: this does not guarantee that the fixed up position is along the ray direction.
		// We apply the clamp-to-edge constraint separately since it's generally more important.
		// But this means that the hit distance is inconsistent with the hit position.
		// The best fix for this is probably to make CastRayGridSpace work in world space and remove these hacks, rather than adding more.

		// fix up hitpos_world to always be exactly along the ray direction
		hitPos_world = in_ray.m_p0 + Math::Project(hitPos_world - in_ray.m_p0, in_ray.m_normDir);

		// fix up hitpos to have the relevant axis/axes exactly on the world space grid line
		// assumes 0 grid rotation
		const auto IsOnGridline = [](float in_val) {
			return std::abs(in_val - Math::Round(in_val)) <= 0.0f;
		};

		for (int i = 0; i <= 1; i++)
		{
			if (IsOnGridline((*hitPos)[i]) || hitNormal_grid[i] != 0.0f)
			{
				hitPos_world[i] = Math::RoundToMultiple(hitPos_world[i] - in_gridToWorldTM.pos[i], in_gridToWorldTM.scale[i]) + in_gridToWorldTM.pos[i];

				SQUID_RUNTIME_CHECK(IsOnGridline(hitPos_world[i]), "hitPos should always be on a gridline");
			}
		}
	}


	const float hitDist_world = hitPos_world.Dist(in_ray.m_p0);

	return Math::RaycastResults(hitPos_world, hitDist_world, hitNormal_world);
}

std::optional<Math::RaycastResults> Math::CastRayAgainstGrid(Ray in_ray, float in_maxDist, Transform in_gridToWorldTM, Box2i in_gridDimsBox, std::function<bool(Vec2i)> in_gridCellIsSolidFunc)
{
	return IntersectRayAndGrid_Internal<int32_t>(in_ray, in_maxDist, in_gridToWorldTM, in_gridDimsBox, in_gridCellIsSolidFunc, true);
}

std::optional<Math::BoxSweepResults> Math::SweepBoxAgainstGrid(Box2f in_box, Vec2f in_sweepVec, Transform in_gridToWorldTM, Box2i in_gridDimsBox, std::function<bool(Vec2i)> in_gridCellIsSolidFunc)
{
	// the general idea is to use IntersectRayAndGrid, but with the ray starting at the box corner most in the sweep direction and with a grid query function that checks all the cells on the leading box edges

	// this check relies on the box and grid being aligned, so rotated grids aren't supported
	SQUID_RUNTIME_CHECK(in_gridToWorldTM.rot == 0.0f, "sweeping box against rotated grid is not supported");


	//DrawDebugBox(in_box, sf::Color::Magenta);


	const Vec2f sweepDir = in_sweepVec.Norm();

	const Vec2f leadingCorner{ 
		sweepDir.x > 0 ? in_box.GetRight() : in_box.GetLeft(),
		sweepDir.y > 0 ? in_box.GetTop() : in_box.GetBottom(),
	};

	// TODO check that the initial box isn't already in collision

	const Box2f box_gridCoords = in_box.TransformedBy(in_gridToWorldTM.Inverse());

	const auto checkLeadingEdges = [in_gridToWorldTM, sweepDir, box_gridCoords, in_gridCellIsSolidFunc](Vec2f in_cornerGridPos) -> bool {
		bool bRet = false;

		if (sweepDir.y != 0.0f)
		{
			for (int x = 0; x <= (int)std::ceil(box_gridCoords.GetDims().x); x++)
			{
				float ax = Math::Min((float)x, box_gridCoords.GetDims().x - KINDA_SMALL_NUMBER);
				const Vec2f curGridPos = in_cornerGridPos + Vec2f{ax + (sweepDir.x > 0 ? -(box_gridCoords.GetDims().x) : 0), 0};

				//DrawDebugPoint(in_gridToWorldTM.TransformPoint(curGridPos), sf::Color::Magenta);

				if (in_gridCellIsSolidFunc(Vec2i{ (int32_t)std::floor(curGridPos.x), (int32_t)std::floor(curGridPos.y) })) bRet = true;
			}
		}

		if (sweepDir.x != 0.0f)
		{
			for (int y = 0; y <= (int)std::ceil(box_gridCoords.GetDims().y); y++)
			{
				float ay = Math::Min((float)y, box_gridCoords.GetDims().y - KINDA_SMALL_NUMBER);
				const Vec2f curGridPos = in_cornerGridPos + Vec2f{0, ay + (sweepDir.y > 0 ? -(box_gridCoords.GetDims().y) : 0)};

				//DrawDebugPoint(in_gridToWorldTM.TransformPoint(curGridPos), sf::Color::Cyan);

				if (in_gridCellIsSolidFunc(Vec2i{ (int32_t)std::floor(curGridPos.x), (int32_t)std::floor(curGridPos.y) })) bRet = true;
			}
		}

		return bRet;
	};

	std::optional<RaycastResults> cornerHitRes = IntersectRayAndGrid_Internal<float>(Ray(leadingCorner, sweepDir), in_sweepVec.Len(), in_gridToWorldTM, in_gridDimsBox, checkLeadingEdges, false);

	if (!cornerHitRes) return {};

	const Box2f endBox = Box2f::FromAnchorPos({
		sweepDir.x > 0 ? 1.0f : 0.0f,
		sweepDir.y > 0 ? 1.0f : 0.0f,
	}, cornerHitRes->m_pos, in_box.GetDims());


	//DrawDebugBox(endBox, sf::Color::Magenta);
	//Polygon startPoly = in_box.ToPolygon();
	//Polygon endPoly = endBox.ToPolygon();
	//for (int i = 0; i < startPoly.GetVerts().size(); i++)
	//{
	//	DrawDebugLine(startPoly.GetVerts()[i], endPoly.GetVerts()[i], sf::Color::Magenta);
	//}


	return BoxSweepResults(endBox, cornerHitRes->m_dist, cornerHitRes->m_normal);
}

std::optional<Vec2f> Math::IntersectLines(Line in_line0, Line in_line1)
{
	// 2D specialization of Goldman, Graphics Gems p304
	// ref https://stackoverflow.com/a/565282

	const float t0_numerator = (in_line1.m_p0 - in_line0.m_p0).Cross(in_line1.GetDir());
	const float t0_denom = in_line0.GetDir().Cross(in_line1.GetDir());

	// check if they're parallel 
	if (t0_denom == 0.0f)
	{
		if (t0_numerator == 0.0f)
		{
			// collinear -> return any point we want
			return in_line0.m_p0;
		}
		else
		{
			// non-intersecting
			return {};
		}
	}

	return in_line0.GetPointFromT(t0_numerator / t0_denom);
}

