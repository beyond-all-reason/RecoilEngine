/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MAC_METAL_PRESENT_H
#define MAC_METAL_PRESENT_H

// macOS manual Metal present path.
//
// Mesa's EGL on macOS is built with the "surfaceless" platform, so
// eglCreateWindowSurface() fails and the engine renders into an off-screen
// pbuffer that eglSwapBuffers() can never present. These helpers take the
// pixels the engine rendered (read back via glReadPixels) and blit them onto
// the window's CAMetalLayer drawable via Metal, so something actually appears.

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// caMetalLayer: the CAMetalLayer* already attached to the SDL window's NSView
// (the same pointer GetNSViewFromSDLWindow returns). Returns true on success.
bool MacMetalPresent_Init(void* caMetalLayer);

// Present a CPU pixel buffer (BGRA8, w*h*4 bytes, top-down) to the layer.
// flipY: if true, the source is treated as OpenGL bottom-up and flipped.
// Used for early splash / solid colors before the main render path is up.
void MacMetalPresent_PresentBGRA(int w, int h, const void* pixels, bool flipY);

// IOSurface zero-copy path.
//
// Acquire returns a CPU-writable pointer backed by an IOSurface that is also
// bound as an MTLTexture. The caller should glReadPixels (or otherwise fill)
// the buffer with BGRA8 pixel data (`*outRowBytes` may exceed `w*4` due to
// alignment, so honor it via glPixelStorei(GL_PACK_ROW_LENGTH, ...)). Then
// call Present to issue the Y-flipped blit to the drawable. Returns NULL on
// failure (caller should fall back to MacMetalPresent_PresentBGRA).
void* MacMetalPresent_AcquireIOSurfaceBuffer(int w, int h, size_t* outRowBytes);
void  MacMetalPresent_PresentIOSurface(bool flipY);

#ifdef __cplusplus
}
#endif

#endif
