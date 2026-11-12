/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_TEXTURES_H
#define LUA_TEXTURES_H

#include <string>
#include <vector>

#include "Rendering/GL/myGL.h"


class LuaTextures {
public:
	static constexpr char prefix = '!';

	~LuaTextures() { FreeAll(); }
	LuaTextures() {
		textureVec.reserve(128);
		textureMap.reserve(128);
		lastCode = 0;
	}

	void Clear() {
		textureVec.clear();
		textureMap.clear();
		freeIndices.clear();
	}

	struct Texture {
		GLuint id = 0;

		// FIXME: obsolete, use raw FBO's
		GLuint fbo = 0;
		GLuint fboDepth = 0;

		GLenum target = GL_TEXTURE_2D;
		GLenum format = GL_RGBA8;

		GLsizei xsize   = 0;
		GLsizei ysize   = 0;
		GLsizei zsize   = 0;
		GLsizei samples = 0;

		GLint border = 0;

		GLenum min_filter = GL_LINEAR;
		GLenum mag_filter = GL_LINEAR;

		GLenum wrap_s = GL_REPEAT;
		GLenum wrap_t = GL_REPEAT;
		GLenum wrap_r = GL_REPEAT;

		GLenum cmpFunc = GL_NONE;

		GLfloat lodBias = 0.0f;

		GLfloat aniso = 0.0f;
	};

	std::string Create(const Texture& tex);
	bool Bind(const std::string& name) const;
	bool Free(const std::string& name);
	bool FreeFBO(const std::string& name);
	void FreeAll();
	void ChangeParams(const Texture& tex) const;
	void ApplyParams(const Texture& tex) const;

	size_t GetIdx(const std::string& name) const;

	const Texture* GetInfo(size_t texIdx) const { return ((texIdx < textureVec.size())? &textureVec[texIdx]: nullptr); }
	      Texture* GetInfo(size_t texIdx)       { return ((texIdx < textureVec.size())? &textureVec[texIdx]: nullptr); }

	const Texture* GetInfo(const std::string& name) const { return (GetInfo(GetIdx(name))); }
	      Texture* GetInfo(const std::string& name)       { return (GetInfo(GetIdx(name))); }

	// Published-texture registry, behind gl.PublishTexture and "$pub:<name>".
	//
	// A Lua texture ("!N") resolves only inside the Lua handle that created it,
	// because ParseTextureImage looks it up in that handle's own LuaTextures.
	// Publishing gives one of them a global name, so other Lua handles (LuaUI
	// widgets sampling a LuaRules texture) and engine code (shaders binding
	// "$pub:<name>" through IProgramObject::AddTextureBinding, which has no Lua
	// state) can sample it. The registry is process-wide: two file-static
	// containers in LuaTextures.cpp, shared by every LuaTextures instance.
	//
	// One slot per name. The slot is created on the first lookup and never
	// removed, so its index stays valid for the rest of the process and a parsed
	// LuaMatTexture keeps the index instead of the name (the same idea as
	// CNamedTextures::GetInfoIndex for LUATEX_NAMED). While nothing is published
	// under the name, or after the texture was unpublished or deleted, id is 0
	// and the name samples as "no texture".
	struct Published {
		// the name after "$pub:"; kept when the slot is cleared
		std::string name;
		// the publishing handle's LuaTextures; nullptr while unpublished
		const LuaTextures* owner = nullptr;
		// GL texture name, 0 while unpublished
		GLuint id = 0;
		// target and size copied from the Lua texture, so binders and
		// gl.TextureInfo need not reach the owner's textures
		GLenum target = GL_TEXTURE_2D;
		GLsizei xsize = 0;
		GLsizei ysize = 0;
	};

	// Called by gl.PublishTexture with this handle's textures. Points the slot
	// "name" at this handle's texture texName ("!N"), or clears it when texName
	// is empty (gl.PublishTexture(name, nil)). Only the handle that currently
	// owns a published name may change or clear it; a name with no owner can be
	// taken by any handle. Returns false and sets error (the slot unchanged) when
	// another handle owns the name or texName is not a texture of this handle.
	// The method is const because it changes only the static registry.
	bool Publish(const std::string& name, const std::string& texName, std::string& error) const;

	// Index of the slot for name, creating an empty slot on the first lookup.
	// Called by Publish and by LuaOpenGLUtils::ParseTextureImage for "$pub:"
	// names (which may run before the publisher has published anything).
	static size_t GetPublishedIndex(const std::string& name);

	// The slot at index, or nullptr for an index that was never handed out.
	// Called by LuaMatTexture::GetTextureID/GetTextureTarget/GetSize each time a
	// "$pub:" texture is bound or queried, so a republished name takes effect on
	// the next bind without re-parsing.
	static const Published* GetPublished(size_t index);
private:
	// Clears every slot that points at GL texture id. Called by Free and FreeAll
	// (gl.DeleteTexture, and the handle shutting down via ~LuaTextures) before
	// the texture is deleted, so no slot keeps a deleted, possibly reused GL name.
	static void Unpublish(GLuint id);

	int lastCode;

	// maps names to textureVec indices
	spring::unsynced_map<std::string, size_t> textureMap;

	std::vector<Texture> textureVec;
	std::vector<size_t> freeIndices;
};


#endif /* LUA_TEXTURES_H */
