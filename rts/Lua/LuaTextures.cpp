/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaTextures.h"
#include "Rendering/Textures/TextureFormat.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/FBO.h"
#include "Rendering/GL/TexBind.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "Lua/LuaFBOs.h"

#include "fmt/format.h"


namespace Impl {
	static inline bool IsValidLuaTextureTarget(GLenum target) {
		switch(target) {
			case GL_TEXTURE_1D:
			case GL_TEXTURE_2D:
			case GL_TEXTURE_3D:
			//case GL_TEXTURE_1D_ARRAY:
			case GL_TEXTURE_2D_ARRAY:
			//case GL_TEXTURE_RECTANGLE:
			case GL_TEXTURE_CUBE_MAP:
			//case GL_TEXTURE_BUFFER:
			case GL_TEXTURE_2D_MULTISAMPLE:
			//case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
				return true;
			default: break;
		}
		return false;
	}
}

/******************************************************************************/
/******************************************************************************/

std::string LuaTextures::Create(const Texture& tex)
{
	GLenum query = 0;
	if (Impl::IsValidLuaTextureTarget(tex.target)) {
		query = GL::GetBindingQueryFromTarget(tex.target);
	}
	if (!query) {
		LOG_L(L_ERROR, "[LuaTextures::%s] texture-target %d is not supported", __func__, tex.target);
		return "";
	}

	GLint currentBinding;
	glGetIntegerv(query, &currentBinding);

	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(tex.target, texID);

	glClearErrors("LuaTex", __func__, globalRendering->glDebugErrors);

	GLenum dataFormat = GL::GetDataFormatFromInternalFormat(tex.format);
	GLenum dataType   = GL::GetDataTypeFromInternalFormat(tex.format);

	switch (tex.target) {
		case GL_TEXTURE_1D: {
			glTexImage1D(tex.target, 0, tex.format, tex.xsize                      , tex.border, dataFormat, dataType, nullptr);
		} break;
		case GL_TEXTURE_2D: {
			glTexImage2D(tex.target, 0, tex.format, tex.xsize, tex.ysize           , tex.border, dataFormat, dataType, nullptr);
		} break;
		case GL_TEXTURE_2D_ARRAY: [[fallthrough]];
		case GL_TEXTURE_3D: {
			glTexImage3D(tex.target, 0, tex.format, tex.xsize, tex.ysize, tex.zsize, tex.border, dataFormat, dataType, nullptr);
		} break;
		case GL_TEXTURE_CUBE_MAP: {
			// IsValidLuaTextureTarget accepts GL_TEXTURE_CUBE_MAP, but upstream had no
			// case here: the texture reached the assert below (debug) or came back
			// with no storage at all (release), so nothing could render into it or
			// sample it. A cube texture's level 0 is six separate 2D images, one per
			// face target, allocated in the OpenGL face order +X, -X, +Y, -Y, +Z, -Z
			// (the face enums are consecutive). Only level 0 is allocated here;
			// gl.GenerateMipmap allocates the rest of the mip chain, as the PBR
			// provider does for its mipmapped cubes. GL requires square faces:
			// xsize != ysize fails with GL_INVALID_VALUE, which the glGetError check
			// below reports, and gl.CreateTexture then returns nil.
			for (GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X; face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; ++face)
				glTexImage2D(face, 0, tex.format, tex.xsize, tex.ysize, tex.border, dataFormat, dataType, nullptr);
		} break;

		case GL_TEXTURE_2D_MULTISAMPLE: {
			assert(tex.samples > 1);

			// 2DMS target only makes sense for FBO's
			if (!globalRendering->supportMSAAFrameBuffer) {
				glDeleteTextures(1, &texID);
				glBindTexture(tex.target, currentBinding);
				return "";
			}

			glTexImage2DMultisample(tex.target, tex.samples, tex.format, tex.xsize, tex.ysize, GL_TRUE);
		} break;

		default: {
			assert(false);
		} break;
	}

	if (const GLenum texErr = glGetError(); texErr != GL_NO_ERROR) {
		LOG_L(L_ERROR, "[LuaTextures::%s] glTexImage failed: target=0x%x size=%dx%d fmt=0x%x dataFmt=0x%x dataType=0x%x border=%d glError=0x%x",
			__func__, tex.target, tex.xsize, tex.ysize, tex.format, dataFormat, dataType, tex.border, texErr);
		glDeleteTextures(1, &texID);
		glBindTexture(tex.target, currentBinding);
		return "";
	}

	ApplyParams(tex);

	glBindTexture(tex.target, currentBinding); // revert the current binding

	GLuint fbo = 0;
	GLuint fboDepth = 0;

	if (tex.fbo != 0) {
		if (!FBO::IsSupported()) {
			glDeleteTextures(1, &texID);
			return "";
		}

		GLint currentFBO;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFBO);

		glGenFramebuffersEXT(1, &fbo);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);

		if (tex.fboDepth != 0) {
			glGenRenderbuffersEXT(1, &fboDepth);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, fboDepth);
			GLenum depthFormat = static_cast<GLenum>(CGlobalRendering::DepthBitsToFormat(globalRendering->supportDepthBufferBitDepth));
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, depthFormat, tex.xsize, tex.ysize);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, fboDepth);
		}

		bool attachFailure = false;
		try {
			LuaFBOs::AttachObjectTexTarget(__func__, GL_FRAMEBUFFER_EXT, tex.target, texID, GL_COLOR_ATTACHMENT0_EXT, 0);
		}
		catch (const opengl_error& e) {
			attachFailure = true;
			LOG_L(L_ERROR, "[LuaTextures::%s] %s", __func__, e.what());
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT || attachFailure) {
			glDeleteTextures(1, &texID);
			glDeleteFramebuffersEXT(1, &fbo);
			glDeleteRenderbuffersEXT(1, &fboDepth);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, currentFBO);
			return "";
		}

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, currentFBO);
	}

	std::string str = fmt::format("{}{}", prefix, ++lastCode);

	Texture newTex = tex;
	newTex.id = texID;
	newTex.fbo = fbo;
	newTex.fboDepth = fboDepth;

	if (freeIndices.empty()) {
		textureMap.emplace(str, textureVec.size());
		textureVec.emplace_back(newTex);
		return str;
	}

	// recycle
	textureMap[str] = freeIndices.back();
	textureVec[freeIndices.back()] = newTex;
	freeIndices.pop_back();
	return str;
}


bool LuaTextures::Bind(const std::string& name) const
{
	const auto it = textureMap.find(name);

	if (it != textureMap.end()) {
		const Texture& tex = textureVec[it->second];
		glBindTexture(tex.target, tex.id);
		return true;
	}

	return false;
}


bool LuaTextures::Free(const std::string& name)
{
	const auto it = textureMap.find(name);

	if (it != textureMap.end()) {
		const Texture& tex = textureVec[it->second];
		// gl.DeleteTexture of a published texture unpublishes it first: GL may
		// hand the deleted name to the next new texture, which a stale slot
		// would then expose under the old "$pub:" name.
		Unpublish(tex.id);
		glDeleteTextures(1, &tex.id);

		if (FBO::IsSupported()) {
			glDeleteFramebuffersEXT(1, &tex.fbo);
			glDeleteRenderbuffersEXT(1, &tex.fboDepth);
		}

		freeIndices.push_back(it->second);
		textureMap.erase(it);
		return true;
	}

	return false;
}


bool LuaTextures::FreeFBO(const std::string& name)
{
	if (!FBO::IsSupported())
		return false;

	const auto it = textureMap.find(name);

	if (it == textureMap.end())
		return false;

	Texture& tex = textureVec[it->second];

	glDeleteFramebuffersEXT(1, &tex.fbo);
	glDeleteRenderbuffersEXT(1, &tex.fboDepth);

	tex.fbo = 0;
	tex.fboDepth = 0;
	return true;
}


void LuaTextures::FreeAll()
{
	for (const auto& item: textureMap) {
		const Texture& tex = textureVec[item.second];
		// the handle is shutting down (~LuaTextures, also on a /luarules or
		// /luaui reload, which destroys and recreates the handle): its published
		// names become unowned and sample as id 0 until a handle publishes again
		Unpublish(tex.id);
		glDeleteTextures(1, &tex.id);

		if (FBO::IsSupported()) {
			glDeleteFramebuffersEXT(1, &tex.fbo);
			glDeleteRenderbuffersEXT(1, &tex.fboDepth);
		}
	}

	textureMap.clear();
	textureVec.clear();
	freeIndices.clear();
}


// The published-texture registry (see LuaTextures::Published in the header).
//
// File statics rather than members: every Lua handle has its own LuaTextures,
// but a published name must be visible to all of them and to engine code that
// has no Lua handle, so the registry belongs to none of them. They live for
// the whole process; slots are only ever cleared, never erased.
//
// A vector plus a name-to-index map, the layout CNamedTextures uses: the map
// is consulted once, when a "$pub:<name>" string is parsed, and every later
// bind or gl.TextureInfo is a bounds-checked vector access by the kept index.
// The vector may reallocate when a new name appears, which is why consumers
// keep an index and never a Published pointer across calls.
static std::vector<LuaTextures::Published> publishedSlots;
static spring::unsynced_map<std::string, size_t> publishedIndices;

// See the header. Never fails; an unknown name gets a new empty slot.
size_t LuaTextures::GetPublishedIndex(const std::string& name)
{
	const auto it = publishedIndices.find(name);
	if (it != publishedIndices.end())
		return it->second;

	// A slot is created on the first lookup, by the publisher or by a consumer that
	// parses "$pub:<name>" before anything is published. The index stays valid for
	// the rest of the session, so consumers keep it instead of the name.
	const size_t index = publishedSlots.size();
	publishedSlots.emplace_back().name = name;
	publishedIndices.emplace(name, index);
	return index;
}

// See the header. The returned pointer is valid only until the next new name
// is looked up (the vector may grow), so callers use it immediately.
const LuaTextures::Published* LuaTextures::GetPublished(size_t index)
{
	return (index < publishedSlots.size()) ? &publishedSlots[index] : nullptr;
}

// See the header; gl.PublishTexture is the only caller.
bool LuaTextures::Publish(const std::string& name, const std::string& texName, std::string& error) const
{
	// Ownership rule: a name belongs to the handle that published it until that
	// handle unpublishes it or frees the texture. Without it a LuaUI widget
	// could replace the LuaRules provider's "pbr_environment" and every
	// consumer would silently sample the widget's texture.
	Published& slot = publishedSlots[GetPublishedIndex(name)];
	if (slot.owner != nullptr && slot.owner != this) {
		error = "\"" + name + "\" is published by another Lua handle";
		return false;
	}

	// gl.PublishTexture(name, nil): clear the slot but keep its name, so the
	// index held by consumers stays valid and now resolves to id 0
	if (texName.empty()) {
		slot = {name};
		return true;
	}

	// only this handle's own textures can be published, so the owner is always
	// the LuaTextures that will unpublish the slot when the texture is freed
	const Texture* tex = GetInfo(texName);
	if (tex == nullptr) {
		error = "\"" + texName + "\" is not a texture of this Lua handle";
		return false;
	}

	// A copy of the texture's GL name, target and size, not a reference to the
	// Texture: consumers in other handles cannot reach this handle's textureVec,
	// and Free/FreeAll clear the copy before the GL name dies.
	slot = {name, this, tex->id, tex->target, tex->xsize, tex->ysize};
	return true;
}

// See the header. A linear scan: there are only a handful of published names
// (the PBR provider uses four), and it runs only when a texture is deleted.
// Several names may point at the same texture; all of them are cleared.
void LuaTextures::Unpublish(GLuint id)
{
	for (Published& slot: publishedSlots) {
		if (slot.id == id)
			slot = {slot.name};
	}
}


void LuaTextures::ApplyParams(const Texture& tex) const
{
	glTexParameteri(tex.target, GL_TEXTURE_WRAP_S, tex.wrap_s);
	glTexParameteri(tex.target, GL_TEXTURE_WRAP_T, tex.wrap_t);
	glTexParameteri(tex.target, GL_TEXTURE_WRAP_R, tex.wrap_r);
	glTexParameteri(tex.target, GL_TEXTURE_MIN_FILTER, tex.min_filter);
	glTexParameteri(tex.target, GL_TEXTURE_MAG_FILTER, tex.mag_filter);
	if (tex.cmpFunc != GL_NONE) {
		glTexParameteri(tex.target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glTexParameteri(tex.target, GL_TEXTURE_COMPARE_FUNC, tex.cmpFunc);
	}
	else {
		glTexParameteri(tex.target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		glTexParameteri(tex.target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL); //sensible default
	}

	if (tex.lodBias != 0.0f)
		glTexParameterf(tex.target, GL_TEXTURE_LOD_BIAS, tex.lodBias);

	if (tex.aniso != 0.0f && GLAD_GL_EXT_texture_filter_anisotropic)
		glTexParameterf(tex.target, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::clamp(tex.aniso, 1.0f, globalRendering->maxTexAnisoLvl));
}

void LuaTextures::ChangeParams(const Texture& tex)  const
{
	auto texBind = GL::TexBind(tex.target, tex.id);
	ApplyParams(tex);
}


size_t LuaTextures::GetIdx(const std::string& name) const
{
	const auto it = textureMap.find(name);

	if (it != textureMap.end())
		return (it->second);

	return (size_t(-1));
}

