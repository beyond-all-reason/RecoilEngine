#pragma once

#include <memory>
#include <cstdint>
#include "System/StringHash.h"
#include "System/StringUtil.h"

// Gated on TRACY_ENABLE, not just HEADLESS: include TracyOpenGL.hpp only where
// TracyClient is linked. Sublibs that pull in myGL.h without it (e.g. the bundled
// lua lib) must not see this include.
#if !defined(HEADLESS) && defined(TRACY_ENABLE)
#include <glad/glad.h> // TracyOpenGL.hpp needs GL symbols; keep this header self-contained
#include <tracy/TracyOpenGL.hpp>
#endif

namespace GL {
	class DebugGroup {
	public:
		virtual ~DebugGroup() {};
		static std::unique_ptr<DebugGroup> GetScoped(uint32_t id, const char* messsage);
	};
	class DebugGroupNoop : public DebugGroup {
	public:
		DebugGroupNoop(uint32_t id, const char* messsage) {}
	};
	class DebugGroupImpl : public DebugGroup {
	public:
		DebugGroupImpl(uint32_t id, const char* messsage);
		~DebugGroupImpl() override final;
	};
}

#define SCOPED_GL_DEBUGGROUP(name) const auto _UTIL_CONCAT(__scopedGLDebugGroup, __LINE__) = GL::DebugGroup::GetScoped(0x824A/*GL_DEBUG_SOURCE_APPLICATION*/ ,name)

// Render-pass annotation: emits a KHR_debug marker (RenderDoc/Nsight) and a Tracy
// GPU zone, for real GPU-work boundaries only. Stays a call-site macro (Tracy keys
// on __LINE__/__FILE__); `name` must be a string literal (backs a constexpr Tracy
// SourceLocationData). Falls back to the plain debug group under HEADLESS/no Tracy.
#if !defined(HEADLESS) && defined(TRACY_ENABLE)
	#define SCOPED_GL_GPU_ZONE(name)                                    \
		SCOPED_GL_DEBUGGROUP(name);                                     \
		TracyGpuNamedZone(_UTIL_CONCAT(__scopedGpuZone, __LINE__), name, true)
#else
	#define SCOPED_GL_GPU_ZONE(name) SCOPED_GL_DEBUGGROUP(name)
#endif