#include "CommonGL.h"

#include <cmath>
#include <algorithm>

void GL::BindTexture(GLuint texUnitIdx, GLenum target, GLuint texture)
{
#ifndef HEADLESS
	if (GLEW_ARB_direct_state_access) {
		const GLuint relTexUnitID = texUnitIdx < GL_TEXTURE0 ? texUnitIdx : texUnitIdx - GL_TEXTURE0;
		glBindTextureUnit(texUnitIdx, texture);
	}
	else {
		const GLuint absTexUnitID = texUnitIdx < GL_TEXTURE0 ? texUnitIdx + GL_TEXTURE0 : texUnitIdx;
		glActiveTexture(absTexUnitID); glBindTexture(target, texture);
	}
#endif
}

void GL::BindTextures(GLuint texUnitIdxStart, GLsizei count, const GLenum target, const GLuint* textures)
{
	const auto getTargetFunc = [target](const GLuint i) { return target; };
	BindTexturesCommon(texUnitIdxStart, count, getTargetFunc, textures);
}

void GL::BindTextures(GLuint texUnitIdxStart, GLsizei count, const GLenum* targets, const GLuint* textures)
{
	const auto getTargetFunc = [targets](const GLuint i) { return targets[i]; };
	BindTexturesCommon(texUnitIdxStart, count, getTargetFunc, textures);
}

void GL::BindTextures(GLuint texUnitIdxStart, GLsizei count, const GLuint* textures)
{
	BindTextures(texUnitIdxStart, GL_TEXTURE_2D, textures);
}

void GL::UnbindTexture()
{
	BindTexture(0u, 0u);
}

void GL::UnbindTexture(GLuint texUnitIdx)
{
	BindTexture(texUnitIdx, 0u);
}

void GL::UnbindTexture(GLuint texUnitIdx, GLenum target)
{
	BindTexture(texUnitIdx, target, 0u);
}

void GL::UnbindTextures(GLuint texUnitIdxStart, GLsizei count, GLenum target)
{
	const auto getTargetFunc = [target](const GLuint i) { return target; };
	BindTexturesCommon(texUnitIdxStart, count, getTargetFunc, nullptr);
}

void GL::BindTexture(GLuint texture)
{
	BindTexture(0u, texture);
}

void GL::BindTexture(GLuint texUnitIdx, GLuint texture)
{
	BindTexture(texUnitIdx, GL_TEXTURE_2D, texture);
}

void GL::TexStorage2D(GLenum target, GLint levels, GLint internalFormat, GLsizei width, GLsizei height)
{
#ifndef HEADLESS
	if (GLEW_ARB_texture_storage) {
		if (levels < 0)
			levels = std::ceil(std::log((float)(std::max(width, height) + 1)));

		glTexStorage2D(target, levels, internalFormat, width, height);
	}
	else
	{
		GLenum format = GL_RGBA, type = GL_UNSIGNED_BYTE;
		switch (internalFormat) {
		case GL_RGBA8: format = GL_RGBA; type = GL_UNSIGNED_BYTE; break;
		case GL_RGB8:  format = GL_RGB;  type = GL_UNSIGNED_BYTE; break;
		default: /*LOG_L(L_ERROR, "[%s] Couldn't detect format type for %i", __FUNCTION__, internalFormat);*/
			break;
		}
		glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, nullptr);
	}
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename TargFunc>
inline void GL::BindTexturesCommon(GLuint texUnitIdxStart, GLsizei count, const TargFunc& targFunc, const GLuint* textures)
{
#ifndef HEADLESS
	const GLuint relTexUnitID = texUnitIdxStart < GL_TEXTURE0 ? texUnitIdxStart : texUnitIdxStart - GL_TEXTURE0;
	if (GLEW_ARB_multi_bind) {
		glBindTextures(relTexUnitID, count, textures);
	}
	else {
		GLint activeTex = -1;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTex);
		const GLuint absTexUnitID = texUnitIdxStart < GL_TEXTURE0 ? texUnitIdxStart + GL_TEXTURE0 : texUnitIdxStart;
		for (GLuint i = 0u; i < count; ++i) {
			GLuint thisTexture = (textures != nullptr) ? textures[i] : 0u;
			glActiveTexture(absTexUnitID + i); glBindTexture(targFunc(i), thisTexture);
		}
		glActiveTexture(activeTex);
	}
#endif
}
