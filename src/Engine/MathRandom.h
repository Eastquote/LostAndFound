#pragma once

#include <random>

#include "MathCore.h"

/*
MathRandom: assorted random functions 
*/ 

namespace Math
{
	inline int RandomInt(int in_min, int in_max)
	{
		if(in_min == in_max) {
			return in_min;
		}
		static std::random_device rd;
		static std::mt19937 mt(rd());
		static std::uniform_int_distribution<int> dist(in_min, in_max);

		return dist(mt);
	};

	inline float RandomFloat(float in_min, float in_max)
	{
		static std::random_device rd;
		static std::mt19937 mt(rd());
		static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

		return Math::Lerp<float, float>(in_min, in_max, dist(mt));
	};

	inline float RandomFloat()
	{
		return RandomFloat(0.0f, 1.0f);
	}

	inline Vec2f RandomDirection()
	{
		const float theta = RandomFloat(0.0f, (float)M_PI * 2.0f);
		return { std::cos(theta), std::sin(theta) };
	}

	inline Vec2f RandomPointInCircle()
	{
		return RandomDirection() * std::sqrt(RandomFloat());
	}
};
