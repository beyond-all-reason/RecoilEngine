/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef MAC_KEYBOARD_H
#define MAC_KEYBOARD_H

#include <vector>

#include <SDL_events.h>
#include <SDL_keyboard.h>

namespace MacKeyboard {
	void Init();

	void TranslateEvent(SDL_Event& event);
	SDL_Keymod TranslateMods(SDL_Keymod mods);

	// keys held down that SDL's own keyboard state does not know about
	const std::vector<SDL_Scancode>& GetHeldKeys();
};

#endif // MAC_KEYBOARD_H
