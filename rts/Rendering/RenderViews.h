/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "Game/Camera.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"

class CWorldDrawer;

/**
 * One extra view of the 3D world: its own camera (explicit pose and
 * frustum), and the offscreen target it renders into, which can have a
 * different size than the window.
 */
struct RenderView {
	// idle camera state; typed VISCUL so that updating it does not touch the
	// shared culling camera, it is put into the player slot only while drawn
	// (not called "camera": that is a macro in Camera.h)
	CCamera viewCam{CCamera::CAMTYPE_VISCUL};

	int sizeX = 0;
	int sizeY = 0;

	// the target is texture backed, so that it can be handed to something
	// else afterwards (a compositor submit, a shader sampling it)
	GLuint colorTex = 0;
	GLuint depthTex = 0;
	std::unique_ptr<FBO> target;

	// downscaled copy of the target for showing it in the window: blitting
	// with scaling into the (possibly multisampled) window framebuffer is
	// not allowed
	std::unique_ptr<FBO> inset;
	int insetSizeX = 0;
	int insetSizeY = 0;

	RenderView() = default;
	RenderView(const RenderView&) = delete;
	RenderView& operator = (const RenderView&) = delete;
	RenderView(RenderView&& o) noexcept { *this = std::move(o); }
	RenderView& operator = (RenderView&& o) noexcept
	{
		if (this != &o) {
			FreeTarget();
			viewCam = o.viewCam;
			sizeX = o.sizeX;
			sizeY = o.sizeY;
			colorTex = std::exchange(o.colorTex, 0);
			depthTex = std::exchange(o.depthTex, 0);
			target = std::move(o.target);
			inset = std::move(o.inset);
			insetSizeX = o.insetSizeX;
			insetSizeY = o.insetSizeY;
		}
		return *this;
	}
	~RenderView() { FreeTarget(); }

	void FreeTarget()
	{
		// the inset is sized separately and re-checked by EnsureInset
		target = nullptr;
		if (colorTex != 0)
			glDeleteTextures(1, &colorTex);
		if (depthTex != 0)
			glDeleteTextures(1, &depthTex);
		colorTex = 0;
		depthTex = 0;
	}
};

/**
 * Extra camera views of the 3D world, each rendered into an offscreen target
 * of its own and composited over the window afterwards.
 *
 * Prototype: the views are drawn one after the other by temporarily
 * pointing the player camera slot at the view's camera (and overriding the
 * view size in globalRendering). Everything that reads those (shader
 * matrices, culling, draw flags) then follows along, and it is put back
 * before the screen/GUI passes.
 *
 * Visibility is computed once per frame for the player camera; the frusta
 * of the extra views are added to it (CCamera::SetExtraCullFrusta) so the
 * result is valid for all of them.
 *
 * See coding-agents/MULTI_RENDER_TARGETS.md (DSN-01, TGT-01, CAM-01, OBJ-01).
 */
class CRenderViews {
public:
	static CRenderViews& GetInstance() {
		static CRenderViews instance;
		return instance;
	}

	// frees the GL targets, must run while the GL context is alive
	void Kill();

	// per draw frame, after the player camera was updated and before the
	// world drawer computes visibility: sets up the views' cameras and the
	// union cull
	void Update();

	// no-op unless the "SecondaryView" config var is set
	void DrawSecondaryViews(const CWorldDrawer& worldDrawer);

	// true while an extra view is being drawn; Lua draw callins are skipped
	// then, since widgets assume the window is the render target
	static bool InSecondaryView() { return inSecondaryView; }

private:
	bool EnsureTarget(RenderView& view);
	bool EnsureInset(RenderView& view);

private:
	inline static bool inSecondaryView = false;

	std::vector<RenderView> views;
};
