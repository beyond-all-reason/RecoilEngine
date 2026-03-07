/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifdef USE_MIMALLOC

#include "MemoryOverride.h"
#include <mimalloc.h>
#include <new>

// ----------------------------------------------------------------------------
// Custom operator new/delete overrides using mimalloc
// This file should be compiled only once into the engine
// Based on mimalloc-new-delete.h from mimalloc distribution
// ----------------------------------------------------------------------------

// C++98 delete operators
void operator delete(void* p) noexcept
{
	mi_free(p);
}

void operator delete[](void* p) noexcept
{
	mi_free(p);
}

// C++98 no-throw delete operators
void operator delete(void* p, const std::nothrow_t&) noexcept
{
	mi_free(p);
}

void operator delete[](void* p, const std::nothrow_t&) noexcept
{
	mi_free(p);
}

// C++98 new operators
void* operator new(std::size_t n) noexcept(false)
{
	return mi_new(n);
}

void* operator new[](std::size_t n) noexcept(false)
{
	return mi_new(n);
}

// C++98 no-throw new operators
void* operator new(std::size_t n, const std::nothrow_t& tag) noexcept
{
	(void)(tag);
	return mi_new_nothrow(n);
}

void* operator new[](std::size_t n, const std::nothrow_t& tag) noexcept
{
	(void)(tag);
	return mi_new_nothrow(n);
}

// C++14 sized deallocation operators
#if (__cplusplus >= 201402L || _MSC_VER >= 1916)
void operator delete(void* p, std::size_t n) noexcept
{
	mi_free_size(p, n);
}

void operator delete[](void* p, std::size_t n) noexcept
{
	mi_free_size(p, n);
}
#endif

// C++17 aligned new/delete operators
#if (__cplusplus > 201402L || defined(__cpp_aligned_new))
void operator delete(void* p, std::align_val_t al) noexcept
{
	mi_free_aligned(p, static_cast<size_t>(al));
}

void operator delete[](void* p, std::align_val_t al) noexcept
{
	mi_free_aligned(p, static_cast<size_t>(al));
}

void operator delete(void* p, std::size_t n, std::align_val_t al) noexcept
{
	mi_free_size_aligned(p, n, static_cast<size_t>(al));
}

void operator delete[](void* p, std::size_t n, std::align_val_t al) noexcept
{
	mi_free_size_aligned(p, n, static_cast<size_t>(al));
}

void operator delete(void* p, std::align_val_t al, const std::nothrow_t&) noexcept
{
	mi_free_aligned(p, static_cast<size_t>(al));
}

void operator delete[](void* p, std::align_val_t al, const std::nothrow_t&) noexcept
{
	mi_free_aligned(p, static_cast<size_t>(al));
}

void* operator new(std::size_t n, std::align_val_t al) noexcept(false)
{
	return mi_new_aligned(n, static_cast<size_t>(al));
}

void* operator new[](std::size_t n, std::align_val_t al) noexcept(false)
{
	return mi_new_aligned(n, static_cast<size_t>(al));
}

void* operator new(std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept
{
	return mi_new_aligned_nothrow(n, static_cast<size_t>(al));
}

void* operator new[](std::size_t n, std::align_val_t al, const std::nothrow_t&) noexcept
{
	return mi_new_aligned_nothrow(n, static_cast<size_t>(al));
}
#endif

#endif // USE_MIMALLOC
