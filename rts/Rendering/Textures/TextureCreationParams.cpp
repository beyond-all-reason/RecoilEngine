#include "TextureCreationParams.hpp"

#include "Rendering/GL/myGL.h"

namespace GL {
	uint32_t TextureCreationParams::GetMinFilter(int32_t numLevels) const
	{
		if (minFilter.has_value())
			return minFilter.value();

		if (numLevels == 1) {
			return linearTextureFilter ? GL_LINEAR : GL_NEAREST;
		}
		else {
			if (linearMipMapFilter) {
				return linearTextureFilter ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST;
			}
			else {
				return linearTextureFilter ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST;
			}
		}
	}

	uint32_t TextureCreationParams::GetMagFilter() const
	{
		if (magFilter.has_value())
			return magFilter.value();

		return linearTextureFilter ? GL_LINEAR : GL_NEAREST;
	}

	uint32_t TextureCreationParams::GetWrapMode() const
	{
		if (wrapMirror)
			return repeatMirror ? GL_MIRRORED_REPEAT : GL_REPEAT;

		return clampBorder.has_value() ? GL_CLAMP_TO_BORDER : GL_CLAMP_TO_EDGE;
	}
}