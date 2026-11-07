/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <SDL.h>
#include <functional>

#include "Rendering/GL/myGL.h"
#include "LoadScreen.h"
#include "Game.h"
#include "GlobalUnsynced.h"
#include "Game/Players/Player.h"
#include "Game/Players/PlayerHandler.h"
#include "Game/UI/MouseHandler.h"
#include "ExternalAI/SkirmishAIHandler.h"
#include "Lua/LuaIntro.h"
#include "Lua/LuaMenu.h"
#include "Map/MapInfo.h"
#include "Rendering/Fonts/glFont.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/VerticalSync.h"
#include "Rendering/GL/RenderBuffers.h"
#include "Rendering/GL/StreamBuffer.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Path/IPathManager.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/Sync/FPUCheck.h"
#include "System/Log/ILog.h"
#include "Net/Protocol/NetProtocol.h"
#include "System/Misc/UnfreezeSpring.h"
#include "System/Matrix44f.h"
#include "System/SafeUtil.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/Watchdog.h"
#include "System/Platform/Threading.h"
#include "System/Sound/ISound.h"
#include "System/Sound/ISoundChannels.h"
#include "System/LoadLock.h"

#if !defined(HEADLESS) && !defined(NO_SOUND)
#include "System/Sound/OpenAL/EFX.h"
#include "System/Sound/OpenAL/EFXPresets.h"
#endif

#include <vector>

#include "System/Misc/TracyDefs.h"

CONFIG(int, LoadingMT)
	.description(
		"Experimental option to load the game in separate thread. Expect visual glitches, crashes and deadlocks."
		" 1: the load screen and the loading thread take turns with one GL context."
		" 2: the load screen draws in its own GL context, independently of the loading thread;"
		" LuaIntro gets no events from the load (e.g. GamePreload) and LuaMenu is not updated until it is done."
	)
	.defaultValue(0)
	.safemodeValue(0);


CLoadScreen* CLoadScreen::singleton = nullptr;

CLoadScreen::CLoadScreen(std::string&& _mapFileName, std::string&& _modFileName, ILoadSaveHandler* _saveFile)
	: saveFile(_saveFile)

	, mapFileName(std::move(_mapFileName))
	, modFileName(std::move(_modFileName))

	, mtLoading(true)
	, lastDrawTime(0)
{
}

CLoadScreen::~CLoadScreen()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// Kill() must have been called first, such that the loading
	// thread can not access singleton while its dtor is running
	assert(!gameLoadThread.joinable());

	if (clientNet != nullptr)
		clientNet->KeepUpdating(false);
	if (netHeartbeatThread.joinable())
		netHeartbeatThread.join();

	if (!gu->globalQuit) {
		activeController = game;

		if (luaMenu != nullptr)
			luaMenu->ActivateGame();
	}

	if (activeController == this)
		activeController = nullptr;
}


bool CLoadScreen::Init()
{
	RECOIL_DETAILED_TRACY_ZONE;
	activeController = this;

	// When calling this function, mod archives have to be loaded
	// and gu->myPlayerNum has to be set.
	skirmishAIHandler.LoadPreGame();


#ifdef HEADLESS
	mtLoading = false;
	splitContext = false;
#else
	const int mtCfg = configHandler->GetInt("LoadingMT");
	// user override
	mtLoading = (mtCfg > 0);
	splitContext = (mtCfg >= 2);
#endif


	// Create a thread during the loading that pings the host/server, so it knows that this client is still alive/loading
	clientNet->KeepUpdating(true);

	netHeartbeatThread = spring::thread(Threading::CreateNewThread(std::bind(&CNetProtocol::UpdateLoop, clientNet)));
	try {
		game = new CGame(mapFileName, modFileName, saveFile);
	} catch (...) {
		// CGame publishes itself before parsing startup content. Its destructor is
		// not called when construction fails, so do not leave a dangling global.
		game = nullptr;
		throw;
	}

	if (splitContext && !globalRendering->CreateIntroContext()) {
		LOG_L(L_WARNING, "[LoadScreen::%s] LoadingMT=2 falls back to 1", __func__);
		splitContext = false;
	}

	CglFont::sync.SetThreadSafety(mtLoading);
	// LoadingMT=2: the loading thread keeps the game's context for the whole load and
	// nothing else takes it, so CLoadLock has nothing to hand over
	CLoadLock::SetThreadSafety(mtLoading && !splitContext);

	// a swap that waits for vblank holds up the loading thread: in LoadingMT=1 the swap holds
	// CLoadLock, in LoadingMT=2 the driver stalls the other context (60% longer loads with
	// NVIDIA). Frames are paced in Update instead. Set while this thread has a context.
	if (mtLoading)
		verticalSync->Suspend();

	if (splitContext) {
		// one frame per display refresh
		constexpr int FALLBACK_REFRESH_RATE = 60;

		SDL_DisplayMode mode;
		const int displayIndex = SDL_GetWindowDisplayIndex(globalRendering->GetWindow());
		const bool haveMode = (displayIndex >= 0 && SDL_GetCurrentDisplayMode(displayIndex, &mode) == 0 && mode.refresh_rate > 0);
		const int refreshRate = haveMode ? mode.refresh_rate : FALLBACK_REFRESH_RATE;
		introFramePeriod = spring_time::fromMicroSecs(1000000 / refreshRate);
		nextFrameTime = spring_gettime();

		// before the loading thread exists: LuaIntro takes itself out of eventHandler,
		// which the loading thread broadcasts through
		CLuaIntro::LoadFreeHandler();

		gameLoadThread = CGameLoadThread(std::bind(&CLoadScreen::LoadInGameContext, this));

		while (!Watchdog::HasThread(WDT_LOAD));
		return true;
	}

	if (mtLoading) {
		try {
			// create the game-loading thread; rebinds primary context to hidden window
			gameLoadThread = CGameLoadThread(std::bind(&CGame::Load, game, mapFileName));

			while (!Watchdog::HasThread(WDT_LOAD));
		} catch (const opengl_error& gle) {
			LOG_L(L_WARNING, "[LoadScreen::%s] offscreen GL context creation failed (error: \"%s\")", __func__, gle.what());

			mtLoading = false;
			CglFont::sync.SetThreadSafety(false);
			CLoadLock::SetThreadSafety(false);
			verticalSync->Resume();
		}
	}

	// LuaIntro must be loaded and killed in the same thread (main)
	// and bound context (secondary) that CLoadScreen::Draw runs in
	//
	// note that it has access to gl.LoadFont (which creates a user
	// data wrapping a local font) but also to gl.*Text (which uses
	// the global font), the latter will cause problems in GL4
	{
		auto lock = CLoadLock::GetUniqueLock();
		CLuaIntro::LoadFreeHandler();
	}

	if (mtLoading)
		return true;

	LOG("[LoadScreen::%s] single-threaded", __func__);
	game->Load(mapFileName);
	return false;
}

void CLoadScreen::Kill()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (mtLoading && !gameLoadThread.joinable() && !splitContext) {
		// the loading thread was never started, this thread still has the context
		verticalSync->Resume();
		return;
	}

	// before LuaIntro is freed: on a quit during the load, the loading thread may still be
	// registering Lua handles and eventHandler clients, which freeing LuaIntro also touches
	gameLoadThread.join();

	// LoadingMT=2: still in the load screen's own context, so what LuaIntro made there
	// (VAOs, FBOs, its fonts) is released in it
	if (luaIntro != nullptr)
		luaIntro->Shutdown();

	CLuaIntro::FreeHandler();

	// LoadingMT=2: done with it; the loading thread released the game's context as it exited
	globalRendering->DestroyIntroContext();

	CFontTexture::sync.SetThreadSafety(false);
	CLoadLock::SetThreadSafety(false);
	// set last time and forever
	globalRendering->MakeCurrentContext(false);
	globalRendering->ToggleMultisampling();
	// also applies a VSync change made during the load (e.g. by LuaIntro), which could
	// not reach the context then
	verticalSync->Resume();

	// LoadingMT=2: a resize during the load only reached the load screen; now that the
	// game's context is back, let the regular handler do the rest
	if (resizedDuringLoad) {
		SDL_Event event = {};
		event.type = SDL_WINDOWEVENT;
		event.window.event = SDL_WINDOWEVENT_SIZE_CHANGED;
		event.window.windowID = SDL_GetWindowID(globalRendering->GetWindow());
		SDL_GetWindowSize(globalRendering->GetWindow(), &event.window.data1, &event.window.data2);
		SDL_PushEvent(&event);
	}
}


void CLoadScreen::LoadInGameContext()
{
	// LoadingMT=2: the game's context stays with this thread for the whole load
	if (!globalRendering->MakeLoadThreadContext(false))
		LOG_L(L_ERROR, "[LoadScreen::%s] the loading thread could not take the game's GL context (\"%s\")", __func__, SDL_GetError());

	globalRendering->ToggleMultisampling();

	// released on every way out (CGame::Load can throw), so the main thread can take it back
	struct ContextRelease {
		~ContextRelease() {
			// fences for the stream buffers the load set up, in the context that uses them
			IStreamBufferConcept::PutBufferLocks();
			globalRendering->MakeLoadThreadContext(true);
		}
	} contextRelease;

	game->Load(mapFileName);
}


/******************************************************************************/

static void FinishedLoading()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (gu->globalQuit)
		return;

	// send our playername to the server to indicate we finished loading
	const CPlayer* p = playerHandler.Player(gu->myPlayerNum);

	clientNet->Send(CBaseNetProtocol::Get().SendPlayerName(gu->myPlayerNum, p->name));
	#ifdef SYNCCHECK
	clientNet->Send(CBaseNetProtocol::Get().SendPathCheckSum(gu->myPlayerNum, pathManager->GetPathCheckSum()));
	#endif
	mouse->ShowMouse();

	#if !defined(HEADLESS) && !defined(NO_SOUND)
	// NB: sound is initialized at this point, but EFX support is *not* guaranteed
	efx.CommitEffects(mapInfo->efxprops);
	#endif
}


void CLoadScreen::CreateDeleteInstance(std::string&& mapFileName, std::string&& modFileName, ILoadSaveHandler* saveFile)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (CreateInstance(std::move(mapFileName), std::move(modFileName), saveFile))
		return;

	// not mtLoading, Game::Load has completed and we can go
	DeleteInstance();
	FinishedLoading();
}

bool CLoadScreen::CreateInstance(std::string&& mapFileName, std::string&& modFileName, ILoadSaveHandler* saveFile)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(singleton == nullptr);
	singleton = new CLoadScreen(std::move(mapFileName), std::move(modFileName), saveFile);

	// returns true when mtLoading, false otherwise
	return (singleton->Init());
}

void CLoadScreen::DeleteInstance()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (singleton == nullptr)
		return;

	singleton->Kill();
	spring::SafeDelete(singleton);
}


/******************************************************************************/

void CLoadScreen::ResizeEvent()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (splitContext)
		resizedDuringLoad = true;

	if (luaIntro != nullptr)
		luaIntro->ViewResize();
}


int CLoadScreen::KeyPressed(int keyCode, int scanCode, bool isRepeat)
{
	RECOIL_DETAILED_TRACY_ZONE;
	//FIXME add mouse events
	if (luaIntro != nullptr)
		luaIntro->KeyPress(keyCode, scanCode, isRepeat);

	return 0;
}

int CLoadScreen::KeyReleased(int keyCode, int scanCode)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (luaIntro != nullptr)
		luaIntro->KeyRelease(keyCode, scanCode);

	return 0;
}


bool CLoadScreen::Update()
{
	ZoneScoped;

	if (luaIntro != nullptr) {
		// keep checking this while we are the active controller
		std::lock_guard<spring::recursive_mutex> lck(mutex);

		for (const auto& pair: loadMessages) {
			good_fpu_control_registers(pair.first.c_str());
			luaIntro->LoadProgress(pair.first, pair.second);
		}

		loadMessages.clear();
	}

	if (game->IsDoneLoading()) {
		CLoadScreen::DeleteInstance();
		FinishedLoading();
		return true;
	}

	// without this call the window manager would think the window is unresponsive and thus ask for hard kill
	if (!mtLoading)
		spring::UnfreezeSpring(WDT_LOAD);

	// LoadingMT=2: one frame per display refresh, see DrawIntroFrame
	if (splitContext) {
		const spring_time now = spring_gettime();

		if (nextFrameTime > now)
			spring_sleep(nextFrameTime - now);

		// a late frame moves the schedule instead of bunching the next ones up
		nextFrameTime = std::max(nextFrameTime, now) + introFramePeriod;

		DrawIntroFrame();
		return true;
	}

	// limit FPS via sleep to not lock a singlethreaded CPU from loading the game;
	// done here rather than in Draw so the sleep does not hold CLoadLock
	if (mtLoading) {
		const unsigned diffTime = spring_tomsecs(spring_gettime() - lastDrawTime);

		constexpr unsigned wantedFPS = 50;
		constexpr unsigned minFrameTime = 1000 / wantedFPS;

		if (diffTime < minFrameTime)
			spring_sleep(spring_msecs(minFrameTime - diffTime));

		lastDrawTime = spring_gettime();
	}

	return true;
}


void CLoadScreen::DrawIntroFrame()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// SpringApp leaves drawing and swapping to the load screen in LoadingMT=2
	Draw();

	// only what the load screen made itself (whose buffers queue no fences); the game's
	// render buffers belong to the loading thread's context and wait for the game's first
	// frame, and its stream buffer fences are put by the loading thread (LoadInGameContext)
	RenderBuffer::SwapIntroRenderBuffers();

	SDL_GL_SwapWindow(globalRendering->GetWindow());
}


bool CLoadScreen::Draw()
{
	RECOIL_DETAILED_TRACY_ZONE;
	globalRendering->drawFrame = std::max(1U, globalRendering->drawFrame + 1);
	// let LuaMenu keep the lobby connection alive; not with LoadingMT=2, where the loading
	// thread can run LuaMenu's state (eventHandler) and its GL objects are in the game's context
	if (luaMenu != nullptr && !splitContext)
		luaMenu->Update();

	if (luaIntro != nullptr) {
		luaIntro->Update();
		luaIntro->DrawGenesis();
		ClearScreen();
		luaIntro->DrawLoadScreen();
	}

	if (!mtLoading)
		globalRendering->SwapBuffers(true, false);

	return true;
}


/******************************************************************************/
/******************************************************************************/

void CLoadScreen::SetLoadMessage(const std::string& text, bool replaceLast)
{
	RECOIL_DETAILED_TRACY_ZONE;
	// LoadingMT: the main thread pumps window events itself; pumping them here as
	// well would hand key, mouse and resize events to LuaIntro from the loading thread
	if (mtLoading)
		Watchdog::ClearTimer(WDT_LOAD);
	else
		spring::UnfreezeSpring(WDT_LOAD);

	std::lock_guard<spring::recursive_mutex> lck(mutex);

	loadMessages.emplace_back(text, replaceLast);

	LOG("[LoadScreen::%s] text=\"%s\"", __func__, text.c_str());
	LOG_CLEANUP();

	// be paranoid about FPU state for the loading thread since some
	// external library might reset it (main thread state is checked
	// in ::Update)
	good_fpu_control_registers(text.c_str());

	if (mtLoading)
		return;

	Update();
	Draw();
}
