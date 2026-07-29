/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SYNC_UPDATED_PATHS_SYSTEM_H__
#define SYNC_UPDATED_PATHS_SYSTEM_H__

namespace QTPFS {

class SyncUpdatedPathsSystem {
public:
    static void Init();
    static void Update();
    static void Shutdown();
};

}

#endif