#include "Curve.h"

#include <iostream>
#include <stdio.h>

#include "MathCore.h"

#include "pugixml.hpp"

Curve Curve::CurveFromXml(const pugi::xml_document& doc)
{
	auto curveNode = doc.child("curve");
	
	Curve newCurve{};

	auto keysGroup = curveNode.child("keys");
	auto keyNode = keysGroup.child("key");
	while (!keyNode.empty())
	{
		CurveKey newKey{};

		newKey.m_time = keyNode.attribute("time").as_float(0.0f);
		newKey.m_val = keyNode.attribute("val").as_float(0.0f);

		newKey.m_leftTangent.x = keyNode.attribute("leftTangent_x").as_float(-1.0f);
		newKey.m_leftTangent.y = keyNode.attribute("leftTangent_y").as_float(0.0f);

		newKey.m_rightTangent.x = keyNode.attribute("rightTangent_x").as_float(1.0f);
		newKey.m_rightTangent.y = keyNode.attribute("rightTangent_y").as_float(0.0f);

		newKey.m_rightSectionMode = (eCurveSectionMode)keyNode.attribute("rightSectionMode").as_int();

		newCurve.m_keys.push_back(newKey);

		keyNode = keyNode.next_sibling("key");
	}
	
	newCurve.m_extrapolateMode_Min = (eCurveExtrapolateMode)curveNode.attribute("extrapolateMode_Min").as_int();
	newCurve.m_extrapolateMode_Max = (eCurveExtrapolateMode)curveNode.attribute("extrapolateMode_Max").as_int();

	return newCurve;
}

Curve::Curve(const std::string& in_filename)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(in_filename.c_str());
	SQUID_RUNTIME_CHECK(result.status == pugi::status_ok, "Failed to load curve.");
	if (result.status == pugi::status_ok)
	{
		*this = CurveFromXml(doc);
	}
}

void Curve::Serialize(const std::string& in_filename) const
{
	pugi::xml_document doc{};

	auto curveNode = doc.append_child("curve");
	curveNode.append_attribute("extrapolateMode_Min").set_value((int)m_extrapolateMode_Min);
	curveNode.append_attribute("extrapolateMode_Max").set_value((int)m_extrapolateMode_Max);

	auto keysNode = curveNode.append_child("keys");
	for (const CurveKey& key : m_keys)
	{
		auto keyNode = keysNode.append_child("key");

		keyNode.append_attribute("time").set_value(key.m_time);
		keyNode.append_attribute("val").set_value(key.m_val);

		keyNode.append_attribute("leftTangent_x").set_value(key.m_leftTangent.x);
		keyNode.append_attribute("leftTangent_y").set_value(key.m_leftTangent.y);

		keyNode.append_attribute("rightTangent_x").set_value(key.m_rightTangent.x);
		keyNode.append_attribute("rightTangent_y").set_value(key.m_rightTangent.y);

		keyNode.append_attribute("rightSectionMode").set_value((int)key.m_rightSectionMode);
	}

	std::cout << "Curve saving result: " << doc.save_file(in_filename.c_str()) << std::endl;

	//assert(CurveFromXml(doc) == *this);
}

template<typename T>
static T EvalCubicBezier(T in_0, T in_1, T in_2, T in_3, float in_alpha)
{
	const T v01 = Math::Lerp(in_0, in_1, in_alpha);
	const T v12 = Math::Lerp(in_1, in_2, in_alpha);
	const T v23 = Math::Lerp(in_2, in_3, in_alpha);

	const T v01_12 = Math::Lerp(v01, v12, in_alpha);
	const T v12_23 = Math::Lerp(v12, v23, in_alpha);

	const T v0123 = Math::Lerp(v01_12, v12_23, in_alpha);

	return v0123;
}

// find roots of cubic equation
// from graphics gems 1 https://github.com/erich666/GraphicsGems/blob/master/gems/Roots3And4.c
// in_coeff: coefficient parameters of form  in_coeff[0] + in_coeff[1]*x + in_coeff[2]*x^2 + in_coeff[3]*x^3 + in_coeff[4]*x^4 = 0
// out_solutions: up to 3 real roots of the equation (ignore imaginary solutions)
// returns the number of solutions in the out_solutions array
static int SolveCubic(double in_coeff[4], double out_solutions[3])
{
	const auto cbrt = [](double in_x) {
		return (in_x > 0.0 ? pow(in_x, 1.0 / 3.0) : (in_x < 0.0 ? -pow((double)-in_x, 1.0 / 3.0) : 0.0));
	};

	const auto IsZero = [](double in_x) {
		return Math::Abs(in_x) <= SMALL_NUMBER;
	};

	int     i, num;
	double  sub;
	double  A, B, C;
	double  sq_A, p, q;
	double  cb_p, D;

	/* normal form: x^3 + Ax^2 + Bx + C = 0 */

	A = in_coeff[ 2 ] / in_coeff[ 3 ];
	B = in_coeff[ 1 ] / in_coeff[ 3 ];
	C = in_coeff[ 0 ] / in_coeff[ 3 ];

	/*  substitute x = y - A/3 to eliminate quadric term:
	x^3 +px + q = 0 */

	sq_A = A * A;
	p = 1.0/3 * (- 1.0/3 * sq_A + B);
	q = 1.0/2 * (2.0/27 * A * sq_A - 1.0/3 * A * B + C);

	/* use Cardano's formula */

	cb_p = p * p * p;
	D = q * q + cb_p;

	if (IsZero(D))
	{
		if (IsZero(q)) /* one triple solution */
		{
			out_solutions[ 0 ] = 0;
			num = 1;
		}
		else /* one single and one double solution */
		{
			double u = cbrt(-q);
			out_solutions[ 0 ] = 2 * u;
			out_solutions[ 1 ] = - u;
			num = 2;
		}
	}
	else if (D < 0) /* Casus irreducibilis: three real solutions */
	{
		double phi = 1.0/3 * acos(-q / sqrt(-cb_p));
		double t = 2 * sqrt(-p);

		out_solutions[ 0 ] =   t * cos(phi);
		out_solutions[ 1 ] = - t * cos(phi + M_PI / 3);
		out_solutions[ 2 ] = - t * cos(phi - M_PI / 3);
		num = 3;
	}
	else /* one real solution */
	{
		double sqrt_D = sqrt(D);
		double u = cbrt(sqrt_D - q);
		double v = - cbrt(sqrt_D + q);

		out_solutions[ 0 ] = u + v;
		num = 1;
	}

	/* resubstitute */

	sub = 1.0/3 * A;

	for (i = 0; i < num; ++i)
		out_solutions[ i ] -= sub;

	return num;
}

int Curve::GetSectionIdxForT(float in_t) const
{
	if (m_keys.size() == 0 || in_t < m_keys[0].m_time)
	{
		return -1;
	}

	for (int idx0 = 0; idx0 < m_keys.size() - 1; idx0++)
	{
		if (m_keys[idx0 + 1].m_time > in_t)
		{
			return idx0;
		}
	}

	return (int)m_keys.size();
}

float Curve::Eval(float in_t) const
{
	// handle <2 keys cases
	if (m_keys.size() == 0)
	{
		return 0.0f;
	}
	if (m_keys.size() == 1)
	{
		return m_keys[0].m_val;
	}
	
	// helper function for extrapolating when in_t is beyond the defined curve sections
	const auto Extrapolate = [in_t](const CurveKey& in_key, eCurveExtrapolateMode in_mode)
	{
		if (in_mode == eCurveExtrapolateMode::Flat)
		{
			return in_key.m_val;
		}
		else if (in_mode == eCurveExtrapolateMode::Linear)
		{
			const Vec2f tangent = in_key.m_leftTangent;
			return in_key.m_val + tangent.y / tangent.x * (in_t - in_key.m_time);
		}
		else
		{
			SQUID_RUNTIME_ERROR("Unhandled curve extrapolate mode");
			return in_key.m_val;
		}
	};

	// handle t before the first key or after the last key
	if (in_t <= m_keys[0].m_time)
	{
		return Extrapolate(m_keys[0], m_extrapolateMode_Min);
	}
	if (in_t >= m_keys[m_keys.size() - 1].m_time)
	{
		return Extrapolate(m_keys[m_keys.size() - 1], m_extrapolateMode_Max);
	}

	// get the pair of keys bordering t
	const int idx0 = GetSectionIdxForT(in_t);

	const CurveKey& key0 = m_keys[idx0];
	const CurveKey& key1 = m_keys[idx0 + 1];
	const eCurveSectionMode mode = key0.m_rightSectionMode;
	
	if (mode == eCurveSectionMode::Linear)
	{
		return Math::MapRange(in_t, key0.m_time, key1.m_time, key0.m_val, key1.m_val);
	}
	else if (mode == eCurveSectionMode::Bezier)
	{
		// calculate the alpha along the curve such that evaluating there will give a point at our desired x position
		// to do this, we'll express the x component of our bezier as a cubic equation in a form ax^3 + bx^2 + cx + d (power basis), translate by our desired t, and solve the roots of the equation
		// (approach based on UE4 implementation)

		const auto BezierToPower = [](double in_0, double in_1, double in_2, double in_3,
			double* out_0, double* out_1, double* out_2, double* out_3)
		{
			double a = in_1 - in_0;
			double b = in_2 - in_1;
			double c = in_3 - in_2;
			double d = b - a;
			*out_0 = c - b - d;
			*out_1 = 3.0 * d;
			*out_2 = 3.0 * a;
			*out_3 = in_0;
		};

		// convert normalized time bezier to cubic equation
		const MinMaxf xRange = { key0.m_time, key1.m_time };
		const float xDiff = xRange.Size();
		double coeff[4];
		BezierToPower(
			0.0, 0.0f + key0.m_rightTangent.x / xDiff, 1.0f + key1.m_leftTangent.x / xDiff, 1.0,
			&(coeff[3]), &(coeff[2]), &(coeff[1]), &(coeff[0])
		);

		// translate down by the value we want to evaluate
		coeff[0] -= xRange.MapUnitRange(in_t);

		// solve the cubic to find our t
		double cubicSolutions[3];
		const int numSolutions = SolveCubic(coeff, cubicSolutions);

		float evalT = xRange.MapUnitRange(in_t);
		if (numSolutions == 1)
		{
			evalT = (float)cubicSolutions[0];
		}
		else
		{
			evalT = 0.0f;
			for (double soln : cubicSolutions)
			{
				if (soln <= 1.0f)
				{
					evalT = Math::Max((float)soln, evalT);
				}
			}
		}
		
		// now that we have our t value, evaluate the bezier
		// we just need to evaluate it on the y axis, since the x axis would give us in_t back
		return EvalCubicBezier(
			key0.m_val, 
			key0.m_val + key0.m_rightTangent.y, 
			key1.m_val + key1.m_leftTangent.y, 
			key1.m_val, 
			evalT);
	}
	else
	{
		SQUID_RUNTIME_ERROR("Unhandled curve section mode");
		return key0.m_val;
	}
}

Vec2f Curve::GetTangent(float in_t) const
{
	const float dx = 0.0001f;
	return Vec2f{
		dx,
		(Eval(in_t + dx / 2) - Eval(in_t - dx / 2))
	}.Norm();
}

MinMaxf Curve::GetTimeRange() const
{
	if (m_keys.size() == 0)
	{
		return {0.0f, 0.0f};
	}

	return { m_keys[0].m_time, m_keys[m_keys.size() - 1].m_time };
}

MinMaxf Curve::GetValueRange() const
{
	const float y0 = m_keys[0].m_val;
	MinMaxf yRange = { y0, y0 };
	for (const auto& key : m_keys)
	{
		const float y = key.m_val;
		yRange.m_min = Math::Min(yRange.m_min, y);
		yRange.m_max = Math::Max(yRange.m_max, y);
	}
	return yRange;
}
