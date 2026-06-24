/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/Mac/MacSDL3EGLBridge.h"

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <SDL.h>
#include <SDL3/SDL_properties.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "System/Log/ILog.h"

namespace {

static constexpr uint32_t BAR_ON_MVK_COCOA_EGL_SURFACE_MAGIC = 0x434f4341u;
static constexpr uint32_t BAR_ON_MVK_COCOA_EGL_SURFACE_VERSION = 1u;

struct BarOnMVKPrivateCocoaSurface {
	uint32_t magic = BAR_ON_MVK_COCOA_EGL_SURFACE_MAGIC;
	uint32_t version = BAR_ON_MVK_COCOA_EGL_SURFACE_VERSION;
	void* metalLayer = nullptr;
	uint32_t width = 0;
	uint32_t height = 0;
};

using PFNGLGETSTRINGPROC_LOCAL = const unsigned char* (*)(unsigned int name);

static constexpr unsigned int GL_VENDOR_LOCAL = 0x1F00;
static constexpr unsigned int GL_RENDERER_LOCAL = 0x1F01;
static constexpr unsigned int GL_VERSION_LOCAL = 0x1F02;
static constexpr unsigned int GL_SHADING_LANGUAGE_VERSION_LOCAL = 0x8B8C;

static const char* EnvValue(const char* name)
{
	const char* value = std::getenv(name);
	return value != nullptr ? value : "<unset>";
}

static bool EnvEnabled(const char* name)
{
	const char* value = std::getenv(name);
	return value != nullptr && value[0] != '\0' && std::strcmp(value, "0") != 0;
}

static float CocoaBackingScale(NSWindow* window)
{
	if (window != nil && window.backingScaleFactor > 0.0)
		return static_cast<float>(window.backingScaleFactor);

	if (NSScreen.mainScreen != nil && NSScreen.mainScreen.backingScaleFactor > 0.0)
		return static_cast<float>(NSScreen.mainScreen.backingScaleFactor);

	return 1.0f;
}

static NSWindow* GetNSWindow(SDL_Window* window)
{
	if (window == nullptr)
		return nil;

	SDL_PropertiesID props = SDL_GetWindowProperties(window);
	return (__bridge NSWindow*)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
}

static CGFloat NativeCocoaDrawableScale(NSWindow* nsWindow)
{
	const char* scaleEnv = std::getenv("BAR_NATIVE_COCOA_DRAWABLE_SCALE");
	if (scaleEnv != nullptr && scaleEnv[0] != '\0') {
		if (std::strcmp(scaleEnv, "native") == 0 || std::strcmp(scaleEnv, "retina") == 0)
			return CocoaBackingScale(nsWindow);

		char* endPtr = nullptr;
		const float requestedScale = std::strtof(scaleEnv, &endPtr);
		if (endPtr != scaleEnv && std::isfinite(requestedScale) && requestedScale > 0.0f)
			return static_cast<CGFloat>(requestedScale);
	}

	return 1.0;
}

static CGSize CurrentLayerDrawableSize(NSWindow* nsWindow)
{
	if (nsWindow == nil || nsWindow.contentView == nil || nsWindow.contentView.layer == nil)
		return CGSizeMake(0.0, 0.0);

	CALayer* layer = nsWindow.contentView.layer;
	if (![layer isKindOfClass:[CAMetalLayer class]])
		return CGSizeMake(0.0, 0.0);

	return ((CAMetalLayer*)layer).drawableSize;
}

static CGSize UpdateLayerDrawableSize(NSWindow* nsWindow, NSView* view, CAMetalLayer* layer, const char* reason)
{
	const CGFloat cocoaScale = CocoaBackingScale(nsWindow);
	const CGFloat drawableScale = NativeCocoaDrawableScale(nsWindow);
	const NSRect bounds = view != nil ? view.bounds : NSMakeRect(0, 0, 0, 0);
	const CGSize drawableSize = CGSizeMake(bounds.size.width * drawableScale, bounds.size.height * drawableScale);
	const CGSize previousDrawableSize = layer.drawableSize;
	const CGFloat previousScale = layer.contentsScale;
	const bool sizeChanged =
		std::abs(static_cast<double>(previousDrawableSize.width - drawableSize.width)) > 0.5 ||
		std::abs(static_cast<double>(previousDrawableSize.height - drawableSize.height)) > 0.5 ||
		std::abs(static_cast<double>(previousScale - drawableScale)) > 0.001;
	const bool quietPreSwap = (reason != nullptr && std::strcmp(reason, "pre-swap") == 0);

	layer.contentsScale = drawableScale;
	if (sizeChanged)
		layer.drawableSize = drawableSize;

	if (sizeChanged || !quietPreSwap) {
		LOG("[MacSDL3EGL::%s] reason=%s logicalView=%.1fx%.1f drawable=%.1fx%.1f cocoaScale=%.3f drawableScale=%.3f changed=%d",
			__func__, reason,
			static_cast<double>(bounds.size.width),
			static_cast<double>(bounds.size.height),
			static_cast<double>(drawableSize.width),
			static_cast<double>(drawableSize.height),
			static_cast<double>(cocoaScale),
			static_cast<double>(drawableScale),
			static_cast<int>(sizeChanged)
		);
	}

	return drawableSize;
}

static const char* PixelFormatName(MTLPixelFormat pixelFormat)
{
	switch (pixelFormat) {
		case MTLPixelFormatBGRA8Unorm: return "MTLPixelFormatBGRA8Unorm";
		case MTLPixelFormatBGRA8Unorm_sRGB: return "MTLPixelFormatBGRA8Unorm_sRGB";
		default: return "unknown";
	}
}

static EGLContext CreateContext(EGLDisplay display, EGLConfig config, int major, int minor, bool coreProfile)
{
	const EGLint contextAttribs[] = {
		EGL_CONTEXT_MAJOR_VERSION, major,
		EGL_CONTEXT_MINOR_VERSION, minor,
		EGL_NONE
	};

	EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
	if (context != EGL_NO_CONTEXT) {
		LOG("[MacSDL3EGL::%s] eglCreateContext requested=%d.%d core=%d result=success",
			__func__, major, minor, int(coreProfile));
		return context;
	}

	LOG_L(L_WARNING, "[MacSDL3EGL::%s] eglCreateContext requested=%d.%d core=%d failed eglGetError=0x%04x; trying default context",
		__func__, major, minor, int(coreProfile), eglGetError());

	const EGLint fallbackAttribs[] = {EGL_NONE};
	context = eglCreateContext(display, config, EGL_NO_CONTEXT, fallbackAttribs);
	if (context != EGL_NO_CONTEXT) {
		LOG("[MacSDL3EGL::%s] eglCreateContext requested=fallback result=success", __func__);
	} else {
		LOG_L(L_ERROR, "[MacSDL3EGL::%s] eglCreateContext requested=fallback failed eglGetError=0x%04x", __func__, eglGetError());
	}

	return context;
}

static bool ChooseConfig(EGLDisplay display, EGLConfig* outConfig)
{
	const EGLint preferredAttribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};

	EGLint count = 0;
	if (eglChooseConfig(display, preferredAttribs, outConfig, 1, &count) && count > 0) {
		LOG("[MacSDL3EGL::%s] eglChooseConfig depth=24 stencil=8 count=%d", __func__, count);
		return true;
	}

	LOG_L(L_WARNING, "[MacSDL3EGL::%s] preferred eglChooseConfig failed count=%d eglGetError=0x%04x; trying color-only config",
		__func__, count, eglGetError());

	const EGLint fallbackAttribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 0,
		EGL_STENCIL_SIZE, 0,
		EGL_NONE
	};

	count = 0;
	if (eglChooseConfig(display, fallbackAttribs, outConfig, 1, &count) && count > 0) {
		LOG("[MacSDL3EGL::%s] eglChooseConfig depth=0 stencil=0 count=%d", __func__, count);
		return true;
	}

	LOG_L(L_ERROR, "[MacSDL3EGL::%s] eglChooseConfig failed count=%d eglGetError=0x%04x", __func__, count, eglGetError());
	return false;
}

static bool LooksLikeM4Pro14Panel(const MacSDL3EGL::DisplayGeometry& geometry)
{
	const bool logical1512 = geometry.displayBounds.w == 1512 || geometry.usableBounds.w == 1512;
	const bool targetHeight = geometry.displayBounds.h == 982 || geometry.displayBounds.h == 950 || geometry.usableBounds.h == 950;
	const bool retinaScale = geometry.sdlScale >= 1.9f || geometry.cocoaScale >= 1.9f;
	const char* name = geometry.displayName != nullptr ? geometry.displayName : "";
	const bool builtinName =
		std::strstr(name, "Built-in") != nullptr ||
		std::strstr(name, "Liquid Retina") != nullptr ||
		std::strstr(name, "Color LCD") != nullptr;

	return logical1512 && targetHeight && retinaScale && builtinName;
}

static int DetectDisplayIndexFromMouse()
{
	const int numDisplays = SDL_GetNumVideoDisplays();
	if (numDisplays <= 1)
		return 0;

	float mouseX = 0.0f;
	float mouseY = 0.0f;
	SDL_GetGlobalMouseState(&mouseX, &mouseY);

	const SDL_Point mousePoint{static_cast<int>(mouseX), static_cast<int>(mouseY)};
	SDL_Rect bounds;

	for (int displayIndex = 0; displayIndex < numDisplays; ++displayIndex) {
		if (SDL_GetDisplayBounds(displayIndex, &bounds) != 0)
			continue;
		if (SDL_PointInRect(&mousePoint, &bounds))
			return displayIndex;
	}

	return 0;
}

static void ApplyNotchPolicy(MacSDL3EGL::DisplayGeometry* geometry)
{
	geometry->safeBounds.x = geometry->displayBounds.x;
	geometry->safeBounds.w = 1512;
	geometry->safeBounds.h = 950;
	geometry->safeBounds.y = geometry->displayBounds.y + std::max(32, geometry->displayBounds.h - geometry->safeBounds.h);
	geometry->m4Pro14NotchPolicy = true;
}

} // namespace

namespace MacSDL3EGL {

struct Bridge::Impl {
	SDL_Window* window = nullptr;
	NSWindow* nsWindow = nil;
	NSView* contentView = nil;
	CAMetalLayer* metalLayer = nil;
	id<MTLDevice> metalDevice = nil;

	BarOnMVKPrivateCocoaSurface privateSurface;

	EGLDisplay display = EGL_NO_DISPLAY;
	EGLConfig config = nullptr;
	EGLContext context = EGL_NO_CONTEXT;
	EGLSurface surface = EGL_NO_SURFACE;
	bool initialized = false;
};

DisplayGeometry QueryDisplayGeometry(SDL_Window* window)
{
	DisplayGeometry geometry;

	geometry.displayIndex = window != nullptr ? SDL_GetWindowDisplayIndex(window) : DetectDisplayIndexFromMouse();
	geometry.displayName = SDL_GetDisplayName(geometry.displayIndex);
	SDL_GetDisplayBounds(geometry.displayIndex, &geometry.displayBounds);
	SDL_GetDisplayUsableBounds(geometry.displayIndex, &geometry.usableBounds);
	geometry.safeBounds = geometry.usableBounds.w > 0 && geometry.usableBounds.h > 0 ? geometry.usableBounds : geometry.displayBounds;

	if (window != nullptr) {
		SDL_GetWindowSize(window, &geometry.logicalWindowWidth, &geometry.logicalWindowHeight);
		SDL_GetWindowSizeInPixels(window, &geometry.drawableWidth, &geometry.drawableHeight);
		geometry.sdlScale = SDL_GetWindowDisplayScale(window);
		NSWindow* nsWindow = GetNSWindow(window);
		geometry.cocoaScale = CocoaBackingScale(nsWindow);

		const CGSize layerDrawable = CurrentLayerDrawableSize(nsWindow);
		if (layerDrawable.width > 0.0 && layerDrawable.height > 0.0) {
			geometry.drawableWidth = static_cast<int>(std::max(0.0, static_cast<double>(layerDrawable.width)));
			geometry.drawableHeight = static_cast<int>(std::max(0.0, static_cast<double>(layerDrawable.height)));
		} else {
			const CGFloat drawableScale = NativeCocoaDrawableScale(nsWindow);
			geometry.drawableWidth = static_cast<int>(std::max(0.0, static_cast<double>(geometry.logicalWindowWidth * drawableScale)));
			geometry.drawableHeight = static_cast<int>(std::max(0.0, static_cast<double>(geometry.logicalWindowHeight * drawableScale)));
		}
	}

	geometry.m4Pro14NotchPolicyForced = EnvEnabled("RECOIL_MACOS_FORCE_M4_14_NOTCH_POLICY");
	if (geometry.m4Pro14NotchPolicyForced || LooksLikeM4Pro14Panel(geometry))
		ApplyNotchPolicy(&geometry);

	return geometry;
}

SDL_Rect GetSafeDisplayBounds(SDL_Window* window)
{
	return QueryDisplayGeometry(window).safeBounds;
}

void LogDisplayGeometry(SDL_Window* window, const char* phase)
{
	const DisplayGeometry g = QueryDisplayGeometry(window);
	LOG("[MacSDL3EGL::%s] phase=%s displayIndex=%d displayName=\"%s\" displayBounds=%d,%d %dx%d usableBounds=%d,%d %dx%d safeBounds=%d,%d %dx%d m4Pro14NotchPolicy=%d forced=%d logicalWindow=%dx%d drawable=%dx%d sdlScale=%.3f cocoaScale=%.3f fixedPolicy=top64Physical_top32Logical_safe1512x950_fillBottomUp",
		__func__, phase,
		g.displayIndex,
		g.displayName != nullptr ? g.displayName : "unknown",
		g.displayBounds.x, g.displayBounds.y, g.displayBounds.w, g.displayBounds.h,
		g.usableBounds.x, g.usableBounds.y, g.usableBounds.w, g.usableBounds.h,
		g.safeBounds.x, g.safeBounds.y, g.safeBounds.w, g.safeBounds.h,
		int(g.m4Pro14NotchPolicy),
		int(g.m4Pro14NotchPolicyForced),
		g.logicalWindowWidth, g.logicalWindowHeight,
		g.drawableWidth, g.drawableHeight,
		g.sdlScale,
		g.cocoaScale
	);
}

void* GetGLProcAddress(const char* name)
{
	return reinterpret_cast<void*>(eglGetProcAddress(name));
}

Bridge::Bridge()
	: impl(new Impl())
{
}

Bridge::~Bridge()
{
	Destroy();
	delete impl;
	impl = nullptr;
}

bool Bridge::Initialize(SDL_Window* window, int requestMajor, int requestMinor, bool requestCoreProfile)
{
	if (impl->initialized)
		return true;

	impl->window = window;
	LogRuntimeEnvironment();

	if (std::getenv("EGL_PLATFORM") == nullptr) {
		setenv("EGL_PLATFORM", "cocoa", 0);
		LOG("[MacSDL3EGL::%s] EGL_PLATFORM was unset; set to cocoa for project-local Mesa bridge", __func__);
	}

	impl->nsWindow = GetNSWindow(window);
	if (impl->nsWindow == nil || impl->nsWindow.contentView == nil) {
		LOG_L(L_ERROR, "[MacSDL3EGL::%s] SDL Cocoa NSWindow/contentView unavailable via %s",
			__func__, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER);
		return false;
	}

	impl->contentView = impl->nsWindow.contentView;
	impl->metalDevice = MTLCreateSystemDefaultDevice();
	if (impl->metalDevice == nil) {
		LOG_L(L_ERROR, "[MacSDL3EGL::%s] MTLCreateSystemDefaultDevice returned nil", __func__);
		return false;
	}

	impl->metalLayer = [[CAMetalLayer layer] retain];
	impl->metalLayer.device = impl->metalDevice;
	impl->metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
	impl->metalLayer.framebufferOnly = YES;
	impl->metalLayer.opaque = YES;
	impl->metalLayer.displaySyncEnabled = YES;
	impl->metalLayer.presentsWithTransaction = NO;
	impl->metalLayer.delegate = static_cast<id<CALayerDelegate>>(impl->contentView);
	impl->contentView.wantsLayer = YES;
	impl->contentView.layer = impl->metalLayer;

	UpdateDrawableSize("initial-owned-layer");
	LOG("[MacSDL3EGL::%s] cocoa nswindow=%p nsview=%p metalDevice=\"%s\" cametallayer=%p pixelFormat=%s(%lu) appOwned=true",
		__func__,
		(__bridge void*)impl->nsWindow,
		(__bridge void*)impl->contentView,
		impl->metalDevice.name.UTF8String,
		(__bridge void*)impl->metalLayer,
		PixelFormatName(impl->metalLayer.pixelFormat),
		static_cast<unsigned long>(impl->metalLayer.pixelFormat)
	);

	impl->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (impl->display == EGL_NO_DISPLAY) {
		LOG_L(L_ERROR, "[MacSDL3EGL::%s] eglGetDisplay(EGL_DEFAULT_DISPLAY) failed eglGetError=0x%04x", __func__, eglGetError());
		return false;
	}

	EGLint eglMajor = 0;
	EGLint eglMinor = 0;
	if (!eglInitialize(impl->display, &eglMajor, &eglMinor)) {
		LOG_L(L_ERROR, "[MacSDL3EGL::%s] eglInitialize failed eglGetError=0x%04x", __func__, eglGetError());
		return false;
	}

	LOG("[MacSDL3EGL::%s] EGL_VERSION=%d.%d EGL_VENDOR=\"%s\" EGL_VERSION_STRING=\"%s\"",
		__func__, eglMajor, eglMinor,
		eglQueryString(impl->display, EGL_VENDOR),
		eglQueryString(impl->display, EGL_VERSION)
	);

	if (!eglBindAPI(EGL_OPENGL_API)) {
		LOG_L(L_ERROR, "[MacSDL3EGL::%s] eglBindAPI(EGL_OPENGL_API) failed eglGetError=0x%04x", __func__, eglGetError());
		return false;
	}

	if (!ChooseConfig(impl->display, &impl->config))
		return false;

	impl->context = CreateContext(impl->display, impl->config, requestMajor, requestMinor, requestCoreProfile);
	if (impl->context == EGL_NO_CONTEXT)
		return false;

	impl->surface = eglCreateWindowSurface(
		impl->display,
		impl->config,
		reinterpret_cast<EGLNativeWindowType>(&impl->privateSurface),
		nullptr
	);

	if (impl->surface == EGL_NO_SURFACE) {
		LOG_L(L_ERROR, "[MacSDL3EGL::%s] eglCreateWindowSurface target=BarOnMVKPrivateCocoaSurface failed eglGetError=0x%04x",
			__func__, eglGetError());
		return false;
	}

	LOG("[MacSDL3EGL::%s] eglCreateWindowSurface target=BarOnMVKPrivateCocoaSurface surface=%p privateSurface=%p magic=0x%08x version=%u layer=%p size=%ux%u",
		__func__,
		impl->surface,
		&impl->privateSurface,
		impl->privateSurface.magic,
		impl->privateSurface.version,
		impl->privateSurface.metalLayer,
		impl->privateSurface.width,
		impl->privateSurface.height
	);

	if (!MakeCurrent(false))
		return false;

	impl->initialized = true;
	LogContextDiagnostics("after-egl-current-before-glad");
	LogGeometry("after-egl-current");
	return true;
}

void Bridge::Destroy()
{
	if (impl == nullptr)
		return;

	if (impl->display != EGL_NO_DISPLAY)
		eglMakeCurrent(impl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	if (impl->display != EGL_NO_DISPLAY && impl->surface != EGL_NO_SURFACE) {
		eglDestroySurface(impl->display, impl->surface);
		impl->surface = EGL_NO_SURFACE;
	}

	if (impl->display != EGL_NO_DISPLAY && impl->context != EGL_NO_CONTEXT) {
		eglDestroyContext(impl->display, impl->context);
		impl->context = EGL_NO_CONTEXT;
	}

	if (impl->display != EGL_NO_DISPLAY) {
		eglTerminate(impl->display);
		impl->display = EGL_NO_DISPLAY;
	}

	if (impl->contentView != nil && impl->contentView.layer == impl->metalLayer)
		impl->contentView.layer = nil;

	if (impl->metalLayer != nil) {
		[impl->metalLayer release];
		impl->metalLayer = nil;
	}

	if (impl->metalDevice != nil) {
		[impl->metalDevice release];
		impl->metalDevice = nil;
	}

	impl->privateSurface = {};
	impl->window = nullptr;
	impl->nsWindow = nil;
	impl->contentView = nil;
	impl->initialized = false;
}

bool Bridge::MakeCurrent(bool clear) const
{
	if (impl->display == EGL_NO_DISPLAY)
		return false;

	const EGLSurface draw = clear ? EGL_NO_SURFACE : impl->surface;
	const EGLSurface read = clear ? EGL_NO_SURFACE : impl->surface;
	const EGLContext context = clear ? EGL_NO_CONTEXT : impl->context;

	if (eglMakeCurrent(impl->display, draw, read, context))
		return true;

	LOG_L(L_ERROR, "[MacSDL3EGL::%s] eglMakeCurrent(clear=%d) failed eglGetError=0x%04x", __func__, int(clear), eglGetError());
	return false;
}

bool Bridge::SwapBuffers()
{
	UpdateDrawableSize("pre-swap");
	if (eglSwapBuffers(impl->display, impl->surface))
		return true;

	LOG_L(L_ERROR, "[MacSDL3EGL::%s] eglSwapBuffers failed eglGetError=0x%04x", __func__, eglGetError());
	return false;
}

void Bridge::UpdateDrawableSize(const char* reason)
{
	if (impl->metalLayer == nil)
		return;

	const CGSize drawable = UpdateLayerDrawableSize(impl->nsWindow, impl->contentView, impl->metalLayer, reason);
	impl->privateSurface.magic = BAR_ON_MVK_COCOA_EGL_SURFACE_MAGIC;
	impl->privateSurface.version = BAR_ON_MVK_COCOA_EGL_SURFACE_VERSION;
	impl->privateSurface.metalLayer = (__bridge void*)impl->metalLayer;
	impl->privateSurface.width = static_cast<uint32_t>(std::max(0.0, static_cast<double>(drawable.width)));
	impl->privateSurface.height = static_cast<uint32_t>(std::max(0.0, static_cast<double>(drawable.height)));
}

void* Bridge::GetContextOpaque() const
{
	return impl->context;
}

bool Bridge::IsInitialized() const
{
	return impl->initialized;
}

int Bridge::GetSwapInterval() const
{
	return SDL_GL_GetSwapInterval();
}

uint64_t Bridge::GetRecommendedMaxWorkingSetSizeBytes() const
{
	if (impl == nullptr || impl->metalDevice == nil)
		return 0;

	return static_cast<uint64_t>(impl->metalDevice.recommendedMaxWorkingSetSize);
}

void Bridge::LogRuntimeEnvironment() const
{
	LOG("[MacSDL3EGL::%s] SDL3 Cocoa + Mesa EGL runtime env VK_DRIVER_FILES=%s MESA_LOADER_DRIVER_OVERRIDE=%s GALLIUM_DRIVER=%s EGL_PLATFORM=%s MESA_GL_VERSION_OVERRIDE=%s MESA_GLSL_VERSION_OVERRIDE=%s VK_ICD_FILENAMES=%s VK_ADD_DRIVER_FILES=%s DYLD_LIBRARY_PATH=%s DYLD_FALLBACK_LIBRARY_PATH=%s",
		__func__,
		EnvValue("VK_DRIVER_FILES"),
		EnvValue("MESA_LOADER_DRIVER_OVERRIDE"),
		EnvValue("GALLIUM_DRIVER"),
		EnvValue("EGL_PLATFORM"),
		EnvValue("MESA_GL_VERSION_OVERRIDE"),
		EnvValue("MESA_GLSL_VERSION_OVERRIDE"),
		EnvValue("VK_ICD_FILENAMES"),
		EnvValue("VK_ADD_DRIVER_FILES"),
		EnvValue("DYLD_LIBRARY_PATH"),
		EnvValue("DYLD_FALLBACK_LIBRARY_PATH")
	);
	LOG("[MacSDL3EGL::%s] nullDescriptorBypassRisk=present_feasibility_only privateCocoaEGLBridge=BarOnMVKPrivateCocoaSurface", __func__);
}

void Bridge::LogContextDiagnostics(const char* phase) const
{
	if (impl->display != EGL_NO_DISPLAY) {
		LOG("[MacSDL3EGL::%s] phase=%s EGL_VENDOR=\"%s\" EGL_VERSION=\"%s\" EGL_EXTENSIONS=\"%s\"",
			__func__, phase,
			eglQueryString(impl->display, EGL_VENDOR),
			eglQueryString(impl->display, EGL_VERSION),
			eglQueryString(impl->display, EGL_EXTENSIONS)
		);
	}

	PFNGLGETSTRINGPROC_LOCAL glGetStringLocal =
		reinterpret_cast<PFNGLGETSTRINGPROC_LOCAL>(eglGetProcAddress("glGetString"));
	if (glGetStringLocal == nullptr)
		return;

	LOG("[MacSDL3EGL::%s] phase=%s GL_VENDOR=\"%s\" GL_RENDERER=\"%s\" GL_VERSION=\"%s\" GL_SHADING_LANGUAGE_VERSION=\"%s\"",
		__func__, phase,
		reinterpret_cast<const char*>(glGetStringLocal(GL_VENDOR_LOCAL)),
		reinterpret_cast<const char*>(glGetStringLocal(GL_RENDERER_LOCAL)),
		reinterpret_cast<const char*>(glGetStringLocal(GL_VERSION_LOCAL)),
		reinterpret_cast<const char*>(glGetStringLocal(GL_SHADING_LANGUAGE_VERSION_LOCAL))
	);
}

void Bridge::LogGeometry(const char* phase) const
{
	LogDisplayGeometry(impl->window, phase);
}

} // namespace MacSDL3EGL
