/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FontHandler.h"

#include "System/Config/ConfigHandler.h"

CONFIG(bool, TextUseNewColorIndicators).defaultValue(false).description("Old color indicators don't allow writing some characters.");

CFontHandler fontHandler;


CFontHandler::CFontHandler()
{
}


bool CFontHandler::Init()
{
	useNewColorIndicators = configHandler->GetBool("TextUseNewColorIndicators");
	return true;
}

