#include "SDLRecoilCompat.h"

#if !defined(SDL_SYSWM_TYPE)
typedef enum SDL_SYSWM_TYPE {
	SDL_SYSWM_UNKNOWN,
	SDL_SYSWM_WINDOWS,
	SDL_SYSWM_X11,
	SDL_SYSWM_WAYLAND,
	SDL_SYSWM_COCOA
} SDL_SYSWM_TYPE;
#endif

typedef struct SDL_SysWMinfo {
	SDL_version version;
	SDL_SYSWM_TYPE subsystem;
	struct {
		struct {
			void* window;
		} win;
		struct {
			void* display;
			void* window;
		} x11;
	} info;
} SDL_SysWMinfo;

static inline int SDL_GetWindowWMInfo(SDL_Window*, SDL_SysWMinfo*)
{
	return 0;
}
