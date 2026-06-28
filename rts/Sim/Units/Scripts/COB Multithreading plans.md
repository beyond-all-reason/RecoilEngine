# Plan: Multi-threaded COB script execution

## Goal
Parallelize COB VM opcode execution across units while preserving determinism/sync.
Phases: A) per-unit LFSR RNG, B) compiler thread-safe flagging + COB format,
C0) stop runtime bytecode mutation, C1) LUA_UNSYNCED_CALL, C) engine MT scheduler.

## Decisions (confirmed with user)
- Implement all parts, phased. C depends on A + B + C0 + C1.
- COB format: bump version, append per-function flag bitmask, engine reads conditionally.
- Unsafe ops: lua_call, explode, emit-sfx, attach-unit, drop-unit, and a SMALL curated
  list of unsafe SET value-IDs (cross-unit: KILL_UNIT, weapon targets, BUGGER_OFF, etc.).
  GET / GET_UNIT_VALUE treated as SAFE (modder responsible to only use sync-safe gets in
  MT section). call-script propagates (caller safe only if callee safe). Safe: piece anim,
  wait/sleep, arithmetic/logic, local+static vars, signal, start-script, rand (after A), get.
- Thread tagging is done by the COMPILER: no dynamic calls exist, so the static call graph
  is complete. Per-function thread-safe flag IS the thread tag; engine just tags a thread
  with the flag of the function it starts with.
- Side-effect determinism: per-worker buffers, merged in unit-ID order (no global locks).
- RNG seed = unit->id + gs->frameNum at unit creation, LFSR 32-bit.

## Key files / facts
- Engine VM: rts/Sim/Units/Scripts/CobEngine.{h,cpp} (Tick, TickRunningThreads,
  WakeSleepingThreads, ProcessQueuedThreads, ScheduleThread, threadInstances map,
  runningThreadIDs/waitingThreadIDs/sleepingThreadIDs, tickAddedThreads/tickRemovedThreads,
  deferredCallins, GenThreadID).
- CobThread.{h,cpp}: Tick() opcode loop. RAND at ~L517 uses gsRNG. START ~L373 queues thread.
  CALL/REAL_CALL/LUA_CALL ~L325. DeferredCall/BATCH_LUA. entry func at Start().
- CobInstance.{h,cpp}: staticVars, threadIDs, Signal(), creg CR_REG_METADATA (add RNG member).
  Constructor CCobInstance(CCobFile*, CUnit*); GetUnit().
- Unit creation: Unit.cpp PreInit sets id (~L196), PostInit creates script (~L313) + Create().
- RNG infra: gsRNG = CGlobalRNG<PCG32,true,true> in GlobalSynced; GlobalRNG.h.
  Will add a separate simple LFSR class for per-unit (don't reuse PCG state semantics).
- Compiler: BARScriptCompiler/bos2cob_py3.py (OPCODES L30-95 std, L101-184 short;
  parse_file collects self._functions L770; parse_funcDec compiles L806;
  call/start emit L880-950; get/set emit). cob_file.py writes header+layout L26-92.
- Engine loader: CobFile.{h,cpp}; READ_COBHEADER macro L39-68; ctor L91-175.
  Add std::vector<bool>/uint8 threadSafe flags member to CCobFile + move-assign.
- UnitScriptEngine.cpp Tick(): calls cobEngine->Tick (sequential), then for_mt anims,
  then ST checksum, then RunDeferredCallins.
- ThreadPool: for_mt in System/Threading/ThreadPool.h.

## Phase A - per-unit deterministic LFSR RNG
WHY: COB `RAND` currently calls the GLOBAL synced RNG `gsRNG.NextInt(...)` in the RAND
  opcode (CobThread.cpp RAND case ~L515-519: `r3 = gsRNG.NextInt(r2 - r1 + 1) + r1;`).
  Under MT this is BOTH a data race on gsRNG's state AND a determinism break: the *order*
  in which units consume gsRNG is exactly what becomes arbitrary once units run in
  parallel, so each client would draw different numbers and desync. Giving every unit its
  own independent, deterministically-seeded stream removes RAND from shared/global state
  so it can be called from any worker.

1. Add a tiny 32-bit LFSR/xorshift generator (e.g. `CCobRNG`) as a new header under
   rts/Sim/Units/Scripts/ (or rts/System/).
   HOW: single uint32 state + advance() (xorshift32 or a Galois LFSR), exposing a bounded
   call like `NextRand(N)` mirroring gsRNG.NextInt(N).
   WHY a NEW class rather than reuse CGlobalRNG<PCG32> (GlobalRNG.h:101-215, typedef'd as
   CGlobalSyncedRNG at GlobalRNG.h:215): PCG32 keeps a 64-bit state and is wired into the
   global synced-RNG bookkeeping (AssureSyncedness / GetGenState at GlobalRNG.h:138, used
   in sync dumps). Per-unit streams must NOT participate in that single global synced
   object. The spec also explicitly asks for a 32-bit LFSR.

2. Add the RNG state as a member of CCobInstance (CobInstance.h, near `std::vector<int>
   staticVars;` at L49) and register it with creg in CobInstance.cpp:45-54 via CR_MEMBER,
   alongside the existing CR_MEMBER(staticVars) / CR_MEMBER(threadIDs).
   WHY creg: save/load must restore the exact stream position, or a reloaded game desyncs
   from a live one; staticVars/threadIDs are CR_MEMBER for the same reason. The existing
   CR_PREALLOC(GetUnit) / CR_POSTLOAD(PostLoad) (CobInstance.cpp:45-54, PostLoad at
   CobInstance.cpp:75-88) already make `unit` available, so re-seeding on load is optional.

3. Seed at construction. WHERE: CCobInstance(CCobFile*, CUnit*) ctor (CobInstance.h:46) ->
   Init() (CobInstance.cpp:69-73). HOW: seed = unit->id + gs->frameNum.
   - unit->id is CWorldObject::id (WorldObject.h:80), set in CUnit::PreInit
     (Unit.cpp:196: `id = params.unitID;`) which runs BEFORE PostInit creates the script
     (Unit.cpp:313: `script = CUnitScriptFactory::CreateScript(...)`), so id is valid at
     ctor time.
   - gs->frameNum is the synced frame counter (GlobalSynced.h:63).
   WHY this seed: both inputs are synced and identical on every client; adding frameNum
   decorrelates units created at different times that would otherwise share a seed.

4. Replace the gsRNG call in the RAND opcode (CobThread.cpp ~L517) with
   `cobInst->NextRand(r2 - r1 + 1) + r1`.
   WHY only here: this is the sole COB-script consumer of gsRNG the spec targets. NOTE:
   CUnitScript::Explode (UnitScript.cpp ~L924) also uses gsRNG.NextFloat for particle
   spread, but EXPLODE is classified UNSAFE (Phase B) and stays on the serial path, so it
   can keep using gsRNG safely.

VERIFY: identical seed -> identical sequence across runs; because no shared RNG state is
  touched, the global sync checksum (Sync::Assert in UnitScriptEngine::Tick) is unaffected.

## Phase B - compiler thread-safe flagging + COB format
WHY: the engine cannot cheaply decide at runtime whether a whole thread is safe (a thread
  runs many opcodes across many functions, and GET/SET take a runtime param). The COMPILER
  (bos2cob_py3.py) sees the entire program and, because COB has NO dynamic/indirect calls,
  can compute an EXACT static call graph and a per-function thread-safe bit once at build
  time. The engine then only reads the bit (cheap, deterministic).

1. Define the unsafe set in bos2cob_py3.py near the OPCODES dict (std L30-95 / short
   L101-184; both struct.pack at ~L200).
   UNSAFE: LUA_CALL (target function named `lua_*`), EXPLODE, EMIT_SFX, ATTACH_UNIT,
   DROP_UNIT, and SET when its value-id is on a SMALL curated cross-unit list (KILL_UNIT,
   SET_WEAPON_*_TARGET, BUGGER_OFF, ...).
   WHY these: per engine exploration (UnitScript.cpp GetUnitVal ~L984-1533 / SetUnitVal
   ~L1541-1775, plus the CobThread opcode cases) these reach OTHER units / global sim /
   synced Lua. GET/GET_UNIT_VALUE are SAFE by decision (modder responsibility). SIGNAL
   only kills the OWNING unit's threads (CobInstance::Signal CobInstance.cpp:730-745) and
   START defers to next frame (CobThread START ~L373 -> cobEngine->QueueAddThread), so
   both are SAFE.

2. Per-function analysis. WHERE: functions are collected in order in parse_file
   (bos2cob_py3.py:770-777, building self._functions so name<->index is fixed), then each
   is compiled into self._functions_code by parse_funcDec (L806-825). CALL_SCRIPT /
   START_SCRIPT grammar at L634-635; emitted in parse_keywordStatement (L915-950) where
   funcName resolves via index(self._functions, name).
   HOW: (a) scan each function's emitted bytecode for any UNSAFE opcode; (b) build a call
   graph using CALL_SCRIPT targets ONLY (START_SCRIPT targets become independent threads,
   so they are not edges); (c) propagate to a fixed point: a function is safe iff it has no
   unsafe op AND every call-script callee is safe.
   WHY a fixed-point iteration: recursion and call cycles require convergence, not a single
   pass.

3. Emit flags + bump format. WHERE: cob_file.py COB.__init__ header build (L50-92), code
   written via write_functions_code_array (L26). The header is the 11-field COBHeader
   (mirrored engine-side at CobFile.cpp:22-38; VersionSignature currently 4 std / 6 TA:K).
   HOW: append a per-function bit array after the existing sections; add header fields
   (OffsetToThreadSafeFlags + count) and bump VersionSignature.
   WHY bump the version: a new engine detects+reads the appended flags while engines that
   validate the old version simply ignore the extra data (forward/backward tolerant).

4. Engine reads flags. WHERE: CCobFile ctor (CobFile.cpp:91-211), after READ_COBHEADER
   (macro L41-85) and the code load/byteswap (L165-172).
   HOW: if VersionSignature >= new, read the bit array into a new CCobFile member (e.g.
   `std::vector<uint8_t> threadSafeFuncs`), and add it to the member list (CobFile.h:41-56)
   AND the move-assignment operator (CobFile.h:21-37) so it survives the `CCobFile&&` moves
   the loader uses.
   WHY backward compat: legacy cobs (no flags) => every function treated UNSAFE => serial
   path => today's exact behavior and sync are preserved.

## Phase C0 - prerequisite: stop runtime bytecode mutation
WHY (core correctness bug): the CALL opcode case (CobThread.cpp ~L325-336) SELF-MODIFIES
  the shared bytecode to cache the lua-vs-real decision:
    if scriptNames[r1].find("lua_")==0 -> cobFile->code[pc-1] = LUA_CALL
    else                              -> cobFile->code[pc-1] = REAL_CALL
  CCobFile and its `code` vector (CobFile.h:43) are SHARED by all instances of a unit type
  (CobInstance.cobFile points at the cached file). Two units of the same type ticking COB
  on different workers would write the same vector concurrently => data race / UB. This
  must be eliminated before any MT.

1. Resolve CALLs once, at load. WHERE: CCobFile ctor (CobFile.cpp:91-211) after the code
   is loaded + byteswapped (L165-172) and scriptOffsets/scriptLengths/scriptNames are built
   (L127-152).
   HOW: add an opcode -> operand-word-count table (every GET_LONG_PC in CobThread::Tick
   consumes exactly one inline word; enumerate counts straight from the Tick switch). Walk
   each function's range [scriptOffsets[i], scriptOffsets[i] + scriptLengths[i]) decoding
   opcode-by-opcode, and rewrite each CALL (0x10062000) to LUA_CALL if
   scriptNames[target].find("lua_")==0 (the same test the loader already uses at
   CobFile.cpp:139) else REAL_CALL.
   WHY a proper operand-aware walk (NOT the linear sweep in cob_decompiler.py): operand
   words can numerically equal opcode constants, so boundaries are only correct if we skip
   operands using the count table.

2. Make Tick read-only. WHERE: CobThread.cpp CALL case. HOW: delete the in-loop rewrite;
   keep the existing REAL_CALL and LUA_CALL cases that follow it. GET_LONG_PC already reads
   via code.at(pc++), so Tick becomes purely read-only over `code` (it can be treated as
   const).
   WHY: a read-only Tick over shared CCobFile is the exact invariant the MT scheduler
   (Phase C) depends on.

3. Optional compiler change: have bos2cob emit REAL_CALL / LUA_CALL directly (it already
   knows the lua_ prefix).
   WHY optional: the engine load-time pass stays authoritative so legacy / third-party cobs
   keep working; pre-resolving in the compiler merely skips the walk for new cobs.

VERIFY: assert no CALL (0x10062000) opcodes remain in `code` after load; a sample script
  behaves identically pre/post change.

## Phase C1 - LUA_UNSYNCED_CALL (unsynced notifications)
WHY: scripts need to drive UNSYNCED-only effects (decals, light/particle fx) without
  affecting sim state. Performing the Lua call inline during the MT phase would touch shared
  unsynced Lua state from many workers simultaneously; instead we COLLECT records during MT
  and REPLAY them once, deterministically, after the parallel region.

1. New opcode LUA_UNSYNCED_CALL. WHERE: CobOpCodes.h flow-control block, next to
   `BATCH_LUA = 0x10062004` (L70); plus bos2cob OPCODES (std + short) and a bos
   keyword/builtin.
   WHY a distinct opcode (vs reusing BATCH_LUA): makes intent explicit and lets us
   guarantee it writes NO return value. Contrast LuaCall (CobThread.cpp LuaCall()) which
   sets `retCode = luaArgs[0]`; LUA_UNSYNCED_CALL must leave retCode/luaArgs untouched =>
   pure fire-and-forget.

2. Classify it THREAD-SAFE (Phase B) so it is usable inside MT functions. WHY safe: it only
   enqueues a record; it reads/writes no other unit and no synced state.

3. Collect + dispatch. HOW: reuse CCobDeferredCallin (CobDeferredCallin.h) which already
   carries unit + luaArgs + funcName/funcHash. During the MT phase push records into a
   PER-WORKER buffer, NOT the shared cobEngine->deferredCallins map.
   WHY not the shared map: AddDeferredCallin (CobEngine.cpp) inserts into a shared
   unordered_map<funcHash, vector<...>> and is not MT-safe. After the parallel region, merge
   the per-worker buffers in unitID order, then dispatch via the existing RunDeferredCallins
   path (CobEngine.cpp:RunDeferredCallins) which calls
   luaRules->unsyncedLuaHandle.Cob2LuaBatch + luaUI->Cob2LuaBatch.
   WHY unitID-ordered merge: keeps unsynced callback order reproducible run-to-run (good
   for demos/replays), even though it does not affect sync.

4. Fix BATCH_LUA on the same path: it already routes to the unsynced handle
   (RunDeferredCallins) but reaches it via the shared-map AddDeferredCallin, so under MT it
   must use the same per-worker buffer + unitID merge.

## Phase C - engine MT scheduler
WHY: cobEngine->Tick (CobEngine.cpp:191-200) ticks every thread sequentially —
  TickRunningThreads (CobEngine.cpp:173-187) iterates runningThreadIDs in order, then swaps
  running<->waiting. Animation ticking is ALREADY parallel (UnitScriptEngine::Tick uses
  for_mt over TickAllAnims, validated by Sync::Assert), proving the pattern; we extend MT to
  the VM opcode execution for SAFE work while keeping unsafe work serial+ordered for sync.

1. Tag threads. HOW: store `isThreadSafe` on CCobThread, set in CCobThread::Start
   (CobThread.cpp:146, where `pc = cobFile->scriptOffsets[functionId]`) =
   cobFile->threadSafeFuncs[functionId] (Phase B).
   WHY set at Start from the entry function: call-script safety was already propagated by
   the compiler, and start-script spawns separate threads, so the entry function's bit
   correctly characterizes the whole thread run.

2. Partition + group. WHERE: TickRunningThreads (CobEngine.cpp:173). HOW: split
   runningThreadIDs into safe vs unsafe. Group SAFE threads by owning cobInst and run
   unit-groups in parallel via for_mt (System/Threading/ThreadPool.h); within a group, run
   its threads sequentially in deterministic order. Run UNSAFE threads single-threaded in
   deterministic ID order exactly as today.
   WHY group by unit: a unit's threads share staticVars and communicate via SIGNAL
   (CobInstance::Signal CobInstance.cpp:730-745), so they MUST stay on one worker, in order;
   only DIFFERENT units may run concurrently.

3. Buffer side-effects per worker, merge deterministically. WHY: Tick mutates shared engine
   state mid-run:
     - START -> cobEngine->QueueAddThread (tickAddedThreads, CobEngine.h)
     - SLEEP/WAIT_*/RETURN-yield -> cobEngine->ScheduleThread (waitingThreadIDs /
       sleepingThreadIDs, CobEngine.cpp:101-120)
     - SIGNAL -> cobEngine->QueueRemoveThread (tickRemovedThreads)
     - new thread IDs via GenThreadID (threadCounter)
     - deferred/unsynced callins (Phase C1)
   HOW: give each worker local buffers for added/removed threads, schedule pushes, and
   callins; either make GenThreadID atomic OR assign IDs during the merge. After the
   parallel region, merge into the global structures in a FIXED order (unit id, then thread
   id).
   WHY deterministic merge order: thread IDs and schedule order feed future ticks and the
   sync checksum, so results must be identical regardless of OS worker scheduling.

4. Apply the same safe/unsafe partitioning to WakeSleepingThreads (CobEngine.cpp:136-171),
   since woken threads also execute the VM and produce the same side-effects.

5. Keep the Sync::Assert checksum in UnitScriptEngine::Tick; ensure ALL per-worker merges
   complete before the ST checksum / RunDeferredCallins stage runs.

## Verification
- Build engine via CMake; run test/validation and test/headercheck.
  WHY headercheck: new CCobFile member + COBHeader changes must keep headers self-contained.
- Phase A: same seed -> same RAND sequence across runs; confirm sync checksum unchanged
  (no shared RNG touched).
- Phase B: recompile BAR scripts with the new compiler; load in engine; confirm flags are
  read (log threadSafeFuncs) and that a known unsafe function (e.g. one calling lua_*) is
  flagged unsafe while a pure-animation function is flagged safe.
- Phase C0: assert no CALL (0x10062000) opcodes remain in `code` post-load; a sample
  script behaves identically before/after.
- Phase C1: assert unsynced callins are dispatched in unitID order regardless of worker
  scheduling (run repeatedly, compare dispatch order).
- Phase C (sync): run the same sim twice and/or a multi-client demo and compare sync
  checksums (Sync::Assert) — must match the single-threaded baseline bit-for-bit.
- Benchmark COB tick time (Tracy zones around cobEngine->Tick / TickRunningThreads) single
  vs multi core to confirm the speedup.
