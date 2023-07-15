/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

// #undef NDEBUG

#include <assert.h>

#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <functional>

#include "System/Threading/ThreadPool.h"
#include "System/Threading/SpringThreading.h"

#include "PathDefines.h"
#include "PathManager.h"

#include "Utils/PathMaxSpeedModSystemUtils.h"

#include "Game/GameSetup.h"
#include "Game/LoadScreen.h"
#include "Map/MapInfo.h"

#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/ModInfo.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/MoveTypes/MoveMath/MoveMath.h"
#include "Sim/Objects/SolidObject.h"
#include "System/Config/ConfigHandler.h"
#include "System/FileSystem/ArchiveScanner.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/Platform/Threading.h"
#include "System/Rectangle.h"
#include "System/TimeProfiler.h"
#include "System/StringUtil.h"

#include "Components/Path.h"
#include "Components/PathSearch.h"
#include "Systems/PathMaxSpeedModSystem.h"
#include "Registry.h"

#include <assert.h>
#include <tracy/Tracy.hpp>

#ifdef GetTempPath
#undef GetTempPath
#undef GetTempPathA
#endif

#define NUL_RECTANGLE SRectangle(0, 0,             0,            0)
#define MAP_RECTANGLE SRectangle(0, 0,  mapDims.mapx, mapDims.mapy)

CONFIG(int, PathingThreadCount).defaultValue(0).safemodeValue(1).minimumValue(0);

namespace QTPFS {
	struct PMLoadScreen {
	public:
		PMLoadScreen() { loadMessages.reserve(8); }
		~PMLoadScreen() { assert(loadMessages.empty()); }

		void Kill() { loading = false; }
		void Show(const std::function<void(QTPFS::PathManager*)>& lf, QTPFS::PathManager* pm) {
			Init(lf, pm);
			Loop();
			Join();
		}

		void AddMessage(std::string&& msg) {
			std::lock_guard<spring::mutex> loadMessageLock(loadMessageMutex);
			loadMessages.emplace_back(std::move(msg));
		}

	private:
		void Init(const std::function<void(QTPFS::PathManager*)>& lf, QTPFS::PathManager* pm) {
			// must be set here to handle reloading
			loading = true;
			loadThread = spring::thread(std::bind(lf, pm));
		}
		void Loop() {
			while (loading) {
				spring::this_thread::sleep_for(std::chrono::milliseconds(50));

				// need this to be always executed after waking up
				SetMessages();
			}

			// handle any leftovers
			SetMessages();
		}
		void Join() {
			loadThread.join();
		}

		void SetMessages() {
			std::lock_guard<spring::mutex> loadMessageLock(loadMessageMutex);

			for (std::string& msg: loadMessages) {
				#ifdef QTPFS_NO_LOADSCREEN
				LOG("%s", msg.c_str());
				#else
				loadscreen->SetLoadMessage(std::move(msg));
				#endif
			}

			loadMessages.clear();
		}

	private:
		std::vector<std::string> loadMessages;
		spring::mutex loadMessageMutex;
		spring::thread loadThread;

		std::atomic<bool> loading = {false};
	};

	static PMLoadScreen pmLoadScreen;

	static size_t GetNumThreads() {
		const size_t numThreads = std::max(0, configHandler->GetInt("PathingThreadCount"));
		const size_t numCores = Threading::GetLogicalCpuCores();
		return ((numThreads == 0)? numCores: numThreads);
	}

	unsigned int PathManager::LAYERS_PER_UPDATE;
	unsigned int PathManager::MAX_TEAM_SEARCHES;

	std::vector<NodeLayer> PathManager::nodeLayers;
}

QTPFS::PathManager::PathManager() {
	QTNode::InitStatic();
	NodeLayer::InitStatic();
	PathManager::InitStatic();

	assert(registry.alive() == 0);

	// reserve entity 0 so it can't be used picked up by a path by accident.
	systemEntity = registry.create();

	assert(entt::to_entity(systemEntity) == 0);
}

QTPFS::PathManager::~PathManager() {
	isFinalized = false;

	PathMaxSpeedModSystem::Shutdown();

	// print out anything still left in the registry - there should be nothing
	registry.each([this](auto entity) {
		bool isPath = registry.all_of<IPath>(entity);
		bool isSearch = registry.all_of<PathSearch>(entity);
		if (isPath) {
			const IPath& path = registry.get<IPath>(entity);
			LOG("path [%x] type=%d, owner=%p", entt::to_integral(entity)
					, path.GetPathType()
					, path.GetOwner()
					);
		}
		if (isSearch) {
			const PathSearch& search = registry.get<PathSearch>(entity);
			LOG("search [%x] type=%d, id=%x", entt::to_integral(entity)
					, search.GetPathType()
					, search.GetID()
					);
			registry.destroy(entity);
		}
	});

	nodeLayerUpdatePriorityOrder.clear();
	for (unsigned int layerNum = 0; layerNum < nodeLayers.size(); layerNum++) {
		auto& nodeLayer = nodeLayers[layerNum];
		for (int i = 0; i < nodeLayer.GetRootNodeCount(); ++i){
			auto curRootNode = nodeLayer.GetPoolNode(i);
			curRootNode->Merge(nodeLayers[layerNum]);
		}

		nodeLayers[layerNum].Clear();
	}
	std::for_each(pathTraces.begin(), pathTraces.end(), [](std::pair<unsigned int, QTPFS::PathSearchTrace::Execution*>& t){delete t.second;} );

	auto clearTrackers = [](auto& track){
		track.damageMap.clear();
		track.damageQueue.clear();
	};

	pathTraces.clear();
	std::for_each(nodeLayersMapDamageTrack.mapChangeTrackers.begin(), nodeLayersMapDamageTrack.mapChangeTrackers.end(), clearTrackers);
	nodeLayersMapDamageTrack.mapChangeTrackers.clear();
	// mapChangeTrack.damageMap.clear();
	// mapChangeTrack.damageQueue.clear();
	sharedPaths.clear();
	sharedPaths.clear();

	// numCurrExecutedSearches.clear();
	// numPrevExecutedSearches.clear();

	searchThreadData.clear();
	updateThreadData.clear();
	pathSearches.clear();

	systemGlobals.ClearComponents();

	// make this is destroyed last to ensure entity 0 will be first picked up next time.
	registry.destroy(systemEntity);

	LOG("%s: %d entities still active!", __func__, (int)registry.alive());

	assert(registry.alive() == 0);

	registry.clear();
}

std::int64_t QTPFS::PathManager::Finalize() {
	const spring_time t0 = spring_gettime();

	{
		pmLoadScreen.Show(&PathManager::Load, this);
	}

	const spring_time t1 = spring_gettime();
	const spring_time dt = t1 - t0;

	return (dt.toMilliSecsi());
}

void QTPFS::PathManager::InitStatic() {
	LAYERS_PER_UPDATE = std::max(1u, mapInfo->pfs.qtpfs_constants.layersPerUpdate);
	MAX_TEAM_SEARCHES = std::max(1u, mapInfo->pfs.qtpfs_constants.maxTeamSearches);
}

void QTPFS::PathManager::Load() {
	// NOTE: offset *must* start at a non-zero value
	searchStateOffset = NODE_STATE_OFFSET;
	numPathRequests   = 0;
	int maxAllocedNodes   = 0;

	deadPathsToUpdatePerFrame = 1;
	recalcDeadPathUpdateRateOnFrame = 0;

	const int numMoveDefs = moveDefHandler.GetNumMoveDefs();

	pathCache.Init(numMoveDefs);
	nodeLayers.resize(numMoveDefs);
	// TODO: magic number - reserve one memory page.
	pathSearches.reserve( 4096 / sizeof(decltype(pathSearches)::value_type) );

	InitRootSize(MAP_RECTANGLE);

	nodeLayerUpdatePriorityOrder.resize(numMoveDefs);

	nodeLayersMapDamageTrack.width = mapDims.mapx / DAMAGE_MAP_BLOCK_SIZE; // + (mapDims.mapx % DAMAGE_MAP_BLOCK_SIZE > 0); -- maps can't be that finely sized!
	nodeLayersMapDamageTrack.height = mapDims.mapy / DAMAGE_MAP_BLOCK_SIZE; // + (mapDims.mapy % DAMAGE_MAP_BLOCK_SIZE > 0);
	nodeLayersMapDamageTrack.cellSize = DAMAGE_MAP_BLOCK_SIZE;

	nodeLayersMapDamageTrack.mapChangeTrackers.clear();
	nodeLayersMapDamageTrack.mapChangeTrackers.reserve(numMoveDefs);
	for (int i = 0; i < numMoveDefs; ++i) {
		{
			MapChangeTrack newChangeTrack;
			newChangeTrack.damageMap.resize(nodeLayersMapDamageTrack.width*nodeLayersMapDamageTrack.height);
			nodeLayersMapDamageTrack.mapChangeTrackers.emplace_back(newChangeTrack);
		}
		nodeLayerUpdatePriorityOrder[i] = i;
	}

	std::stable_sort(nodeLayerUpdatePriorityOrder.begin(), nodeLayerUpdatePriorityOrder.end(), [](int a, int b){
		return (moveDefHandler.GetMoveDefByPathType(a)->xsize > moveDefHandler.GetMoveDefByPathType(b)->xsize);
	});

	// for (int i=0; i < nodeLayerUpdatePriorityOrder.size(); ++i) {
	// 	LOG("%s: %d [priority %d] xsize = %d", __func__, nodeLayerUpdatePriorityOrder[i], i
	// 			, moveDefHandler.GetMoveDefByPathType(nodeLayerUpdatePriorityOrder[i])->xsize);
	// }

	isFinalized = true;
	{
		int threads = ThreadPool::GetNumThreads();
		updateThreadData.reserve(threads);
		while (threads-- > 0) {
			updateThreadData.emplace_back(UpdateThreadData());
		}
	}
	// add one extra element for object-less requests
	// numCurrExecutedSearches.resize(teamHandler.ActiveTeams() + 1, 0);
	// numPrevExecutedSearches.resize(teamHandler.ActiveTeams() + 1, 0);

	{
		const sha512::raw_digest& mapCheckSum = archiveScanner->GetArchiveCompleteChecksumBytes(gameSetup->mapName);
		const sha512::raw_digest& modCheckSum = archiveScanner->GetArchiveCompleteChecksumBytes(gameSetup->modName);

		sha512::hex_digest mapCheckSumHex;
		sha512::hex_digest modCheckSumHex;
		sha512::dump_digest(mapCheckSum, mapCheckSumHex);
		sha512::dump_digest(modCheckSum, modCheckSumHex);

		InitNodeLayersThreaded(MAP_RECTANGLE);
		PathMaxSpeedModSystem::Init();

		// NOTE:
		//   should be sufficient in theory, because if either
		//   the map or the mod changes then the checksum does
		//   (should!) as well and we get a cache-miss
		//   this value is also combined with the tree-sums to
		//   make it depend on the tesselation code specifics
		// FIXME:
		//   assumption is invalid now (Lua inits before we do)
		pfsCheckSum =
			((mapCheckSum[0] << 24) | (mapCheckSum[1] << 16) | (mapCheckSum[2] << 8) | (mapCheckSum[3] << 0)) ^
			((modCheckSum[0] << 24) | (modCheckSum[1] << 16) | (modCheckSum[2] << 8) | (modCheckSum[3] << 0));

		for (unsigned int layerNum = 0; layerNum < nodeLayers.size(); layerNum++) {
			auto& nodeLayer = nodeLayers[layerNum];
			for (int i = 0; i < nodeLayer.GetRootNodeCount(); ++i){
				auto curRootNode = nodeLayer.GetPoolNode(i);
				pfsCheckSum ^= curRootNode->GetCheckSum(nodeLayers[layerNum]);
			}
			maxAllocedNodes = std::max(nodeLayers[layerNum].GetMaxNodesAlloced(), maxAllocedNodes);
		}

		{ SyncedUint tmp(pfsCheckSum); }

		int threads = ThreadPool::GetNumThreads();
		searchThreadData.reserve(threads);
		while (threads-- > 0) {
			searchThreadData.emplace_back(SearchThreadData(maxAllocedNodes));
		}
	}

	{
		const int memFootPrintMb = GetMemFootPrint();
		const std::string sumStr = "pfs-checksum: " + IntToString(pfsCheckSum, "%08x") + ", ";
		const std::string memStr = "mem-footprint: " + IntToString(memFootPrintMb) + "MB";

		LOG("[QTPFS] pfs-checksum: %08x", pfsCheckSum);
		LOG("[QTPFS] pfs-checksum: %dMB", memFootPrintMb);

		char loadMsg[512] = {'\0'};
		const char* fmtString = "[PathManager::%s] used %u threads for %u node-layers";
		sprintf(loadMsg, fmtString, __func__, ThreadPool::GetNumThreads(), nodeLayers.size());
		pmLoadScreen.AddMessage(loadMsg);
		pmLoadScreen.Kill();
	}
}

std::uint64_t QTPFS::PathManager::GetMemFootPrint() const {
	std::uint64_t memFootPrint = sizeof(PathManager);

	for (auto threadData : searchThreadData) {
		memFootPrint += threadData.GetMemFootPrint();
	}

	// std::for_each(updateThreadData.begin(), updateThreadData.end(),[](const auto& d){ d.GetMemFootPrint(); });

	for (unsigned int i = 0; i < nodeLayers.size(); i++) {
		memFootPrint += nodeLayers[i].GetMemFootPrint();

		auto& nodeLayer = nodeLayers[i];
		for (int j = 0; j < nodeLayer.GetRootNodeCount(); ++j){
			auto curRootNode = nodeLayer.GetPoolNode(j);
			memFootPrint += curRootNode->GetMemFootPrint(nodeLayer);
		}
	}

	// convert to megabytes
	return (memFootPrint / (1024 * 1024));
}



void QTPFS::PathManager::InitNodeLayersThreaded(const SRectangle& rect) {
	streflop::streflop_init<streflop::Simple>();

	char loadMsg[512] = {'\0'};
	const char* fmtString = "[PathManager::%s] using %u threads for %u node-layers";

	{
		sprintf(loadMsg, fmtString, __func__, ThreadPool::GetNumThreads(), nodeLayers.size());
		pmLoadScreen.AddMessage(loadMsg);

		#ifndef NDEBUG
		const char* preFmtStr = "  initializing node-layer %u";
		const char* pstFmtStr = "  initialized node-layer %u (%u MB, %u leafs, ratio %f)";
		#endif

		for_mt(0, nodeLayers.size(), [=,&loadMsg, &rect](const int layerNum){
			int currentThread = ThreadPool::GetThreadNum();
			#ifndef NDEBUG
			sprintf(loadMsg, preFmtStr, layerNum);
			pmLoadScreen.AddMessage(loadMsg);
			#endif

			NodeLayer& layer = nodeLayers[layerNum];

			InitNodeLayer(layerNum, rect);

			INode* rootNode = layer.GetPoolNode(0);

			std::vector<SRectangle> rootRects;
			rootRects.reserve(layer.GetRootNodeCount());

			int rootXMax = rootNode->xmax();
			int rootZMax = rootNode->zmax();
			for (int hmz = rect.z1; hmz < rect.z2; hmz += rootZMax) {
				assert(hmz + rootZMax <= rect.z2);
				for (int hmx = rect.x1; hmx < rect.x2; hmx += rootXMax) {
					assert(hmx + rootXMax <= rect.x2);
					rootRects.emplace_back(hmx, hmz, hmx + rootXMax, hmz + rootZMax);
				}
			}
			
			std::for_each(rootRects.begin(), rootRects.end(), [this, layerNum, currentThread](auto &rect){
				UpdateNodeLayer(layerNum, rect, currentThread);
			});
		});

		// Full map-wide allocations have been made, we shouldn't need that much memory in future.
		for (int i = 0; i <ThreadPool::GetNumThreads(); ++i) {
			updateThreadData[i].Reset();
		}
	}

	streflop::streflop_init<streflop::Simple>();
}

void QTPFS::PathManager::InitRootSize(const SRectangle& r) {
	// setup the root node system
	// TODO: make sure resources are released on game end.
	int width = r.x2 - r.x1;
	int height = r.z2 - r.z1;
	LOG("%s: map root size is (%d, %d)", __func__, width, height);

	// Optimal function of QTPFS relies on power of 2 squares. Find the largest 2^x squares that
	// fit the map. 64 is the smallest as understood by map makers. So use 32 here to detect a map
	// that falls below that threshold.
	rootSize = 64;
	int limit = std::min(width, height);
	for (int factor = rootSize<<1; factor <= limit; factor <<= 1) {
		if (width % factor == 0 && height % factor == 0)
			rootSize = factor;
	}
	// Don't allow the root size to get too big to limit memory usage. (sizes given with 60 movetypes and 6 threads)
	// Nine Metal Islands could have gone to 2048x2048 root node (2880MB)
	// Nine Metal Islands has 512x512 nodes in each corner (180MB)
	// 256x256 (45MB)
	// Quick Silver has 128x128 nodes in corners (11.25 MB)
	int maxRootSize = 256;
	rootSize = rootSize > maxRootSize ? maxRootSize : rootSize;
	LOG("%s: root node size is set to: %d", __func__, rootSize);

	assert(rootSize != 64);
	if (rootSize == 64)
		LOG("%s: Warning! Map width and height are supposed to be multiples of 1024 elmos.", __func__);

	// TODO: reduce max levels based on number of root nodes - find a suitable limit?
	// TODO: prevent too big a size being picked due to 15 levels? 2^(steps -1) (steps=(bits-2)/2)
	constexpr float maxNodeLevels = ((sizeof(uint32_t)*4)-2);
	uint32_t maxNodeSize = math::pow(2.f, maxNodeLevels);
	rootSize = rootSize > maxNodeSize ? maxNodeSize : rootSize;
}

void QTPFS::PathManager::InitNodeLayer(unsigned int layerNum, const SRectangle& r) {
	NodeLayer& nl = nodeLayers[layerNum];

	nl.Init(layerNum);

	// TODO: partial zones just in case %64 != 0? need to check tessalation off map is okay.
	int numRootCount = 0;
	int zRootNodes = 0;
	for (int z = r.z1; z < r.z2; z += rootSize) {
		for (int x = r.x1; x < r.x2; x += rootSize) {
			int idx = nl.AllocPoolNode(nullptr, -1, x, z, x + rootSize, z + rootSize);
			// nl.RegisterNode(nl.GetPoolNode(idx));

			LOG("%s: %d root node [%d,%d:%d,%d] allocated.", __func__
					, idx, x, z, x + rootSize, z + rootSize);

			assert(idx == numRootCount);
			numRootCount++;
		}
		zRootNodes++;
	}
	
	uint32_t rootShift = 30;
	for (int factor = 4; factor < numRootCount; factor <<= 2) {
		rootShift -= 2;
	}
	QTNode::MAX_DEPTH = rootShift>>1; // std::min(rootShift>>1, QTNode::MAX_DEPTH);
	uint32_t rootMask = (~0) << rootShift;
	nl.SetRootMask(rootMask);

	LOG("rootShift = %d, maxDepth = %d", rootShift, rootMask);
	LOG("%s: %d root nodes allocated (%d x %d) mask: 0x%08x.", __func__
			, numRootCount, (numRootCount/zRootNodes), zRootNodes, rootMask);

	for (int i=0; i<numRootCount; ++i) {
		nl.GetPoolNode(i)->SetNodeNumber(i << rootShift);
		LOG("check %x (%x) == %x (%x)", i, i << rootShift
			, (nl.GetPoolNode(i)->GetNodeNumber() & rootMask) >> rootShift
			, nl.GetPoolNode(i)->GetNodeNumber()
			);
		assert(i == (nl.GetPoolNode(i)->GetNodeNumber() & rootMask) >> rootShift);
	}
	nl.SetRootNodeCountAndDimensions(numRootCount, (numRootCount/zRootNodes), zRootNodes, rootSize);
}



// __FORCE_ALIGN_STACK__

// called in the non-staggered (#ifndef QTPFS_STAGGERED_LAYER_UPDATES)
// layer update scheme and during initialization; see ::TerrainChange
void QTPFS::PathManager::UpdateNodeLayer(unsigned int layerNum, const SRectangle& rect, int currentThread) {
	const MoveDef* md = moveDefHandler.GetMoveDefByPathType(layerNum);

	// LOG("%s: Starting update for %d", __func__, layerNum);

	if (!IsFinalized())
		return;

	// NOTE:
	//     this is needed for IsBlocked* --> SquareIsBlocked --> IsNonBlocking
	//     but no point doing it in ExecuteSearch because the IsBlocked* calls
	//     are only made from NodeLayer::Update and also no point doing it here
	//     since we are independent of a specific path --> requires redesign
	//
	// md->tempOwner = const_cast<CSolidObject*>(path->GetOwner());

	// adjust the borders so we are not left with "rims" of
	// impassable squares when eg. a structure is reclaimed
	// SRectangle mr;

	// mr.x1 = std::max((r.x1 - md->xsizeh) - int(QTNode::MinSizeX() >> 1),            0);
	// mr.z1 = std::max((r.z1 - md->zsizeh) - int(QTNode::MinSizeZ() >> 1),            0);
	// mr.x2 = std::min((r.x2 + md->xsizeh) + int(QTNode::MinSizeX() >> 1), mapDims.mapx);
	// mr.z2 = std::min((r.z2 + md->zsizeh) + int(QTNode::MinSizeZ() >> 1), mapDims.mapy);
	// ur.x1 = mr.x1;
	// ur.z1 = mr.z1;
	// ur.x2 = mr.x2;
	// ur.z2 = mr.z2;

	// TODO expand r to correct size - i.e. size of smallest node that contains the area
	// TODO: stop r expansion in tessalate - it isn't needed

	SRectangle r(rect);
	if (rect.x1 == 0 && rect.x2 == 0) {
		auto& nlMapDmgTracker = nodeLayersMapDamageTrack.mapChangeTrackers[layerNum];

		// No more damaged areas. Finish up.
		if (nlMapDmgTracker.damageQueue.size() == 0) { return; }

		const int sectorId = nodeLayersMapDamageTrack.mapChangeTrackers[layerNum].damageQueue.front();
		const int blockIdxX = (sectorId % nodeLayersMapDamageTrack.width) * nodeLayersMapDamageTrack.cellSize;
		const int blockIdxY = (sectorId / nodeLayersMapDamageTrack.width) * nodeLayersMapDamageTrack.cellSize;

		assert(sectorId < nlMapDmgTracker.damageMap.size());
		nlMapDmgTracker.damageMap[sectorId] = false;
		nlMapDmgTracker.damageQueue.pop_front();

		r = SRectangle
			( blockIdxX
			, blockIdxY
			, blockIdxX + DAMAGE_MAP_BLOCK_SIZE
			, blockIdxY + DAMAGE_MAP_BLOCK_SIZE
			);
		
		// mark cleared section as updated so the high level updater knows about it
		// const int coarseSectorId =
		// 		( blockIdxY / nodeLayersCoarseMapUpdateTrack.cellSize ) * nodeLayersCoarseMapUpdateTrack.width
		// 		+ blockIdxX / nodeLayersCoarseMapUpdateTrack.cellSize;
		// auto& nlQuadUpdateTracker = nodeLayersCoarseMapUpdateTrack.mapChangeTrackers[layerNum];
		// assert(coarseSectorId < nodeLayersCoarseMapUpdateTrack.width*nodeLayersCoarseMapUpdateTrack.height);
		// // LOG("%s: coarseSectorId %d = %d (cellSize = %d)", __func__
		// // 		, coarseSectorId, int(nlQuadUpdateTracker.damageMap[coarseSectorId])
		// // 		, nodeLayersCoarseMapUpdateTrack.cellSize);
		// if (!nlQuadUpdateTracker.damageMap[coarseSectorId]) {
		// 	nlQuadUpdateTracker.damageMap[coarseSectorId] = true;
		// 	nlQuadUpdateTracker.damageQueue.push_back(coarseSectorId);
		// }
	}

	INode* containingNode = nodeLayers[layerNum].GetNodeThatEncasesPowerOfTwoArea(r);
	SRectangle re(containingNode->xmin(), containingNode->zmin(), containingNode->xmax(), containingNode->zmax());

	assert(re.x1 <= r.x1);
	assert(re.z1 <= r.z1);
	assert(re.x2 >= r.x2);
	assert(re.z2 >= r.z2); // TODO: re can be dropped

	updateThreadData[currentThread].InitUpdate(r, *containingNode, *md, currentThread);
	const bool needTesselation = nodeLayers[layerNum].Update(/* r, md, */updateThreadData[currentThread]);

	// process the affected root nodes.

	// LOG("%s: [%d] needTesselation=%d, wantTesselation=%d", __func__, layerNum, (int)needTesselation, (int)wantTesselation);

	if (needTesselation) {
		SRectangle ur(re.x1, re.z1, re.x2, re.z2);
		auto& nodeLayer = nodeLayers[layerNum];

		containingNode->PreTesselate(nodeLayers[layerNum], re, ur, 0, &updateThreadData[currentThread]);

		pathCache.SetLayerPathCount(layerNum, 200); // TODO sort out placeholder.
		pathCache.MarkDeadPaths(re, layerNum);

		#ifndef QTPFS_CONSERVATIVE_NEIGHBOR_CACHE_UPDATES
		nodeLayers[layerNum].ExecNodeNeighborCacheUpdates(ur, updateThreadData[currentThread]);
		#endif
	}
}

// note that this is called twice per object:
// height-map changes, then blocking-map does
void QTPFS::PathManager::TerrainChange(unsigned int x1, unsigned int z1,  unsigned int x2, unsigned int z2, unsigned int type) {
	if (!IsFinalized())
		return;

	MapChanged(x1, z1, x2, z2);
}

void QTPFS::PathManager::MapChanged(int x1, int y1, int x2, int y2) {
	const int res = DAMAGE_MAP_BLOCK_SIZE;

	const auto layers = nodeLayers.size();
	for (int i = 0; i < layers; ++i) {
		auto& nlChangeTracker = nodeLayersMapDamageTrack.mapChangeTrackers[i];
		const int w = nodeLayersMapDamageTrack.width;
		const int h = nodeLayersMapDamageTrack.height;

		auto* moveDef = moveDefHandler.GetMoveDefByPathType(i);
		int xsizeh = moveDef->xsizeh;
		int zsizeh = moveDef->zsizeh;
		const int2 min  { std::max((x1-xsizeh) / res, 0)
						, std::max((y1-zsizeh) / res, 0)};
		const int2 max  { std::min((x2+xsizeh) / res, (w-1))
						, std::min((y2+zsizeh) / res, (h-1))};

		for (int y = min.y; y <= max.y; ++y) {
			int quad = min.x + y*w;
			for (int x = min.x; x <= max.x; ++x, ++quad) {
				if (!nlChangeTracker.damageMap[quad]) {
					nlChangeTracker.damageMap[quad] = true;
					nlChangeTracker.damageQueue.emplace_back(quad);
				}
			}	
		}
	}
}

void QTPFS::PathManager::Update() {
	SCOPED_TIMER("Sim::Path");
	{
		systemUtils.NotifyUpdate();
	}
	{
		SCOPED_TIMER("Sim::Path::Requests");
		ThreadUpdate();
	}
	{
		// start off with simple update
		// if (mapChangeTrack.damageQueue.empty()) return;

		SCOPED_TIMER("Sim::Path::MapUpdates");

		RequestMaxSpeedModRefreshForLayer(0);

		auto numBlocksToUpdate = [this](int layerNum) {
			int blocksToUpdate = 0;
			int updatedBlocks = nodeLayersMapDamageTrack.mapChangeTrackers[layerNum].damageQueue.size();
			{
				constexpr int BLOCKS_TO_UPDATE = 16;
				const int progressiveUpdates = std::ceil(updatedBlocks * (1.f / (BLOCKS_TO_UPDATE<<3)) * modInfo.pfUpdateRateScale);
				constexpr int MIN_BLOCKS_TO_UPDATE = 0;
				constexpr int MAX_BLOCKS_TO_UPDATE = std::max<int>(BLOCKS_TO_UPDATE, MIN_BLOCKS_TO_UPDATE);

				blocksToUpdate = Clamp(progressiveUpdates, MIN_BLOCKS_TO_UPDATE, MAX_BLOCKS_TO_UPDATE);
			
				// LOG("[%d] blocksToUpdate=%d updatedBlocks=%d [%f]"
				// 		, layerNum, blocksToUpdate, updatedBlocks, modInfo.pfUpdateRateScale);
			}
			return blocksToUpdate;
		};

		SRectangle rect(0,0,0,0);
		for_mt(0, nodeLayers.size(), [this, &rect, &numBlocksToUpdate](const int index) {
			int curThread = ThreadPool::GetThreadNum();
			int layerNum = nodeLayerUpdatePriorityOrder[index];
			int blocksToUpdate = numBlocksToUpdate(layerNum);
			for (int i = 0; i < blocksToUpdate; ++i) { UpdateNodeLayer(layerNum, rect, curThread); }
			// UpdateNodeLayerLowRes(layerNum, curThread);
		});

		// Mark all dirty paths so that they can be recalculated
		int pathsMarkedDirty = 0;
		for (auto& layerDirtyPaths : pathCache.dirtyPaths) {
			// LOG("%s: start: %d", __func__, (int)layerDirtyPaths.size());
			for (auto pathEntity : layerDirtyPaths) {
				assert(registry.valid(pathEntity));
				// assert(!registry.all_of<PathIsDirty>(pathEntity));
				// LOG("%s: alreadyDirty=%d, pathEntity=%x", __func__, (int)registry.all_of<PathIsDirty>(pathEntity)
				// 		, (int)pathEntity);

				// TODO: perhaps not mark paths multiple times if multiple blocks are updated?
				if (registry.all_of<PathIsDirty>(pathEntity)) { continue; }

				registry.emplace<PathIsDirty>(pathEntity);
				RemovePathFromShared(pathEntity);
				pathsMarkedDirty++;
			}
			layerDirtyPaths.clear();
			// LOG("%s: end: %d", __func__, (int)layerDirtyPaths.size());
		}
		if (refreshDirtyPathRateFrame == QTPFS_LAST_FRAME && pathsMarkedDirty > 0)
			refreshDirtyPathRateFrame = gs->frameNum + GAME_SPEED;

		// assert(sectorId < mapChangeTrack.damageMap.size());
		// mapChangeTrack.damageMap[sectorId] = false;
		// mapChangeTrack.damageQueue.pop_front();
	}
}

__FORCE_ALIGN_STACK__
void QTPFS::PathManager::ThreadUpdate() {
	QueueDeadPathSearches();
	ExecuteQueuedSearches();
}


		// TODO:
		// - don't clear path very frame [DONE]
		// - clear on clear [DONE]
		// - on remove path, switch shared path [DONE]
		// - on invalidate [DONE]
		// - on invalidate, remove path -- actually all paths
		//   - make sure loop can handle that
		//   - in fact all linked paths can take answer of the first
		// - on running through paths check on shared.
		//   - if head is set then copy it [DONE]
		//   - if head is unset [DONE]
		//     - if current is head, do query, propogate to all unsets [DONE]
		//     - otherwise skip [DONE]
		//   - return states for searches need updating [DONE]
		//   - sharedFinalize needs checking and fixing [DONE]


void QTPFS::PathManager::InitializeSearch(entt::entity searchEntity) {
	ZoneScoped;
	assert(registry.all_of<PathSearch>(searchEntity));
	PathSearch* search = &registry.get<PathSearch>(searchEntity);
	int pathType = search->GetPathType();

	assert(pathType < nodeLayers.size());
	NodeLayer& nodeLayer = nodeLayers[pathType];

	entt::entity pathEntity = (entt::entity)search->GetID();
	if (registry.valid(pathEntity)) {
		assert(registry.all_of<IPath>(pathEntity));
		IPath* path = &registry.get<IPath>(pathEntity);
		search->Initialize(&nodeLayer, &pathCache, path->GetSourcePoint(), path->GetTargetPoint(), MAP_RECTANGLE);
		path->SetHash(search->GetHash());

		if (path->GetSynced() == true) {
			SharedPathMap::iterator sharedPathsIt = sharedPaths.find(path->GetHash());
			if (sharedPathsIt == sharedPaths.end()) {
				registry.emplace<SharedPathChain>(pathEntity, pathEntity, pathEntity);
				sharedPaths[path->GetHash()] = pathEntity;
			} else {
				linkedListHelper.InsertChain<SharedPathChain>(sharedPaths[path->GetHash()], pathEntity);
			}
		}
	}
}

void QTPFS::PathManager::ExecuteQueuedSearches() {
	ZoneScoped;
	//if (pathSearches.empty()) return;

	auto pathView = registry.view<PathSearch>();

	// execute pending searches collected via
	// RequestPath and QueueDeadPathSearches
	pathSearches.clear();
	pathSearches.reserve(pathView.size());
	pathView.each([this](entt::entity entity){
			InitializeSearch(entity);
			pathSearches.emplace_back(entity);
		});

	for_mt(0, pathView.size(), [this, &pathView](int i){
		entt::entity pathSearchEntity = pathSearches[i];

		assert(registry.valid(pathSearchEntity));
		assert(registry.all_of<PathSearch>(pathSearchEntity));
		PathSearch* search = &pathView.get<PathSearch>(pathSearchEntity);
		int pathType = search->GetPathType();
		NodeLayer& nodeLayer = nodeLayers[pathType];
		ExecuteSearch(search, nodeLayer, pathCache, pathType);
	});

	// const auto CleanUpSearch = [this](QTPFS::PathSearch *search){
	for (auto pathSearchEntity : pathView) {
		// int pathType = search->GetPathType();
		// PathCache& pathCache = pathCaches;
		// IPath* path = pathCache.GetTempPath(search->GetID());
		assert(registry.valid(pathSearchEntity));
		assert(registry.all_of<PathSearch>(pathSearchEntity));
		PathSearch* search = &pathView.get<PathSearch>(pathSearchEntity);
		entt::entity pathEntity = (entt::entity)search->GetID();
		if (registry.valid(pathEntity)) {
			IPath* path = registry.try_get<IPath>(pathEntity);
			if (path != nullptr) {
				if (search->PathWasFound()) {
					registry.remove<PathIsTemp>(pathEntity);
					registry.remove<PathIsDirty>(pathEntity);
					registry.remove<PathSearchRef>(pathEntity);
				} else {
					DeletePath(path->GetID());
				}
			}
		}
		// delete search;
		// LOG("%s: delete search %x", __func__, entt::to_integral(pathSearchEntity));
		registry.destroy(pathSearchEntity);
	}

	// std::for_each(pathSearches.begin(), pathSearches.end(), CleanUpSearch);
	// pathSearches.clear();
}

bool QTPFS::PathManager::ExecuteSearch(
	PathSearch* search,
	NodeLayer& nodeLayer,
	PathCache& pathCache,
	unsigned int pathType
) {
	ZoneScoped;
	// PathSearch* search = *searchesIt;
	// IPath* path = pathCache.GetTempPath(search->GetID());
	entt::entity pathEntity = (entt::entity)search->GetID();
	if (!registry.valid(pathEntity))
		return false;

	IPath* path = registry.try_get<IPath>(pathEntity);

	int currentThread = ThreadPool::GetThreadNum();

	assert(search != nullptr);
	// assert(path != nullptr);

	// temp-path might have been removed already via
	// DeletePath before we got a chance to process it
	// if (path->GetID() == 0)
	if (path == nullptr)
		return false;

	// assert(search->GetID() != 0);
	assert(path->GetID() == search->GetID());

	bool synced = path->GetSynced();

	// search->Initialize(&nodeLayer, &pathCache
	// 		, path->GetSourcePoint(), path->GetTargetPoint()
	// 		, MAP_RECTANGLE, &searchThreadData[currentThread]);
	// path->SetHash(search->GetHash(mapDims.mapx * mapDims.mapy, pathType));

	entt::entity chainHeadEntity = entt::null;
	if (synced)
	{
		SharedPathMap::const_iterator sharedPathsIt = sharedPaths.find(path->GetHash());
		assert(sharedPathsIt != sharedPaths.end());

		chainHeadEntity = sharedPathsIt->second;
		// LOG("%s: chainHeadEntity %x != pathEntity %x", __func__
		// 		, entt::to_integral(chainHeadEntity), entt::to_integral(pathEntity));
		if (chainHeadEntity != pathEntity){
			bool pathIsCopyable = !registry.all_of<PathSearchRef>(chainHeadEntity);
			if (pathIsCopyable) {
				// LOG("%s: pathEntity %x pathIsCopyable = %d", __func__
				// 		, entt::to_integral(pathEntity), int(pathIsCopyable));
				auto& headChainPath = registry.get<IPath>(chainHeadEntity);
				search->SharedFinalize(&headChainPath, path);
			}
			return false;
		}
	}

	search->InitializeThread(&searchThreadData[currentThread]);

	if (search->Execute(searchStateOffset)) {
		search->Finalize(path);

		if (chainHeadEntity == pathEntity){
			ZoneScopedN("post-check shared path");
			// TODO: walk chain backwards and stop early?
			// copy results to all applicable paths
			linkedListHelper.ForEachInChain<SharedPathChain>(chainHeadEntity, [this, path, chainHeadEntity](entt::entity next) {
				if (next == chainHeadEntity) { return; }

				auto* linkedPath = &registry.get<IPath>(next);
				auto* searchRef = registry.try_get<PathSearchRef>(next);
				if (searchRef == nullptr) { return; }

				assert(registry.all_of<PathSearch>(searchRef->value));
				auto& chainSearch = registry.get<PathSearch>(searchRef->value);
				chainSearch.SharedFinalize(path, linkedPath);
			});
		}

		#ifdef QTPFS_TRACE_PATH_SEARCHES
		pathTraces[path->GetID()] = search->GetExecutionTrace();
		#endif
	}

	return true;
}

void QTPFS::PathManager::QueueDeadPathSearches() {
	ZoneScoped;
	auto pathUpdatesView = registry.view<IPath, PathIsToBeUpdated>();
	if (pathUpdatesView.size_hint() == 0 && gs->frameNum >= refreshDirtyPathRateFrame) {
		// LOG("%s: pathUpdatesView=%d,frame=%d>%d", __func__
		// 		, (int)pathUpdatesView.size_hint(), gs->frameNum, refreshDirtyPathRateFrame
		// 		);
		auto dirtyView = registry.view<PathIsDirty>();
		auto pathsToUpdate = dirtyView.size();
		// LOG("%s: dirtyView=%d", __func__, (int)pathsToUpdate);
		if (pathsToUpdate > 0) {
			for (auto path : dirtyView) {
				assert(!registry.any_of<PathIsToBeUpdated>(path));
				registry.emplace<PathIsToBeUpdated>(path);
			}
			updateDirtyPathRate = pathsToUpdate / GAME_SPEED;
			updateDirtyPathRemainder = pathsToUpdate % GAME_SPEED;
			// LOG("%s: updateDirtyPathRate=%d,updateDirtyPathRemainder=%d", __func__
			// 		, updateDirtyPathRate, updateDirtyPathRemainder
			// 		);
		}
		refreshDirtyPathRateFrame = QTPFS_LAST_FRAME;
	}
	
	// auto pathUpdatesView = registry.view<IPath, PathIsToBeUpdated>();
	// LOG("%s: pathUpdatesView=%d", __func__, (int)pathUpdatesView.size_hint());
	if (pathUpdatesView.size_hint() > 0) {
		auto rate = std::min(updateDirtyPathRate + (updateDirtyPathRemainder-- > 0), (int)pathUpdatesView.size_hint());
		updateDirtyPathRemainder += (updateDirtyPathRemainder < 0);
		// LOG("%s: rate=%d", __func__, rate);
		std::for_each_n(pathUpdatesView.begin(), rate, [this, &pathUpdatesView](auto entity) {
			assert(registry.valid(entity));
			IPath* path = &pathUpdatesView.get<IPath>(entity);

			assert(path->GetPathType() < moveDefHandler.GetNumMoveDefs());
			const MoveDef* moveDef = moveDefHandler.GetMoveDefByPathType(path->GetPathType());
			RequeueSearch(path);

			assert(registry.all_of<PathIsToBeUpdated>(entity));
			registry.remove<PathIsToBeUpdated>(entity);
		});
	}
}

unsigned int QTPFS::PathManager::QueueSearch(
	const CSolidObject* object,
	const MoveDef* moveDef,
	const float3& sourcePoint,
	const float3& targetPoint,
	const float radius,
	const bool synced
) {
	assert(!ThreadPool::inMultiThreadedSection);

	// NOTE:
	//     all paths get deleted by the cache they are in;
	//     all searches get deleted by subsequent Update's
	// NOTE:
	//     the path-owner object handed to us can never become
	//     dangling (even with delayed execution) because ~GMT
	//     calls DeletePath, which ensures any path is removed
	//     from its cache before we get to ExecuteSearch

	entt::entity pathEntity = registry.create();
	assert(!registry.all_of<IPath>(pathEntity));
	IPath* newPath = &(registry.emplace<IPath>(pathEntity));
	// LOG("%s: newPath %p", __func__, newPath);

	entt::entity searchEntity = registry.create();
	PathSearch* newSearch = &registry.emplace<PathSearch>(searchEntity, PATH_SEARCH_ASTAR);

	if (synced) {
		assert(object->pos.x == sourcePoint.x);
		assert(object->pos.z == sourcePoint.z);
	}
	assert(targetPoint.x >= 0.f);
	assert(targetPoint.z >= 0.f);
	assert(targetPoint.x / SQUARE_SIZE < mapDims.mapx);
	assert(targetPoint.z / SQUARE_SIZE < mapDims.mapy);

	assert(newPath != nullptr);
	assert(newSearch != nullptr);

	// 0 is considered a null path. Entity id 0 should have been taken by the pathing system itself.
	assert(pathEntity != (entt::entity)0);

	// const CUnit* unit = dynamic_cast<const CUnit*>(object);

	// LOG("%s: %s _ %d -> %d ", __func__
	// 		, unit != nullptr ? unit->unitDef->name.c_str() : "non-unit"
	// 		, (oldPath != nullptr) ? oldPath->GetPathType() : moveDef->pathType
	// 		, moveDef->pathType
	// 		);

	// NOTE:
	//     the unclamped end-points are temporary
	//     zero is a reserved ID, so pre-increment
	newPath->SetID((int)pathEntity);
	newPath->SetRadius(radius);
	newPath->SetSynced(synced);
	newPath->AllocPoints(2);
	newPath->SetOwner(object);
	newPath->SetSourcePoint(sourcePoint);
	newPath->SetTargetPoint(targetPoint);
	newPath->SetPathType(moveDef->pathType);

	registry.emplace<PathIsTemp>(pathEntity);
	registry.emplace<PathSearchRef>(pathEntity, searchEntity);

	if (!synced)
		registry.emplace<PathIsUnsynced>(pathEntity);

	// assert((pathCaches[moveDef->pathType].GetTempPath(newPath->GetID()))->GetID() == 0);

	newSearch->SetID(newPath->GetID());
	newSearch->SetTeam((object != nullptr)? object->team: teamHandler.ActiveTeams());
	newSearch->SetPathType(newPath->GetPathType());


	// if (moveDef->pathType == 2) {
	// 	// LOG("%s: com path", __func__);
	// 	newPath->SetPathType(moveDef->pathType);
	// }

	// map the path-ID to the index of the cache that stores it
	// pathTypes[newPath->GetID()] = moveDef->pathType;
	// pathSearches.push_back(newSearch);
	// pathCaches[moveDef->pathType].AddTempPath(newPath);

	// LOG("%s: %s (%x) %d -> %d ", __func__
	// 		, unit != nullptr ? unit->unitDef->name.c_str() : "non-unit"
	// 		, newPath->GetID()
	// 		, (oldPath != nullptr) ? oldPath->GetPathType() : -1
	// 		, moveDef->pathType
	// 		);

	// LOG("%s: [%d] (%f,%f) -> (%f,%f)", __func__, newPath->GetPathType()
	// 		, sourcePoint.x, sourcePoint.z, targetPoint.x, targetPoint.z);

	return (newPath->GetID());
}

unsigned int QTPFS::PathManager::RequeueSearch(
	IPath* oldPath
) {
	// Always create the search object first to ensure pathEntity can never be 0 (which is
	// considered a non-path)
	entt::entity searchEntity = registry.create();
	PathSearch* newSearch = &registry.emplace<PathSearch>(searchEntity, PATH_SEARCH_ASTAR);
	assert(oldPath != nullptr);
	assert(newSearch != nullptr);
	assert(oldPath->GetID() != 0);
	assert((entt::entity)oldPath->GetID() != entt::null);

	const CSolidObject* obj = oldPath->GetOwner();
	const float3& pos = (obj != nullptr)? obj->pos: oldPath->GetSourcePoint();

	const auto targetPoint = oldPath->GetTargetPoint();

	oldPath->SetHash(-1);
	// newPath->SetID(oldPath->GetID());
	oldPath->SetNextPointIndex(0);
	oldPath->SetNumPathUpdates(oldPath->GetNumPathUpdates() + 1);
	// newPath->SetRadius(oldPath->GetRadius());
	// newPath->SetSynced(oldPath->GetSynced());

	// start re-request from the current point
	// along the path, not the original source
	// (oldPath->GetSourcePoint())
	oldPath->AllocPoints(2);
	// newPath->SetOwner(oldPath->GetOwner());
	oldPath->SetSourcePoint(pos);
	oldPath->SetTargetPoint(targetPoint);
	// newPath->SetPathType(moveDef->pathType);

	newSearch->SetID(oldPath->GetID());
	// newSearch->SetTeam(teamHandler.ActiveTeams());

	auto object = oldPath->GetOwner();
	newSearch->SetTeam((object != nullptr)? object->team: teamHandler.ActiveTeams());
	newSearch->SetPathType(oldPath->GetPathType());

	registry.emplace_or_replace<PathIsTemp>((entt::entity)oldPath->GetID());

	// LOG("%s: [%d] (%f,%f) -> (%f,%f)", __func__, oldPath->GetPathType()
	// 		, pos.x, pos.z, targetPoint.x, targetPoint.z);

	return (oldPath->GetID());
}

void QTPFS::PathManager::UpdatePath(const CSolidObject* owner, unsigned int pathID) {
	// const PathTypeMapIt pathTypeIt = pathTypes.find(pathID);

	// if (pathTypeIt != pathTypes.end()) {
		// PathCache& pathCache = pathCaches[pathTypeIt->second];
		// IPath* livePath = pathCache.GetLivePath(pathID);

		// entt::entity pathEntity = (entt::entity)pathID;
		// IPath* livePath = &registry.get<IPath>(pathEntity);

		// if (livePath->GetID() != 0) {
		// 	assert(owner == livePath->GetOwner());
		// }
	// }
}

void QTPFS::PathManager::DeletePath(unsigned int pathID) {
	// const PathTypeMapIt pathTypeIt = pathTypes.find(pathID);
	const PathTraceMapIt pathTraceIt = pathTraces.find(pathID);

	// LOG("%s: Delete Requested for path %x", __func__, pathID);

	// if (pathTypeIt != pathTypes.end()) {
	// 	PathCache& pathCache = pathCaches;
	// 	pathCache.DelPath(pathID);

	// 	pathTypes.erase(pathTypeIt);
	// }

	// LOG("%s: %x", __func__, pathID);
	RemovePathFromShared((entt::entity)pathID);
	pathCache.DelPath(pathID);

	if (pathTraceIt != pathTraces.end()) {
		delete (pathTraceIt->second);
		pathTraces.erase(pathTraceIt);
	}
}

void QTPFS::PathManager::RemovePathFromShared(entt::entity entity) {
	if (!registry.valid(entity)) return;
	if (!registry.all_of<SharedPathChain>(entity)) return;

	IPath* path = &registry.get<IPath>(entity);
	bool synced = path->GetSynced();
	auto iter = sharedPaths.find(path->GetHash());
	assert(iter != sharedPaths.end());
	if (iter->second == entity) {
		auto& chain = registry.get<SharedPathChain>(entity);
		if (chain.next == entity) {
			sharedPaths.erase(path->GetHash());
			// LOG("%s: shared path %lld head is now empty", __func__, path->GetHash());
		} else {
			sharedPaths[path->GetHash()] = chain.next;
			// LOG("%s: shared path %lld head is now %x", __func__, path->GetHash(), entt::to_integral(chain.next));
		}
	}

	linkedListHelper.RemoveChain<SharedPathChain>(entity);
}

unsigned int QTPFS::PathManager::RequestPath(
	CSolidObject* object,
	const MoveDef* moveDef,
	float3 sourcePoint,
	float3 targetPoint,
	float radius,
	bool synced
) {
	unsigned int returnPathId = 0;

	if (!IsFinalized())
		return returnPathId;

	returnPathId = QueueSearch(object, moveDef, sourcePoint, targetPoint, radius, synced);

	if (!synced && returnPathId != 0) {
		entt::entity pathEntity = entt::entity(returnPathId);
		assert(registry.valid(pathEntity));
		entt::entity pathSearchEntity = registry.get<PathSearchRef>(pathEntity).value;
		assert(registry.valid(pathSearchEntity));
		InitializeSearch(pathSearchEntity);

		PathSearch& pathSearch = registry.get<PathSearch>(pathSearchEntity);
		int pathType = pathSearch.GetPathType();
		NodeLayer& nodeLayer = nodeLayers[pathType];
		ExecuteSearch(&pathSearch, nodeLayer, pathCache, pathType);

		if (registry.valid(pathEntity)) {
			IPath* path = registry.try_get<IPath>(pathEntity);
			if (path != nullptr) {
				if (pathSearch.PathWasFound()) {
					registry.remove<PathIsTemp>(pathEntity);
					registry.remove<PathIsDirty>(pathEntity);
					registry.remove<PathSearchRef>(pathEntity);
				} else {
					DeletePath(path->GetID());
					returnPathId = 0;
				}
			}
		}
		registry.destroy(pathSearchEntity);
	}

	return returnPathId;
}

bool QTPFS::PathManager::PathUpdated(unsigned int pathID) {
	// const PathTypeMapIt pathTypeIt = pathTypes.find(pathID);

	// if (pathTypeIt == pathTypes.end())
	// 	return false;

	// PathCache& pathCache = pathCaches[pathTypeIt->second];
	// IPath* livePath = pathCache.GetLivePath(pathID);

	entt::entity pathEntity = (entt::entity)pathID;
	if (!registry.valid(pathEntity)) { return false; }
	IPath* livePath = registry.try_get<IPath>(pathEntity);

	// if (livePath->GetID() == 0)
	if (livePath == nullptr)
		return false;

	if (livePath->GetNumPathUpdates() == 0)
		return false;

	// livePath->SetNumPathUpdates(livePath->GetNumPathUpdates() - 1);
	livePath->SetNumPathUpdates(0);
	return true;
}



float3 QTPFS::PathManager::NextWayPoint(
	const CSolidObject*, // owner
	unsigned int pathID,
	unsigned int, // numRetries
	float3 point,
	float, // radius,
	bool synced
) {
	// in misc since it is called from many points
	SCOPED_TIMER("Misc::Path::NextWayPoint");

	// const PathTypeMap::const_iterator pathTypeIt = pathTypes.find(pathID);
	const float3 noPathPoint = -XZVector;

	if (!IsFinalized())
		return noPathPoint;
	if (!synced)
		return noPathPoint;

	// dangling ID after a re-request failure or regular deletion
	// return an error-vector so GMT knows it should stop the unit
	// if (pathTypeIt == pathTypes.end())
	// 	return noPathPoint;

	// IPath* tempPath = pathCaches[pathTypeIt->second].GetTempPath(pathID);
	// IPath* livePath = pathCaches[pathTypeIt->second].GetLivePath(pathID);

	entt::entity pathEntity = (entt::entity)pathID;
	if (!registry.valid(pathEntity)) { return noPathPoint; }
	IPath* livePath = registry.try_get<IPath>(pathEntity);
	if (livePath == nullptr) { return noPathPoint; }

	// if (tempPath->GetID() != 0) {
	if (registry.all_of<PathIsTemp>(pathEntity)) {
		// path-request has not yet been processed (so ID still maps to
		// a temporary path); just set the unit off toward its target to
		// hide latency
		//
		// <curPoint> is initially the position of the unit requesting a
		// path, but later changes to the subsequent values returned here
		//
		// NOTE:
		//     if the returned point P is too far away, then a unit U will
		//     never switch to its live-path even after it becomes available
		//     (because NextWayPoint is not called again until U gets close
		//     to P), so always keep it a fixed small distance in front
		//
		//     make the y-coordinate -1 to indicate these are temporary
		//     waypoints to GMT and should not be followed religiously
		const float3& sourcePoint = point;
		// const float3& targetPoint = tempPath->GetTargetPoint();
		const float3& targetPoint = livePath->GetTargetPoint();
		const float3  targetDirec = (targetPoint - sourcePoint).SafeNormalize() * SQUARE_SIZE;
		return float3(sourcePoint.x + targetDirec.x, -1.0f, sourcePoint.z + targetDirec.z);
	}
	// if (livePath->GetID() == 0) {
	if (registry.all_of<PathIsDirty>(pathEntity)) {
		// the request WAS processed but then immediately undone by a
		// TerrainChange --> MarkDeadPaths event in the same frame as
		// NextWayPoint (so pathID is only in deadPaths)
		return point;
	}

	float minRadiusSq = QTPFS_POSITIVE_INFINITY;

	unsigned int minPointIdx = livePath->GetNextPointIndex();
	unsigned int nxtPointIdx = 1;

	for (unsigned int i = (livePath->GetNextPointIndex()); i < (livePath->NumPoints() - 1); i++) {
		const float radiusSq = (point - livePath->GetPoint(i)).SqLength2D();

		// find waypoints <p0> and <p1> such that <point> is
		// "in front" of p0 and "behind" p1 (ie. in between)
		//
		// we do this rather than the radius-based search
		// since depending on the value of <radius> we may
		// or may not find a "next" node (even though one
		// always exists)
		const float3& p0 = livePath->GetPoint(i    ), v0 = float3(p0.x - point.x, 0.0f, p0.z - point.z);
		const float3& p1 = livePath->GetPoint(i + 1), v1 = float3(p1.x - point.x, 0.0f, p1.z - point.z);

		// NOTE:
		//     either v0 or v1 can be a zero-vector (p0 == point or p1 == point)
		//     in those two cases the dot-product is meaningless so we skip them
		//     vectors are NOT normalized, so it can happen that NO case matches
		//     and we must fall back to the radius-based closest point
		if (v0.SqLength() < 0.1f) { nxtPointIdx = i + 1; break; }
		if (v1.SqLength() < 0.1f) { nxtPointIdx = i + 2; break; }
		if (v0.dot(v1) <= -0.01f) { nxtPointIdx = i + 1;        }

		if (radiusSq < minRadiusSq) {
			minRadiusSq = radiusSq;
			minPointIdx = i + 0;
		}
	}

	// handle a corner-case in which a unit is at the start of its path
	// and the goal is in front of it, but on the other side of a cliff
	if ((livePath->GetNextPointIndex() == 0) && (nxtPointIdx == (livePath->NumPoints() - 1)))
		nxtPointIdx = 1;

	if (minPointIdx < nxtPointIdx) {
		// if close enough to at least one waypoint <i>,
		// switch to the point immediately following it
		livePath->SetNextPointIndex(nxtPointIdx);
	} else {
		// otherwise just pick the closest point
		livePath->SetNextPointIndex(minPointIdx);
	}

	return (livePath->GetPoint(livePath->GetNextPointIndex()));
}



void QTPFS::PathManager::GetPathWayPoints(
	unsigned int pathID,
	std::vector<float3>& points,
	std::vector<int>& starts
) const {
	// const PathTypeMap::const_iterator pathTypeIt = pathTypes.find(pathID);

	if (!IsFinalized())
		return;
	// if (pathTypeIt == pathTypes.end())
	// 	return;

	// PathCache& pathCache = pathCaches[pathTypeIt->second];
	// const IPath* path = cache.GetLivePath(pathID);
	entt::entity pathEntity = (entt::entity)pathID;
	if (!registry.valid(pathEntity))
		return;

	const IPath* path = registry.try_get<IPath>(pathEntity);

	// if (path->GetID() == 0)
	if (path == nullptr)
		return;

	// maintain compatibility with the tri-layer legacy PFS
	points.resize(path->NumPoints());
	starts.resize(3, 0);

	for (unsigned int n = 0; n < path->NumPoints(); n++) {
		points[n] = path->GetPoint(n);
	}
}

int2 QTPFS::PathManager::GetNumQueuedUpdates() const {
	int2 data;

	data.x = updateDirtyPathRate;//mapChangeTrack.damageQueue.size();// registry.size();
	data.y = updateDirtyPathRemainder;

	return data;
}
