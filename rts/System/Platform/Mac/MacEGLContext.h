/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include "System/type2.h"

/**
 * @brief The engine's OpenGL context on macOS
 *
 * Apple's OpenGL.framework only offers a compatibility profile up to 2.1, so
 * SDL cannot give the engine the 3.0 or later compatibility context it needs.
 * Mesa can, through EGL, so on macOS the context is created here instead.
 *
 * Mesa's macOS EGL advertises no window platform, so there is no window
 * surface and the default framebuffer is a pbuffer. Getting those pixels onto
 * the screen is not handled here.
 */
namespace MacEGL {
	/// creates a context at @p minCtx or later and makes it current
	bool CreateContext(const int2& minCtx, const int2& size);

	/**
	 * @param terminateDisplay whether to call eglTerminate
	 *
	 * Pass false when the process is on its way out. eglTerminate deadlocks in
	 * Zink's screen teardown, so the engine would never exit.
	 */
	void DestroyContext(bool terminateDisplay);

	/// size of the pbuffer acting as the default framebuffer, zero without one
	int2 GetSurfaceSize();

	/// swaps the pbuffer for one of @p size, keeping the context and its objects
	bool ResizeSurface(const int2& size);

	void MakeCurrent(bool clear);
	bool HasContext();

	/// signature-compatible with glad's GLADloadproc
	void* GetProcAddress(const char* name);
}
