/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

// Installs thin wrappers around the glad function pointers for GL texture and
// renderbuffer allocation/deletion, so every such call -- engine, Lua, third-party,
// regardless of call site -- reports its VRAM footprint to Tracy memory pools
// ("GPU / Textures", "GPU / Renderbuffers"). This is the zero-refactor way to get
// engine-wide texture VRAM coverage for measurement.
//
// Call exactly once, right after gladLoadGL() (the glad pointers are mutable globals
// loaded a single time). No-op unless built with TRACY_ENABLE and not HEADLESS.
//
// Accuracy is test-grade: sizes are estimated from dimensions + internal format
// (glTexStorage mip chains are exact; legacy glTexImage tracks per supplied level;
// compressed uploads use the exact imageSize argument). glGenerateMipmap-produced
// levels and driver padding are not seen -- cross-check against the driver VRAM plot.

namespace GL {
	void InstallGpuMemHooks();
}
