#include "System/Platform/CpuTopology.h"

#include "System/Log/ILog.h"
#include "System/Platform/ThreadAffinityGuard.h"


#include <algorithm>
#include <bitset>
#include <cpuid.h>
#include <pthread.h>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <sched.h>

namespace cpu_topology {

#define MAX_CPUS 32  // Maximum logical CPUs
	
enum Vendor { VENDOR_INTEL, VENDOR_AMD, VENDOR_UNKNOWN };

enum CoreType { CORE_PERFORMANCE, CORE_EFFICIENCY, CORE_UNKNOWN };

// Detect CPU vendor (Intel or VENDOR_AMD)
Vendor detect_cpu_vendor() {
	unsigned int eax, ebx, ecx, edx;
	__get_cpuid(0, &eax, &ebx, &ecx, &edx);
	if (ebx == 0x756E6547) return VENDOR_INTEL; // "GenuineIntel"
	if (ebx == 0x68747541) return VENDOR_AMD;   // "AuthenticAMD"
	return VENDOR_UNKNOWN;
}

std::vector<int> get_online_cpus() {
	std::vector<int> cpus;
	std::ifstream file("/sys/devices/system/cpu/online");
	if (file) {
		// This is a comma-seperated list of ranges
		// or single values.
		// Ex: 0,2,4,6 or 0-7 or 0-3,8-15
		std::string line;
		std::getline(file, line);
		std::istringstream ss(line);
		int min_cpu;
		int max_cpu;
		char sep;
		while (ss >> min_cpu) {
			if ((ss >> sep) && sep == '-') {
				// Range of CPUs separted by '-'
				if (!(ss >> max_cpu)) {
					// Should not ever happen (would need to be a malformed online file)
					if (min_cpu >= MAX_CPUS) {
						LOG_L(L_WARNING, "CPU index %d exceeds bitset limit.", min_cpu);
					} else {
						cpus.push_back(min_cpu);
					}
					break;
				}
				for (int cpu = min_cpu; cpu <= max_cpu; ++cpu) {
					if (cpu >= MAX_CPUS) {
						LOG_L(L_WARNING, "CPU index %d exceeds bitset limit.", cpu);
						continue;
					}
					cpus.push_back(cpu);
				}
				// Consume the trailing comma
				ss >> sep;
			} else {
				// Single CPU
				if (min_cpu >= MAX_CPUS) {
					LOG_L(L_WARNING, "CPU index %d exceeds bitset limit.", min_cpu);
					continue;
				}
				cpus.push_back(min_cpu);
			}
		}
	} else {
		// Fallback in case of permission issues reading from sysfs
		int num_cpus = sysconf(_SC_NPROCESSORS_CONF);
		for (int cpu = 0; cpu < num_cpus; ++cpu) {
			if (cpu >= MAX_CPUS) {
				LOG_L(L_WARNING, "CPU index %d exceeds bitset limit.", cpu);
				continue;
			}
			cpus.push_back(cpu);
		}
	}
	return cpus;
}

// Set CPU affinity to a specific core
void set_cpu_affinity(uint32_t cpu) {
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);

	pthread_t thread = pthread_self();
	if (pthread_setaffinity_np(thread, sizeof(mask), &mask) != 0) {
		perror("pthread_setaffinity_np");
	}
}

// Detect Intel core type using CPUID 0x1A
CoreType get_intel_core_type(int cpu) {
	set_cpu_affinity(cpu);
	unsigned int eax, ebx, ecx, edx;
	if (__get_cpuid(0x1A, &eax, &ebx, &ecx, &edx)) {
		uint8_t coreType = ( eax & 0xFF000000 ) >> 24;  // Extract core type

		if (coreType & 0x40) return CORE_PERFORMANCE;
		if (coreType & 0x20) return CORE_EFFICIENCY;
	}
	return CORE_UNKNOWN;
}

// Get thread siblings for a CPU
std::vector<int> get_thread_siblings(int cpu) {
	std::ifstream file("/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/topology/thread_siblings_list");
	std::vector<int> siblings;
	if (file) {
		std::string line;
		std::getline(file, line);
		std::istringstream ss(line);
		int sibling;
		char sep;
		while (ss >> sibling) {
			siblings.push_back(sibling);
			ss >> sep;  // Skip separator (comma or other)
		}
	}
	return siblings;
}

void collect_smt_affinity_masks(int cpu,
								std::bitset<MAX_CPUS> &low_smt_mask,
								std::bitset<MAX_CPUS> &high_smt_mask) {
	std::vector<int> siblings = get_thread_siblings(cpu);
	bool smt_enabled = siblings.size() > 1;
	if (smt_enabled) {
		if (cpu == *std::min_element(siblings.begin(), siblings.end())) {
			low_smt_mask.set(cpu);
		} else {
			high_smt_mask.set(cpu);
		}
	}
}

// Collect CPU affinity masks for Intel
void collect_intel_affinity_masks(std::bitset<MAX_CPUS> &eff_mask,
								  std::bitset<MAX_CPUS> &perf_mask,
								  std::bitset<MAX_CPUS> &low_ht_mask,
								  std::bitset<MAX_CPUS> &high_ht_mask) {
	const auto cpus = get_online_cpus();

	for (const auto cpu : cpus) {
		CoreType core_type = get_intel_core_type(cpu);
		// default to performance core.
		if (core_type == CORE_UNKNOWN) core_type = CORE_PERFORMANCE;

		if (core_type == CORE_EFFICIENCY) eff_mask.set(cpu);   // Efficiency Core (E-core)
		else if (core_type == CORE_PERFORMANCE) perf_mask.set(cpu);  // Performance Core (P-core)

		collect_smt_affinity_masks(cpu, low_ht_mask, high_ht_mask);
	}
}

// Collect CPU affinity masks for AMD
void collect_amd_affinity_masks(std::bitset<MAX_CPUS> &eff_mask,
								std::bitset<MAX_CPUS> &perf_mask,
								std::bitset<MAX_CPUS> &low_smt_mask,
								std::bitset<MAX_CPUS> &high_smt_mask) {
	const auto cpus = get_online_cpus();

	for (const auto cpu : cpus) {
		perf_mask.set(cpu);

		collect_smt_affinity_masks(cpu, low_smt_mask, high_smt_mask);
	}
}

ProcessorMasks GetProcessorMasks() {
	ThreadAffinityGuard guard;
	ProcessorMasks processorMasks;

	std::bitset<MAX_CPUS> eff_mask, perf_mask, low_ht_mask, high_ht_mask;
	Vendor cpu_vendor = detect_cpu_vendor();

	if (cpu_vendor == VENDOR_INTEL) {
		LOG("Detected Intel CPU.");
		collect_intel_affinity_masks(eff_mask, perf_mask, low_ht_mask, high_ht_mask);
	} else if (cpu_vendor == VENDOR_AMD) {
		LOG("Detected AMD CPU.");
		collect_amd_affinity_masks(eff_mask, perf_mask, low_ht_mask, high_ht_mask);
	} else {
		LOG_L(L_WARNING, "Unknown or unsupported CPU vendor.");
	}

	processorMasks.efficiencyCoreMask = eff_mask.to_ulong();
	processorMasks.performanceCoreMask = perf_mask.to_ulong();
	processorMasks.hyperThreadLowMask = low_ht_mask.to_ulong();
	processorMasks.hyperThreadHighMask = high_ht_mask.to_ulong();

	return processorMasks;
}

uint32_t get_thread_cache(int cpu) {
	std::ifstream file("/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cache/index3/size");
	uint32_t sizeInBytes = 0;
	if (file) {
		std::string line;
		std::getline(file, line);
		std::istringstream ss(line);
		ss >> sizeInBytes;
	}
	return sizeInBytes;
}

ProcessorGroupCaches& get_group_cache(ProcessorCaches& processorCaches, uint32_t cacheSize) {
	auto foundCache = std::ranges::find_if
		( processorCaches.groupCaches
		, [cacheSize](const auto& gc) -> bool { return (gc.cacheSizes[2] == cacheSize); });

	if (foundCache == processorCaches.groupCaches.end()) {
		processorCaches.groupCaches.push_back({});
		auto& newCacheGroup = processorCaches.groupCaches[processorCaches.groupCaches.size()-1];
		newCacheGroup.cacheSizes[2] = cacheSize;
		return newCacheGroup;
	}

	return (*foundCache);
}

// Notes.
// Here we are grouping by the cache size, which isn't the same a groups and their cache sizes.
// This is fine what our needs at the moment. We're currently only looking a performance core
// with the most cache for the main thread.
// We are also only looking at L3 caches at the moment.
ProcessorCaches GetProcessorCache() {
	ProcessorCaches processorCaches;
	const auto cpus = get_online_cpus();

	for (const auto cpu : cpus) {
		uint32_t cacheSize = get_thread_cache(cpu);
		ProcessorGroupCaches& groupCache = get_group_cache(processorCaches, cacheSize);

		groupCache.groupMask |= (0x1 << cpu);
	}

	return processorCaches;
}

ThreadPinPolicy GetThreadPinPolicy() {
	return THREAD_PIN_POLICY_ANY_PERF_CORE;
}

} //namespace cpu_topology
