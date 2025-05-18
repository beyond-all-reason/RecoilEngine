/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ENV_METAL_HANDLER_H
#define ENV_METAL_HANDLER_H

#include "Sim/Misc/GlobalConstants.h"
#include "System/float3.h"
#include <vector>

class CUnit;
class ExtractorBuilding;
struct UnitLoadParams;

class ExtractorHandler
{
	CR_DECLARE_STRUCT(ExtractorHandler)

public:
	ExtractorHandler() { ResetState(); }
	ExtractorHandler(const ExtractorHandler&) = delete;

	ExtractorHandler& operator = (const ExtractorHandler&) = delete;

	void UnitActivated(const CUnit* unit, bool activated);
	void UnitPreInit(const CUnit* unit, const UnitLoadParams& params);
	void UnitReverseBuilt(const CUnit* unit);
	ExtractorBuilding* GetExtractor(const CUnit* unit);

	void ResetState() {}

	bool AddExtractor(const CUnit* u);
	bool DelExtractor(const CUnit* u);

	float maxExtractionRange = 0.0f;
};

extern ExtractorHandler extractorHandler;

#endif

