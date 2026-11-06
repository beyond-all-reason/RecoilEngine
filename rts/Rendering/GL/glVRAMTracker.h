/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

namespace GL {
	// Video memory use in Tracy builds. Plots the driver's numbers (used memory, NVIDIA evictions)
	// and on Windows this process's usage and budget as the OS reports them (DXGI). The texture,
	// buffer and renderbuffer memory the engine and Lua allocated goes to Tracy's memory view per
	// GL object, in the pools "VRAM: textures", "VRAM: buffers" and "VRAM: renderbuffers" (plotted too).
	// Allocations are tracked by wrapping the GL functions that allocate; sizes are estimated from
	// the internal formats. Does nothing in builds without Tracy.
	namespace VRAMTracker {
		void Init(); // with the context current, after the GL functions are loaded
		void Update(); // once per frame
		void Kill(); // while the context is still current
	}
}
