/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_SYSTEMS_PATH_H__
#define QTPFS_SYSTEMS_PATH_H__

#include <vector>

#include "../Registry.h"

#include "System/float3.h"
#include "System/Ecs/Components/BaseComponents.h"

namespace QTPFS {

struct SharedPathChain {
	SharedPathChain() {}

	SharedPathChain(QTPFS::entity initPrev, QTPFS::entity initNext)
		: prev(initPrev), next(initNext) {}

    QTPFS::entity prev{entt::null};
    QTPFS::entity next{entt::null};
};

struct PartialSharedPathChain {
	PartialSharedPathChain() {}

	PartialSharedPathChain(QTPFS::entity initPrev, QTPFS::entity initNext)
		: prev(initPrev), next(initNext) {}

    QTPFS::entity prev{entt::null};
    QTPFS::entity next{entt::null};
};

VOID_COMPONENT(PathIsTemp);
VOID_COMPONENT(PathIsDirty);
VOID_COMPONENT(PathIsToBeUpdated);
VOID_COMPONENT(PathUpdatedCounterIncrease);
VOID_COMPONENT(ProcessPath);

ALIAS_COMPONENT(PathSearchRef, QTPFS::entity);
ALIAS_COMPONENT(PathRequeueSearch, bool);

}

#endif