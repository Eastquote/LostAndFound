#pragma once

#include <cmath>
#include <utility>
#include <stdint.h>

// Pi Constant
#define M_PI 3.14159265358979323846

template <typename T>
class Vec2
{
public:
	// Storage
	union
	{
		struct
		{
			T x, y;
		};
		T vals[2];
	};

	// Index operator
	float& operator[](size_t in_idx)
	{
		return vals[in_idx];
	}
	const float& operator[](size_t in_idx) const
	{
		return vals[in_idx];
	}

	// Identity vectors
	static const Vec2<T> Zero;
	static const Vec2<T> One;
	static const Vec2<T> Up;
	static const Vec2<T> Down;
	static const Vec2<T> Left;
	static const Vec2<T> Right;

	// Conversion
	template <typename U>
	operator Vec2<U>() const
	{
		return { (U)x, (U)y };
	}

	// Comparison
	bool operator==(const Vec2<T>& in_rhs) const
	{
		return x == in_rhs.x && y == in_rhs.y;
	}
	bool operator!=(const Vec2<T>& in_rhs) const
	{
		return x != in_rhs.x || y != in_rhs.y;
	}

	// Arithmetic
	Vec2<T> operator+(const Vec2<T>& in_rhs) const
	{
		Vec2<T> ret = *this;
		ret += in_rhs;
		return std::move(ret);
	}
	Vec2<T>& operator+=(const Vec2<T>& in_rhs)
	{
		x += in_rhs.x;
		y += in_rhs.y;
		return *this;
	}
	Vec2<T> operator-(const Vec2<T>& in_rhs) const
	{
		Vec2<T> ret = *this;
		ret -= in_rhs;
		return std::move(ret);
	}
	Vec2<T>& operator-=(const Vec2<T>& in_rhs)
	{
		x -= in_rhs.x;
		y -= in_rhs.y;
		return *this;
	}
	Vec2<T> operator*(T in_scalar) const
	{
		Vec2<T> ret = *this;
		ret *= in_scalar;
		return std::move(ret);
	}
	Vec2<T>& operator*=(T in_scalar)
	{
		x *= in_scalar;
		y *= in_scalar;
		return *this;
	}
	Vec2<T> operator*(Vec2<T> in_rhs) const
	{
		Vec2<T> ret = *this;
		ret *= in_rhs;
		return std::move(ret);
	}
	Vec2<T>& operator*=(Vec2<T> in_rhs)
	{
		x *= in_rhs.x;
		y *= in_rhs.y;
		return *this;
	}
	Vec2<T> operator/(T in_scalar) const
	{
		Vec2<T> ret = *this;
		ret /= in_scalar;
		return std::move(ret);
	}
	Vec2<T>& operator/=(T in_scalar)
	{
		x /= in_scalar;
		y /= in_scalar;
		return *this;
	}
	Vec2<T> operator/(Vec2<T> in_rhs) const
	{
		Vec2<T> ret = *this;
		ret /= in_rhs;
		return std::move(ret);
	}
	Vec2<T>& operator/=(Vec2<T> in_rhs)
	{
		x /= in_rhs.x;
		y /= in_rhs.y;
		return *this;
	}
	Vec2<T> operator-() const
	{
		 return { -x, -y };
	}

	// Linear Algebra
	Vec2<T> Ortho() const // Returns orthogonal vector (90-degrees CW)
	{
		return { y, -x };
	}
	Vec2<T> Inverse() const
	{
		return { 1 / x,  1 / y };
	}
	float Dot(const Vec2<T>& in_rhs) const
	{
		return x * in_rhs.x + y * in_rhs.y;
	}
	float Cross(const Vec2<T>& in_rhs) const
	{
		return x * in_rhs.y - y * in_rhs.x;
	}

	// Length
	Vec2<T> Norm() const
	{
		const float len = Len();
		if (len == 0.0f) return Zero;
		return *this / len;
	}
	float Len() const
	{
		return std::sqrt(Dot(*this));
	}
	float Dist(const Vec2<T>& in_rhs) const
	{
		return (*this - in_rhs).Len();
	}
	Vec2<T> Trunc(float in_len) const
	{
		return Norm() * in_len;
	}

	// Trigonometry
	T UnsignedAngleDeg() const
	{
		return std::abs(SignedAngleDeg());
	}
	T SignedAngleDeg() const
	{
		return (T)(std::atan2(y, x) / M_PI * 180.0);
	}
	T UnsignedAngleDeg(const Vec2<T>& in_rhs) const
	{
		return std::abs(SignedAngleDeg(in_rhs));
	}
	T SignedAngleDeg(const Vec2<T>& in_rhs) const
	{
		return (T)((std::atan2(in_rhs.y, in_rhs.x) - std::atan2(y, x)) / M_PI * 180.0);
	}
	Vec2<T> RotateDeg(T in_deg) const
	{
		Vec2<T> ret;
		float theta = (float)(in_deg / 180.0 * M_PI);
		float cosTheta = (float)std::cos(theta);
		float sinTheta = (float)std::sin(theta);
		ret.x = (T)(x * cosTheta - y * sinTheta);
		ret.y = (T)(x * sinTheta + y * cosTheta);
		return std::move(ret);
	}
};

// Vector type aliases
using Vec2f = Vec2<float>;
using Vec2d = Vec2<double>;
using Vec2i = Vec2<int32_t>;
using Vec2u = Vec2<uint32_t>;
