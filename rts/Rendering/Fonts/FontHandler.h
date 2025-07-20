/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _FONT_HANDLER_H
#define _FONT_HANDLER_H

#include "System/Misc/NonCopyable.h"

class CFontHandler : public spring::noncopyable
{
public:
	CFontHandler();
	bool Init(bool console);
	bool FullInit();
	void Kill();

	bool disableOldColorIndicators = false;
	int maxFontTries = 5;
	int maxPinnedFonts = 10;
	bool allowColorFonts = false;
	bool useFontConfigLib = true;
	bool searchSystemFonts = true;
	bool searchFontAttributes = true;
	bool searchApplySubstitutions = true;

private:
	void LoadConfig();
};

extern CFontHandler fontHandler;


#endif // _FONT_HANDLER_H
