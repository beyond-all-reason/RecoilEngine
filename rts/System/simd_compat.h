#ifndef _SIMD_COMPAT_H
#define _SIMD_COMPAT_H

#ifdef SSE2NEON
    #include "lib/sse2neon/sse2neon.h"
#else
    #include <x86intrin.h>
    #include <immintrin.h>
    #include <xmmintrin.h>
    #include <emmintrin.h>
#endif

#endif // _SIMD_COMPAT_H
