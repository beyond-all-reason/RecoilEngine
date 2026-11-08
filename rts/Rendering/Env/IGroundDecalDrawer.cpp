/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "IGroundDecalDrawer.h"
#include "Rendering/Env/Decals/GroundDecalHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Exceptions.h"
#include "System/SafeUtil.h"
#include "System/Log/ILog.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"

#include "System/Misc/TracyDefs.h"

IGroundDecalDrawer* groundDecals = nullptr;

CONFIG(bool, GroundDecals).defaultValue(true).headlessValue(false).description("Controls whether ground decals underneath buildings, unit tracks & footprints as well as ground scars from explosions will be rendered.");

CR_BIND_INTERFACE(IGroundDecalDrawer)
CR_REG_METADATA(IGroundDecalDrawer, (
	CR_MEMBER(decals)
))

CR_BIND_DERIVED(NullGroundDecalDrawer, IGroundDecalDrawer, )
CR_REG_METADATA(NullGroundDecalDrawer,  )

void IGroundDecalDrawer::Init()
{
	RECOIL_DETAILED_TRACY_ZONE;
	SetDrawDecals(configHandler->GetBool("GroundDecals"));
}


void IGroundDecalDrawer::FreeInstance()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (groundDecals)
		spring::SafeDelete(groundDecals);
}

void IGroundDecalDrawer::SetDrawDecals(bool v)
{
	RECOIL_DETAILED_TRACY_ZONE;

	if (groundDecals && v == GetDrawDecals())
		return;

	// switched in a running game: the new handler starts empty
	const bool runtimeSwitch = (groundDecals != nullptr);

	FreeInstance();

	hasDecals = v;
	assert(!groundDecals);
	if (IGroundDecalDrawer::GetDrawDecals()) {
		groundDecals = new CGroundDecalHandler();
		LOG_L(L_INFO, "Loaded DecalsDrawer: %s", "standard");
	}
	else {
		groundDecals = new NullGroundDecalDrawer();
		LOG_L(L_INFO, "Loaded DecalsDrawer: %s", "null");
	}

	if (runtimeSwitch) {
		// give what already stands its plate, as RenderUnitCreated / RenderFeatureCreated would have;
		// explosion scars, tracks and Lua decals of the old handler are gone
		for (const CUnit* unit: unitHandler.GetActiveUnits()) {
			if (unit->GetTransporter() == nullptr)
				groundDecals->AddSolidObject(unit);
		}
		for (const int featureID: featureHandler.GetActiveFeatureIDs()) {
			if (const CFeature* feature = featureHandler.GetFeature(featureID); feature != nullptr)
				groundDecals->AddSolidObject(feature);
		}
	}

	configHandler->Set("GroundDecals", hasDecals);
}

void NullGroundDecalDrawer::SetUnitLeaveTracks(CUnit* unit, bool leaveTracks)
{
	unit->leaveTracks = leaveTracks;
}
