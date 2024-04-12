/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "Building.h"
#include "Game/GameHelper.h"
#include "Map/ReadMap.h"
#include "Sim/Units/BuildInfo.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitLoader.h"
#include "System/SpringMath.h"

#include <tracy/Tracy.hpp>


CR_BIND_DERIVED(CBuilding, CUnit, )
CR_REG_METADATA(CBuilding, (
	CR_IGNORED(blockMap), // reloaded in PostLoad
	CR_POSTLOAD(PostLoad)
))


void CBuilding::PostLoad()
{
	//ZoneScoped;
	blockMap = unitDef->GetYardMapPtr();
}


void CBuilding::PreInit(const UnitLoadParams& params)
{
	//ZoneScoped;
	unitDef = params.unitDef;
	blockMap = unitDef->GetYardMapPtr(); // null if empty
	blockHeightChanges = unitDef->levelGround;

	CUnit::PreInit(params);
}

void CBuilding::PostInit(const CUnit* builder)
{
	//ZoneScoped;
	if (unitDef->cantBeTransported)
		mass = CSolidObject::DEFAULT_MASS;

	CUnit::PostInit(builder);
}


void CBuilding::ForcedMove(const float3& newPos) {
	//ZoneScoped;
	// heading might have changed if building was dropped from transport
	// (always needs to be axis-aligned because yardmaps are not rotated)
	heading = GetHeadingFromFacing(buildFacing);

	UpdateDirVectors(false, false, 0.0f);
	SetVelocity(ZeroVector);

	// update quadfield, etc.
	CUnit::ForcedMove(CGameHelper::Pos2BuildPos(BuildInfo(unitDef, newPos, buildFacing), true));

	unitLoader->FlattenGround(this);
}

