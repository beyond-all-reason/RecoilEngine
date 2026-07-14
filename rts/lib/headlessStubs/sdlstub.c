/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * Headless SDL stub library.
 *
 * The headless / dedicated-server builds do not link a real SDL; they link
 * these no-op stubs instead so the engine can be built and run without a
 * graphics stack. The signatures below must match SDL3 exactly because the
 * SDL3 headers are force-included into this TU (see FindSDL2.cmake); any
 * mismatch is a "conflicting types" compile error.
 *
 * Note on return values: SDL3 flipped most int-returning functions to bool
 * where true == success. Stubs that the engine treats as "did it work?" return
 * true; state queries (cursor visible, window grabbed, ...) return false.
 *
 * documentation: https://wiki.libsdl.org/SDL3/
 */

#include <SDL.h>


#ifdef __cplusplus
extern "C" {
#endif


static SDL_Surface     stubSurface;
static bool            stubKeyState[1];
static SDL_DisplayID   stubDisplays[1] = { 1 };
static SDL_DisplayMode stubDisplayMode;
static Uint32          stubSubSystemsInit = 0;


/* --- init / teardown ----------------------------------------------------- */

SDL_DECLSPEC void SDLCALL SDL_free(void* p) { (void)p; }

SDL_DECLSPEC bool SDLCALL SDL_Init(SDL_InitFlags flags) {
	stubSurface.w = 512;
	stubSurface.h = 512;
	stubSubSystemsInit = flags;
	return true;
}

SDL_DECLSPEC bool SDLCALL SDL_InitSubSystem(SDL_InitFlags flags) { stubSubSystemsInit |= flags; return true; }
SDL_DECLSPEC void SDLCALL SDL_QuitSubSystem(SDL_InitFlags flags) { stubSubSystemsInit &= ~flags; }
SDL_DECLSPEC SDL_InitFlags SDLCALL SDL_WasInit(SDL_InitFlags flags) { return (stubSubSystemsInit & flags); }
SDL_DECLSPEC void SDLCALL SDL_Quit(void) {}

SDL_DECLSPEC const char* SDLCALL SDL_GetError(void) { return "using the SDL stub library"; }

SDL_DECLSPEC bool SDLCALL SDL_SetHint(const char* name, const char* value) { (void)name; (void)value; return true; }

SDL_DECLSPEC bool SDLCALL SDL_EnableScreenSaver(void) { return true; }
SDL_DECLSPEC bool SDLCALL SDL_DisableScreenSaver(void) { return true; }


/* --- version ------------------------------------------------------------- */

SDL_DECLSPEC int SDLCALL SDL_GetVersion(void) { return SDL_VERSION; }

SDL_DECLSPEC const char* SDLCALL SDL_GetCurrentVideoDriver(void) { return "headless stub"; }


/* --- window -------------------------------------------------------------- */

SDL_DECLSPEC SDL_Window* SDLCALL SDL_CreateWindow(const char* title, int w, int h, SDL_WindowFlags flags) {
	static int foo;
	(void)title; (void)w; (void)h; (void)flags;
	return (SDL_Window*)(&foo);
}

SDL_DECLSPEC void SDLCALL SDL_DestroyWindow(SDL_Window* window) { (void)window; }
SDL_DECLSPEC bool SDLCALL SDL_MinimizeWindow(SDL_Window* window) { (void)window; return true; }
SDL_DECLSPEC bool SDLCALL SDL_MaximizeWindow(SDL_Window* window) { (void)window; return true; }
SDL_DECLSPEC bool SDLCALL SDL_RestoreWindow(SDL_Window* window) { (void)window; return true; }
SDL_DECLSPEC bool SDLCALL SDL_HideWindow(SDL_Window* window) { (void)window; return true; }

SDL_DECLSPEC const char* SDLCALL SDL_GetWindowTitle(SDL_Window* window) { (void)window; return ""; }
SDL_DECLSPEC bool SDLCALL SDL_SetWindowTitle(SDL_Window* window, const char* title) { (void)window; (void)title; return true; }
SDL_DECLSPEC bool SDLCALL SDL_SetWindowIcon(SDL_Window* window, SDL_Surface* icon) { (void)window; (void)icon; return true; }

SDL_DECLSPEC SDL_PixelFormat SDLCALL SDL_GetWindowPixelFormat(SDL_Window* window) { (void)window; return SDL_PIXELFORMAT_RGB24; }
SDL_DECLSPEC const char* SDLCALL SDL_GetPixelFormatName(SDL_PixelFormat format) { (void)format; return ""; }

SDL_DECLSPEC bool SDLCALL SDL_SetWindowFullscreen(SDL_Window* window, bool fullscreen) { (void)window; (void)fullscreen; return true; }
SDL_DECLSPEC bool SDLCALL SDL_SetWindowFullscreenMode(SDL_Window* window, const SDL_DisplayMode* mode) { (void)window; (void)mode; return true; }
SDL_DECLSPEC const SDL_DisplayMode* SDLCALL SDL_GetWindowFullscreenMode(SDL_Window* window) { (void)window; return NULL; }

SDL_DECLSPEC bool SDLCALL SDL_SetWindowBordered(SDL_Window* window, bool bordered) { (void)window; (void)bordered; return true; }
SDL_DECLSPEC bool SDLCALL SDL_SetWindowPosition(SDL_Window* window, int x, int y) { (void)window; (void)x; (void)y; return true; }
SDL_DECLSPEC bool SDLCALL SDL_SetWindowSize(SDL_Window* window, int w, int h) { (void)window; (void)w; (void)h; return true; }
SDL_DECLSPEC bool SDLCALL SDL_SetWindowMinimumSize(SDL_Window* window, int min_w, int min_h) { (void)window; (void)min_w; (void)min_h; return true; }

SDL_DECLSPEC bool SDLCALL SDL_GetWindowBordersSize(SDL_Window* window, int* top, int* left, int* bottom, int* right) {
	(void)window;
	if (top)    *top = 0;
	if (left)   *left = 0;
	if (bottom) *bottom = 0;
	if (right)  *right = 0;
	return true;
}

SDL_DECLSPEC SDL_WindowFlags SDLCALL SDL_GetWindowFlags(SDL_Window* window) { (void)window; return 0; }

SDL_DECLSPEC bool SDLCALL SDL_SetWindowMouseGrab(SDL_Window* window, bool grabbed) { (void)window; (void)grabbed; return true; }
SDL_DECLSPEC bool SDLCALL SDL_GetWindowMouseGrab(SDL_Window* window) { (void)window; return false; }


/* --- OpenGL -------------------------------------------------------------- */

SDL_DECLSPEC bool SDLCALL SDL_GL_SetAttribute(SDL_GLAttr attr, int value) { (void)attr; (void)value; return true; }
SDL_DECLSPEC bool SDLCALL SDL_GL_GetAttribute(SDL_GLAttr attr, int* value) { (void)attr; if (value) *value = 0; return true; }

SDL_DECLSPEC SDL_GLContext SDLCALL SDL_GL_CreateContext(SDL_Window* window) { static int foo; (void)window; return (SDL_GLContext)(&foo); }
SDL_DECLSPEC bool SDLCALL SDL_GL_DestroyContext(SDL_GLContext context) { (void)context; return true; }
SDL_DECLSPEC bool SDLCALL SDL_GL_MakeCurrent(SDL_Window* window, SDL_GLContext context) { (void)window; (void)context; return true; }
SDL_DECLSPEC bool SDLCALL SDL_GL_SwapWindow(SDL_Window* window) { (void)window; return true; }
SDL_DECLSPEC bool SDLCALL SDL_GL_GetSwapInterval(int* interval) { if (interval) *interval = 0; return true; }
SDL_DECLSPEC bool SDLCALL SDL_GL_SetSwapInterval(int interval) { (void)interval; return true; }


/* --- surfaces ------------------------------------------------------------ */

SDL_DECLSPEC SDL_Surface* SDLCALL SDL_CreateSurface(int width, int height, SDL_PixelFormat format) {
	(void)width; (void)height; (void)format;
	return NULL;
}
SDL_DECLSPEC SDL_Surface* SDLCALL SDL_CreateSurfaceFrom(int width, int height, SDL_PixelFormat format, void* pixels, int pitch) {
	(void)width; (void)height; (void)format; (void)pixels; (void)pitch;
	return NULL;
}
SDL_DECLSPEC void SDLCALL SDL_DestroySurface(SDL_Surface* surface) { (void)surface; }


/* --- events -------------------------------------------------------------- */

SDL_DECLSPEC bool SDLCALL SDL_PollEvent(SDL_Event* event) { (void)event; return false; }
SDL_DECLSPEC bool SDLCALL SDL_PushEvent(SDL_Event* event) { (void)event; return true; }
SDL_DECLSPEC void SDLCALL SDL_PumpEvents(void) {}
SDL_DECLSPEC void SDLCALL SDL_FlushEvent(Uint32 type) { (void)type; }
SDL_DECLSPEC int SDLCALL SDL_PeepEvents(SDL_Event* events, int numevents, SDL_EventAction action, Uint32 minType, Uint32 maxType) {
	(void)events; (void)numevents; (void)action; (void)minType; (void)maxType;
	return 0;
}


/* --- keyboard ------------------------------------------------------------ */

SDL_DECLSPEC const bool* SDLCALL SDL_GetKeyboardState(int* numkeys) { if (numkeys) *numkeys = 0; return stubKeyState; }
SDL_DECLSPEC SDL_Keymod SDLCALL SDL_GetModState(void) { return SDL_KMOD_NONE; }
SDL_DECLSPEC void SDLCALL SDL_SetModState(SDL_Keymod modstate) { (void)modstate; }
SDL_DECLSPEC SDL_Keycode SDLCALL SDL_GetKeyFromScancode(SDL_Scancode scancode, SDL_Keymod modstate, bool key_event) { (void)scancode; (void)modstate; (void)key_event; return 0; }
SDL_DECLSPEC SDL_Scancode SDLCALL SDL_GetScancodeFromKey(SDL_Keycode key, SDL_Keymod* modstate) { (void)key; if (modstate) *modstate = SDL_KMOD_NONE; return SDL_SCANCODE_UNKNOWN; }

SDL_DECLSPEC bool SDLCALL SDL_StartTextInput(SDL_Window* window) { (void)window; return true; }
SDL_DECLSPEC bool SDLCALL SDL_StopTextInput(SDL_Window* window) { (void)window; return true; }
SDL_DECLSPEC bool SDLCALL SDL_SetTextInputArea(SDL_Window* window, const SDL_Rect* rect, int cursor) { (void)window; (void)rect; (void)cursor; return true; }


/* --- mouse / cursor ------------------------------------------------------ */

SDL_DECLSPEC bool SDLCALL SDL_ShowCursor(void) { return true; }
SDL_DECLSPEC bool SDLCALL SDL_HideCursor(void) { return true; }
SDL_DECLSPEC bool SDLCALL SDL_CursorVisible(void) { return false; }
SDL_DECLSPEC SDL_MouseButtonFlags SDLCALL SDL_GetMouseState(float* x, float* y) { if (x) *x = 0.0f; if (y) *y = 0.0f; return 0; }
SDL_DECLSPEC void SDLCALL SDL_WarpMouseInWindow(SDL_Window* window, float x, float y) { (void)window; (void)x; (void)y; }
SDL_DECLSPEC bool SDLCALL SDL_SetWindowRelativeMouseMode(SDL_Window* window, bool enabled) { (void)window; (void)enabled; return true; }
SDL_DECLSPEC bool SDLCALL SDL_CaptureMouse(bool enabled) { (void)enabled; return true; }


/* --- clipboard ----------------------------------------------------------- */

SDL_DECLSPEC char* SDLCALL SDL_GetClipboardText(void) {
	/* SDL3 contract: caller frees with SDL_free; our SDL_free is a no-op, so a
	 * static empty string is safe. */
	static char empty[1] = { '\0' };
	return empty;
}
SDL_DECLSPEC bool SDLCALL SDL_SetClipboardText(const char* text) { (void)text; return false; }


/* --- displays ------------------------------------------------------------ */

SDL_DECLSPEC SDL_DisplayID* SDLCALL SDL_GetDisplays(int* count) { if (count) *count = 1; return stubDisplays; }
SDL_DECLSPEC SDL_DisplayID SDLCALL SDL_GetPrimaryDisplay(void) { return stubDisplays[0]; }
SDL_DECLSPEC SDL_DisplayID SDLCALL SDL_GetDisplayForWindow(SDL_Window* window) { (void)window; return stubDisplays[0]; }
SDL_DECLSPEC const char* SDLCALL SDL_GetDisplayName(SDL_DisplayID displayID) { (void)displayID; return ""; }

static const SDL_DisplayMode* StubDisplayMode(void) {
	stubDisplayMode.format = SDL_PIXELFORMAT_RGB24;
	stubDisplayMode.w = 640;
	stubDisplayMode.h = 480;
	stubDisplayMode.refresh_rate = 100.0f;
	return &stubDisplayMode;
}

SDL_DECLSPEC const SDL_DisplayMode* SDLCALL SDL_GetDesktopDisplayMode(SDL_DisplayID displayID) { (void)displayID; return StubDisplayMode(); }
SDL_DECLSPEC const SDL_DisplayMode* SDLCALL SDL_GetCurrentDisplayMode(SDL_DisplayID displayID) { (void)displayID; return StubDisplayMode(); }
SDL_DECLSPEC SDL_DisplayMode** SDLCALL SDL_GetFullscreenDisplayModes(SDL_DisplayID displayID, int* count) { (void)displayID; if (count) *count = 0; return NULL; }

SDL_DECLSPEC bool SDLCALL SDL_GetDisplayBounds(SDL_DisplayID displayID, SDL_Rect* rect) {
	(void)displayID;
	if (rect == NULL) return false;
	rect->x = 0; rect->y = 0; rect->w = 640; rect->h = 480;
	return true;
}
SDL_DECLSPEC bool SDLCALL SDL_GetDisplayUsableBounds(SDL_DisplayID displayID, SDL_Rect* rect) {
	(void)displayID;
	if (rect == NULL) return false;
	rect->x = 0; rect->y = 0; rect->w = 640; rect->h = 480;
	return true;
}

SDL_DECLSPEC bool SDLCALL SDL_GetRectIntersection(const SDL_Rect* A, const SDL_Rect* B, SDL_Rect* result) {
	(void)A; (void)B; (void)result;
	return false;
}


/* --- window properties (used by the SDL_syswm compat shim) --------------- */

SDL_DECLSPEC SDL_PropertiesID SDLCALL SDL_GetWindowProperties(SDL_Window* window) { (void)window; return 0; }
SDL_DECLSPEC void* SDLCALL SDL_GetPointerProperty(SDL_PropertiesID props, const char* name, void* default_value) { (void)props; (void)name; return default_value; }
SDL_DECLSPEC Sint64 SDLCALL SDL_GetNumberProperty(SDL_PropertiesID props, const char* name, Sint64 default_value) { (void)props; (void)name; return default_value; }


#ifdef __cplusplus
} // extern "C"
#endif
