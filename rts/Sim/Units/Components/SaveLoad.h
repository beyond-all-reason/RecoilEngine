/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_COMPONENTS_SAVELOAD_H__
#define UNIT_COMPONENTS_SAVELOAD_H__

#include "Extractor.h"

namespace UnitComponents {

template<class Archive, class Snapshot>
static void SerializeComponents(Archive &archive, Snapshot &snapshot) {
    snapshot.template component<Extractor>(archive);
};

} // namespace UnitComponents

#endif
