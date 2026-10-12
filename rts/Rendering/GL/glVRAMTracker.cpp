/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "glVRAMTracker.h"

#if defined(TRACY_ENABLE) && !defined(HEADLESS)

#ifdef _WIN32
	#include "System/Platform/Win/win32.h"
	#include <dxgi1_4.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "myGL.h"
#include "Rendering/GlobalRenderingInfo.h"
#include "System/Misc/TracyDefs.h"

namespace {
	enum Kind : uint32_t {
		KIND_TEXTURE      = 1,
		KIND_BUFFER       = 2,
		KIND_RENDERBUFFER = 3,
		KIND_COUNT        = 4,
	};

	// Tracy memory pools, which Tracy also plots; the pointers are the pool identities
	constexpr const char* POOL_NAMES[KIND_COUNT] = { nullptr, "VRAM: textures", "VRAM: buffers", "VRAM: renderbuffers" };

	struct Object {
		uint64_t bytes = 0;
		bool announced = false; // sent to the connected Tracy server
	};
	struct Texture {
		std::vector<std::pair<uint32_t, uint64_t>> images; // (level << 4 | slot, bytes); slot = cube face, IMMUTABLE_SLOT or MIPMAPS_SLOT
		uint32_t w = 0;
		uint32_t h = 0;
		uint32_t d = 1;
		uint32_t faces = 1;
		double bytesPerTexel = 4.0;
		bool depthHalves = false; // 3D texture, the depth shrinks with the mip level too
		bool immutable = false;
	};

	constexpr uint32_t IMMUTABLE_SLOT = 14;
	constexpr uint32_t MIPMAPS_SLOT = 15;

	std::mutex mutex; // GL objects can be created from the loading thread
	std::unordered_map<uint64_t, Object> objects; // key: kind << 32 | GL name
	std::unordered_map<GLuint, Texture> textures;
	bool connected = false;
	bool initialized = false;

	std::chrono::steady_clock::time_point nextSample;

	void* FakePtr(uint64_t key) { return reinterpret_cast<void*>(static_cast<uintptr_t>(key)); }
	uint64_t ObjectKey(Kind kind, GLuint name) { return (uint64_t(kind) << 32) | name; }


	// lock held by the callers
	void SetObjectBytes(Kind kind, GLuint name, uint64_t bytes)
	{
		const uint64_t key = ObjectKey(kind, name);
		Object& obj = objects[key];

		if (obj.bytes == bytes && (obj.announced || !connected || bytes == 0))
			return;

		obj.bytes = bytes;

		if (!connected)
			return;

		if (obj.announced)
			TracyFreeN(FakePtr(key), POOL_NAMES[kind]);
		if ((obj.announced = (bytes > 0)))
			TracyAllocN(FakePtr(key), bytes, POOL_NAMES[kind]);
	}

	void DeleteObject(Kind kind, GLuint name)
	{
		const auto it = objects.find(ObjectKey(kind, name));

		if (it == objects.end())
			return;

		if (it->second.announced)
			TracyFreeN(FakePtr(it->first), POOL_NAMES[kind]);

		objects.erase(it);
	}

	void SetTextureImage(GLuint name, Texture& tex, uint32_t level, uint32_t slot, uint64_t bytes)
	{
		const uint32_t key = (level << 4) | slot;
		const auto it = std::find_if(tex.images.begin(), tex.images.end(), [key](const auto& img) { return (img.first == key); });

		if (it != tex.images.end()) {
			it->second = bytes;
		} else {
			tex.images.emplace_back(key, bytes);
		}

		uint64_t total = 0;
		for (const auto& img: tex.images) {
			total += img.second;
		}

		SetObjectBytes(KIND_TEXTURE, name, total);
	}


	double BytesPerTexel(GLenum format)
	{
		switch (format) {
			case GL_R8: case GL_R8_SNORM: case GL_R8I: case GL_R8UI: case GL_RED: case GL_ALPHA: case GL_ALPHA8:
			case GL_LUMINANCE: case GL_LUMINANCE8: case GL_INTENSITY: case GL_INTENSITY8: case GL_STENCIL_INDEX8: case 1:
				return 1.0;
			case GL_RG8: case GL_RG8_SNORM: case GL_RG8I: case GL_RG8UI: case GL_RG: case GL_LUMINANCE_ALPHA: case GL_LUMINANCE8_ALPHA8:
			case GL_R16: case GL_R16_SNORM: case GL_R16F: case GL_R16I: case GL_R16UI: case GL_DEPTH_COMPONENT16:
			case GL_RGB4: case GL_RGB5: case GL_RGB565: case GL_RGB5_A1: case GL_RGBA4: case 2:
				return 2.0;
			case GL_RGB16: case GL_RGB16_SNORM: case GL_RGB16F: case GL_RGB16I: case GL_RGB16UI:
			case GL_RGBA16: case GL_RGBA16_SNORM: case GL_RGBA16F: case GL_RGBA16I: case GL_RGBA16UI:
			case GL_RG32F: case GL_RG32I: case GL_RG32UI: case GL_DEPTH32F_STENCIL8:
				return 8.0;
			case GL_RGB32F: case GL_RGB32I: case GL_RGB32UI: case GL_RGBA32F: case GL_RGBA32I: case GL_RGBA32UI:
				return 16.0;
			case GL_COMPRESSED_RGB_S3TC_DXT1_EXT: case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
			case GL_COMPRESSED_RED_RGTC1: case GL_COMPRESSED_SIGNED_RED_RGTC1: case GL_COMPRESSED_RGB8_ETC2:
				return 0.5;
			case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT: case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
			case GL_COMPRESSED_RG_RGTC2: case GL_COMPRESSED_SIGNED_RG_RGTC2: case GL_COMPRESSED_RGBA8_ETC2_EAC:
			case GL_COMPRESSED_RGBA_BPTC_UNORM: case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
			case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT: case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
				return 1.0;
			default:
				// RGBA8 and the other 32-bit formats, RGB8 (stored padded), 24/32-bit depth, unsized RGB(A)
				return 4.0;
		}
	}

	bool IsCubeFace(GLenum target) { return (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z); }

	GLenum TextureBinding(GLenum target)
	{
		if (IsCubeFace(target))
			return GL_TEXTURE_BINDING_CUBE_MAP;

		switch (target) {
			case GL_TEXTURE_1D                  : return GL_TEXTURE_BINDING_1D;
			case GL_TEXTURE_2D                  : return GL_TEXTURE_BINDING_2D;
			case GL_TEXTURE_3D                  : return GL_TEXTURE_BINDING_3D;
			case GL_TEXTURE_RECTANGLE           : return GL_TEXTURE_BINDING_RECTANGLE;
			case GL_TEXTURE_1D_ARRAY            : return GL_TEXTURE_BINDING_1D_ARRAY;
			case GL_TEXTURE_2D_ARRAY            : return GL_TEXTURE_BINDING_2D_ARRAY;
			case GL_TEXTURE_CUBE_MAP            : return GL_TEXTURE_BINDING_CUBE_MAP;
			case GL_TEXTURE_CUBE_MAP_ARRAY      : return GL_TEXTURE_BINDING_CUBE_MAP_ARRAY;
			case GL_TEXTURE_2D_MULTISAMPLE      : return GL_TEXTURE_BINDING_2D_MULTISAMPLE;
			case GL_TEXTURE_2D_MULTISAMPLE_ARRAY: return GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY;
			default                             : return 0; // proxy targets allocate nothing
		}
	}

	GLenum BufferBinding(GLenum target)
	{
		switch (target) {
			case GL_ARRAY_BUFFER             : return GL_ARRAY_BUFFER_BINDING;
			case GL_ELEMENT_ARRAY_BUFFER     : return GL_ELEMENT_ARRAY_BUFFER_BINDING;
			case GL_PIXEL_PACK_BUFFER        : return GL_PIXEL_PACK_BUFFER_BINDING;
			case GL_PIXEL_UNPACK_BUFFER      : return GL_PIXEL_UNPACK_BUFFER_BINDING;
			case GL_UNIFORM_BUFFER           : return GL_UNIFORM_BUFFER_BINDING;
			case GL_SHADER_STORAGE_BUFFER    : return GL_SHADER_STORAGE_BUFFER_BINDING;
			case GL_COPY_READ_BUFFER         : return GL_COPY_READ_BUFFER_BINDING;
			case GL_COPY_WRITE_BUFFER        : return GL_COPY_WRITE_BUFFER_BINDING;
			case GL_DRAW_INDIRECT_BUFFER     : return GL_DRAW_INDIRECT_BUFFER_BINDING;
			case GL_DISPATCH_INDIRECT_BUFFER : return GL_DISPATCH_INDIRECT_BUFFER_BINDING;
			case GL_TEXTURE_BUFFER           : return GL_TEXTURE_BUFFER_BINDING;
			case GL_TRANSFORM_FEEDBACK_BUFFER: return GL_TRANSFORM_FEEDBACK_BUFFER_BINDING;
			case GL_ATOMIC_COUNTER_BUFFER    : return GL_ATOMIC_COUNTER_BUFFER_BINDING;
			case GL_QUERY_BUFFER             : return GL_QUERY_BUFFER_BINDING;
			default                          : return 0;
		}
	}

	GLuint BoundName(GLenum binding)
	{
		if (binding == 0)
			return 0;

		GLint name = 0;
		glGetIntegerv(binding, &name);
		return static_cast<GLuint>(name);
	}


	// one image of the bound texture: a level of a 1D/2D/3D/array texture or of one cube face
	void OnTexImage(GLenum target, GLint level, GLenum format, GLsizei w, GLsizei h, GLsizei d, GLsizei samples, uint64_t compressedBytes)
	{
		const GLuint name = BoundName(TextureBinding(target));

		if (name == 0 || level < 0)
			return;

		const double bytesPerTexel = BytesPerTexel(format);
		const uint64_t bytes = (compressedBytes > 0)?
			compressedBytes:
			static_cast<uint64_t>(double(w) * double(h) * double(d) * std::max(samples, 1) * bytesPerTexel + 0.5);

		std::lock_guard<std::mutex> lock(mutex);
		Texture& tex = textures[name];

		if (level == 0) {
			tex.w = w;
			tex.h = h;
			tex.d = d;
			tex.faces = IsCubeFace(target)? 6: 1;
			tex.bytesPerTexel = bytesPerTexel;
			tex.depthHalves = (target == GL_TEXTURE_3D);
		}

		SetTextureImage(name, tex, level, IsCubeFace(target)? (target - GL_TEXTURE_CUBE_MAP_POSITIVE_X): 0, bytes);
	}

	uint64_t MipChainBytes(uint32_t firstLevel, uint32_t numLevels, uint32_t w, uint32_t h, uint32_t d, uint32_t faces, bool hHalves, bool dHalves, double bytesPerTexel)
	{
		double texels = 0.0;

		for (uint32_t level = firstLevel; level < numLevels; ++level) {
			const double lw = std::max(w >> level, 1u);
			const double lh = hHalves? std::max(h >> level, 1u): h;
			const double ld = dHalves? std::max(d >> level, 1u): d;
			texels += lw * lh * ld;

			if (lw == 1.0 && lh == 1.0 && (!dHalves || ld == 1.0))
				break;
		}

		return static_cast<uint64_t>(texels * faces * bytesPerTexel + 0.5);
	}

	// immutable storage: all levels (and faces or layers) at once
	void OnTexStorage(GLenum target, GLsizei levels, GLenum format, GLsizei w, GLsizei h, GLsizei d)
	{
		const GLuint name = BoundName(TextureBinding(target));

		if (name == 0)
			return;

		const bool isCube = (target == GL_TEXTURE_CUBE_MAP);
		const bool hHalves = (target != GL_TEXTURE_1D_ARRAY);
		const bool dHalves = (target == GL_TEXTURE_3D);
		const double bytesPerTexel = BytesPerTexel(format);

		std::lock_guard<std::mutex> lock(mutex);
		Texture& tex = textures[name];
		tex.w = w;
		tex.h = h;
		tex.d = d;
		tex.faces = isCube? 6: 1;
		tex.bytesPerTexel = bytesPerTexel;
		tex.depthHalves = dHalves;
		tex.immutable = true;

		SetTextureImage(name, tex, 0, IMMUTABLE_SLOT, MipChainBytes(0, levels, w, h, d, tex.faces, hHalves, dHalves, bytesPerTexel));
	}

	void OnGenerateMipmap(GLenum target)
	{
		const GLuint name = BoundName(TextureBinding(target));

		if (name == 0)
			return;

		std::lock_guard<std::mutex> lock(mutex);
		const auto it = textures.find(name);

		// immutable textures have all their levels already
		if (it == textures.end() || it->second.immutable || it->second.w == 0)
			return;

		Texture& tex = it->second;
		SetTextureImage(name, tex, 0, MIPMAPS_SLOT, MipChainBytes(1, 32, tex.w, tex.h, tex.d, tex.faces, true, tex.depthHalves, tex.bytesPerTexel));
	}

	void OnBufferData(GLenum target, GLsizeiptr size)
	{
		const GLuint name = BoundName(BufferBinding(target));

		if (name == 0)
			return;

		std::lock_guard<std::mutex> lock(mutex);
		SetObjectBytes(KIND_BUFFER, name, static_cast<uint64_t>(std::max<GLsizeiptr>(size, 0)));
	}

	void OnRenderbufferStorage(GLsizei samples, GLenum format, GLsizei w, GLsizei h)
	{
		const GLuint name = BoundName(GL_RENDERBUFFER_BINDING);

		if (name == 0)
			return;

		std::lock_guard<std::mutex> lock(mutex);
		SetObjectBytes(KIND_RENDERBUFFER, name, static_cast<uint64_t>(double(w) * h * std::max(samples, 1) * BytesPerTexel(format) + 0.5));
	}

	void OnDelete(Kind kind, GLsizei n, const GLuint* names)
	{
		if (names == nullptr)
			return;

		std::lock_guard<std::mutex> lock(mutex);
		for (GLsizei i = 0; i < n; ++i) {
			if (names[i] == 0)
				continue;

			DeleteObject(kind, names[i]);

			if (kind == KIND_TEXTURE)
				textures.erase(names[i]);
		}
	}


	// wrappers of the GL functions that allocate or free texture, buffer and renderbuffer memory
	PFNGLTEXIMAGE1DPROC origTexImage1D = nullptr;
	PFNGLTEXIMAGE2DPROC origTexImage2D = nullptr;
	PFNGLTEXIMAGE3DPROC origTexImage3D = nullptr;
	PFNGLTEXIMAGE2DMULTISAMPLEPROC origTexImage2DMultisample = nullptr;
	PFNGLCOMPRESSEDTEXIMAGE2DPROC origCompressedTexImage2D = nullptr;
	PFNGLTEXSTORAGE2DPROC origTexStorage2D = nullptr;
	PFNGLTEXSTORAGE3DPROC origTexStorage3D = nullptr;
	PFNGLGENERATEMIPMAPPROC origGenerateMipmap = nullptr;
	PFNGLGENERATEMIPMAPEXTPROC origGenerateMipmapEXT = nullptr;
	PFNGLBUFFERDATAPROC origBufferData = nullptr;
	PFNGLBUFFERSTORAGEPROC origBufferStorage = nullptr;
	PFNGLRENDERBUFFERSTORAGEPROC origRenderbufferStorage = nullptr;
	PFNGLRENDERBUFFERSTORAGEEXTPROC origRenderbufferStorageEXT = nullptr;
	PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC origRenderbufferStorageMultisample = nullptr;
	PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC origRenderbufferStorageMultisampleEXT = nullptr;
	PFNGLDELETETEXTURESPROC origDeleteTextures = nullptr;
	PFNGLDELETEBUFFERSPROC origDeleteBuffers = nullptr;
	PFNGLDELETERENDERBUFFERSPROC origDeleteRenderbuffers = nullptr;
	PFNGLDELETERENDERBUFFERSEXTPROC origDeleteRenderbuffersEXT = nullptr;

	void APIENTRY HookTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels) {
		origTexImage1D(target, level, internalformat, width, border, format, type, pixels);
		OnTexImage(target, level, internalformat, width, 1, 1, 1, 0);
	}
	void APIENTRY HookTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels) {
		origTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
		OnTexImage(target, level, internalformat, width, height, 1, 1, 0);
	}
	void APIENTRY HookTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels) {
		origTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
		OnTexImage(target, level, internalformat, width, height, depth, 1, 0);
	}
	void APIENTRY HookTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
		origTexImage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
		OnTexImage(target, 0, internalformat, width, height, 1, samples, 0);
	}
	void APIENTRY HookCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data) {
		origCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
		OnTexImage(target, level, internalformat, width, height, 1, 1, std::max(imageSize, 1));
	}
	void APIENTRY HookTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
		origTexStorage2D(target, levels, internalformat, width, height);
		OnTexStorage(target, levels, internalformat, width, height, 1);
	}
	void APIENTRY HookTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
		origTexStorage3D(target, levels, internalformat, width, height, depth);
		OnTexStorage(target, levels, internalformat, width, height, depth);
	}
	void APIENTRY HookGenerateMipmap(GLenum target) {
		origGenerateMipmap(target);
		OnGenerateMipmap(target);
	}
	void APIENTRY HookGenerateMipmapEXT(GLenum target) {
		origGenerateMipmapEXT(target);
		OnGenerateMipmap(target);
	}
	void APIENTRY HookBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
		origBufferData(target, size, data, usage);
		OnBufferData(target, size);
	}
	void APIENTRY HookBufferStorage(GLenum target, GLsizeiptr size, const void* data, GLbitfield flags) {
		origBufferStorage(target, size, data, flags);
		OnBufferData(target, size);
	}
	void APIENTRY HookRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
		origRenderbufferStorage(target, internalformat, width, height);
		OnRenderbufferStorage(1, internalformat, width, height);
	}
	void APIENTRY HookRenderbufferStorageEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
		origRenderbufferStorageEXT(target, internalformat, width, height);
		OnRenderbufferStorage(1, internalformat, width, height);
	}
	void APIENTRY HookRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
		origRenderbufferStorageMultisample(target, samples, internalformat, width, height);
		OnRenderbufferStorage(samples, internalformat, width, height);
	}
	void APIENTRY HookRenderbufferStorageMultisampleEXT(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
		origRenderbufferStorageMultisampleEXT(target, samples, internalformat, width, height);
		OnRenderbufferStorage(samples, internalformat, width, height);
	}
	void APIENTRY HookDeleteTextures(GLsizei n, const GLuint* names) {
		OnDelete(KIND_TEXTURE, n, names);
		origDeleteTextures(n, names);
	}
	void APIENTRY HookDeleteBuffers(GLsizei n, const GLuint* names) {
		OnDelete(KIND_BUFFER, n, names);
		origDeleteBuffers(n, names);
	}
	void APIENTRY HookDeleteRenderbuffers(GLsizei n, const GLuint* names) {
		OnDelete(KIND_RENDERBUFFER, n, names);
		origDeleteRenderbuffers(n, names);
	}
	void APIENTRY HookDeleteRenderbuffersEXT(GLsizei n, const GLuint* names) {
		OnDelete(KIND_RENDERBUFFER, n, names);
		origDeleteRenderbuffersEXT(n, names);
	}

	template<typename F> void Hook(F& glad, F& orig, F hook) {
		if (glad == nullptr)
			return;

		orig = glad;
		glad = hook;
	}
	template<typename F> void Unhook(F& glad, F& orig) {
		if (orig == nullptr)
			return;

		glad = orig;
		orig = nullptr;
	}

	#define VRAM_HOOKS(X) \
		X(TexImage1D) X(TexImage2D) X(TexImage3D) X(TexImage2DMultisample) X(CompressedTexImage2D) \
		X(TexStorage2D) X(TexStorage3D) X(GenerateMipmap) X(GenerateMipmapEXT) \
		X(BufferData) X(BufferStorage) \
		X(RenderbufferStorage) X(RenderbufferStorageEXT) X(RenderbufferStorageMultisample) X(RenderbufferStorageMultisampleEXT) \
		X(DeleteTextures) X(DeleteBuffers) X(DeleteRenderbuffers) X(DeleteRenderbuffersEXT)


#ifdef _WIN32
	// this process's video memory usage and budget as the OS sees them, on the adapter with the most
	// dedicated memory (the GL context does not tell which adapter it runs on)
	IDXGIAdapter3* dxgiAdapter = nullptr;

	void InitDXGI()
	{
		// IIDs spelled out so that nothing extra has to be linked
		static constexpr GUID IID_DXGIFactory1 = { 0x770aae78, 0xf26f, 0x4dba, { 0xa8, 0x29, 0x25, 0x3c, 0x83, 0xd1, 0xb3, 0x87 } };
		static constexpr GUID IID_DXGIAdapter3 = { 0x645967a4, 0x1392, 0x4310, { 0xa7, 0x98, 0x80, 0x53, 0xce, 0x3e, 0x93, 0xfd } };
		using CreateDXGIFactory1Func = HRESULT (WINAPI*)(REFIID, void**);

		const HMODULE dxgi = LoadLibraryA("dxgi.dll");
		if (dxgi == nullptr)
			return;

		const auto createFactory = reinterpret_cast<CreateDXGIFactory1Func>(reinterpret_cast<void*>(GetProcAddress(dxgi, "CreateDXGIFactory1")));
		IDXGIFactory1* factory = nullptr;

		if (createFactory == nullptr || FAILED(createFactory(IID_DXGIFactory1, reinterpret_cast<void**>(&factory))))
			return;

		IDXGIAdapter1* adapter = nullptr;
		SIZE_T bestMemory = 0;

		for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
			DXGI_ADAPTER_DESC1 desc;
			IDXGIAdapter3* adapter3 = nullptr;

			if (SUCCEEDED(adapter->GetDesc1(&desc)) && (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 && desc.DedicatedVideoMemory > bestMemory) {
				if (SUCCEEDED(adapter->QueryInterface(IID_DXGIAdapter3, reinterpret_cast<void**>(&adapter3)))) {
					if (dxgiAdapter != nullptr)
						dxgiAdapter->Release();

					dxgiAdapter = adapter3;
					bestMemory = desc.DedicatedVideoMemory;
				}
			}

			adapter->Release();
		}

		factory->Release();
	}

	void KillDXGI()
	{
		if (dxgiAdapter == nullptr)
			return;

		dxgiAdapter->Release();
		dxgiAdapter = nullptr;
	}

	void PlotDXGI()
	{
		DXGI_QUERY_VIDEO_MEMORY_INFO info;

		if (dxgiAdapter == nullptr || FAILED(dxgiAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info)))
			return;

		TracyPlot("VRAM: process usage", static_cast<int64_t>(info.CurrentUsage));
		TracyPlot("VRAM: process budget", static_cast<int64_t>(info.Budget));
	}
#endif

	void ConfigurePlots()
	{
		for (const char* name: { "VRAM: driver used", "VRAM: driver evicted", "VRAM: process usage", "VRAM: process budget" }) {
			TracyPlotConfig(name, tracy::PlotFormatType::Memory, true, true, 0);
		}
	}
}


void GL::VRAMTracker::Init()
{
	if (initialized)
		return;

	#define X(name) Hook(glad_gl##name, orig##name, Hook##name);
	VRAM_HOOKS(X)
	#undef X

	#ifdef _WIN32
	InitDXGI();
	#endif

	ConfigurePlots();
	initialized = true;
}

void GL::VRAMTracker::Kill()
{
	if (!initialized)
		return;

	#define X(name) Unhook(glad_gl##name, orig##name);
	VRAM_HOOKS(X)
	#undef X

	#ifdef _WIN32
	KillDXGI();
	#endif

	std::lock_guard<std::mutex> lock(mutex);
	objects.clear();
	textures.clear();
	initialized = false;
}

void GL::VRAMTracker::Update()
{
	if (!initialized)
		return;

	const bool isConnected = TracyIsConnected;

	{
		std::lock_guard<std::mutex> lock(mutex);

		// a new Tracy connection learns about the objects that already exist
		if (isConnected != connected) {
			connected = isConnected;

			for (auto& [key, obj]: objects) {
				if ((obj.announced = (connected && obj.bytes > 0)))
					TracyAllocN(FakePtr(key), obj.bytes, POOL_NAMES[key >> 32]);
			}

			if (connected)
				ConfigurePlots();
		}
	}

	if (!connected)
		return;

	const auto now = std::chrono::steady_clock::now();

	if (now < nextSample)
		return;

	nextSample = now + std::chrono::milliseconds(250);

	// driver: the whole GPU, other processes included
	GLint memory[2] = { 0, 0 };

	if (GetAvailableVideoRAM(memory, globalRenderingInfo.glVendor))
		TracyPlot("VRAM: driver used", static_cast<int64_t>(memory[0] - memory[1]) * 1024);

	if (GLAD_GL_NVX_gpu_memory_info) {
		GLint evictionCount = 0;
		GLint evictedKB = 0;
		glGetIntegerv(GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX, &evictionCount);
		glGetIntegerv(GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, &evictedKB);
		TracyPlot("VRAM: driver evictions", static_cast<int64_t>(evictionCount));
		TracyPlot("VRAM: driver evicted", static_cast<int64_t>(evictedKB) * 1024);
	}

	#ifdef _WIN32
	PlotDXGI();
	#endif
}

#else

void GL::VRAMTracker::Init() {}
void GL::VRAMTracker::Update() {}
void GL::VRAMTracker::Kill() {}

#endif
