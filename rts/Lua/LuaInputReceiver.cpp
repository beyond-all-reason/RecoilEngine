/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <string>

#include "LuaInputReceiver.h"
#include "System/EventHandler.h"


CLuaInputReceiver::CLuaInputReceiver()
: CInputReceiver(MANUAL)
{
}


CLuaInputReceiver::~CLuaInputReceiver() = default;


CLuaInputReceiver* CLuaInputReceiver::GetInstace()
{
	static CLuaInputReceiver instance;

	return &instance;
}


bool CLuaInputReceiver::KeyPressedSC(int keyScanCode, int keySym, bool isRepeat)
{
	return eventHandler.KeyPressSC(keyScanCode, isRepeat) || eventHandler.KeyPress(keySym, isRepeat);
}


bool CLuaInputReceiver::KeyReleasedSC(int keyScanCode, int keySym)
{
	return eventHandler.KeyReleaseSC(keyScanCode) || eventHandler.KeyRelease(keySym);
}


bool CLuaInputReceiver::MousePress(int x, int y, int button)
{
	return eventHandler.MousePress(x, y, button);
}


void CLuaInputReceiver::MouseMove(int x, int y, int dx, int dy, int button)
{
	eventHandler.MouseMove(x, y, dx, dy, button);
}


void CLuaInputReceiver::MouseRelease(int x, int y, int button)
{
	eventHandler.MouseRelease(x, y, button);
}


bool CLuaInputReceiver::IsAbove(int x, int y)
{
	return eventHandler.IsAbove(x, y);
}


std::string CLuaInputReceiver::GetTooltip(int x, int y)
{
	return eventHandler.GetTooltip(x, y);
}


void CLuaInputReceiver::Draw()
{
	return eventHandler.DrawScreen();
}
