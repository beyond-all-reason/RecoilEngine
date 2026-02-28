/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include "SyncUpdatedPathsSystem.h"

#include "Sim/Path/IPathManager.h"
#include "Sim/Path/QTPFS/Components/Path.h"
#include "Sim/Path/QTPFS/Components/SyncUpdatedPaths.h"
#include "Sim/Path/QTPFS/PathManager.h"
#include "Sim/Path/QTPFS/Registry.h"
#include "Sim/Path/QTPFS/Utils/DestroyEntityUtils.h"
#include "Sim/Path/QTPFS/Utils/SyncUpdatedPathsSystemUtils.h"

#include "System/Ecs/EcsMain.h"
#include "System/Ecs/Utils/SystemGlobalUtils.h"
#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"

#include "System/Misc/TracyDefs.h"

using namespace SystemGlobals;
using namespace QTPFS;


void SyncUpdatedPathsSystem::Init()
{
    RECOIL_DETAILED_TRACY_ZONE;
    auto& comp = systemGlobals.CreateSystemComponent<SyncUpdatedPathsComponent>();
    systemUtils.OnUpdate().connect<&SyncUpdatedPathsSystem::Update>();
}

void SyncUpdatedPathsSystem::Update()
{
	RECOIL_DETAILED_TRACY_ZONE;
    SCOPED_TIMER("ECS::SyncUpdatedPathsSystem::Update");

    auto* pm = dynamic_cast<PathManager*>(pathManager);
    auto& comp = systemGlobals.GetSystemComponent<SyncUpdatedPathsComponent>();

    // Ensure any outstanding pathing tasks are completed before synchronisation.
	if (comp.backgroundTask){
		wait_for_mt_background(comp.backgroundTask);
		comp.backgroundTask.reset();
	}

    auto pathSearchView = registry.group<PathSearch, ProcessPath>();

	for (auto pathSearchEntity : pathSearchView) {
		assert(registry.valid(pathSearchEntity));
		assert(registry.all_of<PathSearch>(pathSearchEntity));

		PathSearch* search = &pathSearchView.get<PathSearch>(pathSearchEntity);
		// assert(search->rawPathCheck == false); // raw path checks should have been processed already
		FinishPathSearch(pm, search);

		// LOG("%s: delete search %x", __func__, entt::to_integral(pathSearchEntity));
		if (registry.valid(pathSearchEntity))
			DestroyPathSearchEntity(pathSearchEntity);
	}
}

void SyncUpdatedPathsSystem::Shutdown() {
    RECOIL_DETAILED_TRACY_ZONE;
    systemUtils.OnUpdate().disconnect<&SyncUpdatedPathsSystem::Update>();

	// drain the task group if it still exists, to ensure all tasks have completed before we shutdown.
	auto& comp = systemGlobals.GetSystemComponent<SyncUpdatedPathsComponent>();
	if (comp.backgroundTask){
		wait_for_mt_background(comp.backgroundTask);
		comp.backgroundTask.reset();
	}
}
