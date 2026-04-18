#ifndef _SIMD_COMPAT_H
#define _SIMD_COMPAT_H

#ifdef SSE2NEON
    #include "lib/sse2neon/sse2neon.h"
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
