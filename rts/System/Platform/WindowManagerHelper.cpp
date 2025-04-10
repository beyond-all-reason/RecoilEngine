/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WindowManagerHelper.h"

#include "Game/GameVersion.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/Bitmap.h"
#include "System/Log/ILog.h"
#include "System/Platform/Threading.h"
#include "System/QueueToMain.h"

#include <SDL_video.h>

namespace WindowManagerHelper {

struct WindowIcon {
	// do not rely on static (de)init, pool might not yet exist or be destructed first
	uint8_t bmpMem[sizeof(CBitmap)] = {0};

	CBitmap* bmp = nullptr;
	SDL_Surface* surf = nullptr;
};

// LuaUnsyncedCtrl only
void SetIcon(CBitmap* bmp, bool forceResolution)
{
	if (SpringVersion::IsHeadless())
		return;

	// 24bit RGB or 32bit RGBA
	if (((bmp->channels != 3) && (bmp->channels != 4))
	    // #ifdef    _WIN32
	    //  on windows, the icon has to be 32x32
	    //  ^^ no longer true, allow override
	    || (!forceResolution && bmp->xsize != 32) || (!forceResolution && bmp->ysize != 32)
	    // #endif
	) {
		LOG_L(
		    L_WARNING, "[WindowManager::%s] icon-format has to be RGB or RGBA, and 32x32 pixels on Windows", __func__);
		return;
	}

	// supplied bitmap is usable as icon, keep it
	SetIconSurface(globalRendering->GetWindow(), bmp);
}

bool SetIconSurface(SDL_Window* win, CBitmap* bmp)
{
	static WindowIcon windowIcon;

	if (windowIcon.bmp == nullptr)
		windowIcon.bmp = new (windowIcon.bmpMem) CBitmap();

	if (bmp == nullptr) {
		// only reached on exit
		SDL_FreeSurface(windowIcon.surf);
		SDL_SetWindowIcon(win, windowIcon.surf = nullptr);

		*(windowIcon.bmp) = {};
		return false;
	}

	*(windowIcon.bmp) = std::move(*bmp);
	SDL_Surface* surf = windowIcon.bmp->CreateSDLSurface();

	if (surf == nullptr) {
		// keep any previous surface in case of failure
		LOG_L(
		    L_WARNING, "[WindowManagerHelper::%s] failed to create SDL surface, reason: %s", __func__, SDL_GetError());
		return false;
	}

	static auto SetWindowIconImpl = [](SDL_Window* win, SDL_Surface* surf) {
		SDL_SetWindowIcon(win, surf);
		SDL_FreeSurface(windowIcon.surf);
		windowIcon.surf = surf;
	};

	if (Threading::IsMainThread())
		SetWindowIconImpl(win, surf);
	else
		spring::QueuedFunction::Enqueue(SetWindowIconImpl, win, surf);

	return true;
}


}; // namespace WindowManagerHelper
