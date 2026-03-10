/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QTPFS_SYSTEMS_SYNC_UPDATED_PATHS_H__
#define QTPFS_SYSTEMS_SYNC_UPDATED_PATHS_H__

// #include <cstddef>
#include "System/Ecs/Components/BaseComponents.h"
#include "System/Threading/ThreadPool.h"

namespace QTPFS {


typedef TaskPool<ForBackgroundTaskGroup, std::function<void(int)> &>::FuncTaskGroupPtr BackgroundTaskPtr;

struct SyncUpdatedPathsComponent {
	static constexpr std::size_t page_size = 1;

	BackgroundTaskPtr backgroundTask;
};

}

#endif