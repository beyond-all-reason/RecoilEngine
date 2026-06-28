/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#pragma once

/*
 * Simple VM responsible for "scheduling" and running COB threads.
 * It also manages reading and caching of the actual .cob files.
 */

#include <vector>

#include "RasThread.h"
#include "RasDeferredCallin.h"
#include "System/creg/creg_cond.h"
#include "System/creg/STL_Queue.h"
#include "System/creg/STL_Map.h"
#include "System/Cpp11Compat.hpp"
#include <mutex>

class CRasThread;
class CRasInstance;
class CRasFile;
class CRasFileHandler;

class CRasEngine
{
	CR_DECLARE_STRUCT(CRasEngine)

public:
	struct SleepingThread {
		CR_DECLARE_STRUCT(SleepingThread)

		int id;
		int wt;
	};

	struct CRasThreadComp {
	public:
		bool operator() (const SleepingThread& a, const SleepingThread& b) const {
			return a.wt > b.wt || (a.wt == b.wt && a.id > b.id);
		}
	};

public:
	void Init() {
		threadInstances.reserve(2048);
		tickAddedThreads.reserve(128);

		runningThreadIDs.reserve(512);
		waitingThreadIDs.reserve(512);

		sleepingThreadIDs = {};

		curThread = nullptr;

		currentTime = 0;
		threadCounter = 0;
	}
	void Kill() {
		// threadInstances is never explicitly iterated in the actual code,
		// but iterated during sync dumps, so clean it with clear_unordered_map
		spring::clear_unordered_map(threadInstances);
		spring::clear_unordered_map(deferredCallins);
		tickAddedThreads.clear();

		runningThreadIDs.clear();
		waitingThreadIDs.clear();

		while (!sleepingThreadIDs.empty()) {
			sleepingThreadIDs.pop();
		}
	}

	void Tick(int deltaTime);
	void ShowScriptError(const std::string& msg);


	CRasThread* GetThread(int threadID) {
		const auto it = threadInstances.find(threadID);

		if (it == threadInstances.end())
			return nullptr;

		return &(it->second);
	}

	bool RemoveThread(int threadID);
	int AddThread(CRasThread&& thread);
	int GenThreadID() { return (threadCounter++); }

	void QueueAddThread(CRasThread&& thread) { tickAddedThreads.emplace_back(std::move(thread)); }
	void QueueRemoveThread(int threadID) { tickRemovedThreads.emplace_back(threadID); }
	void ProcessQueuedThreads();

	void ScheduleThread(const CRasThread* thread);
	void SanityCheckThreads(const CRasInstance* owner);

	const auto& GetThreadInstances() const { return threadInstances; }
//	const auto& GetTickAddedThreads() const { return tickAddedThreads; }
//	const auto& GetTickRemovedThreads() const { return tickRemovedThreads; }
//	const auto& GetRunningThreadIDs() const { return runningThreadIDs; }
	const auto& GetWaitingThreadIDs() const { return waitingThreadIDs; }
	const auto& GetSleepingThreadIDs() const { return sleepingThreadIDs; }
	const auto  GetCurrTime() const { return currentTime; }
	const auto  GetThreadCounter() const { return threadCounter; }
	const auto  GetCurrCounter() const { return threadCounter; }

	void AddDeferredCallin(CRasDeferredCallin&& deferredCallin);
	void RunDeferredCallins();

	/// Part VI: Merge per-thread deferred callins into the shared map.
	void mergeThreadDeferredCallins(const std::vector<CRasDeferredCallin>& callins);


private:
	void TickThread(CRasThread* thread);

	void WakeSleepingThreads();
	void TickRunningThreads();

private:
	// registry of every thread across all script instances
	spring::unordered_map<int, CRasThread> threadInstances;
	// threads that are spawned during Tick
	std::vector<CRasThread> tickAddedThreads;
	// threads that are killed during Tick
	std::vector<int> tickRemovedThreads;

	std::vector<int> runningThreadIDs;
	std::vector<int> waitingThreadIDs;

	spring::unordered_map<int, std::vector<CRasDeferredCallin> > deferredCallins;

	// stores <id, waketime> pairs s.t. after waking up the ID can be checked
	// for validity; thread owner might get removed while a thread is sleeping
	std::priority_queue<SleepingThread, std::vector<SleepingThread>, CRasThreadComp> sleepingThreadIDs;

	CRasThread* curThread = nullptr;

	int currentTime = 0;
	int threadCounter = 0;


#ifdef THREADPOOL
	std::mutex scheduleMutex;
#endif
};


extern CRasEngine* rasEngine;

