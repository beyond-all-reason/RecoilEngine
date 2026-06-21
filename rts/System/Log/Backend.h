/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LOG_BACKEND_H
#define LOG_BACKEND_H

/**
 * This is the universal, global backend for the ILog.h logging API.
 * It may format log records, and routes them to all the registered sinks.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name logging_backend_control
 * ILog.h backend control interface.
 */
///@{

typedef void (*log_sink_ptr)(int level, const char* section, const char* record);

/// Start routing log records to the supplied sink
void log_backend_registerSink(log_sink_ptr sink);

/// Stop routing log records to the supplied sink
void log_backend_unregisterSink(log_sink_ptr sink);


typedef void (*log_cleanup_ptr)();

/**
 * Registers a cleanup function, which will be called in certain exceptional
 * situations only, for example during the graceful handling of a crash.
 */
void log_backend_registerCleanup(log_cleanup_ptr cleanupFunc);

/**
 * Unregisters a cleanup function.
 */
void log_backend_unregisterCleanup(log_cleanup_ptr cleanupFunc);

///@}

#ifdef __cplusplus
} // extern "C"
#endif


#ifdef __cplusplus
#include <mutex>

/**
 * The single mutex serializing the whole logging subsystem (filter config, sink
 * registry, record dispatch, every sink). One shared lock rather than one per
 * component, because the record path runs filter -> backend -> file sink while a
 * file-sink rotate/truncate logs and re-enters from file sink, so separate locks
 * would invert and deadlock.
 *
 * Recursive: many log functions log on their own error paths, re-entering on the
 *            same thread. Recursive allows the same thread to grab the lock again
 *            without deadlocking.
 * 
 * Timed: lets the crash/hang handler acquire it best-effort
 *        (see log_lockForStacktrace).
 * 
 * Intentionally never destroyed: intended to be around until process exit
 */
std::recursive_timed_mutex& log_getMutex();

/**
 * Whether the calling thread is currently bypassing the log mutex. Set only by
 * the crash/hang stacktrace handler (see log_lockForStacktrace) when it could
 * not acquire the mutex; while set, this thread's LogMutexGuard does not lock,
 * so the handler can never deadlock against a thread stuck holding the mutex.
 */
bool log_getMutexBypass();

/**
 * Scoped guard for the logging subsystem: locks log_getMutex() for its lifetime
 * -- unless the calling thread is in stacktrace-bypass mode, in which case it
 * does nothing (see log_getMutexBypass / log_lockForStacktrace).
 */
class LogMutexGuard {
public:
	explicit LogMutexGuard(std::recursive_timed_mutex& mtx): mutex(mtx) {
		if (log_getMutexBypass())
			return;
		mutex.lock();
		locked = true;
	}
	~LogMutexGuard() {
		if (locked)
			mutex.unlock();
	}
	LogMutexGuard(const LogMutexGuard&) = delete;
	LogMutexGuard& operator=(const LogMutexGuard&) = delete;
private:
	std::recursive_timed_mutex& mutex;
	bool locked = false;
};

/**
 * Helpers to surround a crash/hang stacktrace dump. Calls must be balanced!
 *
 * Why a dump must never just block on the log mutex: it suspends other threads
 * to walk their stacks, so a suspended thread that holds the mutex can never
 * release it. Waiting on it would hang the handler forever.
 * 
 * To avoid that, we acquires the lock best-effort with a timeout
 * - on success it is held until unlock 
 * - on timeout -- a thread is stuck holding it, so we enter bypass mode and
 *   skip the mutex instead of deadlocking
 */
void log_lockForStacktrace();
void log_unlockForStacktrace();
#endif

#endif // LOG_BACKEND_H

