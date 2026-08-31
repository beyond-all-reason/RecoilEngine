/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "MacEGLContext.h"

#include <algorithm>

#include <EGL/egl.h>

#include "System/Log/ILog.h"

namespace {
	EGLDisplay eglDisplay = EGL_NO_DISPLAY;
	EGLSurface eglSurface = EGL_NO_SURFACE;
	EGLContext eglContext = EGL_NO_CONTEXT;
	EGLConfig eglConfig = nullptr;

	EGLSurface CreatePbuffer(const int2& size)
	{
		const EGLint surfAttribs[] = {
			EGL_WIDTH, std::max(size.x, 1),
			EGL_HEIGHT, std::max(size.y, 1),
			EGL_NONE
		};

		const EGLSurface surface = eglCreatePbufferSurface(eglDisplay, eglConfig, surfAttribs);

		if (surface == EGL_NO_SURFACE)
			LOG_L(L_ERROR, "[MacEGL::%s] error (0x%x) creating a %dx%d pbuffer surface", __func__, eglGetError(), size.x, size.y);

		return surface;
	}

	// highest first, the sweep stops once it drops below the wanted version
	constexpr int2 glCtxs[] = {{4, 6}, {4, 5}, {4, 4}, {4, 3}, {4, 2}, {4, 1}, {4, 0}, {3, 3}, {3, 2}, {3, 1}, {3, 0}};

	EGLContext CreateContextForProfile(EGLConfig config, EGLint profileMask, const int2& minCtx)
	{
		const char* profName = (profileMask == EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT) ? "core" : "compatibility";

		for (const int2 ctxVer: glCtxs) {
			if (ctxVer.x < minCtx.x || (ctxVer.x == minCtx.x && ctxVer.y < minCtx.y))
				break;

			const EGLint ctxAttribs[] = {
				EGL_CONTEXT_MAJOR_VERSION, ctxVer.x,
				EGL_CONTEXT_MINOR_VERSION, ctxVer.y,
				EGL_CONTEXT_OPENGL_PROFILE_MASK, profileMask,
				EGL_NONE
			};

			const EGLContext ctx = eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT, ctxAttribs);

			if (ctx == EGL_NO_CONTEXT) {
				LOG_L(L_DEBUG, "[MacEGL::%s] error (0x%x) creating GL%d.%d %s-context", __func__, eglGetError(), ctxVer.x, ctxVer.y, profName);
				continue;
			}

			LOG("[MacEGL::%s] created GL%d.%d %s-context", __func__, ctxVer.x, ctxVer.y, profName);
			return ctx;
		}

		return EGL_NO_CONTEXT;
	}

	bool InitContext(const int2& minCtx, const int2& size)
	{
		if ((eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
			LOG_L(L_ERROR, "[MacEGL::%s] no EGL display, is Mesa installed?", __func__);
			return false;
		}

		EGLint eglMajor = 0;
		EGLint eglMinor = 0;

		if (!eglInitialize(eglDisplay, &eglMajor, &eglMinor)) {
			LOG_L(L_ERROR, "[MacEGL::%s] error (0x%x) initializing EGL", __func__, eglGetError());
			return false;
		}

		LOG("[MacEGL::%s] EGL %d.%d (vendor \"%s\")", __func__, eglMajor, eglMinor, eglQueryString(eglDisplay, EGL_VENDOR));

		if (!eglBindAPI(EGL_OPENGL_API)) {
			LOG_L(L_ERROR, "[MacEGL::%s] error (0x%x) binding the OpenGL API", __func__, eglGetError());
			return false;
		}

		// Mesa's macOS EGL exposes no window platform, so a pbuffer is the only
		// kind of surface available to act as the default framebuffer
		constexpr EGLint configAttribs[] = {
			EGL_RED_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_BLUE_SIZE, 8,
			EGL_ALPHA_SIZE, 8,
			EGL_DEPTH_SIZE, 24,
			EGL_STENCIL_SIZE, 8,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
			EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
			EGL_NONE
		};

		EGLint numConfigs = 0;

		if (!eglChooseConfig(eglDisplay, configAttribs, &eglConfig, 1, &numConfigs) || numConfigs == 0) {
			LOG_L(L_ERROR, "[MacEGL::%s] error (0x%x) choosing an EGL config", __func__, eglGetError());
			return false;
		}

		if ((eglSurface = CreatePbuffer(size)) == EGL_NO_SURFACE)
			return false;

		// compatibility is the profile the engine asks SDL for elsewhere, and
		// core is a fallback so that a missing legacy path fails where it is
		// used rather than leaving the engine with no context at all
		if ((eglContext = CreateContextForProfile(eglConfig, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT, minCtx)) == EGL_NO_CONTEXT)
			eglContext = CreateContextForProfile(eglConfig, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT, minCtx);

		if (eglContext == EGL_NO_CONTEXT) {
			LOG_L(L_ERROR, "[MacEGL::%s] no context at GL%d.%d or later, set MESA_GL_VERSION_OVERRIDE if Mesa reports a lower version", __func__, minCtx.x, minCtx.y);
			return false;
		}

		if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
			LOG_L(L_ERROR, "[MacEGL::%s] error (0x%x) making the context current", __func__, eglGetError());
			return false;
		}

		return true;
	}
}

bool MacEGL::CreateContext(const int2& minCtx, const int2& size)
{
	if (InitContext(minCtx, size))
		return true;

	DestroyContext(true);
	return false;
}

void MacEGL::DestroyContext(bool terminateDisplay)
{
	if (eglDisplay == EGL_NO_DISPLAY)
		return;

	eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	if (eglContext != EGL_NO_CONTEXT)
		eglDestroyContext(eglDisplay, eglContext);
	if (eglSurface != EGL_NO_SURFACE)
		eglDestroySurface(eglDisplay, eglSurface);

	// zink_destroy_screen waits on a barrier that every thread in its work queue
	// must reach, and a thread created while that wait is already running never
	// gets a barrier job, so the wait never ends. Observed on Mesa 26.2.0-rc3.
	if (terminateDisplay)
		eglTerminate(eglDisplay);

	eglDisplay = EGL_NO_DISPLAY;
	eglSurface = EGL_NO_SURFACE;
	eglContext = EGL_NO_CONTEXT;
}

bool MacEGL::ResizeSurface(const int2& size)
{
	if (eglContext == EGL_NO_CONTEXT)
		return false;

	const EGLSurface surface = CreatePbuffer(size);

	if (surface == EGL_NO_SURFACE)
		return false;

	// a pbuffer cannot be resized, so the context moves to a new one and the
	// old one goes only once nothing is drawing into it
	const EGLSurface oldSurface = eglSurface;

	if (!eglMakeCurrent(eglDisplay, surface, surface, eglContext)) {
		LOG_L(L_ERROR, "[MacEGL::%s] error (0x%x) making the %dx%d surface current", __func__, eglGetError(), size.x, size.y);
		eglDestroySurface(eglDisplay, surface);
		return false;
	}

	eglSurface = surface;
	eglDestroySurface(eglDisplay, oldSurface);

	LOG("[MacEGL::%s] framebuffer is now %dx%d", __func__, size.x, size.y);
	return true;
}

void MacEGL::MakeCurrent(bool clear)
{
	if (eglDisplay == EGL_NO_DISPLAY)
		return;

	if (clear)
		eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	else
		eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
}

int2 MacEGL::GetSurfaceSize()
{
	if (eglSurface == EGL_NO_SURFACE)
		return {0, 0};

	EGLint w = 0;
	EGLint h = 0;

	eglQuerySurface(eglDisplay, eglSurface, EGL_WIDTH, &w);
	eglQuerySurface(eglDisplay, eglSurface, EGL_HEIGHT, &h);

	return {w, h};
}

bool MacEGL::HasContext()
{
	return (eglContext != EGL_NO_CONTEXT);
}

void* MacEGL::GetProcAddress(const char* name)
{
	return reinterpret_cast<void*>(eglGetProcAddress(name));
}
