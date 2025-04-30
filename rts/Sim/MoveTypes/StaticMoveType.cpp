/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "StaticMoveType.h"
#include "Components/MoveTypesComponents.h"
#include "Map/Ground.h"
#include "Sim/Ecs/Registry.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/Rectangle.h"
#include "Sim/Misc/QuadField.h"

#include "System/Misc/TracyDefs.h"

using namespace MoveTypes;

CR_BIND_DERIVED(CStaticMoveType, AMoveType, (nullptr))
CR_REG_METADATA(CStaticMoveType, (
	CR_PREALLOC(GetPreallocContainer)
))

CStaticMoveType::CStaticMoveType(CUnit* unit) : AMoveType(unit) {
	RECOIL_DETAILED_TRACY_ZONE;
	useWantedSpeed[false] = false;
	useWantedSpeed[ true] = false;
	needsUpdate = true;

	// creg
	if (unit == nullptr)
		return;

	Sim::registry.emplace_or_replace<GeneralMoveType>(unit->entityReference, unit->id);
}

void CStaticMoveType::UpdateGroundFit()
{
	FitToGround();
	needsUpdate = false;
}

bool CStaticMoveType::FitToGround()
{
	RECOIL_DETAILED_TRACY_ZONE;
	// buildings and pseudo-static units can be moved and ground can change.
	if (owner->GetTransporter() != nullptr)
		return false;

	// NOTE:
	//   static buildings don't have any MoveDef instance, hence we need
	//   to get the ground height instead of calling CMoveMath::yLevel()
	float change;
	if (owner->FloatOnWater() && owner->IsInWater()) {
		change = -waterline - owner->pos.y;
	} else {
		/* Using GetHeightReal gives smoother result, but desyncs with ground drawing and also needs
		 * needsUpdate above to be set to the result of FitToGround. */
		change = CGround::GetApproximateHeight(owner->pos.x, owner->pos.z) - owner->pos.y;
	}
	if (std::abs(change) > float3::cmp_eps()) {
		owner->Move(UpVector * change, true);
		return true;
	}
	return false;
}

void CStaticMoveType::TerrainChanged(int x1, int y1, int x2, int y2)
{
	QuadFieldQuery qfQuery;
	const float3 mins(x1 * SQUARE_SIZE, 0, y1 * SQUARE_SIZE);
	const float3 maxs(x2 * SQUARE_SIZE, 0, y2 * SQUARE_SIZE);

	quadField.GetUnitsExact(qfQuery, mins, maxs);
	const auto& units = (*qfQuery.units);
	for (auto unit: units) {
		auto staticMoveType = dynamic_cast<CStaticMoveType*>(unit->moveType);
		if (staticMoveType) {
			staticMoveType->needsUpdate = true;
		}
	}

}
