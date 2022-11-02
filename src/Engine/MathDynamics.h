#pragma once

#include "MathCore.h"

/*
MathDynamics: various functions for dealing with velocity and kinematics
*/

namespace Math
{
	/////////////////////////////////////////////////////////////////////////////////
	// accelerate to a target velocity, without overshooting
	/////////////////////////////////////////////////////////////////////////////////
	inline float AccelTo(float in_current, float in_target, float in_accel, float in_dt)
	{
		const float deltaToTrg = in_target - in_current;

		const float step = in_accel * in_dt;
		if (step > std::abs(deltaToTrg)) return in_target;

		return in_current + Math::Sign(deltaToTrg) * step;
	}

	// vector accel along shortest path
	template<typename T>
	Vec2<T> AccelTo(Vec2<T> in_current, Vec2<T> in_target, float in_accel, float in_dt) {
		const Vec2<T> deltaToTrg = in_target - in_current;

		const float step = in_accel * in_dt;
		if (step > deltaToTrg.Len()) return in_target;

		return in_current + deltaToTrg.Norm() * step;
	}

	// component-wise vector accel
	template<typename T>
	Vec2<T> AccelTo(Vec2<T> in_current, Vec2<T> in_target, Vec2<T> in_accel, float in_dt) {
		return Vec2<T>{
			AccelTo(in_current.x, in_target.x, in_accel.x, in_dt),
			AccelTo(in_current.y, in_target.y, in_accel.y, in_dt)
		};
	}

	/////////////////////////////////////////////////////////////////////////////////
	// drag, automatically calculated from terminal velocity and reference force (eg gravity) under which that terminal velocity applies
	// models drag in air/fluid
	/////////////////////////////////////////////////////////////////////////////////
	inline float ApplyDragFromTerminalVel(float in_current, float in_terminalVel, float in_refForce, float in_dt) {
		const float dragCoef = std::abs(in_refForce) / Sqr(in_terminalVel);
		const float drag = dragCoef * Sqr(in_current);
		return AccelTo(in_current, 0.0f, drag, in_dt);
	}

	inline Vec2f ApplyDragFromTerminalVel(Vec2f in_current, float in_terminalVel, float in_refForce, float in_dt) {
		return in_current.Norm() * ApplyDragFromTerminalVel(in_current.Len(), in_terminalVel, in_refForce, in_dt);
	}

	/////////////////////////////////////////////////////////////////////////////////
	// SmoothDamp_FixedAccel
	// when called each frame, eases in_current to in_target, given maximum speed and acceleration, such that it stops when it reaches in_target
	// inout_currentVelocity must be stored externally 
	// similar to SmoothDamp, but using capped vel/accel rather than damped spring
	// if the target moves closer or velocity gets added externally, the function will break the vel/accel constraints rather than overshooting
	/////////////////////////////////////////////////////////////////////////////////
	inline float SmoothDamp_FixedAccel(float in_current, float in_target, float& inout_currentVelocity, float in_maxSpeed, float in_maxAccel, float in_dt)
	{
		const float distToTarget = std::abs(in_target - in_current);
		if (std::abs(distToTarget) < SMALL_NUMBER)
		{
			inout_currentVelocity = 0.0f;
			return in_target;
		}

		// we want to know the fastest we can be going and still stop on time
		// first, given vel and max decel, how far would we go before stopping
		// t_stop = V_0 / A
		// dist_min(t)		= V_0t - 0.5*At^2
		// dist_min(t_stop) = (V_0*V_0 / A) - 0.5*A * (V_0*V_0)/(A*A)
		//					= 0.5 * V_0^2 / A

		// now set dist_min = DistToin_target and solve for V_0
		// DistToin_target = 0.5 * V_0^2 / A
		// 2 * DistToin_target * A = V_0^2
		// sqrt(2 * DistToin_target * A) = V_0
		const float maxSpeedWithoutOvershooting = std::sqrt(2.0f * distToTarget * in_maxAccel);

		// also make sure we don't overshoot in a single euler integration step
		const float instantFinishSpeed = distToTarget / in_dt;

		// overall max speed towards target
		const float maxSpeedTowardsin_target = Math::Min3(
			maxSpeedWithoutOvershooting,
			instantFinishSpeed,
			in_maxSpeed
		);

		// integrate velocity with max acceleration
		inout_currentVelocity += in_maxAccel * in_dt * Math::Sign(in_target - in_current);

		// clamp velocity (and indirectly clamp applied acceleration) to avoid overshooting
		if (in_target > in_current)
		{
			inout_currentVelocity = Math::Clamp(inout_currentVelocity, -maxSpeedTowardsin_target, maxSpeedTowardsin_target);
		}
		else
		{
			inout_currentVelocity = Math::Clamp(inout_currentVelocity, -maxSpeedTowardsin_target, maxSpeedTowardsin_target);
		}

		// final overshoot protection 
		const float step = inout_currentVelocity * in_dt;
		if (std::abs(step) > distToTarget)
		{
			inout_currentVelocity = 0.0f;
			return in_target;
		}

		// integrate position
		return in_current + inout_currentVelocity * in_dt;
	}

	template<typename T>
	Vec2<T> SmoothDamp_FixedAccel(Vec2<T> in_current, Vec2<T> in_target, Vec2<T>& inout_currentVelocity, float in_duration, float in_maxSpeed, float in_dt) {
		return Vec2<T>{
			SmoothDamp_FixedAccel(in_current.x, in_target.x, inout_currentVelocity.x, in_duration, in_maxSpeed, in_dt),
			SmoothDamp_FixedAccel(in_current.y, in_target.y, inout_currentVelocity.y, in_duration, in_maxSpeed, in_dt)
		};
	}

	/////////////////////////////////////////////////////////////////////////////////
	// SmoothDamp (critically damped spring)
	/////////////////////////////////////////////////////////////////////////////////

	// Game Programming Gems 4 Chapter 1.10
	// https://github.com/Unity-Technologies/UnityCsReference/blob/master/Runtime/Export/Math/Mathf.cs
	inline float SmoothDamp(float in_current, float in_target, float& inout_currentVelocity, float in_duration, float in_maxSpeed, float in_dt)
	{
		in_duration = Max(0.0001F, in_duration);

		const float omega = 2.0f / in_duration;
		const float x = omega * in_dt;
		const float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
		float change = in_current - in_target;
		const float originalTo = in_target;

		// Clamp maximum speed
		const float maxChange = in_maxSpeed * in_duration;
		change = Clamp(change, -maxChange, maxChange);
		in_target = in_current - change;

		const float temp = (inout_currentVelocity + omega * change) * in_dt;
		inout_currentVelocity = (inout_currentVelocity - omega * temp) * exp;
		float output = in_target + (change + temp) * exp;

		// Prevent overshooting
		if (originalTo - in_current > 0.0F == output > originalTo)
		{
			output = originalTo;
			inout_currentVelocity = (output - originalTo) / in_dt;
		}

		return output;
	}

	template<typename T>
	Vec2<T> SmoothDamp(Vec2<T> in_current, Vec2<T> in_target, Vec2<T>& inout_currentVelocity, float in_duration, float in_maxSpeed, float in_dt) {
		return Vec2<T>{
			SmoothDamp(in_current.x, in_target.x, inout_currentVelocity.x, in_duration, in_maxSpeed, in_dt),
			SmoothDamp(in_current.y, in_target.y, inout_currentVelocity.y, in_duration, in_maxSpeed, in_dt)
		};
	}

	template<typename T>
	Vec2<T> SmoothDamp(Vec2<T> in_current, Vec2<T> in_target, Vec2<T>& inout_currentVelocity, Vec2<float> in_duration, Vec2<float> in_maxSpeed, float in_dt) {
		return Vec2<T>{
			SmoothDamp(in_current.x, in_target.x, inout_currentVelocity.x, in_duration.x, in_maxSpeed.x, in_dt),
			SmoothDamp(in_current.y, in_target.y, inout_currentVelocity.y, in_duration.y, in_maxSpeed.y, in_dt)
		};
	}

	inline float SmoothDampAngle(float in_current, float in_target, float& inout_currentVelocity, float in_duration, float in_maxSpeed, float in_dt) {
		return NormalizeAngle(in_current + SmoothDamp(0.0f, NormalizeAngle(in_target - in_current), inout_currentVelocity, in_duration, in_maxSpeed, in_dt));
	}

	///////////////////////////////////////////////////////////////////////////////
	// easing over time
	///////////////////////////////////////////////////////////////////////////////
	// moves in_current towards in_target at speed in_interpSpeed, without overshooting
	template<class T>
	T LinearEase(T in_current, T in_target, float in_interpSpeed, float in_dt) {
		float diff = in_target - in_current;
		float delta = in_interpSpeed * in_dt;

		if (delta > Abs(diff)) {
			return in_target;
		}
		else {
			return in_current + Sign(diff) * delta;
		}
	}

	inline float GetEquivalentEaseAlphaForDT(float in_alpha, float in_baseFPS, float in_dt) {
		return 1.0f - std::pow(1.0f - in_alpha, in_baseFPS * in_dt);
	}

	// FPS-independent equivalent of calling Lerp(current, target, alpha) every frame
	// see https://www.rorydriscoll.com/2016/03/07/frame-rate-independent-damping-using-lerp/
	template<class T>
	T EaseTo(T in_current, T in_target, float in_alpha, float in_dt, float in_baseFPS=30.0f) {
		return Lerp(in_current, in_target, GetEquivalentEaseAlphaForDT(in_alpha, in_baseFPS, in_dt));
	}
	// version of EaseTo that properly wraps angles 
	inline float EaseAngleTo(float in_current, float in_target, float in_alpha, float in_dt, float in_baseFPS=30.0f) {
		return LerpAngle(in_current, in_target, GetEquivalentEaseAlphaForDT(in_alpha, in_baseFPS, in_dt));
	}

	// similar to EaseTo, but with configurable increased response speed (while still smoothing out jitters)
	// https://twitter.com/mmalex/status/1282006562123534338
	// https://cytomic.com/files/dsp/DynamicSmoothing.pdf
	template<class T>
	T BandEaseTo(T in_current, T in_target, T& in_intermediateSmoothedVal, float in_sense, float in_alpha, float in_dt, float in_baseFPS = 30.0f) {
		in_alpha = Math::Min(1.0f, in_alpha + in_sense * std::abs(in_intermediateSmoothedVal - in_current));
		in_alpha = GetEquivalentEaseAlphaForDT(in_alpha, in_baseFPS, in_dt);
		in_intermediateSmoothedVal = Lerp(in_intermediateSmoothedVal, in_target, in_alpha);
		return Lerp(in_current, in_intermediateSmoothedVal, in_alpha);
	}

	///////////////////////////////////////////////////////////////////////////////
	// damped springs
	///////////////////////////////////////////////////////////////////////////////
	// float damped spring spring 
	// see http://www.huwbowles.com/spring-dynamics-production/
	inline float Spring(
		float in_curPos, float in_targetPos,
		float& inout_curVel, float in_targetVel,
		float in_oscFreq, // undamped angular frequency (the frequency that the spring will oscillate at (bounces per second))
		float in_dampRatio, // <1 = underdamped; >1 = overdamped
		float in_dt,
		float in_maxForce = BIG_NUMBER
	)
	{
		const float maxDT = 1.0f / 60.0f;
		const float stepCountF = std::ceil(in_dt / maxDT);
		const float substepDt = in_dt / stepCountF;
		int stepCount = (int)stepCountF;

		const int MaxStepCount = 8;
		stepCount = Math::Min(stepCount, MaxStepCount);

		for (int i = 0; i < stepCount; i++)
		{
			// compute spring and damping forces
			float posNextState = in_curPos + substepDt * inout_curVel;
			float fSpring = in_oscFreq * in_oscFreq * (in_targetPos - posNextState);
			float fDamp = 2.0f * in_dampRatio * in_oscFreq * (in_targetVel - inout_curVel);

			// integrate dynamics	
			float force = (fSpring + fDamp) / (1.0f + substepDt * 2.0f * in_dampRatio * in_oscFreq);
			inout_curVel += Math::Min(in_maxForce, std::abs(force)) * Math::Sign(force) * substepDt;
			in_curPos = in_curPos + inout_curVel * substepDt;
		}
		return in_curPos;
	}

	// angle damped spring
	inline float SpringAngle(
		float in_curPos, float in_targetPos,
		float& inout_curVel, float in_targetVel,
		float in_oscFreq, // undamped angular frequency
		float in_dampRatio, // <1 = underdamed; >1 = overdamped
		float in_dt
	)
	{
		in_curPos = Wrap(in_curPos, -180.0f, 180.0f);
		in_targetPos = Wrap(in_targetPos, in_curPos - 180.0f, in_curPos + 180.0f);

		return Spring(in_curPos, in_targetPos, inout_curVel, in_targetVel, in_oscFreq, in_dampRatio, in_dt);
	}

	// Vec2 damped spring
	template<typename T>
	Vec2<T> Spring(
		const Vec2<T> in_curPos, const Vec2<T> in_targetPos,
		Vec2<T>& inout_curVel, Vec2<T> in_targetVel,
		float in_oscFreq, // undamped angular frequency
		float in_dampRatio, // <1 = underdamed; >1 = overdamped
		float in_dt
	) {

		return Vec2<T>{
			Spring(in_curPos.x, in_targetPos.x, inout_curVel.x, in_targetVel.x, in_oscFreq, in_dampRatio, in_dt),
			Spring(in_curPos.y, in_targetPos.y, inout_curVel.y, in_targetVel.y, in_oscFreq, in_dampRatio, in_dt)
		};
	}
};