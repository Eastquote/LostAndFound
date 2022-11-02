#pragma once

#include "Engine/Vec2.h"

template <typename T>
struct TransformT
{
	// Apply transformation in SRT Order (Scale -> Rotate -> Translate)
	Vec2<T> pos = Vec2<T>::Zero;
	T rot = 0;
	Vec2<T> scale = Vec2<T>::One;

	static const TransformT<T> Identity;

	template <typename U>
	TransformT<T> operator*(const TransformT<U>& in_rhs) const
	{
		return {
			in_rhs.TransformPoint(pos),
			rot + in_rhs.rot,
			scale * in_rhs.scale
		};
	}
	TransformT<T> Inverse() const
	{
		return { InvTransformVector(-pos), -rot, scale.Inverse() };
	}

	// Transformations
	template <typename U>
	Vec2<U> TransformVector(const Vec2<U>& in_v) const // Linear transform
	{
		return (in_v * scale).RotateDeg((U)rot);
	}
	template <typename U>
	Vec2<U> TransformPoint(const Vec2<U>& in_p) const // Affine transform
	{
		return TransformVector(in_p) + pos;
	}
	template <typename U>
	Vec2<U> InvTransformVector(const Vec2<U>& in_v) const
	{
		auto v = in_v.RotateDeg((U)-rot);
		return v / scale;
	}
	template <typename U>
	Vec2<U> InvTransformPoint(const Vec2<U>& in_p) const
	{
		return InvTransformVector(in_p - pos);
	}
};
using Transform = TransformT<float>;
using TransformD = TransformT<double>;
