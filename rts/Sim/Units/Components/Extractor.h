/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_COMPONENTS_EXTRACTOR_H__
#define UNIT_COMPONENTS_EXTRACTOR_H__

#include <vector>
#include <cereal/types/vector.hpp>

class CUnit;
struct UnitLoadParams;

namespace UnitComponents {

struct MetalSquareOfControl {
	int x;
	int z;
	float extractionDepth;
	template<class Archive>
	void serialize(Archive &ar) { ar(x, z, extractionDepth); };
};


class Extractor {
public:
	Extractor() {};
	Extractor(CUnit* unit, float extractionRange, float extractionDepth);

	void PreInit(const UnitLoadParams& params);
	void PostLoad(CUnit* myUnit);

	void ResetExtraction();
	void FindNeighbours();
	void SetExtractionRangeAndDepth(float range, float depth);
	void AddNeighbour(Extractor* neighbour);
	void RemoveNeighbour(Extractor* neighbour);

	float GetExtractionRange() const { return extractionRange; }
	float GetExtractionDepth() const { return extractionDepth; }

	void Activate();
	void Deactivate();

private:
	void ReCalculateMetalExtraction();
	bool IsNeighbour(Extractor* neighbour);

private:
	CUnit *unit;

	float extractionRange, extractionDepth;

	std::vector<MetalSquareOfControl> metalAreaOfControl;
	std::vector<Extractor*> neighbours;

public:
	// Serialization
	template<class Archive>
	void serialize(Archive &ar) {
		ar(extractionRange, extractionDepth, metalAreaOfControl);
	};
};

} // namespace UnitComponents

#endif
