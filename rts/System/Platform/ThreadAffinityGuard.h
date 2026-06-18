#ifndef THREAD_AFFINITY_GUARD_H__
#define THREAD_AFFINITY_GUARD_H__

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <pthread.h>
#include <mach/mach.h>
#else
#include <sched.h>
#endif

class ThreadAffinityGuard {
private:
#ifdef _WIN32
	DWORD_PTR savedAffinity;
	HANDLE threadHandle;
#elif defined(__APPLE__)
	// macOS has no portable CPU affinity API; stub out
	bool dummy;
#else
	cpu_set_t savedAffinity;
	pid_t tid;
#endif
	bool affinitySaved;

public:
	ThreadAffinityGuard();
	~ThreadAffinityGuard();
	ThreadAffinityGuard(const ThreadAffinityGuard&) = delete;
	ThreadAffinityGuard& operator=(const ThreadAffinityGuard&) = delete;
};

#endif
