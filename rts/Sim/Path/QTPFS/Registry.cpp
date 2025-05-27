#include "Registry.h"

entt::basic_registry<QTPFS::entity> QTPFS::registry;
SystemGlobals::SystemGlobal<decltype(QTPFS::registry)> QTPFS::systemGlobals(QTPFS::registry);
SystemUtils::SystemUtils QTPFS::systemUtils;
ecs_dll::DoubleLinkList<decltype(QTPFS::registry)> QTPFS::linkedListHelper(QTPFS::registry);
