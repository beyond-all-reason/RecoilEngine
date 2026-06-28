/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* heavily based on CobEngine.cpp */

#include "UnitScriptEngine.h"

#include "CobEngine.h"
#include "CobFileHandler.h"
#include "CobInstance.h"
#include "RasEngine.h"
#include "RasFileHandler.h"
#include "RasInstance.h"
#include "UnitScript.h"
#include "UnitScriptFactory.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "System/ContainerUtil.h"
#include "System/HashSpec.h"
#include "System/SafeUtil.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/FileSystem.h"

#include "System/Misc/TracyDefs.h"

CONFIG(bool, AnimationMT).deprecated(true);

static CCobEngine gCobEngine;
static CCobFileHandler gCobFileHandler;
static CRasEngine gRasEngine;
static CRasFileHandler gRasFileHandler;
static CUnitScriptEngine gUnitScriptEngine;

CCobEngine* cobEngine = nullptr;
CCobFileHandler* cobFileHandler = nullptr;
CRasEngine* rasEngine = nullptr;
CRasFileHandler* rasFileHandler = nullptr;
CUnitScriptEngine* unitScriptEngine = nullptr;


CR_BIND(CUnitScriptEngine, )

CR_REG_METADATA(CUnitScriptEngine, (
	CR_MEMBER(animating),

	// always null when saving
	CR_IGNORED(currentScript)
))


void CUnitScriptEngine::InitStatic() {
	RECOIL_DETAILED_TRACY_ZONE;
	cobEngine = &gCobEngine;
	cobFileHandler = &gCobFileHandler;
	rasEngine = &gRasEngine;
	rasFileHandler = &gRasFileHandler;
	unitScriptEngine = &gUnitScriptEngine;

	cobEngine->Init();
	cobFileHandler->Init();
	rasEngine->Init();
	rasFileHandler->Init();
	unitScriptEngine->Init();
}

void CUnitScriptEngine::KillStatic() {
	RECOIL_DETAILED_TRACY_ZONE;
	cobEngine->Kill();
	cobFileHandler->Kill();
	rasEngine->Kill();
	rasFileHandler->Kill();
	unitScriptEngine->Kill();

	cobEngine = nullptr;
	cobFileHandler = nullptr;
	rasEngine = nullptr;
	rasFileHandler = nullptr;
	unitScriptEngine = nullptr;
}



void CUnitScriptEngine::ReloadScripts(const UnitDef* udef)
{
	RECOIL_DETAILED_TRACY_ZONE;

	const std::string ext = FileSystem::GetExtensionLowerCase(udef->scriptName);

	if (ext == "ras" || ext == "rasc") {
		const CRasFile* oldScriptFile = rasFileHandler->GetScriptFile(udef->scriptName);

		if (oldScriptFile == nullptr) {
			LOG_L(L_WARNING, "[UnitScriptEngine::%s] unknown RAS script for unit \"%s\": %s", __func__, udef->name.c_str(), udef->scriptName.c_str());
			return;
		}

		CRasFile* newScriptFile = rasFileHandler->ReloadRasFile(udef->scriptName);

		if (newScriptFile == nullptr) {
			LOG_L(L_WARNING, "[UnitScriptEngine::%s] could not load RAS script for unit \"%s\" from: %s", __func__, udef->name.c_str(), udef->scriptName.c_str());
			return;
		}

		unsigned int count = 0;

		for (unsigned int i = 0, n = unitHandler.MaxUnits(); i < n; i++) {
			CUnit* unit = unitHandler.GetUnit(i);

			if (unit == nullptr)
				continue;

			CUnitScript*& unitScript = unit->script;
			CRasInstance* rasInstance = dynamic_cast<CRasInstance*>(unitScript);

			if (rasInstance == nullptr || rasInstance->GetFile() != oldScriptFile)
				continue;

			count++;

			spring::SafeDestruct(unitScript);

			unitScript = CUnitScriptFactory::CreateRASScript(unit, newScriptFile);
			unitScript->Create();
		}

		LOG("[UnitScriptEngine::%s] reloaded RAS scripts for %i units", __func__, count);
		return;
	}

	const CCobFile* oldScriptFile = cobFileHandler->GetScriptFile(udef->scriptName);

	if (oldScriptFile == nullptr) {
		LOG_L(L_WARNING, "[UnitScriptEngine::%s] unknown COB script for unit \"%s\": %s", __func__, udef->name.c_str(), udef->scriptName.c_str());
		return;
	}

	CCobFile* newScriptFile = cobFileHandler->ReloadCobFile(udef->scriptName);

	if (newScriptFile == nullptr) {
		LOG_L(L_WARNING, "[UnitScriptEngine::%s] could not load COB script for unit \"%s\" from: %s", __func__, udef->name.c_str(), udef->scriptName.c_str());
		return;
	}

	unsigned int count = 0;

	for (unsigned int i = 0, n = unitHandler.MaxUnits(); i < n; i++) {
		CUnit* unit = unitHandler.GetUnit(i);

		if (unit == nullptr)
			continue;

		CUnitScript*& unitScript = unit->script;
		CCobInstance* cobInstance = dynamic_cast<CCobInstance*>(unitScript);

		if (cobInstance == nullptr || cobInstance->GetFile() != oldScriptFile)
			continue;

		count++;

		spring::SafeDestruct(unitScript);

		unitScript = CUnitScriptFactory::CreateCOBScript(unit, newScriptFile);
		unitScript->Create();
	}

	LOG("[UnitScriptEngine::%s] reloaded COB scripts for %i units", __func__, count);
}


void CUnitScriptEngine::AddInstance(CUnitScript* instance)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (instance == currentScript)
		return;

	spring::VectorInsertUnique(animating, instance/*, true*/);
}

void CUnitScriptEngine::RemoveInstance(CUnitScript* instance)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (instance == currentScript)
		return;

	spring::VectorErase(animating, instance);
}

void CUnitScriptEngine::Tick(int deltaTime)
{
	SCOPED_TIMER("CUnitScriptEngine::Tick");

	cobEngine->Tick(deltaTime);
	rasEngine->Tick(deltaTime);

	// tick all (COB or LUS) script instances that have registered themselves as animating
	{
		ZoneScopedN("CUnitScriptEngine::Tick(MT)");

		// setting currentScript = animating[i]; is not required here, only in ST section below
		for_mt(0, animating.size(), [&](const int i) {
			animating[i]->TickAllAnims(deltaTime);
		});
	}
	{
		ZoneScopedN("CUnitScriptEngine::Tick(ST)");

		uint32_t cs = 0;
		for (size_t i = 0; i < animating.size(); /*NO-OP*/) {
			currentScript = animating[i];
			// deal with synced checksum here, before animating is possibly popped below
			cs = spring::hash_combine(currentScript->GetAnimArrayChecksum(), cs);

			if (!currentScript->TickAnimFinished()) {
				animating[i] = animating.back();
				animating.pop_back();
				continue;
			}
			i++;
		}

		currentScript = nullptr;
		Sync::Assert(cs, "animating");
	}

	cobEngine->RunDeferredCallins();
	rasEngine->RunDeferredCallins();
}
