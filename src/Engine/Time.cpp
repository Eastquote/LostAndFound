#include "Time.h"

#include "TasksConfig.h"
#include <cstdio>

//--- Game Time ---//
float g_gameDt = 0.0;
float g_audioDt = 0.0;
float g_realDt = 0.0;
float g_gameTime = 0.0;
float g_audioTime = 0.0;
float g_realTime = 0.0;

namespace Time
{
	float DT()
	{
		return g_gameDt;
	}
	float AudioDT()
	{
		return g_audioDt;
	}
	float RealDT()
	{
		return g_realDt;
	}
	float Time()
	{
		return g_gameTime;
	}
	float AudioTime()
	{
		return g_audioTime;
	}
	float RealTime()
	{
		return g_realTime;
	}
}

// User-defined GlobalTime() is required to link Task.h
NAMESPACE_SQUID_BEGIN
tTaskTime GetGlobalTime()
{
	return g_gameTime;
}
NAMESPACE_SQUID_END
