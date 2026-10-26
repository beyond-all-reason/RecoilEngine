#ifndef DESTROY_ENTITY_UTILS_H_
#define DESTROY_ENTITY_UTILS_H_

#include "Sim/Path/QTPFS/Registry.h"
#include "Sim/Path/QTPFS/Path.h"
#include "Sim/Path/QTPFS/PathSearch.h"

namespace QTPFS {

    // Destroy an entity. It should be a path and is checked when asserts are enabled.
    inline void DestroyPathEntity(entt::basic_registry<QTPFS::entity> &reg, QTPFS::entity pathEntity) {
        assert((reg.any_of<IPath, UnsyncedIPath, ExternallyManagedSyncedIPath, InternalPathEntityId, ExternalPathRequest>(pathEntity)));
        assert((!reg.any_of<PathSearch, UnsyncedPathSearch, ExternallyManagedPathSearch>(pathEntity)));
        reg.destroy(pathEntity);
    }

     // Destroy an entity. It should be a path search and is checked when asserts are enabled.
    inline void DestroyPathSearchEntity(entt::basic_registry<QTPFS::entity> &reg, QTPFS::entity pathSearchEntity) {
        assert((reg.any_of<PathSearch, UnsyncedPathSearch, ExternallyManagedPathSearch>(pathSearchEntity)));
        assert((!reg.any_of<IPath, UnsyncedIPath, ExternallyManagedSyncedIPath, InternalPathEntityId, ExternalPathRequest>(pathSearchEntity)));
        reg.destroy(pathSearchEntity);
    }

}

#endif