/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_COMPONENTS_EXTRACTOR_H__
#define UNIT_COMPONENTS_EXTRACTOR_H__

#include <vector>

#include "System/float3.h"
#include "System/Ecs/Components/BaseComponents.h"

struct UnitLoadParams;
class CUnit;

struct MetalSquareOfControl {
	int x;
	int z;
	float extractionDepth;
};


class ExtractorBuilding {
public:
	ExtractorBuilding() {};
	ExtractorBuilding(int unitID, float extractionRange, float extractionDepth);

	CUnit *unit;

	float extractionRange, extractionDepth;

	std::vector<MetalSquareOfControl> metalAreaOfControl;
	std::vector<ExtractorBuilding*> neighbours;

	void PreInit(const UnitLoadParams& params);

	void ResetExtraction();
	void SetExtractionRangeAndDepth(float range, float depth);
	void ReCalculateMetalExtraction();
	bool IsNeighbour(ExtractorBuilding* neighbour);
	void AddNeighbour(ExtractorBuilding* neighbour);
	void RemoveNeighbour(ExtractorBuilding* neighbour);

	float GetExtractionRange() const { return extractionRange; }
	float GetExtractionDepth() const { return extractionDepth; }

	void Activate();
	void Deactivate();


};

VOID_COMPONENT(ExtractorBuildingActive);

#endif
