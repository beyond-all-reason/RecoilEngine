/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <memory>

#include "Rendering/GL/FBO.h"

class CWorldDrawer;

/**
 * Extra camera views of the 3D world, each rendered into an offscreen target
 * of its own and composited over the window afterwards.
 *
 * Prototype: the views are drawn one after the other by temporarily
 * pointing the player camera slot at the view's camera. Everything that
 * reads that slot (shader matrices, culling, draw flags) then follows
 * along, and it is put back before the screen/GUI passes.
 * See coding-agents/MULTI_RENDER_TARGETS.md (DSN-01, TGT-01, CAM-01).
 */
class CRenderViews {
public:
	static CRenderViews& GetInstance() {
		static CRenderViews instance;
		return instance;
	}

	// frees the GL targets, must run while the GL context is alive
	void Kill();

	// no-op unless the "SecondaryView" config var is set
	void DrawSecondaryViews(const CWorldDrawer& worldDrawer);

	// true while an extra view is being drawn; Lua draw callins are skipped
	// then, since widgets assume the window is the render target
	static bool InSecondaryView() { return inSecondaryView; }

private:
	bool EnsureTarget(int sizeX, int sizeY);

private:
	inline static bool inSecondaryView = false;

	std::unique_ptr<FBO> target;
	// downscaled copy of the target: blitting with scaling into the
	// (possibly multisampled) window framebuffer is not allowed
	std::unique_ptr<FBO> inset;

	int insetSizeX = 0;
	int insetSizeY = 0;

	int targetSizeX = 0;
	int targetSizeY = 0;
};
