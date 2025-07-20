/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#ifndef _FT_INCLUDES_H
#define _FT_INCLUDES_H

#ifdef HEADLESS
	typedef unsigned char FT_Byte;
#else  // not HEADLESS
	#include <ft2build.h>
	#include FT_FREETYPE_H
	#ifdef USE_FONTCONFIG
		#include <fontconfig/fontconfig.h>
		#include <fontconfig/fcfreetype.h>
	#endif

	// FtError helper code
	#include <algorithm>
	#include <iterator>

	#undef __FTERRORS_H__
	#define FT_ERRORDEF( e, v, s )  { e, s },
	#define FT_ERROR_START_LIST     {
	#define FT_ERROR_END_LIST       { 0, 0 } };
	struct FTErrorRecord {
		int          err_code;
		const char*  err_msg;
	} static errorTable[] =
	#include FT_ERRORS_H

	struct IgnoreMe {}; // MSVC IntelliSense is confused by #include FT_ERRORS_H above. This seems to fix it.

	static const char* GetFTError(FT_Error e) {
		const auto it = std::find_if(std::begin(errorTable), std::end(errorTable), [e](FTErrorRecord er) { return er.err_code == e; });
		if (it != std::end(errorTable))
			return it->err_msg;

		return "Unknown error";
	}
#endif // HEADLESS

#endif // FT_INCLUDES_H
