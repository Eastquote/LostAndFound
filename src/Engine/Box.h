#pragma once

#include "Vec2.h"
#include "MathCore.h"
#include "MinMax.h"
#include "Polygon.h"
#include "Transform.h"

template <typename T>
struct BoxT
{
	T x, y, w, h;

	// enable conversion if we're not converting nonint->int (if we're converting from int, or to non-int)
	template <typename U, typename TU = T, typename std::enable_if_t<std::is_integral<TU>::value || !std::is_integral<U>::value>* = nullptr>
	operator BoxT<U>() const
	{
		return BoxT<U>{ (U)x, (U)y, (U)w, (U)h };
	}

	// basic factories
	static BoxT<T> FromCorners(Vec2<T> in_minPt, Vec2<T> in_maxPt) { 
		Vec2<T> dims = in_maxPt - in_minPt;
		return BoxT<T>{ in_minPt.x, in_minPt.y, dims.x, dims.y }; 
	}
	static BoxT<T> FromAxisExtents(MinMaxT<T> in_xExtents, MinMaxT<T> in_yExtents) { 
		return BoxT<T>{ in_xExtents.m_min, in_yExtents.m_min, in_xExtents.Size(), in_yExtents.Size() };
	}

	// factories from a given position on the box and dims
	static BoxT<T> FromBottomLeft(Vec2<T> in_pos, Vec2<T> in_dims)		{ return FromAnchorPos({0,		0}, in_pos, in_dims); }
	static BoxT<T> FromBottomCenter(Vec2<T> in_pos, Vec2<T> in_dims)	{ return FromAnchorPos({0.5,	0}, in_pos, in_dims); }
	static BoxT<T> FromBottomRight(Vec2<T> in_pos, Vec2<T> in_dims)		{ return FromAnchorPos({1,		0}, in_pos, in_dims); }
	static BoxT<T> FromCenterLeft(Vec2<T> in_pos, Vec2<T> in_dims)		{ return FromAnchorPos({0,		0.5}, in_pos, in_dims); }
	static BoxT<T> FromCenter(Vec2<T> in_pos, Vec2<T> in_dims)			{ return FromAnchorPos({0.5,	0.5}, in_pos, in_dims); }
	static BoxT<T> FromCenterRight(Vec2<T> in_pos, Vec2<T> in_dims)		{ return FromAnchorPos({1,		0.5}, in_pos, in_dims); }
	static BoxT<T> FromTopLeft(Vec2<T> in_pos, Vec2<T> in_dims)			{ return FromAnchorPos({0,		1}, in_pos, in_dims); }
	static BoxT<T> FromTopCenter(Vec2<T> in_pos, Vec2<T> in_dims)		{ return FromAnchorPos({0.5,	1}, in_pos, in_dims); }
	static BoxT<T> FromTopRight(Vec2<T> in_pos, Vec2<T> in_dims)		{ return FromAnchorPos({1,		1}, in_pos, in_dims); }

	static BoxT<T> FromAnchorPos(Vec2<float> in_anchorNormPos, Vec2<T> in_worldPos, Vec2<T> in_dims) {
		BoxT<T> Box;
		// have to do this multiplication explicitly because the Vec2i operator* will cast to int
		Vec2<T> minOffset = { (T)(in_dims.x * in_anchorNormPos.x), (T)(in_dims.y * in_anchorNormPos.y) };
		Box.SetMin(in_worldPos - minOffset);
		Box.SetDims(in_dims);
		return Box;
	}

	// basic setter/getters
	void SetMin(Vec2<T> in_min) { 
		x = in_min.x; 
		y = in_min.y; 
	}
	void SetMax(Vec2<T> in_max) { 
		w = in_max.x - x;
		h = in_max.y - y;
	}  
	void SetDims(Vec2<T> in_dims) { 
		w = in_dims.x;
		h = in_dims.y;
	}
	Vec2<T> GetMin() const			{ return { x, y }; }
	Vec2<T> GetMax() const			{ return { x + w, y + h}; }
	Vec2<T> GetDims() const			{ return { w, h }; }
	MinMaxT<T> GetExtentsX() const	{ return { x, x + w }; }
	MinMaxT<T> GetExtentsY() const	{ return { y, y + h }; }

	// get specific points on the box
	Vec2<T> GetBottomLeft() const	{ return GetMin(); }
	Vec2<T> GetBottomCenter() const	{ return Lerp(0.5,	0); }
	Vec2<T> GetBottomRight() const	{ return Lerp(1,	0); }
	Vec2<T> GetCenterLeft() const	{ return Lerp(0,	0.5); }
	Vec2<T> GetCenter() const		{ return Lerp(0.5,	0.5); }
	Vec2<T> GetCenterRight() const	{ return Lerp(1,	0.5); }
	Vec2<T> GetTopLeft() const		{ return Lerp(0,	1); }
	Vec2<T> GetTopCenter() const	{ return Lerp(0.5,	1); }
	Vec2<T> GetTopRight() const		{ return GetMax(); }

	T GetLeft() const { return x; }
	T GetRight() const { return x + w; }
	T GetBottom() const { return y; }
	T GetTop() const { return y + h; }

	Vec2<T> Lerp(float in_x, float in_y) const { return Lerp(Vec2<float>{in_x, in_y}); }
	Vec2<T> Lerp(Vec2<float> in_normPos) const {
		in_normPos = Math::ClampVector(in_normPos, Vec2<float>::Zero, Vec2<float>::One);
		return Math::Lerp(GetMin(), GetMax(), in_normPos); 
	}

	// point setters
	void SetBottomLeft_Move(Vec2<T> in_pos)		{ SetAnchorPos_Move({ 0,	0 }, in_pos); }
	void SetBottomCenter_Move(Vec2<T> in_pos)	{ SetAnchorPos_Move({ 0.5,	0 }, in_pos); }
	void SetBottomRight_Move(Vec2<T> in_pos)	{ SetAnchorPos_Move({ 1,	0 }, in_pos); }
	void SetCenterLeft_Move(Vec2<T> in_pos)		{ SetAnchorPos_Move({ 0,	0.5 }, in_pos); }
	void SetCenter_Move(Vec2<T> in_pos)			{ SetAnchorPos_Move({ 0.5,	0.5 }, in_pos); }
	void SetCenterRight_Move(Vec2<T> in_pos)	{ SetAnchorPos_Move({ 1,	0.5 }, in_pos); }
	void SetTopLeft_Move(Vec2<T> in_pos)		{ SetAnchorPos_Move({ 0,	1 }, in_pos); }
	void SetTopCenter_Move(Vec2<T> in_pos)		{ SetAnchorPos_Move({ 0.5,	1 }, in_pos); }
	void SetTopRight_Move(Vec2<T> in_pos)		{ SetAnchorPos_Move({ 1,	1 }, in_pos); }

	void SetAnchorPos_Move(Vec2<T> in_anchorNormPos, Vec2<T> in_worldPos)	{
		*this = FromAnchorPos(in_anchorNormPos, in_worldPos, GetDims());
	}

	void SetBottomLeft_Resize(Vec2<T> in_pos)	{ SetAnchorPos_Resize({ 0,		0 }, in_pos); }
	void SetBottomCenter_Resize(Vec2<T> in_pos)	{ SetAnchorPos_Resize({ 0.5,	0 }, in_pos); }
	void SetBottomRight_Resize(Vec2<T> in_pos)	{ SetAnchorPos_Resize({ 1,		0 }, in_pos); }
	void SetCenterLeft_Resize(Vec2<T> in_pos)	{ SetAnchorPos_Resize({ 0,		0.5 }, in_pos); }
	//void SetCenter_Resize(Vec2<T> in_pos)		{ SetAnchorPos_Resize({ 0.5,	0.5 }, in_pos); }
	void SetCenterRight_Resize(Vec2<T> in_pos)	{ SetAnchorPos_Resize({ 1,		0.5 }, in_pos); }
	void SetTopLeft_Resize(Vec2<T> in_pos)		{ SetAnchorPos_Resize({ 0,		1 }, in_pos); }
	void SetTopCenter_Resize(Vec2<T> in_pos)	{ SetAnchorPos_Resize({ 0.5,	1 }, in_pos); }
	void SetTopRight_Resize(Vec2<T> in_pos)		{ SetAnchorPos_Resize({ 1,		1 }, in_pos); }

	void SetAnchorPos_Resize(Vec2<T> in_anchorNormPos, Vec2<T> in_worldPos) {
		SetAnchorPos_Resize_Component(0, in_anchorNormPos.X, in_worldPos.X);
		SetAnchorPos_Resize_Component(1, in_anchorNormPos.Y, in_worldPos.Y);
	}

	void SetDims_BottomLeftAnchor(Vec2<T> in_dims)		{ SetDims_Anchor({ 0,		0 }, in_dims); }
	void SetDims_BottomCenterAnchor(Vec2<T> in_dims)	{ SetDims_Anchor({ 0.5,		0 }, in_dims); }
	void SetDims_BottomRightAnchor(Vec2<T> in_dims)		{ SetDims_Anchor({ 1,		0 }, in_dims); }
	void SetDims_CenterLeftAnchor(Vec2<T> in_dims)		{ SetDims_Anchor({ 0,		0.5 }, in_dims); }
	void SetDims_CenterAnchor(Vec2<T> in_dims)			{ SetDims_Anchor({ 0.5,		0.5 }, in_dims); }
	void SetDims_CenterRightAnchor(Vec2<T> in_dims)		{ SetDims_Anchor({ 1,		0.5 }, in_dims); }
	void SetDims_TopLeftAnchor(Vec2<T> in_dims)			{ SetDims_Anchor({ 0,		1 }, in_dims); }
	void SetDims_TopCenterAnchor(Vec2<T> in_dims)		{ SetDims_Anchor({ 0.5,		1 }, in_dims); }
	void SetDims_TopRightAnchor(Vec2<T> in_dims)		{ SetDims_Anchor({ 1,		1 }, in_dims); }

	void SetDims_Anchor(Vec2<T> in_anchorNormPos, Vec2<T> in_dims) {
		*this = FromAnchorPos(in_anchorNormPos, Lerp(in_anchorNormPos), in_dims);
	}

	Polygon ToPolygon() const {
		return Polygon{{ 
			GetBottomLeft(),
			GetBottomRight(),
			GetTopRight(),
			GetTopLeft(),
		}, ePolygonType::Solid};
	}

	BoxT<float> TransformedBy(Transform in_tm) const {
		return BoxT<float>::FromCorners(in_tm.TransformPoint(GetMin()), in_tm.TransformPoint(GetMax()));
	}

	bool Contains(Vec2<T> in_val) const {
		return GetExtentsX().Contains_Incl(in_val.x) && GetExtentsY().Contains_Incl(in_val.y);
	}
	bool Contains_Excl(Vec2<T> in_val) const {
		return GetExtentsX().Contains_Excl(in_val.x) && GetExtentsY().Contains_Excl(in_val.y);
	}  
	bool Contains_InclExcl(Vec2<T> in_val) const {
		return GetExtentsX().Contains_InclExcl(in_val.x) && GetExtentsY().Contains_InclExcl(in_val.y);
	}

	bool Overlaps(BoxT<T> in_other) const {
		std::optional<MinMaxf> xIntersect = GetExtentsX().Intersect(in_other.GetExtentsX());
		if (!xIntersect) return false;

		std::optional<MinMaxf> yIntersect = GetExtentsY().Intersect(in_other.GetExtentsY());
		if (!yIntersect) return false;

		return true;
	}

	void ExpandToInclude(Vec2<T> in_val)	{
		x = Math::Min(x, in_val.x);
		y = Math::Min(y, in_val.y);
		w = Math::Max(w, in_val.x - x);
		h = Math::Max(h, in_val.y - y);
	}

private:
	// internal per-component part of SetAnchorPos_Resize. not for general use!
	void SetAnchorPos_Resize_Component(int32_t in_componentIdx, float in_anchorNormPos, float in_worldPos) {
		if (in_anchorNormPos == 0.5f) return;
		if (in_anchorNormPos == 0)
		{
			if (in_componentIdx == 0)
			{
				x = in_worldPos;
			}
			else
			{
				y = in_worldPos;
			}
		}
		else if (in_anchorNormPos == 1)
		{
			if (in_componentIdx == 0)
			{
				w = in_worldPos - x;
			}
			else
			{
				h = in_worldPos - y;
			}
		}
		else
		{
			// for simplicity, this function only supports setting anchor points 0, 0.5, or 1
			//ensure(false);
		}
	}
};

using Box2i = BoxT<int32_t>;
using Box2u = BoxT<uint32_t>;
using Box2f = BoxT<float>;
using Box2d = BoxT<double>;
