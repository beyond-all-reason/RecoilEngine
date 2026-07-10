// macOS-specific CPU topology implementation
// Simplified stub for macOS - uses system calls for basic info

#include "System/Platform/CpuTopology.h"

#include <unistd.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/thread_act.h>

namespace cpu_topology {

ThreadPinPolicy GetThreadPinPolicy()
{
	return THREAD_PIN_POLICY_NONE;
}

ProcessorMasks GetProcessorMasks()
{
	ProcessorMasks masks;
	// On macOS, we don't have direct access to processor topology info
	// Set a default mask that allows all CPUs
	int cpuCount = 0;
	size_t len = sizeof(cpuCount);
	if (sysctlbyname("hw.logicalcpu", &cpuCount, &len, NULL, 0) == 0) {
		// Calculate mask: all available CPUs
		for (int i = 0; i < cpuCount && i < 32; ++i) {
			masks.performanceCoreMask |= (1 << i);
		}
	}
	return masks;
}

ProcessorCaches GetProcessorCache()
{
	ProcessorCaches caches;
	// On macOS, we don't have direct access to cache topology
	// Return empty caches - this is acceptable for stub
	ProcessorGroupCaches group;
	group.groupMask = 0xFFFFFFFF; // All CPUs share the same cache group
	group.cacheSizes[0] = 0; // L1
	group.cacheSizes[1] = 0; // L2
	group.cacheSizes[2] = 0; // L3
	caches.groupCaches.push_back(group);
	return caches;
}

} // namespace cpu_topology
