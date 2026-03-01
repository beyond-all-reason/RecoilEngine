/*
 * Double-precision (dbl-64) bridge for streflop_libm.
 *
 * streflop only bundles single-precision (flt-32) libm implementations.
 * On Linux x86_64, the double-precision streflop_libm::__ieee754_*(double)
 * symbols happen to resolve from glibc's exported internal symbols. On macOS
 * (and other non-glibc platforms), these are undefined.
 *
 * This file provides all double-precision streflop_libm functions by
 * delegating to system <cmath>. This is safe because:
 * 1. Double-precision is NOT sync-critical (engine uses float/Simple for sync)
 * 2. On Linux, these already resolve to glibc's system libm anyway
 * 3. The C++ mangled names in streflop_libm:: don't conflict with C libm symbols
 */

#include <cmath>
#include <cfloat>
#include <climits>

// Get the streflop type definitions (Double = double for SSE/NEON/X87 modes)
#include "../streflop.h"

using streflop::Double;

// On GCC/Linux, <cmath> puts classification functions (signbit, fpclassify,
// isnan, isinf) in std:: only. On macOS/clang they're macros in the global
// namespace. Use std:: versions unconditionally since <cmath> is included.
using std::signbit;
using std::fpclassify;
using std::isnan;
using std::isinf;

namespace streflop_libm {

// Square root, cube root, hypotenuse
Double __ieee754_sqrt(Double x) { return ::sqrt(x); }
Double __cbrt(Double x) { return ::cbrt(x); }
Double __ieee754_hypot(Double x, Double y) { return ::hypot(x, y); }

// Exponential and logarithmic
Double __ieee754_exp(Double x) { return ::exp(x); }
Double __ieee754_log(Double x) { return ::log(x); }
Double __ieee754_log2(Double x) { return ::log2(x); }
Double __ieee754_exp2(Double x) { return ::exp2(x); }
Double __ieee754_log10(Double x) { return ::log10(x); }
Double __ieee754_pow(Double x, Double y) { return ::pow(x, y); }

// Trigonometric
Double __sin(Double x) { return ::sin(x); }
Double __cos(Double x) { return ::cos(x); }
Double tan(Double x) { return ::tan(x); }
Double __ieee754_acos(Double x) { return ::acos(x); }
Double __ieee754_asin(Double x) { return ::asin(x); }
Double atan(Double x) { return ::atan(x); }
Double __ieee754_atan2(Double x, Double y) { return ::atan2(x, y); }

// Hyperbolic
Double __ieee754_cosh(Double x) { return ::cosh(x); }
Double __ieee754_sinh(Double x) { return ::sinh(x); }
Double __tanh(Double x) { return ::tanh(x); }
Double __ieee754_acosh(Double x) { return ::acosh(x); }
Double __asinh(Double x) { return ::asinh(x); }
Double __ieee754_atanh(Double x) { return ::atanh(x); }

// Absolute value, rounding
Double __fabs(Double x) { return ::fabs(x); }
Double __floor(Double x) { return ::floor(x); }
Double __ceil(Double x) { return ::ceil(x); }
Double __trunc(Double x) { return ::trunc(x); }
Double __ieee754_fmod(Double x, Double y) { return ::fmod(x, y); }
Double __ieee754_remainder(Double x, Double y) { return ::remainder(x, y); }
Double __remquo(Double x, Double y, int *quo) { return ::remquo(x, y, quo); }
Double __rint(Double x) { return ::rint(x); }
long int __lrint(Double x) { return ::lrint(x); }
long long int __llrint(Double x) { return ::llrint(x); }
Double __round(Double x) { return ::round(x); }
long int __lround(Double x) { return ::lround(x); }
long long int __llround(Double x) { return ::llround(x); }
Double __nearbyint(Double x) { return ::nearbyint(x); }

// Decomposition and scaling
Double __frexp(Double x, int *exp) { return ::frexp(x, exp); }
Double __ldexp(Double value, int exp) { return ::ldexp(value, exp); }
Double __logb(Double x) { return ::logb(x); }
int __ilogb(Double x) { return ::ilogb(x); }
// Note: streflop declares copysign with 1 arg (likely dead code; standard takes 2)
Double __copysign(Double x) { return ::fabs(x); }
int __signbit(Double x) { return ::signbit(x); }
Double __nextafter(Double x, Double y) { return ::nextafter(x, y); }

// Special functions
Double __expm1(Double x) { return ::expm1(x); }
Double __log1p(Double x) { return ::log1p(x); }
Double __erf(Double x) { return ::erf(x); }
// POSIX Bessel functions: available on Linux/macOS but not on Windows (MSVCRT).
// These are only used for double-precision paths which are not sync-critical.
#if !defined(_WIN32)
Double __ieee754_j0(Double x) { return ::j0(x); }
Double __ieee754_j1(Double x) { return ::j1(x); }
Double __ieee754_jn(int n, Double x) { return ::jn(n, x); }
Double __ieee754_y0(Double x) { return ::y0(x); }
Double __ieee754_y1(Double x) { return ::y1(x); }
Double __ieee754_yn(int n, Double x) { return ::yn(n, x); }
#endif
Double __scalbn(Double x, int n) { return ::scalbn(x, n); }
Double __scalbln(Double x, long int n) { return ::scalbln(x, n); }

// Classification
int __fpclassify(Double x) { return ::fpclassify(x); }
int __isnanl(Double x) { return ::isnan(x); }
int __isinf(Double x) { return ::isinf(x); }

} // namespace streflop_libm
