/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RECOIL_SDL_RECOIL_COMPAT_H
#define RECOIL_SDL_RECOIL_COMPAT_H

#if defined(RECOIL_MACOS_SDL3_EGL)

/*
 * The macOS SDL3/EGL runtime keeps Recoil's existing SDL2-shaped source
 * contract at this include boundary. This is a source-compatibility facade:
 * the linked runtime remains SDL3.
 */

#ifndef SDL_ENABLE_OLD_NAMES
#define SDL_ENABLE_OLD_NAMES 1
#endif

#if defined(camera)
#define RECOIL_SDL_COMPAT_RESTORE_CAMERA_MACRO 1
#pragma push_macro("camera")
#undef camera
#endif

#include <SDL3/SDL.h>

#if defined(RECOIL_SDL_COMPAT_RESTORE_CAMERA_MACRO)
#pragma pop_macro("camera")
#undef RECOIL_SDL_COMPAT_RESTORE_CAMERA_MACRO
#endif

typedef struct SDL_version {
	int major;
	int minor;
	int patch;
} SDL_version;

static inline void SDLCompat_VersionFromNumber(int version, SDL_version* outVersion)
{
	if (outVersion == NULL)
		return;

	outVersion->major = version / 1000000;
	outVersion->minor = (version / 1000) % 1000;
	outVersion->patch = version % 1000;
}

static inline void SDLCompat_GetCompiledVersion(SDL_version* outVersion)
{
	if (outVersion == NULL)
		return;

	outVersion->major = SDL_MAJOR_VERSION;
	outVersion->minor = SDL_MINOR_VERSION;
	outVersion->patch = SDL_MICRO_VERSION;
}

static inline void SDLCompat_GetLinkedVersion(SDL_version* outVersion)
{
	SDLCompat_VersionFromNumber(SDL_GetVersion(), outVersion);
}

#undef SDL_VERSION
#define SDL_VERSION(outVersion) SDLCompat_GetCompiledVersion(outVersion)
#define SDL_GetVersion(outVersion) SDLCompat_GetLinkedVersion(outVersion)

#ifndef SDL_BPP
#define SDL_BPP(format) SDL_BITSPERPIXEL(format)
#endif

#ifndef SDL_QUERY
#define SDL_QUERY   -1
#endif
#ifndef SDL_DISABLE
#define SDL_DISABLE  0
#endif
#ifndef SDL_ENABLE
#define SDL_ENABLE   1
#endif
#ifndef SDL_HINT_MOUSE_RELATIVE_MODE_WARP
#define SDL_HINT_MOUSE_RELATIVE_MODE_WARP "SDL_MOUSE_RELATIVE_MODE_WARP"
#endif

static inline SDL_DisplayID SDLCompat_GetDisplayIDFromIndex(int displayIndex)
{
	int count = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&count);
	if (displays == NULL || count <= 0)
		return 0;

	const int boundedIndex = displayIndex < 0 ? 0 : (displayIndex >= count ? count - 1 : displayIndex);
	const SDL_DisplayID displayID = displays[boundedIndex];
	SDL_free(displays);
	return displayID;
}

static inline int SDLCompat_GetNumVideoDisplays()
{
	int count = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&count);
	SDL_free(displays);
	return count;
}

static inline int SDLCompat_GetDisplayBounds(int displayIndex, SDL_Rect* rect)
{
	return SDL_GetDisplayBounds(SDLCompat_GetDisplayIDFromIndex(displayIndex), rect) ? 0 : -1;
}

static inline int SDLCompat_GetDisplayUsableBounds(int displayIndex, SDL_Rect* rect)
{
	return SDL_GetDisplayUsableBounds(SDLCompat_GetDisplayIDFromIndex(displayIndex), rect) ? 0 : -1;
}

static inline const char* SDLCompat_GetDisplayName(int displayIndex)
{
	const char* name = SDL_GetDisplayName(SDLCompat_GetDisplayIDFromIndex(displayIndex));
	return name != NULL ? name : "unknown";
}

static inline int SDLCompat_GetNumDisplayModes(int displayIndex)
{
	int count = 0;
	SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(SDLCompat_GetDisplayIDFromIndex(displayIndex), &count);
	SDL_free(modes);
	return count;
}

static inline int SDLCompat_CopyDisplayMode(const SDL_DisplayMode* mode, SDL_DisplayMode* outMode)
{
	if (mode == NULL || outMode == NULL)
		return -1;

	*outMode = *mode;
	return 0;
}

static inline int SDLCompat_GetDisplayMode(int displayIndex, int modeIndex, SDL_DisplayMode* outMode)
{
	int count = 0;
	SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(SDLCompat_GetDisplayIDFromIndex(displayIndex), &count);
	if (modes == NULL || modeIndex < 0 || modeIndex >= count) {
		SDL_free(modes);
		return -1;
	}

	const int result = SDLCompat_CopyDisplayMode(modes[modeIndex], outMode);
	SDL_free(modes);
	return result;
}

static inline int SDLCompat_GetDesktopDisplayMode(int displayIndex, SDL_DisplayMode* outMode)
{
	return SDLCompat_CopyDisplayMode(SDL_GetDesktopDisplayMode(SDLCompat_GetDisplayIDFromIndex(displayIndex)), outMode);
}

static inline int SDLCompat_GetCurrentDisplayMode(int displayIndex, SDL_DisplayMode* outMode)
{
	return SDLCompat_CopyDisplayMode(SDL_GetCurrentDisplayMode(SDLCompat_GetDisplayIDFromIndex(displayIndex)), outMode);
}

static inline int SDLCompat_GetWindowDisplayMode(SDL_Window* window, SDL_DisplayMode* outMode)
{
	return SDLCompat_CopyDisplayMode(SDL_GetWindowFullscreenMode(window), outMode);
}

static inline int SDLCompat_GetWindowDisplayIndex(SDL_Window* window)
{
	const SDL_DisplayID windowDisplay = SDL_GetDisplayForWindow(window);
	int count = 0;
	SDL_DisplayID* displays = SDL_GetDisplays(&count);
	if (displays == NULL)
		return 0;

	int index = 0;
	for (int i = 0; i < count; ++i) {
		if (displays[i] == windowDisplay) {
			index = i;
			break;
		}
	}

	SDL_free(displays);
	return index;
}

static inline int SDLCompat_SetWindowFullscreen(SDL_Window* window, Uint32 flags)
{
	return SDL_SetWindowFullscreen(window, flags != 0) ? 0 : -1;
}

static inline SDL_Window* SDLCompat_CreateWindow4(const char* title, int w, int h, SDL_WindowFlags flags)
{
	return SDL_CreateWindow(title, w, h, flags);
}

static inline SDL_Window* SDLCompat_CreateWindow6(const char* title, int x, int y, int w, int h, Uint32 flags)
{
	SDL_Window* window = SDL_CreateWindow(title, w, h, (SDL_WindowFlags)flags);
	if (window != NULL)
		SDL_SetWindowPosition(window, x, y);
	return window;
}

static inline void SDLCompat_SetWindowGrab(SDL_Window* window, bool grabbed)
{
	SDL_SetWindowMouseGrab(window, grabbed);
}

static inline bool SDLCompat_GetWindowGrab(SDL_Window* window)
{
	return SDL_GetWindowMouseGrab(window);
}

static inline int SDLCompat_GL_SetSwapInterval(int interval)
{
	return SDL_GL_SetSwapInterval(interval) ? 0 : -1;
}

static inline int SDLCompat_GL_GetSwapInterval()
{
	int interval = 0;
	if (!SDL_GL_GetSwapInterval(&interval))
		return 0;
	return interval;
}

static inline int SDLCompat_ShowCursor(int toggle)
{
	if (toggle == SDL_QUERY)
		return SDL_CursorVisible() ? SDL_ENABLE : SDL_DISABLE;

	const bool result = (toggle == SDL_ENABLE) ? SDL_ShowCursor() : SDL_HideCursor();
	return result ? SDL_ENABLE : SDL_DISABLE;
}

static inline int SDLCompat_PauseAudioDevice(SDL_AudioDeviceID deviceID, int pauseOn)
{
	return (pauseOn ? SDL_PauseAudioDevice(deviceID) : SDL_ResumeAudioDevice(deviceID)) ? 0 : -1;
}

static inline bool SDLCompat_StartTextInput0()
{
	SDL_Window* window = SDL_GetKeyboardFocus();
	return window != NULL ? SDL_StartTextInput(window) : false;
}

static inline bool SDLCompat_StartTextInput1(SDL_Window* window)
{
	return SDL_StartTextInput(window);
}

static inline bool SDLCompat_StopTextInput0()
{
	SDL_Window* window = SDL_GetKeyboardFocus();
	return window != NULL ? SDL_StopTextInput(window) : false;
}

static inline bool SDLCompat_StopTextInput1(SDL_Window* window)
{
	return SDL_StopTextInput(window);
}

static inline bool SDLCompat_SetTextInputRect(const SDL_Rect* rect)
{
	SDL_Window* window = SDL_GetKeyboardFocus();
	return window != NULL ? SDL_SetTextInputArea(window, rect, 0) : false;
}

static inline SDL_Surface* SDLCompat_CreateRGBSurfaceFrom(void* pixels, int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
	const SDL_PixelFormat format = SDL_GetPixelFormatForMasks(depth, Rmask, Gmask, Bmask, Amask);
	if (format == SDL_PIXELFORMAT_UNKNOWN)
		return NULL;

	return SDL_CreateSurfaceFrom(width, height, format, pixels, pitch);
}

static inline SDL_MouseButtonFlags SDLCompat_GetMouseState(int* x, int* y)
{
	float xf = 0.0f;
	float yf = 0.0f;
	const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&xf, &yf);

	if (x != NULL)
		*x = static_cast<int>(xf);
	if (y != NULL)
		*y = static_cast<int>(yf);

	return buttons;
}

static inline SDL_Keycode SDLCompat_GetKeyFromScancode(SDL_Scancode scancode)
{
	return SDL_GetKeyFromScancode(scancode, SDL_KMOD_NONE, false);
}

static inline SDL_Scancode SDLCompat_GetScancodeFromKey(SDL_Keycode keycode)
{
	SDL_Keymod modstate = SDL_KMOD_NONE;
	return SDL_GetScancodeFromKey(keycode, &modstate);
}

#undef SDL_GetNumVideoDisplays
#undef SDL_GetDisplayBounds
#undef SDL_GetDisplayUsableBounds
#undef SDL_GetDisplayName
#undef SDL_GetNumDisplayModes
#undef SDL_GetDisplayMode
#undef SDL_GetDesktopDisplayMode
#undef SDL_GetCurrentDisplayMode
#undef SDL_GetWindowDisplayMode
#undef SDL_GetWindowDisplayIndex
#undef SDL_CreateWindow
#undef SDL_SetWindowFullscreen
#undef SDL_SetWindowGrab
#undef SDL_GetWindowGrab
#undef SDL_GL_SetSwapInterval
#undef SDL_GL_GetSwapInterval
#undef SDL_ShowCursor
#undef SDL_PauseAudioDevice
#undef SDL_StartTextInput
#undef SDL_StopTextInput
#undef SDL_SetTextInputRect
#undef SDL_CreateRGBSurfaceFrom
#undef SDL_GetMouseState
#undef SDL_GetKeyFromScancode
#undef SDL_GetScancodeFromKey

#define SDL_GetNumVideoDisplays() SDLCompat_GetNumVideoDisplays()
#define SDL_GetDisplayBounds(displayIndex, rect) SDLCompat_GetDisplayBounds(displayIndex, rect)
#define SDL_GetDisplayUsableBounds(displayIndex, rect) SDLCompat_GetDisplayUsableBounds(displayIndex, rect)
#define SDL_GetDisplayName(displayIndex) SDLCompat_GetDisplayName(displayIndex)
#define SDL_GetNumDisplayModes(displayIndex) SDLCompat_GetNumDisplayModes(displayIndex)
#define SDL_GetDisplayMode(displayIndex, modeIndex, outMode) SDLCompat_GetDisplayMode(displayIndex, modeIndex, outMode)
#define SDL_GetDesktopDisplayMode(displayIndex, outMode) SDLCompat_GetDesktopDisplayMode(displayIndex, outMode)
#define SDL_GetCurrentDisplayMode(displayIndex, outMode) SDLCompat_GetCurrentDisplayMode(displayIndex, outMode)
#define SDL_GetWindowDisplayMode(window, outMode) SDLCompat_GetWindowDisplayMode(window, outMode)
#define SDL_GetWindowDisplayIndex(window) SDLCompat_GetWindowDisplayIndex(window)
#define SDLCompat_CreateWindowSelect(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define SDL_CreateWindow(...) SDLCompat_CreateWindowSelect(__VA_ARGS__, SDLCompat_CreateWindow6, SDLCompat_CreateWindow5_unsupported, SDLCompat_CreateWindow4)(__VA_ARGS__)
#define SDL_SetWindowFullscreen(window, flags) SDLCompat_SetWindowFullscreen(window, flags)
#define SDL_SetWindowGrab(window, grabbed) SDLCompat_SetWindowGrab(window, grabbed)
#define SDL_GetWindowGrab(window) SDLCompat_GetWindowGrab(window)
#define SDL_GL_SetSwapInterval(interval) SDLCompat_GL_SetSwapInterval(interval)
#define SDL_GL_GetSwapInterval() SDLCompat_GL_GetSwapInterval()
#define SDL_ShowCursor(toggle) SDLCompat_ShowCursor(toggle)
#define SDL_PauseAudioDevice(deviceID, pauseOn) SDLCompat_PauseAudioDevice(deviceID, pauseOn)
#define SDLCompat_TextInputSelect(_0, _1, NAME, ...) NAME
#define SDL_StartTextInput(...) SDLCompat_TextInputSelect(_, ##__VA_ARGS__, SDLCompat_StartTextInput1, SDLCompat_StartTextInput0)(__VA_ARGS__)
#define SDL_StopTextInput(...) SDLCompat_TextInputSelect(_, ##__VA_ARGS__, SDLCompat_StopTextInput1, SDLCompat_StopTextInput0)(__VA_ARGS__)
#define SDL_SetTextInputRect(rect) SDLCompat_SetTextInputRect(rect)
#define SDL_CreateRGBSurfaceFrom(pixels, width, height, depth, pitch, Rmask, Gmask, Bmask, Amask) SDLCompat_CreateRGBSurfaceFrom(pixels, width, height, depth, pitch, Rmask, Gmask, Bmask, Amask)
#define SDL_GetMouseState(x, y) SDLCompat_GetMouseState(x, y)
#define SDL_GetKeyFromScancode(scancode) SDLCompat_GetKeyFromScancode(scancode)
#define SDL_GetScancodeFromKey(keycode) SDLCompat_GetScancodeFromKey(keycode)

#else
#include <SDL.h>
#endif

#endif // RECOIL_SDL_RECOIL_COMPAT_H
