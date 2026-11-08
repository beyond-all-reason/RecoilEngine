/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "RequestPathInternalSystem.h"

#include "Sim/MoveTypes/MoveDefHandler.h"

#include "Sim/Path/IPathManager.h"
#include "Sim/Path/QTPFS/Components/Path.h"
#include "Sim/Path/QTPFS/Components/RemoveDeadPaths.h"
#include "Sim/Path/QTPFS/PathManager.h"
#include "Sim/Path/QTPFS/Registry.h"


#include "System/Ecs/EcsMain.h"
#include "System/Ecs/Utils/SystemGlobalUtils.h"
#include "System/TimeProfiler.h"
#include "System/Log/ILog.h"

#include "System/Misc/TracyDefs.h"

using namespace SystemGlobals;
using namespace QTPFS;


void RequestPathInternalSystem::Init()
{
    RECOIL_DETAILED_TRACY_ZONE;
}

void RequestPathInternalSystem::Update()
{
    SCOPED_TIMER("ECS::RequestPathInternalSystem::Update");

    auto* pm = dynamic_cast<PathManager*>(pathManager);

    // walk the available path requests in the external registry and create internal path entities for them.

    auto view = externalRegistry.view<PendingPathRequestDetails>();
    for (auto externalPathEntity : view) {
        assert(externalRegistry.valid(externalPathEntity));
        assert(externalRegistry.all_of<PendingPathRequestDetails>(externalPathEntity));

        PendingPathRequestDetails* request = &view.get<PendingPathRequestDetails>(externalPathEntity);

        // LOG("%s: delete search %x", __func__, entt::to_integral(pathSearchEntity));
        if (externalRegistry.valid(externalPathEntity)) {
            // get movedef from the pathtype
            MoveDef* moveDef = moveDefHandler.GetMoveDefByPathType(request->pathType);

            const QTPFS::entity internalPathID = static_cast<QTPFS::entity>( pm->QueueSearch
                ( registry
                , request->object, moveDef
                , request->startPos, request->endPos
                , request->radius, request->synced
                , request->externalRequest, request->allowRawSearch ) );

            externalRegistry.emplace_or_replace<InternalPathEntityId>(externalPathEntity, internalPathID);
            registry.emplace_or_replace<ExternalPathEntityId>(internalPathID, externalPathEntity);
        }
    }

    // clear all path requests from the external registry, since they have been processed and converted into internal path entities.
    externalRegistry.clear<PendingPathRequestDetails>();
}

void RequestPathInternalSystem::Shutdown() {
    RECOIL_DETAILED_TRACY_ZONE;
}
