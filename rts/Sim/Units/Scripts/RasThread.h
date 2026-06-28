/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef RAS_THREAD_H
#define RAS_THREAD_H

#include <string>
#include <array>
#include <vector>

#include "RasInstance.h"
#include "Lua/LuaRules.h"

// I.0 baseline harness: profiling-only opcode counter, see RasThread.cpp.
// Zero-cost unless compiled with -DRAS_PROFILE_OPCODES.
#ifdef RAS_PROFILE_OPCODES
	#include <atomic>
	#include <cstdint>
	extern std::atomic<uint64_t> gRasOpcodeCount;
#endif

class CRasDeferredCallin;
class CRasFile;
class CRasInstance;

class CRasStackGuard
{
public:
	CRasStackGuard(std::vector<int>* dataStack, int  nArgs) : data(dataStack), nArgs(nArgs)
	{ }

	~CRasStackGuard() {
		const int size = GetSize();
		if (nArgs >= size) {
			data->clear();
		} else {
			data->resize(size - nArgs);
		}
	}

	int GetSize() {
		return static_cast<int>(data->size());
	}

	std::vector<int>* data;
	int nArgs;
};

class CRasThread
{
	CR_DECLARE_STRUCT(CRasThread)
	CR_DECLARE_SUB(CallInfo)

public:
	// default and copy-ctor are creg only
	CRasThread() {}

	CRasThread(CRasInstance* _rasInst);
	CRasThread(CRasThread&& t) { *this = std::move(t); }
	CRasThread(const CRasThread& t) { *this = t; }

	~CRasThread();

	CRasThread& operator = (CRasThread&& t);
	CRasThread& operator = (const CRasThread& t);

	enum State {Init, Sleep, Run, Dead, WaitTurn, WaitMove, WaitScale};

	/**
	 * Returns false if this thread is dead and needs to be killed.
	 */
	bool Tick();
	/**
	 * This function sets the thread in motion. Should only be called once.
	 * If schedule is false the thread is not added to the scheduler, and thus
	 * it is expected that the starter is responsible for ticking it.
	 */
	void Start(int functionId, int sigMask, const std::array<int, 1 + MAX_COB_ARGS>& args, bool schedule);
	void Stop();

	void SetID(int threadID) { id = threadID; }
	void SetState(State s) { state = s; }

	/**
	 * Sets a callback that will be called when the thread dies.
	 * There can be only one.
	 */
	void SetCallback(CRasInstance::ThreadCallbackType cb, int cbp) {
		cbType = cb;
		cbParam = cbp;
	}
	void MakeGarbage() {
		rasInst = nullptr;
		rasFile = nullptr;
	}


	/**
	 * @brief Checks whether the stack has at least size items.
	 * @returns min(size, stack.size())
	 */
	int CheckStack(unsigned int size, bool warn);
	void InitStack(unsigned int n, CRasThread* t);

	/**
	 * Shows an errormessage which includes the current state of the script
	 * interpreter.
	 */
	void ShowError(const char* msg);
	void AnimFinished(CUnitScript::AnimType type, int piece, int axis);

	const std::string& GetName();

	int GetID() const { return id; }
	int GetStackVal(int pos) const { return dataStack[pos]; }
	int GetWakeTime() const { return wakeTime; }
	int GetRetCode() const { return retCode; }
	int GetSignalMask() const { return signalMask; }
	State GetState() const { return state; }

	bool Reschedule(CUnitScript::AnimType type) const {
		return ((state == WaitMove && type == CRasInstance::AMove) || (state == WaitTurn && type == CRasInstance::ATurn) || (state == WaitScale && type == CRasInstance::AScale));
	}

	bool IsDead() const { return (state == Dead); }
	bool IsGarbage() const { return (rasInst == nullptr); }
	bool IsWaiting() const { return (waitAxis != -1); }

	/// Part VII: Return the function ID at the bottom of the call stack (entry-point function).
	int GetRootFunctionId() const {
		return callStack.empty() ? -1 : callStack[0].functionId;
	}

	/// Part VI: Take ownership of this thread's deferred callins buffer (clears internal copy).
	std::vector<CRasDeferredCallin> TakeDeferredCallins() {
		return std::move(threadDeferredCallins);
	}

	// script instance that owns this thread
	CRasInstance* rasInst = nullptr;
	CRasFile* rasFile = nullptr;

protected:
	struct CallInfo {
		CR_DECLARE_STRUCT(CallInfo)
		int functionId = -1;
		int returnAddr = -1;
		int stackTop = -1;
	};

	void LuaCall();
	void DeferredCall(bool synced);

	void PushCallStack(CallInfo v) { callStack.push_back(v); }
	void PushDataStack(int v) { dataStack.push_back(v); }
	CallInfo& PushCallStackRef() { return callStack.emplace_back(); }

	int LocalFunctionID() const { return callStack.back().functionId; }
	int LocalReturnAddr() const { return callStack.back().returnAddr; }
	int LocalStackFrame() const { return callStack.back().stackTop; }

	int PopDataStack() {
		if (dataStack.empty()) {
			return 0;
		}
		int ret = dataStack.back();
		dataStack.pop_back();
		return ret;
	}

protected:
	int id = -1;
	int pc = 0;

	int wakeTime = 0;
	int paramCount = 0;
	int retCode = -1;
	int cbParam = 0;
	int signalMask = 0;

	int waitAxis = -1;
	int waitPiece = -1;

	int errorCounter = 100;

	int luaArgs[MAX_LUA_COB_ARGS] = {0};


	std::vector<CallInfo> callStack;
	std::vector<int> dataStack;
	// std::vector<int> execTrace;

	State state = Init;

	CRasInstance::ThreadCallbackType cbType = CRasInstance::CBNone;

	// Hold references to the stacks from destructed threads working as a
	// memory pool to speed up thread creation.
	static std::vector<decltype(dataStack)> freeDataStacks;
	static std::vector<decltype(callStack)> freeCallStacks;

	/// Part VI: Collected deferred callins for this thread (merged into engine after Tick).
	std::vector<CRasDeferredCallin> threadDeferredCallins;
};

#endif // RAS_THREAD_H
