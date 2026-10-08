/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include "GeneralMoveSystem.h"

#include "Sim/Ecs/Registry.h"
#include "Sim/MoveTypes/Components/MoveTypesComponents.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"

#include "System/EventHandler.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"
#include "System/Sync/SyncChecker.h"
#include "System/Sync/SyncedPrimitiveBase.h"
#include "Sim/Units/UnitDef.h"

#include "System/Misc/TracyDefs.h"

using namespace MoveTypes;

// Units per ThreadPool task (see GroundMoveSystem). Aircraft updates take ~1 us each, so small
// chunks keep a few hundred of them spread over all threads.
static constexpr int MT_UNITS_PER_TASK = 8;

void GeneralMoveSystem::Init() {
    RECOIL_DETAILED_TRACY_ZONE;
    CMoveMath::InitRangeIsBlockedHashes();
    Sim::systemUtils.OnPostLoad().connect<&CMoveMath::InitRangeIsBlockedHashes>();
}

struct MtUpdateState {
    entt::entity entity = entt::null;
    unsigned syncChecksum = 0;
    bool done = false;
    bool moved = false;
};

// indexed like the GeneralMoveType storage
static std::vector<MtUpdateState> mtUpdateStates;

void GeneralMoveSystem::Update() {
    RECOIL_DETAILED_TRACY_ZONE;
    auto view = Sim::registry.view<GeneralMoveType>();
    auto& units = *view.storage<GeneralMoveType>();
	{
        SCOPED_TIMER("Sim::Unit::MoveType::6::GeneralUpdate");

        // Units whose update only touches themselves this frame (see CanUpdateMT) are updated on worker
        // threads, the same way as in GroundMoveSystem; the others are updated in the ordered pass below.
        mtUpdateStates.resize(units.size());

        CSyncChecker::SetDeferred(true);
        for_mt_chunk(0, units.size(), [&units](const int i){
            CUnit* unit = unitHandler.GetUnit(units.get(units[i]).value);
            MtUpdateState& state = mtUpdateStates[i];

            state.entity = units[i];
            // perf-pr-stack master compat: no MT updates, master's order
            if (!(state.done = !CSyncChecker::MasterCompat() && unit->moveType->CanUpdateMT()))
                return;

            CSyncChecker::ResetThreadChecksum();
            state.moved = unit->moveType->Update();
            state.syncChecksum = CSyncChecker::GetThreadChecksum();
        }, MT_UNITS_PER_TASK, MT_UNITS_PER_TASK);
        CSyncChecker::SetDeferred(false);

        view.each([&units](const entt::entity entity, GeneralMoveType& unitId){
            CUnit* unit = unitHandler.GetUnit(unitId.value);
            AMoveType* moveType = unit->moveType;
            const MtUpdateState& state = mtUpdateStates[units.index(entity)];

            #ifndef NDEBUG
            unit->SanityCheck();
            #endif

            bool moved;
            if (state.done && state.entity == entity) {
                Sync::FoldDeferred(state.syncChecksum, "GeneralMoveType::Update");
                moveType->CallDeferredScripts();
                moved = state.moved;
            } else {
                moved = moveType->Update();
            }

            if (moved)
                eventHandler.UnitMoved(unit);

            // this unit is not coming back, kill it now without any death
            // sequence (s.t. deathScriptFinished becomes true immediately)
            if (!unit->pos.IsInBounds() && (unit->speed.w > MAX_UNIT_SPEED))
                unit->ForcedKillUnit(nullptr, false, true, -CSolidObject::DAMAGE_KILLED_OOB);

            #ifndef NDEBUG
            unit->SanityCheck();
            #endif
        });
	}
}

void GeneralMoveSystem::Shutdown() {
    Sim::systemUtils.OnPostLoad().disconnect<&CMoveMath::InitRangeIsBlockedHashes>();
}
