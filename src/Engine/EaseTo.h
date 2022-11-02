#pragma once

#include <cmath>
#include <algorithm>
#include "Vec2.h"
#include "TasksConfig.h"

// TODO: Make an EaseTo_Linear for Vec2, float, euler float
// takes in a from, a to, a DT, and a speed
// goes to the target, doesn't overshoot (clamp to length)

// TODO: Make an asymptotic easing function (power function)

// Normalize an angle (in degrees) into (-180 -> 180) range
inline float NormalizeAngleDegrees(float in_angle)
{
	in_angle = std::fmod(in_angle, 360.0f);
	if(in_angle < 0.0f)
	{
		in_angle += 360.0f;
	}
	if(in_angle > 180.0f)
	{
		in_angle -= 360.0f;
	}
	return in_angle;
}

// Clamp a vector (float, Vec2f) to be no longer than a specified length
inline float ClampToLength(float in_val, float in_len)
{
	return std::min(std::abs(in_val), in_len) * (std::signbit(in_val) ? 1.0f : -1.0f);
}
inline Vec2f ClampToLength(const Vec2f& in_val, float in_len)
{
	float len = in_val.Len();
	if(len == 0.0f)
	{
		return in_val;
	}
	Vec2f norm = in_val / len;
	len = std::min(len, in_len);
	return norm * len;
}

inline float EaseTo_Linear(float in_from, float in_to, float in_speed) 
{
	if(in_from < in_to) {
		return Math::Clamp(in_from + in_speed, in_from, in_to);
	}
	else {
		return Math::Clamp(in_from + in_speed, in_to, in_from);
	}
}

inline Vec2f EaseTo_Linear(Vec2f in_from, Vec2f in_to, float in_speed)
{
	auto vel = (in_to - in_from).Norm() * in_speed;
	vel = ClampToLength(vel, (in_to - in_from).Len()); // BUG: seems this isn't stopping overshoots?

	return in_from + vel;
}

// Ease a vector (float/Vec2f) from one value to another using a critically-damped spring function
template <typename T>
T EaseTo_Spring(T in_from, T in_to, float in_dt, T& in_vel, float in_dur, std::optional<float> in_maxSpeed = {})
{
	// Early-out if smooth time is zero or negative
	if(in_dur <= 0.0f)
	{
		in_vel = { 0 };
		return in_to;
	}

	// From Game Gems 4 (Section 1.10, pg. 99)
	float omega = 2.0f / in_dur;
	float x = omega * in_dt;
	float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x); // std::pow() approximation
	T change = in_from - in_to;
	if(in_maxSpeed)
	{
		float maxChange = in_maxSpeed.value() * in_dur;
		change = ClampToLength(change, maxChange);
		in_to = in_from - change;
	}
	T temp = (in_vel + change * omega) * in_dt;
	in_vel = (in_vel - temp * omega) * exp;
	T result = in_to + ((change + temp) * exp);
	return result;
}

// Ease an angle (in degrees) from one value to another using a critically-damped spring function
inline float EaseTo_Spring_Euler(float in_from, float in_to, float in_dt, float& in_vel, float in_dur, std::optional<float> in_maxSpeed = {})
{
	return NormalizeAngleDegrees(in_from + EaseTo_Spring(0.0f, NormalizeAngleDegrees(in_to - in_from), in_dt, in_vel, in_dur, in_maxSpeed));
}

// Simple test function to verify that the ease functions are behaving correctly
inline void TestEasing()
{
	// Spring ease a vector value (float)
	float a = 100.0f;
	float b = -100.0f;
	float vel = 0.0f;
	while(std::abs(b - a) > 0.01f)
	{
		a = EaseTo_Spring(a, b, 1 / 60.0f, vel, 2.0f);
		printf("a = %f\n", a);
	}

	// Spring ease a vector value (Vec2f)
	Vec2f vecA = { 100.0f, -100.0f };
	Vec2f vecB = { -100.0f, 100.0f };
	Vec2f vecVel = { 0.0f };
	while((vecB - vecA).Len() > 0.01f)
	{
		vecA = EaseTo_Spring(vecA, vecB, 1 / 60.0f, vecVel, 2.0f);
		printf("vecA = (%f, %f)\n", vecA.x, vecA.y);
	}

	// Spring ease an angle value (in degrees)
	float angleA = 100.0f;
	float angleB = -100.0f;
	float angleVel = 0.0f;
	while(std::abs(angleB - angleA) > 0.01f)
	{
		angleA = EaseTo_Spring_Euler(angleA, angleB, 1 / 60.0f, angleVel, 2.0f);
		printf("angleA = %f\n", angleA);
	}
}
