/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "XRPanels.h"

#include <cmath>

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
	panels[id] = panel;
}

void CXRPanels::Remove(const std::string& id)
{
	if (panels.erase(id) > 0)
		CXRSession::GetInstance().RemovePanel(id);
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

void CXRPanels::LocatePanel(const Panel& panel, bool& headLocked, float pos[3], float rot[4], float& widthMetres) const
{
	headLocked = panel.headLocked;

	if (panel.headLocked) {
		pos[0] = panel.pos.x;
		pos[1] = panel.pos.y;
		pos[2] = -panel.pos.z;
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

void CXRPanels::Submit()
{
	RECOIL_DETAILED_TRACY_ZONE;
	CXRSession& xr = CXRSession::GetInstance();
	if (panels.empty() || !xr.IsCreated() || !xr.IsFrameActive())
		return;

	for (const auto& [id, panel]: panels) {
		// the texture may have been freed by its widget
		if (!panel.visible || panel.texture == 0 || !glIsTexture(panel.texture))
			continue;
		if (!xr.EnsurePanel(id, panel.sizeX, panel.sizeY))
			continue;

		bool headLocked = true;
		float pos[3];
		float rot[4];
		float widthMetres = 1.0f;
		LocatePanel(panel, headLocked, pos, rot, widthMetres);
		xr.SetPanelPlacement(id, headLocked, pos, rot, widthMetres);

		const GLuint image = xr.AcquirePanelImage(id);
		if (image == 0)
			continue;

		if (srcFbo == nullptr)
			srcFbo = std::make_unique<FBO>();
		if (dstFbo == nullptr)
			dstFbo = std::make_unique<FBO>();

		srcFbo->Bind();
		srcFbo->AttachTexture(panel.texture, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0_EXT);
		dstFbo->Bind();
		dstFbo->AttachTexture(image, GL_TEXTURE_2D, GL_COLOR_ATTACHMENT0_EXT);
		FBO::Unbind();

		FBO::Blit(srcFbo->GetId(), dstFbo->GetId(), {0, 0, panel.sizeX, panel.sizeY}, {0, 0, panel.sizeX, panel.sizeY}, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		// the compositor reads the image once released
		glFlush();
		xr.ReleasePanelImage(id);
	}
}
