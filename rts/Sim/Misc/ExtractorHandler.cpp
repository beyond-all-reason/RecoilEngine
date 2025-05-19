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

using UnitComponents::Extractor;

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


Extractor* ExtractorHandler::TryGetExtractor(const CUnit* unit) const
{
	if (!IsExtractor(unit))
		return nullptr;

	return GetExtractor(unit);
}

Extractor* ExtractorHandler::GetExtractor(const CUnit* unit) const
{
	entt::entity unitEntity = entt::entity(unit->entityReference);
	return Sim::registry.try_get<Extractor>(unitEntity);
}


void ExtractorHandler::UnitPreInit(CUnit* unit, const UnitLoadParams& params) const
{
	// Called when creating the unit during the game.
	if (IsExtractor(unit)) {
		Sim::registry.emplace<Extractor>(unit->entityReference, unit, unit->unitDef->extractRange, unit->unitDef->extractsMetal);
	}
}


void ExtractorHandler::UnitPostLoad(CUnit* unit) const
{
	// Called after loading a unit from savegame.
	auto* extractor = TryGetExtractor(unit);
	if (extractor != nullptr) {
		extractor->PostLoad(unit);
	}
}


void ExtractorHandler::PostFinalizeRefresh() const
{
	// Called after all units are loaded.
	auto& activeUnits = unitHandler.GetActiveUnits();
	auto view = Sim::registry.view<Extractor>();
	for(auto entity: view) {
		auto* extractor = Sim::registry.try_get<Extractor>(entity);
		extractor->FindNeighbours();
	}
}


void ExtractorHandler::UnitActivated(const CUnit* unit, bool activated) const
{
	auto* extractor = TryGetExtractor(unit);
	if (extractor == nullptr)
		return;
	if (activated)
		extractor->Activate();
	else
		extractor->Deactivate();
}


void ExtractorHandler::UnitReverseBuilt(const CUnit* unit) const
{
	auto* extractor = TryGetExtractor(unit);
	if (extractor != nullptr)
		extractor->ResetExtraction();
}

