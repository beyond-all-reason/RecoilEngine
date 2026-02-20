/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FontHandler.h"

#include "System/Config/ConfigHandler.h"

CONFIG(bool, TextDisableOldColorIndicators).defaultValue(false).description("Disable support for old color indicators. The old color indicators don't allow writing some characters.");

CFontHandler fontHandler;


CFontHandler::CFontHandler()
{
}


bool CFontHandler::Init()
{
	if (configHandler != nullptr)
		disableOldColorIndicators = configHandler->GetBool("TextDisableOldColorIndicators");
	return true;
}


