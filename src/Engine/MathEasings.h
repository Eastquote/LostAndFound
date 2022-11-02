#pragma once

#include <functional>

#include "MathCore.h"
#include "Task.h"
#include "Time.h"

/*
MathEasings: assorted easing functions
general reference: http://robertpenner.com/easing/
visual reference: https://easings.net 
*/ 

namespace Math
{
	/////////////////////////////////////////////////////////////////////////////////
	// easings
	// for each curve type, there are Ease and Interp variants
	// Ease just applies the curve to an alpha value in the range [0, 1]
	// Interp takes a start and end value, and maps the output between those
	/////////////////////////////////////////////////////////////////////////////////


	// macro that defines all of our easing and interp funcs for a given curve type
	// supports passing extra arguments by passing their decls and name lists to the in_extraArgs params
#define EASINGS_DEFINE_FUNCS_ONE_ARG(in_name, in_interpInAlphaExpr, in_extraArgsDecl, in_extraArgsNames) \
	inline float EaseIn ## in_name ## (float in_alpha in_extraArgsDecl ) { \
		return in_interpInAlphaExpr; \
	} \
	inline float EaseOut ## in_name ## (float in_alpha in_extraArgsDecl ) { \
		in_alpha = 1.0f - in_alpha; \
		in_alpha = in_interpInAlphaExpr; \
		in_alpha = 1.0f - in_alpha; \
		return in_alpha; \
	} \
	inline float EaseInOut ## in_name ## (float in_alpha in_extraArgsDecl ) { \
		return (in_alpha < 0.5f) ? \
			EaseIn ## in_name ## (in_alpha * 2.f in_extraArgsNames) * 0.5f : \
			EaseOut ## in_name ## (in_alpha * 2.f - 1.f in_extraArgsNames) * 0.5f + 0.5f; \
	} \
	template<typename T> T InterpIn ## in_name ## (T in_start, T in_end, float in_alpha in_extraArgsDecl ) { \
		in_alpha = in_interpInAlphaExpr; \
		return Lerp<T>(in_start, in_end, in_alpha); \
	} \
	template<typename T> T InterpOut ## in_name ## (T in_start, T in_end, float in_alpha in_extraArgsDecl ) { \
		in_alpha = 1.0f - in_alpha; \
		in_alpha = in_interpInAlphaExpr; \
		in_alpha = 1.0f - in_alpha; \
		return Lerp<T>(in_start, in_end, in_alpha); \
	} \
	template<typename T> T InterpInOut ## in_name ## (T in_start, T in_end, float in_alpha in_extraArgsDecl ) { \
		return Lerp<T>(in_start, in_end, EaseInOut ## in_name ## (in_alpha in_extraArgsNames)); \
	}


	// no-extra-args macro variant (just passes nothing for the extraArgs params)
#define EASINGS_DEFINE_FUNCS(in_name, in_interpInAlphaExpr) EASINGS_DEFINE_FUNCS_ONE_ARG(in_name, in_interpInAlphaExpr, , )


	// comma macro so that we can have commas within a single macro parameter
#define EASINGS_COMMA ,


	// the actual curve functions
	EASINGS_DEFINE_FUNCS(Linear, in_alpha)
	EASINGS_DEFINE_FUNCS(Quad, in_alpha*in_alpha)
	EASINGS_DEFINE_FUNCS(Cubic, in_alpha*in_alpha*in_alpha)
	EASINGS_DEFINE_FUNCS(Quart, in_alpha*in_alpha*in_alpha*in_alpha)
	EASINGS_DEFINE_FUNCS_ONE_ARG(Pow, std::powf(in_alpha, in_exponent), EASINGS_COMMA float in_exponent, EASINGS_COMMA in_exponent)
	
	EASINGS_DEFINE_FUNCS(Sin, (float)std::sin(in_alpha * M_PI * 0.5f))
	EASINGS_DEFINE_FUNCS(Circ, -1.0f * (std::sqrt(1.0f - in_alpha * in_alpha) - 1.0f))
	EASINGS_DEFINE_FUNCS(Expo, (in_alpha == 0.0f) ? 0.0f : std::pow(2.0f, 10.0f * (in_alpha - 1.0f)))

	EASINGS_DEFINE_FUNCS_ONE_ARG(Back, in_alpha * in_alpha * ((in_s + 1.0f) * in_alpha - in_s), EASINGS_COMMA float in_s=1.70158f, EASINGS_COMMA in_s)


	// smoothstep/smootherstep (these are inout by default, so we just define them directly)
	inline float EaseInOutSmoothstep(float in_alpha) {
		return in_alpha * in_alpha * (3 - 2 * in_alpha);
	}
	template<typename T> T InterpInOutSmoothstep(T in_start, T in_end, float in_alpha) {
		return Lerp<T>(in_start, in_end, EaseInOutSmoothstep(in_alpha));
	}
	inline float EaseInOutSmootherstep(float in_alpha) {
		return in_alpha * in_alpha * in_alpha * (in_alpha * (in_alpha * 6 - 15) + 10);
	}
	template<typename T> T InterpInOutSmootherstep(T in_start, T in_end, float in_alpha) {
		return Lerp<T>(in_start, in_end, EaseInOutSmootherstep(in_alpha));
	}

	// task that interpolates linearly from 0 to 1, calling in_tickFunc each frame with the current value
	// to use it with a non-linear curve, use the desired EaseXX() function inside of in_tickFunc
	Task<> Interp(float in_duration, std::function<void(float)> in_tickFunc);
};
