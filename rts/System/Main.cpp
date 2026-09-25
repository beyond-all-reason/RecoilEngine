/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
	\mainpage
	This is the documentation of the Spring RTS Engine.
	https://springrts.com/
*/

#include "System/ExportDefines.h"
#include "System/SpringApp.h"
#include "System/Exceptions.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Platform/errorhandler.h"
#include "System/Platform/Threading.h"
#include "System/Platform/Misc.h"
#include "System/Log/ILog.h"

#include <nowide/args.hpp>

#include <clocale>
#include <cstdlib>
#include <cstdint>

#ifdef _WIN32
	#include "lib/SOP/SOP.hpp" // NvOptimus
#endif

// https://stackoverflow.com/a/27881472/9819318
EXTERNALIZER_B EXPORT_CLAUSE uint32_t NvOptimusEnablement =                  1; EXTERNALIZER_E //Optimus/NV use discrete GPU hint
EXTERNALIZER_B EXPORT_CLAUSE uint32_t AmdPowerXpressRequestHighPerformance = 1; EXTERNALIZER_E // AMD use discrete GPU hint

int Run(int argc, char* argv[])
{
#ifdef __MINGW32__
	// For the MinGW backtrace() implementation we need to know the stack end.
	{
		extern void* stack_end;
		char here;
		stack_end = (void*) &here;
	}
#endif
	// already the default, but be explicit for locale-dependent functions (atof,strtof,...)
	setlocale(LC_ALL, "C");

	Threading::DetectCores();
	Threading::SetMainThread();

	LOG("%s: thread affinity %x", __func__, Threading::GetAffinity());
	SpringApp app(argc, argv);
	LOG("%s: thread affinity %x", __func__, Threading::GetAffinity());
	return (app.Run());
}


#if defined(_WIN32) && !defined(HEADLESS)
// Declare per-monitor DPI awareness before any window exists. Without it Windows treats
// the process as DPI-unaware (the bundled SDL predates SDL_HINT_WINDOWS_DPI_AWARENESS)
// and DWM bitmap-stretches the window on every monitor whose scale factor differs from
// the primary's. A DualScreenMode window spanning mixed-DPI displays then shows up as a
// magnified crop on the secondary screen. Resolved dynamically, same pattern as the
// RmlUi Win32 backend: the context API needs Win10 1703, the shcore fallback Win8.1,
// the last resort Vista.
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((HANDLE) -4)
#endif

static void SetDpiAwareness()
{
	using SetCtxFn = BOOL (WINAPI*)(HANDLE);
	if (const auto setCtx = (SetCtxFn) (void*) GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "SetProcessDpiAwarenessContext"); setCtx != nullptr) {
		if (setCtx(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
			return;
	}

	// PROCESS_PER_MONITOR_DPI_AWARE from shellscalingapi.h, spelled locally so the
	// header (Win8.1+) is not required at build time.
	constexpr int processPerMonitorDpiAware = 2;

	using SetPmFn = HRESULT (WINAPI*)(int);
	if (HMODULE shcore = LoadLibrary(TEXT("shcore.dll")); shcore != nullptr) {
		if (const auto setPm = (SetPmFn) (void*) GetProcAddress(shcore, "SetProcessDpiAwareness"); setPm != nullptr) {
			if (SUCCEEDED(setPm(processPerMonitorDpiAware)))
				return;
		}
		FreeLibrary(shcore);
	}

	SetProcessDPIAware();
}
#endif


/**
 * Always run on dedicated GPU
 * @return true when restart is required with new env vars
 */
#if !defined(PROFILE) && !defined(HEADLESS)
static bool SetNvOptimusProfile(const std::string& processFileName)
{
#ifdef _WIN32
	if (SOP_CheckProfile("Spring"))
		return false;

	// sic; on Windows execvp spawns a new process which breaks lobby state-tracking by PID
	return (SOP_SetProfile("Spring", processFileName) == SOP_RESULT_CHANGE, false);
#endif
	return false;
}
#endif



/**
 * @brief main
 * @return exit code
 * @param argc argument count
 * @param argv array of argument strings
 *
 * Main entry point function
 */
int main(int argc, char* argv[])
{
	nowide::args a(argc, argv); // Fix arguments - make them UTF-8

#if defined(_WIN32) && !defined(HEADLESS)
	// Before the first window; the process DPI awareness can only be set once.
	SetDpiAwareness();
#endif

// PROFILE builds exit on execv, HEADLESS does not use the GPU
#if !defined(PROFILE) && !defined(HEADLESS)
#define MAX_ARGS 32

	if (SetNvOptimusProfile(FileSystem::GetFilename(argv[0]))) {
		// prepare for restart
		std::array<std::string, MAX_ARGS> args;

		for (int i = 0, n = std::min(argc, MAX_ARGS); i < n; i++)
			args[i] = argv[i];

		// ExecProc normally does not return; if it does the retval is an error-string
		ErrorMessageBox(Platform::ExecuteProcess(args), "Execv error:", MBF_OK | MBF_EXCL);
	}
#undef MAX_ARGS
#endif

	return (Run(argc, argv));
}



#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstanceIn, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	return main(__argc, __argv);
}
#endif

