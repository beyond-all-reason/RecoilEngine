#include "glxHandler.h"

#if !defined(HEADLESS) && !defined(_WIN32) && !defined(__APPLE__)

#include <glad/glad_glx.h>
#include <SDL_syswm.h>

void GLX::Load(SDL_Window* window)
{
	supported = false;

	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	if (!SDL_GetWindowWMInfo(window, &info))
		return;

	switch (info.subsystem)
	{
	case SDL_SYSWM_X11: {
		Display* display = info.info.x11.display;
		supported = static_cast<bool>(gladLoadGLX(display, DefaultScreen(display)));
	} break;
	case SDL_SYSWM_WAYLAND: {
		// unsafe in case XWayland is not available
		//supported = static_cast<bool>(gladLoadGLX(nullptr, 0));
	} break;
	default:
		break;
	}
}

void GLX::Unload()
{
	if (!supported)
		return;
	
	gladUnloadGLX();
}

bool GLX::GetVideoMemInfoMESA(int* memInfo)
{
	if (!supported)
		return false;

#if (defined(GLX_MESA_query_renderer))
	if (!GLAD_GLX_MESA_query_renderer)
		return false;

	// note: unlike the others, this value is returned in megabytes
	glad_glXQueryCurrentRendererIntegerMESA(GLX_RENDERER_VIDEO_MEMORY_MESA, reinterpret_cast<unsigned int*>(&memInfo[0]));

	memInfo[0] *= 1024;
	memInfo[1] = memInfo[0];
	return true;
#else
	return false;
#endif
}
#else
void GLX::Load(SDL_Window* window) {}
void GLX::Unload() {}
bool GLX::GetVideoMemInfoMESA(int* memInfo) { return false; }
#endif