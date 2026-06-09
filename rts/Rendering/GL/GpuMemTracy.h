/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

// GPU memory tracking via Tracy named memory pools. Each tracked GL object
// (buffers for now; textures/RBOs later) reports its VRAM footprint to a pool
// keyed by the GL object name, surfacing in Tracy's Memory window with a live
// bytes timeline and allocation call-stacks.
//
// Gated on TRACY_ENABLE only: memory events need no GL symbols, unlike
// glDebugGroup.hpp which gates on HEADLESS to keep TracyOpenGL.hpp out of sublibs
// that lack a GL context. Compiles to a no-op otherwise.
//
// IMPORTANT: only call these for real GL allocations. Sysmem fallback paths (e.g.
// VBO without buffer-object support) are already accounted for by the global
// operator new/delete tracking and must not be double-counted here.

#if defined(TRACY_ENABLE)

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <tracy/Tracy.hpp>

namespace GL {
	// Named memory pools shown in the Tracy "Memory" window.
	inline constexpr const char* GpuMemPoolBuffers     = "GPU / GL Buffers";
	inline constexpr const char* GpuMemPoolLuaTextures = "GPU / Lua Textures";
	inline constexpr const char* GpuMemPoolLuaRBOs     = "GPU / Lua RBOs";

	// Engine-wide pools populated by the glad-level hooks (GpuMemTracyHooks). These
	// catch ALL textures/renderbuffers regardless of call site, including the Lua
	// ones already in the pools above -- so the Lua pools are a *subset* view, not an
	// addend. For a grand VRAM total sum Buffers + Textures + Renderbuffers only.
	inline constexpr const char* GpuMemPoolTextures      = "GPU / Textures";
	inline constexpr const char* GpuMemPoolRenderbuffers = "GPU / Renderbuffers";

	// Tracy keys allocations by pointer; synthesize a unique-per-pool key from the
	// GL object id. Keys are per-pool, so id reuse across pools never collides.
	//
	// Tracy's server is strict: freeing an address it never saw allocated, or
	// allocating an address that is already live, is a FATAL trace failure that
	// halts the whole capture (it is only silently ignored in on-demand mode). Call
	// sites cannot guarantee perfect Alloc/Free symmetry -- a zero estimated size
	// skips the alloc, GL ids get recycled, objects may be deleted through paths we
	// do not see -- so we enforce balance centrally here: track the set of live ids
	// per pool, emit at most one Alloc per id and a Free only for a live id. This
	// makes every pool self-consistent regardless of call-site behaviour.
	namespace detail {
		inline std::mutex& GpuMemMutex()
		{
			static std::mutex m;
			return m;
		}
		inline std::unordered_map<const char*, std::unordered_set<uint32_t>>& GpuMemLive()
		{
			static std::unordered_map<const char*, std::unordered_set<uint32_t>> m;
			return m;
		}
	}

	// Callstack capture depth for GPU memory events. The plain TracyAllocN/FreeN
	// macros record a callstack only when the global TRACY_CALLSTACK depth is defined
	// (it is OFF here, to avoid forcing callstacks on every engine-wide alloc); the
	// ...NS variants take an explicit depth and capture whenever TRACY_HAS_CALLSTACK
	// is available (libunwind on Linux, dbghelp on Windows), so the Memory window
	// shows where each GPU allocation came from. Degrades to no-callstack otherwise.
	inline constexpr int GpuMemCallstackDepth = 24;

	inline void GpuMemAlloc(const char* pool, uint32_t glId, std::size_t size)
	{
		if (size == 0)
			return;
		std::lock_guard<std::mutex> lk(detail::GpuMemMutex());
		if (!detail::GpuMemLive()[pool].insert(glId).second)
			return; // already live: avoid an allocate-twice failure
		TracyAllocNS(reinterpret_cast<void*>(static_cast<uintptr_t>(glId)), size, GpuMemCallstackDepth, pool);
	}

	inline void GpuMemFree(const char* pool, uint32_t glId)
	{
		std::lock_guard<std::mutex> lk(detail::GpuMemMutex());
		if (detail::GpuMemLive()[pool].erase(glId) == 0)
			return; // never tracked: avoid a free-of-untracked failure
		TracyFreeNS(reinterpret_cast<void*>(static_cast<uintptr_t>(glId)), GpuMemCallstackDepth, pool);
	}
}

#define GPU_MEM_ALLOC(pool, glId, size) GL::GpuMemAlloc(pool, glId, size)
#define GPU_MEM_FREE(pool, glId)        GL::GpuMemFree(pool, glId)

#else

#define GPU_MEM_ALLOC(pool, glId, size) ((void)0)
#define GPU_MEM_FREE(pool, glId)        ((void)0)

#endif
