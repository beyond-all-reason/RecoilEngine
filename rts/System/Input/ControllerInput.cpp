/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ControllerInput.h"

#include "System/Input/InputHandler.h"
#include "System/Log/ILog.h"

#ifndef CONTROLLER_INPUT_LOG_EVENTS
#define CONTROLLER_INPUT_LOG_EVENTS 0
#endif

#ifndef HEADLESS
#include <SDL.h>
#include <SDL_events.h>
#include <SDL_error.h>
#include <SDL_gamecontroller.h>
#include <SDL_joystick.h>
#endif

CControllerInput* controllerInput = nullptr;

CControllerInput* CControllerInput::GetInstance()
{
	if (controllerInput == nullptr) {
		controllerInput = new CControllerInput();
	}

	return controllerInput;
}

void CControllerInput::FreeInstance(CControllerInput* controllerInputPtr)
{
	if (controllerInputPtr == controllerInput) {
		delete controllerInput;
		controllerInput = nullptr;
	}
}

CControllerInput::ControllerStateSnapshot CControllerInput::MakeControllerStateSnapshot(const ControllerState& state)
{
	ControllerStateSnapshot snapshot;
	snapshot.deviceId = state.deviceId;
	snapshot.instanceId = state.instanceId;
	snapshot.name = state.name;
	snapshot.axes = state.axes;
	snapshot.buttons = state.buttons;

	return snapshot;
}

std::vector<CControllerInput::ControllerStateSnapshot> CControllerInput::GetAvailableControllers() const
{
	std::vector<ControllerStateSnapshot> controllers;
	controllers.reserve(controllersByInstanceId.size());

	for (const auto& controllerIt : controllersByInstanceId) {
		controllers.push_back(MakeControllerStateSnapshot(controllerIt.second));
	}

	return controllers;
}

bool CControllerInput::GetControllerState(int instanceId, ControllerStateSnapshot& state) const
{
	auto controllerIt = controllersByInstanceId.find(instanceId);
	if (controllerIt == controllersByInstanceId.end()) {
		return false;
	}

	state = MakeControllerStateSnapshot(controllerIt->second);
	return true;
}

#ifndef HEADLESS

CControllerInput::CControllerInput()
{
	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
		LOG_L(L_WARNING, "[ControllerInput] Failed to initialize SDL joystick/gamecontroller subsystem: %s", SDL_GetError());
	} else {
		LOG_L(L_INFO, "[ControllerInput] SDL joystick/gamecontroller subsystem initialized");
	}

	SDL_GameControllerEventState(SDL_ENABLE);
	SDL_JoystickEventState(SDL_ENABLE);

	inputCon = input.AddHandler([this](const SDL_Event& event) {
		return this->HandleSDLControllerEvent(event);
	});

	LOG_L(L_INFO, "[ControllerInput] SDL_NumJoysticks at init: %d", SDL_NumJoysticks());
	ScanExistingControllers();

	LOG_L(L_INFO, "[ControllerInput] Initialized SDL controller input handler");
}

CControllerInput::~CControllerInput()
{
	for (auto& controllerIt : controllersByInstanceId) {
		auto& state = controllerIt.second;

		if (state.gameController != nullptr) {
			SDL_GameControllerClose(state.gameController);
			state.gameController = nullptr;
		}
	}

	controllersByInstanceId.clear();

	LOG_L(L_INFO, "[ControllerInput] Shutting down SDL controller input handler");
}

bool CControllerInput::HandleSDLControllerEvent(const SDL_Event& event)
{
	switch (event.type) {
		case SDL_CONTROLLERDEVICEADDED: {
			HandleDeviceAdded(event.cdevice.which);
		} break;

		case SDL_CONTROLLERDEVICEREMOVED: {
			HandleDeviceRemoved(event.cdevice.which);
		} break;

		case SDL_CONTROLLERDEVICEREMAPPED: {
			HandleDeviceRemapped(event.cdevice.which);
		} break;

		case SDL_CONTROLLERBUTTONDOWN: {
			HandleButtonDown(event.cbutton.which, event.cbutton.button, event.cbutton.state);
		} break;

		case SDL_CONTROLLERBUTTONUP: {
			HandleButtonUp(event.cbutton.which, event.cbutton.button, event.cbutton.state);
		} break;

		case SDL_CONTROLLERAXISMOTION: {
			HandleAxisMotion(event.caxis.which, event.caxis.axis, event.caxis.value);
		} break;

		default:
			break;
	}

	return false;
}

void CControllerInput::LogAvailableController(int deviceId) const
{
	if (SDL_IsGameController(deviceId)) {
		const char* name = SDL_GameControllerNameForIndex(deviceId);
		LOG_L(L_INFO, "[ControllerInput] SDL game controller available: deviceId=%d name=%s", deviceId, name != nullptr ? name : "unknown");
		return;
	}

	const char* name = SDL_JoystickNameForIndex(deviceId);
	LOG_L(L_INFO, "[ControllerInput] SDL joystick available but not game controller: deviceId=%d name=%s", deviceId, name != nullptr ? name : "unknown");
}

void CControllerInput::ScanExistingControllers()
{
	const int joystickCount = SDL_NumJoysticks();
	LOG_L(L_INFO, "[ControllerInput] Scanning existing SDL joysticks: count=%d", joystickCount);

	for (int deviceId = 0; deviceId < joystickCount; ++deviceId) {
		LogAvailableController(deviceId);

		if (SDL_IsGameController(deviceId)) {
			HandleDeviceAdded(deviceId);
			continue;
		}

		LOG_L(L_INFO, "[ControllerInput] Skipping existing non-game-controller device: deviceId=%d", deviceId);
	}
}

void CControllerInput::HandleDeviceAdded(int deviceId)
{
	LOG_L(L_INFO, "[ControllerInput] Controller device added: deviceId=%d", deviceId);
	LogAvailableController(deviceId);

	if (!SDL_IsGameController(deviceId)) {
		LOG_L(L_INFO, "[ControllerInput] Ignoring non-game-controller device: deviceId=%d", deviceId);
		return;
	}

	SDL_GameController* gameController = SDL_GameControllerOpen(deviceId);
	if (gameController == nullptr) {
		LOG_L(L_WARNING, "[ControllerInput] Failed to open SDL game controller: deviceId=%d error=%s", deviceId, SDL_GetError());
		return;
	}

	SDL_Joystick* joystick = SDL_GameControllerGetJoystick(gameController);
	const int instanceId = joystick != nullptr ? SDL_JoystickInstanceID(joystick) : -1;

	if (instanceId < 0) {
		LOG_L(L_WARNING, "[ControllerInput] Failed to get joystick instance id: deviceId=%d", deviceId);
		SDL_GameControllerClose(gameController);
		return;
	}

	ControllerState state;
	state.deviceId = deviceId;
	state.instanceId = instanceId;

	const char* name = SDL_GameControllerName(gameController);
	state.name = name != nullptr ? name : "unknown";
	state.gameController = gameController;

	auto existingIt = controllersByInstanceId.find(instanceId);
	if (existingIt != controllersByInstanceId.end() && existingIt->second.gameController != nullptr) {
		SDL_GameControllerClose(existingIt->second.gameController);
	}

	controllersByInstanceId[instanceId] = state;

	LOG_L(L_INFO, "[ControllerInput] Controller connected: deviceId=%d instanceId=%d name=%s", deviceId, instanceId, state.name.c_str());
}

void CControllerInput::HandleDeviceRemoved(int instanceId)
{
	LOG_L(L_INFO, "[ControllerInput] Controller device removed: instanceId=%d", instanceId);

	auto it = controllersByInstanceId.find(instanceId);
	if (it == controllersByInstanceId.end()) {
		return;
	}

	LOG_L(L_INFO, "[ControllerInput] Removed tracked controller: instanceId=%d name=%s", instanceId, it->second.name.c_str());

	if (it->second.gameController != nullptr) {
		SDL_GameControllerClose(it->second.gameController);
		it->second.gameController = nullptr;
	}

	controllersByInstanceId.erase(it);
}

void CControllerInput::HandleDeviceRemapped(int instanceId)
{
	LOG_L(L_INFO, "[ControllerInput] Controller remapped: instanceId=%d", instanceId);
}

void CControllerInput::HandleButtonDown(int instanceId, int buttonId, std::uint8_t value)
{
	auto controllerIt = controllersByInstanceId.find(instanceId);
	if (controllerIt == controllersByInstanceId.end()) {
		return;
	}

	auto& state = controllerIt->second;

	if (buttonId >= 0 && buttonId < static_cast<int>(state.buttons.size())) {
		state.buttons[buttonId] = value;
	}

#if CONTROLLER_INPUT_LOG_EVENTS
	LOG_L(L_INFO, "[ControllerInput] ButtonDown: instanceId=%d buttonId=%d value=%u", instanceId, buttonId, static_cast<unsigned int>(value));
#endif
}

void CControllerInput::HandleButtonUp(int instanceId, int buttonId, std::uint8_t value)
{
	auto controllerIt = controllersByInstanceId.find(instanceId);
	if (controllerIt == controllersByInstanceId.end()) {
		return;
	}

	auto& state = controllerIt->second;

	if (buttonId >= 0 && buttonId < static_cast<int>(state.buttons.size())) {
		state.buttons[buttonId] = value;
	}

#if CONTROLLER_INPUT_LOG_EVENTS
	LOG_L(L_INFO, "[ControllerInput] ButtonUp: instanceId=%d buttonId=%d value=%u", instanceId, buttonId, static_cast<unsigned int>(value));
#endif
}

void CControllerInput::HandleAxisMotion(int instanceId, int axisId, std::int16_t value)
{
	auto controllerIt = controllersByInstanceId.find(instanceId);
	if (controllerIt == controllersByInstanceId.end()) {
		return;
	}

	auto& state = controllerIt->second;

	if (axisId >= 0 && axisId < static_cast<int>(state.axes.size())) {
		state.axes[axisId] = value;
	}

#if CONTROLLER_INPUT_LOG_EVENTS
	LOG_L(L_INFO, "[ControllerInput] AxisMotion: instanceId=%d axisId=%d value=%d", instanceId, axisId, static_cast<int>(value));
#endif
}

#else

CControllerInput::CControllerInput() = default;
CControllerInput::~CControllerInput() = default;

bool CControllerInput::HandleSDLControllerEvent(const SDL_Event&)
{
	return false;
}

void CControllerInput::LogAvailableController(int) const {}
void CControllerInput::ScanExistingControllers() {}
void CControllerInput::HandleDeviceAdded(int) {}
void CControllerInput::HandleDeviceRemoved(int) {}
void CControllerInput::HandleDeviceRemapped(int) {}
void CControllerInput::HandleButtonDown(int, int, std::uint8_t) {}
void CControllerInput::HandleButtonUp(int, int, std::uint8_t) {}
void CControllerInput::HandleAxisMotion(int, int, std::int16_t) {}

#endif
