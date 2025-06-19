/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "SyncUpdatedPathsSystem.h"

#include "Sim/Path/IPathManager.h"
#include "Sim/Path/QTPFS/Components/Path.h"
#include "Sim/Path/QTPFS/Components/SyncUpdatedPaths.h"
#include "Sim/Path/QTPFS/PathManager.h"
#include "Sim/Path/QTPFS/Registry.h"

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
    SCOPED_TIMER("ECS::SyncUpdatedPathsSystem::Update");

    auto* pm = dynamic_cast<PathManager*>(pathManager);
    auto& comp = systemGlobals.GetSystemComponent<SyncUpdatedPathsComponent>();

    // Ensure any outstanding pathing tasks are completed before synchronisation.
	if (comp.backgroundTask)
		wait_for_mt_background(comp.backgroundTask);

	auto completePath = [pm](QTPFS::entity pathEntity, IPath* path){

        // Transfer search update path, to the simulation-visible path.
        (*path) = std::move(registry.get<SearchModeIPath>(pathEntity));

		// inform the movement system that the path has been changed.
		if (registry.all_of<PathUpdatedCounterIncrease>(pathEntity)) {
			path->SetNumPathUpdates(path->GetNumPathUpdates() + 1);
			path->SetNextPointIndex(0);
			registry.remove<PathUpdatedCounterIncrease>(pathEntity);
		}
		registry.remove<PathIsTemp>(pathEntity);
		registry.remove<PathIsDirty>(pathEntity);
		registry.remove<PathSearchRef>(pathEntity);

		// If the node data wasn't recorded, then the path isn't shareable.
		if (!path->IsBoundingBoxOverriden() || path->GetNodeList().size() == 0) {
			pm->RemovePathFromShared(pathEntity);
			pm->RemovePathFromPartialShared(pathEntity);
		}
	};

    auto pathView = registry.group<PathSearch, ProcessPath>();

	for (auto pathSearchEntity : pathView) {
		assert(registry.valid(pathSearchEntity));
		assert(registry.all_of<PathSearch>(pathSearchEntity));

		PathSearch* search = &pathView.get<PathSearch>(pathSearchEntity);
		QTPFS::entity pathEntity = (QTPFS::entity)search->GetID();
		if (registry.valid(pathEntity)) {
			// Only owned paths should be actioned in this function.
			IPath* path = registry.try_get<IPath>(pathEntity);
			if (path != nullptr) {
				if (search->PathWasFound()) {
					completePath(pathEntity, path);
					// LOG("%s: %x - path found", __func__, entt::to_integral(pathEntity));
				} else {
					if (search->rawPathCheck) {
						registry.remove<PathSearchRef>(pathEntity);
						registry.remove<PathIsDirty>(pathEntity);

						// adding a new search doesn't break this loop because new paths do not
						// have the tag ProcessPath and so don't impact this group view.
						pm->RequeueSearch(path, false, true, search->tryPathRepair);
						// LOG("%s: %x - raw path check failed", __func__, entt::to_integral(pathEntity));
					} else if (search->pathRequestWaiting) {
						// nothing to do - it will be rerun next frame
						// LOG("%s: %x - waiting for partial root path", __func__, entt::to_integral(pathEntity));
						// continue;
						registry.remove<PathSearchRef>(pathEntity);
						pm->RequeueSearch(path, false, search->allowPartialSearch, false);
					} else if (search->rejectPartialSearch) {
						registry.remove<PathSearchRef>(pathEntity);
						pm->RequeueSearch(path, false, false, false);
					}
					else {
						// LOG("%s: %x - search failed", __func__, entt::to_integral(pathEntity));
						// Don't invalid the path, now, give the unit the chance to escape from
						// being stuck inside something.
						// DeletePathEntity(pathEntity);
						path->SetBoundingBox();
						completePath(pathEntity, path);
					}
				}
			}
		}

		// LOG("%s: delete search %x", __func__, entt::to_integral(pathSearchEntity));
		if (registry.valid(pathSearchEntity))
			registry.destroy(pathSearchEntity);
	}
}

void SyncUpdatedPathsSystem::Shutdown() {
    RECOIL_DETAILED_TRACY_ZONE;
    systemUtils.OnUpdate().disconnect<&SyncUpdatedPathsSystem::Update>();
}
