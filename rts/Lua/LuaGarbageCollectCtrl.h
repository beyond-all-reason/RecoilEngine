/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_LUA_GARBAGE_COLLECT_CTRL_H
#define SPRING_LUA_GARBAGE_COLLECT_CTRL_H

#include <cstdint>
#include <limits>

struct SLuaGarbageCollectCtrl {
	// maximum number of lua_gc calls made in each CollectGarbage loop
	int itersPerBatch = std::numeric_limits<int>::max();

	// number of steps executed by a single lua_gc call
	int numStepsPerIter =    10;
	int minStepsPerIter =     1;
	int maxStepsPerIter = 10000;

	// CollectGarbage loop runtime bounds, in milliseconds
	float minLoopRunTime =   0.0f;
	float maxLoopRunTime = 100.0f;

	float baseRunTimeMult = 0.0f;
	float baseMemLoadMult = 0.0f;
	// KB of collector stepping owed per KB the state allocates
	float baseWorkMult = 0.0f;
	// debt (in KB) below which a call does nothing, so a near-idle state is not
	// visited every frame for a few KB (the call has a fixed cost of a few us)
	static constexpr int MIN_STEP_DEBT_KB = 16;

	// state footprint at the end of the previous CollectGarbage call
	uint64_t allocedBytesAtLastGC = 0;
	// stepping (in KB) owed to the collector but not yet performed
	int64_t stepDebtKB = 0;
};

#endif

