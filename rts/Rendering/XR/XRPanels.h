/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"
#include "System/float3.h"

/**
 * Flat 2D panels shown in the headset as compositor quad layers, fed by
 * textures that Lua widgets render into (gl.CreateTexture with fbo, then
 * gl.RenderToTexture). A panel either floats in front of the face (head
 * locked) or is anchored at a position in the game world.
 *
 * Panels are created and updated through Spring.XRSetPanel; the images are
 * copied into the headset's swapchains once per frame at the end of the
 * draw, and without a headset (or XREnable off) nothing happens.
 */
class CXRPanels {
public:
	struct Panel {
		// the Lua texture to show (GL_TEXTURE_2D), and its size in pixels
		GLuint texture = 0;
		int sizeX = 0;
		int sizeY = 0;

		bool headLocked = true;
		bool visible = true;

		// head locked: x right, y up, z distance in front of the eyes, in metres
		// world: position in elmos
		float3 pos;
		// head locked: width in metres, world: width in elmos
		float width = 1.0f;
		// world only: turn to face the viewer, or else face the direction yaw
		// (degrees around the vertical axis, 0 = towards +z of the world)
		bool billboard = true;
		float yaw = 0.0f;
	};

	static CXRPanels& GetInstance() {
		static CXRPanels instance;
		return instance;
	}

	void Set(const std::string& id, const Panel& panel);
	void Remove(const std::string& id);

	// frees the GL objects, must run while the GL context is alive
	void Kill();

	// where the headset's play space sits in the game world this frame (set by
	// CRenderViews::Update): the position, the camera's axes, and the elmos per metre
	void SetOrigin(const float3& pos, const float3& right, const float3& up, const float3& forward, float worldScale);

	// the headset's position and viewing direction in game world coordinates;
	// false without a headset
	bool GetHeadPose(float3& pos, float3& dir) const;
	// the same with the axes of the headset (looking along forward) as well
	bool GetHeadBasis(float3& pos, float3& forward, float3& up, float3& right) const;

	// Where the pointer (see CRenderViews::CalcCursorDir) meets a head locked panel:
	// u to the right and v up, both 0..1 over the panel. x, y are window pixels, with
	// y from the top. False if it does not hit the panel.
	bool HitTest(const std::string& id, int x, int y, float& u, float& v) const;
	bool IsPointerOverPanel(int x, int y) const;

	// at the end of the draw, before the headset frame ends
	void Submit();

private:
	// Head locked panels away from the middle of the view fade out, and fade in and
	// drift towards the middle while the pointer (where the player looks) is near them
	struct PanelState {
		float focus = 0.0f; // 0..1, smoothed
		float alpha = 1.0f;
	};

	void LocatePanel(const std::string& id, const Panel& panel, bool& headLocked, float pos[3], float rot[4], float& widthMetres) const;
	void UpdateStates();
	// where the panel is now, drifted towards the middle by its focus
	float3 GetEffectivePos(const std::string& id, const Panel& panel) const;

private:
	std::map<std::string, Panel> panels;
	std::map<std::string, PanelState> states;

	std::unique_ptr<FBO> srcFbo;
	std::unique_ptr<FBO> dstFbo;

	float3 originPos;
	float3 originRight = RgtVector;
	float3 originUp = UpVector;
	float3 originForward = FwdVector;
	float worldScale = 200.0f;
};
