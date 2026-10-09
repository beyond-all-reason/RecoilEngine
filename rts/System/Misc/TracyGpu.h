/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

// Tracy GPU (OpenGL timer query) zones. TracyOpenGL.hpp needs the GL
// function pointers declared before it is included, hence glad first.
// Without TRACY_ENABLE every Tracy macro expands to nothing. Only include
// this from .cpp files of targets that link Tracy (myGL.h reaches targets
// that do not, such as the Lua library).
#include <glad/glad.h>
#include "TracyDefs.h"
#include <tracy/TracyOpenGL.hpp>

namespace TracyGpu {
	// true once CGlobalRendering created the Tracy GPU context (needs
	// ARB_timer_query); zones created before that or without it are inactive
	extern bool ready;
}

// A GPU zone with a runtime name, inactive until the GPU context exists.
#define TRACY_GPU_ZONE_TRANSIENT(varname, name) TracyGpuZoneTransient(varname, name, TracyGpu::ready)
