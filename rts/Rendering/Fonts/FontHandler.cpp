/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "FontHandler.h"
#include "FtLibraryHandler.h"
#include "CFontTexture.h"
#include "glFont.h"

#include "System/Config/ConfigHandler.h"

CONFIG(bool, UseFontConfigLib).defaultValue(true).description("Whether the system fontconfig library (if present and enabled at compile-time) should be used for handling fonts.");
CONFIG(bool, UseFontConfigSystemFonts).defaultValue(true).description("Whether the system fonts will be searched by fontconfig.");
CONFIG(bool, FontConfigSearchAttributes).defaultValue(true).description("Whether the font characteristics will used to refine the search by fontconfig. Results in better glyph matches in some cases, but has a nontrivial performance cost.");
CONFIG(bool, FontConfigApplySubstitutions).defaultValue(true).description("[EXPERIMENTAL] In case it's disabled FcConfigSubstitute is not getting called, this might break non-ASCII font rendering.");
CONFIG(int, MaxFontTries).defaultValue(5).description("Represents the maximum number of attempts to search for a glyph replacement using the FontConfig library (lower = foreign glyphs may fail to render, higher = searching for foreign glyphs can lag the game).");
CONFIG(int, MaxPinnedFonts).defaultValue(10).description("Maximum number of fonts to pin to cache. Increasing this will eventually use more memory, but can alleviate processing spikes when rendering new glyphs.");
CONFIG(bool, AllowColorFonts).defaultValue(false).description("Allow working with colored fonts (experimental).");
CONFIG(bool, TextDisableOldColorIndicators).defaultValue(false).description("Disable support for old color indicators. The old color indicators don't allow writing some characters.");

CFontHandler fontHandler;


CFontHandler::CFontHandler()
{
}


bool CFontHandler::Init(bool console)
{
	FtLibraryHandlerProxy::InitFtLibrary();

	return FtLibraryHandlerProxy::InitFontconfig(console);
}

bool CFontHandler::FullInit()
{
	assert(configHandler != nullptr);

	LoadConfig();
	Init(false);

	CFontTexture::InitFonts();
	return CglFont::LoadConfigFonts();
}

void CFontHandler::LoadConfig()
{
	maxFontTries = configHandler->GetInt("MaxFontTries");
	maxPinnedFonts = configHandler->GetInt("MaxPinnedFonts");
	disableOldColorIndicators = configHandler->GetBool("TextDisableOldColorIndicators");
	allowColorFonts = configHandler->GetBool("AllowColorFonts");
	useFontConfigLib = configHandler->GetBool("UseFontConfigLib");
	searchSystemFonts = configHandler->GetBool("UseFontConfigSystemFonts");
	searchFontAttributes = configHandler->GetBool("FontConfigSearchAttributes");
	searchApplySubstitutions = configHandler->GetBool("FontConfigApplySubstitutions");
}

void CFontHandler::Kill()
{
	font      = {};
	smallFont = {};

	CFontTexture::KillFonts();
}
