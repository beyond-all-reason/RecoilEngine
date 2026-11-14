#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>

#include "System/Threading/SpringThreading.h"
#include "System/Threading/WrappedSync.h"
#include "System/ScopedResource.h"
#include "Rendering/GlobalRendering.h"

/// LoadingMT: the main thread announces each load screen frame it wants to draw, and the
/// loading thread lets it through before taking CLoadLock again (a plain mutex starves the draw).
class CLoadLockHandoff {
public:
	/// longest the loading thread waits for a frame, in case it never comes (e.g. the main thread quitting)
	static constexpr std::chrono::milliseconds MAX_FRAME_WAIT{100};

	/// main thread: about to take the lock to draw a load screen frame
	static void BeginDraw() { drawWaiting.store(true); }

	/// main thread: the frame is presented and the lock released again
	static void EndDraw() {
		if (!drawWaiting.exchange(false))
			return;
		{
			std::lock_guard<std::mutex> lk(frameMtx);
			++framesDrawn;
		}
		frameCv.notify_all();
	}

	/// loading thread, not holding the lock: wait until a frame the main thread is waiting to draw is done
	static void WaitForPendingFrame() {
		if (!drawWaiting.load())
			return;

		std::unique_lock<std::mutex> lk(frameMtx);
		const uint32_t frame = framesDrawn;

		if (!drawWaiting.load())
			return;

		frameCv.wait_for(lk, MAX_FRAME_WAIT, [frame]() { return framesDrawn != frame; });
	}
private:
	inline static std::atomic<bool> drawWaiting = false;
	inline static std::mutex frameMtx;
	inline static std::condition_variable frameCv;
	inline static uint32_t framesDrawn = 0;
};

class CLoadLockMtx {
public:
	using native_handle_type = uint32_t;

	void lock() {
		// the loading thread steps aside for a waiting load screen frame before taking the outermost lock
		if (Threading::IsGameLoadThread() && locksCount[true] == 0)
			CLoadLockHandoff::WaitForPendingFrame();

		mtx.lock();

		// hack to protect from multiple context acquisions in case of recursive lock
		auto& thisThreadLocksCount = locksCount[Threading::IsGameLoadThread()];
		++thisThreadLocksCount;
		if (thisThreadLocksCount == 1) {
			globalRendering->MakeCurrentContext(false); //set
			globalRendering->ToggleMultisampling();
		}
	}
	void unlock() {

		// hack to protect from multiple context clear in case of recursive lock
		auto& thisThreadLocksCount = locksCount[Threading::IsGameLoadThread()];
		--thisThreadLocksCount;

		if (thisThreadLocksCount == 0)
			globalRendering->MakeCurrentContext(true ); //clear

		mtx.unlock();
	}
	bool try_lock() { assert(false); return true; } // placeholder
	native_handle_type native_handle() { return native_handle_type{}; } // placeholder

	/// lock depth of the loading thread, only meaningful when read from that thread
	static uint32_t GetLoadThreadDepth() { return locksCount[true]; }
private:
	inline static std::array<uint32_t, 2> locksCount = { 0 };
	std::recursive_mutex mtx = {};
};

class CLoadLockImpl : public spring::WrappedSync<CLoadLockMtx> {
public:
	CLoadLockImpl() : spring::WrappedSync<CLoadLockMtx>() {
		needThreadSafety = false;
	}
	auto& GetMutex() {
		return *sync[needThreadSafety];
	}
};

class CLoadLock {
public:
	static auto& GetMutex() {
		return loadLockImpl.GetMutex();
	}
	static auto GetUniqueLock() {
		return loadLockImpl.GetUniqueLock();
	}
	static void SetThreadSafety(bool b) { loadLockImpl.SetThreadSafety(b); }
	static bool GetThreadSafety() { return loadLockImpl.GetThreadSafety(); }

	/// main thread: wraps one load screen frame, see CLoadLockHandoff; declare it before the lock so it ends after the unlock
	struct ScopedDraw {
		ScopedDraw() {
			if (GetThreadSafety())
				CLoadLockHandoff::BeginDraw();
		}
		~ScopedDraw() { CLoadLockHandoff::EndDraw(); }

		ScopedDraw(const ScopedDraw&) = delete;
		ScopedDraw& operator=(const ScopedDraw&) = delete;
	};

	/// loading thread: hand the lock and GL context to a waiting load screen frame, outermost lock only
	/// @return false when LoadingMT is off
	static bool YieldToDraw() {
		if (!GetThreadSafety())
			return false;

		if (CLoadLockMtx::GetLoadThreadDepth() != 1)
			return true;

		auto& mtx = GetMutex();

		// lock() waits for the pending frame
		mtx.unlock();
		mtx.lock();
		return true;
	}
private:
	inline static CLoadLockImpl loadLockImpl;
};