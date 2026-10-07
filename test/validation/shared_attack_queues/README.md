# Shared attack queue prototype validation

AI assistance: OpenAI Codex implemented the prototype and these tests. No human
playtest or upstream submission is implied by the automated results.

Build and run `test_CommandQueue` for storage, mutation, tag wraparound,
reentrant batch contexts, reference lifetime, and creg save/load coverage.
`test_ObjectDependencies` checks listener ordering, nested target death, filtering,
shared dependency storage, divergent mutations, recycled addresses and save/load.
The scale case creates 1,000 independent queues with 200 attacks each, checks
every command's fields, and verifies one shared range and zero private Commands
per queue before activation. Reading does not materialize the ranges.

Use an unmodified binary from `401b77dba6` as the baseline. Run the same BAR
checkout and development maps with each binary in separate new directories:

```sh
python3 test/validation/shared_attack_queues/run.py BASELINE_ENGINE BASELINE_ROOT \
  --bar /home/aron/Projects/bar/Beyond-All-Reason \
  --assets /home/aron/Projects/bar/bar-dev/data \
  --base /home/aron/Projects/bar/RecoilEngine/build-amd64-linux/base
python3 test/validation/shared_attack_queues/run.py CANDIDATE_ENGINE CANDIDATE_ROOT \
  --bar /home/aron/Projects/bar/Beyond-All-Reason \
  --assets /home/aron/Projects/bar/bar-dev/data \
  --base /home/aron/Projects/bar/RecoilEngine/build-amd64-linux/base \
  --compare BASELINE_ROOT/trace.txt
```

The runner hardlinks game files where possible, replaces files before editing,
uses only the explicitly provided development assets, and refuses an existing
output directory. Keep the source checkout unchanged during comparisons.
The isolated BAR handler exposes `UnitCommandEnded` if it is not already present.
A minimal LuaUI script sends one controlled network batch; ordinary widgets are
not loaded. Never use the production data directory as an output or asset source.

The trace compares full queue fields, positive/negative indexed Lua getters,
AllowCommand/UnitCommand/UnitCmdDone/UnitCommandEnded order, per-unit rejection,
a nested batch from AllowCommand, insertion/removal, duplicate cancellation,
queued target death, independent progress, stop, ground and bomber command AIs,
synced array/map admission and the actual unsynced network batch receiver.
It is not a complete replay-state comparison.

Additional scenarios use the same runner and fresh output directories:

- `--scenario repeat`: native FinishCommand repeats at different rates for two
  attackers; compare callback order, queue fields and tags with the baseline.
- `--scenario death`: interleave independently issued and batch-issued attacks
  across six units, reject one order, cancel a duplicate, then kill targets.
  A target-loss callback stops another subscriber, issues a nested batch and
  destroys another target. Compare all 39 queue/event snapshots to the baseline.
- `--scenario save`: create `Saves/shared-queues.ssf` with shared and split
  queues. Run again with `--scenario save --resume SAVE_FILE --compare
  SAVE_ROOT/trace.txt` to check the resumed queues, independent Stop and
  removal of a queued target after loading.
  Load each binary's own save; cross-version saves are not supported.
- `--scenario benchmark --attackers 1000 --targets 200`: hold mobile attackers
  with Wait, issue attacks, read all queues, then Stop. `measurements.json`
  records engine-log timings and process RSS sampled at the log markers.
  Run baseline and candidate sequentially with no concurrent builds/tests.
  RSS includes Lua allocation, dependencies and allocator retention; this is
  not an isolated measurement of Command storage or a precise peak RSS.

The creg unit tests additionally verify shared-list identity across 1,001 queues
and two independent save packages, compact serialized size, and sharing after
Repeat. A reverse-scan comparison exercises indexed cancellation after mutations.

## Current boundaries

Only one-parameter `CMD_ATTACK` inputs from the three batch entry points share
storage. Pairwise orders, generated commands, factory queues and other command
types retain private Commands. Acceptance and attack execution are unchanged.
Getters and iterators return owning command values; mutations use `Edit`, while
non-const `front`/`back` retain stable mutable access. Stable references survive
pushes at either end, as with the previous deque. Middle mutations can invalidate
references, as before. Ordinary adjacent commands use a private deque per run.

Command dependencies use persistent ordered sets. A batch memoizes identical
set transitions, so identical target sets and listener sets share one tree each.
Shared roots live in allocated dependence-type slots, so empty preallocated
CObjects keep their original size. The two graph directions and CommandAI's
dependency history remain logically
separate. Death notifications still visit individual command AIs in original
creation order, within the original dependence-type order. No fan-out proxy is
inserted into that order. Save/load retains shared tree identity.

Memoization is scoped to admission/cleanup batches, supports nesting, and retains
at most 32,768 transition entries per batch. Highly divergent histories or cache
eviction can reduce sharing. Ordinary command edges also use persistent sets;
broader performance measurements outside homogeneous batches remain necessary.
The logical N-by-M edges and admission calls still exist; physical storage can
share them. Target-death queue scans remain per command AI. Segment lookup is linear
in the number of ranges; highly divergent queues still need fragmentation and
performance work. Repeat reuses shared inputs when command fields remain equal;
individual modifications fall back to private Commands. Serialization preserves
shared-list identity, range boundaries, private edits and the tag counter.
Attack cancellation uses a shared target-position index, rebuilt lazily after
load; private one-parameter Attack/Fight commands are scanned normally. The same
index rules out impossible Attack overlaps before the ordinary overlap scan.
The internal queue representation and creg schema changed;
this prototype does not claim old-save or C++ queue-API compatibility.

Further acceptance work includes targeted target-death removal, fragmentation limits,
long replay/state comparisons, packet splitting, LOS/unattackability and target
ID reuse scenarios, and broader resource/time measurements in the actual engine.

For human review, run the same isolated `check`, `repeat`, and `death` fixtures
with the graphical `spring` binary as the first runner argument. Observe attack
progress and independent Stop, then inspect the baseline trace comparison. These
scripted checks do not substitute for an ordinary interactive multiplayer playtest.

## Stable replay comparison

`replays.py` reuses the downloaded four-replay selection and the process-group,
heartbeat and timeout implementation from
`pr8935-performance-20261003/replay-search-20261004/find-target-lists.py`.
It adds a fixed three-job matrix per replay, bounded workers, validated resume,
and a state/event comparison that permits different engine binaries.
The external runner/harness and frozen game checkout must remain available in
this BAR workspace (or set `--search-root` and `--game`). No downloads occur.

From the workspace root:

```sh
python3 RecoilEngine/test/validation/shared_attack_queues/replays.py \
  --candidate /absolute/path/to/stable-plus-patch/spring-headless \
  --output /home/aron/Projects/bar/shared-attack-queues-replay-results \
  --workers 2
```

The stable default is the downloaded `2026.07.04` release, the latest stable
release checked on 2026-10-07. Build the candidate from that release plus only
the intended patch, using equivalent compiler/build settings. The existing
`401b77dba6` prototype builds are **not** stable-plus-patch candidates. The
script verifies the release version string and records binary hashes; a version
string cannot prove the candidate's source provenance. Preserve its source diff
and build command with the results. `--stable` and `--stable-tag` explicitly
select a different stable release when needed.

Each replay runs stable, stable-repeat, and candidate against the same frozen
original game and identical input replay. The old preparation helper also emits
an unused `pr-controller` directory: it is never executed. No attack commands
are translated for the three executed variants. Only the game setup wrapper
and playback timing are changed. All writes use isolated output directories;
assets come from `bar-dev/data`, never the production installation.

Re-run the same command to resume. Successful jobs require exit code zero,
the exact completion marker, no detected Lua errors, and every expected sample.
Incomplete jobs restart from the beginning; this is job-level resume, not a
saved-game checkpoint. Changed binaries, scripts, observer, replay bytes, game
commit, map archives or endpoints require a new output directory. Keep all
inputs unchanged while a matrix is running. Ctrl-C terminates active process
groups; the output lock rejects another supervisor, and orphan engines are
reported rather than overwritten. Hashing the map archive directory at startup
can take a minute.

`report.json` contains overall success, both comparisons for every replay, and
the first differing frame. Exit status is nonzero for differences, incomplete
jobs or errors. Stable-repeat must match before the overall result can pass.
`provenance.json` records inputs and native multiunit attack packet coverage.
Logs, `hashes.tsv`, `states.bin`, event traces and timing samples remain in each
job directory. State observations every 150 frames include units, weapons,
features, projectiles, resources, and full unit command queues including tags.
Damage, destruction, command admission and command completion events are
compared between samples. This is observable state parity, not a complete
internal engine checksum or a proof of all queue mutation paths. Native packet
coverage excludes batches generated inside synced Lua. A passing replay with
no native batches does not establish storage-sharing coverage.

For a quick harness check (does not validate the engine change):

```sh
python3 RecoilEngine/test/validation/shared_attack_queues/replays.py \
  --candidate replay-parity/engines/recoil-2026.07.04/spring-headless \
  --self-test --output /tmp/shared-queue-replays-smoke \
  --replay-id e0bcbf6a883a2e3ae14e64612833e410 --finish 300 --workers 2
python3 -m unittest discover \
  -s RecoilEngine/test/validation/shared_attack_queues -p test_replays.py
```

Use `--prepare-only` to inspect the inputs without launching simulations.
Omit `--replay-id` and `--finish` for all four full replays. Parallel elapsed
and profiler timings are diagnostic, not controlled performance results; for
performance work run with `--workers 1` and account for observer overhead and
repeat variability. Full state traces can occupy substantial disk space.
This runner and its checks were implemented with OpenAI Codex; automated smoke
results do not imply human verification or a completed stable/candidate matrix.

The selected corpus contains native multiunit attack batches in two recordings:
`3c6ac16a…` has three (up to 16 units and 33 attack commands per packet), and
`982fc16a…` has one (8 units, 11 attacks). `a567c16a…` and `e0bcbf6a…` have none;
they remain general replay-regression cases. These counts describe input
packets, not observed command acceptance or measured memory sharing.
The engine binary must have its matching `base/` assets beside it; their hashes
are included in resume provenance as well.
