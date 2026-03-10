/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <cstddef>
#include <cstdlib>

namespace recoil {

// Memory allocation functions
// These are implemented in MemoryOverride.cpp
// When USE_MIMALLOC is enabled, they use mimalloc
// Otherwise, they use standard library functions

#if !defined(USE_MIMALLOC) || !USE_MIMALLOC
// Inline implementations for when mimalloc is not used
static inline void* malloc(size_t size) { return std::malloc(size); }
static inline void free(void* ptr) { return std::free(ptr); }
#else
void* malloc(size_t size);
void free(void* ptr);
#endif

void* calloc(size_t count, size_t size);
void* realloc(void* ptr, size_t size);

// Aligned allocation functions
void* aligned_alloc(size_t alignment, size_t size);
void* aligned_realloc(void* ptr, size_t oldsize, size_t newsize, size_t alignment);
void aligned_free(void* ptr);

// Get usable size of allocated memory
size_t usable_size(void* ptr);

} // namespace recoil
