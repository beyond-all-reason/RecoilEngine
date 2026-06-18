#include "System/Platform/CpuTopology.h"
#include <sys/sysctl.h>
#include <thread>

namespace cpu_topology {

namespace {

// Read an unsigned sysctl value by name. Returns 0 if the key is unavailable
// (e.g. perflevel keys on Intel Macs or pre-Apple-Silicon kernels).
unsigned int ReadSysctlUInt(const char* name) {
	int value = 0;
	size_t valueSize = sizeof(value);
	if (sysctlbyname(name, &value, &valueSize, nullptr, 0) != 0)
		return 0;
	return (value > 0) ? static_cast<unsigned int>(value) : 0;
}

unsigned int BitsForCount(unsigned int n) {
	if (n == 0) return 0;
	if (n >= 32) return 0xFFFFFFFFu;
	return (1u << n) - 1u;
}

} // namespace

ThreadPinPolicy GetThreadPinPolicy() {
	// macOS has no pthread_setaffinity_np equivalent. Scheduling locality is
	// instead expressed via QOS classes; see Platform/Mac/ThreadSupport.cpp.
	return THREAD_PIN_POLICY_NONE;
}

ProcessorMasks GetProcessorMasks() {
	ProcessorMasks masks{};

	// Apple Silicon exposes per-perflevel core counts. perflevel0 is the
	// high-performance (P) cluster; perflevel1, when present, is the
	// efficiency (E) cluster. Intel Macs and older kernels do not expose
	// these keys; fall back to treating every core as a P-core there.
	const unsigned int numPCores = ReadSysctlUInt("hw.perflevel0.physicalcpu");
	const unsigned int numECores = ReadSysctlUInt("hw.perflevel1.physicalcpu");

	if (numPCores > 0) {
		masks.performanceCoreMask = BitsForCount(numPCores);
		// E-cores occupy the bits above the P-cores in the combined mask.
		const unsigned int totalCores = numPCores + numECores;
		const unsigned int allMask = BitsForCount(totalCores);
		masks.efficiencyCoreMask = allMask & ~masks.performanceCoreMask;
	} else {
		// Intel Mac / unknown topology: treat the visible core count as
		// homogeneous P-cores. Matches prior behavior on those targets.
		unsigned int numCores = std::thread::hardware_concurrency();
		if (numCores == 0) numCores = 4;
		masks.performanceCoreMask = BitsForCount(numCores);
		masks.efficiencyCoreMask = 0;
	}

	// macOS does not expose SMT/HT details; report all visible cores as
	// hyper-thread-low so callers consuming those masks stay consistent.
	masks.hyperThreadLowMask = masks.performanceCoreMask | masks.efficiencyCoreMask;
	masks.hyperThreadHighMask = 0;

	return masks;
}

ProcessorCaches GetProcessorCache() {
	ProcessorCaches caches;

	ProcessorGroupCaches group;
	unsigned int numCores = std::thread::hardware_concurrency();
	if (numCores == 0) numCores = 4;
	group.groupMask = BitsForCount(numCores);

	// Try to get cache sizes via sysctl
	size_t size = sizeof(uint64_t);
	uint64_t cacheSize = 0;

	if (sysctlbyname("hw.l1dcachesize", &cacheSize, &size, nullptr, 0) == 0)
		group.cacheSizes[0] = static_cast<uint32_t>(cacheSize);

	if (sysctlbyname("hw.l2cachesize", &cacheSize, &size, nullptr, 0) == 0)
		group.cacheSizes[1] = static_cast<uint32_t>(cacheSize);

	if (sysctlbyname("hw.l3cachesize", &cacheSize, &size, nullptr, 0) == 0)
		group.cacheSizes[2] = static_cast<uint32_t>(cacheSize);

	caches.groupCaches.push_back(group);
	return caches;
}

} // namespace cpu_topology
