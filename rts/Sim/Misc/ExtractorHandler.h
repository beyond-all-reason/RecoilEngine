/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXTRACTOR_HANDLER_H
#define EXTRACTOR_HANDLER_H

#include "System/creg/creg.h"

class CUnit;
struct UnitLoadParams;

namespace UnitComponents { class Extractor; }
using UnitComponents::Extractor;

class ExtractorHandler
{
	CR_DECLARE_STRUCT(ExtractorHandler)
public:
	ExtractorHandler() { ResetState(); }
	ExtractorHandler(const ExtractorHandler&) = delete;

	ExtractorHandler& operator = (const ExtractorHandler&) = delete;

	bool IsExtractor(const CUnit* unit) const;
	Extractor* GetExtractor(const CUnit* unit) const;
	Extractor* TryGetExtractor(const CUnit* unit) const;

	void UnitActivated(const CUnit* unit, bool activated) const;
	void UnitPreInit(CUnit* unit, const UnitLoadParams& params) const;
	void UnitPostLoad(CUnit* unit) const;
	void UnitReverseBuilt(const CUnit* unit) const;

	void PostFinalizeRefresh() const;

	void UpdateMaxExtractionRange(float newExtractorRange);
	void ResetState();

	float maxExtractionRange = 0.0f;
};

extern ExtractorHandler extractorHandler;

#endif

