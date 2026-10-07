/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "GlobalSynced.h"

#include <algorithm>
#include <assert.h>
#include <common/TracyColor.hpp>
#include <cstring>

#include "ExternalAI/SkirmishAIHandler.h"
#include "Game/GameSetup.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/SafeUtil.h"
#include "System/Log/FramePrefixer.h"
#include "System/Log/ILog.h"
#include "System/Config/ConfigHandler.h"
#include "System/Sync/SyncChecker.h"

CONFIG(bool, PrStackMasterCompat).defaultValue(false).description("perf-pr-stack TEST ONLY: master's MoveType order and checksums. Changes the simulation, so every player and replay must use the same value.");

const char* const tracingSpeedFactor = "SpeedFactor";
const char* const tracingWantedSpeedFactor = "WantedSpeedFactor";

/**
 * @brief global synced
 *
 * Global instance of CGlobalSynced
 */

CGlobalSynced gsOBJ;
CGlobalSyncedRNG gsRNG;

CGlobalSynced* gs = &gsOBJ;


CR_BIND(CGlobalSynced, )

CR_REG_METADATA(CGlobalSynced, (
	CR_MEMBER(frameNum),
	CR_MEMBER(tempNum),
	CR_MEMBER(mtTempNum),
	CR_MEMBER(godMode),

	CR_MEMBER(speedFactor),
	CR_MEMBER(wantedSpeedFactor),

	CR_MEMBER(paused),
	CR_MEMBER(cheatEnabled),
	CR_MEMBER(noHelperAIs),
	CR_MEMBER(editDefsEnabled),
	CR_MEMBER(useLuaGaia)
))


void CGlobalSynced::Kill()
{
	log_framePrefixer_setFrameNumReference(nullptr);
}


void CGlobalSynced::ResetState() {
	frameNum = -1; // first real frame is 0
	tempNum  =  1;
	godMode  =  0;

	std::fill(std::begin(mtTempNum), std::end(mtTempNum), 1);

#ifdef SYNCCHECK
	// reset checksum
	CSyncChecker::NewFrame();
#endif
	TracyPlotConfig(tracingSpeedFactor, tracy::PlotFormatType::Number, true, false, tracy::Color::Aqua);
	TracyPlotConfig(tracingWantedSpeedFactor, tracy::PlotFormatType::Number, true, false, tracy::Color::Aqua);

	speedFactor       = 1.0f;
	TracyPlot(tracingSpeedFactor, speedFactor);
	wantedSpeedFactor = 1.0f;
	TracyPlot(tracingWantedSpeedFactor, wantedSpeedFactor);

	paused          = false;
	cheatEnabled    = false;
	noHelperAIs     = false;
	editDefsEnabled = false;
	useLuaGaia      = true;

	gsRNG.SetSeed(18655, true);
	log_framePrefixer_setFrameNumReference(&frameNum);
}

void CGlobalSynced::LoadFromSetup(const CGameSetup* setup)
{
	noHelperAIs = setup->noHelperAIs;
	useLuaGaia  = setup->useLuaGaia;

	CSyncChecker::SetMasterCompat(configHandler->GetBool("PrStackMasterCompat"));
	LOG("[perf-pr-stack] master compat mode %s", CSyncChecker::MasterCompat()? "ON": "off");

	teamHandler.ResetState();
	teamHandler.LoadFromSetup(setup);
	skirmishAIHandler.ResetState();
	skirmishAIHandler.LoadFromSetup(*setup);
}

