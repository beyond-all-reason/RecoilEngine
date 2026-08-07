/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Rendering/GL/GpuMemTracyHooks.h"

#if defined(TRACY_ENABLE) && !defined(HEADLESS)

#include <algorithm>
#include <mutex>
#include <unordered_map>

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/GpuMemTracy.h"
#include "Rendering/Textures/TextureFormat.h"

// NB: the wrapper bodies call the *saved original* pointers, plus glGetIntegerv
// (which we never hook), so there is no recursion. They run on whatever thread the
// allocation happens on (main render thread, or a loader worker), hence the mutex
// around the bookkeeping maps. Allocations are infrequent relative to draws.

namespace {
	std::mutex g_mtx;
	std::unordered_map<GLuint, size_t> g_texBytes; // live bytes per texture id
	std::unordered_map<GLuint, size_t> g_rboBytes; // live bytes per renderbuffer id

	// saved original glad pointers
	PFNGLTEXIMAGE1DPROC                        o_glTexImage1D = nullptr;
	PFNGLTEXIMAGE2DPROC                        o_glTexImage2D = nullptr;
	PFNGLTEXIMAGE3DPROC                        o_glTexImage3D = nullptr;
	PFNGLTEXSTORAGE1DPROC                      o_glTexStorage1D = nullptr;
	PFNGLTEXSTORAGE2DPROC                      o_glTexStorage2D = nullptr;
	PFNGLTEXSTORAGE3DPROC                      o_glTexStorage3D = nullptr;
	PFNGLTEXIMAGE2DMULTISAMPLEPROC             o_glTexImage2DMultisample = nullptr;
	PFNGLTEXSTORAGE2DMULTISAMPLEPROC           o_glTexStorage2DMultisample = nullptr;
	PFNGLCOMPRESSEDTEXIMAGE1DPROC              o_glCompressedTexImage1D = nullptr;
	PFNGLCOMPRESSEDTEXIMAGE2DPROC              o_glCompressedTexImage2D = nullptr;
	PFNGLCOMPRESSEDTEXIMAGE3DPROC              o_glCompressedTexImage3D = nullptr;
	PFNGLTEXTURESTORAGE2DPROC                  o_glTextureStorage2D = nullptr;
	PFNGLTEXTURESTORAGE3DPROC                  o_glTextureStorage3D = nullptr;
	PFNGLDELETETEXTURESPROC                    o_glDeleteTextures = nullptr;

	PFNGLRENDERBUFFERSTORAGEPROC               o_glRenderbufferStorage = nullptr;
	PFNGLRENDERBUFFERSTORAGEEXTPROC            o_glRenderbufferStorageEXT = nullptr;
	PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC    o_glRenderbufferStorageMultisample = nullptr;
	PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC o_glRenderbufferStorageMultisampleEXT = nullptr;
	PFNGLDELETERENDERBUFFERSPROC               o_glDeleteRenderbuffers = nullptr;
	PFNGLDELETERENDERBUFFERSEXTPROC            o_glDeleteRenderbuffersEXT = nullptr;

	size_t TexelBytes(GLenum intFormat) {
		const size_t numChannels = GL::GetNumChannelsFromInternalFormat(intFormat);
		const size_t dataSize    = GL::GetDataTypeSize(GL::GetDataTypeFromInternalFormat(intFormat));
		return numChannels * dataSize;
	}

	GLuint BoundTexture(GLenum target) {
		const GLenum query = GL::GetBindingQueryFromTarget(target);
		if (query == 0)
			return 0;
		GLint id = 0;
		glGetIntegerv(query, &id); // glad_glGetIntegerv -- not hooked
		return static_cast<GLuint>(id);
	}
	GLuint BoundRenderbuffer() {
		GLint id = 0;
		glGetIntegerv(GL_RENDERBUFFER_BINDING, &id);
		return static_cast<GLuint>(id);
	}

	// full immutable mip-chain byte count for a glTexStorage*-style allocation
	size_t StorageBytes(GLenum target, GLsizei levels, GLenum intFormat, GLsizei w, GLsizei h, GLsizei d) {
		const size_t texelBytes = TexelBytes(intFormat);
		const bool mipDepth = (target == GL_TEXTURE_3D); // 2D arrays keep layer count across mips

		size_t total = 0;
		for (GLsizei l = 0; l < std::max(levels, GLsizei(1)); ++l) {
			const size_t lw = std::max(w >> l, 1);
			const size_t lh = (h > 0) ? size_t(std::max(h >> l, 1)) : 1;
			const size_t ld = (d > 0) ? (mipDepth ? size_t(std::max(d >> l, 1)) : size_t(d)) : 1;
			total += lw * lh * ld * texelBytes;
		}
		if (target == GL_TEXTURE_CUBE_MAP)
			total *= 6;
		return total;
	}

	// (re)spec: replace whatever was tracked for this id with a single known size
	void TexSet(GLuint id, size_t bytes) {
		if (id == 0)
			return;
		std::lock_guard<std::mutex> lk(g_mtx);
		const auto it = g_texBytes.find(id);
		if (it != g_texBytes.end()) {
			GL::GpuMemFree(GL::GpuMemPoolTextures, id);
			g_texBytes.erase(it);
		}
		if (bytes > 0) {
			GL::GpuMemAlloc(GL::GpuMemPoolTextures, id, bytes);
			g_texBytes[id] = bytes;
		}
	}
	// legacy glTexImage mip loop: level 0 sets the base, level>0 accumulates
	void TexAddLevel(GLuint id, size_t bytes) {
		if (id == 0 || bytes == 0)
			return;
		std::lock_guard<std::mutex> lk(g_mtx);
		size_t& cur = g_texBytes[id];
		if (cur > 0)
			GL::GpuMemFree(GL::GpuMemPoolTextures, id);
		cur += bytes;
		GL::GpuMemAlloc(GL::GpuMemPoolTextures, id, cur);
	}
	void TexDelete(GLuint id) {
		std::lock_guard<std::mutex> lk(g_mtx);
		const auto it = g_texBytes.find(id);
		if (it != g_texBytes.end()) {
			GL::GpuMemFree(GL::GpuMemPoolTextures, id);
			g_texBytes.erase(it);
		}
	}

	void RboSet(GLuint id, size_t bytes) {
		if (id == 0)
			return;
		std::lock_guard<std::mutex> lk(g_mtx);
		const auto it = g_rboBytes.find(id);
		if (it != g_rboBytes.end()) {
			GL::GpuMemFree(GL::GpuMemPoolRenderbuffers, id);
			g_rboBytes.erase(it);
		}
		if (bytes > 0) {
			GL::GpuMemAlloc(GL::GpuMemPoolRenderbuffers, id, bytes);
			g_rboBytes[id] = bytes;
		}
	}
	void RboDelete(GLuint id) {
		std::lock_guard<std::mutex> lk(g_mtx);
		const auto it = g_rboBytes.find(id);
		if (it != g_rboBytes.end()) {
			GL::GpuMemFree(GL::GpuMemPoolRenderbuffers, id);
			g_rboBytes.erase(it);
		}
	}

	// ---- texture wrappers ---------------------------------------------------

	void APIENTRY w_glTexImage1D(GLenum target, GLint level, GLint intFormat, GLsizei w, GLint border, GLenum format, GLenum type, const void* px) {
		o_glTexImage1D(target, level, intFormat, w, border, format, type, px);
		const size_t bytes = size_t(std::max(w, 1)) * TexelBytes(GLenum(intFormat));
		(level == 0) ? TexSet(BoundTexture(target), bytes) : TexAddLevel(BoundTexture(target), bytes);
	}
	void APIENTRY w_glTexImage2D(GLenum target, GLint level, GLint intFormat, GLsizei w, GLsizei h, GLint border, GLenum format, GLenum type, const void* px) {
		o_glTexImage2D(target, level, intFormat, w, h, border, format, type, px);
		const size_t bytes = size_t(std::max(w, 1)) * std::max(h, 1) * TexelBytes(GLenum(intFormat));
		(level == 0) ? TexSet(BoundTexture(target), bytes) : TexAddLevel(BoundTexture(target), bytes);
	}
	void APIENTRY w_glTexImage3D(GLenum target, GLint level, GLint intFormat, GLsizei w, GLsizei h, GLsizei d, GLint border, GLenum format, GLenum type, const void* px) {
		o_glTexImage3D(target, level, intFormat, w, h, d, border, format, type, px);
		const size_t bytes = size_t(std::max(w, 1)) * std::max(h, 1) * std::max(d, 1) * TexelBytes(GLenum(intFormat));
		(level == 0) ? TexSet(BoundTexture(target), bytes) : TexAddLevel(BoundTexture(target), bytes);
	}

	void APIENTRY w_glTexStorage1D(GLenum target, GLsizei levels, GLenum intFormat, GLsizei w) {
		o_glTexStorage1D(target, levels, intFormat, w);
		TexSet(BoundTexture(target), StorageBytes(target, levels, intFormat, w, 0, 0));
	}
	void APIENTRY w_glTexStorage2D(GLenum target, GLsizei levels, GLenum intFormat, GLsizei w, GLsizei h) {
		o_glTexStorage2D(target, levels, intFormat, w, h);
		TexSet(BoundTexture(target), StorageBytes(target, levels, intFormat, w, h, 0));
	}
	void APIENTRY w_glTexStorage3D(GLenum target, GLsizei levels, GLenum intFormat, GLsizei w, GLsizei h, GLsizei d) {
		o_glTexStorage3D(target, levels, intFormat, w, h, d);
		TexSet(BoundTexture(target), StorageBytes(target, levels, intFormat, w, h, d));
	}

	void APIENTRY w_glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum intFormat, GLsizei w, GLsizei h, GLboolean fixed) {
		o_glTexImage2DMultisample(target, samples, intFormat, w, h, fixed);
		const size_t bytes = size_t(std::max(w, 1)) * std::max(h, 1) * std::max(samples, GLsizei(1)) * TexelBytes(intFormat);
		TexSet(BoundTexture(target), bytes);
	}
	void APIENTRY w_glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum intFormat, GLsizei w, GLsizei h, GLboolean fixed) {
		o_glTexStorage2DMultisample(target, samples, intFormat, w, h, fixed);
		const size_t bytes = size_t(std::max(w, 1)) * std::max(h, 1) * std::max(samples, GLsizei(1)) * TexelBytes(intFormat);
		TexSet(BoundTexture(target), bytes);
	}

	// compressed uploads carry the exact byte size of the level
	void APIENTRY w_glCompressedTexImage1D(GLenum target, GLint level, GLenum intFormat, GLsizei w, GLint border, GLsizei imageSize, const void* data) {
		o_glCompressedTexImage1D(target, level, intFormat, w, border, imageSize, data);
		(level == 0) ? TexSet(BoundTexture(target), imageSize) : TexAddLevel(BoundTexture(target), imageSize);
	}
	void APIENTRY w_glCompressedTexImage2D(GLenum target, GLint level, GLenum intFormat, GLsizei w, GLsizei h, GLint border, GLsizei imageSize, const void* data) {
		o_glCompressedTexImage2D(target, level, intFormat, w, h, border, imageSize, data);
		(level == 0) ? TexSet(BoundTexture(target), imageSize) : TexAddLevel(BoundTexture(target), imageSize);
	}
	void APIENTRY w_glCompressedTexImage3D(GLenum target, GLint level, GLenum intFormat, GLsizei w, GLsizei h, GLsizei d, GLint border, GLsizei imageSize, const void* data) {
		o_glCompressedTexImage3D(target, level, intFormat, w, h, d, border, imageSize, data);
		(level == 0) ? TexSet(BoundTexture(target), imageSize) : TexAddLevel(BoundTexture(target), imageSize);
	}

	// DSA: texture id is passed directly, no binding query needed; target is implied
	void APIENTRY w_glTextureStorage2D(GLuint texture, GLsizei levels, GLenum intFormat, GLsizei w, GLsizei h) {
		o_glTextureStorage2D(texture, levels, intFormat, w, h);
		TexSet(texture, StorageBytes(GL_TEXTURE_2D, levels, intFormat, w, h, 0));
	}
	void APIENTRY w_glTextureStorage3D(GLuint texture, GLsizei levels, GLenum intFormat, GLsizei w, GLsizei h, GLsizei d) {
		o_glTextureStorage3D(texture, levels, intFormat, w, h, d);
		TexSet(texture, StorageBytes(GL_TEXTURE_2D_ARRAY, levels, intFormat, w, h, d));
	}

	void APIENTRY w_glDeleteTextures(GLsizei n, const GLuint* textures) {
		if (textures != nullptr) {
			for (GLsizei i = 0; i < n; ++i)
				TexDelete(textures[i]);
		}
		o_glDeleteTextures(n, textures);
	}

	// ---- renderbuffer wrappers ---------------------------------------------

	void APIENTRY w_glRenderbufferStorage(GLenum target, GLenum intFormat, GLsizei w, GLsizei h) {
		o_glRenderbufferStorage(target, intFormat, w, h);
		RboSet(BoundRenderbuffer(), size_t(std::max(w, 1)) * std::max(h, 1) * TexelBytes(intFormat));
	}
	void APIENTRY w_glRenderbufferStorageEXT(GLenum target, GLenum intFormat, GLsizei w, GLsizei h) {
		o_glRenderbufferStorageEXT(target, intFormat, w, h);
		RboSet(BoundRenderbuffer(), size_t(std::max(w, 1)) * std::max(h, 1) * TexelBytes(intFormat));
	}
	void APIENTRY w_glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum intFormat, GLsizei w, GLsizei h) {
		o_glRenderbufferStorageMultisample(target, samples, intFormat, w, h);
		RboSet(BoundRenderbuffer(), size_t(std::max(w, 1)) * std::max(h, 1) * std::max(samples, GLsizei(1)) * TexelBytes(intFormat));
	}
	void APIENTRY w_glRenderbufferStorageMultisampleEXT(GLenum target, GLsizei samples, GLenum intFormat, GLsizei w, GLsizei h) {
		o_glRenderbufferStorageMultisampleEXT(target, samples, intFormat, w, h);
		RboSet(BoundRenderbuffer(), size_t(std::max(w, 1)) * std::max(h, 1) * std::max(samples, GLsizei(1)) * TexelBytes(intFormat));
	}

	void APIENTRY w_glDeleteRenderbuffers(GLsizei n, const GLuint* rbos) {
		if (rbos != nullptr) {
			for (GLsizei i = 0; i < n; ++i)
				RboDelete(rbos[i]);
		}
		o_glDeleteRenderbuffers(n, rbos);
	}
	void APIENTRY w_glDeleteRenderbuffersEXT(GLsizei n, const GLuint* rbos) {
		if (rbos != nullptr) {
			for (GLsizei i = 0; i < n; ++i)
				RboDelete(rbos[i]);
		}
		o_glDeleteRenderbuffersEXT(n, rbos);
	}
}

// save the original glad pointer and redirect it to our wrapper, but only if the
// driver actually exposed the function (glad leaves unsupported entries null)
#define HOOK(fn) do { if (glad_##fn != nullptr) { o_##fn = glad_##fn; glad_##fn = w_##fn; } } while (false)

void GL::InstallGpuMemHooks()
{
	HOOK(glTexImage1D);
	HOOK(glTexImage2D);
	HOOK(glTexImage3D);
	HOOK(glTexStorage1D);
	HOOK(glTexStorage2D);
	HOOK(glTexStorage3D);
	HOOK(glTexImage2DMultisample);
	HOOK(glTexStorage2DMultisample);
	HOOK(glCompressedTexImage1D);
	HOOK(glCompressedTexImage2D);
	HOOK(glCompressedTexImage3D);
	HOOK(glTextureStorage2D);
	HOOK(glTextureStorage3D);
	HOOK(glDeleteTextures);

	HOOK(glRenderbufferStorage);
	HOOK(glRenderbufferStorageEXT);
	HOOK(glRenderbufferStorageMultisample);
	HOOK(glRenderbufferStorageMultisampleEXT);
	HOOK(glDeleteRenderbuffers);
	HOOK(glDeleteRenderbuffersEXT);
}

#undef HOOK

#else // !TRACY_ENABLE || HEADLESS

void GL::InstallGpuMemHooks() {}

#endif
