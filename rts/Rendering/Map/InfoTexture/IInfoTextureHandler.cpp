/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IInfoTextureHandler.h"
#include "Modern/InfoTextureHandler.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GL/FBO.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"

#include "System/Misc/TracyDefs.h"

CONFIG(bool, HighResLos).deprecated(true);
CONFIG(int, ExtraTextureUpdateRate).deprecated(true);

decltype(infoTextureHandler) infoTextureHandler = nullptr;

void IInfoTextureHandler::Create()
{
	RECOIL_DETAILED_TRACY_ZONE;
	try {
#ifdef HEADLESS
		infoTextureHandler = std::make_unique<CNullInfoTextureHandler>();
#else
		infoTextureHandler = std::make_unique<CInfoTextureHandler>();
#endif
	} catch (const opengl_error& glerr) {
		infoTextureHandler = nullptr;
		throw glerr;
	}

	LOG("InfoTexture: shaders");
}