/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "RasEngine.h"

#include "RasDeferredCallin.h"
#include "RasThread.h"
#include "RasFile.h"

#include <cstdint>
#include "System/Misc/TracyDefs.h"
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

	CR_MEMBER(currentTime),
	CR_MEMBER(threadCounter)
))

CR_BIND(CRasEngine::SleepingThread, )
CR_REG_METADATA(CRasEngine::SleepingThread, (
	CR_MEMBER(id),
	CR_MEMBER(wt)
))

static const char* const numCobThreadsPlot = "CobThreads";

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
	switch (thread->GetState()) {
		case CRasThread::Run: {
			waitingThreadIDs.push_back(thread->GetID());
		} break;
		case CRasThread::Sleep: {
			sleepingThreadIDs.push(SleepingThread{thread->GetID(), thread->GetWakeTime()});
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
	// for error messages originating in CUnitScript
	curThread = thread;

	// NB: threadID is still in <runningThreadIDs> here, TickRunningThreads clears it
	if (thread != nullptr && !thread->Tick())
		RemoveThread(thread->GetID());

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
	// advance all currently running threads
	for (const int threadID: runningThreadIDs) {
		TickThread(GetThread(threadID));
	}

	// a thread can never go from running->running, so clear the list
	// note: if preemption was to be added, this would no longer hold
	// however, TA scripts can not run preemptively anyway since there
	// aren't any synchronization methods available
	runningThreadIDs.clear();

	// prepare threads that will run next frame
	std::swap(runningThreadIDs, waitingThreadIDs);
}

void CRasEngine::Tick(int deltaTime)
{
	ZoneScoped;
	currentTime += deltaTime;

	TickRunningThreads();
	ProcessQueuedThreads();

	WakeSleepingThreads();
	ProcessQueuedThreads();
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


void CRasEngine::AddDeferredCallin(CRasDeferredCallin&& deferredCallin)
{
	deferredCallins[deferredCallin.funcHash].push_back(deferredCallin);
}


void CRasEngine::RunDeferredCallins()
{
	std::vector<int> funcHashes;
	funcHashes.reserve(deferredCallins.size());
	for(auto& it: deferredCallins)
		funcHashes.push_back(it.first);

	for(auto funcHash: funcHashes) {
		auto it = deferredCallins.find(funcHash); // 'it' has to necessarily be present at this point

		auto callins = std::move(it->second);
		deferredCallins.erase(it);

		const LuaHashString cmdStr = LuaHashString(callins[0].funcName.c_str());
		luaRules->unsyncedLuaHandle.Cob2LuaBatch(cmdStr, callins);
		if (luaUI)
			luaUI->Cob2LuaBatch(cmdStr, callins);
	}
}
