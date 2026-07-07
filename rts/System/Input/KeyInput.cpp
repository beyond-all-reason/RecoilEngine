/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>
#include <functional>
#include <cassert>
#include <cctype>

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_events.h>

#include "KeyInput.h"

/**
* @brief keys
*
* Array of possible keys, and which are being pressed
*/

namespace KeyInput {
	static       std::vector<Key> keyVec;
	static       std::vector<Key> scanVec;
	static const std::function<bool(const Key&, const Key&)> keyCmp = [](const Key& a, const Key& b) { return (a.first < b.first); };

	static SDL_Keymod keyMods;


	bool IsKeyPressed(int keyCode) {
		const auto& pred = keyCmp;
		const auto  iter = std::lower_bound(keyVec.begin(), keyVec.end(), Key{keyCode, false}, pred);

		return (iter != keyVec.end() && iter->first == keyCode && iter->second);
	}

	bool IsScanPressed(int scanCode) {
		const auto& pred = keyCmp;
		const auto  iter = std::lower_bound(scanVec.begin(), scanVec.end(), Key{scanCode, false}, pred);

		return (iter != scanVec.end() && iter->first == scanCode && iter->second);
	}

	void SetScanPressed(int scanCode, bool isPressed) {
		const auto& pred = keyCmp;
		const auto  iter = std::lower_bound(scanVec.begin(), scanVec.end(), Key{scanCode, false}, pred);

		// not reachable for default modifiers
		if (iter == scanVec.end())
			return;

		iter->second = isPressed;
	}

	void SetKeyPressed(int keyCode, bool isPressed) {
		const auto& pred = keyCmp;
		const auto  iter = std::lower_bound(keyVec.begin(), keyVec.end(), Key{keyCode, false}, pred);

		// not reachable for default modifiers
		if (iter == keyVec.end())
			return;

		iter->second = isPressed;
	}

	void SetKeyModState(int mod, bool isPressed) {
		if (isPressed) {
			keyMods = SDL_Keymod(keyMods | mod);
		} else {
			keyMods = SDL_Keymod(keyMods & ~mod);
		}
	}

	bool GetKeyModState(int mod) {
		return (keyMods & mod);
	}

	/**
	* Tests SDL keystates and sets values in key array
	*/
	void Update(int fakeMetaKey)
	{
		int numKeys = 0;
		const bool* kbState = SDL_GetKeyboardState(&numKeys);

		keyMods = SDL_GetModState();

		keyVec.clear();
		keyVec.reserve(numKeys);
		scanVec.clear();
		scanVec.reserve(numKeys);

		for (int i = 0; i < numKeys; ++i) {
			const auto scanCode = (SDL_Scancode)i;
			const auto keyCode  = SDL_GetKeyFromScancode(scanCode, 0, true);

			keyVec.emplace_back(keyCode, kbState[scanCode]);
			scanVec.emplace_back(scanCode, kbState[scanCode]);
		}

		std::sort(keyVec.begin(), keyVec.end(), keyCmp);
		std::sort(scanVec.begin(), scanVec.end(), keyCmp);

		SetKeyModState(SDL_KMOD_GUI, IsKeyPressed(fakeMetaKey));
		SetKeyPressed(SDLK_LALT  , GetKeyModState(SDL_KMOD_ALT  ));
		SetKeyPressed(SDLK_LCTRL , GetKeyModState(SDL_KMOD_CTRL ));
		SetKeyPressed(SDLK_LGUI  , GetKeyModState(SDL_KMOD_GUI  ));
		SetKeyPressed(SDLK_LSHIFT, GetKeyModState(SDL_KMOD_SHIFT));
		SetKeyPressed(SDL_SCANCODE_LALT  , GetKeyModState(SDL_KMOD_ALT  ));
		SetKeyPressed(SDL_SCANCODE_LCTRL , GetKeyModState(SDL_KMOD_CTRL ));
		SetKeyPressed(SDL_SCANCODE_LGUI  , GetKeyModState(SDL_KMOD_GUI  ));
		SetKeyPressed(SDL_SCANCODE_LSHIFT, GetKeyModState(SDL_KMOD_SHIFT));
	}

	const std::vector<Key>& GetPressedKeys()
	{
		return keyVec;
	}

	const std::vector<Key>& GetPressedScans()
	{
		return scanVec;
	}

	void ReleaseAllKeys()
	{
		for (const auto& key: keyVec) {
			auto keycode  = (SDL_Keycode)key.first;
			auto scancode = SDL_GetScancodeFromKey(keycode, nullptr);

			if (keycode == SDLK_NUMLOCKCLEAR || keycode == SDLK_CAPSLOCK || keycode == SDLK_SCROLLLOCK)
				continue;

			if (!KeyInput::IsKeyPressed(keycode))
				continue;

			SDL_Event event;
			event.type = event.key.type = SDL_EVENT_KEY_UP;
			event.key.down = false;
			event.key.key = keycode;
			event.key.mod = 0;
			event.key.scancode = scancode;
			SDL_PushEvent(&event);
		}
	}
} // namespace KeyInput
