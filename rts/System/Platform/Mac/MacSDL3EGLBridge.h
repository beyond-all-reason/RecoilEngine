/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MAC_SDL3_EGL_BRIDGE_H
#define MAC_SDL3_EGL_BRIDGE_H

#include <cstdint>

#include <SDL_rect.h>

struct SDL_Window;

namespace MacSDL3EGL {

struct DisplayGeometry {
	int displayIndex = 0;
	const char* displayName = "unknown";

	SDL_Rect displayBounds = {0, 0, 0, 0};
	SDL_Rect usableBounds = {0, 0, 0, 0};
	SDL_Rect safeBounds = {0, 0, 0, 0};

	int logicalWindowWidth = 0;
	int logicalWindowHeight = 0;
	int drawableWidth = 0;
	int drawableHeight = 0;
	float sdlScale = 1.0f;
	float cocoaScale = 1.0f;

	bool m4Pro14NotchPolicy = false;
	bool m4Pro14NotchPolicyForced = false;
};

DisplayGeometry QueryDisplayGeometry(SDL_Window* window);
SDL_Rect GetSafeDisplayBounds(SDL_Window* window);
void LogDisplayGeometry(SDL_Window* window, const char* phase);
void* GetGLProcAddress(const char* name);

class Bridge {
public:
	Bridge();
	~Bridge();

	Bridge(const Bridge&) = delete;
	Bridge& operator=(const Bridge&) = delete;

	bool Initialize(SDL_Window* window, int requestMajor, int requestMinor, bool requestCoreProfile);
	void Destroy();

	bool MakeCurrent(bool clear) const;
	bool SwapBuffers();
	void UpdateDrawableSize(const char* reason);

	void* GetContextOpaque() const;
	bool IsInitialized() const;
	int GetSwapInterval() const;
	uint64_t GetRecommendedMaxWorkingSetSizeBytes() const;

	void LogRuntimeEnvironment() const;
	void LogContextDiagnostics(const char* phase) const;
	void LogGeometry(const char* phase) const;

private:
	struct Impl;
	Impl* impl;
};

} // namespace MacSDL3EGL

#endif // MAC_SDL3_EGL_BRIDGE_H
