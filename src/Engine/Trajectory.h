#pragma once

#include <cassert>

#include "Engine/DebugDrawSystem.h"
#include "Engine/MathCore.h"

// Trajectory: struct modeling projectile motion under constant gravity, with a variety of options for construction and queries

struct Trajectory
{
	static Trajectory FromP0V0GravDur(Vec2f in_p0, Vec2f in_v0, Vec2f in_gravity, float in_dur) {
		Trajectory traj{};
		traj.m_p0 = in_p0;
		traj.m_v0 = in_v0;
		traj.m_gravity = in_gravity;
		traj.m_dur = in_dur;
		return traj;
	}

	static Trajectory FromP0P1GravDur(Vec2f in_p0, Vec2f in_p1, Vec2f in_gravity, float in_dur) {
		Trajectory traj{};
		traj.m_p0 = in_p0;
		traj.m_gravity = in_gravity;
		traj.m_dur = in_dur;

		const Vec2f curP1_ZeroV0 = traj.GetP1();
		const Vec2f additionalDisplacementNeeded = in_p1 - curP1_ZeroV0;
		traj.m_v0 = additionalDisplacementNeeded / in_dur;

		{
			assert((traj.GetP1() - in_p1).Len() < 2.0f);
		}

		return traj;
	}

	static Trajectory FromP0P1GravLaunchPitch(Vec2f in_p0, Vec2f in_p1, Vec2f in_gravity, float in_launchPitch) {
		Trajectory Traj{};
		Traj.m_p0 = in_p0;
		Traj.m_gravity = in_gravity;

		const Vec2f delta = in_p1 - in_p0;

		// calculate the launch direction vector, in_launchPitch degrees off the plane normal to gravity, using -gravity as "up"
		// the launch direction will be either cw or ccw of "up" depending on whether the direction from p0 to p1 is cw or ccw
		const Vec2f up = -in_gravity;
		const Vec2f v0Norm = up.RotateDeg((90.0f - in_launchPitch) * Math::Sign(up.Cross(delta)));

		// calc V0 magnitude and dur (T)
		// P0 + (V0Norm * V0Mag * T) + (0.5f * Gravity * T*T) = P1
		// (V0Norm * V0Mag * T) + (0.5f * Gravity * T*T) = P1 - P0

		// split into vertical and horizontal components relative to gravity and P0->P1
		// horizontal is 1D because the trajectory is aimed straight from P0 to P1
		const float v0NormZ = v0Norm.Dot( in_gravity.Norm() );
		const float v0NormXY = (v0Norm - in_gravity.Norm() * v0NormZ).Len();
		const float deltaZ = delta.Dot( in_gravity.Norm() );
		const float deltaXY = (delta - in_gravity.Norm() * deltaZ).Len();

		// solve on the horizontal (relative to gravity) first:
		// let DeltaXY = P1 - P0 along the XY plane
		// on the horizontal, gravity is 0:
		// V0NormXY * V0Mag * T + (0.5f * GravityXY * T*T) = DeltaXY
		// V0NormXY * V0Mag * T = DeltaXY
		// V0Mag * T = DeltaXY/V0NormXY

		// sub into quadratic on Z
		// (V0NormZ * V0Mag * T) + (0.5f * Gravity * T*T) = DeltaZ
		// (V0NormZ * DeltaXY/V0NormXY) + (0.5f * Gravity * T*T) = DeltaZ
		// (0.5f * Gravity * T*T) = DeltaZ - (V0NormZ * DeltaXY/V0NormXY)
		// T*T = (DeltaZ - (V0NormZ * DeltaXY/V0NormXY)) / (0.5f * Gravity)
		// T = Sqrt((DeltaZ - (V0NormZ * DeltaXY/V0NormXY)) / (0.5f * Gravity))

		const float t = std::sqrt((deltaZ - (v0NormZ * deltaXY/v0NormXY)) / (0.5f * in_gravity.Len()));
		const float v0Mag = deltaXY / v0NormXY / t;

		Traj.m_dur = t;
		Traj.m_v0 = v0Norm * v0Mag;

		assert((Traj.GetP1() - in_p1).Len() < 2.0f);

		return Traj;
	}

	Vec2f GetPos(float in_t) const {
		return m_p0 + (m_v0 * in_t) + (m_gravity * 0.5f * in_t*in_t);
	}
	Vec2f GetVel(float in_t) const {
		return m_v0 + m_gravity * in_t;
	}

	Vec2f GetP1() const { return GetPos(m_dur); }

	void SetP1_ChangeGravity(Vec2f in_p1) {
		*this = FromP0P1GravLaunchPitch(m_p0, in_p1, m_gravity, GetLaunchPitch());
	}
	void SetP1_ChangeV0(Vec2f in_p1) {
		*this = FromP0P1GravDur(m_p0, in_p1, m_gravity, m_dur);
	}

	void SetStartTime(float in_startT) {
		const Vec2f newP0 = GetPos(in_startT);
		const Vec2f newV0 = GetVel(in_startT);
		m_p0 = newP0;
		m_v0 = newV0;
		m_dur -= in_startT;
	}

	float GetLaunchPitch() const {
		return std::atan2(m_v0.y, Math::Abs(m_v0.x));
	}

	void DrawDebug(int32_t in_numSteps=30, sf::Color in_color=sf::Color::Red, float in_duration=-1.0f) const {
		const float step = m_dur / in_numSteps;
		for (int i = 0; i < in_numSteps; i++)
		{
			DrawDebugLine(GetPos(i * step), GetPos((i+1) * step), in_color, {}, in_duration);
		}
	}

	Vec2f m_p0 = Vec2f::Zero;
	Vec2f m_v0 = Vec2f::Zero;
	Vec2f m_gravity = Vec2f::Zero;

	float m_dur = 0.0f;
};
