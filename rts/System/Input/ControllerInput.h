/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <SDL3/SDL_gamepad.h>
#include "System/Input/InputHandler.h"
#include "System/UnorderedMap.hpp"
#include "System/UnorderedSet.hpp"

class ControllerInput
{
public:
	ControllerInput();
	~ControllerInput();

	static void InitStatic();
	static void KillStatic();

	bool HandleSDLControllerEvent(const SDL_Event& event);

	SDL_Gamepad* GetConnectedController(int instanceId) const;

protected:
	SDL_Gamepad* ConnectController(int deviceId, int& instanceId);
	bool DisconnectController(int instanceId);
	bool DisconnectController(int instanceId, SDL_Gamepad* gamepad);
	void DisconnectControllers();

	typedef spring::unsynced_map<int, SDL_Gamepad*> ControllerMap;
	typedef spring::unsynced_set<int> ControllerSet;

protected:
	ControllerMap connectedControllers;

public:
	ControllerSet GetConnectedControllers() const;

protected:
	InputHandler::HandlerTokenT inputCon;
};

extern ControllerInput* controllerInput;
