/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#pragma once

#ifdef USE_MIMALLOC
#include <mimalloc.h>
#else
#include <cstdlib>
#include <malloc.h>
#endif

namespace recoil {

// Memory allocation functions
inline void* malloc(size_t size) {
#ifdef USE_MIMALLOC
	return mi_malloc(size);
#else
	return std::malloc(size);
#endif
}

inline void* calloc(size_t count, size_t size) {
#ifdef USE_MIMALLOC
	return mi_calloc(count, size);
#else
	return std::calloc(count, size);
#endif
}

inline void* realloc(void* ptr, size_t size) {
#ifdef USE_MIMALLOC
	return mi_realloc(ptr, size);
#else
	return std::realloc(ptr, size);
#endif
}

inline void free(void* ptr) {
#ifdef USE_MIMALLOC
	mi_free(ptr);
#else
	std::free(ptr);
#endif
}

// Aligned allocation functions
inline void* aligned_alloc(size_t alignment, size_t size) {
#ifdef USE_MIMALLOC
	return mi_aligned_alloc(alignment, size);
#else
	// Platform-specific aligned allocation
	#if defined(_WIN32) || defined(__MINGW32__)
		return _aligned_malloc(size, alignment);
	#else
		return std::aligned_alloc(alignment, size);
	#endif
#endif
}

inline void aligned_free(void* ptr) {
#ifdef USE_MIMALLOC
	mi_free(ptr);  // mimalloc handles aligned free automatically
#else
	#if defined(_WIN32) || defined(__MINGW32__)
		_aligned_free(ptr);
	#else
		std::free(ptr);
	#endif
#endif
}

inline size_t usable_size(void* ptr) {
#ifdef USE_MIMALLOC
	return mi_usable_size(ptr);
#else
	#if defined(_WIN32) || defined(__MINGW32__)
		return _msize(ptr);
	#else
		return malloc_usable_size(ptr);
	#endif
#endif
}

} // namespace recoil
