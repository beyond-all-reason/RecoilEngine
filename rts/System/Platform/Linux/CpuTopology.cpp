#include "System/Platform/CpuTopology.h"

#include "System/Log/ILog.h"
#include "System/Platform/ThreadAffinityGuard.h"

#include <algorithm>
#include <bitset>
#if !defined(__aarch64__) && !defined(__arm__)
#include <cpuid.h>
#endif
#include <pthread.h>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <sched.h>
#include <filesystem>

namespace cpu_topology {

#define MAX_CPUS 32  // Maximum logical CPUs
	
enum Vendor { VENDOR_INTEL, VENDOR_AMD, VENDOR_ARM, VENDOR_UNKNOWN };

enum CoreType { CORE_PERFORMANCE, CORE_EFFICIENCY, CORE_UNKNOWN };

// Get number of logical CPUs
int get_cpu_count() {
	return sysconf(_SC_NPROCESSORS_CONF);
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

// Get thread siblings for a CPU (works for all Linux architectures)
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

#if defined(__aarch64__) || defined(__arm__)

// Read a numeric value from a sysfs file
template<typename T>
T read_sysfs_value(const std::string& path, T default_value) {
	std::ifstream file(path);
	if (file) {
		T value;
		file >> value;
		return value;
	}
	return default_value;
}

// Detect ARM core type using cpu_capacity
// On ARM big.LITTLE systems, bigger capacity indicates performance cores
CoreType get_arm_core_type(int cpu) {
	std::string capacity_path = "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cpu_capacity";
	
	// Not all ARM systems have cpu_capacity (older kernels or non-big.LITTLE)
	if (!std::filesystem::exists(capacity_path)) {
		return CORE_PERFORMANCE; // Default to performance if capacity not available
	}
	
	uint32_t capacity = read_sysfs_value<uint32_t>(capacity_path, 0);
	
	// Find the maximum capacity across all CPUs to determine what's a performance core
	static uint32_t max_capacity = 0;
	static bool capacity_initialized = false;
	
	if (!capacity_initialized) {
		int num_cpus = get_cpu_count();
		for (int i = 0; i < num_cpus; ++i) {
			std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpu_capacity";
			uint32_t cap = read_sysfs_value<uint32_t>(path, 0);
			if (cap > max_capacity) {
				max_capacity = cap;
			}
		}
		capacity_initialized = true;
	}
	
	// If capacity is less than 75% of max, consider it an efficiency core
	// This threshold works for most big.LITTLE systems
	if (capacity > 0 && max_capacity > 0) {
		if (capacity < (max_capacity * 3 / 4)) {
			return CORE_EFFICIENCY;
		} else {
			return CORE_PERFORMANCE;
		}
	}
	
	return CORE_PERFORMANCE; // Default to performance
}

// Collect CPU affinity masks for ARM
void collect_arm_affinity_masks(std::bitset<MAX_CPUS> &eff_mask,
								std::bitset<MAX_CPUS> &perf_mask,
								std::bitset<MAX_CPUS> &low_smt_mask,
								std::bitset<MAX_CPUS> &high_smt_mask) {
	int num_cpus = get_cpu_count();
	
	for (int cpu = 0; cpu < num_cpus; ++cpu) {
		if (cpu >= MAX_CPUS) {
			LOG_L(L_WARNING, "CPU index %d exceeds bitset limit.", cpu);
			continue;
		}
		
		CoreType core_type = get_arm_core_type(cpu);
		
		if (core_type == CORE_EFFICIENCY) {
			eff_mask.set(cpu);   // Efficiency Core (LITTLE core)
		} else {
			perf_mask.set(cpu);  // Performance Core (big core)
		}
		
		collect_smt_affinity_masks(cpu, low_smt_mask, high_smt_mask);
	}
}

uint32_t get_thread_cache(int cpu) {
	uint32_t sizeInBytes = 0;
	
	// First try L3 cache (index3)
	std::string l3_path = "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cache/index3/size";
	if (std::filesystem::exists(l3_path)) {
		std::ifstream file(l3_path);
		if (file) {
			std::string line;
			std::getline(file, line);
			// Parse size - format is like "4096K" or "12288K"
			if (!line.empty()) {
				std::istringstream ss(line);
				uint32_t size;
				char unit;
				ss >> size >> unit;
				if (unit == 'K' || unit == 'k') {
					sizeInBytes = size * 1024;
				} else if (unit == 'M' || unit == 'm') {
					sizeInBytes = size * 1024 * 1024;
				} else {
					sizeInBytes = size;
				}
			}
		}
	}
	
	// If no L3, fall back to L2 cache (index2) on ARM
	if (sizeInBytes == 0) {
		std::string l2_path = "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/cache/index2/size";
		if (std::filesystem::exists(l2_path)) {
			std::ifstream file(l2_path);
			if (file) {
				std::string line;
				std::getline(file, line);
				// Parse size - format is like "4096K" or "12288K"
				if (!line.empty()) {
					std::istringstream ss(line);
					uint32_t size;
					char unit;
					ss >> size >> unit;
					if (unit == 'K' || unit == 'k') {
						sizeInBytes = size * 1024;
					} else if (unit == 'M' || unit == 'm') {
						sizeInBytes = size * 1024 * 1024;
					} else {
						sizeInBytes = size;
					}
				}
			}
		}
	}
	
	return sizeInBytes;
}

#else  // x86

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

// Collect CPU affinity masks for Intel
void collect_intel_affinity_masks(std::bitset<MAX_CPUS> &eff_mask,
								  std::bitset<MAX_CPUS> &perf_mask,
								  std::bitset<MAX_CPUS> &low_ht_mask,
								  std::bitset<MAX_CPUS> &high_ht_mask) {
	int num_cpus = get_cpu_count();

	for (int cpu = 0; cpu < num_cpus; ++cpu) {
		if (cpu >= MAX_CPUS) {
			LOG_L(L_WARNING, "CPU index %d exceeds bitset limit.", cpu);
			continue;
		}

		CoreType core_type = get_intel_core_type(cpu);
		// default to performance core.
		if (core_type == CORE_UNKNOWN) core_type = CORE_PERFORMANCE;

		if (core_type == CORE_EFFICIENCY) eff_mask.set(cpu);   // Efficiency Core (E-core)
		else if (core_type == CORE_PERFORMANCE) perf_mask.set(cpu);  // Performance Core (P-core)

		collect_smt_affinity_masks(cpu, low_ht_mask, high_ht_mask);
	}
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

#endif // Architecture-specific implementations

// Collect CPU affinity masks for AMD (same for all architectures)
void collect_amd_affinity_masks(std::bitset<MAX_CPUS> &eff_mask,
								std::bitset<MAX_CPUS> &perf_mask,
								std::bitset<MAX_CPUS> &low_smt_mask,
								std::bitset<MAX_CPUS> &high_smt_mask) {
	int num_cpus = get_cpu_count();

	for (int cpu = 0; cpu < num_cpus; ++cpu) {
		if (cpu >= MAX_CPUS) {
			LOG_L(L_WARNING, "CPU index %d exceeds bitset limit.", cpu);
			continue;
		}

		perf_mask.set(cpu);

		collect_smt_affinity_masks(cpu, low_smt_mask, high_smt_mask);
	}
}

// Detect CPU vendor
Vendor detect_cpu_vendor() {
#if defined(__aarch64__) || defined(__arm__)
	return VENDOR_ARM;
#else
	unsigned int eax, ebx, ecx, edx;
	__get_cpuid(0, &eax, &ebx, &ecx, &edx);
	if (ebx == 0x756E6547) return VENDOR_INTEL; // "GenuineIntel"
	if (ebx == 0x68747541) return VENDOR_AMD;   // "AuthenticAMD"
	return VENDOR_UNKNOWN;
#endif
}

ProcessorMasks GetProcessorMasks() {
	ThreadAffinityGuard guard;
	ProcessorMasks processorMasks;

	std::bitset<MAX_CPUS> eff_mask, perf_mask, low_ht_mask, high_ht_mask;
	Vendor cpu_vendor = detect_cpu_vendor();

	if (cpu_vendor == VENDOR_INTEL) {
		LOG("Detected Intel CPU.");
#if !defined(__aarch64__) && !defined(__arm__)
		collect_intel_affinity_masks(eff_mask, perf_mask, low_ht_mask, high_ht_mask);
#endif
	} else if (cpu_vendor == VENDOR_AMD) {
		LOG("Detected AMD CPU.");
		collect_amd_affinity_masks(eff_mask, perf_mask, low_ht_mask, high_ht_mask);
	} else if (cpu_vendor == VENDOR_ARM) {
		LOG("Detected ARM CPU.");
#if defined(__aarch64__) || defined(__arm__)
		collect_arm_affinity_masks(eff_mask, perf_mask, low_ht_mask, high_ht_mask);
#endif
	} else {
		LOG_L(L_WARNING, "Unknown or unsupported CPU vendor.");
	}

	processorMasks.efficiencyCoreMask = eff_mask.to_ulong();
	processorMasks.performanceCoreMask = perf_mask.to_ulong();
	processorMasks.hyperThreadLowMask = low_ht_mask.to_ulong();
	processorMasks.hyperThreadHighMask = high_ht_mask.to_ulong();

	return processorMasks;
}

// Notes.
// Here we are grouping by the cache size, which isn't the same a groups and their cache sizes.
// This is fine what our needs at the moment. We're currently only looking a performance core
// with the most cache for the main thread.
// We are also only looking at L3 caches at the moment.
ProcessorCaches GetProcessorCache() {
	ProcessorCaches processorCaches;
	int num_cpus = get_cpu_count();

	for (int cpu = 0; cpu < num_cpus; ++cpu) {
		if (cpu >= MAX_CPUS) {
			LOG_L(L_WARNING, "CPU index %d exceeds bitset limit.", cpu);
			continue;
		}
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
