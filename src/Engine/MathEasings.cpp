#include "MathEasings.h"

Task<> Math::Interp(float in_duration, std::function<void(float)> in_tickFunc)
{
	float t = 0.0f;

	while (true)
	{
		t += Time::DT();
		const float alpha = Math::ClampUnit(t / in_duration);

		in_tickFunc(alpha);

		if (alpha == 1.0f) co_return;

		co_await Suspend();
	}
}
