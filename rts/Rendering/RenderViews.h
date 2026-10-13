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

	// views that are skipped this frame (e.g. the headset asks for no rendering)
	bool active = true;
	// the colour image comes from an OpenXR swapchain each frame, the
	// own colorTex is then only a placeholder keeping the target complete
	bool xrDriven = false;
	GLuint externalColorTex = 0;

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
			active = o.active;
			xrDriven = o.xrDriven;
			externalColorTex = o.externalColorTex;
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

 * The views are drawn one after the other by temporarily
 * pointing the player camera slot at the view's camera (and overriding the
 * view size in globalRendering). Everything that reads those (shader
 * matrices, culling, draw flags) then follows along, and it is put back
 * before the screen/GUI passes.
 *
 * Visibility is computed once per frame for the player camera; the frusta
 * of the extra views are added to it (CCamera::SetExtraCullFrusta) so the
 * result is valid for all of them.
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

	// The direction of the ray under window pixel (x, y), for picking. With a
	// headset, the window is a virtual screen floating in front of the face: its
	// centre is where the head looks, and its height spans the XRPointerFov, so
	// that the mouse moves a pointer through what is seen in the headset.
	// Otherwise it is the camera's own pixel direction.
	static float3 CalcCursorDir(const CCamera* cam, int x, int y);

	// The same pointer ray in the headset's own space, for hitting head locked
	// panels: where it passes a plane one unit in front of the eyes (x right, y up).
	// False when the pointer is not following the headset.
	static bool CalcCursorHeadOffset(int x, int y, float& offsetX, float& offsetY);

	// ends the headset frame begun in Update, after everything was drawn: the
	// UI panels (see CXRPanels) are copied to the headset first
	void EndXRFrame();

private:
	bool EnsureTarget(RenderView& view);
	bool EnsureInset(RenderView& view);

private:
	inline static bool inSecondaryView = false;

	// the pointer: the headset's axes in the world, and where the pointer ray
	// meets the ground (updated per frame in Update)
	inline static bool pointerActive = false;
	inline static float3 pointerForward = FwdVector;
	inline static float3 pointerUp = UpVector;
	inline static float3 pointerRight = RgtVector;
	inline static float pointerTanHalf = 0.5f;

	bool reticleValid = false;
	float3 reticlePos;

	std::vector<RenderView> views;
};
