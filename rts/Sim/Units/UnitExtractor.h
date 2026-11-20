/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _UNIT_EXTRACTOR_H
#define _UNIT_EXTRACTOR_H

#include <vector>

#include "System/creg/creg_cond.h"

class CUnit;


class CUnitExtractor
{
	CR_DECLARE(CUnitExtractor)
	CR_DECLARE_SUB(MetalSquareOfControl)

public:
	CUnitExtractor() = default; // creg only; owner is filled in on load
	CUnitExtractor(CUnit* owner);
	~CUnitExtractor();

	void ResetExtraction();
	void SetExtractionRangeAndDepth(float range, float depth);
	void ReCalculateMetalExtraction();

	void OnActivate();
	void OnDeactivate();

	float GetExtractionRange() const { return extractionRange; }
	float GetExtractionDepth() const { return extractionDepth; }

private:
	bool IsNeighbour(const CUnitExtractor* other) const;
	void AddNeighbour(CUnitExtractor* neighbour);
	void RemoveNeighbour(CUnitExtractor* neighbour);

	struct MetalSquareOfControl {
		CR_DECLARE_STRUCT(MetalSquareOfControl)
		int x;
		int z;
		float extractionDepth;
	};

	CUnit* owner = nullptr;

	float extractionRange = 0.0f;
	float extractionDepth = 0.0f;

	std::vector<MetalSquareOfControl> metalAreaOfControl;
	std::vector<CUnitExtractor*> neighbours;

	// TODO: How are class statics incorporated into creg?
	static float maxExtractionRange;
};

#endif // _UNIT_EXTRACTOR_H
