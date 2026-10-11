/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RenderViews.h"

#include <array>

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/UniformConstants.h"
#include "Rendering/WorldDrawer.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/myGL.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/SpringMath.h"

#include "System/Misc/TracyDefs.h"

CONFIG(bool, SecondaryView)
	.defaultValue(false)
	.description("Render a second camera into an offscreen target, shown as an inset. Prototype for multi-window rendering.");
CONFIG(int, SecondaryViewYaw)
	.defaultValue(40)
	.description("Yaw offset in degrees of the secondary view's camera relative to the player camera. Objects are only culled for the player camera, so large offsets show no units.");


void CRenderViews::Kill()
{
	target = nullptr;
	inset = nullptr;
	targetSizeX = 0;
	targetSizeY = 0;
}


bool CRenderViews::EnsureTarget(int sizeX, int sizeY)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (target != nullptr && targetSizeX == sizeX && targetSizeY == sizeY)
		return true;

	target = std::make_unique<FBO>();
	target->Bind();
	target->CreateRenderBuffer(GL_COLOR_ATTACHMENT0_EXT, GL_RGBA8, sizeX, sizeY);
	target->CreateRenderBuffer(GL_DEPTH_STENCIL_ATTACHMENT, GL_DEPTH24_STENCIL8, sizeX, sizeY);
	bool complete = target->CheckStatus("RenderViews");

	insetSizeX = std::max(1, sizeX / 3);
	insetSizeY = std::max(1, sizeY / 3);

	inset = std::make_unique<FBO>();
	inset->Bind();
	inset->CreateRenderBuffer(GL_COLOR_ATTACHMENT0_EXT, GL_RGBA8, insetSizeX, insetSizeY);
	complete &= inset->CheckStatus("RenderViewsInset");
	FBO::Unbind();

	if (!complete) {
		LOG_L(L_ERROR, "[RenderViews::%s] incomplete %dx%d target, disabling secondary view", __func__, sizeX, sizeY);
		configHandler->Set("SecondaryView", false);
		target = nullptr;
		inset = nullptr;
		return false;
	}

	targetSizeX = sizeX;
	targetSizeY = sizeY;
	return true;
}


void CRenderViews::DrawSecondaryViews(const CWorldDrawer& worldDrawer)
{
	RECOIL_DETAILED_TRACY_ZONE;

	// TEMP-MRT-DEBUG
	static bool lastEnabled = false;
	const bool enabled = configHandler->GetBool("SecondaryView");
	if (enabled != lastEnabled) {
		LOG("[MRT] SecondaryView enabled=%d fboSupported=%d viewSize=%dx%d", enabled, FBO::IsSupported(), (int)globalRendering->viewSizeX, (int)globalRendering->viewSizeY);
		lastEnabled = enabled;
	}

	if (!FBO::IsSupported() || !enabled)
		return;

	const int sizeX = globalRendering->viewSizeX;
	const int sizeY = globalRendering->viewSizeY;

	if (!EnsureTarget(sizeX, sizeY))
		return;

	CCamera* cam = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);
	const CCamera primary = *cam;

	{
		// the view's own camera: same position, turned by a yaw offset
		cam->SetRotY(primary.GetRot().y + configHandler->GetInt("SecondaryViewYaw") * math::DEG_TO_RAD);
		cam->Update();
		cam->UpdateViewPort(0, 0, sizeX, sizeY);

		// shaders read the player camera through the matrix UBO
		UniformConstants::GetInstance().UpdateMatrices();
		UniformConstants::GetInstance().Bind();
		// NB: draw flags are deliberately not recomputed for this camera; they are
		// a single per-object state (also mirrored in a GPU buffer), so a second
		// culling pass corrupts the primary view

		// passes that "go back to the screen" through FBO::Unbind land in this view's target
		const GLuint prevTarget = FBO::SetDefaultTarget(target->GetId());

		target->Bind();
		cam->LoadViewport();

		inSecondaryView = true;
		worldDrawer.Draw();
		inSecondaryView = false;

		FBO::SetDefaultTarget(prevTarget);
		FBO::Unbind();
	}

	// back to the primary camera for the remaining passes of this frame
	*cam = primary;
	cam->Update();
	UniformConstants::GetInstance().UpdateMatrices();
	UniformConstants::GetInstance().Bind();
	cam->LoadViewport();

	// inset in the top-right corner of the window
	const int x1 = globalRendering->viewPosX + sizeX;
	const int y1 = globalRendering->viewPosY + sizeY;
	const int x0 = x1 - insetSizeX;
	const int y0 = y1 - insetSizeY;

	bool blitted = FBO::Blit(target->GetId(), inset->GetId(), {0, 0, sizeX, sizeY}, {0, 0, insetSizeX, insetSizeY}, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	// same-size rects, so this is valid into a multisampled window framebuffer
	blitted &= FBO::Blit(inset->GetId(), FBO::GetDefaultTarget(), {0, 0, insetSizeX, insetSizeY}, {x0, y0, x1, y1}, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	// TEMP-MRT-DEBUG
	static int dbgFrames = 0;
	if (dbgFrames++ < 3)
		LOG("[MRT] blit=%d boundFBO=%d src=%u dst=(%d,%d)-(%d,%d) err=0x%x", blitted, FBO::GetCurrentBoundFBO(), target->GetId(), x0, y0, x1, y1, glGetError());
}
