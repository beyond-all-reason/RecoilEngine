/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "RasEngine.h"

#include "CobDeferredCallin.h"
#include "RasInstance.h"
#include "RasThread.h"
#include "RasFile.h"

#include <atomic>
#include <algorithm>
#include <cstdint>
#include "System/Misc/TracyDefs.h"
#include "System/UnorderedSet.hpp"
#include "System/Threading/ThreadPool.h"
#include "Lua/LuaUI.h"

CR_BIND(CRasEngine, )

CR_REG_METADATA(CRasEngine, (
	CR_MEMBER(threadInstances),
	CR_MEMBER(tickAddedThreads),
	CR_MEMBER(tickRemovedThreads),
	CR_MEMBER(runningThreadIDs),
	CR_MEMBER(sleepingThreadIDs),
	// always null/empty when saving
	CR_IGNORED(waitingThreadIDs),

	CR_IGNORED(curThread),
	CR_IGNORED(deferredCallins),
#ifdef THREADPOOL
	CR_IGNORED(scheduleMutex),
#endif

	CR_MEMBER(currentTime),
	CR_MEMBER(threadCounter)
))

CR_BIND(CRasEngine::SleepingThread, )
CR_REG_METADATA(CRasEngine::SleepingThread, (
	CR_MEMBER(id),
	CR_MEMBER(wt)
))

static const char* const numCobThreadsPlot = "CobThreads";
static const char* const rasRunningThreadsPlot = "RasRunningThreads";
static const char* const rasWaitingThreadsPlot = "RasWaitingThreads";
static const char* const rasSleepingThreadsPlot = "RasSleepingThreads";
static const char* const rasSafeThreadsPlot = "RasSafeThreads";
static const char* const rasUnsafeThreadsPlot = "RasUnsafeThreads";
static const char* const rasTickAddedPlot = "RasTickAdded";
static const char* const rasTickRemovedPlot = "RasTickRemoved";

int CRasEngine::AddThread(CRasThread&& thread)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (thread.GetID() == -1)
		thread.SetID(GenThreadID());

	CRasInstance* o = thread.rasInst;
	CRasThread& t = threadInstances[thread.GetID()];

	// move thread into registry, hand its ID to owner
	t = std::move(thread);
	o->AddThreadID(t.GetID());

	TracyPlot(numCobThreadsPlot, static_cast<int64_t>(threadInstances.size()));

	return (t.GetID());
}

bool CRasEngine::RemoveThread(int threadID) {
	RECOIL_DETAILED_TRACY_ZONE;
	const auto it = threadInstances.find(threadID);

	if (it != threadInstances.end()) {
		threadInstances.erase(it);
		TracyPlot(numCobThreadsPlot, static_cast<int64_t>(threadInstances.size()));
		return true;
	}

	return false;
}

void CRasEngine::ProcessQueuedThreads() {
	ZoneScoped;

	TracyPlot(rasTickRemovedPlot, static_cast<int64_t>(tickRemovedThreads.size()));
	TracyPlot(rasTickAddedPlot, static_cast<int64_t>(tickAddedThreads.size()));

	// Remove threads killed during Tick by other thread (SIGNAL), we do it
	// here as nothing is actively referencing any thread's memory here.
	for (int threadID: tickRemovedThreads) {
		RemoveThread(threadID);
	}
	tickRemovedThreads.clear();

	// move new threads spawned by START into threadInstances;
	// their ID's will already have been scheduled into either
	// waitingThreadIDs or sleepingThreadIDs
	for (CRasThread& t: tickAddedThreads) {
		AddThread(std::move(t));
	}

	tickAddedThreads.clear();
}

// a thread wants to continue running at a later time, and adds itself to the scheduler
void CRasEngine::ScheduleThread(const CRasThread* thread)
{
	RECOIL_DETAILED_TRACY_ZONE;
#ifdef THREADPOOL
	std::lock_guard<std::mutex> lock(scheduleMutex);
#endif
	switch (thread->GetState()) {
		case CRasThread::Run: {
			waitingThreadIDs.push_back(thread->GetID());
			TracyPlot(rasWaitingThreadsPlot, static_cast<int64_t>(waitingThreadIDs.size()));
		} break;
		case CRasThread::Sleep: {
			sleepingThreadIDs.push(SleepingThread{thread->GetID(), thread->GetWakeTime()});
			TracyPlot(rasSleepingThreadsPlot, static_cast<int64_t>(sleepingThreadIDs.size()));
		} break;
		default: {
			LOG_L(L_ERROR, "[RASEngine::%s] unknown state %d for thread %d", __func__, thread->GetState(), thread->GetID());
		} break;
	}
}

void CRasEngine::SanityCheckThreads(const CRasInstance* owner)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (false) {
		// no threads belonging to owner should be left
		for (const auto& p: threadInstances) {
			assert(p.second.rasInst != owner);
		}
		for (const CRasThread& t: tickAddedThreads) {
			assert(t.rasInst != owner);
		}
	}
}


void CRasEngine::TickThread(CRasThread* thread)
{
	RECOIL_DETAILED_TRACY_ZONE;
	curThread = thread;

	if (thread != nullptr) {
		bool alive = thread->Tick();
		auto callins = thread->TakeDeferredCallins();
		if (!callins.empty()) {
			mergeThreadDeferredCallins(callins);
		}
		if (!alive) {
			RemoveThread(thread->GetID());
		}
	}

	curThread = nullptr;
}

void CRasEngine::WakeSleepingThreads()
{
	ZoneScoped;
	// check on the sleeping threads, remove any whose owner died
	while (!sleepingThreadIDs.empty()) {
		CRasThread* zzzThread = GetThread((sleepingThreadIDs.top()).id);

		if (zzzThread == nullptr) {
			sleepingThreadIDs.pop();
			continue;
		}

		// not yet time to execute this thread or any subsequent sleepers
		if (zzzThread->GetWakeTime() >= currentTime)
			break;

		// remove executing thread from the queue
		sleepingThreadIDs.pop();

		// wake up the thread and tick it (if not dead)
		// this can quite possibly re-add the thread to <sleepingThreadIDs>
		// again, but any thread is guaranteed to sleep for at least 1 tick
		switch (zzzThread->GetState()) {
			case CRasThread::Sleep: {
				zzzThread->SetState(CRasThread::Run);
				TickThread(zzzThread);
			} break;
			case CRasThread::Dead: {
				RemoveThread(zzzThread->GetID());
			} break;
			default: {
				LOG_L(L_ERROR, "[RASEngine::%s] unknown state %d for thread %d", __func__, zzzThread->GetState(), zzzThread->GetID());
			} break;
		}
	}
}

void CRasEngine::TickRunningThreads()
{
	ZoneScoped;

	// Snapshot runningThreadIDs for this tick.  Threads scheduled during
	// this tick are pushed to waitingThreadIDs/sleepingThreadIDs and will
	// be picked up next frame.
	std::vector<int> threadIDs;
	threadIDs.swap(runningThreadIDs);

	TracyPlot(rasRunningThreadsPlot, static_cast<int64_t>(threadIDs.size()));
	TracyPlot(rasWaitingThreadsPlot, static_cast<int64_t>(waitingThreadIDs.size()));
	TracyPlot(rasSleepingThreadsPlot, static_cast<int64_t>(sleepingThreadIDs.size()));

	if (threadIDs.empty()) {
		return;
	}

	// Split into safe (parallel) and unsafe (serial) groups.
	// Per-unit exclusivity: at most one thread per CRasInstance runs on the
	// parallel path, so threads that share a unit's staticVars/RNG never race.
	// Surplus same-unit threads fall back to the serial group, preserving
	// deterministic ordering.
	std::vector<int> safeIDs;
	std::vector<int> unsafeIDs;
	safeIDs.reserve(threadIDs.size());
	spring::unordered_set<const CRasInstance*> parallelInsts;

	for (int tid : threadIDs) {
		CRasThread* th = GetThread(tid);
		bool safe = false;
		if (th && th->rasFile && th->rasInst) {
			const int funcId = th->GetRootFunctionId();
			if (funcId >= 0 &&
			    static_cast<size_t>(funcId) < th->rasFile->threadSafeFuncs.size() &&
			    th->rasFile->threadSafeFuncs[funcId] &&
			    parallelInsts.insert(th->rasInst).second) {
				safe = true;
			}
		}
		if (safe) {
			safeIDs.push_back(tid);
		} else {
			unsafeIDs.push_back(tid);
		}
	}

	TracyPlot(rasSafeThreadsPlot, static_cast<int64_t>(safeIDs.size()));
	TracyPlot(rasUnsafeThreadsPlot, static_cast<int64_t>(unsafeIDs.size()));

	// Tick thread-safe scripts in parallel.
	if (!safeIDs.empty()) {
		std::vector<int> removedBuffer(safeIDs.size(), -1);
		std::atomic<int> removedCount{0};

		for_mt(0, static_cast<int>(safeIDs.size()), [&](int i) {
			CRasThread* thread = GetThread(safeIDs[i]);
			if (thread == nullptr) {
				return;
			}

			thread->SetParallel(true);
			if (!thread->Tick()) {
				int pos = removedCount.fetch_add(1);
				removedBuffer[pos] = safeIDs[i];
			}
			thread->SetParallel(false);
		});

		// Assign IDs + schedule START spawns in deterministic safeIDs order
		// (no GenThreadID/QueueAddThread races inside for_mt) -> sync-stable.
		for (int i = 0; i < static_cast<int>(safeIDs.size()); ++i) {
			CRasThread* th = GetThread(safeIDs[i]);
			if (!th)
				continue;
			auto spawns = th->TakePendingSpawns();
			for (auto& s : spawns) {
				s.SetID(GenThreadID());
				ScheduleThread(&s);
				QueueAddThread(std::move(s));
			}
		}

		// Drain deferred callins from all safe threads (alive and dead).
		for (int i = 0; i < static_cast<int>(safeIDs.size()); ++i) {
			CRasThread* th = GetThread(safeIDs[i]);
			if (th) {
				auto callins = th->TakeDeferredCallins();
				if (!callins.empty()) {
					mergeThreadDeferredCallins(callins);
				}
			}
		}

		// Remove dead threads after the parallel barrier.
		for (int i = 0; i < removedCount.load(); ++i) {
			if (removedBuffer[i] >= 0) {
				RemoveThread(removedBuffer[i]);
			}
		}
	}

	// Tick unsafe scripts serially on the main thread.
	for (int tid : unsafeIDs) {
		TickThread(GetThread(tid));
	}

	// Prepare threads for next frame.
	TracyPlot(rasWaitingThreadsPlot, static_cast<int64_t>(waitingThreadIDs.size()));
	std::swap(runningThreadIDs, waitingThreadIDs);
	TracyPlot(rasRunningThreadsPlot, static_cast<int64_t>(runningThreadIDs.size()));
}

void CRasEngine::Tick(int deltaTime)
{
	ZoneScoped;
	TracyPlot("RasInstances", static_cast<int64_t>(threadInstances.size()));
	currentTime += deltaTime;

	TickRunningThreads();
	ProcessQueuedThreads();

	WakeSleepingThreads();
	ProcessQueuedThreads();

#ifdef RAS_PROFILE_OPCODES
	// I.0 baseline harness: emit opcodes-executed-this-tick to Tracy so a
	// headless capture yields opcodes/sec (divide by the sim tick period).
	// Profiling-only; does not touch synced state.
	static uint64_t prevOpcodeCount = 0;
	const uint64_t curOpcodeCount = gRasOpcodeCount.load(std::memory_order_relaxed);
	TracyPlot("RasOpcodes/tick", static_cast<int64_t>(curOpcodeCount - prevOpcodeCount));
	prevOpcodeCount = curOpcodeCount;
#endif
}


void CRasEngine::ShowScriptError(const std::string& msg)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (curThread != nullptr) {
		curThread->ShowError(msg.c_str());
		return;
	}

	LOG_L(L_ERROR, "[RASEngine::%s] \"%s\" outside script execution", __func__, msg.c_str());
}


void CRasEngine::AddDeferredCallin(CCobDeferredCallin&& deferredCallin)
{
	deferredCallins[deferredCallin.funcHash].push_back(deferredCallin);
}


void CRasEngine::RunDeferredCallins()
{
	std::vector<int> funcHashes;
	funcHashes.reserve(deferredCallins.size());
	for(auto& it: deferredCallins)
		funcHashes.push_back(it.first);

	// Deterministic replay: dispatch buckets in funcHash order, and within
	// each bucket order callins by unitID so unsynced callbacks reproduce
	// identically regardless of MT worker scheduling (Part VI).
	std::sort(funcHashes.begin(), funcHashes.end());

	for(auto funcHash: funcHashes) {
		auto it = deferredCallins.find(funcHash); // 'it' has to necessarily be present at this point

		auto callins = std::move(it->second);
		deferredCallins.erase(it);

		std::stable_sort(callins.begin(), callins.end(),
			[](const CCobDeferredCallin& a, const CCobDeferredCallin& b) {
				return a.unit->id < b.unit->id;
			});

		const LuaHashString cmdStr = LuaHashString(callins[0].funcName.c_str());
		luaRules->unsyncedLuaHandle.Cob2LuaBatch(cmdStr, callins);
		if (luaUI)
			luaUI->Cob2LuaBatch(cmdStr, callins);
	}
}


/// Part VI: Merge per-thread deferred callins into the shared map.
void CRasEngine::mergeThreadDeferredCallins(const std::vector<CCobDeferredCallin>& callins)
{
	for (const auto& c : callins) {
		deferredCallins[c.funcHash].push_back(c);
	}
}
