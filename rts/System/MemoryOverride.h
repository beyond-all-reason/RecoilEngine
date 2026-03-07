/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#include <cstddef>

namespace recoil {

// Memory allocation functions
// These are implemented in MemoryOverride.cpp
// When USE_MIMALLOC is defined, they use mimalloc
// Otherwise, they use standard library functions

void* malloc(size_t size);
void* calloc(size_t count, size_t size);
void* realloc(void* ptr, size_t size);
void free(void* ptr);

// Aligned allocation functions
void* aligned_alloc(size_t alignment, size_t size);
void* aligned_realloc(void* ptr, size_t oldsize, size_t newsize, size_t alignment);
void aligned_free(void* ptr);

// Get usable size of allocated memory
size_t usable_size(void* ptr);

} // namespace recoil
