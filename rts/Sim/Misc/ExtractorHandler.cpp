/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "ExtractorHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "System/ContainerUtil.h"
#include "System/SpringMath.h"
#include "Sim/Ecs/Registry.h"
#include "Sim/Units/Components/Extractor.h"

#include "System/Misc/TracyDefs.h"

CR_BIND(ExtractorHandler, )

CR_REG_METADATA(ExtractorHandler, (
	CR_MEMBER(maxExtractionRange)
))
	
ExtractorHandler extractorHandler;

void ExtractorHandler::UnitActivated(const CUnit* unit, bool activated) {

	if (!unit->unitDef->extractsMetal)
		return;
	entt::entity unitEntity = entt::entity(unit->entityReference);
	auto* extractor = Sim::registry.try_get<ExtractorBuilding>(unitEntity);
	if (!extractor)
		return;
	if (activated) {
		extractor->Activate();
	}
	else{
		extractor->Deactivate();
	}
}

ExtractorBuilding* ExtractorHandler::GetExtractor(const CUnit* unit) {
	entt::entity unitEntity = entt::entity(unit->entityReference);
	return Sim::registry.try_get<ExtractorBuilding>(unitEntity);
}

void ExtractorHandler::UnitReverseBuilt(const CUnit* unit) {
	if (!unit->unitDef->extractsMetal)
		return;
	entt::entity unitEntity = entt::entity(unit->entityReference);
	auto* extractor = Sim::registry.try_get<ExtractorBuilding>(unitEntity);
	if (extractor != nullptr)
		extractor->ResetExtraction();
}

void ExtractorHandler::UnitPreInit(const CUnit* unit, const UnitLoadParams& params)
{
	if (unit->unitDef->extractsMetal) {
		Sim::registry.emplace<ExtractorBuilding>(unit->entityReference, unit->id, unit->unitDef->extractRange, unit->unitDef->extractsMetal); //TODO VALUEs
	}
}


bool ExtractorHandler::AddExtractor(const CUnit* u) {
	RECOIL_DETAILED_TRACY_ZONE;
	Sim::registry.emplace<ExtractorBuildingActive>(u->entityReference);
	return true;
}

bool ExtractorHandler::DelExtractor(const CUnit* u) {
	RECOIL_DETAILED_TRACY_ZONE;
	Sim::registry.remove<ExtractorBuildingActive>(u->entityReference);
	return true;
}


