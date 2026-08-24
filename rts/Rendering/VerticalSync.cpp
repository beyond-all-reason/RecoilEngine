/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <string>

#include "VerticalSync.h"
#include "GL/myGL.h"
#include "System/SpringMath.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"

#include <SDL_video.h>

#if defined(__APPLE__) && !defined(HEADLESS)
	// SDL owns neither the GL context nor the present on macOS, so
	// SDL_GL_SetSwapInterval below reaches nothing. The layer that actually
	// shows the frame has its own switch, see MetalPresent.h.
	#include "System/Platform/Mac/MetalPresent.h"
#endif

static constexpr int MAX_ADAPTIVE_INTERVAL = -6;
static constexpr int MAX_STANDARD_INTERVAL = +6;

static CVerticalSync instance;


CONFIG(int, VSync).
	defaultValue(-1).
	minimumValue(MAX_ADAPTIVE_INTERVAL).
	maximumValue(MAX_STANDARD_INTERVAL).
	description(
		"Synchronize buffer swaps with vertical blanking interval."
		" Modes are -N (adaptive), +N (standard), or 0 (disabled)."
	);

CVerticalSync* CVerticalSync::GetInstance()
{
	return &instance;
}

void CVerticalSync::WrapNotifyOnChange()
{
	configHandler->NotifyOnChange(this, {"VSync"});
}

void CVerticalSync::WrapRemoveObserver()
{
	// can't do this in the dtor because VerticalSync outlives configHandler
	configHandler->RemoveObserver(this);
}

void CVerticalSync::ConfigNotify(const std::string& key, const std::string& value)
{
	SetInterval(configHandler->GetInt("VSync"));
}


void CVerticalSync::Toggle()
{
	// no-arg switch, select smallest interval
	switch (std::clamp(SDL_GL_GetSwapInterval(), -1, 1)) {
		case -1: { SetInterval( 0); } break;
		case  0: { SetInterval(+1); } break;
		case +1: { SetInterval(-1); } break;
		default: {} break;
	}
}

void CVerticalSync::SetInterval() { SetInterval(configHandler->GetInt("VSync")); }
void CVerticalSync::SetInterval(int i)
{
	// recursion is already prevented (Set only notifies on changed
	// values), this just avoids making the SDL calls a second time
	if ((i = std::clamp(i, MAX_ADAPTIVE_INTERVAL, MAX_STANDARD_INTERVAL)) == interval)
		return;

	configHandler->Set("VSync", interval = i);

	#if defined HEADLESS
	return;
	#endif

	#if defined(__APPLE__) && !defined(HEADLESS)
	// Both adaptive and standard mean "wait for the blank" to a CAMetalLayer,
	// which has one switch rather than an interval. Only 0 turns it off.
	// Return here: the SDL calls below reach no context on this path, and their
	// failure branch would log a swap interval that was never applied.
	MacMetalPresent_SetVSync(interval != 0);
	return;
	#endif

	// adaptive (delay swap iff frame-rate > vblank-rate)
	if (interval < 0 && SDL_GL_SetSwapInterval(interval) == 0) {
		LOG("[VSync::%s] interval=%d (adaptive)", __func__, interval);
		return;
	}
	// standard (<interval> vblanks per swap)
	if (interval > 0 && SDL_GL_SetSwapInterval(interval) == 0) {
		LOG("[VSync::%s] interval=%d (standard)", __func__, interval);
		return;
	}

	// disabled (never wait for vblank)
	SDL_GL_SetSwapInterval(0);
	LOG("[VSync::%s] interval=%d (disabled)", __func__, interval);
}
