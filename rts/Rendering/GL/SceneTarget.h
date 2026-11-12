/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SCENE_TARGET_H
#define SCENE_TARGET_H

// The PBR HDR scene target and the one display pass (planning M1, increments
// I5-I7). With PBRSceneTarget=hdr the world is drawn into an RGBA16F target
// that replaces the window framebuffer during the world phase (FBO::Unbind and
// Lua's nil framebuffer bind it); Present() then decodes, exposes, tone-maps and
// encodes it once into the window and copies depth and stencil there, so the
// screen phase draws display-referred as before. With PBRSceneTarget=direct
// (the default) nothing here binds anything and the frame is today's.

#include <array>
#include <atomic>
#include <cstdint>
#include <string>

#include "Rendering/GL/DisplayTransforms_pbr.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VAO.h"

namespace Shader { struct IProgramObject; }

struct SceneTargetInfo {
	std::string requested;         // PBRSceneTarget as saved
	std::string active;            // "direct" or "hdr"
	std::string fallback;          // why active differs from requested, or what was substituted
	std::string colorFormat;       // active PBRHDRColorFormat
	std::string depthFormat;       // active PBRSceneDepthFormat
	std::string requestedDepthFormat;
	std::string transfer;          // active PBRSceneTransfer
	std::string displayTransform;  // active PBRDisplayTransform
	std::string exposureMode;
	std::string boundary;          // PBRDisplayBoundary
	std::string depthCopy;         // "blit", "shader" or "none"
	bool stencilCopied = false;
	int width = 0;
	int height = 0;
	int samples = 0;
	int revision = 0;              // increments on every successful (re)allocation
	int allocationFailures = 0;
	float exposureEV = 0.0f;
	float exposureLinear = 1.0f;
};

struct DisplayTransformTestResult {
	bool ran = false;
	bool pass = false;
	int samples = 0;              // ramp values checked per transform
	float maxError = 0.0f;        // worst |GPU - CPU| over all transforms
	std::string worstTransform;
	std::string failure;          // empty unless the test could not run
};

class CSceneTarget {
public:
	enum class Boundary { AfterWorld, AfterScreenEffects };

	// Before the world is drawn: reads the settings, (re)allocates, binds the
	// scene target when hdr is active. Leaves framebuffer 0 bound otherwise.
	void BeginFrame();
	// Runs the display pass if this boundary is the configured one and the
	// scene target is bound this frame; otherwise does nothing.
	void Present(Boundary at);

	void Kill();

	// True between BeginFrame and Present while hdr is active.
	bool IsWorldPhase() const { return worldPhase; }

	// The framebuffer that stands for "the window" right now (0 outside the world phase).
	GLuint GetDefaultFBO() const { return worldPhase? sceneFBO: 0; }
	GLuint GetSceneFBO() const { return sceneFBO; }
	bool IsMultisampled() const { return samples > 0; }

	// Single-sample views of the scene for $scene_color / $scene_depth; inside
	// the world phase a multisampled scene is resolved first.
	GLuint GetColorTexture();
	GLuint GetDepthTexture();
	std::array<int, 2> GetSize() const { return {width, height}; }

	// For screen copies (gl.CopyToTexture, gl.ReadPixels, gl.BlitFBO(nil, ...)):
	// when `readFBO` is the multisampled scene, resolves it and returns the
	// single-sample resolve framebuffer to read from instead; else `readFBO`.
	GLuint ResolvedReadFBO(GLuint readFBO);

	SceneTargetInfo GetInfo() const { return info; }

	// Renders a 0..64 brightness ramp through every curve on the GPU and compares
	// with PBRDisplay::ApplyDisplayPass (acceptance test of increment I6).
	DisplayTransformTestResult TestDisplayTransforms();

	void ConfigNotify(const std::string& key, const std::string& value);

private:
	struct Desc {
		int width = 0;
		int height = 0;
		int samples = 0;
		GLenum colorFormat = 0;
		GLenum depthFormat = 0;
		bool operator == (const Desc& o) const {
			return width == o.width && height == o.height && samples == o.samples && colorFormat == o.colorFormat && depthFormat == o.depthFormat;
		}
	};
	struct Resources {
		GLuint sceneFBO = 0;
		GLuint colorTex = 0;      // single-sample scene colour (no MSAA)
		GLuint depthTex = 0;      // single-sample scene depth-stencil (no MSAA)
		GLuint colorRBO = 0;      // multisampled scene colour
		GLuint depthRBO = 0;      // multisampled scene depth-stencil
		GLuint resolveFBO = 0;    // MSAA only: single-sample copies for reading
		GLuint resolveColorTex = 0;
		GLuint resolveDepthTex = 0;
		Desc desc;
	};

	void Init();
	void ReadSettings();
	bool Allocate(const Desc& desc, Resources& res, std::string& error) const;
	static void Free(Resources& res);
	void Adopt(Resources& res);
	void ResolveScene(GLbitfield mask);
	void DrawDisplayPass();
	void CopyDepthToWindow();

private:
	bool initialized = false;
	bool worldPhase = false;
	std::atomic<bool> settingsDirty = {true};

	// requested settings (re-read when a key changes)
	bool wantHDR = false;
	bool forceFailure = false;
	Boundary boundary = Boundary::AfterWorld;
	GLenum wantDepthFormat = 0;
	PBRDisplay::Transform transform = PBRDisplay::Transform::KhronosNeutral;
	PBRDisplay::SceneTransfer transfer = PBRDisplay::SceneTransfer::SRGBExtended;
	float exposureEV = 0.0f;

	Resources res;
	GLuint sceneFBO = 0;
	int width = 0;
	int height = 0;
	int samples = 0;
	bool resolvedThisFrame = false;
	bool depthBlitChecked = false;
	bool depthBlitWorks = true;

	Shader::IProgramObject* displayShader = nullptr;
	Shader::IProgramObject* depthCopyShader = nullptr;
	VAO emptyVAO;

	SceneTargetInfo info;
	std::string settingsFallback;
	std::string blockedReason; // set when allocation failed; cleared by a settings change
};

extern CSceneTarget sceneTarget;

// Makes a screen copy from the multisampled scene work: for the lifetime of this
// object the read framebuffer is the scene's single-sample resolve, if the scene
// was the read framebuffer. Copies from a multisampled framebuffer object into a
// texture fail in GL, while the same copy from a multisampled window works.
class ScopedSceneReadResolve {
public:
	ScopedSceneReadResolve();
	~ScopedSceneReadResolve();
private:
	GLint prevReadFBO = 0;
	bool rebound = false;
};

#endif
