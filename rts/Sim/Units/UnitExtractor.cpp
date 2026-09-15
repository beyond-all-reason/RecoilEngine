/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// Used for all metal-extractors.
// Handles the metal-make-process.

#include "UnitExtractor.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Map/ReadMap.h"
#include "Map/MetalMap.h"
#include "Sim/Misc/QuadField.h"
#include "System/ContainerUtil.h"

#include "System/Misc/TracyDefs.h"


CR_BIND(CUnitExtractor, )
CR_REG_METADATA(CUnitExtractor, (
	CR_MEMBER(owner),
	CR_MEMBER(extractionRange),
	CR_MEMBER(extractionDepth),
	CR_MEMBER(metalAreaOfControl),
	CR_MEMBER(neighbours)
))

CR_BIND(CUnitExtractor::MetalSquareOfControl, )

CR_REG_METADATA_SUB(CUnitExtractor,MetalSquareOfControl, (
	CR_MEMBER(x),
	CR_MEMBER(z),
	CR_MEMBER(extractionDepth)
))

float CUnitExtractor::maxExtractionRange = 0.0f;


CUnitExtractor::CUnitExtractor(CUnit* owner)
	: owner(owner)
	, extractionRange(owner->unitDef->extractRange)
	, extractionDepth(owner->unitDef->extractsMetal)
{
}

CUnitExtractor::~CUnitExtractor()
{
	ResetExtraction();
}


/* resets the metalMap and notifies the neighbours */
void CUnitExtractor::ResetExtraction()
{
	RECOIL_DETAILED_TRACY_ZONE;
	owner->metalExtract = 0;
	owner->script->ExtractionRateChanged(owner->metalExtract);

	// undo the extraction-area
	for (auto si = metalAreaOfControl.begin(); si != metalAreaOfControl.end(); ++si) {
		metalMap.RemoveExtraction(si->x, si->z, si->extractionDepth);
	}

	metalAreaOfControl.clear();

	// tell the neighbours (if any) to take it over
	for (CUnitExtractor* ngb: neighbours) {
		ngb->RemoveNeighbour(this);
		ngb->ReCalculateMetalExtraction();
	}
	neighbours.clear();
}



/* determine if two extraction areas overlap */
bool CUnitExtractor::IsNeighbour(const CUnitExtractor* other) const
{
	RECOIL_DETAILED_TRACY_ZONE;
	// circle vs. circle
	return (owner->pos.SqDistance2D(other->owner->pos) < Square(this->extractionRange + other->extractionRange));
}

/* sets the range of extraction for this extractor, also finds overlapping neighbours. */
void CUnitExtractor::SetExtractionRangeAndDepth(float range, float depth)
{
	RECOIL_DETAILED_TRACY_ZONE;
	extractionRange = std::max(range, 0.001f);
	extractionDepth = std::max(depth, 0.0f);
	maxExtractionRange = std::max(extractionRange, maxExtractionRange);

	// find any neighbouring extractors
	QuadFieldQuery qfQuery;
	quadField.GetUnits(qfQuery, owner->pos, extractionRange + maxExtractionRange);

	for (CUnit* u: *qfQuery.units) {
		if (u == owner)
			continue;
		if (u->extractor == nullptr)
			continue;

		CUnitExtractor* eb = u->extractor;

		if (!IsNeighbour(eb))
			continue;

		this->AddNeighbour(eb);
		eb->AddNeighbour(this);
	}

	if (!owner->activated) {
		assert(owner->metalExtract == 0); // when deactivated metalExtract should always be 0

		return;
	}

	// calculate this extractor's area of control and metalExtract amount
	owner->metalExtract = 0;

	const float3& pos = owner->pos;

	const int xBegin = std::max(                   0, (int) ((pos.x - extractionRange) / METAL_MAP_SQUARE_SIZE));
	const int xEnd   = std::min(mapDims.mapx / 2 - 1, (int) ((pos.x + extractionRange) / METAL_MAP_SQUARE_SIZE));
	const int zBegin = std::max(                   0, (int) ((pos.z - extractionRange) / METAL_MAP_SQUARE_SIZE));
	const int zEnd   = std::min(mapDims.mapy / 2 - 1, (int) ((pos.z + extractionRange) / METAL_MAP_SQUARE_SIZE));

	metalAreaOfControl.reserve((xEnd - xBegin + 1) * (zEnd - zBegin + 1));

	// go through the whole (x, z)-square
	for (int x = xBegin; x <= xEnd; x++) {
		for (int z = zBegin; z <= zEnd; z++) {
			// center of metalsquare at (x, z)
			const float3 msqrPos((x + 0.5f) * METAL_MAP_SQUARE_SIZE, pos.y,
													 (z + 0.5f) * METAL_MAP_SQUARE_SIZE);
			const float sqrCenterDistance = msqrPos.SqDistance2D(pos);

			if (sqrCenterDistance < Square(extractionRange)) {
				MetalSquareOfControl msqr;
				msqr.x = x;
				msqr.z = z;
				// extraction is done in a cylinder of height <depth>
				msqr.extractionDepth = metalMap.RequestExtraction(x, z, depth);
				metalAreaOfControl.push_back(msqr);
				owner->metalExtract += msqr.extractionDepth * metalMap.GetMetalAmount(msqr.x, msqr.z);
			}
		}
	}

	// set the COB animation speed
	owner->script->ExtractionRateChanged(owner->metalExtract);
}


/* adds a neighbour for this extractor */
void CUnitExtractor::AddNeighbour(CUnitExtractor* neighbour)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(neighbour != this);
	spring::VectorInsertUnique(neighbours, neighbour, true);
}

/* removes a neighbour for this extractor */
void CUnitExtractor::RemoveNeighbour(CUnitExtractor* neighbour)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(neighbour != this);
	spring::VectorErase(neighbours, neighbour);
}


/* recalculate metalExtract for this extractor (eg. when a neighbour dies) */
void CUnitExtractor::ReCalculateMetalExtraction()
{
	RECOIL_DETAILED_TRACY_ZONE;
	owner->metalExtract = 0;

	for (MetalSquareOfControl& msqr: metalAreaOfControl) {
		metalMap.RemoveExtraction(msqr.x, msqr.z, msqr.extractionDepth);

		if (owner->activated) {
			// extraction is done in a cylinder
			msqr.extractionDepth = metalMap.RequestExtraction(msqr.x, msqr.z, extractionDepth);
			owner->metalExtract += (msqr.extractionDepth * metalMap.GetMetalAmount(msqr.x, msqr.z));
		}
	}

	// set the new rotation-speed
	owner->script->ExtractionRateChanged(owner->metalExtract);
}


void CUnitExtractor::OnActivate()
{
	RECOIL_DETAILED_TRACY_ZONE;
	/* Finds the amount of metal to extract and sets the rotationspeed when the extractor is built. */
	SetExtractionRangeAndDepth(extractionRange, extractionDepth);
}


void CUnitExtractor::OnDeactivate()
{
	RECOIL_DETAILED_TRACY_ZONE;
	ResetExtraction();
}
