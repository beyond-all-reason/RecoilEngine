/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RenderViews.h"

#include <algorithm>

#include "Game/Camera.h"
#include "Game/CameraHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/UniformConstants.h"
#include "Rendering/WorldDrawer.h"
#include "Rendering/Textures/Bitmap.h"
#include "Game/UI/MouseHandler.h"
#include "Map/Ground.h"
#include "Rendering/XR/XRPanels.h"
#include "Rendering/XR/XRSession.h"
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
CONFIG(int, SecondaryViewWidth)
	.defaultValue(0)
	.minimumValue(0)
	.description("Fixed pixel width of the secondary view's target (e.g. a headset's render size); 0 = use SecondaryViewScale. Needs SecondaryViewHeight too.");
CONFIG(int, SecondaryViewHeight)
	.defaultValue(0)
	.minimumValue(0)
	.description("Fixed pixel height of the secondary view's target; 0 = use SecondaryViewScale.");
CONFIG(bool, SecondaryViewStereo)
	.defaultValue(false)
	.description("Render a left and a right eye view (side by side insets) instead of one free-standing view. Uses SecondaryViewEyeSeparation, and SecondaryViewShift as the inward lens shift; the yaw/offset vars are ignored.");
CONFIG(int, SecondaryViewEyeSeparation)
	.defaultValue(16)
	.description("Distance in elmos between the two eyes in stereo mode.");
CONFIG(bool, XREnable)
	.defaultValue(false)
	.description("Render the world to an OpenXR headset (both eyes, runtime frame pacing); the eye views are also shown as insets. Turn the window's VSync off, the headset paces the frames.");
CONFIG(int, XRWorldScale)
	.defaultValue(200)
	.minimumValue(1)
	.description("Elmos per metre of headset movement; also the eye separation (an IPD of 0.063m gives 0.063 * this).");
CONFIG(int, XRCursorSize)
	.defaultValue(100)
	.minimumValue(10)
	.maximumValue(1000)
	.description("Headset: size of the pointer's cursor image in percent (100 = 32 pixel cursor spans about 1.5 degrees of view).");
CONFIG(int, XRResolutionScale)
	.defaultValue(100)
	.minimumValue(10)
	.maximumValue(200)
	.description("Per-eye render size in percent of the size the headset runtime recommends (read when the session starts).");
CONFIG(bool, XRPointer)
	.defaultValue(true)
	.description("Headset: the mouse moves a pointer through the view of the headset (see XRPointerFov) instead of over the flat camera view, and a reticle shows where it hits the ground.");
CONFIG(int, XRPointerFov)
	.defaultValue(60)
	.minimumValue(10)
	.maximumValue(150)
	.description("Headset: the vertical angle in degrees that the window's height covers for the mouse pointer.");
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

	// v rotated by the unit quaternion q (x y z w)
	float3 RotateByQuat(const float q[4], const float3& v)
	{
		const float3 qv(q[0], q[1], q[2]);
		const float3 t = qv.cross(v) * 2.0f;
		return v + t * q[3] + qv.cross(t);
	}

	// maps a vector from headset space (+x right, +y up, -z forward) onto the
	// axes of the given camera
	float3 XRVectorToWorld(const CCamera& base, const float3& v)
	{
		return base.GetRight() * v.x + base.GetUp() * v.y - base.GetForward() * v.z;
	}

	// a ring on the ground around the pointer, of a constant size as seen from the eye,
	// drawn over everything with the (already loaded) matrices of the current view
	void DrawReticle(const float3& pos, const float3& eyePos)
	{
		const float radius = std::max(4.0f, pos.distance(eyePos) * 0.012f);

		GLint prevProgram = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
		glUseProgram(0);

		glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_FOG);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		const auto ring = [&](float r, float width, float cr, float cg, float cb) {
			glLineWidth(width);
			glColor4f(cr, cg, cb, 1.0f);
			glBegin(GL_LINE_LOOP);
			for (int i = 0; i < 48; ++i) {
				const float a = float(i) * (math::TWOPI / 48.0f);
				glVertex3f(pos.x + std::cos(a) * r, pos.y + 2.0f, pos.z + std::sin(a) * r);
			}
			glEnd();
		};

		ring(radius, 6.0f, 0.0f, 0.0f, 0.0f);
		ring(radius, 3.0f, 1.0f, 1.0f, 1.0f);

		// a mark in the middle, so that the exact point can be told
		glLineWidth(3.0f);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glBegin(GL_LINES);
			glVertex3f(pos.x - radius * 0.35f, pos.y + 2.0f, pos.z);
			glVertex3f(pos.x + radius * 0.35f, pos.y + 2.0f, pos.z);
			glVertex3f(pos.x, pos.y + 2.0f, pos.z - radius * 0.35f);
			glVertex3f(pos.x, pos.y + 2.0f, pos.z + radius * 0.35f);
		glEnd();

		glPopAttrib();
		glUseProgram(prevProgram);
	}

	void UploadPlayerCameraMatrices()
	{
		UniformConstants::GetInstance().UpdateMatrices();
		UniformConstants::GetInstance().Bind();
	}
}


float3 CRenderViews::CalcCursorDir(const CCamera* cam, int x, int y)
{
	if (!pointerActive)
		return cam->CalcPixelDir(x, y);

	const int vsx = std::max(1, globalRendering->viewSizeX);
	const int vsy = std::max(1, globalRendering->viewSizeY);

	const float dx = float(x - globalRendering->viewPosX - (vsx >> 1)) / vsy * (pointerTanHalf * 2.0f);
	const float dy = float(y - (vsy >> 1)) / vsy * (pointerTanHalf * 2.0f);

	return (pointerForward - pointerUp * dy + pointerRight * dx).Normalize();
}


bool CRenderViews::CalcCursorHeadOffset(int x, int y, float& offsetX, float& offsetY)
{
	if (!pointerActive)
		return false;

	const int vsx = std::max(1, globalRendering->viewSizeX);
	const int vsy = std::max(1, globalRendering->viewSizeY);

	offsetX =  float(x - globalRendering->viewPosX - (vsx >> 1)) / vsy * (pointerTanHalf * 2.0f);
	offsetY = -float(y - (vsy >> 1)) / vsy * (pointerTanHalf * 2.0f);
	return true;
}


void CRenderViews::Kill()
{
	views.clear();
	CXRPanels::GetInstance().Kill();
}


bool CRenderViews::EnsureTarget(RenderView& view)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (view.target != nullptr)
		return true;

	view.FreeTarget();

	glGenTextures(1, &view.colorTex);
	glBindTexture(GL_TEXTURE_2D, view.colorTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, view.sizeX, view.sizeY, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	glGenTextures(1, &view.depthTex);
	glBindTexture(GL_TEXTURE_2D, view.depthTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, view.sizeX, view.sizeY, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);

	view.target = std::make_unique<FBO>();
	view.target->Bind();
	view.target->AttachTexture(view.colorTex, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0_EXT);
	view.target->AttachTexture(view.depthTex, GL_TEXTURE_2D, GL_DEPTH_STENCIL_ATTACHMENT);
	const bool complete = view.target->CheckStatus("RenderViews");
	FBO::Unbind();

	if (!complete) {
		LOG_L(L_ERROR, "[RenderViews::%s] incomplete %dx%d target, disabling secondary view", __func__, view.sizeX, view.sizeY);
		configHandler->Set("SecondaryView", false);
		view.FreeTarget();
	}
	return complete;
}

bool CRenderViews::EnsureInset(RenderView& view)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// largest rect with the view's aspect ratio that fits in a third of the
	// window's height and, for all views of the row together, a third of its width
	const float boxX = std::max(1.0f, globalRendering->viewSizeX / 3.0f / float(views.size()));
	const float boxY = std::max(1.0f, globalRendering->viewSizeY / 3.0f);
	const float fit = std::min(boxX / view.sizeX, boxY / view.sizeY);
	const int wantedX = std::max(1, int(view.sizeX * fit));
	const int wantedY = std::max(1, int(view.sizeY * fit));

	if (view.inset != nullptr && view.insetSizeX == wantedX && view.insetSizeY == wantedY)
		return true;

	view.inset = std::make_unique<FBO>();
	view.inset->Bind();
	view.inset->CreateRenderBuffer(GL_COLOR_ATTACHMENT0_EXT, GL_RGBA8, wantedX, wantedY);
	const bool complete = view.inset->CheckStatus("RenderViewsInset");
	FBO::Unbind();

	if (!complete) {
		LOG_L(L_ERROR, "[RenderViews::%s] incomplete %dx%d inset, disabling secondary view", __func__, wantedX, wantedY);
		configHandler->Set("SecondaryView", false);
		view.inset = nullptr;
		return false;
	}

	view.insetSizeX = wantedX;
	view.insetSizeY = wantedY;
	return true;
}


void CRenderViews::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CXRSession& xr = CXRSession::GetInstance();
	const bool xrWanted = configHandler->GetBool("XREnable");

	pointerActive = false;
	reticleValid = false;

	if (!FBO::IsSupported() || !(xrWanted || configHandler->GetBool("SecondaryView"))) {
		if (xr.IsCreated())
			xr.Stop();
		if (!views.empty())
			Kill();

		CCamera::SetExtraCullFrusta({});
		return;
	}

	// the headset: the session is started on demand and dropped again when
	// it is lost or the user turns the option off
	bool xrFrame = false;
	if (xrWanted) {
		if (!xr.IsCreated() && !xr.Start(configHandler->GetInt("XRResolutionScale"))) {
			configHandler->Set("XREnable", false);
			return;
		}
		if (xr.IsLost()) {
			LOG("[RenderViews::%s] the OpenXR session ended, disabling XREnable", __func__);
			xr.Stop();
			configHandler->Set("XREnable", false);
			return;
		}
		// blocks until the runtime wants the next frame
		xrFrame = xr.BeginFrame();
	} else if (xr.IsCreated()) {
		xr.Stop();
		Kill();
	}

	const CCamera* player = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);

	int sizeX = 0;
	int sizeY = 0;

	if (xrWanted) {
		sizeX = xr.GetEyeWidth();
		sizeY = xr.GetEyeHeight();
	} else {
		const int scale = std::clamp(configHandler->GetInt("SecondaryViewScale"), 10, 200);
		// an explicit size (a headset's recommended render size) wins over the scale
		const int fixedX = configHandler->GetInt("SecondaryViewWidth");
		const int fixedY = configHandler->GetInt("SecondaryViewHeight");
		const bool useFixed = (fixedX > 0 && fixedY > 0);
		sizeX = useFixed ? std::clamp(fixedX, 16, 8192) : std::max(16, globalRendering->viewSizeX * scale / 100);
		sizeY = useFixed ? std::clamp(fixedY, 16, 8192) : std::max(16, globalRendering->viewSizeY * scale / 100);
	}

	// stereo: a left and a right eye looking where the player camera looks,
	// each offset sideways by half the eye separation and with its frustum
	// shifted inward; otherwise one free-standing view
	const bool stereo = xrWanted || configHandler->GetBool("SecondaryViewStereo");
	views.resize(stereo ? 2 : 1);

	const float aspect = float(sizeX) / float(sizeY);
	const float tanV = player->GetTanHalfFov();
	const float shift = configHandler->GetInt("SecondaryViewShift") * 0.01f * tanV * aspect;

	std::vector<CCamera::Frustum> frusta;

	for (size_t i = 0; i < views.size(); ++i) {
		RenderView& view = views[i];

		view.xrDriven = xrWanted;
		view.active = !xrWanted || xrFrame;
		view.externalColorTex = 0;

		if (view.sizeX != sizeX || view.sizeY != sizeY) {
			view.FreeTarget();
			view.sizeX = sizeX;
			view.sizeY = sizeY;
		}

		if (!view.active)
			continue;

		// the view's camera: the player camera turned by a yaw offset and moved
		// sideways, with its own (shifted, hence asymmetric) frustum
		CCamera& cam = view.viewCam;
		cam = *player;
		cam.SetCamType(CCamera::CAMTYPE_VISCUL);

		if (xrWanted) {
			// the headset's pose is relative to its starting position, which is
			// put at the player camera, turned the way the player camera looks
			const XREye& eye = xr.GetEye(int(i));
			const float worldScale = float(configHandler->GetInt("XRWorldScale"));

			const float3 headPos = XRVectorToWorld(*player, float3(eye.pos[0], eye.pos[1], eye.pos[2]));
			const float3 eyeFwd = XRVectorToWorld(*player, RotateByQuat(eye.rot, float3(0.0f, 0.0f, -1.0f)));
			const float3 eyeUp = XRVectorToWorld(*player, RotateByQuat(eye.rot, float3(0.0f, 1.0f, 0.0f)));

			cam.SetExternalPose(player->GetPos() + headPos * worldScale, eyeFwd, eyeUp);
			cam.SetFrustumTangents(eye.tanLeft, eye.tanRight, eye.tanBottom, eye.tanTop);
		} else {
			float3 rot = player->GetRot();
			float sideOffset = float(configHandler->GetInt("SecondaryViewOffset"));
			float lensShift = shift;

			if (stereo) {
				const float eyeSign = (i == 0) ? -1.0f : 1.0f;
				sideOffset = eyeSign * 0.5f * float(configHandler->GetInt("SecondaryViewEyeSeparation"));
				lensShift = -eyeSign * shift;
			} else {
				rot.y += configHandler->GetInt("SecondaryViewYaw") * math::DEG_TO_RAD;
			}

			const float3 fwd = CCamera::GetFwdFromRot(rot);
			const float3 rgt = CCamera::GetRgtFromRot(rot);
			const float3 upv = rgt.cross(fwd);

			cam.SetExternalPose(player->GetPos() + rgt * sideOffset, fwd, upv);
			cam.SetFrustumTangents(-tanV * aspect + lensShift, tanV * aspect + lensShift, -tanV, tanV);
		}

		{
			ViewSizeScope sizeScope(sizeX, sizeY);
			cam.Update();
		}

		// objects and terrain visible to any view count as visible
		frusta.push_back(cam.GetFrustum());
	}

	if (xrWanted) {
		CXRPanels& panels = CXRPanels::GetInstance();
		panels.SetOrigin(player->GetPos(), player->GetRight(), player->GetUp(), player->GetForward(), float(configHandler->GetInt("XRWorldScale")));

		float3 headPos;
		if (xrFrame && configHandler->GetBool("XRPointer") && panels.GetHeadBasis(headPos, pointerForward, pointerUp, pointerRight)) {
			const float fovDeg = float(configHandler->GetInt("XRPointerFov"));
			pointerTanHalf = std::tan(0.5f * fovDeg * math::DEG_TO_RAD);
			pointerActive = true;

			// where the pointer meets the ground, for the reticle; like the picking, from the camera
			if (mouse != nullptr && mouse->lastx >= 0 && mouse->lasty >= 0) {
				const float3 dir = CalcCursorDir(player, mouse->lastx, mouse->lasty);
				const float range = player->GetFarPlaneDist() * 1.4f;
				const float dist = CGround::LineGroundCol(player->GetPos(), dir, range, false);

				// no ground marker while pointing at a panel
				if (dist > 0.0f && !CXRPanels::GetInstance().IsPointerOverPanel(mouse->lastx, mouse->lasty)) {
					reticlePos = player->GetPos() + dir * dist;
					reticleValid = true;
				}
			}
		}
	}

	// the camera updates loaded the views' matrices into the GL matrix stacks
	CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER)->LoadMatrices();

	CCamera::SetExtraCullFrusta(std::move(frusta));
}


void CRenderViews::DrawSecondaryViews(const CWorldDrawer& worldDrawer)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (views.empty())
		return;

	for (RenderView& view: views) {
		if (!EnsureInset(view))
			return;
	}

	CCamera* slot = CCameraHandler::GetCamera(CCamera::CAMTYPE_PLAYER);
	const CCamera primary = *slot;

	// insets in a row along the top-right corner of the window, first view leftmost
	int rowWidth = 0;
	for (const RenderView& view: views)
		rowWidth += view.insetSizeX;

	const int y1 = globalRendering->viewPosY + globalRendering->viewSizeY;
	int nextX = globalRendering->viewPosX + globalRendering->viewSizeX - rowWidth;

	for (RenderView& view: views) {
		const int x0 = nextX;
		const int x1 = x0 + view.insetSizeX;
		const int y0 = y1 - view.insetSizeY;
		nextX = x1;

		if (!view.active)
			continue;

		if (!EnsureTarget(view))
			return;

		const int viewIdx = int(&view - views.data());

		if (view.xrDriven) {
			view.externalColorTex = CXRSession::GetInstance().AcquireEyeImage(viewIdx);
			if (view.externalColorTex == 0)
				continue;

			view.target->Bind();
			view.target->AttachTexture(view.externalColorTex, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0_EXT);
			FBO::Unbind();
		}

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

			if (view.xrDriven && reticleValid) {
				slot->LoadMatrices();
				const CMouseCursor* cursor = (mouse != nullptr) ? mouse->GetActiveCursor() : nullptr;

				if (cursor != nullptr) {
					// the desktop's cursor image for the current command, turned to face the eye;
					// at the default size 32 pixels span about 1.5 degrees of view
					const float angle = configHandler->GetInt("XRCursorSize") * 0.01f * 0.0008f;
					const float pixelSize = reticlePos.distance(slot->GetPos()) * angle * std::abs(mouse->GetCurrentCursorScale());
					cursor->DrawInWorld(reticlePos, slot->GetRight(), slot->GetUp(), pixelSize);
				} else {
					DrawReticle(reticlePos, slot->GetPos());
				}
			}

			if (configHandler->GetBool("SecondaryViewDump")) {
				configHandler->Set("SecondaryViewDump", false);
				DumpBoundTarget(view, &view - views.data());
			}

			FBO::SetDefaultTarget(prevTarget);
			FBO::Unbind();
		}

		if (view.xrDriven) {
			// the compositor reads the image once released
			glFlush();
			CXRSession::GetInstance().ReleaseEyeImage(viewIdx);
		}

		// back to the primary camera for the remaining passes of this frame
		*slot = primary;
		slot->Update();
		UploadPlayerCameraMatrices();
		slot->LoadViewport();

		const bool scaled = FBO::Blit(view.target->GetId(), view.inset->GetId(), {0, 0, view.sizeX, view.sizeY}, {0, 0, view.insetSizeX, view.insetSizeY}, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		// same-size rects, so this is valid into a multisampled window framebuffer
		const bool shown = FBO::Blit(view.inset->GetId(), FBO::GetDefaultTarget(), {0, 0, view.insetSizeX, view.insetSizeY}, {x0, y0, x1, y1}, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		(void) scaled;
		(void) shown;
	}
}


void CRenderViews::EndXRFrame()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CXRSession& xr = CXRSession::GetInstance();
	if (xr.IsCreated()) {
		CXRPanels::GetInstance().Submit();
		xr.EndFrame();
	}
}
