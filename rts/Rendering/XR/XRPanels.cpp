/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "XRPanels.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

#include "Game/UI/MouseHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/RenderViews.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/DataDirLocater.h"
#include "Rendering/XR/XRSession.h"
#include "System/Log/ILog.h"
#include "System/Misc/TracyDefs.h"
#include "System/SpringMath.h"

namespace {
	// v rotated by the unit quaternion q (x y z w)
	float3 RotateByQuat(const float q[4], const float3& v)
	{
		const float3 qv(q[0], q[1], q[2]);
		const float3 t = qv.cross(v) * 2.0f;
		return v + t * q[3] + qv.cross(t);
	}

	// the unit quaternion of the rotation that turns the axes into x, y, z
	void QuatFromAxes(const float3& x, const float3& y, const float3& z, float q[4])
	{
		const float m00 = x.x, m01 = y.x, m02 = z.x;
		const float m10 = x.y, m11 = y.y, m12 = z.y;
		const float m20 = x.z, m21 = y.z, m22 = z.z;
		const float trace = m00 + m11 + m22;

		if (trace > 0.0f) {
			const float s = std::sqrt(trace + 1.0f) * 2.0f;
			q[3] = 0.25f * s;
			q[0] = (m21 - m12) / s;
			q[1] = (m02 - m20) / s;
			q[2] = (m10 - m01) / s;
		} else if (m00 > m11 && m00 > m22) {
			const float s = std::sqrt(1.0f + m00 - m11 - m22) * 2.0f;
			q[3] = (m21 - m12) / s;
			q[0] = 0.25f * s;
			q[1] = (m01 + m10) / s;
			q[2] = (m02 + m20) / s;
		} else if (m11 > m22) {
			const float s = std::sqrt(1.0f + m11 - m00 - m22) * 2.0f;
			q[3] = (m02 - m20) / s;
			q[0] = (m01 + m10) / s;
			q[1] = 0.25f * s;
			q[2] = (m12 + m21) / s;
		} else {
			const float s = std::sqrt(1.0f + m22 - m00 - m11) * 2.0f;
			q[3] = (m10 - m01) / s;
			q[0] = (m02 + m20) / s;
			q[1] = (m12 + m21) / s;
			q[2] = 0.25f * s;
		}
	}
}


void CXRPanels::Set(const std::string& id, const Panel& panel)
{
	const auto it = panels.find(id);
	if (it == panels.end() || it->second.texture != panel.texture)
		LOG("[XRPanels] panel '%s': texture %u (%dx%d), %s at %.2f %.2f %.2f, width %.2f", id.c_str(), panel.texture, panel.sizeX, panel.sizeY, panel.headLocked ? "head" : "world", panel.pos.x, panel.pos.y, panel.pos.z, panel.width);

	panels[id] = panel;
}

void CXRPanels::Remove(const std::string& id)
{
	states.erase(id);
	if (panels.erase(id) > 0)
		CXRSession::GetInstance().RemovePanel(id);
}

CONFIG(int, XRPanelFade).defaultValue(1).description("Headset panels away from the middle of the view fade out, and fade in and move towards the middle when the pointer comes near.");
CONFIG(int, XRPanelFadeMin).defaultValue(10).minimumValue(0).maximumValue(100).description("The opacity in percent of a faded out headset panel.");

namespace {
	float SmoothStep(float edge0, float edge1, float x)
	{
		const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	// how far the panel is from the middle of the view, 0 (in the middle) to 1 (at the side)
	float Sideness(const CXRPanels::Panel& panel)
	{
		return SmoothStep(0.12f, 0.35f, std::hypot(panel.pos.x, panel.pos.y) / std::max(panel.pos.z, 0.1f));
	}

	// the largest drift towards the middle, as a fraction of the panel's position
	constexpr float MAX_DRIFT = 0.2f;
}

void CXRPanels::UpdateStates()
{
	static auto lastTime = std::chrono::steady_clock::now();
	const auto now = std::chrono::steady_clock::now();
	const float dt = std::min(0.1f, std::chrono::duration<float>(now - lastTime).count());
	lastTime = now;

	const bool fade = (configHandler->GetInt("XRPanelFade") != 0);
	const float minAlpha = configHandler->GetInt("XRPanelFadeMin") * 0.01f;

	// where the pointer points, as the offsets in the view (per metre of distance)
	float pointerX = 0.0f;
	float pointerY = 0.0f;
	if (mouse != nullptr && mouse->lastx >= 0 && mouse->lasty >= 0)
		CRenderViews::CalcCursorHeadOffset(mouse->lastx, mouse->lasty, pointerX, pointerY);

	// a quick response coming into view, slower going out
	const float rateIn = 1.0f - std::exp(-dt * 12.0f);
	const float rateOut = 1.0f - std::exp(-dt * 4.0f);

	for (const auto& [id, panel]: panels) {
		if (!panel.headLocked || panel.pos.z <= 0.0f)
			continue;

		PanelState& state = states[id];

		float target = 1.0f;
		if (fade) {
			const float halfWidth = 0.5f * panel.width / panel.pos.z;
			const float halfHeight = 0.5f * panel.width * float(panel.sizeY) / float(std::max(panel.sizeX, 1)) / panel.pos.z;
			const float dx = std::max(0.0f, std::fabs(pointerX - panel.pos.x / panel.pos.z) - halfWidth);
			const float dy = std::max(0.0f, std::fabs(pointerY - panel.pos.y / panel.pos.z) - halfHeight);
			target = 1.0f - SmoothStep(0.08f, 0.4f, std::hypot(dx, dy));
		}

		state.focus += (target - state.focus) * (target > state.focus ? rateIn : rateOut);

		const float side = Sideness(panel);
		state.alpha = fade ? (1.0f - side * (1.0f - std::lerp(minAlpha, 1.0f, state.focus))) : 1.0f;
	}
}

float3 CXRPanels::GetEffectivePos(const std::string& id, const Panel& panel) const
{
	const auto it = states.find(id);
	if (it == states.end() || !panel.headLocked)
		return panel.pos;

	const float drift = MAX_DRIFT * it->second.focus * Sideness(panel);
	return float3(panel.pos.x * (1.0f - drift), panel.pos.y * (1.0f - drift), panel.pos.z);
}

void CXRPanels::Kill()
{
	srcFbo = nullptr;
	dstFbo = nullptr;
}

void CXRPanels::SetOrigin(const float3& pos, const float3& right, const float3& up, const float3& forward, float scale)
{
	originPos = pos;
	originRight = right;
	originUp = up;
	originForward = forward;
	worldScale = scale;
}

bool CXRPanels::GetHeadPose(float3& pos, float3& dir) const
{
	float3 up;
	float3 right;
	return GetHeadBasis(pos, dir, up, right);
}

bool CXRPanels::GetHeadBasis(float3& pos, float3& forward, float3& up, float3& right) const
{
	const CXRSession& xr = CXRSession::GetInstance();
	if (!xr.IsCreated())
		return false;

	const XREye& left = xr.GetEye(0);
	const XREye& rightEye = xr.GetEye(1);

	const float3 headPos(
		0.5f * (left.pos[0] + rightEye.pos[0]),
		0.5f * (left.pos[1] + rightEye.pos[1]),
		0.5f * (left.pos[2] + rightEye.pos[2])
	);

	// headset space (+x right, +y up, -z forward) onto the camera's axes
	const auto toWorld = [this](const float3& v) {
		return originRight * v.x + originUp * v.y - originForward * v.z;
	};

	pos = originPos + toWorld(headPos) * worldScale;
	forward = toWorld(RotateByQuat(left.rot, float3(0.0f, 0.0f, -1.0f)));
	up = toWorld(RotateByQuat(left.rot, float3(0.0f, 1.0f, 0.0f)));
	right = toWorld(RotateByQuat(left.rot, float3(1.0f, 0.0f, 0.0f)));
	return true;
}

bool CXRPanels::HitTest(const std::string& id, int x, int y, float& u, float& v) const
{
	const auto it = panels.find(id);
	if (it == panels.end())
		return false;

	const Panel& panel = it->second;
	if (!panel.headLocked || !panel.visible || panel.sizeX <= 0 || panel.sizeY <= 0 || panel.pos.z <= 0.0f)
		return false;

	float offsetX = 0.0f;
	float offsetY = 0.0f;
	if (!CRenderViews::CalcCursorHeadOffset(x, y, offsetX, offsetY))
		return false;

	// the ray passes the panel's plane at distance pos.z
	const float3 pos = GetEffectivePos(id, panel);
	const float hitX = offsetX * pos.z;
	const float hitY = offsetY * pos.z;
	const float height = panel.width * float(panel.sizeY) / float(panel.sizeX);

	u = (hitX - pos.x) / panel.width + 0.5f;
	v = (hitY - pos.y) / height + 0.5f;
	return (u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f);
}

bool CXRPanels::IsPointerOverPanel(int x, int y) const
{
	float u, v;
	for (const auto& [id, panel]: panels) {
		if (HitTest(id, x, y, u, v))
			return true;
	}
	return false;
}


void CXRPanels::LocatePanel(const std::string& id, const Panel& panel, bool& headLocked, float pos[3], float rot[4], float& widthMetres) const
{
	headLocked = panel.headLocked;

	if (panel.headLocked) {
		const float3 effective = GetEffectivePos(id, panel);
		pos[0] = effective.x;
		pos[1] = effective.y;
		pos[2] = -effective.z;
		rot[0] = rot[1] = rot[2] = 0.0f;
		rot[3] = 1.0f;
		widthMetres = panel.width;
		return;
	}

	// the inverse of what places the eyes in the world: from the game world
	// into the headset's space
	const auto toLocal = [this](const float3& v) {
		return float3(v.dot(originRight), v.dot(originUp), -v.dot(originForward));
	};

	const float3 local = toLocal((panel.pos - originPos) / worldScale);
	pos[0] = local.x;
	pos[1] = local.y;
	pos[2] = local.z;
	widthMetres = panel.width / worldScale;

	// the panel's normal, towards the viewer; its up stays the world's up
	float3 normal;
	if (panel.billboard) {
		const CXRSession& xr = CXRSession::GetInstance();
		const XREye& left = xr.GetEye(0);
		const XREye& right = xr.GetEye(1);
		const float3 head(
			0.5f * (left.pos[0] + right.pos[0]),
			0.5f * (left.pos[1] + right.pos[1]),
			0.5f * (left.pos[2] + right.pos[2])
		);
		normal = head - local;
	} else {
		const float yawRad = panel.yaw * math::DEG_TO_RAD;
		normal = toLocal(float3(std::sin(yawRad), 0.0f, std::cos(yawRad)));
	}
	normal.Normalize();

	float3 up = toLocal(UpVector);
	float3 xAxis = up.cross(normal);
	if (xAxis.SqLength() < 1e-6f) {
		// looking straight along the up axis
		up = float3(0.0f, 1.0f, 0.0f);
		xAxis = up.cross(normal);
	}
	xAxis.Normalize();
	const float3 yAxis = normal.cross(xAxis);

	QuatFromAxes(xAxis, yAxis, normal, rot);
}

CONFIG(int, XRPanelDump).defaultValue(0).description("Set to 1 to save the images of the VR panels to the data directory, once");

// saves the colour attachment of the bound framebuffer
static void DumpBoundFramebuffer(const std::string& name, int width, int height)
{
	CBitmap bmp;
	bmp.Alloc(width, height, 4);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, bmp.GetRawMem());

	const std::string path = dataDirLocater.GetWriteDirPath() + name;
	bmp.Save(path, false, true);
}

void CXRPanels::Submit()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CXRSession& xr = CXRSession::GetInstance();
	if (panels.empty() || !xr.IsCreated() || !xr.IsFrameActive())
		return;

	UpdateStates();

	const bool dumpWanted = (configHandler->GetInt("XRPanelDump") != 0);
	if (dumpWanted)
		configHandler->Set("XRPanelDump", 0);

	// the panels go side by side in rows on the shared image
	constexpr int PADDING = 2;
	const int atlasWidth = xr.GetPanelAtlasWidth();
	const int atlasHeight = xr.GetPanelAtlasHeight();

	struct Placed {
		const std::string* id;
		const Panel* panel;
		int x;
		int y;
	};
	std::vector<Placed> placed;

	int cursorX = 0;
	int cursorY = 0;
	int rowHeight = 0;

	for (const auto& [id, panel]: panels) {
		// the texture may have been freed by its widget
		if (!panel.visible || panel.texture == 0 || !glIsTexture(panel.texture))
			continue;
		if (panel.sizeX > atlasWidth || panel.sizeY > atlasHeight)
			continue;

		if (cursorX + panel.sizeX > atlasWidth) {
			cursorX = 0;
			cursorY += rowHeight + PADDING;
			rowHeight = 0;
		}
		if (cursorY + panel.sizeY > atlasHeight)
			continue;

		placed.push_back({&id, &panel, cursorX, cursorY});
		cursorX += panel.sizeX + PADDING;
		rowHeight = std::max(rowHeight, panel.sizeY);
	}

	if (placed.empty())
		return;

	const GLuint image = xr.AcquirePanelAtlas();
	if (image == 0)
		return;

	if (srcFbo == nullptr)
		srcFbo = std::make_unique<FBO>();
	if (dstFbo == nullptr)
		dstFbo = std::make_unique<FBO>();

	dstFbo->Bind();
	dstFbo->AttachTexture(image, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0_EXT);
	FBO::Unbind();

	for (const Placed& p: placed) {
		const Panel& panel = *p.panel;

		bool headLocked = true;
		float pos[3];
		float rot[4];
		float widthMetres = 1.0f;
		LocatePanel(*p.id, panel, headLocked, pos, rot, widthMetres);
		xr.SetPanel(*p.id, p.x, p.y, panel.sizeX, panel.sizeY, headLocked, pos, rot, widthMetres);

		srcFbo->Bind();
		srcFbo->AttachTexture(panel.texture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0_EXT);
		FBO::Unbind();

		const auto stateIt = states.find(*p.id);
		const float alpha = (stateIt != states.end()) ? stateIt->second.alpha : 1.0f;

		if (alpha >= 0.999f) {
			FBO::Blit(srcFbo->GetId(), dstFbo->GetId(), {0, 0, panel.sizeX, panel.sizeY}, {p.x, p.y, p.x + panel.sizeX, p.y + panel.sizeY}, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		} else {
			// the images are premultiplied: scaling all four channels fades them
			GLint program = 0;
			glGetIntegerv(GL_CURRENT_PROGRAM, &program);
			glUseProgram(0);

			glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT);
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();
			glOrtho(0.0, atlasWidth, 0.0, atlasHeight, -1.0, 1.0);
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();

			dstFbo->Bind();
			glViewport(0, 0, atlasWidth, atlasHeight);
			glDisable(GL_SCISSOR_TEST);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
			glDisable(GL_CULL_FACE);
			glActiveTexture(GL_TEXTURE0);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, panel.texture);
			glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			glColor4f(alpha, alpha, alpha, alpha);

			glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 0.0f); glVertex2f(float(p.x), float(p.y));
				glTexCoord2f(1.0f, 0.0f); glVertex2f(float(p.x + panel.sizeX), float(p.y));
				glTexCoord2f(1.0f, 1.0f); glVertex2f(float(p.x + panel.sizeX), float(p.y + panel.sizeY));
				glTexCoord2f(0.0f, 1.0f); glVertex2f(float(p.x), float(p.y + panel.sizeY));
			glEnd();

			glBindTexture(GL_TEXTURE_2D, 0);
			FBO::Unbind();

			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
			glPopAttrib();
			glUseProgram(program);
		}

		if (dumpWanted) {
			// what the widget drew, and what arrived in the headset image
			srcFbo->Bind();
			DumpBoundFramebuffer("xrpanel_" + *p.id + "_widget.png", panel.sizeX, panel.sizeY);
			dstFbo->Bind();
			CBitmap bmp;
			bmp.Alloc(panel.sizeX, panel.sizeY, 4);
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glReadPixels(p.x, p.y, panel.sizeX, panel.sizeY, GL_RGBA, GL_UNSIGNED_BYTE, bmp.GetRawMem());
			bmp.Save(dataDirLocater.GetWriteDirPath() + "xrpanel_" + *p.id + "_headset.png", false, true);
			FBO::Unbind();
		}
	}

	// the compositor reads the image once released
	glFinish();
	xr.ReleasePanelAtlas();
}
