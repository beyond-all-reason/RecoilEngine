/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * macOS Thread Support implementation
 * Provides thread suspend/resume and thread startup functionality using POSIX APIs
 * that are available on macOS (without Linux-specific syscalls or /proc filesystem)
 */

#include <cassert>
#include <functional>
#include <pthread.h>
#include <unistd.h>
#include <csignal>
#include <mach/mach.h>
#include <mach/thread_act.h>

#include "System/Log/ILog.h"
#include "System/Platform/Threading.h"

#define LOG_SECTION_CRASHHANDLER "CrashHandler"

#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_CRASHHANDLER


namespace Threading {

// macOS thread state tracking (simplified compared to Linux)
enum MacThreadState {
	MTS_RUNNING,
	MTS_SLEEP,
	MTS_UNKNOWN
};

/**
 * Get a unique thread ID for the current thread on macOS
 * Uses pthread_self() cast to uintptr_t for a unique identifier
 */
static uintptr_t gettid() {
	pthread_t self = pthread_self();
	return reinterpret_cast<uintptr_t>(self);
}


static void ThreadSIGUSR1Handler(int signum, siginfo_t* info, void* pCtx)
{
	LOG_L(L_DEBUG, "[%s][1]", __func__);

	// Fill in ucontext_t structure before locking, this allows stack walking...
	const int err = getcontext(&(localThreadControls->ucontext));

	if (err != 0) {
		LOG_L(L_ERROR, "[%s] couldn't get thread context within suspend signal handler: %s", __func__, strerror(err));
		return;
	}

	// Change the "running" flag to false.
	localThreadControls->running.store(false);

	LOG_L(L_DEBUG, "[%s][2]", __func__);

	// Wait on the mutex. This should block the thread.
	{
		localThreadControls->mutSuspend.lock();
		localThreadControls->running.store(true);
		localThreadControls->mutSuspend.unlock();
	}

	LOG_L(L_DEBUG, "[%s][3]", __func__);
}


static bool SetThreadSignalHandler()
{
	// Installing new ThreadControls object, so install signal handler also
	int err = 0;
	sigset_t sigSet;
	sigemptyset(&sigSet);
	sigaddset(&sigSet, SIGUSR1);

	err = pthread_sigmask(SIG_UNBLOCK, &sigSet, nullptr);

	if (err != 0) {
		LOG_L(L_FATAL, "[%s] error while setting new pthread's signal mask: %s", __func__, strerror(err));
		return false;
	}

	struct sigaction sa;
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_sigaction = ThreadSIGUSR1Handler;
	sa.sa_flags |= SA_SIGINFO;

	if (sigaction(SIGUSR1, &sa, nullptr)) {
		LOG_L(L_FATAL, "[%s] error while installing pthread SIGUSR1 handler", __func__);
		return false;
	}

	return true;
}


void SetupCurrentThreadControls(std::shared_ptr<ThreadControls>& threadCtls)
{
	assert(!Threading::IsWatchDogThread());

	#ifndef _WIN32
	if (threadCtls.get() != nullptr) {
		LOG_L(L_WARNING, "[%s] thread already has ThreadControls installed", __func__);
	} else {
		// new ThreadControls object, so install SIGUSR1 signal handler also
		if (!SetThreadSignalHandler())
			return;
	}

	{
		threadCtls.reset(new Threading::ThreadControls());

		threadCtls->handle = GetCurrentThread();
		threadCtls->thread_id = gettid();
		threadCtls->running.store(true);
	}
	#endif
}


/**
 * @brief ThreadStart Entry point for wrapped pthread. Allows us to register signal handlers specific to that thread, enabling suspend/resume functionality.
 */
void ThreadStart(
	std::function<void()> taskFunc,
	std::shared_ptr<ThreadControls>* threadCtls,
	ThreadControls* tempCtls
) {
	// install the SIGUSR1 handler
	SetupCurrentThreadControls(localThreadControls);

	if (threadCtls != nullptr)
		*threadCtls = localThreadControls;

	{
		// Lock the thread object so that users can't suspend/resume yet.
		tempCtls->mutSuspend.lock();

		LOG_L(L_DEBUG, "[%s] new thread handle %.4lx", __func__, localThreadControls->handle);

		// Notify the condition variable.
		tempCtls->condInitialized.notify_all();

		// Unlock the suspend mutex.
		tempCtls->mutSuspend.unlock();
	}

	// Run the task function...
	taskFunc();

	// Finish up: change the thread's running state to false.
	localThreadControls->mutSuspend.lock();
	localThreadControls->running = false;
	localThreadControls->mutSuspend.unlock();
}



SuspendResult ThreadControls::Suspend()
{
	// Return an error if the running flag is false.
	if (!running) {
		LOG_L(L_ERROR, "[ThreadControls::%s] cannot suspend if a thread's running flag is set to false, refusing to use pthread_kill", __func__);
		return Threading::THREADERR_NOT_RUNNING;
	}

	mutSuspend.lock();

	LOG_L(L_DEBUG, "[ThreadControls::%s] sending SIGUSR1 to thread", __func__);

	// Send signal to thread to trigger its handler
	const int err = pthread_kill(handle, SIGUSR1);

	if (err != 0) {
		LOG_L(L_ERROR, "[ThreadControls::%s] error while trying to send signal to suspend thread: %s", __func__, strerror(err));
		return Threading::THREADERR_MISC;
	}

	// macOS doesn't have /proc to query thread state, so we use a simple delay
	// to allow the signal handler to run
	constexpr int max_attempts = 40;
	for (int a = 0; a < max_attempts; a++) {
		// Check if the thread is no longer running (signal handler set it to false)
		if (!running)
			break;
		// usleep is not available on all POSIX, use poll instead
		struct timespec ts = {0, 25000000L}; // 25ms
		nanosleep(&ts, nullptr);
	}

	return Threading::THREADERR_NONE;
}

SuspendResult ThreadControls::Resume()
{
	mutSuspend.unlock();

	return Threading::THREADERR_NONE;
}

} // namespace Threading
