/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

// TEMP diagnostic, not for merge. Captures the framebuffer between draw phases
// so a stray polygon can be attributed to the phase that drew it.
//
// Off unless SPRING_PHASE_DUMP is set. Writes quarter-size PPMs to
// SPRING_PHASE_DUMP_DIR (default /tmp), one set every 120 calls per label.

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "System/Log/ILog.h"

inline void DumpFramePhase(const char* label, int everyNth = 120)
{
#ifndef HEADLESS
	static const char* enabled = getenv("SPRING_PHASE_DUMP");
	if (enabled == nullptr)
		return;

	// Each dump is a full-framebuffer glReadPixels, which forces a command
	// stream submission and so hides the very bug this is used to hunt. Set
	// SPRING_PHASE_DUMP to a label substring to capture only that phase.
	// "7-presented" is the cheap one: the macOS present path reads the whole
	// framebuffer back a moment later anyway, so it adds almost nothing.
	if (enabled[0] != '1' && strstr(label, enabled) == nullptr)
		return;

	// key on the draw frame, not on a call counter. Per-label counters drift,
	// because SwapBuffers runs during loading and menus while the draw phases
	// do not, so same-index files from two labels are different moments and
	// comparing them says nothing.
	const unsigned int frame = globalRendering->drawFrame;

	// SPRING_PHASE_DUMP_EVERY raises the sample rate without a rebuild. The
	// artefact varies frame to frame, so a run needs enough frames to compare
	// as a distribution rather than as a single peak.
	static const char* everyEnv = getenv("SPRING_PHASE_DUMP_EVERY");
	if (everyEnv != nullptr && atoi(everyEnv) > 0)
		everyNth = atoi(everyEnv);

	if ((frame % everyNth) != 0)
		return;

	const int w = globalRendering->viewSizeX;
	const int h = globalRendering->viewSizeY;
	if (w <= 0 || h <= 0)
		return;

	static std::vector<uint8_t> pixels;
	pixels.resize(static_cast<size_t>(w) * h * 4);

	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

	const char* outDir = getenv("SPRING_PHASE_DUMP_DIR");
	if (outDir == nullptr)
		outDir = "/tmp";

	char path[1024];
	snprintf(path, sizeof(path), "%s/frame_%06u_%s.ppm", outDir, frame, label);

	FILE* f = fopen(path, "wb");
	if (f == nullptr)
		return;

	const int step = 4;
	fprintf(f, "P6\n%d %d\n255\n", w / step, h / step);

	// glReadPixels gives bottom-up rows, PPM wants top-down
	for (int y = h - step; y >= 0; y -= step) {
		for (int x = 0; x < w - step + 1; x += step) {
			const uint8_t* p = &pixels[(static_cast<size_t>(y) * w + x) * 4];
			fwrite(p, 1, 3, f);
		}
	}

	fclose(f);
	LOG("[PhaseDump] wrote %s", path);
#endif
}
