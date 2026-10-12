/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

// Renders through Mesa EGL + Zink into a pbuffer and presents it via SDL_Renderer.
// Disable at runtime with RECOIL_MAC_NATIVE_GL=1.

#if defined(__APPLE__) && defined(MAC_EGL_ZINK) && !defined(HEADLESS)
	#define MAC_EGL_BRIDGE_AVAILABLE 1
#endif

#ifdef MAC_EGL_BRIDGE_AVAILABLE
struct SDL_Window;

namespace MacEGLBridge {
	// true if the bridge is compiled in and not disabled via env
	bool Wanted();
	// true after a context was successfully created
	bool Active();

	void* CreateContext(SDL_Window* window, int major, int minor, bool core, bool debug);
	void MakeCurrent(bool clear);
	void DestroyContext();
	void* GetProcAddress(const char* name);

	// reads back the pbuffer and presents it; resizes the pbuffer to track the window
	void SwapWindow(SDL_Window* window);
	void SetSwapInterval(int interval);
}
#endif
