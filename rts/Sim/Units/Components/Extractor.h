/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_COMPONENTS_EXTRACTOR_H__
#define UNIT_COMPONENTS_EXTRACTOR_H__

#include <vector>

#include "Sim/Units/Unit.h"
#include <cereal/types/vector.hpp>

class CUnit;
struct UnitLoadParams;

struct MetalSquareOfControl {
	int x;
	int z;
	float extractionDepth;
	template<class Archive>
	void serialize(Archive &ar) { ar(x, z, extractionDepth); };
};


class ExtractorBuilding {
public:
	ExtractorBuilding() {};
	ExtractorBuilding(CUnit* unit, float extractionRange, float extractionDepth);

	CUnit *unit;

	float extractionRange, extractionDepth;

	std::vector<MetalSquareOfControl> metalAreaOfControl;
	std::vector<ExtractorBuilding*> neighbours;

	void PreInit(const UnitLoadParams& params);
	void PostLoad(CUnit* myUnit);

	void ResetExtraction();
	void FindNeighbours();
	void SetExtractionRangeAndDepth(float range, float depth);
	void ReCalculateMetalExtraction();
	bool IsNeighbour(ExtractorBuilding* neighbour);
	void AddNeighbour(ExtractorBuilding* neighbour);
	void RemoveNeighbour(ExtractorBuilding* neighbour);

	float GetExtractionRange() const { return extractionRange; }
	float GetExtractionDepth() const { return extractionDepth; }

	void Activate();
	void Deactivate();

	// Serialization

	template<class Archive, class Snapshot>
	static void SerializeComponents(Archive &archive, Snapshot &snapshot) {
	    snapshot.template component<ExtractorBuilding>(archive);
	};

	template<class Archive>
	void serialize(Archive &ar) {
		ar(extractionRange, extractionDepth, metalAreaOfControl);
	};
};

#endif
