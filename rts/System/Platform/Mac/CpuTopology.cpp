/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

/**
 * macOS-specific CPU topology detection using sysctl APIs.
 *
 * Apple Silicon (M1/M2/M3/M4) uses a big.LITTLE architecture with
 * Performance (P) cores and Efficiency (E) cores. This implementation
 * queries the hw.perflevel* sysctl keys to detect core types and cache
 * topology.
 *
 * LIMITATION: macOS does not support pthread_setaffinity_np(). Thread
 * placement on P-cores vs E-cores is controlled indirectly via QoS
 * classes (e.g. QOS_CLASS_USER_INTERACTIVE for P-cores,
 * QOS_CLASS_BACKGROUND for E-cores). The masks returned here are
 * informational -- they follow the convention that P-cores occupy the
 * low CPU indices and E-cores occupy the high indices, matching the
 * logical numbering exposed by macOS on Apple Silicon.
 */

#if defined(__APPLE__)

#include "System/Platform/CpuTopology.h"
#include "System/Log/ILog.h"

#include <algorithm>
#include <cstdint>
#include <sys/sysctl.h>
#include <unistd.h>

namespace cpu_topology {

#define MAX_CPUS 32  // Maximum logical CPUs (matches Linux/Windows limit)

// ---------------------------------------------------------------------------
// sysctl helpers
// ---------------------------------------------------------------------------

/// Query an integer value via sysctlbyname. Returns true on success.
static bool SysctlInt(const char* name, int* out) {
	size_t size = sizeof(*out);
	if (sysctlbyname(name, out, &size, nullptr, 0) != 0)
		return false;
	return true;
}

/// Query a 64-bit value via sysctlbyname. Returns true on success.
static bool SysctlInt64(const char* name, int64_t* out) {
	size_t size = sizeof(*out);
	if (sysctlbyname(name, out, &size, nullptr, 0) != 0)
		return false;
	return true;
}

// ---------------------------------------------------------------------------
// GetProcessorMasks
// ---------------------------------------------------------------------------

ProcessorMasks GetProcessorMasks() {
	ProcessorMasks masks;

	int nperflevels = 0;
	if (!SysctlInt("hw.nperflevels", &nperflevels) || nperflevels < 1) {
		// Fallback: no performance level info (Intel Mac or very old macOS).
		// Treat every logical CPU as a performance core.
		int ncpu = 0;
		if (!SysctlInt("hw.ncpu", &ncpu))
			ncpu = static_cast<int>(sysconf(_SC_NPROCESSORS_CONF));

		if (ncpu > MAX_CPUS)
			ncpu = MAX_CPUS;

		for (int i = 0; i < ncpu; ++i)
			masks.performanceCoreMask |= (1u << i);

		LOG("macOS CpuTopology: no perflevel info, treating all %d CPUs as performance cores.", ncpu);
		return masks;
	}

	// Apple Silicon with big.LITTLE -----------------------------------------
	// perflevel0 = Performance cores (highest perf level)
	// perflevel1 = Efficiency cores
	int pcores = 0;
	int ecores = 0;
	SysctlInt("hw.perflevel0.physicalcpu", &pcores);
	if (nperflevels >= 2)
		SysctlInt("hw.perflevel1.physicalcpu", &ecores);

	// Clamp to MAX_CPUS
	const int totalCores = pcores + ecores;
	if (totalCores > MAX_CPUS) {
		LOG_L(L_WARNING, "macOS CpuTopology: total cores (%d) exceeds MAX_CPUS (%d), clamping.", totalCores, MAX_CPUS);
		if (pcores > MAX_CPUS) pcores = MAX_CPUS;
		ecores = std::min(ecores, MAX_CPUS - pcores);
	}

	// Convention: P-cores occupy logical CPU indices [0, pcores),
	//             E-cores occupy [pcores, pcores + ecores).
	for (int i = 0; i < pcores; ++i)
		masks.performanceCoreMask |= (1u << i);

	for (int i = pcores; i < pcores + ecores; ++i)
		masks.efficiencyCoreMask |= (1u << i);

	// Apple Silicon has no SMT / hyperthreading.
	masks.hyperThreadLowMask  = 0;
	masks.hyperThreadHighMask = 0;

	LOG("macOS CpuTopology: %d P-cores, %d E-cores (perflevels=%d).", pcores, ecores, nperflevels);
	return masks;
}

// ---------------------------------------------------------------------------
// GetProcessorCache
// ---------------------------------------------------------------------------

ProcessorCaches GetProcessorCache() {
	ProcessorCaches caches;

	int nperflevels = 0;
	SysctlInt("hw.nperflevels", &nperflevels);

	int pcores = 0;
	int ecores = 0;

	if (nperflevels >= 1)
		SysctlInt("hw.perflevel0.physicalcpu", &pcores);
	if (nperflevels >= 2)
		SysctlInt("hw.perflevel1.physicalcpu", &ecores);

	// Clamp
	if (pcores + ecores > MAX_CPUS) {
		if (pcores > MAX_CPUS) pcores = MAX_CPUS;
		ecores = std::min(ecores, MAX_CPUS - pcores);
	}

	// Helper: build a ProcessorGroupCaches for a set of cores sharing the
	// same L2 cache.  macOS does not expose per-cluster cache sharing maps,
	// so we treat all P-cores as one group and all E-cores as another.

	auto addGroup = [&](int startCpu, int count, const char* l2Key) {
		if (count <= 0)
			return;

		ProcessorGroupCaches group;
		for (int i = startCpu; i < startCpu + count && i < MAX_CPUS; ++i)
			group.groupMask |= (1u << i);

		// L2 cache size (stored in cacheSizes[1], index 1 = L2)
		int64_t l2Size = 0;
		if (l2Key && SysctlInt64(l2Key, &l2Size))
			group.cacheSizes[1] = static_cast<uint32_t>(l2Size);

		// macOS does not expose per-perflevel L1 or L3 via sysctl in a
		// consistent way.  Query the system-wide L3 as a fallback.
		int64_t l3Size = 0;
		// There is no hw.perflevel*.l3cachesize, but some SoCs expose
		// hw.l3cachesize for the shared system-level cache (SLC).
		// Only P-cores typically have access to the full SLC.
		if (startCpu == 0 && SysctlInt64("hw.l3cachesize", &l3Size))
			group.cacheSizes[2] = static_cast<uint32_t>(l3Size);

		caches.groupCaches.push_back(group);
	};

	if (nperflevels >= 1)
		addGroup(0, pcores, "hw.perflevel0.l2cachesize");
	if (nperflevels >= 2)
		addGroup(pcores, ecores, "hw.perflevel1.l2cachesize");

	if (caches.groupCaches.empty()) {
		// Fallback for Intel Macs or missing perflevel data.
		int ncpu = 0;
		if (!SysctlInt("hw.ncpu", &ncpu))
			ncpu = static_cast<int>(sysconf(_SC_NPROCESSORS_CONF));
		if (ncpu > MAX_CPUS) ncpu = MAX_CPUS;

		ProcessorGroupCaches group;
		for (int i = 0; i < ncpu; ++i)
			group.groupMask |= (1u << i);

		int64_t l2 = 0;
		if (SysctlInt64("hw.l2cachesize", &l2))
			group.cacheSizes[1] = static_cast<uint32_t>(l2);

		int64_t l3 = 0;
		if (SysctlInt64("hw.l3cachesize", &l3))
			group.cacheSizes[2] = static_cast<uint32_t>(l3);

		caches.groupCaches.push_back(group);
	}

	// Sort: largest L2 first (P-cores should already be first, but be safe).
	std::ranges::stable_sort(
		caches.groupCaches,
		[](const auto& lh, const auto& rh) {
			return lh.cacheSizes[1] > rh.cacheSizes[1];
		});

	return caches;
}

// ---------------------------------------------------------------------------
// GetThreadPinPolicy
// ---------------------------------------------------------------------------

ThreadPinPolicy GetThreadPinPolicy() {
	// macOS cannot pin threads to specific cores (no pthread_setaffinity_np).
	// ANY_PERF_CORE lets the scheduler choose among the performance cores,
	// which aligns with using QoS_CLASS_USER_INTERACTIVE at the caller level.
	return THREAD_PIN_POLICY_ANY_PERF_CORE;
}

} // namespace cpu_topology

#endif // __APPLE__
