/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "MacEGLBridge.h"

#ifdef MAC_EGL_BRIDGE_AVAILABLE

#include <dlfcn.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <mutex>

#include <mach-o/dyld.h>

#include <SDL.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/graphics/IOGraphicsTypes.h>
#include <IOSurface/IOSurfaceRef.h>

#include "Rendering/GL/myGL.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/Platform/Misc.h"

CONFIG(float, MacRenderScale).defaultValue(1.0f).minimumValue(1.0f).maximumValue(2.0f).description("macOS only. Pixels rendered per window point: 1, 1.5 or 2. A Retina display shows 2, so higher values are sharper and cost frame rate. Capped at the display's own pixel density.");

// the few CoreGraphics display calls the bridge needs, gcc cannot parse the CoreGraphics headers
extern "C" {
	typedef struct CGDisplayMode* CGDisplayModeRef;
	struct MacCGPoint { double x, y; };
	struct MacCGRect { double x, y, w, h; };

	int32_t CGGetDisplaysWithPoint(MacCGPoint point, uint32_t maxDisplays, uint32_t* displays, uint32_t* matchingDisplayCount);
	MacCGRect CGDisplayBounds(uint32_t display);
	CFArrayRef CGDisplayCopyAllDisplayModes(uint32_t display, CFDictionaryRef options);
	size_t CGDisplayModeGetPixelWidth(CGDisplayModeRef mode);
	uint32_t CGDisplayModeGetIOFlags(CGDisplayModeRef mode);

	// CoreVideo, whose headers gcc cannot parse either
	typedef struct __CVBuffer* CVPixelBufferRef;
	int32_t CVPixelBufferCreateWithIOSurface(CFAllocatorRef allocator, IOSurfaceRef surface, CFDictionaryRef attributes, CVPixelBufferRef* pixelBufferOut);
	void CVPixelBufferRelease(CVPixelBufferRef pixelBuffer);
}

// minimal EGL declarations; libEGL is dlopen()ed so there is no build-time dependency on Mesa
namespace {
	typedef void* EGLDisplay;
	typedef void* EGLConfig;
	typedef void* EGLSurface;
	typedef void* EGLContext;
	typedef int32_t EGLint;
	typedef unsigned int EGLBoolean;
	typedef unsigned int EGLenum;

	constexpr EGLint EGL_NONE = 0x3038;
	constexpr EGLint EGL_RED_SIZE = 0x3024, EGL_GREEN_SIZE = 0x3023, EGL_BLUE_SIZE = 0x3022, EGL_ALPHA_SIZE = 0x3021;
	constexpr EGLint EGL_DEPTH_SIZE = 0x3025, EGL_STENCIL_SIZE = 0x3026;
	constexpr EGLint EGL_SURFACE_TYPE = 0x3033, EGL_PBUFFER_BIT = 0x0001;
	constexpr EGLint EGL_RENDERABLE_TYPE = 0x3040, EGL_OPENGL_BIT = 0x0008;
	constexpr EGLint EGL_WIDTH = 0x3057, EGL_HEIGHT = 0x3056;
	constexpr EGLenum EGL_OPENGL_API = 0x30A2;
	constexpr EGLint EGL_CONTEXT_MAJOR_VERSION = 0x3098, EGL_CONTEXT_MINOR_VERSION = 0x30FB;
	constexpr EGLint EGL_CONTEXT_OPENGL_PROFILE_MASK = 0x30FD;
	constexpr EGLint EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT = 0x1, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT = 0x2;
	constexpr EGLenum EGL_PLATFORM_SURFACELESS_MESA = 0x31DD;

	struct EGLFuncs {
		void* lib = nullptr;
		void* (*GetProcAddress)(const char*) = nullptr;
		EGLDisplay (*GetPlatformDisplayEXT)(EGLenum, void*, const EGLint*) = nullptr;
		EGLBoolean (*Initialize)(EGLDisplay, EGLint*, EGLint*) = nullptr;
		EGLBoolean (*Terminate)(EGLDisplay) = nullptr;
		EGLBoolean (*BindAPI)(EGLenum) = nullptr;
		EGLBoolean (*ChooseConfig)(EGLDisplay, const EGLint*, EGLConfig*, EGLint, EGLint*) = nullptr;
		EGLSurface (*CreatePbufferSurface)(EGLDisplay, EGLConfig, const EGLint*) = nullptr;
		EGLBoolean (*DestroySurface)(EGLDisplay, EGLSurface) = nullptr;
		EGLContext (*CreateContext)(EGLDisplay, EGLConfig, EGLContext, const EGLint*) = nullptr;
		EGLBoolean (*DestroyContext)(EGLDisplay, EGLContext) = nullptr;
		EGLBoolean (*MakeCurrent)(EGLDisplay, EGLSurface, EGLSurface, EGLContext) = nullptr;
		EGLint (*GetError)() = nullptr;
	} egl;

	EGLDisplay display = nullptr;
	EGLConfig config = nullptr;
	EGLContext context = nullptr;
	EGLSurface surface = nullptr;
	int surfSizeX = 0;
	int surfSizeY = 0;
	float renderScale = 1.0f;

	SDL_Renderer* renderer = nullptr;
	SDL_Texture* texture = nullptr;
	int texSizeX = 0;
	int texSizeY = 0;

	// One frame read back from the pbuffer. A pinned slot reads straight into an IOSurface
	// that SDL draws from, a copy slot reads into a PBO that is copied into the streaming texture.
	struct Slot {
		GLuint buffer = 0;
		GLsync fence = nullptr;
		int sizeX = 0;
		int sizeY = 0;
		int rowPixels = 0;
		IOSurfaceRef ioSurface = nullptr;
		CVPixelBufferRef pixelBuffer = nullptr;
		SDL_Texture* texture = nullptr;
	};

	// three, so a slot is written again only after Metal has had two frames to finish drawing it
	constexpr unsigned int NUM_SLOTS = 3;
	Slot slots[NUM_SLOTS];
	unsigned int frameIdx = 0;

	enum class Readback { Unknown, Pinned, Copy };
	Readback readback = Readback::Unknown;

	// SDL3 calls for wrapping an IOSurface as a texture, from the SDL3 that sdl2-compat already loaded
	struct SDL3Funcs {
		uint32_t (*CreateProperties)() = nullptr;
		bool (*SetPointerProperty)(uint32_t, const char*, void*) = nullptr;
		bool (*SetNumberProperty)(uint32_t, const char*, int64_t) = nullptr;
		void (*DestroyProperties)(uint32_t) = nullptr;
		SDL_Texture* (*CreateTextureWithProperties)(SDL_Renderer*, uint32_t) = nullptr;
	} sdl3;

	std::mutex surfMutex;

	// a Mesa tree shipped next to the engine binary takes precedence over the system one
	const std::string& MesaPrefix() {
		static const std::string prefix = []() -> std::string {
			const std::string bundled = Platform::GetProcessExecutablePath() + "mesa";
			return FileSystem::FileExists(bundled + "/lib/libEGL.1.dylib") ? bundled : MAC_MESA_PREFIX;
		}();
		return prefix;
	}

	bool LoadEGL() {
		if (egl.lib != nullptr)
			return true;

		if ((egl.lib = dlopen((MesaPrefix() + "/lib/libEGL.1.dylib").c_str(), RTLD_NOW | RTLD_LOCAL)) == nullptr) {
			LOG_L(L_ERROR, "[MacEGLBridge] could not load Mesa libEGL (%s)", dlerror());
			return false;
		}

		#define LOADSYM(n) egl.n = reinterpret_cast<decltype(egl.n)>(dlsym(egl.lib, "egl" #n))
		LOADSYM(GetProcAddress);
		LOADSYM(Initialize);
		LOADSYM(Terminate);
		LOADSYM(BindAPI);
		LOADSYM(ChooseConfig);
		LOADSYM(CreatePbufferSurface);
		LOADSYM(DestroySurface);
		LOADSYM(CreateContext);
		LOADSYM(DestroyContext);
		LOADSYM(MakeCurrent);
		LOADSYM(GetError);
		#undef LOADSYM
		if (egl.GetProcAddress != nullptr)
			egl.GetPlatformDisplayEXT = reinterpret_cast<decltype(egl.GetPlatformDisplayEXT)>(egl.GetProcAddress("eglGetPlatformDisplayEXT"));

		return (egl.GetPlatformDisplayEXT != nullptr && egl.Initialize != nullptr && egl.CreateContext != nullptr && egl.MakeCurrent != nullptr);
	}

	void SetupEnvironment() {
		setenv("GALLIUM_DRIVER", "zink", 0);

		// Zink needs robustness2.nullDescriptor, which MoltenVK lacks, so the loader gets KosmicKrisp
		if (getenv("VK_ICD_FILENAMES") == nullptr && getenv("VK_DRIVER_FILES") == nullptr) {
			const std::string icd = MesaPrefix() + "/share/vulkan/icd.d/kosmickrisp_mesa_icd.aarch64.json";
			setenv("VK_DRIVER_FILES", icd.c_str(), 0);
		}

		// Zink reports GL 2.1 on KosmicKrisp due to missing geometry shaders
		setenv("MESA_GL_VERSION_OVERRIDE", "4.6COMPAT", 0);
		setenv("MESA_GLSL_VERSION_OVERRIDE", "460", 0);
	}

	bool CreateSurface(int sx, int sy) {
		sx = std::max(sx, 1);
		sy = std::max(sy, 1);

		const EGLint attrs[] = {EGL_WIDTH, sx, EGL_HEIGHT, sy, EGL_NONE};
		EGLSurface s = egl.CreatePbufferSurface(display, config, attrs);

		if (s == nullptr) {
			LOG_L(L_ERROR, "[MacEGLBridge] eglCreatePbufferSurface(%dx%d) failed: 0x%x", sx, sy, egl.GetError());
			return false;
		}

		EGLSurface old = surface;
		surface = s;
		surfSizeX = sx;
		surfSizeY = sy;

		if (context != nullptr)
			egl.MakeCurrent(display, surface, surface, context);
		if (old != nullptr)
			egl.DestroySurface(display, old);

		return true;
	}

	bool CreatePresenter(SDL_Window* window) {
		if (renderer != nullptr)
			return true;

		SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
		renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
		if (renderer == nullptr) {
			LOG_L(L_ERROR, "[MacEGLBridge] SDL_CreateRenderer failed: %s", SDL_GetError());
			return false;
		}

		SDL_RendererInfo info;
		if (SDL_GetRendererInfo(renderer, &info) == 0)
			LOG("[MacEGLBridge] presenting through SDL renderer \"%s\"", info.name);

		return true;
	}

	bool LoadSDL3() {
		void* lib = nullptr;

		// dlopen by the loaded image's own path, a second copy of SDL3 would not know the renderer
		for (uint32_t i = 0; i < _dyld_image_count() && lib == nullptr; ++i) {
			const char* name = _dyld_get_image_name(i);
			if (strstr(name, "libSDL3") != nullptr)
				lib = dlopen(name, RTLD_NOLOAD | RTLD_LAZY);
		}

		if (lib == nullptr)
			return false;

		sdl3.CreateProperties = reinterpret_cast<decltype(sdl3.CreateProperties)>(dlsym(lib, "SDL_CreateProperties"));
		sdl3.SetPointerProperty = reinterpret_cast<decltype(sdl3.SetPointerProperty)>(dlsym(lib, "SDL_SetPointerProperty"));
		sdl3.SetNumberProperty = reinterpret_cast<decltype(sdl3.SetNumberProperty)>(dlsym(lib, "SDL_SetNumberProperty"));
		sdl3.DestroyProperties = reinterpret_cast<decltype(sdl3.DestroyProperties)>(dlsym(lib, "SDL_DestroyProperties"));
		sdl3.CreateTextureWithProperties = reinterpret_cast<decltype(sdl3.CreateTextureWithProperties)>(dlsym(lib, "SDL_CreateTextureWithProperties"));

		return sdl3.CreateProperties != nullptr && sdl3.SetPointerProperty != nullptr && sdl3.SetNumberProperty != nullptr &&
			sdl3.DestroyProperties != nullptr && sdl3.CreateTextureWithProperties != nullptr;
	}

	void DestroySlot(Slot& s) {
		// the GPU may still be writing into a pinned slot's pages
		if (s.fence != nullptr) {
			glClientWaitSync(s.fence, 0, GL_TIMEOUT_IGNORED);
			glDeleteSync(s.fence);
		}
		if (s.buffer != 0)
			glDeleteBuffers(1, &s.buffer);
		if (s.texture != nullptr)
			SDL_DestroyTexture(s.texture);
		if (s.pixelBuffer != nullptr)
			CVPixelBufferRelease(s.pixelBuffer);
		if (s.ioSurface != nullptr) {
			IOSurfaceUnlock(s.ioSurface, 0, nullptr);
			CFRelease(s.ioSurface);
		}

		s = Slot{};
	}

	void DestroySlots() {
		for (Slot& s: slots)
			DestroySlot(s);
	}

	IOSurfaceRef CreateIOSurface(int sx, int sy, size_t rowBytes) {
		// The pixels are RGBA, but CVPixelBufferCreateWithIOSurface rejects 'RGBA' surfaces and BGRA readback
		// stalls Zink. The label is only read by CoreVideo, SDL makes an RGBA Metal texture from the format it is given.
		constexpr int32_t pixelFormatLabel = 0x42475241; // the four-char code 'BGRA'
		const int32_t values[] = {sx, sy, 4, static_cast<int32_t>(rowBytes), pixelFormatLabel};
		const CFStringRef keys[] = {kIOSurfaceWidth, kIOSurfaceHeight, kIOSurfaceBytesPerElement, kIOSurfaceBytesPerRow, kIOSurfacePixelFormat};
		CFTypeRef numbers[std::size(values)];

		for (size_t i = 0; i < std::size(values); ++i)
			numbers[i] = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &values[i]);

		const CFDictionaryRef props = CFDictionaryCreate(kCFAllocatorDefault, reinterpret_cast<const void**>(const_cast<CFStringRef*>(keys)), numbers, std::size(keys), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		const IOSurfaceRef ioSurface = IOSurfaceCreate(props);

		CFRelease(props);
		for (CFTypeRef n: numbers)
			CFRelease(n);

		return ioSurface;
	}

	// The IOSurface's pages back both a GL pack buffer (GL_AMD_pinned_memory) and the SDL texture,
	// so the readback is the only copy of the frame.
	bool CreatePinnedSlot(Slot& s, int sx, int sy) {
		const size_t rowBytes = IOSurfaceAlignProperty(kIOSurfaceBytesPerRow, sx * 4);

		if ((s.ioSurface = CreateIOSurface(sx, sy, rowBytes)) == nullptr) {
			LOG_L(L_ERROR, "[MacEGLBridge] IOSurfaceCreate(%dx%d) failed", sx, sy);
			return false;
		}

		// the pinned GL buffer needs the pages at a fixed address for as long as it exists
		IOSurfaceLock(s.ioSurface, 0, nullptr);

		while (glGetError() != GL_NO_ERROR) {}

		glGenBuffers(1, &s.buffer);
		glBindBuffer(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, s.buffer);
		glBufferData(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, IOSurfaceGetAllocSize(s.ioSurface), IOSurfaceGetBaseAddress(s.ioSurface), GL_STREAM_READ);
		glBindBuffer(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, 0);

		if (const GLenum err = glGetError(); err != GL_NO_ERROR) {
			LOG_L(L_ERROR, "[MacEGLBridge] pinning the IOSurface as a GL buffer failed: 0x%x", err);
			return false;
		}

		if (CVPixelBufferCreateWithIOSurface(kCFAllocatorDefault, s.ioSurface, nullptr, &s.pixelBuffer) != 0) {
			LOG_L(L_ERROR, "[MacEGLBridge] CVPixelBufferCreateWithIOSurface failed");
			return false;
		}

		const uint32_t props = sdl3.CreateProperties();
		sdl3.SetPointerProperty(props, "SDL.texture.create.metal.pixelbuffer", s.pixelBuffer);
		sdl3.SetNumberProperty(props, "SDL.texture.create.format", SDL_PIXELFORMAT_ABGR8888);
		sdl3.SetNumberProperty(props, "SDL.texture.create.width", sx);
		sdl3.SetNumberProperty(props, "SDL.texture.create.height", sy);
		s.texture = sdl3.CreateTextureWithProperties(renderer, props);
		sdl3.DestroyProperties(props);

		if (s.texture == nullptr) {
			LOG_L(L_ERROR, "[MacEGLBridge] SDL texture over the IOSurface failed: %s", SDL_GetError());
			return false;
		}

		// what SDL_CreateTexture would have set
		SDL_SetTextureBlendMode(s.texture, SDL_BLENDMODE_NONE);
		SDL_SetTextureScaleMode(s.texture, SDL_ScaleModeLinear);

		s.sizeX = sx;
		s.sizeY = sy;
		s.rowPixels = static_cast<int>(rowBytes / 4);
		return true;
	}

	void CreateCopySlot(Slot& s, int sx, int sy) {
		glGenBuffers(1, &s.buffer);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, s.buffer);
		glBufferData(GL_PIXEL_PACK_BUFFER, sx * sy * 4, nullptr, GL_STREAM_READ);

		s.sizeX = sx;
		s.sizeY = sy;
		s.rowPixels = sx;
	}

	// copies a copy slot's pixels into the streaming texture, which SDL then uploads to Metal
	void CopyToTexture(const Slot& s) {
		glBindBuffer(GL_PIXEL_PACK_BUFFER, s.buffer);
		const void* data = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, s.sizeX * s.sizeY * 4, GL_MAP_READ_BIT);

		if (data == nullptr)
			return;

		if (texture == nullptr || texSizeX != s.sizeX || texSizeY != s.sizeY) {
			if (texture != nullptr)
				SDL_DestroyTexture(texture);
			texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, s.sizeX, s.sizeY);
			texSizeX = s.sizeX;
			texSizeY = s.sizeY;
		}

		void* pixels = nullptr;
		int pitch = 0;
		if (texture != nullptr && SDL_LockTexture(texture, nullptr, &pixels, &pitch) == 0) {
			if (pitch == s.sizeX * 4) {
				memcpy(pixels, data, s.sizeX * s.sizeY * 4);
			} else {
				for (int y = 0; y < s.sizeY; ++y)
					memcpy(static_cast<uint8_t*>(pixels) + y * pitch, static_cast<const uint8_t*>(data) + y * s.sizeX * 4, s.sizeX * 4);
			}
			SDL_UnlockTexture(texture);
		}

		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	}

	void ChooseReadback() {
		if (GLAD_GL_AMD_pinned_memory && LoadSDL3()) {
			readback = Readback::Pinned;
			LOG("[MacEGLBridge] reading frames back into IOSurfaces that SDL draws directly");
		} else {
			readback = Readback::Copy;
			LOG("[MacEGLBridge] reading frames back through a copy (GL_AMD_pinned_memory=%d)", GLAD_GL_AMD_pinned_memory);
		}
	}

	size_t NativePixelWidth(uint32_t cgDisplay) {
		size_t width = 0;
		const CFArrayRef modes = CGDisplayCopyAllDisplayModes(cgDisplay, nullptr);

		if (modes == nullptr)
			return 0;

		for (CFIndex i = 0; i < CFArrayGetCount(modes); ++i) {
			const auto mode = static_cast<CGDisplayModeRef>(const_cast<void*>(CFArrayGetValueAtIndex(modes, i)));

			if (CGDisplayModeGetIOFlags(mode) & kDisplayModeNativeFlag)
				width = std::max(width, CGDisplayModeGetPixelWidth(mode));
		}

		CFRelease(modes);
		return width;
	}

	// panel pixels per point on the window's display. In a scaled mode, e.g. a 4K panel set to look like
	// 2560x1440, macOS gives windows a 2x backing and shrinks it to fit, so 1.5 is all the panel can show.
	// Returns 0 when the panel's native mode is unknown.
	float PanelDensity(SDL_Window* window) {
		static uint32_t cachedDisplay = 0;
		static size_t cachedWidth = 0;

		SDL_Rect bounds;
		if (SDL_GetDisplayBounds(SDL_GetWindowDisplayIndex(window), &bounds) != 0)
			return 0.0f;

		uint32_t cgDisplay = 0;
		uint32_t count = 0;
		const MacCGPoint centre = {bounds.x + bounds.w * 0.5, bounds.y + bounds.h * 0.5};

		if (CGGetDisplaysWithPoint(centre, 1, &cgDisplay, &count) != 0 || count == 0)
			return 0.0f;

		// listing display modes is slow, and a panel's native mode does not change
		if (cgDisplay != cachedDisplay) {
			cachedDisplay = cgDisplay;
			cachedWidth = NativePixelWidth(cgDisplay);
		}

		const MacCGRect cgBounds = CGDisplayBounds(cgDisplay);
		return (cgBounds.w > 0.0) ? static_cast<float>(cachedWidth / cgBounds.w) : 0.0f;
	}

	float WantedRenderScale(SDL_Window* window) {
		int wx = 0, wy = 0;
		int ox = 0, oy = 0;
		SDL_GetWindowSize(window, &wx, &wy);

		if (renderer == nullptr || wx <= 0 || SDL_GetRendererOutputSize(renderer, &ox, &oy) != 0)
			return 1.0f;

		// the backing macOS gives the window, 2 on a Retina display and 1 on most external ones
		float density = ox / static_cast<float>(wx);

		if (const float panel = PanelDensity(window); panel > 0.0f)
			density = std::min(density, panel);

		return std::clamp(configHandler->GetFloat("MacRenderScale"), 1.0f, std::max(density, 1.0f));
	}
}


// KosmicKrisp has neither wideLines nor geometry shaders, so Zink clamps every line to one pixel.
// Wide lines are approximated by repeating the draw with the viewport shifted by whole pixels.
namespace WideLines {
	thread_local float lineWidth = 1.0f;

	PFNGLLINEWIDTHPROC realLineWidth = nullptr;
	PFNGLDRAWARRAYSPROC realDrawArrays = nullptr;
	PFNGLDRAWELEMENTSPROC realDrawElements = nullptr;
	PFNGLDRAWRANGEELEMENTSPROC realDrawRangeElements = nullptr;
	PFNGLDRAWARRAYSINSTANCEDPROC realDrawArraysInstanced = nullptr;
	PFNGLDRAWELEMENTSINSTANCEDPROC realDrawElementsInstanced = nullptr;
	PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC realDrawArraysInstancedBaseInstance = nullptr;
	PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC realDrawElementsInstancedBaseInstance = nullptr;
	PFNGLDRAWELEMENTSBASEVERTEXPROC realDrawElementsBaseVertex = nullptr;
	PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC realDrawElementsInstancedBaseVertex = nullptr;
	PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC realDrawElementsInstancedBaseVertexBaseInstance = nullptr;

	bool IsLineMode(GLenum mode) {
		switch (mode) {
			case GL_LINES:
			case GL_LINE_STRIP:
			case GL_LINE_LOOP:
			case GL_LINES_ADJACENCY:
			case GL_LINE_STRIP_ADJACENCY:
				return true;
			default:
				return false;
		}
	}

	template<typename F>
	void Draw(GLenum mode, F&& draw) {
		draw();

		if (lineWidth < 1.5f || !IsLineMode(mode))
			return;

		// glPopAttrib restores width and viewport behind our back, so query both before redrawing
		GLfloat width = 1.0f;
		glGetFloatv(GL_LINE_WIDTH, &width);
		if (width < 1.5f)
			return;

		GLint viewport[4] = {};
		glGetIntegerv(GL_VIEWPORT, viewport);

		const int radius = static_cast<int>((width - 1.0f) * 0.5f + 0.5f);
		for (int i = 1; i <= radius; ++i) {
			const int offsets[4][2] = {{i, 0}, {-i, 0}, {0, i}, {0, -i}};
			for (const auto& o: offsets) {
				glViewport(viewport[0] + o[0], viewport[1] + o[1], viewport[2], viewport[3]);
				draw();
			}
		}
		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	}

	void APIENTRY LineWidth(GLfloat width) {
		lineWidth = width;
		realLineWidth(width);
	}
	void APIENTRY DrawArrays(GLenum mode, GLint first, GLsizei count) {
		Draw(mode, [&]() { realDrawArrays(mode, first, count); });
	}
	void APIENTRY DrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
		Draw(mode, [&]() { realDrawElements(mode, count, type, indices); });
	}
	void APIENTRY DrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void* indices) {
		Draw(mode, [&]() { realDrawRangeElements(mode, start, end, count, type, indices); });
	}
	void APIENTRY DrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instances) {
		Draw(mode, [&]() { realDrawArraysInstanced(mode, first, count, instances); });
	}
	void APIENTRY DrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instances) {
		Draw(mode, [&]() { realDrawElementsInstanced(mode, count, type, indices, instances); });
	}
	void APIENTRY DrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count, GLsizei instances, GLuint baseInstance) {
		Draw(mode, [&]() { realDrawArraysInstancedBaseInstance(mode, first, count, instances, baseInstance); });
	}
	void APIENTRY DrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instances, GLuint baseInstance) {
		Draw(mode, [&]() { realDrawElementsInstancedBaseInstance(mode, count, type, indices, instances, baseInstance); });
	}
	void APIENTRY DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void* indices, GLint baseVertex) {
		Draw(mode, [&]() { realDrawElementsBaseVertex(mode, count, type, indices, baseVertex); });
	}
	void APIENTRY DrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instances, GLint baseVertex) {
		Draw(mode, [&]() { realDrawElementsInstancedBaseVertex(mode, count, type, indices, instances, baseVertex); });
	}
	void APIENTRY DrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instances, GLint baseVertex, GLuint baseInstance) {
		Draw(mode, [&]() { realDrawElementsInstancedBaseVertexBaseInstance(mode, count, type, indices, instances, baseVertex, baseInstance); });
	}

	template<typename T>
	void* Wrap(void* real, T& realPtr, T wrapper) {
		if (real == nullptr)
			return nullptr;

		realPtr = reinterpret_cast<T>(real);
		return reinterpret_cast<void*>(wrapper);
	}

	void* GetProcAddress(const char* name, void* real) {
		#define WRAP(fn) if (strcmp(name, "gl" #fn) == 0) return Wrap(real, real##fn, &fn)
		WRAP(LineWidth);
		WRAP(DrawArrays);
		WRAP(DrawElements);
		WRAP(DrawRangeElements);
		WRAP(DrawArraysInstanced);
		WRAP(DrawElementsInstanced);
		WRAP(DrawArraysInstancedBaseInstance);
		WRAP(DrawElementsInstancedBaseInstance);
		WRAP(DrawElementsBaseVertex);
		WRAP(DrawElementsInstancedBaseVertex);
		WRAP(DrawElementsInstancedBaseVertexBaseInstance);
		#undef WRAP
		return real;
	}
}


bool MacEGLBridge::Active() { return (context != nullptr); }


void* MacEGLBridge::CreateContext(SDL_Window* window, int major, int minor, bool core)
{
	SetupEnvironment();

	if (!LoadEGL())
		return nullptr;

	if ((display = egl.GetPlatformDisplayEXT(EGL_PLATFORM_SURFACELESS_MESA, nullptr, nullptr)) == nullptr) {
		LOG_L(L_ERROR, "[MacEGLBridge] eglGetPlatformDisplayEXT(surfaceless) failed: 0x%x", egl.GetError());
		return nullptr;
	}

	EGLint vmaj = 0, vmin = 0;
	if (!egl.Initialize(display, &vmaj, &vmin)) {
		LOG_L(L_ERROR, "[MacEGLBridge] eglInitialize failed: 0x%x", egl.GetError());
		return nullptr;
	}
	LOG("[MacEGLBridge] EGL %d.%d initialized (GALLIUM_DRIVER=%s VK_DRIVER_FILES=%s)", vmaj, vmin, getenv("GALLIUM_DRIVER"), getenv("VK_DRIVER_FILES"));

	egl.BindAPI(EGL_OPENGL_API);

	const EGLint cfgAttrs[] = {
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24, EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};
	EGLint numCfgs = 0;
	if (!egl.ChooseConfig(display, cfgAttrs, &config, 1, &numCfgs) || numCfgs < 1) {
		LOG_L(L_ERROR, "[MacEGLBridge] eglChooseConfig failed: 0x%x", egl.GetError());
		return nullptr;
	}

	const EGLint ctxAttrs[] = {
		EGL_CONTEXT_MAJOR_VERSION, major,
		EGL_CONTEXT_MINOR_VERSION, minor,
		EGL_CONTEXT_OPENGL_PROFILE_MASK, core ? EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT : EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT,
		EGL_NONE
	};
	if ((context = egl.CreateContext(display, config, nullptr, ctxAttrs)) == nullptr) {
		LOG_L(L_ERROR, "[MacEGLBridge] GL %d.%d %s context failed: 0x%x", major, minor, core ? "core" : "compatibility", egl.GetError());
		return nullptr;
	}

	if (!CreatePresenter(window))
		return nullptr;

	renderScale = WantedRenderScale(window);
	LOG("[MacEGLBridge] render scale %.2f", renderScale);

	int wx = 0, wy = 0;
	SDL_GetWindowSize(window, &wx, &wy);
	if (!CreateSurface(ToPixels(wx), ToPixels(wy)) || !egl.MakeCurrent(display, surface, surface, context)) {
		LOG_L(L_ERROR, "[MacEGLBridge] eglMakeCurrent failed: 0x%x", egl.GetError());
		return nullptr;
	}

	return context;
}

void MacEGLBridge::MakeCurrent(bool clear)
{
	if (context == nullptr)
		return;

	std::lock_guard<std::mutex> lck(surfMutex);
	if (clear)
		egl.MakeCurrent(display, nullptr, nullptr, nullptr);
	else
		egl.MakeCurrent(display, surface, surface, context);
}

void MacEGLBridge::DestroyContext()
{
	if (context == nullptr)
		return;

	egl.MakeCurrent(display, surface, surface, context);
	DestroySlots();

	if (texture != nullptr)
		SDL_DestroyTexture(texture);
	if (renderer != nullptr)
		SDL_DestroyRenderer(renderer);

	texture = nullptr;
	renderer = nullptr;
	texSizeX = texSizeY = 0;

	egl.MakeCurrent(display, nullptr, nullptr, nullptr);
	egl.DestroySurface(display, surface);
	egl.DestroyContext(display, context);
	egl.Terminate(display);

	surface = nullptr;
	context = nullptr;
	display = nullptr;
}

void* MacEGLBridge::GetProcAddress(const char* name)
{
	if (!LoadEGL())
		return nullptr;

	return WideLines::GetProcAddress(name, egl.GetProcAddress(name));
}

float MacEGLBridge::GetRenderScale() { return renderScale; }

int MacEGLBridge::ToPixels(int points) { return static_cast<int>(std::lround(points * renderScale)); }

void MacEGLBridge::SetSwapInterval(int interval)
{
	if (renderer != nullptr)
		SDL_RenderSetVSync(renderer, interval != 0);
}

void MacEGLBridge::SwapWindow(SDL_Window* window)
{
	if (context == nullptr || renderer == nullptr)
		return;

	GLint prevPackBuf = 0, prevReadFBO = 0, prevAlign = 4, prevRowLen = 0, prevReadBuf = GL_BACK;
	glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &prevPackBuf);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFBO);
	glGetIntegerv(GL_PACK_ALIGNMENT, &prevAlign);
	glGetIntegerv(GL_PACK_ROW_LENGTH, &prevRowLen);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glGetIntegerv(GL_READ_BUFFER, &prevReadBuf);
	glReadBuffer(GL_BACK);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);

	if (readback == Readback::Unknown)
		ChooseReadback();

	// Asynchronous readback: queue this frame into one slot and present the previous frame's.
	// The flush submits the frame right away, so presenting never waits for the frame in flight.
	const unsigned int cur = frameIdx % NUM_SLOTS;
	const unsigned int prv = (frameIdx + NUM_SLOTS - 1) % NUM_SLOTS;

	if (Slot& s = slots[cur]; s.sizeX != surfSizeX || s.sizeY != surfSizeY) {
		DestroySlot(s);

		if (readback == Readback::Pinned && !CreatePinnedSlot(s, surfSizeX, surfSizeY)) {
			LOG_L(L_WARNING, "[MacEGLBridge] falling back to reading frames back through a copy");
			readback = Readback::Copy;
			DestroySlots();
		}
		if (readback == Readback::Copy)
			CreateCopySlot(s, surfSizeX, surfSizeY);
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, slots[cur].buffer);
	glPixelStorei(GL_PACK_ROW_LENGTH, slots[cur].rowPixels);
	glReadPixels(0, 0, surfSizeX, surfSizeY, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	if (slots[cur].fence != nullptr)
		glDeleteSync(slots[cur].fence);
	slots[cur].fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glFlush();

	SDL_Texture* frame = (readback == Readback::Copy) ? texture : nullptr;

	if (Slot& p = slots[prv]; p.fence != nullptr) {
		glClientWaitSync(p.fence, 0, GL_TIMEOUT_IGNORED);
		glDeleteSync(p.fence);
		p.fence = nullptr;

		if (readback == Readback::Pinned) {
			frame = p.texture;
		} else {
			CopyToTexture(p);
			frame = texture;
		}
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, prevPackBuf);
	glReadBuffer(prevReadBuf);
	glPixelStorei(GL_PACK_ALIGNMENT, prevAlign);
	glPixelStorei(GL_PACK_ROW_LENGTH, prevRowLen);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);

	if (frame != nullptr) {
		SDL_RenderCopyEx(renderer, frame, nullptr, nullptr, 0.0, nullptr, SDL_FLIP_VERTICAL);
		SDL_RenderPresent(renderer);
	}

	frameIdx++;

	// a new MacRenderScale, or a move to a display with a different density, changes the engine's
	// window size in pixels, so send the resize event the engine already handles
	if (const float wanted = WantedRenderScale(window); wanted != renderScale) {
		LOG("[MacEGLBridge] render scale %.2f -> %.2f", renderScale, wanted);
		renderScale = wanted;

		SDL_Event event = {};
		event.type = SDL_WINDOWEVENT;
		event.window.event = SDL_WINDOWEVENT_SIZE_CHANGED;
		event.window.windowID = SDL_GetWindowID(window);
		SDL_GetWindowSize(window, &event.window.data1, &event.window.data2);
		SDL_PushEvent(&event);
	}

	// track window size changes, the pbuffer is the engine's default framebuffer
	int wx = 0, wy = 0;
	SDL_GetWindowSize(window, &wx, &wy);
	wx = ToPixels(wx);
	wy = ToPixels(wy);
	if ((wx != surfSizeX || wy != surfSizeY) && wx > 0 && wy > 0) {
		std::lock_guard<std::mutex> lck(surfMutex);
		LOG("[MacEGLBridge] resizing pbuffer %dx%d -> %dx%d", surfSizeX, surfSizeY, wx, wy);
		CreateSurface(wx, wy);
	}
}

#endif
