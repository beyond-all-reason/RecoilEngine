/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RequeuePathsSystem.h"

#include "Sim/Path/IPathManager.h"
#include "Sim/Path/QTPFS/Components/Path.h"
#include "Sim/Path/QTPFS/PathManager.h"
#include "Sim/Path/QTPFS/Registry.h"

#include "System/Ecs/EcsMain.h"
#include "System/Ecs/Utils/SystemGlobalUtils.h"
#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"

using namespace SystemGlobals;
using namespace QTPFS;


void RequeuePathsSystem::Init()
{
    systemUtils.OnUpdate().connect<&RequeuePathsSystem::Update>();
}

void RequeuePathsSystem::Update()
{
    SCOPED_TIMER("ECS::RequeuePathsSystem::Update");

    auto* pm = dynamic_cast<PathManager*>(pathManager);
    auto view = registry.view<PathRequeueSearch>();
    for (auto pathEntity : view) {
        bool &requeueSearch = view.get<PathRequeueSearch>(pathEntity).value;
        if (requeueSearch) {
            requeueSearch = false;

            // The path is already scheduled to be requeued.
            bool dirtyPath = registry.any_of<PathIsDirty>(pathEntity);
            if (dirtyPath) { continue; }

            pm->RequeueSearch(&registry.get<IPath>(pathEntity), true, false);
            registry.emplace_or_replace<PathUpdatedCounterIncrease>(pathEntity);
            
        }
    }
}

void RequeuePathsSystem::Shutdown() {
    systemUtils.OnUpdate().disconnect<&RequeuePathsSystem::Update>();
}
