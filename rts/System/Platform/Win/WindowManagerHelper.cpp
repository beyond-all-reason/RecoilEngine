/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/Platform/WindowManagerHelper.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>

#include <windows.h>


namespace WindowManagerHelper {

void BlockCompositing(SDL_Window* window)
{
#ifndef HEADLESS
	// only available in WinVista+
	HMODULE dwmapiDllHandle = LoadLibrary(L"dwmapi.dll");

	if (dwmapiDllHandle != nullptr) {
		using DwmEnableCompositionFunction = HRESULT(*)(UINT uCompositionAction);
		auto DwmEnableComposition = (DwmEnableCompositionFunction) ::GetProcAddress(dwmapiDllHandle, "DwmEnableComposition");
		if (DwmEnableComposition != nullptr) {
			static const unsigned int DWM_EC_DISABLECOMPOSITION = 0U;
			DwmEnableComposition(DWM_EC_DISABLECOMPOSITION);
		}

		FreeLibrary(dwmapiDllHandle);
	}
#endif
}


int GetWindowState(SDL_Window* window)
{
	int state = 0;
#ifndef HEADLESS
	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);

	const SDL_PropertiesID props = SDL_GetWindowProperties(window);
	HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	if (!hwnd)
		return 0;

	if (GetWindowPlacement(hwnd, &wp)) {
		if (wp.showCmd == SW_SHOWMAXIMIZED)
			state = SDL_WINDOW_MAXIMIZED;
		if (wp.showCmd == SW_SHOWMINIMIZED)
			state = SDL_WINDOW_MINIMIZED;
	}
#endif
	return state;
}


// taken from http://stackoverflow.com/questions/27116152
void SetWindowResizable(SDL_Window* window, bool resizable)
{
#ifndef HEADLESS
	const SDL_PropertiesID props = SDL_GetWindowProperties(window);
	HWND hwnd = (HWND)SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	if (!hwnd)
		return;

	DWORD style = GetWindowLong(hwnd, GWL_STYLE);
	if (resizable) {
		style |= (WS_THICKFRAME | WS_MAXIMIZEBOX);
	} else {
		style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
	}
	SetWindowLong(hwnd, GWL_STYLE, style);
#endif
}

}; // namespace WindowManagerHelper
