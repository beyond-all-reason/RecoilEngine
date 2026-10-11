/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RenderViews.h"

#include <algorithm>

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/UniformConstants.h"
#include "Rendering/WorldDrawer.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Config/ConfigHandler.h"
#include "System/EventHandler.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"

#include "System/Misc/TracyDefs.h"

CONFIG(bool, SecondaryView)
	.defaultValue(false)
	.description("Render a second camera into an offscreen target, shown as an inset. Prototype for multi-window rendering.");
CONFIG(int, SecondaryViewYaw)
	.defaultValue(40)
	.description("Yaw offset in degrees of the secondary view's camera relative to the player camera.");
CONFIG(int, SecondaryViewOffset)
	.defaultValue(0)
	.description("Offset in elmos of the secondary view's camera along the player camera's right axis (an eye-separation stand-in).");
CONFIG(int, SecondaryViewShift)
	.defaultValue(0)
	.description("Horizontal lens shift of the secondary view in percent of its half-width, makes the frustum asymmetric like a headset eye.");
CONFIG(int, SecondaryViewScale)
	.defaultValue(100)
	.minimumValue(10)
	.maximumValue(200)
	.description("Size of the secondary view's target in percent of the window.");
CONFIG(bool, SecondaryViewDump)
	.defaultValue(false)
	.description("Saves the next rendered image of the secondary view to screenshots/mrt_view<N>.png, then resets itself to 0.");


namespace {
	// Overrides the view size state in globalRendering for the duration of
	// a view's pass, so that everything derived from it (aspect ratio,
	// viewports, pixel sizes) matches the view's target instead of the window.
	class ViewSizeScope {
	public:
		ViewSizeScope(int sizeX, int sizeY)
			: viewPosX(globalRendering->viewPosX)
			, viewPosY(globalRendering->viewPosY)
			, viewSizeX(globalRendering->viewSizeX)
			, viewSizeY(globalRendering->viewSizeY)
			, pixelX(globalRendering->pixelX)
			, pixelY(globalRendering->pixelY)
			, aspectRatio(globalRendering->aspectRatio)
		{
			globalRendering->viewPosX = 0;
			globalRendering->viewPosY = 0;
			globalRendering->viewSizeX = sizeX;
			globalRendering->viewSizeY = sizeY;
			globalRendering->pixelX = 1.0f / sizeX;
			globalRendering->pixelY = 1.0f / sizeY;
			globalRendering->aspectRatio = float(sizeX) / float(sizeY);
		}
		~ViewSizeScope()
		{
			globalRendering->viewPosX = viewPosX;
			globalRendering->viewPosY = viewPosY;
			globalRendering->viewSizeX = viewSizeX;
			globalRendering->viewSizeY = viewSizeY;
			globalRendering->pixelX = pixelX;
			globalRendering->pixelY = pixelY;
			globalRendering->aspectRatio = aspectRatio;
		}

		ViewSizeScope(const ViewSizeScope&) = delete;
		ViewSizeScope& operator = (const ViewSizeScope&) = delete;

	private:
		int viewPosX, viewPosY, viewSizeX, viewSizeY;
		float pixelX, pixelY, aspectRatio;
	};

	// debugging aid: saves the (currently bound) view target, so that what the view
	// rendered can be told apart from how it was shown in the window
	void DumpBoundTarget(const RenderView& view, size_t viewIdx)
	{
		std::vector<uint8_t> pixels(size_t(view.sizeX) * view.sizeY * 4);

		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, view.sizeX, view.sizeY, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

		CBitmap bmp(pixels.data(), view.sizeX, view.sizeY);
		bmp.ReverseYAxis();
		bmp.Save("screenshots/mrt_view" + std::to_string(viewIdx) + ".png", true, true);
	}

	void UploadPlayerCameraMatrices()
	{
		UniformConstants::GetInstance().UpdateMatrices();
		UniformConstants::GetInstance().Bind();
	}
}


void CRenderViews::Kill()
{
	views.clear();
	inset = nullptr;
	insetSizeX = 0;
	insetSizeY = 0;
}


bool CRenderViews::EnsureTarget(RenderView& view)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (view.target != nullptr)
		return true;

	view.target = std::make_unique<FBO>();
	view.target->Bind();
	view.target->CreateRenderBuffer(GL_COLOR_ATTACHMENT0_EXT, GL_RGBA8, view.sizeX, view.sizeY);
	view.target->CreateRenderBuffer(GL_DEPTH_STENCIL_ATTACHMENT, GL_DEPTH24_STENCIL8, view.sizeX, view.sizeY);
	const bool complete = view.target->CheckStatus("RenderViews");
	FBO::Unbind();

	if (!complete) {
		LOG_L(L_ERROR, "[RenderViews::%s] incomplete %dx%d target, disabling secondary view", __func__, view.sizeX, view.sizeY);
		configHandler->Set("SecondaryView", false);
		view.target = nullptr;
	}
	return complete;
}

bool CRenderViews::EnsureInset()
{
	RECOIL_DETAILED_TRACY_ZONE;
	const int wantedX = std::max(1, globalRendering->viewSizeX / 3);
	const int wantedY = std::max(1, globalRendering->viewSizeY / 3);

	if (inset != nullptr && insetSizeX == wantedX && insetSizeY == wantedY)
		return true;

	inset = std::make_unique<FBO>();
	inset->Bind();
	inset->CreateRenderBuffer(GL_COLOR_ATTACHMENT0_EXT, GL_RGBA8, wantedX, wantedY);
	const bool complete = inset->CheckStatus("RenderViewsInset");
	FBO::Unbind();

	if (!complete) {
		LOG_L(L_ERROR, "[RenderViews::%s] incomplete %dx%d inset, disabling secondary view", __func__, wantedX, wantedY);
		configHandler->Set("SecondaryView", false);
		inset = nullptr;
		return false;
	}

	insetSizeX = wantedX;
	insetSizeY = wantedY;
	return true;
}


void CRenderViews::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!FBO::IsSupported() || !configHandler->GetBool("SecondaryView")) {
		if (!views.empty())
			Kill();

		CCamera::SetExtraCullFrusta({});
		return;
	}

	const CCamera* player = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);

	const int scale = std::clamp(configHandler->GetInt("SecondaryViewScale"), 10, 200);
	const int sizeX = std::max(16, globalRendering->viewSizeX * scale / 100);
	const int sizeY = std::max(16, globalRendering->viewSizeY * scale / 100);

	views.resize(1);
	RenderView& view = views[0];

	if (view.sizeX != sizeX || view.sizeY != sizeY) {
		view.target = nullptr;
		view.sizeX = sizeX;
		view.sizeY = sizeY;
	}

	// the view's camera: the player camera turned by a yaw offset and moved
	// sideways, with its own (shifted, hence asymmetric) frustum
	CCamera& cam = view.viewCam;
	cam = *player;
	cam.SetCamType(CCamera::CAMTYPE_VISCUL);

	const float aspect = float(sizeX) / float(sizeY);
	const float tanV = player->GetTanHalfFov();
	const float shift = configHandler->GetInt("SecondaryViewShift") * 0.01f * tanV * aspect;

	float3 rot = player->GetRot();
	rot.y += configHandler->GetInt("SecondaryViewYaw") * math::DEG_TO_RAD;

	const float3 fwd = CCamera::GetFwdFromRot(rot);
	const float3 rgt = CCamera::GetRgtFromRot(rot);
	const float3 upv = rgt.cross(fwd);

	cam.SetExternalPose(player->GetPos() + rgt * float(configHandler->GetInt("SecondaryViewOffset")), fwd, upv);
	cam.SetFrustumTangents(-tanV * aspect + shift, tanV * aspect + shift, -tanV, tanV);

	{
		ViewSizeScope sizeScope(sizeX, sizeY);
		cam.Update();
	}

	// the camera update loaded the view's matrices into the GL matrix stacks
	CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER)->LoadMatrices();

	// objects and terrain visible to any view count as visible
	std::vector<CCamera::Frustum> frusta;
	frusta.push_back(cam.GetFrustum());
	CCamera::SetExtraCullFrusta(std::move(frusta));
}


void CRenderViews::DrawSecondaryViews(const CWorldDrawer& worldDrawer)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (views.empty() || !EnsureInset())
		return;

	CCamera* slot = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);
	const CCamera primary = *slot;

	// inset in the top-right corner of the window
	const int x1 = globalRendering->viewPosX + globalRendering->viewSizeX;
	const int y1 = globalRendering->viewPosY + globalRendering->viewSizeY;
	const int x0 = x1 - insetSizeX;
	const int y0 = y1 - insetSizeY;

	for (RenderView& view: views) {
		if (!EnsureTarget(view))
			return;

		{
			ViewSizeScope sizeScope(view.sizeX, view.sizeY);

			// shaders, culling and drawers all read the player camera
			*slot = view.viewCam;
			slot->SetCamType(CCamera::CAMTYPE_PLAYER);
			slot->Update();
			UploadPlayerCameraMatrices();

			// passes that "go back to the screen" through FBO::Unbind land in this view's target
			const GLuint prevTarget = FBO::SetDefaultTarget(view.target->GetId());

			view.target->Bind();
			slot->LoadViewport();

			inSecondaryView = true;
			CEventHandler::SetDrawCallinsSuppressed(true);
			worldDrawer.Draw();
			CEventHandler::SetDrawCallinsSuppressed(false);
			inSecondaryView = false;

			if (configHandler->GetBool("SecondaryViewDump")) {
				configHandler->Set("SecondaryViewDump", false);
				DumpBoundTarget(view, &view - views.data());
			}

			FBO::SetDefaultTarget(prevTarget);
			FBO::Unbind();
		}

		// back to the primary camera for the remaining passes of this frame
		*slot = primary;
		slot->Update();
		UploadPlayerCameraMatrices();
		slot->LoadViewport();

		const bool scaled = FBO::Blit(view.target->GetId(), inset->GetId(), {0, 0, view.sizeX, view.sizeY}, {0, 0, insetSizeX, insetSizeY}, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		// same-size rects, so this is valid into a multisampled window framebuffer
		const bool shown = FBO::Blit(inset->GetId(), FBO::GetDefaultTarget(), {0, 0, insetSizeX, insetSizeY}, {x0, y0, x1, y1}, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		// TEMP-MRT-DEBUG
		static int dbgFrames = 0;
		if (dbgFrames++ < 3)
			LOG("[MRT] view %dx%d blit=%d/%d err=0x%x", view.sizeX, view.sizeY, scaled, shown, glGetError());
	}
}
