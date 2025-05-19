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

void ExtractorHandler::ResetState() {
	maxExtractionRange = 0.0f;
}

void ExtractorHandler::UpdateMaxExtractionRange(float newExtractorRange) {
	maxExtractionRange = std::max(newExtractorRange, maxExtractionRange);
}

bool ExtractorHandler::IsExtractor(const CUnit* unit) const
{
	return unit->unitDef->extractsMetal > 0.0f;
}

ExtractorBuilding* ExtractorHandler::GetExtractor(const CUnit* unit) const
{
	if (!IsExtractor(unit))
		return nullptr;

	entt::entity unitEntity = entt::entity(unit->entityReference);
	return Sim::registry.try_get<ExtractorBuilding>(unitEntity);
}

void ExtractorHandler::UnitPreInit(const CUnit* unit, const UnitLoadParams& params) const
{
	if (IsExtractor(unit)) {
		Sim::registry.emplace<ExtractorBuilding>(unit->entityReference, unit->id, unit->unitDef->extractRange, unit->unitDef->extractsMetal);
	}
}

void ExtractorHandler::UnitActivated(const CUnit* unit, bool activated)
{
	auto* extractor = GetExtractor(unit);
	if (extractor == nullptr)
		return;
	if (activated)
		extractor->Activate();
	else
		extractor->Deactivate();
}

void ExtractorHandler::UnitReverseBuilt(const CUnit* unit) const
{
	auto* extractor = GetExtractor(unit);
	if (extractor != nullptr)
		extractor->ResetExtraction();
}

