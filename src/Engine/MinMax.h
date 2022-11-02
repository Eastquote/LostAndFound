#pragma once

#include "MathCore.h"
#include "TasksConfig.h" // for std::optional

template <typename T>
struct MinMaxT
{
public:
	T m_min = 0;
	T m_max = 0;

	MinMaxT<T>() = default;
	MinMaxT<T>(T in_min, T in_max) : m_min(in_min), m_max(in_max) {}

	void ExpandToInclude(T in_val)	{
		m_min = Math::Min(m_min, in_val);
		m_max = Math::Max(m_max, in_val);
	}

	T Size() const { return std::abs(m_max - m_min); }

	T Lerp(T in_val) const {
		return Math::Lerp(m_min, m_max, in_val);
	}
	T MapUnitRange(T in_val) const {
		return Math::MapUnitRange(in_val, m_min, m_max);
	}

	T Clamp(T in_val) const {
		return Math::Clamp(in_val, m_min, m_max);
	}

	T ClosestEdge(T in_val) const {
		return in_val > Mid() ? m_max : m_min;
	}

	T Mid() const { return m_min + (m_max - m_min) * 0.5f; }
	
	bool Contains_Incl(T in_val) const { return in_val >= m_min && in_val <= m_max; }
	bool Contains_Excl(T in_val) const { return in_val > m_min && in_val < m_max; }
	bool Contains_InclExcl(T in_val) const { return in_val >= m_min && in_val < m_max; }
	
	std::optional<MinMaxT<T>> Intersect(MinMaxT<T> in_other) const {
		const MinMaxT<T> asc0 = GetAscending();
		const MinMaxT<T> asc1 = in_other.GetAscending();

		// not intersecting
		if (asc0.m_max < asc1.m_min || asc0.m_min > asc1.m_max)
		{
			return std::optional<MinMaxT<T>>{};
		}

		return MinMaxT<T>(
			Math::Max(asc0.m_min, asc1.m_min),
			Math::Min(asc0.m_max, asc1.m_max)
		);
	}

	// component-wise multiplication
	MinMaxT<T> operator* (T in_mult) {
		MinMaxT<T> ret = *this;
		ret.m_min *= in_mult;
		ret.m_max *= in_mult;
		return ret;
	}

	MinMaxT<T> GetAscending() const {
		MinMaxT<T> ret = *this;
		ret.EnforceAscending();
		return ret;
	}
	void EnforceAscending() {
		if (m_min > m_max) {
			T temp = m_max;
			m_max = m_min;
			m_min = temp;
		}
	}
};

using MinMaxi = MinMaxT<int32_t>;
using MinMaxu = MinMaxT<uint32_t>;
using MinMaxf = MinMaxT<float>;
using MinMaxd = MinMaxT<double>;
