#ifndef _SIMD_COMPAT_H
#define _SIMD_COMPAT_H

#ifdef SSE2NEON
    #include "lib/sse2neon/sse2neon.h"
    // sse2neon.h includes <fenv.h> purely to implement its rounding-mode
    // helpers; it captures the FE_XXX values into its own inline code at this
    // point and exposes only _MM_ROUND_* publicly. The leaked system FE_XXX
    // macros, however, collide with the ones streflop (re)defines for its NEON
    // FPU control, making streflop emit a redefinition #warning in every TU
    // that includes it afterwards. Drop the leaked macros to contain the leak
    // at this boundary; streflop then sees a clean slate and defines its own.
    #undef FE_INVALID
    #undef FE_DENORMAL
    #undef FE_DIVBYZERO
    #undef FE_OVERFLOW
    #undef FE_UNDERFLOW
    #undef FE_INEXACT
    #undef FE_ALL_EXCEPT
    #undef FE_TONEAREST
    #undef FE_DOWNWARD
    #undef FE_UPWARD
    #undef FE_TOWARDZERO
#else
    #ifdef _MSC_VER
        #include <intrin.h>   // MSVC umbrella
    #else
        #include <x86intrin.h> // GCC / Clang umbrella
    #endif
    #include <immintrin.h>
    #include <xmmintrin.h>
    #include <emmintrin.h>
#endif

#endif // _SIMD_COMPAT_H
