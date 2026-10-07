#include "glDebugGroup.hpp"

#include <cstring>

#include "myGL.h"
#include "System/Misc/TracyGpu.h"

GL::DebugGroupImpl::DebugGroupImpl(uint32_t id, const char* messsage)
{
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, id, -1, messsage);
}

GL::DebugGroupImpl::~DebugGroupImpl()
{
	glPopDebugGroup();
}

#ifdef TRACY_ENABLE
namespace GL {
	// A debug group that is also a Tracy GPU zone of the same name. Lives here so that
	// the header, which myGL.h drags into targets without Tracy, stays free of it.
	class DebugGroupTracyImpl final : public DebugGroup {
	public:
		DebugGroupTracyImpl(uint32_t id, const char* message, bool glGroup)
			: glGroup(glGroup)
			, gpuZone(static_cast<uint32_t>(__LINE__), __FILE__, strlen(__FILE__), __func__, strlen(__func__), message, strlen(message), true)
		{
			if (glGroup)
				glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, id, -1, message);
		}
		~DebugGroupTracyImpl() override final
		{
			if (glGroup)
				glPopDebugGroup();
		}
	private:
		bool glGroup;
		tracy::GpuCtxScope gpuZone;
	};
}
#endif

std::unique_ptr<GL::DebugGroup> GL::DebugGroup::GetScoped(uint32_t id, const char* messsage)
{
#ifdef TRACY_ENABLE
	if (TracyGpu::ready)
		return std::make_unique<GL::DebugGroupTracyImpl>(id, messsage, GLAD_GL_KHR_debug);
#endif

	if (GLAD_GL_KHR_debug)
		return std::make_unique<GL::DebugGroupImpl>(id, messsage);
	else
		return std::make_unique<GL::DebugGroupNoop>(id, messsage);
}
