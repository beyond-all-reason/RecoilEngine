/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include "GroundMoveSystem.h"

#include "Sim/Ecs/Registry.h"
#include "Sim/Features/Feature.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/MoveTypes/Components/MoveTypesComponents.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"

#include "System/EventHandler.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"
#include "System/Sync/SyncChecker.h"
#include "System/Sync/SyncedPrimitiveBase.h"

using namespace MoveTypes;

// Units per ThreadPool task: one unit per task costs an atomic per unit and false sharing between
// neighbouring components. The heavier steps get smaller chunks to balance their uneven cost.
static constexpr int MT_UNITS_PER_TASK = 32;
static constexpr int MT_UNITS_PER_TASK_HEAVY = 16;

void GroundMoveSystem::Init() {}

template<typename T, typename F>
void issue_events(F func)
{
    auto view = Sim::registry.view<T>();
    view.each([&](T& comp){
        std::for_each(comp.value.begin(), comp.value.end(), func);
        comp.value.clear();
    });
}

void GroundMoveSystem::Update() {
    // TODO: GroundMove could become a component (or series of components) and then the extra indirection wouldn't be
    // needed. Though that will be a bigger change.
	{
		SCOPED_TIMER("Sim::Unit::MoveType::1::UpdateTraversalPlan");
        auto view = Sim::registry.view<GroundMoveType>();
        for_mt_chunk(0, view.size(), [&view](const int i){
            auto entity = (*view.storage<GroundMoveType>())[i];
            auto unitId = view.get<GroundMoveType>(entity);

            CUnit* unit = unitHandler.GetUnit(unitId.value);
			CGroundMoveType* moveType = static_cast<CGroundMoveType*>(unit->moveType);
            assert(moveType != nullptr);

            #ifndef NDEBUG
			unit->SanityCheck();
            #endif

			moveType->UpdateTraversalPlan();
		}, MT_UNITS_PER_TASK_HEAVY, MT_UNITS_PER_TASK_HEAVY);
	}
	{
		SCOPED_TIMER("Sim::Unit::MoveType::2::ChangeHeading");

        // Headings are synced vars: each unit's writes are hashed on its own thread and added to the
        // sync checksum in a fixed order afterwards. Units that may call script or weapon code stay ST.
        auto view = Sim::registry.view<GroundMoveType, ChangeHeadingEvent, ChangeMainHeadingEvent>();
        auto& units = *view.storage<GroundMoveType>();

        CSyncChecker::SetDeferred(true);
        for_mt_chunk(0, units.size(), [&view, &units](const int i){
            auto entity = units[i];
            ChangeHeadingEvent& headingEvent = view.get<ChangeHeadingEvent>(entity);
            ChangeMainHeadingEvent& mainHeadingEvent = view.get<ChangeMainHeadingEvent>(entity);
            if (!headingEvent.changed && !mainHeadingEvent.changed)
                return;

            CUnit* unit = unitHandler.GetUnit(headingEvent.unitId);
            CGroundMoveType* moveType = static_cast<CGroundMoveType*>(unit->moveType);
            if (!moveType->CanChangeHeadingMT())
                return;

            if (headingEvent.changed) {
                CSyncChecker::ResetThreadChecksum();
                moveType->ChangeHeading(headingEvent.deltaHeading);
                headingEvent.syncChecksum = CSyncChecker::GetThreadChecksum();
                headingEvent.syncChecksumPending = true;
                headingEvent.changed = false;
            }
            if (mainHeadingEvent.changed) {
                CSyncChecker::ResetThreadChecksum();
                moveType->SetMainHeading();
                mainHeadingEvent.syncChecksum = CSyncChecker::GetThreadChecksum();
                mainHeadingEvent.syncChecksumPending = true;
                mainHeadingEvent.changed = false;
            }
        }, MT_UNITS_PER_TASK, MT_UNITS_PER_TASK);
        CSyncChecker::SetDeferred(false);

        Sim::registry.view<ChangeHeadingEvent>().each([](ChangeHeadingEvent& event){
            if (event.syncChecksumPending) {
                Sync::FoldDeferred(event.syncChecksum, "ChangeHeading");
                event.syncChecksumPending = false;
            } else if (event.changed) {
                CUnit* unit = unitHandler.GetUnit(event.unitId);
                CGroundMoveType* moveType = static_cast<CGroundMoveType*>(unit->moveType);
                moveType->ChangeHeading(event.deltaHeading);
                event.changed = false;
            }
        });
        Sim::registry.view<ChangeMainHeadingEvent>().each([](ChangeMainHeadingEvent& event){
            if (event.syncChecksumPending) {
                Sync::FoldDeferred(event.syncChecksum, "SetMainHeading");
                event.syncChecksumPending = false;
            } else if (event.changed) {
                CUnit* unit = unitHandler.GetUnit(event.unitId);
                CGroundMoveType* moveType = static_cast<CGroundMoveType*>(unit->moveType);
                moveType->SetMainHeading();
                event.changed = false;
            }
        });
    }
	{
        SCOPED_TIMER("Sim::Unit::MoveType::2::UpdateUnitPosition");
        auto view = Sim::registry.view<GroundMoveType>();
        for_mt_chunk(0, view.size(), [&view](const int i){
            auto entity = (*view.storage<GroundMoveType>())[i];
            auto unitId = view.get<GroundMoveType>(entity);

            CUnit* unit = unitHandler.GetUnit(unitId.value);
			CGroundMoveType* moveType = static_cast<CGroundMoveType*>(unit->moveType);
            assert(moveType != nullptr);

			moveType->UpdateUnitPosition();
		}, MT_UNITS_PER_TASK, MT_UNITS_PER_TASK);
	}
	{
        SCOPED_TIMER("Sim::Unit::MoveType::2::UpdatePreCollisions");

        // perf-pr-stack master compat: master's serial order
        if (CSyncChecker::MasterCompat()) {
            Sim::registry.view<GroundMoveType>().each([](GroundMoveType& unitId){
                CUnit* unit = unitHandler.GetUnit(unitId.value);
                CGroundMoveType* moveType = static_cast<CGroundMoveType*>(unit->moveType);

                moveType->ApplyResultantForces();
                moveType->UpdatePreCollisions();

                if (!unit->pos.IsInBounds() && (unit->speed.w > MAX_UNIT_SPEED))
                    unit->ForcedKillUnit(nullptr, false, true, -CSolidObject::DAMAGE_KILLED_OOB);
            });
        } else {
        auto view = Sim::registry.view<GroundMoveType, PreCollisionsMtState>();
        auto& units = *view.storage<GroundMoveType>();

        // same scheme as the heading changes above; units that need ST work this frame finish below
        CSyncChecker::SetDeferred(true);
        for_mt_chunk(0, units.size(), [&view, &units](const int i){
            auto entity = units[i];
            CUnit* unit = unitHandler.GetUnit(view.get<GroundMoveType>(entity).value);
            CGroundMoveType* moveType = static_cast<CGroundMoveType*>(unit->moveType);
            assert(moveType != nullptr);

            PreCollisionsMtState& state = view.get<PreCollisionsMtState>(entity);
            CSyncChecker::ResetThreadChecksum();
            moveType->ApplyResultantForces();
            if ((state.done = moveType->CanUpdatePreCollisionsMT()))
                moveType->UpdatePreCollisions();
            state.syncChecksum = CSyncChecker::GetThreadChecksum();
        }, MT_UNITS_PER_TASK, MT_UNITS_PER_TASK);
        CSyncChecker::SetDeferred(false);

        view.each([](GroundMoveType& unitId, PreCollisionsMtState& state){
            CUnit* unit = unitHandler.GetUnit(unitId.value);

            Sync::FoldDeferred(state.syncChecksum, "UpdatePreCollisions");
            if (!state.done)
                static_cast<CGroundMoveType*>(unit->moveType)->UpdatePreCollisions();

            // this unit is not coming back, kill it now without any death
            // sequence (s.t. deathScriptFinished becomes true immediately)
            if (!unit->pos.IsInBounds() && (unit->speed.w > MAX_UNIT_SPEED))
                unit->ForcedKillUnit(nullptr, false, true, -CSolidObject::DAMAGE_KILLED_OOB);
        });
        }
	}
    {
        SCOPED_TIMER("Sim::Unit::MoveType::3::CollisionDetection");
        auto view = Sim::registry.view<GroundMoveType>();
        //size_t count = view.storage<GroundMoveType>().size();
        for_mt_chunk(0, view.size(), [&view](const int i){
            auto entity = (*view.storage<GroundMoveType>())[i];
            assert( Sim::registry.valid(entity) );
            assert( Sim::registry.all_of<GroundMoveType>(entity) );
            assert( !Sim::registry.all_of<GeneralMoveType>(entity) );

            auto unitId = view.get<GroundMoveType>(entity);

            CUnit* unit = unitHandler.GetUnit(unitId.value);
            CGroundMoveType* moveType = static_cast<CGroundMoveType*>(unit->moveType);
            assert(moveType != nullptr);

            moveType->SetMtJobId(i);
            moveType->UpdateCollisionDetections();
        }, MT_UNITS_PER_TASK_HEAVY, MT_UNITS_PER_TASK_HEAVY);
    }
	{
        SCOPED_TIMER("Sim::Unit::MoveType::4::ProcessCollisionEvents");

        issue_events<UnitCrushEvents>([](const UnitCrushEvent& event) {
            event.collidee->Kill(event.collider, event.crushImpulse, true);
        });
        issue_events<FeatureCrushEvents>([](const FeatureCrushEvent& event) {
            event.collidee->Kill(event.collider, event.crushImpulse, true);
        });
        issue_events<UnitCollisionEvents>([&](const UnitCollisionEvent& event) {
            eventHandler.UnitUnitCollision(event.collider, event.collidee);
        });
        issue_events<FeatureCollisionEvents>([](const FeatureCollisionEvent& event) {
            eventHandler.UnitFeatureCollision(event.collider, event.collidee);
        });
        issue_events<FeatureMoveEvents>([](const FeatureMoveEvent& event) {
            quadField.RemoveFeature(event.collidee);
            event.collidee->Move(event.moveImpulse, true);
            quadField.AddFeature(event.collidee);
        });
	}
	{
        SCOPED_TIMER("Sim::Unit::MoveType::5::Update");
        auto view = Sim::registry.view<GroundMoveType, UnitMovedEvent>();
        auto& units = *view.storage<GroundMoveType>();

        // same scheme as the heading changes above; UnitMoved events are sent afterwards
        CSyncChecker::SetDeferred(true);
        for_mt_chunk(0, units.size(), [&view, &units](const int i){
            auto entity = units[i];
            CUnit* unit = unitHandler.GetUnit(view.get<GroundMoveType>(entity).value);
            CGroundMoveType* moveType = static_cast<CGroundMoveType*>(unit->moveType);
            assert(moveType != nullptr);

            UnitMovedEvent& movedEvent = view.get<UnitMovedEvent>(entity);
            CSyncChecker::ResetThreadChecksum();
            movedEvent.moved = moveType->Update();
            movedEvent.syncChecksum = CSyncChecker::GetThreadChecksum();
        }, MT_UNITS_PER_TASK, MT_UNITS_PER_TASK);
        CSyncChecker::SetDeferred(false);

        view.each([](GroundMoveType& unitId, UnitMovedEvent& movedEvent){
            CUnit* unit = unitHandler.GetUnit(unitId.value);

            Sync::FoldDeferred(movedEvent.syncChecksum, "GroundMoveType::Update");
            if (movedEvent.moved)
                eventHandler.UnitMoved(unit);

            #ifndef NDEBUG
            unit->SanityCheck();
            #endif
        });
    }
}

void GroundMoveSystem::Shutdown() {}
