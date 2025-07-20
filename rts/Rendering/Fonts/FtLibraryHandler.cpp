/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

#include "FontHandler.h"
#include "FtLibraryHandler.h"
#include "FontLogSection.h"
#include "FtIncludes.h"

#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "fmt/printf.h"

#include "System/Misc/TracyDefs.h"
#ifdef _WIN32
	#include <windows.h>	// ExpandEnvironmentStrings
#endif

#ifndef HEADLESS
class FtLibraryHandler {
public:
	FtLibraryHandler()
		: config(nullptr)
		, lib(nullptr)
		#ifdef USE_FONTCONFIG
		, gameFontSet(nullptr)
		, basePattern(nullptr)
		#endif // USE_FONTCONFIG
	{
		const FT_Error error = FT_Init_FreeType(&lib);

		if (error != 0) {
			FT_Int version[3];
			FT_Library_Version(lib, &version[0], &version[1], &version[2]);

			std::string err = fmt::sprintf("[%s] FT_Init_FreeType failure (version %d.%d.%d) \"%s\"",
						       __func__, version[0], version[1], version[2], GetFTError(error));
			throw std::runtime_error(err);
		}
	}

	~FtLibraryHandler() {
		FT_Done_FreeType(lib);

		#ifdef USE_FONTCONFIG
		if (!config)
			return;

		FcConfigDestroy(config);
		if (gameFontSet) {
			FcFontSetDestroy(gameFontSet);
		}
		if (basePattern) {
			FcPatternDestroy(basePattern);
		}
		FcFini();
		config = nullptr;
		#endif
	}

	bool InitFontconfig(bool console) {
		#ifdef USE_FONTCONFIG
		auto LOG_MSG = [console](const std::string& fmt, bool isError, auto&&... args) {
			if (console) {
				std::string fmtNL = fmt + "\n";
				printf(fmtNL.c_str(), args...);
			}
			else {
				if (isError) {
					LOG_L(L_ERROR, fmt.c_str(), args...);
				}
				else {
					LOG(fmt.c_str(), args...);
				}
			}
		};

		if (!UseFontConfig())
			return false;

		{
			std::string msg = fmt::sprintf("%s::FontConfigInit (version %d.%d.%d)", __func__, FC_MAJOR, FC_MINOR, FC_REVISION);
			ScopedOnceTimer timer(msg);
			ZoneScopedNC("FtLibraryHandler::FontConfigInit", tracy::Color::Purple);

			FcBool res;
			std::string errprefix = fmt::sprintf("[%s] Fontconfig(version %d.%d.%d) failed to initialize", __func__, FC_MAJOR, FC_MINOR, FC_REVISION);

			// init configuration
			FcConfigEnableHome(FcFalse);
			config = FcConfigCreate();

			// we cant directly use the usual fontconfig methods because those won't let us have both first our cache
			// and system fonts included. also linux actually has system config files that can be used by fontconfig.

			#ifdef _WIN32
			static constexpr auto winFontPath = "%WINDIR%\\fonts";
			const int neededSize = ExpandEnvironmentStrings(winFontPath, nullptr, 0);
			std::vector <char> osFontsDir (neededSize);
			ExpandEnvironmentStrings(winFontPath, osFontsDir.data(), osFontsDir.size());

			static constexpr const char* configFmt = R"(<fontconfig><dir>%s</dir><cachedir>fontcache</cachedir></fontconfig>)";
			const std::string configFmtVar = fmt::sprintf(configFmt, osFontsDir.data());
			#else
			const std::string configFmtVar = R"(<fontconfig><cachedir>fontcache</cachedir></fontconfig>)";
			#endif

			#ifdef _WIN32
			// Explicitly set the config with xml for windows.
			res = FcConfigParseAndLoadFromMemory(config, reinterpret_cast<const FcChar8*>(configFmtVar.c_str()), FcTrue);
			#else
			// Load system configuration (passing 0 here so fc will use the default os config file if possible).
			res = FcConfigParseAndLoad(config, 0, true);
			#endif
			if (res) {
				#ifndef _WIN32
				// add local cache after system config for linux
				FcConfigParseAndLoadFromMemory(config, reinterpret_cast<const FcChar8*>(configFmtVar.c_str()), FcTrue);
				#endif

				LOG_MSG("[%s] Using Fontconfig light init", false, __func__);

				// build system fonts
				res = FcConfigBuildFonts(config);
				if (!res) {
					LOG_MSG("%s fonts", true, errprefix.c_str());
					InitFailed();
					return false;
				}
			} else {
				// Can't load step by step to use our cache, so retry with general
				// fontconfig init method, that has a few extra fallbacks.

				// Init everything. Normally this would be enough, but the method before
				// accounts for situations where system config is borked due to incompatible
				// lib and system config files.
				FcConfig *fcConfig = FcInitLoadConfigAndFonts();
				if (fcConfig) {
					FcConfigDestroy(config); // release previous config
					config = fcConfig;

					// add our cache at the back of the new config.
					FcConfigParseAndLoadFromMemory(config, reinterpret_cast<const FcChar8*>(configFmtVar.c_str()), FcTrue);
				} else {
					LOG_MSG("%s config and fonts. No system fallbacks will be available", false, errprefix.c_str());
				}
			}

			gameFontSet = FcFontSetCreate();
			basePattern = FcPatternCreate();

			// init app fonts dir
			res = FcConfigAppFontAddDir(config, reinterpret_cast<const FcChar8*>("fonts"));
			if (!res) {
				LOG_MSG("%s font dir", true, errprefix.c_str());
				InitFailed();
				return false;
			}

			// print cache dirs
			auto dirs = FcConfigGetCacheDirs(config);
			FcStrListFirst(dirs);
			for (FcChar8* dir = FcStrListNext(dirs); dir != nullptr; dir = FcStrListNext(dirs)) {
				LOG_MSG("[%s] Using Fontconfig cache dir \"%s\"", false, __func__, dir);
			}
			FcStrListDone(dirs);
		}

		#endif // USE_FONTCONFIG

		return true;
	}

	void InitFailed() {
		FcConfigDestroy(config);
		FcFini();
		config = nullptr;
	}
	static bool InitSingletonFontconfig(bool console) { return singleton->InitFontconfig(console); }

	static bool UseFontConfig() { return fontHandler.useFontConfigLib; }

	#ifdef USE_FONTCONFIG
	// command-line CheckGenFontConfigFull invocation checks
	static bool CheckFontConfig() { return (UseFontConfig() && FcConfigUptoDate(GetFCConfig())); }
	#else

	static bool CheckFontConfig() { return false; }
	static bool CheckGenFontConfig(bool fromCons) { return false; }
	#endif

	static FT_Library& GetLibrary() {
		if (singleton == nullptr)
			singleton = std::make_unique<FtLibraryHandler>();

		return singleton->lib;
	};
	static FcConfig* GetFCConfig() {
		if (singleton == nullptr)
			singleton = std::make_unique<FtLibraryHandler>();

		return singleton->config;
	}
	static inline bool CanUseFontConfig() {
		return GetFCConfig() != nullptr;
	}
	#ifdef USE_FONTCONFIG
	static FcFontSet *GetGameFontSet() {
		return singleton->gameFontSet;
	}
	static FcPattern *GetBasePattern() {
		return singleton->basePattern;
	}
	static void ClearGameFontSet() {
		FcFontSetDestroy(singleton->gameFontSet);
		singleton->gameFontSet = FcFontSetCreate();
	}
	static void ClearBasePattern() {
		FcPatternDestroy(singleton->basePattern);
		singleton->basePattern = FcPatternCreate();
	}
	#endif
private:
	FcConfig* config;
	FT_Library lib;
	#ifdef USE_FONTCONFIG
	FcFontSet *gameFontSet;
	FcPattern *basePattern;
	#endif

	static inline std::unique_ptr<FtLibraryHandler> singleton = nullptr;
};
#endif



void FtLibraryHandlerProxy::InitFtLibrary()
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	FtLibraryHandler::GetLibrary();
#endif
}

bool FtLibraryHandlerProxy::InitFontconfig(bool console)
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifndef HEADLESS
	return FtLibraryHandler::InitSingletonFontconfig(console);
#else
	return false;
#endif
}

bool FtLibraryHandlerProxy::CanUseFontConfig()
{
#ifndef HEADLESS
	return FtLibraryHandler::CanUseFontConfig();
#else
	return false;
#endif
}

FcConfig* FtLibraryHandlerProxy::GetFCConfig() {
#ifndef HEADLESS
	return FtLibraryHandler::GetFCConfig();
#else
	return nullptr;
#endif
}

#ifndef HEADLESS
FcFontSet* FtLibraryHandlerProxy::GetGameFontSet() {
	return FtLibraryHandler::GetGameFontSet();
}

FcPattern* FtLibraryHandlerProxy::GetBasePattern() {
	return FtLibraryHandler::GetBasePattern();
}

void FtLibraryHandlerProxy::ClearGameFontSet() {
	FtLibraryHandler::ClearGameFontSet();
}

void FtLibraryHandlerProxy::ClearBasePattern() {
	FtLibraryHandler::ClearBasePattern();
}

int FtLibraryHandlerProxy::NewMemoryFace(const unsigned char* file_base, signed long file_size, signed long face_index, FT_Face *aface)
{
	FT_Error error = FT_New_Memory_Face(FtLibraryHandler::GetLibrary(), file_base, file_size, face_index, aface);
	return error;
}
#endif

