/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "InputHandler.h"

#include "Rendering/GlobalRendering.h"
#include "System/TimeProfiler.h"

InputHandler input;

/**
 * SDL reports the mouse in window points. Everything that handles these events
 * tests the position against a viewport, and viewports are in the framebuffer's
 * backing pixels, so the two have to be put in the same units once, here, where
 * every event enters the engine.
 *
 * A point is a pixel everywhere except a macOS window on a Retina display.
 */
static void ToPixelCoords(SDL_Event& ev)
{
	switch (ev.type) {
		case SDL_MOUSEMOTION: {
			const int2 pos = globalRendering->PointToPixel({ev.motion.x, ev.motion.y});
			const int2 rel = globalRendering->PointToPixel({ev.motion.xrel, ev.motion.yrel});

			ev.motion.x = pos.x;
			ev.motion.y = pos.y;
			ev.motion.xrel = rel.x;
			ev.motion.yrel = rel.y;
		} break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP: {
			const int2 pos = globalRendering->PointToPixel({ev.button.x, ev.button.y});

			ev.button.x = pos.x;
			ev.button.y = pos.y;
		} break;
	}
}

InputHandler::InputHandler() = default;

void InputHandler::PushEvent(const SDL_Event& ev)
{
	for (const auto& eventHandler : eventHandlers) {
		if (eventHandler) {
			if (eventHandler(ev))
				break;
		}
	}
}

void InputHandler::PushEvents()
{
	SCOPED_TIMER("Misc::InputHandler::PushEvents");

	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		// SDL_PollEvent may modify FPU flags
		streflop::streflop_init<streflop::Simple>();
		ToPixelCoords(event);
		PushEvent(event);
	}
}

InputHandler::HandlerTokenT InputHandler::AddHandler(InputHandler::HandlerFuncT func)
{
	for (size_t i = 0; i < eventHandlers.size(); ++i) {
		if (eventHandlers[i] == nullptr) {
			eventHandlers[i] = func;
			return InputHandler::HandlerTokenT{ *this, i};
		}
	}
	eventHandlers.emplace_back(func);
	return InputHandler::HandlerTokenT{ *this, eventHandlers.size() - 1 };
}


