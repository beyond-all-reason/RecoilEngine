/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SDL2_WM_COMPAT_H
#define SDL2_WM_COMPAT_H

/*
 * SDL2 -> SDL3 window-manager-info compatibility shim.
 *
 * SDL3 removed <SDL_syswm.h>, the SDL_SysWMinfo struct, the SDL_SYSWM_* enum
 * and SDL_GetWindowWMInfo(). Native handles are now obtained through the
 * window-properties API (SDL_GetWindowProperties + SDL_Get*Property).
 *
 * The engine only ever read a handful of fields (the X11 display/window, the
 * Win32 HWND and the active subsystem), so this header re-exports just those
 * under the historical names. Because SDL3 no longer defines any of these
 * symbols, redefining them here is conflict-free.
 *
 * Call sites change in only two ways:
 *   - include this header instead of <SDL_syswm.h>
 *   - drop the old `SDL_VERSION(&info.version);` line (SDL_VERSION is now a
 *     plain version number in SDL3, not a macro taking a pointer)
 */

#include <SDL_video.h>
#include <SDL_properties.h>
#include <SDL_stdinc.h>

enum SDL_SYSWM_TYPE {
	SDL_SYSWM_UNKNOWN = 0,
	SDL_SYSWM_X11,
	SDL_SYSWM_WAYLAND,
	SDL_SYSWM_WINDOWS,
	SDL_SYSWM_COCOA,
};

struct SDL_SysWMinfo {
	SDL_SYSWM_TYPE subsystem;

	union {
		struct {
			void*         display; // Display*
			unsigned long window;  // ::Window (XID)
		} x11;
		struct {
			void* display;         // wl_display*
			void* surface;         // wl_surface*
		} wl;
		struct {
			void* window;          // HWND
		} win;
		struct {
			void* window;          // NSWindow*
		} cocoa;
	} info;
};

/* Drop-in replacement for SDL2's SDL_GetWindowWMInfo(); returns true on success. */
static inline bool SDL_GetWindowWMInfo(SDL_Window* window, SDL_SysWMinfo* info)
{
	if (window == nullptr || info == nullptr)
		return false;

	*info = SDL_SysWMinfo{};

	const char* driver = SDL_GetCurrentVideoDriver();
	const SDL_PropertiesID props = SDL_GetWindowProperties(window);

	if (driver == nullptr || props == 0)
		return false;

	if (SDL_strcmp(driver, "x11") == 0) {
		info->subsystem = SDL_SYSWM_X11;
		info->info.x11.display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
		info->info.x11.window  = (unsigned long)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
		return true;
	}

	if (SDL_strcmp(driver, "wayland") == 0) {
		info->subsystem = SDL_SYSWM_WAYLAND;
		info->info.wl.display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
		info->info.wl.surface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
		return true;
	}

	if (SDL_strcmp(driver, "windows") == 0) {
		info->subsystem = SDL_SYSWM_WINDOWS;
		info->info.win.window = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
		return true;
	}

	if (SDL_strcmp(driver, "cocoa") == 0) {
		info->subsystem = SDL_SYSWM_COCOA;
		info->info.cocoa.window = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
		return true;
	}

	return false;
}

#endif // SDL2_WM_COMPAT_H
