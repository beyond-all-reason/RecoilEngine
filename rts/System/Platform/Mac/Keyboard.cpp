/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Keyboard.h"

#include <algorithm>
#include <array>
#include <cstdint>

#include <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>
#include <objc/message.h>
#include <objc/runtime.h>

#include <SDL_scancode.h>
#include <SDL_timer.h>
#include <SDL_video.h>

#include "System/Config/ConfigHandler.h"

CONFIG(bool, MacCommandAsCtrl).defaultValue(true).description("Command acts as Control, so Cmd+key works like Ctrl+key on other platforms. Also takes the key equivalents (Cmd+Q, Cmd+H, Cmd+M, ...) out of the application menu.");

namespace MacKeyboard {

static constexpr unsigned long NSEventTypeKeyDown = 10;
static constexpr unsigned long NSEventTypeKeyUp = 11;
static constexpr unsigned long NSEventModifierFlagCommand = 1 << 20;

// macOS virtual key codes of the character keys, the ones a keyboard layout can make dead
static constexpr std::array<SDL_Scancode, 51> characterKeys = {
	SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_F, SDL_SCANCODE_H, SDL_SCANCODE_G,
	SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_C, SDL_SCANCODE_V, SDL_SCANCODE_NONUSBACKSLASH, SDL_SCANCODE_B,
	SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_R, SDL_SCANCODE_Y, SDL_SCANCODE_T,
	SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4, SDL_SCANCODE_6, SDL_SCANCODE_5,
	SDL_SCANCODE_EQUALS, SDL_SCANCODE_9, SDL_SCANCODE_7, SDL_SCANCODE_MINUS, SDL_SCANCODE_8, SDL_SCANCODE_0,
	SDL_SCANCODE_RIGHTBRACKET, SDL_SCANCODE_O, SDL_SCANCODE_U, SDL_SCANCODE_LEFTBRACKET, SDL_SCANCODE_I, SDL_SCANCODE_P,
	SDL_SCANCODE_RETURN, SDL_SCANCODE_L, SDL_SCANCODE_J, SDL_SCANCODE_APOSTROPHE, SDL_SCANCODE_K, SDL_SCANCODE_SEMICOLON,
	SDL_SCANCODE_BACKSLASH, SDL_SCANCODE_COMMA, SDL_SCANCODE_SLASH, SDL_SCANCODE_N, SDL_SCANCODE_M, SDL_SCANCODE_PERIOD,
	SDL_SCANCODE_TAB, SDL_SCANCODE_SPACE, SDL_SCANCODE_GRAVE,
};

// keys macOS takes for itself or Mac keyboards lack, reachable with Cmd instead
struct Alias {
	SDL_Scancode from;
	SDL_Scancode to;
};

static constexpr std::array<Alias, 2> aliases = {{
	{SDL_SCANCODE_F11,    SDL_SCANCODE_F11  }, // Show Desktop
	{SDL_SCANCODE_PERIOD, SDL_SCANCODE_PAUSE}, // Cmd+Esc never arrives
}};

static bool commandAsCtrl = false;

static std::vector<SDL_Scancode> heldKeys;
static std::vector<Alias> heldAliases;

using SendEventFunc = void (*)(id, SEL, id);
static SendEventFunc sdlSendEvent = nullptr;


template<typename R = id, typename... Args>
static R Send(id obj, const char* sel, Args... args)
{
	return reinterpret_cast<R (*)(id, SEL, Args...)>(objc_msgSend)(obj, sel_registerName(sel), args...);
}

static id GetApp()
{
	return Send(reinterpret_cast<id>(objc_getClass("NSApplication")), "sharedApplication");
}


// HIToolbox comes with AppKit, the engine does not link it
static bool IsISOKeyboard()
{
	using GetKbdType = uint8_t (*)();
	using GetLayoutType = uint32_t (*)(int16_t);

	static const auto getKbdType = reinterpret_cast<GetKbdType>(dlsym(RTLD_DEFAULT, "LMGetKbdType"));
	static const auto getLayoutType = reinterpret_cast<GetLayoutType>(dlsym(RTLD_DEFAULT, "KBGetLayoutType"));

	if (getKbdType == nullptr || getLayoutType == nullptr)
		return false;

	// kKeyboardISO, 'ISO '
	return (getLayoutType(getKbdType()) == 0x49534F20);
}

static SDL_Scancode GetCharacterKey(unsigned short keyCode)
{
	if (keyCode >= characterKeys.size())
		return SDL_SCANCODE_UNKNOWN;

	// SDL swaps these two on ISO keyboards, see its scancodes_darwin.h
	if ((keyCode == 10 || keyCode == 50) && IsISOKeyboard())
		keyCode = 60 - keyCode;

	return characterKeys[keyCode];
}

static void PushKeyEvent(SDL_Scancode scanCode, bool pressed)
{
	SDL_Window* window = SDL_GetKeyboardFocus();

	SDL_Event event = {};
	event.type = pressed ? SDL_KEYDOWN : SDL_KEYUP;
	event.key.timestamp = SDL_GetTicks();
	event.key.windowID = (window != nullptr) ? SDL_GetWindowID(window) : 0;
	event.key.state = pressed ? SDL_PRESSED : SDL_RELEASED;
	event.key.keysym.scancode = scanCode;
	event.key.keysym.sym = SDL_GetKeyFromScancode(scanCode);
	event.key.keysym.mod = SDL_GetModState();
	SDL_PushEvent(&event);
}

// With text input active SDL hands every key to the input method first and
// drops the key event when that starts or ends a composition, so dead keys
// (^ and ´ on a German layout, Option+n, ...) never arrive as key presses,
// neither does the key typed after them. Other platforms deliver both the key
// and the text, so send the key whenever SDL's keyboard state did not take it.
static void SendEvent(id self, SEL sel, id event)
{
	const unsigned long type = Send<unsigned long>(event, "type");

	if (type != NSEventTypeKeyDown && type != NSEventTypeKeyUp) {
		sdlSendEvent(self, sel, event);
		return;
	}

	const SDL_Scancode scanCode = GetCharacterKey(Send<unsigned short>(event, "keyCode"));
	const Uint8* keyState = SDL_GetKeyboardState(nullptr);
	const bool wasDown = (keyState[scanCode] != 0);

	sdlSendEvent(self, sel, event);

	if (scanCode == SDL_SCANCODE_UNKNOWN)
		return;

	const auto heldIt = std::find(heldKeys.begin(), heldKeys.end(), scanCode);

	if (type == NSEventTypeKeyUp) {
		if (heldIt == heldKeys.end())
			return;

		heldKeys.erase(heldIt);
		PushKeyEvent(scanCode, false);
		return;
	}

	if (wasDown || keyState[scanCode] != 0 || heldIt != heldKeys.end())
		return;
	if (SDL_GetKeyboardFocus() == nullptr || Send<signed char>(event, "isARepeat"))
		return;
	// may have been a menu key equivalent
	if ((Send<unsigned long>(event, "modifierFlags") & NSEventModifierFlagCommand) != 0)
		return;

	heldKeys.push_back(scanCode);
	PushKeyEvent(scanCode, true);
}

static void HookSendEvent(id app)
{
	Class appClass = object_getClass(app);
	SEL sel = sel_registerName("sendEvent:");
	Method method = class_getInstanceMethod(appClass, sel);

	if (method == nullptr)
		return;

	sdlSendEvent = reinterpret_cast<SendEventFunc>(method_getImplementation(method));
	class_replaceMethod(appClass, sel, reinterpret_cast<IMP>(&SendEvent), method_getTypeEncoding(method));
}

static void ClearKeyEquivalents(id menu)
{
	id items = Send(menu, "itemArray");
	const unsigned long count = Send<unsigned long>(items, "count");

	for (unsigned long i = 0; i < count; ++i) {
		id item = Send(items, "objectAtIndex:", i);
		id submenu = Send(item, "submenu");

		Send<void>(item, "setKeyEquivalent:", reinterpret_cast<id>(const_cast<__CFString*>(CFSTR(""))));

		if (submenu != nullptr)
			ClearKeyEquivalents(submenu);
	}
}


void Init()
{
	id app = GetApp();

	if (app == nullptr)
		return;

	commandAsCtrl = configHandler->GetBool("MacCommandAsCtrl");

	if (sdlSendEvent == nullptr)
		HookSendEvent(app);

	if (!commandAsCtrl)
		return;

	if (id menu = Send(app, "mainMenu"); menu != nullptr)
		ClearKeyEquivalents(menu);
}

void TranslateEvent(SDL_Event& event)
{
	if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
		heldKeys.clear();
		heldAliases.clear();
		return;
	}

	if (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP)
		return;

	SDL_Keysym& keysym = event.key.keysym;
	const auto heldIt = std::find_if(heldAliases.begin(), heldAliases.end(), [&](const Alias& a) { return (a.from == keysym.scancode); });

	if (heldIt != heldAliases.end()) {
		keysym.scancode = heldIt->to;
		keysym.sym = SDL_GetKeyFromScancode(heldIt->to);
		keysym.mod = KMOD_NONE;

		if (event.type == SDL_KEYUP)
			heldAliases.erase(heldIt);

		return;
	}

	if (event.type == SDL_KEYDOWN && (keysym.mod & (KMOD_GUI | KMOD_CTRL | KMOD_SHIFT | KMOD_ALT)) == (keysym.mod & KMOD_GUI) && (keysym.mod & KMOD_GUI) != 0) {
		const auto aliasIt = std::find_if(aliases.begin(), aliases.end(), [&](const Alias& a) { return (a.from == keysym.scancode); });

		if (aliasIt != aliases.end()) {
			heldAliases.push_back(*aliasIt);
			keysym.scancode = aliasIt->to;
			keysym.sym = SDL_GetKeyFromScancode(aliasIt->to);
			keysym.mod = KMOD_NONE;
			return;
		}
	}

	keysym.mod = TranslateMods(SDL_Keymod(keysym.mod));

	if (!commandAsCtrl)
		return;

	switch (keysym.scancode) {
		case SDL_SCANCODE_LGUI: { keysym.scancode = SDL_SCANCODE_LCTRL; keysym.sym = SDLK_LCTRL; } break;
		case SDL_SCANCODE_RGUI: { keysym.scancode = SDL_SCANCODE_RCTRL; keysym.sym = SDLK_RCTRL; } break;
		default: {} break;
	}
}

SDL_Keymod TranslateMods(SDL_Keymod mods)
{
	// an alias stands in for a key pressed on its own
	if (!heldAliases.empty())
		return SDL_Keymod(mods & ~(KMOD_GUI | KMOD_CTRL));

	if (!commandAsCtrl)
		return mods;

	int ctrl = mods & KMOD_CTRL;

	if ((mods & KMOD_LGUI) != 0)
		ctrl |= KMOD_LCTRL;
	if ((mods & KMOD_RGUI) != 0)
		ctrl |= KMOD_RCTRL;

	return SDL_Keymod((mods & ~(KMOD_GUI | KMOD_CTRL)) | ctrl);
}

const std::vector<SDL_Scancode>& GetHeldKeys()
{
	return heldKeys;
}

};
