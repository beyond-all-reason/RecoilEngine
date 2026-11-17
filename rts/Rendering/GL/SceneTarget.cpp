/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SceneTarget.h"

#include <cmath>
#include <vector>

#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/glDebugGroup.hpp"
#include "Rendering/Shaders/Shader.h"
#include "Rendering/Shaders/ShaderHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Log/ILog.h"
#include "System/Misc/TracyDefs.h"
#include "System/StringUtil.h"

CONFIG(std::string, PBRSceneTarget).defaultValue("direct").description("direct (draw into the window) or hdr (draw into an HDR scene target, presented through one display pass). A failed allocation falls back to direct.");
CONFIG(std::string, PBRSceneTransfer).defaultValue("srgb-extended").description("How the scene target stores non-PBR colour: srgb-extended. linear is reserved and falls back to srgb-extended.");
CONFIG(std::string, PBRSceneDepthFormat).defaultValue("d24s8").description("d24s8 (copied into the window by a blit) or d32fs8 (depth copied by a shader, stencil not copied).");
CONFIG(std::string, PBRHDRColorFormat).defaultValue("rgba16f").description("rgba16f or r11g11b10f (half the memory; no alpha, no negative values, about 6 bits of mantissa). compare is reserved and falls back to rgba16f.");
CONFIG(std::string, PBRDisplayTransform).defaultValue("khronos-neutral").description("khronos-neutral, agx, aces, aces-hill, hable, uchimura, reinhard, reinhard-jodie, filmic-hejl, unreal (inspired by Unreal's filmic tone mapper), bar-custom, legacy (no curve) or linear-debug.");
CONFIG(std::string, PBRExposureMode).defaultValue("manual").description("Only manual is built; auto and compare are reserved and fall back to manual.");
CONFIG(float, PBRExposureEV).defaultValue(0.0f).description("Manual exposure in EV stops (linear multiplier 2^EV).");
CONFIG(std::string, PBRDisplayBoundary).defaultValue("after-world").description("after-world (screen effects draw into the window) or after-screen-effects (they draw into the scene and are tone-mapped).");
CONFIG(std::string, PBRDisplayDither).defaultValue("unreal").description("unreal (triangular grain of 1/255 hiding banding in the 8-bit output) or off.");
CONFIG(bool, PBRSceneTargetForceFailure).defaultValue(false).description("Test only: make the allocation fail to exercise the fallback to direct.");

CSceneTarget sceneTarget;

namespace {
	constexpr const char* SHADER_POOL = "[SceneTarget]";

	constexpr const char* DEPTH_D24S8 = "d24s8";
	constexpr float DITHER_8BIT = 1.0f / 255.0f;
	constexpr unsigned DITHER_SEED_PERIOD = 1024u;  // keeps the float seed exact
	constexpr const char* DEPTH_D32FS8 = "d32fs8";
	constexpr GLenum PBR_MASK_FORMAT = GL_R8;
	constexpr GLenum PBR_MASK_ATTACHMENT = GL_COLOR_ATTACHMENT1;

	const std::vector<std::string> CONFIG_KEYS = {
		"PBRSceneTarget", "PBRSceneTransfer", "PBRSceneDepthFormat", "PBRHDRColorFormat",
		"PBRDisplayTransform", "PBRExposureMode", "PBRExposureEV", "PBRDisplayBoundary",
		"PBRSceneTargetForceFailure", "PBRDisplayDither",
	};

	const char* DepthFormatName(GLenum f) { return (f == GL_DEPTH32F_STENCIL8)? DEPTH_D32FS8: DEPTH_D24S8; }
	GLenum DepthFormatType(GLenum f) { return (f == GL_DEPTH32F_STENCIL8)? GL_FLOAT_32_UNSIGNED_INT_24_8_REV: GL_UNSIGNED_INT_24_8; }

	void DrainGLErrors() { while (glGetError() != GL_NO_ERROR) {} }

	GLuint MakeTexture(GLenum internalFormat, GLenum format, GLenum type, int w, int h) {
		GLuint tex = 0;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, type, nullptr);
		return tex;
	}

	// R11F_G11F_B10F has no alpha, so its client format is GL_RGB
	GLenum ColorClientFormat(GLenum internalFormat) {
		return (internalFormat == GL_R11F_G11F_B10F)? GL_RGB: GL_RGBA;
	}

	GLuint MakeRenderbuffer(GLenum internalFormat, int samples, int w, int h) {
		GLuint rbo = 0;
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, internalFormat, w, h);
		return rbo;
	}

	// Saves and restores each state a pass changes (no glPushAttrib: absent in core profile). Add a state here before a pass changes it.
	class ScopedPassState {
	public:
		ScopedPassState() {
			glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFBO);
			glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFBO);
			glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
			glActiveTexture(GL_TEXTURE0);
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture2D);
			glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
			glGetIntegerv(GL_CURRENT_PROGRAM, &program);
			for (size_t i = 0; i < CAPABILITIES.size(); ++i)
				enabled[i] = glIsEnabled(CAPABILITIES[i]);
			glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
			glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
			glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
			glGetIntegerv(GL_STENCIL_WRITEMASK, &stencilMask);
			glGetIntegerv(GL_STENCIL_BACK_WRITEMASK, &stencilBackMask);
			glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &stencilClear);
			glGetIntegerv(GL_VIEWPORT, viewport);
			glGetIntegerv(GL_POLYGON_MODE, polygonMode);

			for (const GLenum cap: CAPABILITIES)
				glDisable(cap); // GL_FRAMEBUFFER_SRGB too: the pass encodes itself
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		~ScopedPassState() {
			for (size_t i = 0; i < CAPABILITIES.size(); ++i)
				(enabled[i] ? glEnable : glDisable)(CAPABILITIES[i]);
			glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
			glDepthMask(depthMask);
			glDepthFunc(depthFunc);
			glStencilMaskSeparate(GL_FRONT, stencilMask);
			glStencilMaskSeparate(GL_BACK, stencilBackMask);
			glClearStencil(stencilClear);
			glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
			glPolygonMode(GL_FRONT, polygonMode[0]);
			glPolygonMode(GL_BACK, polygonMode[1]);

			glUseProgram(program);
			glBindVertexArray(vao);
			glBindTexture(GL_TEXTURE_2D, texture2D);
			glActiveTexture(activeTexture);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, readFBO);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFBO);
		}
	private:
		static constexpr std::array<GLenum, 7> CAPABILITIES = {
			GL_DEPTH_TEST, GL_STENCIL_TEST, GL_BLEND, GL_SCISSOR_TEST, GL_CULL_FACE, GL_ALPHA_TEST, GL_FRAMEBUFFER_SRGB
		};
		std::array<GLboolean, CAPABILITIES.size()> enabled = {};
		GLboolean colorMask[4] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
		GLboolean depthMask = GL_TRUE;
		GLint depthFunc = GL_LESS, stencilMask = ~0, stencilBackMask = ~0, stencilClear = 0;
		GLint viewport[4] = {0, 0, 0, 0};
		GLint polygonMode[2] = {GL_FILL, GL_FILL};
		GLint drawFBO = 0, readFBO = 0, activeTexture = 0, texture2D = 0, vao = 0, program = 0;
	};
}


void CSceneTarget::Init()
{
	RECOIL_DETAILED_TRACY_ZONE;
	initialized = true;
	configHandler->NotifyOnChange(this, CONFIG_KEYS);
	settingsDirty = true;
}

// Built on first use, so a game that never sets PBRSceneTarget=hdr compiles nothing.
void CSceneTarget::CreatePrograms()
{
	RECOIL_DETAILED_TRACY_ZONE;
	programsCreated = true;

	displayShader = shaderHandler->CreateProgramObject(SHADER_POOL, "PBRDisplay");
	displayShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/FullscreenTriangleVS.glsl", "", GL_VERTEX_SHADER));
	// the shader loader has no #include, so the shared curves are prepended; a source containing "void main()" is used as the source itself
	const auto readShader = [](const char* path) {
		CFileHandler file(path);
		std::string text;
		if (file.FileExists()) {
			text.resize(file.FileSize());
			file.Read(text.data(), file.FileSize());
		} else {
			LOG_L(L_ERROR, "[SceneTarget] file not found \"%s\"", path);
		}
		return text;
	};
	const std::string displaySource = readShader("shaders/GLSL/PBRDisplayCurves.glsl") + "\n" + readShader("shaders/GLSL/PBRDisplayFS.glsl");
	displayShader->AttachShaderObject(shaderHandler->CreateShaderObject(displaySource, "", GL_FRAGMENT_SHADER));
	displayShader->Link();
	if (displayShader->IsValid()) {
		displayShader->Enable();
		displayShader->SetUniform("sceneColor", 0);
		displayShader->SetUniform("sceneMask", 1);
		displayShader->Disable();
	} else {
		LOG_L(L_ERROR, "[SceneTarget] display pass shader failed; PBRSceneTarget=hdr will fall back to direct:\n%s", displayShader->GetLog().c_str());
	}

	depthCopyShader = shaderHandler->CreateProgramObject(SHADER_POOL, "PBRSceneDepthCopy");
	depthCopyShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/FullscreenTriangleVS.glsl", "", GL_VERTEX_SHADER));
	depthCopyShader->AttachShaderObject(shaderHandler->CreateShaderObject("GLSL/PBRSceneDepthCopyFS.glsl", "", GL_FRAGMENT_SHADER));
	depthCopyShader->Link();
	if (depthCopyShader->IsValid()) {
		depthCopyShader->Enable();
		depthCopyShader->SetUniform("sceneDepth", 0);
		depthCopyShader->Disable();
	}
}

void CSceneTarget::Kill()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!initialized)
		return;

	configHandler->RemoveObserver(this);
	Free(res);
	sceneFBO = 0;
	worldPhase = false;
	if (programsCreated)
		shaderHandler->ReleaseProgramObjects(SHADER_POOL);
	displayShader = nullptr;
	depthCopyShader = nullptr;
	programsCreated = false;
	emptyVAO.Delete();
	initialized = false;
}

void CSceneTarget::ConfigNotify(const std::string& key, const std::string& value)
{
	settingsDirty = true;
}

void CSceneTarget::ReadSettings()
{
	RECOIL_DETAILED_TRACY_ZONE;
	settingsFallback.clear();
	blockedReason.clear();
	const auto note = [this](const std::string& s) {
		if (!settingsFallback.empty())
			settingsFallback += "; ";
		settingsFallback += s;
	};

	info.requested = configHandler->GetString("PBRSceneTarget");
	wantHDR = (info.requested == "hdr");
	if (!wantHDR && info.requested != "direct")
		note("PBRSceneTarget=" + info.requested + " is unknown, using direct");

	const std::string transferName = configHandler->GetString("PBRSceneTransfer");
	transfer = PBRDisplay::SceneTransfer::SRGBExtended;
	if (transferName != "srgb-extended")
		note("PBRSceneTransfer=" + transferName + " is not built, using srgb-extended");

	info.requestedDepthFormat = configHandler->GetString("PBRSceneDepthFormat");
	wantDepthFormat = (info.requestedDepthFormat == DEPTH_D32FS8)? GL_DEPTH32F_STENCIL8: GL_DEPTH24_STENCIL8;
	if (info.requestedDepthFormat != DEPTH_D24S8 && info.requestedDepthFormat != DEPTH_D32FS8)
		note("PBRSceneDepthFormat=" + info.requestedDepthFormat + " is unknown, using d24s8");

	const std::string colorName = configHandler->GetString("PBRHDRColorFormat");
	wantColorFormat = (colorName == "r11g11b10f")? GL_R11F_G11F_B10F: GL_RGBA16F;
	if (colorName != "rgba16f" && colorName != "r11g11b10f")
		note("PBRHDRColorFormat=" + colorName + " is not built, using rgba16f");

	const std::string transformName = configHandler->GetString("PBRDisplayTransform");
	if (!PBRDisplay::ParseTransform(transformName, transform)) {
		transform = PBRDisplay::Transform::KhronosNeutral;
		note("PBRDisplayTransform=" + transformName + " is unknown, using khronos-neutral");
	}

	const std::string exposureModeName = configHandler->GetString("PBRExposureMode");
	if (exposureModeName != "manual")
		note("PBRExposureMode=" + exposureModeName + " is not built, using manual");

	exposureEV = configHandler->GetFloat("PBRExposureEV");
	if (!std::isfinite(exposureEV)) {
		exposureEV = 0.0f;
		note("PBRExposureEV is not finite, using 0");
	}

	const std::string boundaryName = configHandler->GetString("PBRDisplayBoundary");
	boundary = (boundaryName == "after-screen-effects")? Boundary::AfterScreenEffects: Boundary::AfterWorld;
	if (boundaryName != "after-world" && boundaryName != "after-screen-effects")
		note("PBRDisplayBoundary=" + boundaryName + " is unknown, using after-world");

	forceFailure = configHandler->GetBool("PBRSceneTargetForceFailure");

	const std::string ditherName = configHandler->GetString("PBRDisplayDither");
	dither = (ditherName != "off");
	if (ditherName != "off" && ditherName != "unreal")
		note("PBRDisplayDither=" + ditherName + " is unknown, using unreal");

	info.colorFormat = (wantColorFormat == GL_R11F_G11F_B10F)? "r11g11b10f": "rgba16f";
	info.transfer = "srgb-extended";
	info.displayTransform = std::string(PBRDisplay::TransformName(transform));
	info.exposureMode = "manual";
	info.exposureEV = exposureEV;
	info.exposureLinear = PBRDisplay::ExposureLinear(exposureEV);
	info.boundary = (boundary == Boundary::AfterScreenEffects)? "after-screen-effects": "after-world";
	info.dither = dither? "unreal": "off";
}

bool CSceneTarget::Allocate(const Desc& d, Resources& out, std::string& error) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	GLint prevFBO = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
	DrainGLErrors();

	out = {};
	out.desc = d;
	glGenFramebuffers(1, &out.sceneFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, out.sceneFBO);

	if (d.samples == 0) {
		out.colorTex = MakeTexture(d.colorFormat, ColorClientFormat(d.colorFormat), GL_HALF_FLOAT, d.width, d.height);
		out.depthTex = MakeTexture(d.depthFormat, GL_DEPTH_STENCIL, DepthFormatType(d.depthFormat), d.width, d.height);
		out.maskTex = MakeTexture(PBR_MASK_FORMAT, GL_RED, GL_UNSIGNED_BYTE, d.width, d.height);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, out.colorTex, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, PBR_MASK_ATTACHMENT, GL_TEXTURE_2D, out.maskTex, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, out.depthTex, 0);
	} else {
		out.colorRBO = MakeRenderbuffer(d.colorFormat, d.samples, d.width, d.height);
		out.depthRBO = MakeRenderbuffer(d.depthFormat, d.samples, d.width, d.height);
		out.maskRBO = MakeRenderbuffer(PBR_MASK_FORMAT, d.samples, d.width, d.height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, out.colorRBO);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, PBR_MASK_ATTACHMENT, GL_RENDERBUFFER, out.maskRBO);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, out.depthRBO);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (status == GL_FRAMEBUFFER_COMPLETE && d.samples > 0) {
		glGenFramebuffers(1, &out.resolveFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, out.resolveFBO);
		out.resolveColorTex = MakeTexture(d.colorFormat, ColorClientFormat(d.colorFormat), GL_HALF_FLOAT, d.width, d.height);
		out.resolveDepthTex = MakeTexture(d.depthFormat, GL_DEPTH_STENCIL, DepthFormatType(d.depthFormat), d.width, d.height);
		out.resolveMaskTex = MakeTexture(PBR_MASK_FORMAT, GL_RED, GL_UNSIGNED_BYTE, d.width, d.height);
		glBindTexture(GL_TEXTURE_2D, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, out.resolveColorTex, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, PBR_MASK_ATTACHMENT, GL_TEXTURE_2D, out.resolveMaskTex, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, out.resolveDepthTex, 0);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	}

	// out of memory can leave a framebuffer "complete" but set a GL error
	const GLenum glError = glGetError();
	glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);

	if (status == GL_FRAMEBUFFER_COMPLETE && glError == GL_NO_ERROR)
		return true;

	error = (status != GL_FRAMEBUFFER_COMPLETE)
		? ("framebuffer incomplete, status 0x" + IntToString(status, "%x"))
		: ("GL error 0x" + IntToString(glError, "%x"));
	error += " for " + IntToString(d.width) + "x" + IntToString(d.height) + " samples " + IntToString(d.samples) + " depth " + DepthFormatName(d.depthFormat);
	Free(out);
	return false;
}

void CSceneTarget::Free(Resources& r)
{
	RECOIL_DETAILED_TRACY_ZONE;
	for (GLuint* fbo: {&r.sceneFBO, &r.resolveFBO}) {
		if (*fbo != 0)
			glDeleteFramebuffers(1, fbo);
	}
	for (GLuint* tex: {&r.colorTex, &r.depthTex, &r.resolveColorTex, &r.resolveDepthTex, &r.maskTex, &r.resolveMaskTex}) {
		if (*tex != 0)
			glDeleteTextures(1, tex);
	}
	for (GLuint* rbo: {&r.colorRBO, &r.depthRBO, &r.maskRBO}) {
		if (*rbo != 0)
			glDeleteRenderbuffers(1, rbo);
	}
	r = {};
}

void CSceneTarget::BeginFrame()
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	worldPhase = false;
	resolvedThisFrame = false;

	if (!initialized)
		Init();
	if (settingsDirty.exchange(false))
		ReadSettings();

	info.fallback = settingsFallback;
	info.active = "direct";

	const auto goDirect = [this](const std::string& why) {
		Free(res);
		sceneFBO = 0;
		width = height = samples = 0;
		info.width = info.height = info.samples = 0;
		info.depthCopy = "none";
		info.stencilCopied = false;
		if (!why.empty())
			info.fallback = info.fallback.empty()? why: (why + "; " + info.fallback);
	};

	if (!wantHDR) {
		goDirect("");
		return;
	}
	if (!blockedReason.empty()) {
		goDirect(blockedReason);
		return;
	}
	if (!programsCreated)
		CreatePrograms();
	if (forceFailure) {
		blockedReason = "forced allocation failure (PBRSceneTargetForceFailure), using direct";
		info.allocationFailures++;
		goDirect(blockedReason);
		return;
	}
	if (!displayShader->IsValid()) {
		goDirect("display pass shader failed to link, using direct");
		return;
	}

	// multisample blits need equal sample counts, so the scene matches the window
	GLint windowSamples = 0;
	glGetIntegerv(GL_SAMPLES, &windowSamples);

	const Desc want = {globalRendering->winSizeX, globalRendering->winSizeY, windowSamples, wantColorFormat, wantDepthFormat};

	if (!(res.desc == want) || res.sceneFBO == 0) {
		Resources fresh;
		std::string error;

		if (Allocate(want, fresh, error)) {
			Free(res);
			res = fresh;
			info.revision++;
			depthBlitChecked = false;
			depthBlitWorks = true;
		} else {
			info.allocationFailures++;
			LOG_L(L_WARNING, "[SceneTarget] scene target allocation failed: %s", error.c_str());

			// a failed format change keeps the last valid target if it still fits the window
			const bool keep = (res.sceneFBO != 0 && res.desc.width == want.width && res.desc.height == want.height && res.desc.samples == want.samples);
			if (!keep) {
				blockedReason = "allocation failed (" + error + "), using direct";
				goDirect(blockedReason);
				return;
			}
			info.fallback += (info.fallback.empty()? "": "; ") + std::string("allocation failed (") + error + "), kept " + DepthFormatName(res.desc.depthFormat);
			wantDepthFormat = res.desc.depthFormat;
		}
	}

	sceneFBO = res.sceneFBO;
	width = res.desc.width;
	height = res.desc.height;
	samples = res.desc.samples;

	info.active = "hdr";
	info.width = width;
	info.height = height;
	info.samples = samples;
	info.depthFormat = DepthFormatName(res.desc.depthFormat);

	glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
	ClearPBRMask();
	worldPhase = true;
#endif
}

void CSceneTarget::ClearPBRMask()
{
	// glClearBuffer obeys the scissor test and the colour write mask
	const GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
	glDisable(GL_SCISSOR_TEST);
	const GLenum both[] = {GL_COLOR_ATTACHMENT0, PBR_MASK_ATTACHMENT};
	glDrawBuffers(2, both);
	glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	const GLfloat zero[] = {0.0f, 0.0f, 0.0f, 0.0f};
	glClearBufferfv(GL_COLOR, 1, zero);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	maskWrites = false;
	if (scissor)
		glEnable(GL_SCISSOR_TEST);
}

bool CSceneTarget::SetPBRMaskWrites(bool on)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!worldPhase)
		return false;
	GLint drawFBO = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFBO);
	if (GLuint(drawFBO) != sceneFBO)
		return false;
	if (on) {
		const GLenum both[] = {GL_COLOR_ATTACHMENT0, PBR_MASK_ATTACHMENT};
		glDrawBuffers(2, both);
	} else {
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
	}
	maskWrites = on;
	return on;
}

void CSceneTarget::ResolveScene(GLbitfield mask)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (samples == 0 || res.resolveFBO == 0)
		return;

	GLint prevRead = 0, prevDraw = 0;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevRead);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDraw);
	// blits obey the scissor test
	const GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
	glDisable(GL_SCISSOR_TEST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, res.sceneFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, res.resolveFBO);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, mask, GL_NEAREST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, prevRead);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDraw);
	if (scissor)
		glEnable(GL_SCISSOR_TEST);
}

void CSceneTarget::ResolvePBRMask()
{
	if (samples == 0 || res.resolveFBO == 0)
		return;

	// point read and draw buffer at the mask for the blit, then back at the colour
	glBindFramebuffer(GL_READ_FRAMEBUFFER, res.sceneFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, res.resolveFBO);
	glReadBuffer(PBR_MASK_ATTACHMENT);
	glDrawBuffer(PBR_MASK_ATTACHMENT);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, res.resolveFBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
}

GLuint CSceneTarget::GetColorTexture()
{
	if (res.sceneFBO == 0)
		return 0;
	if (samples == 0)
		return res.colorTex;
	if (worldPhase)
		ResolveScene(GL_COLOR_BUFFER_BIT);
	return res.resolveColorTex;
}

GLuint CSceneTarget::GetDepthTexture()
{
	if (res.sceneFBO == 0)
		return 0;
	if (samples == 0)
		return res.depthTex;
	if (worldPhase || !resolvedThisFrame) {
		ResolveScene(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		resolvedThisFrame = !worldPhase;
	}
	return res.resolveDepthTex;
}

GLuint CSceneTarget::ResolvedReadFBO(GLuint readFBO)
{
	if (samples == 0 || readFBO == 0 || readFBO != res.sceneFBO)
		return readFBO;

	ResolveScene(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	return res.resolveFBO;
}

void CSceneTarget::Present(Boundary at)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!worldPhase || at != boundary)
		return;

	SCOPED_GL_DEBUGGROUP("PBRDisplayPass");
	worldPhase = false;

	{
		ScopedPassState state;
		if (maskWrites) {
			// a draw left the mask on
			glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			maskWrites = false;
		}
		ResolveScene(GL_COLOR_BUFFER_BIT);
		ResolvePBRMask();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, width, height);
		DrawDisplayPass();
		CopyDepthToWindow();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CSceneTarget::DrawDisplayPass()
{
	displayShader->Enable();
	displayShader->SetUniform("displayTransform", int(transform));
	displayShader->SetUniform("sceneTransfer", int(transfer));
	displayShader->SetUniform("exposureLinear", PBRDisplay::ExposureLinear(exposureEV));
	displayShader->SetUniform("ditherAmount", dither? DITHER_8BIT: 0.0f);
	displayShader->SetUniform("ditherSeed", float(ditherFrame++ % DITHER_SEED_PERIOD));
	displayShader->SetUniform("useSceneMask", 1);

	GLint prevUnit1 = 0;
	glActiveTexture(GL_TEXTURE1);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevUnit1);
	glBindTexture(GL_TEXTURE_2D, (samples > 0)? res.resolveMaskTex: res.maskTex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, (samples > 0)? res.resolveColorTex: res.colorTex);
	glBindVertexArray(emptyVAO.GetId());
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, prevUnit1);
	glActiveTexture(GL_TEXTURE0);
	displayShader->Disable();
}

void CSceneTarget::CopyDepthToWindow()
{
	if (res.desc.depthFormat == GL_DEPTH24_STENCIL8 && depthBlitWorks) {
		if (!depthBlitChecked)
			DrainGLErrors();

		glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneFBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

		// checked once per allocation: a window with a different depth format refuses the blit
		if (!depthBlitChecked) {
			depthBlitChecked = true;
			const GLenum err = glGetError();
			depthBlitWorks = (err == GL_NO_ERROR);
			if (!depthBlitWorks)
				LOG_L(L_WARNING, "[SceneTarget] depth/stencil blit into the window failed (GL error 0x%x); depth is copied by a shader, stencil is not copied", err);
		}
		if (depthBlitWorks) {
			info.depthCopy = "blit";
			info.stencilCopied = true;
			return;
		}
	}

	if (depthCopyShader == nullptr || !depthCopyShader->IsValid()) {
		info.depthCopy = "none";
		info.stencilCopied = false;
		return;
	}

	ResolveScene(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	resolvedThisFrame = true;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// this path cannot copy stencil, so leave it zero
	glStencilMask(~0u);
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);
	glDepthMask(GL_TRUE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	depthCopyShader->Enable();
	glBindTexture(GL_TEXTURE_2D, (samples > 0)? res.resolveDepthTex: res.depthTex);
	glBindVertexArray(emptyVAO.GetId());
	glDrawArrays(GL_TRIANGLES, 0, 3);
	depthCopyShader->Disable();

	info.depthCopy = "shader";
	info.stencilCopied = false;
}

DisplayTransformTestResult CSceneTarget::TestDisplayTransforms()
{
	RECOIL_DETAILED_TRACY_ZONE;
	DisplayTransformTestResult result;
#ifndef HEADLESS
	if (!initialized)
		Init();
	if (!programsCreated)
		CreatePrograms();
	if (!displayShader->IsValid()) {
		result.failure = "display pass shader failed to link";
		return result;
	}

	// Row 0 is gray and must be monotonic; row 1 has unequal channels (matrix curves need not be monotonic per channel).
	constexpr int RAMP_SIZE = 257;
	constexpr int RAMP_ROWS = 2;
	constexpr float RAMP_MAX = 64.0f;
	// two half-float steps at 1.0: RGBA16F quantisation plus GPU pow/exp/log2 differences
	constexpr float TOLERANCE = 2.0f / 1024.0f;

	constexpr int RAMP_TEXELS = RAMP_SIZE * RAMP_ROWS;
	std::vector<float> ramp(RAMP_TEXELS * 4);
	for (int i = 0; i < RAMP_SIZE; ++i) {
		const float t = float(i) / float(RAMP_SIZE - 1);
		const float x = RAMP_MAX * t * t;
		const float gray[4] = {x, x, x, 1.0f};
		const float color[4] = {x, x * 0.5f, x * 0.25f, 1.0f};
		std::copy(gray, gray + 4, &ramp[i * 4]);
		std::copy(color, color + 4, &ramp[(RAMP_SIZE + i) * 4]);
	}

	ScopedPassState state;

	const GLuint inTex = MakeTexture(GL_RGBA16F, GL_RGBA, GL_FLOAT, RAMP_SIZE, RAMP_ROWS);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, RAMP_SIZE, RAMP_ROWS, GL_RGBA, GL_FLOAT, ramp.data());
	// read back the quantised inputs for the CPU reference
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, ramp.data());

	const GLuint outTex = MakeTexture(GL_RGBA32F, GL_RGBA, GL_FLOAT, RAMP_SIZE, RAMP_ROWS);
	GLuint outFBO = 0;
	glGenFramebuffers(1, &outFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, outFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outTex, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		result.failure = "RGBA32F test framebuffer incomplete";
	} else {
		result.ran = true;
		result.pass = true;
		result.samples = RAMP_TEXELS;

		std::vector<float> out(RAMP_TEXELS * 4);
		glViewport(0, 0, RAMP_SIZE, RAMP_ROWS);
		glBindTexture(GL_TEXTURE_2D, inTex);
		glBindVertexArray(emptyVAO.GetId());
		displayShader->Enable();
		displayShader->SetUniform("exposureLinear", 1.0f);
		displayShader->SetUniform("ditherAmount", 0.0f);
		displayShader->SetUniform("useSceneMask", 0);

		using namespace PBRDisplay;
		for (int transferIdx = 0; transferIdx <= int(SceneTransfer::Linear); ++transferIdx) {
			for (int t = 0; t < int(Transform::Count); ++t) {
				displayShader->SetUniform("displayTransform", t);
				displayShader->SetUniform("sceneTransfer", transferIdx);
				glDrawArrays(GL_TRIANGLES, 0, 3);
				glReadPixels(0, 0, RAMP_SIZE, RAMP_ROWS, GL_RGBA, GL_FLOAT, out.data());

				float prev[3] = {-1.0f, -1.0f, -1.0f};
				for (int i = 0; i < RAMP_TEXELS; ++i) {
					const RGB in = {ramp[i * 4 + 0], ramp[i * 4 + 1], ramp[i * 4 + 2]};
					const RGB ref = ApplyDisplayPass(Transform(t), SceneTransfer(transferIdx), 1.0f, in);
					const float gpu[3] = {out[i * 4 + 0], out[i * 4 + 1], out[i * 4 + 2]};
					const float cpu[3] = {ref.r, ref.g, ref.b};

					for (int c = 0; c < 3; ++c) {
						const bool finite = std::isfinite(gpu[c]);
						// relative error: the legacy transform is unbounded
						const float scale = std::max(1.0f, std::fabs(cpu[c]));
						const float err = finite? (std::fabs(gpu[c] - cpu[c]) / scale): 1e30f;
						const bool monotonic = (i >= RAMP_SIZE) || (gpu[c] + TOLERANCE >= prev[c]);
						prev[c] = gpu[c];

						if (err > result.maxError) {
							result.maxError = err;
							result.worstTransform = std::string(TransformName(Transform(t))) + ((transferIdx == 0)? " srgb-extended": " linear");
						}
						if (!finite || err > TOLERANCE || !monotonic)
							result.pass = false;
					}
				}
			}
		}
		displayShader->Disable();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &outFBO);
	glDeleteTextures(1, &inTex);
	glDeleteTextures(1, &outTex);
#else
	result.failure = "headless build";
#endif
	return result;
}


ScopedSceneReadResolve::ScopedSceneReadResolve()
{
	if (!sceneTarget.IsMultisampled() || sceneTarget.GetSceneFBO() == 0)
		return;

	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFBO);
	const GLuint readFBO = sceneTarget.ResolvedReadFBO(prevReadFBO);
	rebound = (readFBO != GLuint(prevReadFBO));
	if (rebound)
		glBindFramebuffer(GL_READ_FRAMEBUFFER, readFBO);
}

ScopedSceneReadResolve::~ScopedSceneReadResolve()
{
	if (rebound)
		glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
}
