#pragma once

#include <vector>
#include <cassert>

#include "Time.h"
#include "MathCore.h"
#include "TasksConfig.h" // for std::optional

///////////////////////////////////////////////////////
// HistoryBuffer: 
// basically just a fixed-size ring buffer where you can add items to the front and the oldest gets forgotten when it's out of space
template<typename T>
struct HistoryBuffer
{
private:
	std::vector<T> m_data;
	int m_nextIdx = 0;
	int m_curNumItems = 0;

public:
	HistoryBuffer(int Size) {
		m_data.resize(Size);
		m_nextIdx = 0;
		m_curNumItems = 0;
	}

	int GetBufferCapacity() const { return (int)m_data.size(); }
	int NumItems() const { return m_curNumItems; }

	void Stomp(T in_val) {
		const int curIdx = m_nextIdx > 0 ? m_nextIdx - 1 : GetBufferCapacity() - 1;
		m_data[curIdx] = in_val;
	}

	void Push(T in_val) {
		m_data[m_nextIdx] = in_val;

		m_nextIdx = m_nextIdx + 1;
		if (m_nextIdx >= GetBufferCapacity()) { m_nextIdx = 0; }

		m_curNumItems++;
		m_curNumItems = Math::Min(m_curNumItems, GetBufferCapacity());
	}

	T operator[] (int in_framesBack) const {
		return Get(in_framesBack);
	}

	T Get(int in_framesBack) const {
		assert(m_curNumItems > 0);
		assert(in_framesBack >= 0);
		assert(in_framesBack < m_curNumItems);
		in_framesBack = Math::Clamp(in_framesBack, 0, m_curNumItems - 1);

		int idx = m_nextIdx - 1 - in_framesBack;
		if (idx < 0) idx += GetBufferCapacity();

		return m_data[idx];
	}

	T GetEarliest() const { return Get(m_curNumItems - 1); }
};

///////////////////////////////////////////////////////
// HistoryBuffer_WithTimes: 
// Similar to HistoryBuffer, but also stores the timestamp of each entry, so you can look up past values by timestamp
template<typename T>
class HistoryBuffer_WithTimes
{
public:
	HistoryBuffer_WithTimes(int in_size, bool in_bRealTime=false) : m_history_Times(in_size), m_history_States(in_size), m_bRealTime(in_bRealTime) {}
	virtual ~HistoryBuffer_WithTimes() = default;

	int GetBufferCapacity() const { return m_history_States.GetBufferCapacity(); }
	int NumItems() const { return m_history_States.NumItems(); }

	T GetValAtPastFrame(int in_framesBack) const { 
		assert(NumItems() > 0); 
		if (in_framesBack >= m_history_States.NumItems()) return m_history_States.GetEarliest();
		return m_history_States[in_framesBack]; 
	}

	T GetValAtPastTime(float in_secondsBack) const {
		int frameIdx = GetFrameIdxAtPastTime(in_secondsBack);
		return GetValAtPastFrame(frameIdx);
	}

	float GetTimeAtPastFrame(int in_framesBack) const { 
		assert(NumItems() > 0); 
		if (in_framesBack >= m_history_Times.NumItems()) return m_history_Times.GetEarliest();
		return m_history_Times[in_framesBack]; 
	}

	int GetFrameIdxAtPastTime(float in_secondsBack) const {
		assert(NumItems() > 0);
		assert(in_secondsBack >= 0);

		const double targetTime = GetCurTime() - in_secondsBack;

		if (targetTime <= m_history_Times.GetEarliest())
		{
			return GetBufferCapacity() + 1;
		}

		int frameIdx = 0;
		while (m_history_Times[frameIdx] >= targetTime && frameIdx < HistoryLength())
		{
			frameIdx++;
		}

		return frameIdx;
	}

	T GetEarliest() const { return m_history_States.GetEarliest(); }
	float GetEarliestTime() const { return m_history_Times.GetEarliest(); }

	virtual void Stomp(T in_newState) {
		m_history_States.Stomp(in_newState);
	}
	virtual void Push(T in_newState) {
		m_history_States.Push(in_newState);
		m_history_Times.Push(GetCurTime());
	}

protected:
	int HistoryLength() const { return m_history_States.NumItems(); }

	float GetCurTime() const { return m_bRealTime ? Time::RealTime() : Time::Time(); }

	HistoryBuffer<float> m_history_Times;

private:
	HistoryBuffer<T> m_history_States;
	bool m_bRealTime = false;
};
