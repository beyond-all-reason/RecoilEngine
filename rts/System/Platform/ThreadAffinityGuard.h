#ifndef THREAD_AFFINITY_GUARD_H__
#define THREAD_AFFINITY_GUARD_H__

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <sched.h>
#endif
// Note: sched.h is NOT included on macOS - thread affinity is not supported

class ThreadAffinityGuard {
private:
#ifdef _WIN32
	DWORD_PTR savedAffinity;
	HANDLE threadHandle;
#elif defined(__APPLE__)
	// macOS thread affinity is not supported via POSIX API
	int dummy;
#elif defined(__linux__)
	cpu_set_t savedAffinity;
	pid_t tid;
#endif
	bool affinitySaved;

public:
	// Constructor: Saves the current thread's affinity
	ThreadAffinityGuard();

	// Destructor: Restores the saved affinity if it was successfully stored
	~ThreadAffinityGuard();

	// Delete copy constructor to prevent copying
	ThreadAffinityGuard(const ThreadAffinityGuard&) = delete;

	// Delete copy assignment operator to prevent assignment
	ThreadAffinityGuard& operator=(const ThreadAffinityGuard&) = delete;
};

#endif
