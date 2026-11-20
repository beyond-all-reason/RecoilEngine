/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "YardmapStatusEffectsMap.h"

#include <algorithm>
#include <cassert>
#include <cstring>

YardmapStatusEffectsMap yardmapStatusEffectsMap;

CR_BIND(YardmapStatusEffectsMap, )

CR_REG_METADATA(YardmapStatusEffectsMap, (
	CR_MEMBER(stateMap),
	CR_IGNORED(tileStride),   // rebuilt from stateMap size on PostLoad
	CR_IGNORED(exitOnlyBlockStride), // derived from stateMap, rebuilt on PostLoad
	CR_IGNORED(exitOnlyBlocks),      // ditto
	CR_POSTLOAD(PostLoad)
))

CR_BIND(YardmapStatusEffectsMap::Tile, )

CR_REG_METADATA(YardmapStatusEffectsMap::Tile, (
	CR_MEMBER(squares)
))

void YardmapStatusEffectsMap::ClearTile(int tileId) {
	assert(tileId >= 0 && tileId < static_cast<int>(stateMap.size()));

	// an aligned 8x8 tile falls entirely inside one 16x16 block, so the
	// counter can be corrected with a single subtraction
	const Tile& tile = stateMap[tileId];
	int erased = 0;
	for (int i = 0; i < TILE_AREA; ++i)
		erased += ((tile.squares[i] & EXIT_ONLY) != 0);

	if (erased != 0) {
		const int tx = (tileId % tileStride) * TILE_SIZE;
		const int tz = (tileId / tileStride) * TILE_SIZE;
		assert(exitOnlyBlocks[ExitOnlyBlockIdx(tx, tz)] >= erased);
		exitOnlyBlocks[ExitOnlyBlockIdx(tx, tz)] -= erased;
	}

	memset(&stateMap[tileId], 0, sizeof(Tile));
}

void YardmapStatusEffectsMap::RebuildExitOnlyBlocks() {
	exitOnlyBlockStride = ((mapDims.mapx - 1) >> EXIT_ONLY_BLOCK_SHIFT) + 1;
	const int blocksZ = ((mapDims.mapy - 1) >> EXIT_ONLY_BLOCK_SHIFT) + 1;
	exitOnlyBlocks.assign(exitOnlyBlockStride * blocksZ, 0);

	for (int z = 0; z < mapDims.mapy; ++z) {
		for (int x = 0; x < mapDims.mapx; ++x) {
			if (GetMapState(x, z) & EXIT_ONLY)
				++exitOnlyBlocks[ExitOnlyBlockIdx(x, z)];
		}
	}
}

void YardmapStatusEffectsMap::InitNewYardmapStatusEffectsMap() {
	const int tilesX = (mapDims.mapx + TILE_SIZE - 1) / TILE_SIZE;
	const int tilesZ = (mapDims.mapy + TILE_SIZE - 1) / TILE_SIZE;

	tileStride = tilesX;

	stateMap.clear();
	stateMap.resize(tilesX * tilesZ); // value-initialises: squares are zeroed

	RebuildExitOnlyBlocks();
}

void YardmapStatusEffectsMap::PostLoad() {
	tileStride = (mapDims.mapx + TILE_SIZE - 1) / TILE_SIZE;

	RebuildExitOnlyBlocks();
}
