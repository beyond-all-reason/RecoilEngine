/* Copyright (C) 2008 Tobi Vollebregt */

/* Conditionally include streflop depending on STREFLOP_* #defines:
   If one of those is present, #include "streflop.h", otherwise #include <math.h>

   When faced with ambiguous call errors with e.g. fabs, use math::function.
   Add it to math namespace if it doesn't exist there yet. */

#ifndef STREFLOP_COND_H
#define STREFLOP_COND_H

#if (!defined(NOT_USING_STREFLOP) && (defined(STREFLOP_SSE) || defined(STREFLOP_NEON) || defined(STREFLOP_X87) || defined(STREFLOP_SOFT)))
#define STREFLOP_ENABLED 1
#endif

// these need to be known in FastMath.h and SpringMath.h which both include us
#ifdef __GNUC__
	#define _const __attribute__((const))
	#define _pure __attribute__((pure))
	#define _warn_unused_result __attribute__((warn_unused_result))
#else
	#define _const
	#define _pure
	#define _warn_unused_result
#endif



#if STREFLOP_ENABLED
#include "streflop.h"

namespace math {
	using namespace streflop;

	// Explicit overloads for raw float/double types to resolve ambiguity
	// when called from template code (e.g., assimp) that uses float/double
	// Note: math::sqrt(float) and math::sqrt(int) are always provided by FastMath.h
	// (via fastmath::sqrt_sse). Do NOT define them here — it causes redefinition
	// errors when streflop_cond.h is included before FastMath.h (e.g., lmathlib.cpp).
	// The using-directive above imports streflop::sqrt as a fallback for TUs without FastMath.h.
	inline float fabs(float x) { return streflop::fabs(Simple(x)); }
	inline double sqrt(double x) { return streflop::sqrt(Double(x)); }
	inline double fabs(double x) { return streflop::fabs(Double(x)); }
	inline float sin(float x) { return streflop::sin(Simple(x)); }
	inline double sin(double x) { return streflop::sin(Double(x)); }
	inline float cos(float x) { return streflop::cos(Simple(x)); }
	inline double cos(double x) { return streflop::cos(Double(x)); }
	inline float acos(float x) { return streflop::acos(Simple(x)); }
	inline double acos(double x) { return streflop::acos(Double(x)); }
	inline float asin(float x) { return streflop::asin(Simple(x)); }
	inline double asin(double x) { return streflop::asin(Double(x)); }
	inline float atan(float x) { return streflop::atan(Simple(x)); }
	inline double atan(double x) { return streflop::atan(Double(x)); }
	inline float atan2(float y, float x) { return streflop::atan2(Simple(y), Simple(x)); }
	inline double atan2(double y, double x) { return streflop::atan2(Double(y), Double(x)); }
	inline float tan(float x) { return streflop::tan(Simple(x)); }
	inline double tan(double x) { return streflop::tan(Double(x)); }
	inline float pow(float x, float y) { return streflop::pow(Simple(x), Simple(y)); }
	inline double pow(double x, double y) { return streflop::pow(Double(x), Double(y)); }
	inline float exp(float x) { return streflop::exp(Simple(x)); }
	inline double exp(double x) { return streflop::exp(Double(x)); }
	inline float log(float x) { return streflop::log(Simple(x)); }
	inline double log(double x) { return streflop::log(Double(x)); }
	inline float floor(float x) { return streflop::floor(Simple(x)); }
	inline double floor(double x) { return streflop::floor(Double(x)); }
	inline float ceil(float x) { return streflop::ceil(Simple(x)); }
	inline double ceil(double x) { return streflop::ceil(Double(x)); }
	inline int isnan(float x) { return streflop::isnan(Simple(x)); }
	inline int isnan(double x) { return streflop::isnan(Double(x)); }
	inline int isinf(float x) { return streflop::isinf(Simple(x)); }
	inline int isinf(double x) { return streflop::isinf(Double(x)); }
}

#else
#include <cmath>

namespace streflop {
	typedef float Simple;
	typedef double Double;
	template<typename T> void streflop_init() {}
};



#ifdef __APPLE__
// macosx's cmath doesn't include c++11's std::hypot yet (tested 2013)
namespace std {
	template<typename T> T hypot(T x, T y);
};
#endif



namespace math {
	using std::fabs;

	// see FastMath NOTE below
	// using std::sqrt;

	using std::sin;
	using std::cos;

	using std::sinh;
	using std::cosh;
	using std::tan;
	using std::tanh;
	using std::asin;
	using std::acos;
	using std::atan;
	using std::atan2;

	using std::ceil;
	using std::floor;
	using std::fmod;
	using std::hypot;
	using std::pow;
	using std::log;
	using std::log2;
	using std::log10;
	using std::exp;
	using std::frexp;
	using std::ldexp;
	using std::round;
	using std::erf;
	using std::cbrt;

	// these are in streflop:: but not in std::, FastMath adds sqrtf
	// static inline float sqrtf(float x) { return std::sqrt(x); }
	static inline float cosf(float x) { return std::cos(x); }
	static inline float sinf(float x) { return std::sin(x); }
	static inline float tanf(float x) { return std::tan(x); }
	static inline float acosf(float x) { return std::acos(x); }
	static inline float fabsf(float x) { return std::fabs(x); }
	static inline float roundf(float x) { return std::round(x); }
	static inline float floorf(float x) { return std::floor(x); }
	static inline float ceilf(float x) { return std::ceil(x); }
	static inline float cbrtf(float x) { return std::cbrt(x); }


// the following are C99 functions -> not supported by VS C
#if !defined(_MSC_VER) || _MSC_VER < 1500
	using std::isnan;
	using std::isinf;
	using std::isfinite;
#else
}

#include <limits>
namespace math {
	template<typename T> inline bool isnan(T value) {
		return value != value;
	}
	// requires include <limits>
	template<typename T> inline bool isinf(T value) {
		return std::numeric_limits<T>::has_infinity && value == std::numeric_limits<T>::infinity();
	}
	// requires include <limits>
	template<typename T> inline bool isfinite(T value) {
		return !isinf<T>(value);
	}
#endif
}



// NOTE:
//   for non-streflop builds we replace std::sqrt by fastmath::sqrt_sse in math::
//   any code that only includes streflop_cond.h (assimp, etc) and not FastMath.h
//   would not know about this without also including the latter, do so here
#include "System/FastMath.h"



#ifdef __APPLE__
#include <algorithm>

namespace std {
	template<typename T>
	T hypot(T x, T y) {
		x = std::abs(x);
		y = std::abs(y);
		auto t = std::min(x, y);
		     x = std::max(x, y);
		t = t / x;
		return x * math::sqrtf(1.0f + t*t);
	}
}
#endif



#endif // STREFLOP_ENABLED

#endif // STREFLOP_COND_H
