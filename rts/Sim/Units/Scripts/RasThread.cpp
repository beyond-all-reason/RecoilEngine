/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "RasThread.h"

#include "CobDeferredCallin.h"
#include "RasFile.h"
#include "RasInstance.h"
#include "RasOpCodes.h"
#include "RasEngine.h"
#include "Sim/Misc/GlobalConstants.h"

#ifdef RAS_PROFILE_OPCODES
	#include <atomic>
	#include <cstdint>
#endif

#include "System/Misc/TracyDefs.h"

// -----------------------------------------------------------------------------
// I.0 Baseline & harness (RASC plan Part I.0)
//
// Compile-guarded opcode counter for the RAS* VM. This is a profiling-only
// diagnostic and is ZERO-COST in normal/release builds: unless the translation
// unit is compiled with -DRAS_PROFILE_OPCODES the RAS_COUNT_OPCODE() macro
// expands to an empty statement and gRasOpcodeCount does not exist. The counter
// is a relaxed atomic so it can be sampled from a headless bench harness without
// data races; it NEVER feeds back into synced state, so determinism is preserved.
//
// Headless micro-bench (how to use this harness):
//   1. Configure the build with the counter + detailed Tracy zones enabled, e.g.
//        cmake -DCMAKE_CXX_FLAGS="-DRAS_PROFILE_OPCODES -DRECOIL_DETAILED_TRACY_ZONING" ...
//      (MSVC: add /D RAS_PROFILE_OPCODES /D RECOIL_DETAILED_TRACY_ZONING).
//   2. Run spring-headless on a minimal map/mod whose units use representative
//      hot scripts (a constant-acceleration turret + a walker, e.g.
//      constant_acceleration_turret_turning*.bos and a legged walker .bos),
//      driving them every sim frame (aim/turn/walk).
//   3. ns/tick: read the "CRasEngine::Tick" / "CRasThread::Tick" Tracy zones
//      (connect tracy-profiler, or capture with tracy-capture for a CLI dump).
//      opcodes/sec: the "RasOpcodes/tick" Tracy plot (sampled per sim Tick below)
//      divided by the sim tick period, or sample gRasOpcodeCount before/after a
//      fixed wall-clock window from a tiny harness.
//   4. Record ns/tick and opcodes/sec on both GCC and MSVC as the I.0 baseline;
//      every later Part I/VII change is measured against these numbers.
// -----------------------------------------------------------------------------
#ifdef RAS_PROFILE_OPCODES
	std::atomic<uint64_t> gRasOpcodeCount{0};
	#define RAS_COUNT_OPCODE() (gRasOpcodeCount.fetch_add(1, std::memory_order_relaxed))
#else
	#define RAS_COUNT_OPCODE() do {} while (0)
#endif

CR_BIND(CRasThread, )

CR_REG_METADATA(CRasThread, (
	CR_MEMBER(rasInst),
	CR_IGNORED(rasFile),

	CR_MEMBER(id),
	CR_MEMBER(pc),

	CR_MEMBER(wakeTime),
	CR_MEMBER(paramCount),
	CR_MEMBER(retCode),
	CR_MEMBER(cbParam),
	CR_MEMBER(signalMask),

	CR_MEMBER(waitAxis),
	CR_MEMBER(waitPiece),

	CR_IGNORED(errorCounter),

	CR_MEMBER(cbType),
	CR_MEMBER(state),

	CR_MEMBER(luaArgs),
	CR_MEMBER(callStack),
	CR_MEMBER(dataStack),

	CR_IGNORED(threadDeferredCallins)
))

CR_BIND(CRasThread::CallInfo,)

CR_REG_METADATA_SUB(CRasThread, CallInfo,(
	CR_MEMBER(functionId),
	CR_MEMBER(returnAddr),
	CR_MEMBER(stackTop)
))

std::vector<decltype(CRasThread::dataStack)> CRasThread::freeDataStacks;
std::vector<decltype(CRasThread::callStack)> CRasThread::freeCallStacks;

CRasThread::CRasThread(CRasInstance* _rasInst)
	: rasInst(_rasInst)
	, rasFile(_rasInst->rasFile)
{
	// If there are any free data and call stacks available, reuse them by 
	// moving them to the current thread's data and call stack variables to
	// amortize memory allocations.
	if (!freeDataStacks.empty()) {
		assert(freeDataStacks.size() == freeCallStacks.size());
		dataStack = std::move(freeDataStacks.back());
		freeDataStacks.pop_back();
		callStack = std::move(freeCallStacks.back());
		freeCallStacks.pop_back();
	} else {
    	// These reservation sizes were experimentally obtained from a few
    	// games in BAR, but regardless of the game being played, the size of
    	// all stacks in use will over time converge to the max size because we
    	// are reusing vectors from older threads.
		dataStack.reserve(16);
		callStack.reserve(4);
	}
	memset(&luaArgs[0], 0, MAX_LUA_COB_ARGS * sizeof(luaArgs[0]));
}

CRasThread::~CRasThread()
{
	RECOIL_DETAILED_TRACY_ZONE;
	Stop();

	if (dataStack.capacity() > 0) {
		dataStack.clear();
		freeDataStacks.emplace_back(std::move(dataStack));
		callStack.clear();
		freeCallStacks.emplace_back(std::move(callStack));
	}
}

CRasThread& CRasThread::operator = (CRasThread&& t) {
	id = t.id;
	pc = t.pc;

	wakeTime = t.wakeTime;
	paramCount = t.paramCount;
	retCode = t.retCode;
	cbParam = t.cbParam;
	signalMask = t.signalMask;

	waitAxis = t.waitAxis;
	waitPiece = t.waitPiece;

	std::memcpy(luaArgs, t.luaArgs, sizeof(luaArgs));

	callStack = std::move(t.callStack);
	dataStack = std::move(t.dataStack);
	// execTrace = std::move(t.execTrace);

	state = t.state;
	cbType = t.cbType;

	rasInst = t.rasInst; t.rasInst = nullptr;
	rasFile = t.rasFile; t.rasFile = nullptr;
	threadDeferredCallins = std::move(t.threadDeferredCallins);
	pendingSpawns = std::move(t.pendingSpawns);
	parallelTick = t.parallelTick;
	return *this;
}

CRasThread& CRasThread::operator = (const CRasThread& t) {
	id = t.id;
	pc = t.pc;

	wakeTime = t.wakeTime;
	paramCount = t.paramCount;
	retCode = t.retCode;
	cbParam = t.cbParam;
	signalMask = t.signalMask;

	waitAxis = t.waitAxis;
	waitPiece = t.waitPiece;

	std::memcpy(luaArgs, t.luaArgs, sizeof(luaArgs));

	callStack = t.callStack;
	dataStack = t.dataStack;
	// execTrace = t.execTrace;

	state = t.state;
	cbType = t.cbType;

	rasInst = t.rasInst;
	rasFile = t.rasFile;
	threadDeferredCallins = t.threadDeferredCallins;
	pendingSpawns = t.pendingSpawns;
	parallelTick = t.parallelTick;
	return *this;
}


void CRasThread::Start(int functionId, int sigMask, const std::array<int, 1 + MAX_COB_ARGS>& args, bool schedule)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(callStack.size() == 0);

	state = Run;
	pc = rasFile->decodedOffsets[functionId];

	// Part I.5: Pre-reserve data stack to avoid reallocations during Tick().
	if (functionId >= 0 && functionId < static_cast<int>(rasFile->maxStackDepth.size())) {
		const int mdepth = rasFile->maxStackDepth[functionId];
		if (mdepth > static_cast<int>(dataStack.capacity()))
			dataStack.reserve(mdepth);
	}

	paramCount = args[0];
	signalMask = sigMask;

	CallInfo& ci = PushCallStackRef();
	ci.functionId = functionId;
	ci.returnAddr = -1;
	ci.stackTop   = 0;

	// copy arguments; args[0] holds the count
	// handled by InitStack if thread has a parent that STARTs it,
	// in which case args[0] is 0 and stack already contains data
	if (paramCount > 0) {
		dataStack.resize(paramCount);
		dataStack.assign(args.begin() + 1, args.begin() + 1 + paramCount);
	}

	// add to scheduler
	if (schedule)
		rasEngine->ScheduleThread(this);
}

void CRasThread::Stop()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (rasInst == nullptr)
		return;

	if (cbType != CRasInstance::CBNone)
		rasInst->ThreadCallback(cbType, retCode, cbParam);

	rasInst->RemoveThreadID(id);
	SetState(Dead);

	rasInst = nullptr;
	rasFile = nullptr;
}


const std::string& CRasThread::GetName()
{
	RECOIL_DETAILED_TRACY_ZONE;
	return rasFile->scriptNames[callStack[0].functionId];
}


int CRasThread::CheckStack(unsigned int size, bool warn)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (size <= dataStack.size())
		return size;

	if (warn) {
		char msg[512];
		const char* fmt =
			"stack-size mismatch: need %u but have %d arguments "
			"(too many passed to function or too few returned?)";

		SNPRINTF(msg, sizeof(msg), fmt, size, dataStack.size());
		ShowError(msg);
	}

	return dataStack.size();
}

void CRasThread::InitStack(unsigned int n, CRasThread* t)
{
	RECOIL_DETAILED_TRACY_ZONE;
	assert(dataStack.size() == 0);

	// move n arguments from caller's stack onto our own
	for (unsigned int i = 0; i < n; ++i) {
		PushDataStack(t->PopDataStack());
	}
}

#if 0
#define GET_LONG_PC() (rasFile->code[pc++])
#else
// mantis #5981
#define GET_LONG_PC() (rasFile->code.at(pc++))
#endif

bool CRasThread::Tick()
{
	assert(state != Sleep);
	assert(rasInst != nullptr);

	if (IsDead())
		return false;

	ZoneScoped;

	state = Run;

	int r1, r2, r3, r4, r5, r6;

	while (state == Run) {
		const RasInstr& I = rasFile->decoded[pc];
		pc++;
		RAS_COUNT_OPCODE();

		switch (I.op) {
			case static_cast<uint8_t>(RasOp::PushConstant): {
				r1 = I.a;
				PushDataStack(r1);
			} break;
			case static_cast<uint8_t>(RasOp::Sleep): {
				r1 = PopDataStack();
				wakeTime = rasEngine->GetCurrTime() + r1;
				state = Sleep;

				rasEngine->ScheduleThread(this);
				return true;
			} break;
			case static_cast<uint8_t>(RasOp::Spin): {
				r1 = I.a;
				r2 = I.b;
				r3 = PopDataStack();
				r4 = PopDataStack();
				rasInst->Spin(r1, r2, r3, r4);
			} break;
			case static_cast<uint8_t>(RasOp::StopSpin): {
				r1 = I.a;
				r2 = I.b;
				r3 = PopDataStack();

				rasInst->StopSpin(r1, r2, r3);
			} break;
			case static_cast<uint8_t>(RasOp::Return): {
				retCode = PopDataStack();

				if (LocalReturnAddr() == -1) {
					state = Dead;

					return false;
				}

				pc = LocalReturnAddr();
				if (dataStack.size() > LocalStackFrame())
					dataStack.resize(LocalStackFrame());

				callStack.pop_back();
			} break;

			case static_cast<uint8_t>(RasOp::Shade): {
				r1 = I.a;
			} break;
			case static_cast<uint8_t>(RasOp::DontShade): {
				r1 = I.a;
			} break;
			case static_cast<uint8_t>(RasOp::Cache): {
				r1 = I.a;
			} break;
			case static_cast<uint8_t>(RasOp::DontCache): {
				r1 = I.a;
			} break;

			case static_cast<uint8_t>(RasOp::SignatureLua): {
				LOG_L(L_ERROR, "BAD ACCESS: Entered a lua method reference.");
				state = Dead;
				return false;
			} break;

			case static_cast<uint8_t>(RasOp::BatchLua): {
				const int dr1 = I.a;
				const int dr2 = I.b;

				if (!luaRules) {
					retCode = 0;
					break;
				}

				if (static_cast<size_t>(dr1) >= rasFile->luaScripts.size()) {
					retCode = 0;
					break;
				}

				auto d = CCobDeferredCallin(rasInst->GetUnit(), rasFile->luaScripts[dr1], dataStack, dr2);
				threadDeferredCallins.push_back(std::move(d));
				retCode = 1;
			} break;

			case static_cast<uint8_t>(RasOp::LuaUnsynced): {
				const int dr1 = I.a;
				const int dr2 = I.b;

				if (!luaRules) {
					break;
				}

				if (static_cast<size_t>(dr1) >= rasFile->luaScripts.size()) {
					break;
				}

				auto d = CCobDeferredCallin(rasInst->GetUnit(), rasFile->luaScripts[dr1], dataStack, dr2);
				threadDeferredCallins.push_back(std::move(d));
			} break;

			case static_cast<uint8_t>(RasOp::Call): {
				r1 = I.a;
				r2 = I.b;

				// lua_synced_*/lua_unsynced_* CALLs are pre-resolved to
				// LuaCall/LuaUnsynced during decode, so here r1 is always a
				// regular function index.
				if (rasFile->scriptLengths[r1] == 0)
					break;
				// Never enter a Lua-signature (non-executable) body.
				if (rasFile->luaScripts[r1].GetString()[0] != '\0')
					break;

				CallInfo& ci = PushCallStackRef();
				ci.functionId = r1;
				ci.returnAddr = pc;
				ci.stackTop = dataStack.size() - r2;

				paramCount = r2;

				pc = rasFile->decodedOffsets[r1];
			} break;
			case static_cast<uint8_t>(RasOp::LuaCall): {
				r1 = I.a;
				r2 = I.b;
				CRasStackGuard guard{&dataStack, r2};

				const int size = static_cast<int>(dataStack.size());
				const int argCount = std::min(r2, MAX_LUA_COB_ARGS);
				const int start = std::max(0, size - r2);
				const int end = std::min(size, start + argCount);

				for (int a = 0, i = start; i < end; i++) {
					luaArgs[a++] = dataStack[i];
				}

				if (!luaRules) {
					luaArgs[0] = 0;
					retCode = 0;
					break;
				}

				if (static_cast<size_t>(r1) >= rasFile->luaScripts.size()) {
					luaArgs[0] = 0;
					retCode = 0;
					break;
				}

				int argsCount = argCount;
				luaRules->syncedLuaHandle.Cob2Lua(rasFile->luaScripts[r1], rasInst->GetUnit(), argsCount, luaArgs);
				retCode = luaArgs[0];
			} break;

			case static_cast<uint8_t>(RasOp::PopStatic): {
				r1 = I.a;
				r2 = PopDataStack();

				if (static_cast<size_t>(r1) < rasInst->staticVars.size())
					rasInst->staticVars[r1] = r2;
			} break;
			case static_cast<uint8_t>(RasOp::PopStack): {
				PopDataStack();
			} break;

			case static_cast<uint8_t>(RasOp::Start): {
				r1 = I.a;
				r2 = I.b;

				if (rasFile->scriptLengths[r1] == 0)
					break;
				// Never spawn a thread into a Lua-signature (non-executable) body.
				if (rasFile->luaScripts[r1].GetString()[0] != '\0')
					break;

				CRasThread t(rasInst);

				if (parallelTick) {
					// Defer ID assignment + scheduling to after the parallel
					// barrier so spawn order is deterministic (Part VII).
					t.InitStack(r2, this);
					t.Start(r1, signalMask, {{0}}, false);
					AddPendingSpawn(std::move(t));
				} else {
					t.SetID(rasEngine->GenThreadID());
					t.InitStack(r2, this);
					t.Start(r1, signalMask, {{0}}, true);
					rasEngine->QueueAddThread(std::move(t));
				}
			} break;

			case static_cast<uint8_t>(RasOp::CreateLocalVar): {
				if (paramCount == 0) {
					PushDataStack(0);
				} else {
					paramCount--;
				}
			} break;
			case static_cast<uint8_t>(RasOp::GetUnitValue): {
				r1 = PopDataStack();
				if ((r1 >= LUA0) && (r1 <= LUA9)) {
					PushDataStack(luaArgs[r1 - LUA0]);
					break;
				}
				r1 = rasInst->GetUnitVal(r1, 0, 0, 0, 0);
				PushDataStack(r1);
			} break;

			case static_cast<uint8_t>(RasOp::JumpNotEqual): {
				r1 = I.a;
				r2 = PopDataStack();

				if (r2 == 0)
					pc = r1;
			} break;
			case static_cast<uint8_t>(RasOp::Jump): {
				pc = I.a;
			} break;

			case static_cast<uint8_t>(RasOp::PopLocalVar): {
				r1 = I.a;
				r2 = PopDataStack();
				dataStack[LocalStackFrame() + r1] = r2;
			} break;
			case static_cast<uint8_t>(RasOp::PushLocalVar): {
				r1 = I.a;
				r2 = dataStack[LocalStackFrame() + r1];
				PushDataStack(r2);
			} break;

			case static_cast<uint8_t>(RasOp::BitwiseAnd): {
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(r1 & r2);
			} break;
			case static_cast<uint8_t>(RasOp::BitwiseOr): {
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(r1 | r2);
			} break;
			case static_cast<uint8_t>(RasOp::BitwiseXor): {
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(r1 ^ r2);
			} break;
			case static_cast<uint8_t>(RasOp::BitwiseNot): {
				r1 = PopDataStack();
				PushDataStack(~r1);
			} break;

			case static_cast<uint8_t>(RasOp::Explode): {
				r1 = I.a;
				r2 = PopDataStack();
				rasInst->Explode(r1, r2);
			} break;

			case static_cast<uint8_t>(RasOp::PlaySound): {
				r1 = I.a;
				r2 = PopDataStack();
				rasInst->PlayUnitSound(r1, r2);
			} break;

		case static_cast<uint8_t>(RasOp::PushStatic): {
			r1 = I.a;

			if (static_cast<size_t>(r1) < rasInst->staticVars.size())
				PushDataStack(rasInst->staticVars[r1]);
		} break;

		case static_cast<uint8_t>(RasOp::PushStaticIdx): {
			r1 = I.a;
			r2 = PopDataStack();
			r3 = r1 + r2;

			if (r3 < 0) {
				r3 = 0;
				ShowError("static array index out of range (low)");
			}
			if (static_cast<size_t>(r3) >= rasInst->staticVars.size()) {
				r3 = static_cast<int>(rasInst->staticVars.size()) - 1;
				ShowError("static array index out of range (high)");
			}
			if (r3 >= 0)
				PushDataStack(rasInst->staticVars[r3]);
		} break;

		case static_cast<uint8_t>(RasOp::PopStaticIdx): {
			r1 = I.a;
			r2 = PopDataStack();
			r3 = PopDataStack();
			r4 = r1 + r3;

			if (r4 < 0) {
				r4 = 0;
				ShowError("static array index out of range (low)");
			}
			if (static_cast<size_t>(r4) >= rasInst->staticVars.size()) {
				r4 = static_cast<int>(rasInst->staticVars.size()) - 1;
				ShowError("static array index out of range (high)");
			}
			if (r4 >= 0 && static_cast<size_t>(r4) < rasInst->staticVars.size())
				rasInst->staticVars[r4] = r2;
		} break;

		case static_cast<uint8_t>(RasOp::SetNotEqual): {
				r1 = PopDataStack();
				r2 = PopDataStack();

				PushDataStack(int(r1 != r2));
			} break;
			case static_cast<uint8_t>(RasOp::SetEqual): {
				r1 = PopDataStack();
				r2 = PopDataStack();

				PushDataStack(int(r1 == r2));
			} break;

			case static_cast<uint8_t>(RasOp::SetLess): {
				r2 = PopDataStack();
				r1 = PopDataStack();

				PushDataStack(int(r1 < r2));
			} break;
			case static_cast<uint8_t>(RasOp::SetLessOrEqual): {
				r2 = PopDataStack();
				r1 = PopDataStack();

				PushDataStack(int(r1 <= r2));
			} break;

			case static_cast<uint8_t>(RasOp::SetGreater): {
				r2 = PopDataStack();
				r1 = PopDataStack();

				PushDataStack(int(r1 > r2));
			} break;
			case static_cast<uint8_t>(RasOp::SetGreaterOrEq): {
				r2 = PopDataStack();
				r1 = PopDataStack();

				PushDataStack(int(r1 >= r2));
			} break;

			case static_cast<uint8_t>(RasOp::Rand): {
				r2 = PopDataStack();
				r1 = PopDataStack();
				const int range = r2 - r1 + 1;
				r3 = (range > 0) ? (rasInst->rng.NextInt(range) + r1) : r1;
				PushDataStack(r3);
			} break;
			case static_cast<uint8_t>(RasOp::EmitSfx): {
				r1 = PopDataStack();
				r2 = I.a;
				rasInst->EmitSfx(r1, r2);
			} break;
			case static_cast<uint8_t>(RasOp::Mul): {
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(r1 * r2);
			} break;

			case static_cast<uint8_t>(RasOp::Signal): {
				r1 = PopDataStack();
				rasInst->Signal(r1);
			} break;
			case static_cast<uint8_t>(RasOp::SetSignalMask): {
				r1 = PopDataStack();
				signalMask = r1;
			} break;

			case static_cast<uint8_t>(RasOp::Turn): {
				r2 = PopDataStack();
				r1 = PopDataStack();
				r3 = I.a;
				r4 = I.b;

				rasInst->Turn(r3, r4, r1, r2);
			} break;
			case static_cast<uint8_t>(RasOp::Get): {
				r5 = PopDataStack();
				r4 = PopDataStack();
				r3 = PopDataStack();
				r2 = PopDataStack();
				r1 = PopDataStack();
				if ((r1 >= LUA0) && (r1 <= LUA9)) {
					PushDataStack(luaArgs[r1 - LUA0]);
					break;
				}
				r6 = rasInst->GetUnitVal(r1, r2, r3, r4, r5);
				PushDataStack(r6);
			} break;
			case static_cast<uint8_t>(RasOp::Add): {
				r2 = PopDataStack();
				r1 = PopDataStack();
				PushDataStack(r1 + r2);
			} break;
			case static_cast<uint8_t>(RasOp::Sub): {
				r2 = PopDataStack();
				r1 = PopDataStack();
				r3 = r1 - r2;
				PushDataStack(r3);
			} break;

			case static_cast<uint8_t>(RasOp::Div): {
				r2 = PopDataStack();
				r1 = PopDataStack();

				if (r2 != 0) {
					r3 = r1 / r2;
				} else {
					r3 = 1000;
					ShowError("division by zero");
				}
				PushDataStack(r3);
			} break;
			case static_cast<uint8_t>(RasOp::Mod): {
				r2 = PopDataStack();
				r1 = PopDataStack();

				if (r2 != 0) {
					PushDataStack(r1 % r2);
				} else {
					PushDataStack(0);
					ShowError("modulo division by zero");
				}
			} break;

			case static_cast<uint8_t>(RasOp::Move): {
				r1 = I.a;
				r2 = I.b;
				r4 = PopDataStack();
				r3 = PopDataStack();
				rasInst->Move(r1, r2, r3, r4);
			} break;
			case static_cast<uint8_t>(RasOp::MoveNow): {
				r1 = I.a;
				r2 = I.b;
				r3 = PopDataStack();
				rasInst->MoveNow(r1, r2, r3);
			} break;
			case static_cast<uint8_t>(RasOp::TurnNow): {
				r1 = I.a;
				r2 = I.b;
				r3 = PopDataStack();
				rasInst->TurnNow(r1, r2, r3);
			} break;
			case static_cast<uint8_t>(RasOp::Scale): {
				r1 = I.a;
				r3 = PopDataStack();
				r2 = PopDataStack();
				rasInst->Scale(r1, r2, r3);
			} break;
		case static_cast<uint8_t>(RasOp::ScaleNow): {
			r1 = I.a;
			r2 = PopDataStack();
			rasInst->ScaleNow(r1, r2);
		} break;

		case static_cast<uint8_t>(RasOp::TurnRel): {
			r1 = PopDataStack();  // piece index
			r2 = PopDataStack();  // angle
			if (r1 < 0 || static_cast<size_t>(r1) >= rasFile->pieceNames.size()) {
				ShowError("turn-rel: invalid piece");
				break;
			}
			rasInst->TurnNow(r1, 2, r2);
		} break;
		case static_cast<uint8_t>(RasOp::MoveRel): {
			r1 = PopDataStack();  // piece index
			r2 = PopDataStack();  // delta
			if (r1 < 0 || static_cast<size_t>(r1) >= rasFile->pieceNames.size()) {
				ShowError("move-rel: invalid piece");
				break;
			}
			rasInst->MoveNow(r1, 0, r2);
		} break;
		case static_cast<uint8_t>(RasOp::ExplodeRel): {
			r1 = PopDataStack();  // piece index
			if (r1 < 0 || static_cast<size_t>(r1) >= rasFile->pieceNames.size()) {
				ShowError("explode-rel: invalid piece");
				break;
			}
			rasInst->Explode(r1, 0);
		} break;
		case static_cast<uint8_t>(RasOp::ScaleRel): {
			r1 = PopDataStack();  // piece index
			r2 = PopDataStack();  // scale
			if (r1 < 0 || static_cast<size_t>(r1) >= rasFile->pieceNames.size()) {
				ShowError("scale-rel: invalid piece");
				break;
			}
			rasInst->ScaleNow(r1, r2);
		} break;

		case static_cast<uint8_t>(RasOp::WaitTurn): {
				r1 = I.a;
				r2 = I.b;

				if (rasInst->NeedsWait(CRasInstance::ATurn, r1, r2)) {
					state = WaitTurn;
					waitPiece = r1;
					waitAxis = r2;
					return true;
				}
			} break;
			case static_cast<uint8_t>(RasOp::WaitMove): {
				r1 = I.a;
				r2 = I.b;

				if (rasInst->NeedsWait(CRasInstance::AMove, r1, r2)) {
					state = WaitMove;
					waitPiece = r1;
					waitAxis = r2;
					return true;
				}
			} break;
			case static_cast<uint8_t>(RasOp::WaitScale): {
				r1 = I.a;

				if (rasInst->NeedsWait(CRasInstance::AScale, r1, -1)) {
					state = WaitScale;
					waitPiece = r1;
					waitAxis = -1;
					return true;
				}
			} break;

			case static_cast<uint8_t>(RasOp::Set): {
				r2 = PopDataStack();
				r1 = PopDataStack();

				if ((r1 >= LUA0) && (r1 <= LUA9)) {
					luaArgs[r1 - LUA0] = r2;
					break;
				}

				rasInst->SetUnitVal(r1, r2);
			} break;

			case static_cast<uint8_t>(RasOp::Attach): {
				r3 = PopDataStack();
				r2 = PopDataStack();
				r1 = PopDataStack();
				rasInst->AttachUnit(r2, r1);
			} break;
			case static_cast<uint8_t>(RasOp::Drop): {
				r1 = PopDataStack();
				rasInst->DropUnit(r1);
			} break;

			case static_cast<uint8_t>(RasOp::LogicalNot): {
				r1 = PopDataStack();
				PushDataStack(int(r1 == 0));
			} break;
			case static_cast<uint8_t>(RasOp::LogicalAnd): {
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(int(r1 && r2));
			} break;
			case static_cast<uint8_t>(RasOp::LogicalOr): {
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(int(r1 || r2));
			} break;
			case static_cast<uint8_t>(RasOp::LogicalXor): {
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(int((!!r1) ^ (!!r2)));
			} break;

			case static_cast<uint8_t>(RasOp::Absolute): {
				r1 = PopDataStack();
				PushDataStack(r1 < 0 ? -r1 : r1);
			} break;
			case static_cast<uint8_t>(RasOp::Minimum): {
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(r1 < r2 ? r1 : r2);
			} break;
			case static_cast<uint8_t>(RasOp::Maximum): {
				r1 = PopDataStack();
				r2 = PopDataStack();
				PushDataStack(r1 > r2 ? r1 : r2);
			} break;
			case static_cast<uint8_t>(RasOp::Sign): {
				r1 = PopDataStack();
				PushDataStack(r1 > 0 ? 1 : (r1 < 0 ? -1 : 0));
			} break;
			case static_cast<uint8_t>(RasOp::Clamp): {
				r1 = PopDataStack();
				r2 = PopDataStack();
				r3 = PopDataStack();
				if (r1 < r2) r1 = r2;
				if (r1 > r3) r1 = r3;
				PushDataStack(r1);
			} break;
			case static_cast<uint8_t>(RasOp::AddImm): {
				r1 = PopDataStack();
				PushDataStack(r1 + I.a);
			} break;
			case static_cast<uint8_t>(RasOp::MulImm): {
				r1 = PopDataStack();
				PushDataStack(r1 * I.a);
			} break;

			case static_cast<uint8_t>(RasOp::Hide): {
				r1 = I.a;
				rasInst->SetVisibility(r1, false);
			} break;

		case static_cast<uint8_t>(RasOp::Show): {
			r1 = I.a;
			rasInst->SetVisibility(r1, true);
		} break;

			default: {
				const char* name = rasFile->name.c_str();
				const char* func = rasFile->scriptNames[LocalFunctionID()].c_str();

				LOG_L(L_ERROR, "[COBThread::%s] unknown opcode %x (in %s:%s at %x)", __func__, I.op, name, func, pc - 1);

				state = Dead;
				return false;
			} break;
		}
	}

	return (state != Dead);
}

void CRasThread::ShowError(const char* msg)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if ((errorCounter = std::max(errorCounter - 1, 0)) == 0)
		return;

	if (callStack.size() == 0) {
		LOG_L(L_ERROR, "[COBThread::%s] %s outside script execution (?)", __func__, msg);
		return;
	}

	const char* name = rasFile->name.c_str();
	const char* func = rasFile->scriptNames[LocalFunctionID()].c_str();

	LOG_L(L_ERROR, "[COBThread::%s] %s (in %s:%s at %x)", __func__, msg, name, func, pc - 1);
}


void CRasThread::AnimFinished(CUnitScript::AnimType type, int piece, int axis)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (piece != waitPiece || axis != waitAxis)
		return;

	if (!Reschedule(type))
		return;

	state = Run;
	waitPiece = -1;
	waitAxis = -1;

	rasEngine->ScheduleThread(this);
}

