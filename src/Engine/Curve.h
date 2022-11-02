#pragma once

#include <vector>
#include <string>

#include "Vec2.h"
#include "MinMax.h"

namespace pugi
{
	class xml_document;
}

enum class eCurveSectionMode
{
	Linear,
	Bezier,
};

enum class eCurveKeyTangentMode
{
	Continuous,
	Split,
};

enum class eCurveExtrapolateMode
{
	Flat,
	Linear,
};

struct CurveKey
{
	float m_time;
	float m_val;

	Vec2f m_leftTangent = {-1.0f, 0.0f};
	Vec2f m_rightTangent = {1.0f, 0.0f};

	eCurveSectionMode m_rightSectionMode = eCurveSectionMode::Bezier;

	Vec2f GetPos() const { return { m_time, m_val }; }
};

// Curve: 
// A 1D piecewise curve composed of linear and/or cubic bezier sections
// Supports evaluating the value at a given time
struct Curve
{
	Curve() {}
	Curve(const std::string& in_filename);
	Curve(std::vector<CurveKey> in_keys) : m_keys(in_keys) {}

	void Init() 
	{
		CurveKey key{};
		key.m_time = 0.0f;
		key.m_val = 0.0f;
		key.m_rightSectionMode = eCurveSectionMode::Bezier;

		m_keys.clear();
		m_keys.push_back(key);
	}

	void Serialize(const std::string& in_filename) const;

	float Eval(float in_t) const;
	Vec2f GetTangent(float in_t) const;

	MinMaxf GetTimeRange() const;
	MinMaxf GetValueRange() const; // just looks at the keys

	bool IsValid() const { return m_keys.size() > 0; }

	std::vector<CurveKey>& GetKeys() { return m_keys; }
	const std::vector<CurveKey>& GetKeys() const { return m_keys; }

	int GetNumSections() const { return Math::Max(0, (int)m_keys.size() - 1); }
	eCurveSectionMode GetSectionMode(int in_sectionIdx) const {
		return m_keys[in_sectionIdx].m_rightSectionMode; 
	}

	int GetSectionIdxForT(float in_t) const;

private:
	static Curve CurveFromXml(const pugi::xml_document& doc);

	// sorted by time
	std::vector<CurveKey> m_keys;

	eCurveExtrapolateMode m_extrapolateMode_Min = eCurveExtrapolateMode::Flat;
	eCurveExtrapolateMode m_extrapolateMode_Max = eCurveExtrapolateMode::Flat;
};
