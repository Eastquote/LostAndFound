#pragma once

#include <cmath>
#include "Vec2.h"

/*
MathCore: 
- basic operations that aren't in std (and nicer versions of some that are kinda there)
- range remapping and lerp
*/ 

#define SMALL_NUMBER		(1.e-8f)
#define KINDA_SMALL_NUMBER	(1.e-4f)
#define BIG_NUMBER			(3.4e+38f)

namespace Math
{
	/////////////////////////////////////////////////////////////////////////////////
	// misc convenience and basic math functions
	/////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	T Sqr(T in_val) { return in_val * in_val; }
	
	template<typename T> 
	T Sign(T in_val) {
		return (T)((in_val > T(0)) - (in_val < T(0)));
	}

	template<typename ValT, typename BaseT>
	static Vec2<ValT> Sign(Vec2<ValT> in_val) { 
		return {
			Sign(in_val.x),
			Sign(in_val.y)
		}; 
	}

	template<typename T>
	T Max(T in_a, T in_b) { return (in_a > in_b) ? in_a : in_b;	}

	template<typename T>
	T Min(T in_a, T in_b) { return (in_a < in_b) ? in_a : in_b; }

	template<typename T>
	static Vec2<T> Min(Vec2<T> in_a, Vec2<T> in_b) { return { Min(in_a.x, in_b.x), Min(in_a.y, in_b.y) }; }

	template<typename T>
	static Vec2<T> Max(Vec2<T> in_a, Vec2<T> in_b) { return { Max(in_a.x, in_b.x), Max(in_a.y, in_b.y) }; }

	template<typename T> 
	T Max3(T in_a, T in_b, T in_c )
	{
		return Max(Max(in_a, in_b), in_c);
	}

	template<typename T> 
	T Min3(T in_a, T in_b, T in_c)
	{
		return Min(Min(in_a, in_b), in_c);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// floor/ceil/round
	// floor/ceil go towards negative/positive infinity by default. use FloorTowardsZero/CeilAwayFromZero if you want those behaviors
	/////////////////////////////////////////////////////////////////////////////////

	template<typename T>
	static T Floor(T in_val) { return std::floor(in_val); }

	template<typename T>
	static T Ceil(T in_val) { return std::ceil(in_val); }

	template<typename T>
	static T Round(T in_val) { return std::floor((float)in_val + 0.5f); }


	template<typename T>
	static Vec2<T> Floor(Vec2<T> in_val) { return { Floor(in_val.x), Floor(in_val.y) }; }

	template<typename T>
	static Vec2<T> Ceil(Vec2<T> in_val) { return { Ceil(in_val.x), Ceil(in_val.y) }; }

	template<typename T>
	static Vec2<T> Round(Vec2<T> in_val) { return { Round(in_val.x), Round(in_val.y) }; }


	template<typename T>
	static T FloorTowardsZero(T in_val) { return in_val >= 0.0f ? std::floor(in_val) : std::ceil(in_val); }

	template<typename T>
	static T CeilAwayFromZero(T in_val) { return in_val < 0.0f ? std::floor(in_val) : std::ceil(in_val); }


	template<typename T>
	static Vec2<T> FloorTowardsZero(Vec2<T> in_val) { return { FloorTowardsZero(in_val.x), FloorTowardsZero(in_val.y) }; }

	template<typename T>
	static Vec2<T> CeilAwayFromZero(Vec2<T> in_val) { return { CeilAwayFromZero(in_val.x), CeilAwayFromZero(in_val.y) }; }


	template<typename T>
	static int32_t FloorToInt(T in_val) { return (int32_t)std::floor(in_val); }

	template<typename T>
	static int32_t CeilToInt(T in_val) { return (int32_t)std::ceil(in_val); }

	template<typename T>
	static int32_t RoundToInt(T in_val) { return (int32_t)std::floor((float)in_val + 0.5f); }
	

	template<typename ValT, typename BaseT>
	static ValT FloorToMultiple(ValT in_val, BaseT Base) {
		return Floor(in_val / Base) * Base;
	}

	template<typename ValT, typename BaseT>
	static ValT CeilToMultiple(ValT in_val, BaseT Base) {
		return Ceil(in_val / Base) * Base;
	}

	template<typename ValT, typename BaseT>
	static ValT RoundToMultiple(ValT in_val, BaseT Base) {
		return Round(in_val / Base) * Base;
	}


	template<typename ValT, typename BaseT>
	static Vec2<ValT> FloorToMultiple(Vec2<ValT> in_val, BaseT Base) { 
		return {
			FloorToMultiple(in_val.x, Vec2<ValT>(Base).x),
			FloorToMultiple(in_val.y, Vec2<ValT>(Base).y)
		}; 
	}
	template<typename ValT, typename BaseT>
	static Vec2<ValT> CeilToMultiple(Vec2<ValT> in_val, BaseT Base) { 
		return {
			CeilToMultiple(in_val.x, Vec2<ValT>(Base).x),
			CeilToMultiple(in_val.y, Vec2<ValT>(Base).y)
		}; 
	}
	template<typename ValT, typename BaseT>
	static Vec2<ValT> RoundToMultiple(Vec2<ValT> in_val, BaseT Base) {
		return {
			RoundToMultiple(in_val.x, Vec2<ValT>(Base).x),
			RoundToMultiple(in_val.y, Vec2<ValT>(Base).y)
		};
	}

	/////////////////////////////////////////////////////////////////////////////////
	// abs
	/////////////////////////////////////////////////////////////////////////////////

	template<typename T>
	static T Abs(T in_val) { return std::abs(in_val); }

	template<typename ValT, typename BaseT>
	static Vec2<ValT> Abs(Vec2<ValT> in_val) { 
		return {
			Abs(in_val.x),
			Abs(in_val.y)
		}; 
	}

	/////////////////////////////////////////////////////////////////////////////////
	// degrees <-> radians
	/////////////////////////////////////////////////////////////////////////////////
	inline double RadiansToDegrees(float in_radians)
	{ 
		return in_radians * (180.0f / M_PI); 
	}

	inline double DegreesToRadians(float in_degrees)
	{ 
		return in_degrees * (M_PI / 180.0f); 
	}

	/////////////////////////////////////////////////////////////////////////////////
	// vec2fs <-> degrees
	/////////////////////////////////////////////////////////////////////////////////

	inline double VecToDegrees(Vec2f in_vector)
	{
		return RadiansToDegrees(atan2f(in_vector.y, in_vector.x));
	}
	inline Vec2f DegreesToVec(float in_degrees) // returns a *normalized* Vec2f, natch
	{
		return Vec2f{ cos(float(DegreesToRadians(in_degrees))), sin(float(DegreesToRadians(in_degrees))) };
	}
	/////////////////////////////////////////////////////////////////////////////////
	// wrap
	/////////////////////////////////////////////////////////////////////////////////
	// wraps to [Min, Max)
	inline float Wrap(float in_val, float in_min, float in_max) 
	{
		return fmod(fmod(in_val - in_min, in_max - in_min) + (in_max - in_min), (in_max - in_min)) + in_min;
	}

	// https://stackoverflow.com/questions/14415753/wrap-value-into-range-min-max-without-division
	// wraps to [Min, Max)
	inline int32_t Wrap(int32_t in_val, int32_t in_min, int32_t in_max)
	{
		return (((in_val - in_min) % (in_max - in_min)) + (in_max - in_min)) % (in_max - in_min) + in_min;
	}
	inline int32_t Wrap(int32_t in_val, int32_t in_max) 
	{ 
		return Wrap(in_val, 0, in_max); 
	}

	/////////////////////////////////////////////////////////////////////////////////
	// clamp
	/////////////////////////////////////////////////////////////////////////////////
	template<typename T>
	T Clamp(T in_val, T in_min, T in_max)
	{
		if (in_val <= in_min) return in_min;
		if (in_val >= in_max) return in_max;
		return in_val;
	}

	template<typename T>
	T ClampUnit(T in_val) {
		return Clamp<T>(in_val, 0, 1);
	}

	// component-wise vector clamp
	template<typename T>
	Vec2<T> ClampVector(Vec2<T> in_vec, Vec2<T> in_min, Vec2<T> in_max) {
		return Vec2<T>{
			Clamp(in_vec.x, in_min.x, in_max.x),
			Clamp(in_vec.y, in_min.y, in_max.y)
		};
	}

	// vector length clamp
	template<typename T>
	Vec2<T> ClampMagnitude(Vec2<T> in_vec, T in_max) {
		const T size = in_vec.Len();
		return size > in_max ? in_vec.Trunc(in_max) : in_vec;
	}
	template<typename T>
	Vec2<T> ClampMagnitude(Vec2<T> in_vec, T in_min, T in_max) {
		const float size = in_vec.Len();
		return 
			size < in_min ? in_vec.Trunc(in_min)
			: size > in_max ? in_vec.Trunc(in_max)
			: in_vec;
	}


	/////////////////////////////////////////////////////////////////////////////////
	// lerp and range mapping
	/////////////////////////////////////////////////////////////////////////////////
	template<typename T, typename U>
	T Lerp(T in_start, T in_end, U in_alpha) {
		return in_start + (in_end - in_start) * in_alpha;
	}

	template<typename T>
	T MapUnitRange(T in_val, T in_min, T in_max, bool in_bClamp = true) {
		const T mapped = (in_val - in_min) / (in_max - in_min);

		if (in_bClamp) {
			return ClampUnit(mapped);
		}

		return mapped;
	}

	template<typename InT, typename OutT = InT>
	OutT MapRange(InT in_val, InT in_inMin, InT in_inMax, OutT in_outMin, OutT in_outMax, bool in_bClamp = true) {
		return Lerp(in_outMin, in_outMax, MapUnitRange(in_val, in_inMin, in_inMax, in_bClamp));
	}


	/////////////////////////////////////////////////////////////////////////////////
	// Angle wrapping/normalization and LerpAngle
	/////////////////////////////////////////////////////////////////////////////////
	// wraps angle to [0,360)
	template<typename T>
	T WrapAngle(T in_degrees)
	{
		// wrap to (-360,360)
		in_degrees = fmod(in_degrees, 360.f);

		if (in_degrees < 0.f)
		{
			// shift to [0,360)
			in_degrees += 360.f;
		}

		return in_degrees;
	}

	// normalizes angle to (-180,180]
	template<typename T>
	T NormalizeAngle(T in_degrees)
	{
		// wrap to [0,360)
		in_degrees = WrapAngle(in_degrees);

		// shift to (-180,180]
		if (in_degrees > 180.f)
		{
			in_degrees -= 360.f;
		}

		return in_degrees;
	}

	template<typename T>
	T LerpAngle(T in_start, T in_end, T in_alpha)
	{
		return NormalizeAngle(in_start + NormalizeAngle(in_end - in_start) * in_alpha);
	}

};
