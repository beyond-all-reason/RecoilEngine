#pragma once

#include <cstdint>
#include <vector>

namespace cpu_topology {

// Identifies policy for how threads are pinned to logical processors. Windows/Linux work better with different
// policies.
enum ThreadPinPolicy {
	THREAD_PIN_POLICY_NONE,          // No pinning
	THREAD_PIN_POLICY_PER_PERF_CORE, // Pin each thread to one unique performance core (don't use HT/SMT)
	THREAD_PIN_POLICY_ANY_PERF_CORE  // Pin threads so that they share a group of performance cores (don't use HT/SMT)
	                                 // The same cores will be used as per THREAD_PIN_POLICY_PER_PERF_CORE, but the
	                                 // threads are allowed to move between cores.
};

ThreadPinPolicy GetThreadPinPolicy();

static constexpr uint32_t MAX_CACHE_LEVELS = 3;

struct ProcessorMasks {
	uint32_t performanceCoreMask = 0;
	uint32_t efficiencyCoreMask = 0;
	uint32_t hyperThreadLowMask = 0;
	uint32_t hyperThreadHighMask = 0;
};

struct ProcessorGroupCaches {
	uint32_t groupMask = 0;
	uint32_t cacheSizes[MAX_CACHE_LEVELS] = {0, 0, 0};
};

struct ProcessorCaches {
	std::vector<ProcessorGroupCaches> groupCaches;
};

// OS-specific implementation to get the processor masks.
ProcessorMasks GetProcessorMasks();

// OS-specific implementation to get the logical processor masks and the L3 cache they have access to.
ProcessorCaches GetProcessorCache();

}
