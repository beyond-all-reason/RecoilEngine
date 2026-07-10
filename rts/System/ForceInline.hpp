#pragma once

// https://meghprkh.github.io/blog/posts/c++-force-inline/

#if defined(__clang__)
#define RECOIL_FORCE_INLINE [[gnu::always_inline]] [[gnu::gnu_inline]] extern inline

#elif defined(__GNUC__)
#define RECOIL_FORCE_INLINE [[gnu::always_inline]] inline

#elif defined(_MSC_VER)
// Do NOT promote C4714 to an error: under LTCG (/GL, used by RelWithDebInfo)
// MSVC defers inlining to link time and reports "__forceinline not inlined"
// harmlessly, which would otherwise abort the link with LNK1257.
#define RECOIL_FORCE_INLINE __forceinline

#else
#error Unsupported compiler
#endif
