/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PathSpeedModInfoSystem.h"

#include <cassert>
#include <limits>

#include "Map/ReadMap.h"

#include "Sim/Misc/GlobalSynced.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Path/IPathManager.h"
#include "Sim/Path/QTPFS/Components/PathSpeedModInfo.h"
#include "Sim/Path/QTPFS/NodeLayer.h"
#include "Sim/Path/QTPFS/PathManager.h"
#include "Sim/Path/QTPFS/Registry.h"

#include "System/Ecs/EcsMain.h"
#include "System/Ecs/Utils/SystemGlobalUtils.h"
#include "System/Log/ILog.h"
#include "System/TimeProfiler.h"
#include "System/Threading/ThreadPool.h"
#include "System/SpringMath.h"

#include "System/Misc/TracyDefs.h"


using namespace SystemGlobals;
using namespace QTPFS;

// Tracking a crash in live, so need optimizations turned down here for awhile.
// #ifdef __GNUC__
// #pragma GCC push_options
// #pragma GCC optimize ("O0")
// #endif

// layers of the scan running in the background
static std::vector<std::pair<NodeLayerSpeedInfoSweep*, const NodeLayer*>> scanLayers;

static void InitScan() {
    auto layersView = registry.view<NodeLayerSpeedInfoSweep>();
    auto pm = dynamic_cast<QTPFS::PathManager*>(pathManager);

    for ( QTPFS::entity entity : layersView ) {
            auto& layer = layersView.get<NodeLayerSpeedInfoSweep>(entity);
            auto& nodeLayer = pm->GetNodeLayer(layer.layerNum);
            layer.updateCurMaxSpeed = (-std::numeric_limits<float>::infinity());
            layer.updateMaxNodes = nodeLayer.GetMaxNodesAlloced();
            layer.updateInProgress = true;
            layer.updateCurSumSpeed = 0.f;
            layer.updateNumLeafNodes = 0.f;
        };
}

// Get maximum speed mod from the nodes walked thus far.
static void ScanLayerChunk(NodeLayerSpeedInfoSweep& layer, const NodeLayer& nodeLayer, int dataChunk, int refreshTimeInFrames) {
        const int idxBeg = ((dataChunk + 0) * layer.updateMaxNodes) / refreshTimeInFrames;
	    const int idxEnd = ((dataChunk + 1) * layer.updateMaxNodes) / refreshTimeInFrames;

        // if (layer.layerNum == 2) {
        //     LOG("Searching %d: %d/%d", dataChunk, layer.updateMaxNodes, refreshTimeInFrames);
        // }

        // TODO: store speed mods in a separate component? Allows for SSE perhaps?
        for (int i = idxBeg; i < idxEnd; ++i) {
            auto* curNode = nodeLayer.GetPoolNode(i);
            if (curNode->IsLeaf()) {
                float speedMod = curNode->GetSpeedMod();
                layer.updateCurMaxSpeed = std::max(layer.updateCurMaxSpeed, speedMod);
                layer.updateCurSumSpeed += speedMod;
                layer.updateNumLeafNodes++;
            }

            // if (layer.layerNum == 2) {
            //     LOG("node: %d max(%f,%f)", i, layer.updateCurMaxSpeed, curNode->GetSpeedMod());
            // }
        }
}

static void FinishScan(int dataChunk) {
    auto& comp = systemGlobals.GetSystemComponent<PathSpeedModInfoSystemComponent>();
    auto layersView = registry.view<NodeLayerSpeedInfoSweep>();

    // Finished search
    if (dataChunk == comp.refreshTimeInFrames + (-1))
        for ( QTPFS::entity entity : layersView ) {
            auto& layer = layersView.get<NodeLayerSpeedInfoSweep>(entity);
            comp.relSpeedModinfos[layer.layerNum].max = layer.updateCurMaxSpeed;
            comp.relSpeedModinfos[layer.layerNum].mean = layer.updateCurSumSpeed / layer.updateNumLeafNodes;
            layer.updateInProgress = false;

            // if (layer.layerNum == 2) {
            //     LOG("Finished Search - result is %f", comp.relSpeedModinfos[layer.layerNum]);
            // }
            };
}

static void WaitForScan(PathSpeedModInfoSystemComponent& comp) {
    if (comp.scanTask) {
        wait_for_mt_background(comp.scanTask);
        comp.scanTask.reset();
    }
}

void ScanForPathSpeedModInfo(int frameModulus) {
    RECOIL_DETAILED_TRACY_ZONE;
    auto& comp = systemGlobals.GetSystemComponent<PathSpeedModInfoSystemComponent>();
    auto layersView = registry.view<NodeLayerSpeedInfoSweep>();
    auto pm = dynamic_cast<QTPFS::PathManager*>(pathManager);
    int dataChunk = frameModulus;

    assert(layersView.size() <= MoveDefHandler::MAX_MOVE_DEFS);

    // Initialization
    if (frameModulus <= 0) {
        InitScan();
        dataChunk = 0;
    }

    // One thread per layer
    for_mt(0, layersView.size(), [&layersView, &comp, dataChunk, pm](int idx){
        QTPFS::entity entity = (*layersView.storage<NodeLayerSpeedInfoSweep>())[idx];
        auto& layer = layersView.get<NodeLayerSpeedInfoSweep>(entity);

        ScanLayerChunk(layer, pm->GetNodeLayer(layer.layerNum), dataChunk, comp.refreshTimeInFrames);
    });

    FinishScan(dataChunk);
}

// #ifdef __GNUC__
// #pragma GCC pop_options
// #endif

void InitLayers() {
    RECOIL_DETAILED_TRACY_ZONE;
    std::vector<QTPFS::entity> layers((size_t)moveDefHandler.GetNumMoveDefs());
    QTPFS::registry.create<decltype(layers)::iterator>(layers.begin(), layers.end());

    int counter = 0;
    std::for_each(layers.begin(), layers.end(), [&counter](QTPFS::entity entity){
        auto& layer = QTPFS::registry.emplace<NodeLayerSpeedInfoSweep>(entity);
        layer.layerNum = counter++;
        layer.updateCurMaxSpeed = 0.f;
        // LOG("%s: added %x:%x entity for layer %d '%s'", __func__, entt::to_integral(entity), entt::to_version(entity)
        //     , layer.layerNum, moveDefHandler.GetMoveDefByPathType(layer.layerNum)->name.c_str());
    });
}

void ScanWholeMap()
{
    auto& comp = systemGlobals.GetSystemComponent<PathSpeedModInfoSystemComponent>();
    WaitForScan(comp);

    // Scan whole map for initial max speed for hCost
    comp.refreshTimeInFrames = 1;
    ScanForPathSpeedModInfo(-1);
    comp.refreshTimeInFrames = GAME_SPEED * 30;
}

void PathSpeedModInfoSystem::Init()
{
    RECOIL_DETAILED_TRACY_ZONE;

    if (systemGlobals.IsSystemActive<PathSpeedModInfoSystemComponent>()) {
        ScanWholeMap();
        return;
    }

    auto& comp = systemGlobals.CreateSystemComponent<PathSpeedModInfoSystemComponent>();
    auto pm = dynamic_cast<QTPFS::PathManager*>(IPathManager::GetInstance(QTPFS_TYPE));

    InitLayers();

    systemUtils.OnUpdate().connect<&PathSpeedModInfoSystem::Update>();

    ScanWholeMap();

    comp.startRefreshOnFrame = NEXT_FRAME_NEVER;
    comp.refeshDelayInFrames = GAME_SPEED;

    comp.state = PathSpeedModInfoSystemComponent::STATE_READY;
}

void PathSpeedModInfoSystem::Update()
{
    SCOPED_TIMER("ECS::PathSpeedModInfoSystem::Update");

    auto& comp = systemGlobals.GetSystemComponent<PathSpeedModInfoSystemComponent>();
    auto nextFrameNum = gs->frameNum + 1;

    // this frame's chunk is usually scanned in the background already, see StartNextScan
    const bool scanned = (comp.scanFrame == gs->frameNum);
    WaitForScan(comp);

    switch (comp.state) {
    case (PathSpeedModInfoSystemComponent::STATE_UPDATING):
        if (scanned)
            FinishScan(gs->frameNum % comp.refreshTimeInFrames);
        else
            ScanForPathSpeedModInfo(gs->frameNum % comp.refreshTimeInFrames);
        if (nextFrameNum % comp.refreshTimeInFrames == 0)
            comp.state = PathSpeedModInfoSystemComponent::STATE_READY;
        break;
    case (PathSpeedModInfoSystemComponent::STATE_READY):
        if ( (nextFrameNum >= comp.startRefreshOnFrame) && (nextFrameNum % comp.refreshTimeInFrames == 0) ) {
            comp.state = PathSpeedModInfoSystemComponent::STATE_UPDATING;
            comp.startRefreshOnFrame = NEXT_FRAME_NEVER;
        }
        break;
    default:
        assert(false);
    }
}

// The node layers only change during the PathManager's map updates, so once those are done, the chunk of the next frame
// can already be scanned in the background. The tasks must not touch the registry, the simulation changes it meanwhile.
void PathSpeedModInfoSystem::StartNextScan()
{
    auto& comp = systemGlobals.GetSystemComponent<PathSpeedModInfoSystemComponent>();
    if (comp.state != PathSpeedModInfoSystemComponent::STATE_UPDATING)
        return;

    const int dataChunk = (gs->frameNum + 1) % comp.refreshTimeInFrames;
    if (dataChunk == 0)
        InitScan();

    auto layersView = registry.view<NodeLayerSpeedInfoSweep>();
    auto pm = dynamic_cast<QTPFS::PathManager*>(pathManager);

    scanLayers.clear();
    for ( QTPFS::entity entity : layersView ) {
        auto& layer = layersView.get<NodeLayerSpeedInfoSweep>(entity);
        scanLayers.emplace_back(&layer, &pm->GetNodeLayer(layer.layerNum));
    }

    comp.scanFrame = gs->frameNum + 1;
    comp.scanTask = for_mt_background(0, int(scanLayers.size()), std::function<void(int)>{[dataChunk, refreshTimeInFrames = comp.refreshTimeInFrames](int i){
        ScanLayerChunk(*scanLayers[i].first, *scanLayers[i].second, dataChunk, refreshTimeInFrames);
    }});
}

void PathSpeedModInfoSystem::Shutdown() {
    RECOIL_DETAILED_TRACY_ZONE;
    systemUtils.OnUpdate().disconnect<&PathSpeedModInfoSystem::Update>();

    WaitForScan(systemGlobals.GetSystemComponent<PathSpeedModInfoSystemComponent>());

    auto view = registry.view<NodeLayerSpeedInfoSweep>();
    for ( QTPFS::entity entity : view ) { registry.destroy(entity); }
}
