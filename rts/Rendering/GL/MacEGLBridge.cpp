/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "MacEGLBridge.h"

#ifdef MAC_EGL_BRIDGE_AVAILABLE

#include <dlfcn.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <mutex>

#include <SDL.h>

#include "Rendering/GL/myGL.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/Platform/Misc.h"

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

	SDL_Renderer* renderer = nullptr;
	SDL_Texture* texture = nullptr;
	int texSizeX = 0;
	int texSizeY = 0;

	constexpr unsigned int NUM_PBOS = 2;
	GLuint pbos[NUM_PBOS] = {};
	GLsync fences[NUM_PBOS] = {};
	int pboSizes[NUM_PBOS][2] = {};
	unsigned int frameIdx = 0;

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

	int wx = 0, wy = 0;
	SDL_GetWindowSize(window, &wx, &wy);
	if (!CreateSurface(wx, wy) || !egl.MakeCurrent(display, surface, surface, context)) {
		LOG_L(L_ERROR, "[MacEGLBridge] eglMakeCurrent failed: 0x%x", egl.GetError());
		return nullptr;
	}

	if (!CreatePresenter(window))
		return nullptr;

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

	if (texture != nullptr)
		SDL_DestroyTexture(texture);
	if (renderer != nullptr)
		SDL_DestroyRenderer(renderer);

	texture = nullptr;
	renderer = nullptr;
	texSizeX = texSizeY = 0;

	egl.MakeCurrent(display, surface, surface, context);
	for (unsigned int i = 0; i < NUM_PBOS; i++) {
		if (fences[i] != nullptr)
			glDeleteSync(fences[i]);
		fences[i] = nullptr;
	}
	if (pbos[0] != 0)
		glDeleteBuffers(NUM_PBOS, pbos);
	std::fill(std::begin(pbos), std::end(pbos), 0);

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

	return egl.GetProcAddress(name);
}

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
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);

	if (pbos[0] == 0)
		glGenBuffers(NUM_PBOS, pbos);

	// asynchronous readback: queue this frame into one PBO and present the oldest one; the flush
	// submits the frame right away so mapping never has to wait for the frame currently in flight
	const unsigned int cur = frameIdx % NUM_PBOS;
	const unsigned int prv = (frameIdx + 1) % NUM_PBOS;

	glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[cur]);
	if (pboSizes[cur][0] != surfSizeX || pboSizes[cur][1] != surfSizeY) {
		glBufferData(GL_PIXEL_PACK_BUFFER, surfSizeX * surfSizeY * 4, nullptr, GL_STREAM_READ);
		pboSizes[cur][0] = surfSizeX;
		pboSizes[cur][1] = surfSizeY;
	}
	glReadPixels(0, 0, surfSizeX, surfSizeY, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	if (fences[cur] != nullptr)
		glDeleteSync(fences[cur]);
	fences[cur] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	glFlush();

	const int px = pboSizes[prv][0];
	const int py = pboSizes[prv][1];

	if (fences[prv] != nullptr && px > 0 && py > 0) {
		glClientWaitSync(fences[prv], 0, GL_TIMEOUT_IGNORED);
		glDeleteSync(fences[prv]);
		fences[prv] = nullptr;

		glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[prv]);
		const void* data = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, px * py * 4, GL_MAP_READ_BIT);

		if (data != nullptr) {
			if (texture == nullptr || texSizeX != px || texSizeY != py) {
				if (texture != nullptr)
					SDL_DestroyTexture(texture);
				texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, px, py);
				texSizeX = px;
				texSizeY = py;
			}

			void* pixels = nullptr;
			int pitch = 0;
			if (texture != nullptr && SDL_LockTexture(texture, nullptr, &pixels, &pitch) == 0) {
				if (pitch == px * 4) {
					memcpy(pixels, data, px * py * 4);
				} else {
					for (int y = 0; y < py; ++y)
						memcpy(static_cast<uint8_t*>(pixels) + y * pitch, static_cast<const uint8_t*>(data) + y * px * 4, px * 4);
				}
				SDL_UnlockTexture(texture);
			}

			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		}
	}

	glBindBuffer(GL_PIXEL_PACK_BUFFER, prevPackBuf);
	glReadBuffer(prevReadBuf);
	glPixelStorei(GL_PACK_ALIGNMENT, prevAlign);
	glPixelStorei(GL_PACK_ROW_LENGTH, prevRowLen);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);

	if (texture != nullptr) {
		SDL_RenderCopyEx(renderer, texture, nullptr, nullptr, 0.0, nullptr, SDL_FLIP_VERTICAL);
		SDL_RenderPresent(renderer);
	}

	frameIdx++;

	// track window size changes; the pbuffer is the engine's default framebuffer
	int wx = 0, wy = 0;
	SDL_GetWindowSize(window, &wx, &wy);
	if ((wx != surfSizeX || wy != surfSizeY) && wx > 0 && wy > 0) {
		std::lock_guard<std::mutex> lck(surfMutex);
		LOG("[MacEGLBridge] resizing pbuffer %dx%d -> %dx%d", surfSizeX, surfSizeY, wx, wy);
		CreateSurface(wx, wy);
	}
}

#endif
