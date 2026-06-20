/* This file is part of the Recoil engine (GPL v2 or later), see LICENSE.html */

// Generic, platform-agnostic cpu_topology implementation for tool builds
// (e.g. unitsync) that link the thread pool for parallel work but never pin
// threads. It reports every logical core as a single group of performance
// cores with no P/E split, no SMT/hyper-threading distinction and no cache
// grouping, and requests no thread pinning. This satisfies the three
// cpu_topology symbols pulled in by CpuID/Threading without dragging in the
// real per-platform topology detection (Platform/{Linux,Win,Mac}/CpuTopology.cpp).
//
// The engine and dedicated server do NOT use this; they link the real
// per-platform implementation for sim-worker pinning.

#include "CpuTopology.h"

#include <algorithm>
#include <thread>

namespace cpu_topology {

ThreadPinPolicy GetThreadPinPolicy() {
	return THREAD_PIN_POLICY_NONE;
}

ProcessorMasks GetProcessorMasks() {
	ProcessorMasks processorMasks;

	// Masks are 32 bits wide; cap to match the real per-platform implementations
	// (MAX_CPUS == 32). Treat every logical core as a performance core so that
	// CPUID::EnumerateCores() derives a correct logical-core count for the pool.
	const unsigned int logicalCores = std::min(32u, std::thread::hardware_concurrency());

	processorMasks.performanceCoreMask =
		(logicalCores >= 32u) ? ~0u : ((1u << logicalCores) - 1u);

	return processorMasks;
}

ProcessorCaches GetProcessorCache() {
	// No cache grouping: the only consumers (the affinity/pinning helpers in
	// Threading.cpp) are never reached on the tool path.
	return ProcessorCaches{};
}

} // namespace cpu_topology
