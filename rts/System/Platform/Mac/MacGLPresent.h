/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#pragma once

/**
 * @brief The macOS end of a frame
 *
 * Mesa's macOS EGL has no window surface, so there is nothing for
 * SDL_GL_SwapWindow to swap, see MacEGLContext.h. Instead the default
 * framebuffer is read back and handed to Metal to draw, see MetalPresent.h.
 */
namespace MacGLPresent {
	/// takes the default framebuffer to the screen, in place of a buffer swap
	void SwapBuffers();
}
