#pragma once

// Game Time
enum class eTimeStream
{
	Game, // time dilation, pauses
	Audio, // NO time dilation, does NOT pause
	Real, // NO time dilation, does NOT pause
};
namespace Time
{
	float DT(); // Delta-time (time dilation, pauses)
	float AudioDT(); // Delta-time (NO time dilation, pauses)
	float RealDT(); // Delta-time (NO time dilation, does NOT pause)
	float Time(); // Current time (time dilation, pauses)
	float AudioTime(); // Current time (NO time dilation, pauses)
	float RealTime(); // Current time (NO time dilation, does NOT pause)
}
