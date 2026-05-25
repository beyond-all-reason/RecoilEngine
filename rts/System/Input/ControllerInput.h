/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

#include <SDL_events.h>
#include <SDL_gamecontroller.h>

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "System/Input/InputHandler.h"

class CControllerInput
{
public:
	static CControllerInput* GetInstance();
	static void FreeInstance(CControllerInput* controllerInput);

	CControllerInput();
	~CControllerInput();

	struct ControllerStateSnapshot {
		int deviceId = -1;
		int instanceId = -1;
		std::string name;

		std::array<std::int16_t, 16> axes = {};
		std::array<std::uint8_t, 32> buttons = {};
	};

	std::vector<ControllerStateSnapshot> GetAvailableControllers() const;
	bool GetControllerState(int instanceId, ControllerStateSnapshot& state) const;

	bool HandleSDLControllerEvent(const SDL_Event& event);

private:
	struct ControllerState {
		int deviceId = -1;
		int instanceId = -1;
		std::string name;

		SDL_GameController* gameController = nullptr;

		std::array<std::int16_t, 16> axes = {};
		std::array<std::uint8_t, 32> buttons = {};
	};

	static ControllerStateSnapshot MakeControllerStateSnapshot(const ControllerState& state);

	void LogAvailableController(int deviceId) const;
	void ScanExistingControllers();
	void HandleDeviceAdded(int deviceId);
	void HandleDeviceRemoved(int instanceId);
	void HandleDeviceRemapped(int instanceId);
	void HandleButtonDown(int instanceId, int buttonId, std::uint8_t value);
	void HandleButtonUp(int instanceId, int buttonId, std::uint8_t value);
	void HandleAxisMotion(int instanceId, int axisId, std::int16_t value);

private:
	InputHandler::HandlerTokenT inputCon;
	std::unordered_map<int, ControllerState> controllersByInstanceId;
};

extern CControllerInput* controllerInput;
