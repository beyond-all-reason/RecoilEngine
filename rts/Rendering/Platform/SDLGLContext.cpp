/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GlobalRendering.h"
#include "Rendering/GlobalRenderingInfo.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>

#include <SDL.h>
#include <fmt/format.h>
#include <fmt/printf.h>

#include "Rendering/GL/glxHandler.h"
#include "Rendering/GL/myGL.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Platform/WindowManagerHelper.h"
#include "System/Platform/errorhandler.h"

SDL_Window* CGlobalRendering::CreateSDLWindow(const char* title) const
{
	const int2 newRes = GetCfgWinRes();
	SDL_Window* newWindow = nullptr;

	const std::array aaLvls = {msaaLevel, msaaLevel / 2, msaaLevel / 4, msaaLevel / 8, msaaLevel / 16, msaaLevel / 32, 0};
	const std::array zbBits = {24, 32, 16};

	const char* frmts[2] = {
		"[GR::%s] error \"%s\" using %dx anti-aliasing and %d-bit depth-buffer for main window",
		"[GR::%s] using %dx anti-aliasing and %d-bit depth-buffer (PF=\"%s\") for main window"
	};
	const char* wpfName = "";

	bool borderless_ = configHandler->GetBool("WindowBorderless");
	bool fullScreen_ = configHandler->GetBool("Fullscreen");
	int winPosX_ = configHandler->GetInt("WindowPosX");
	int winPosY_ = configHandler->GetInt("WindowPosY");

	uint32_t sdlFlags = (SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	sdlFlags |= (borderless_ ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN) * fullScreen_;
	sdlFlags |= (SDL_WINDOW_BORDERLESS * borderless_);

	for (size_t i = 0; i < std::size(aaLvls) && (newWindow == nullptr); i++) {
		if (i > 0 && aaLvls[i] == aaLvls[i - 1])
			break;

		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, aaLvls[i] > 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, aaLvls[i]);

		for (size_t j = 0; j < std::size(zbBits) && (newWindow == nullptr); j++) {
			SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, zbBits[j]);

			if ((newWindow = SDL_CreateWindow(title, winPosX_, winPosY_, newRes.x, newRes.y, sdlFlags)) == nullptr) {
				LOG_L(L_WARNING, frmts[0], __func__, SDL_GetError(), aaLvls[i], zbBits[j]);
				continue;
			}

			LOG(frmts[1], __func__, aaLvls[i], zbBits[j], wpfName = SDL_GetPixelFormatName(SDL_GetWindowPixelFormat(newWindow)));
		}
	}

	if (newWindow == nullptr) {
		auto buf = fmt::sprintf("[GR::%s] could not create SDL-window\n", __func__);
		handleerror(nullptr, buf.c_str(), "ERROR", MBF_OK | MBF_EXCL);
		return nullptr;
	}

	UpdateWindowBorders(newWindow);
	return newWindow;
}

SDL_GLContext CGlobalRendering::CreateGLContext(const int2& minCtx)
{
	SDL_GLContext newContext = nullptr;

	constexpr int2 glCtxs[] = {{2, 0}, {2, 1}, {3, 0}, {3, 1}, {3, 2}, {3, 3}, {4, 0}, {4, 1}, {4, 2}, {4, 3}, {4, 4}, {4, 5}, {4, 6}};
	int2 cmpCtx;

	if (std::find(std::begin(glCtxs), std::end(glCtxs), minCtx) == std::end(glCtxs)) {
		handleerror(nullptr, "illegal OpenGL context-version specified, aborting", "ERROR", MBF_OK | MBF_EXCL);
		return nullptr;
	}

	if ((newContext = SDL_GL_CreateContext(sdlWindow)) != nullptr)
		return newContext;

	const char* frmts[] = {"[GR::%s] error (\"%s\") creating main GL%d.%d %s-context", "[GR::%s] created main GL%d.%d %s-context"};
	const char* profs[] = {"compatibility", "core"};

	char buf[1024] = {0};
	SNPRINTF(buf, sizeof(buf), frmts[false], __func__, SDL_GetError(), minCtx.x, minCtx.y, profs[forceCoreContext]);

	for (const int2 tmpCtx: glCtxs) {
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, tmpCtx.x);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, tmpCtx.y);

		for (uint32_t mask: {SDL_GL_CONTEXT_PROFILE_CORE, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY}) {
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, mask);

			if ((newContext = SDL_GL_CreateContext(sdlWindow)) == nullptr) {
				LOG_L(L_WARNING, frmts[false], __func__, SDL_GetError(), tmpCtx.x, tmpCtx.y, profs[mask == SDL_GL_CONTEXT_PROFILE_CORE]);
			} else {
				if (mask == SDL_GL_CONTEXT_PROFILE_COMPATIBILITY && cmpCtx.x == 0 && tmpCtx.x >= minCtx.x)
					cmpCtx = tmpCtx;

				LOG_L(L_WARNING, frmts[true], __func__, tmpCtx.x, tmpCtx.y, profs[mask == SDL_GL_CONTEXT_PROFILE_CORE]);
			}

			SDL_GL_DeleteContext(newContext);
		}
	}

	if (cmpCtx.x == 0) {
		handleerror(nullptr, buf, "ERROR", MBF_OK | MBF_EXCL);
		return nullptr;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, cmpCtx.x);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, cmpCtx.y);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

	return (newContext = SDL_GL_CreateContext(sdlWindow));
}

bool CGlobalRendering::CreateWindowAndContext(const char* title)
{
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		LOG_L(L_FATAL, "[GR::%s] error \"%s\" initializing SDL", __func__, SDL_GetError());
		return false;
	}

	if (!CheckAvailableVideoModes()) {
		handleerror(nullptr, "desktop color-depth should be at least 24 bits per pixel, aborting", "ERROR", MBF_OK | MBF_EXCL);
		return false;
	}

	const char* mesaGL = getenv("MESA_GL_VERSION_OVERRIDE");
	const char* softGL = getenv("LIBGL_ALWAYS_SOFTWARE");

	const int2 minCtx = (mesaGL != nullptr && std::strlen(mesaGL) >= 3) ?
		int2{std::max(mesaGL[0] - '0', 3), std::max(mesaGL[2] - '0', 0)} :
		int2{configHandler->GetInt("GLContextMajorVersion"), configHandler->GetInt("GLContextMinorVersion")};

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, forceCoreContext ? SDL_GL_CONTEXT_PROFILE_CORE : SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, minCtx.x);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minCtx.y);

	if (msaaLevel > 0) {
		if (softGL != nullptr)
			LOG_L(L_WARNING, "MSAALevel > 0 and LIBGL_ALWAYS_SOFTWARE set, this will very likely crash!");

		if (msaaLevel % 2 == 1)
			++msaaLevel;
	}

	if ((sdlWindow = CreateSDLWindow(title)) == nullptr)
		return false;

	if (configHandler->GetInt("MinimizeOnFocusLoss") == 0)
		SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

	SetWindowAttributes(sdlWindow);

#if !defined(HEADLESS)
	if (configHandler->GetBool("BlockCompositing"))
		WindowManagerHelper::BlockCompositing(sdlWindow);
#endif

	if ((glContext = CreateGLContext(minCtx)) == nullptr)
		return false;

	gladLoadGL();
	GLX::Load(sdlWindow);

	if (!CheckGLContextVersion(minCtx)) {
		int ctxProfile = 0;
		SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &ctxProfile);

		const std::string errStr = fmt::format("current OpenGL version {}.{}(core={}) is less than required {}.{}(core={}), aborting",
			globalRenderingInfo.glContextVersion.x, globalRenderingInfo.glContextVersion.y, globalRenderingInfo.glContextIsCore,
			minCtx.x, minCtx.y, (ctxProfile == SDL_GL_CONTEXT_PROFILE_CORE)
		);

		handleerror(nullptr, errStr.c_str(), "ERROR", MBF_OK | MBF_EXCL);
		return false;
	}

	MakeCurrentContext(false);
	SDL_DisableScreenSaver();
	return true;
}

void CGlobalRendering::DestroyWindowAndContext()
{
	if (!sdlWindow)
		return;

	WindowManagerHelper::SetIconSurface(sdlWindow, nullptr);
	SetWindowInputGrabbing(false);

	SDL_GL_MakeCurrent(sdlWindow, nullptr);
	SDL_DestroyWindow(sdlWindow);

#if !defined(HEADLESS)
	if (glContext)
		SDL_GL_DeleteContext(glContext);
#endif

	sdlWindow = nullptr;
	glContext = nullptr;

	GLX::Unload();
}

void CGlobalRendering::KillSDL() const
{
#if !defined(HEADLESS)
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif

	SDL_EnableScreenSaver();
	SDL_Quit();
}
