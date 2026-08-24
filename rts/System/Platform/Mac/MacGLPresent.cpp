/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "MacGLPresent.h"

#include "MacEGLContext.h"
#include "MetalPresent.h"

#include "Rendering/GL/myGL.h"

void MacGLPresent::SwapBuffers()
{
	const int2 size = MacEGL::GetSurfaceSize();

	if (size.x <= 0 || size.y <= 0)
		return;

	size_t rowBytes = 0;
	void* image = MacMetalPresent_AcquireIOSurfaceBuffer(size.x, size.y, &rowBytes);

	if (image == nullptr)
		return;

	// an IOSurface pads its rows, a 3024 pixel row is 12160 bytes and not 12096
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ROW_LENGTH, rowBytes / 4);

	glReadPixels(0, 0, size.x, size.y, GL_BGRA, GL_UNSIGNED_BYTE, image);

	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);

	// GL hands back the bottom row of the framebuffer first
	MacMetalPresent_PresentIOSurface(true);
}
