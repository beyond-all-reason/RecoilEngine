#ifndef DESTROY_ENTITY_UTILS_H_
#define DESTROY_ENTITY_UTILS_H_

#include "Sim/Path/QTPFS/Registry.h"
#include "Sim/Path/QTPFS/Path.h"
#include "Sim/Path/QTPFS/PathSearch.h"

namespace QTPFS {

    // Destroy an entity. It should be a path and is checked when asserts are enabled.
    inline void DestroyPathEntity(QTPFS::entity pathEntity) {
        assert((registry.any_of<IPath, UnsyncedIPath, ExternallyManagedSyncedIPath>(pathEntity)));
        assert((!registry.any_of<PathSearch, UnsyncedPathSearch, ExternallyManagedPathSearch>(pathEntity)));
        registry.destroy(pathEntity);
    }

     // Destroy an entity. It should be a path search and is checked when asserts are enabled.
    inline void DestroyPathSearchEntity(QTPFS::entity pathSearchEntity) {
        assert((registry.any_of<PathSearch, UnsyncedPathSearch, ExternallyManagedPathSearch>(pathSearchEntity)));
        assert((!registry.any_of<IPath, UnsyncedIPath, ExternallyManagedSyncedIPath>(pathSearchEntity)));
        registry.destroy(pathSearchEntity);
    }

}

#endif