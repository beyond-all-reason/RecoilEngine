/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Puts pixels on the screen on macOS
 *
 * Mesa's macOS EGL advertises no window platform, so the engine renders into a
 * pbuffer that nothing displays, see MacEGLContext.h. This attaches a
 * CAMetalLayer to the SDL window and draws a caller-supplied BGRA image into
 * that layer's drawable.
 *
 * The image lives in an IOSurface, which the caller writes to directly, so
 * there is no staging copy between the caller's pixels and the texture the GPU
 * samples. Only one such surface exists, so a present waits for the previous
 * frame to leave the GPU before handing the buffer back.
 *
 * The interface is C on purpose. This is the only Objective-C in the engine and
 * clang compiles it against libc++ while the rest of the engine is built by GCC
 * against libstdc++, so no C++ type may cross this boundary.
 */

#ifdef __cplusplus
extern "C" {
#endif

/// attaches the layer, @p nsWindow is the SDL window's NSWindow
bool MacMetalPresent_Init(void* nsWindow);

/// backing-pixel size of the layer's drawable, the size a frame should be
void MacMetalPresent_GetDrawableSize(int* outW, int* outH);

/**
 * Returns a writable BGRA8 image of @p w x @p h to fill, or null on failure.
 * Rows are @p outRowBytes apart, which is not necessarily @p w * 4. Every
 * successful call must be followed by MacMetalPresent_PresentIOSurface.
 */
void* MacMetalPresent_AcquireIOSurfaceBuffer(int w, int h, size_t* outRowBytes);

/// draws the acquired image to the window, @p flipY for bottom-up GL rows
void MacMetalPresent_PresentIOSurface(bool flipY);

/// acquires, copies and presents, for callers that already hold their pixels
void MacMetalPresent_PresentBGRA(int w, int h, const void* pixels, bool flipY);

#ifdef __cplusplus
}
#endif
