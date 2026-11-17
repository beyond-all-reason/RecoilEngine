/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

// With PBRSceneTarget=hdr the world is drawn into an HDR target that stands in for the window
// framebuffer; Present() then runs the one display pass into the window.
// Only PBR pixels are curved: a second attachment holds a mask (1 where a PBR model was drawn),
// written only between SetPBRMaskWrites(true) and (false).

#include <array>
#include <atomic>
#include <cstdint>
#include <string>

#include "Rendering/GL/DisplayTransforms.h"
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
	std::string dither;            // PBRDisplayDither: "triangular" or "off"
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

	// Before the world is drawn: reads settings, (re)allocates, binds the scene target when hdr is active.
	void BeginFrame();
	// Runs the display pass if `at` is the configured boundary and the scene target is bound.
	void Present(Boundary at);

	void Kill();

	bool IsWorldPhase() const { return worldPhase; }

	// The framebuffer standing for "the window" (0 outside the world phase).
	GLuint GetDefaultFBO() const { return worldPhase? sceneFBO: 0; }
	GLuint GetSceneFBO() const { return sceneFBO; }
	bool IsMultisampled() const { return samples > 0; }

	// Single-sample views for $scene_color / $scene_depth; a multisampled scene is resolved first.
	GLuint GetColorTexture();
	GLuint GetDepthTexture();
	std::array<int, 2> GetSize() const { return {width, height}; }

	// For screen copies: if `readFBO` is the multisampled scene, resolves it and returns the resolve FBO instead.
	GLuint ResolvedReadFBO(GLuint readFBO);

	// While on, every fragment shader drawn must write output location 1 (1 = PBR, 0 = display-referred).
	// Only acts while the scene target is the bound draw framebuffer; returns whether the mask is written.
	bool SetPBRMaskWrites(bool on);

	SceneTargetInfo GetInfo() const { return info; }

	// Renders a 0..64 ramp through every curve on the GPU and compares with PBRDisplay::ApplyDisplayPass.
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
		GLuint colorTex = 0;      // no MSAA
		GLuint depthTex = 0;      // no MSAA
		GLuint colorRBO = 0;      // MSAA
		GLuint depthRBO = 0;      // MSAA
		GLuint resolveFBO = 0;    // MSAA only
		GLuint resolveColorTex = 0;
		GLuint resolveDepthTex = 0;
		GLuint maskTex = 0;        // no MSAA
		GLuint maskRBO = 0;        // MSAA
		GLuint resolveMaskTex = 0; // MSAA only
		Desc desc;
	};

	void Init();
	void CreatePrograms();
	void ReadSettings();
	bool Allocate(const Desc& desc, Resources& res, std::string& error) const;
	static void Free(Resources& res);
	void Adopt(Resources& res);
	void ResolveScene(GLbitfield mask);
	void ResolvePBRMask();
	void ClearPBRMask();
	void DrawDisplayPass();
	void CopyDepthToWindow();

private:
	bool initialized = false;
	bool programsCreated = false;
	bool worldPhase = false;
	std::atomic<bool> settingsDirty = {true};

	// re-read when a config key changes
	bool wantHDR = false;
	bool forceFailure = false;
	Boundary boundary = Boundary::AfterWorld;
	GLenum wantDepthFormat = 0;
	GLenum wantColorFormat = GL_RGBA16F;
	PBRDisplay::Transform transform = PBRDisplay::Transform::KhronosNeutral;
	PBRDisplay::SceneTransfer transfer = PBRDisplay::SceneTransfer::SRGBExtended;
	float exposureEV = 0.0f;
	bool dither = true;
	unsigned ditherFrame = 0;

	Resources res;
	GLuint sceneFBO = 0;
	int width = 0;
	int height = 0;
	int samples = 0;
	bool resolvedThisFrame = false;
	bool maskWrites = false;
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

// Copies from a multisampled FBO into a texture fail in GL (unlike from a multisampled window), so while
// this object lives the read framebuffer is the scene's single-sample resolve, if the scene was being read.
class ScopedSceneReadResolve {
public:
	ScopedSceneReadResolve();
	~ScopedSceneReadResolve();
private:
	GLint prevReadFBO = 0;
	bool rebound = false;
};
