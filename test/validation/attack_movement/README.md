# Attack movement validation

Use an isolated development data directory, a headless build, BAR with the
`AttackCommandMovement` handler and `unit_attack_movement.lua`, and development
map assets. Never point these tests at a normal installed game directory.
Copy `check.lua` into the test game's `luarules/gadgets/` directory.

## Replay comparison

For per-frame comparisons, install `parity.lua` as a gadget instead of `check.lua`.
Use a frozen engine from the PR's parent and a separately stored candidate engine,
each with its own `base/` assets. Prepare four roots using the same historical BAR
checkout and identical handler/policy files: parent (`--mode native`), candidate
without a handler (`native`), candidate `fallback`, and candidate `lua`.
`prepare.py` disables LuaUI in the isolated game to prevent widgets from changing
orders or garbage-collection settings during accelerated runs.

`parity.lua` records SHA-512 digests every frame of sorted unit IDs, definitions,
teams, positions, velocities, headings, health/build progress, experience, complete
command queues, weapon aim/reload/salvo/target state, and projectile IDs,
definitions, positions and velocities. It packs the Lua-visible float values in
binary and reuses its buffer to limit garbage. This is a broader observable-state
comparison, not a checksum of every internal simulation field or Lua variable.

Run `compare.py MANIFEST.json`; the manifest has this shape:

```json
{
  "frames": 36000,
  "parity": true,
  "runs": [
    {"name": "parent", "engine": "/tmp/parent/spring-headless", "root": "/tmp/parent-run"},
    {"name": "native", "engine": "/tmp/candidate/spring-headless", "root": "/tmp/native-run"},
    {"name": "fallback", "engine": "/tmp/candidate/spring-headless", "root": "/tmp/fallback-run"},
    {"name": "lua", "engine": "/tmp/candidate/spring-headless", "root": "/tmp/lua-run"}
  ]
}
```

The runner rejects missing/duplicate frames and Lua failures, reports the first
difference, and writes a `.results.json` alongside the manifest with binary and
policy hashes. `--read-only` compares existing logs. Engine `--only-local` avoids
listening-port conflicts when independent replay comparisons run in parallel.

For performance, prepare fresh roots with `prepare.py --benchmark`, set manifest
`parity` to false, and run at least three interleaved repetitions of each variant.
Do not run other engine tests or builds concurrently. The observer then only
records frame timings; it does not collect or hash simulation state. Timings
measure wall time between frame callbacks, including shared observer/logging
overhead, rather than the exclusive time in attack movement. Mean and p95 omit
the first 300 frames; elapsed time excludes engine/game loading.

For controlled policy coverage, install `branches.lua` alongside `parity.lua`
and use `branches.txt`, whose simulation seed and game ID are fixed.
Override `attackmovementmode` for each variant.
Place an empty `luaui.lua` at the root of each isolated test game: widgets must
not issue orders asynchronously during this live-game fixture. The `LuaUI=0`
config key does not disable loading LuaUI in this engine. Repeat the parent run
to establish fixture reproducibility before comparing candidate results.
Set each manifest run's `input` to the start-script filename instead of the
default `input.sdfz`. The 22 cases cover unit/ground orders, clear and blocked
shots, range, hold position, chasing, strafing, gunships, different weapon
families, multiple weapons, manual fire, paralyzed targets and fight orders.
They exercise representative scenarios, not exhaustive branch coverage.

The older sampled comparison remains available through `check.lua`:

Check out the BAR revision matching the replay and copy the handler changes and
**the identical policy gadget** from the BAR companion PR into that checkout.
`prepare.py` supports demo format 5 and creates an isolated directory archive,
rewrites the game name/modoptions, accelerates packet delivery and drops old
engine sync-response packets. It hardlinks game files: keep the source checkout
unchanged during all runs. It does not alter the original demo or engine.

```sh
python3 test/validation/attack_movement/prepare.py REPLAY OUTPUT \
  --bar TEST_BAR --assets DEVELOPMENT_DATA --engine ENGINE \
  --mode native --frames 36000
python3 test/validation/attack_movement/run.py ENGINE OUTPUT OUTPUT/input.sdfz
```

Repeat with separate output directories and `--mode lua` and `--mode fallback`.
Compare every `[AttackValidationSnapshot]` payload. Each hashes sorted unit IDs,
positions and current command ID/options/tag every 1,800 frames. Matching
snapshots are a movement regression check, not a complete simulation checksum
or a performance benchmark. A complete run must print `[AttackValidation] PASS`.

## Blocker/filter matrix

Prepare an isolated game directory as above. Install Quicksilver Remake 1.24
in the development maps directory and run `blockers.txt` instead of the demo:

```sh
python3 test/validation/attack_movement/run.py ENGINE OUTPUT \
  test/validation/attack_movement/blockers.txt
```

Expected: `PASS blockers=86`. The test controls visibility, reloads and collision
volumes, then checks unit and ground attacks for beam, cannon, straight missile,
curved missile and starburst weapons. Cases cover clear, friendly, neutral,
feature, terrain and simultaneous terrain/friendly obstruction. Starburst's
native test checks its initial vertical cone, so its terrain cases are omitted.
Additional cases cover static allies and mixed mobile/static allies in both
creation/spatial orders. Each source identity must match the actual blocker;
clear, range, terrain and unchecked rotation must not retain a previous source.

Each callback compares unfiltered, all-categories, friendly-only and terrain-only
queries and verifies the weapon's stored flags and later default query do not
change. It also rejects invalid weapon indices, queries for another unit and
queries outside the callback. The runner rejects missing completion, failed
assertions and callback errors.

Candidate-position checks compare the current position/native heading with the
heading query, test a distant out-of-range position and both pre-aim/muzzle modes,
and verify restoration of position, heading, weapon vectors and default queries.
Narrow blockers provide explicitly blocked-current/clear-candidate cases for
sidestepping. This checks shot geometry, not candidate path reachability.
Clear beam/cannon cases also put the candidate source underground and confirm
terrain rejection with default checks and a clear pre-aim query with `noGround`,
covering the interaction with the merged #3328 checks.

The query-only `noMobileFriendlies` and `noStaticFriendlies` masks subdivide
allies by UnitDef movement capability. Existing masks/default weapon flags keep
their previous meaning. Scripts can query with just mobile allies remaining,
then separately test buildings/terrain; a first blocker ID alone cannot answer
whether both kinds obstruct the shot.
The matrix also verifies that previously unused high bits in stored avoidance
flags do not opt native/default queries into the new filters. Actual firing
raytraces do not receive the query-only filtering argument.

For interactive review, use the same placements and masks with a graphical
engine. Compare `attackmovementmode=lua` with the unset option and `fallback`.
The equivalent BAR policy deliberately retains the native 90% range behavior;
these tests do not claim that all behavior reports in #3331 are fixed.

## Command lifecycle

Also copy `ended.lua` into the isolated game's gadget directory, then run
`ended.txt` through `run.py`. Expected: `PASS ended=10`. Cases cover completion,
explicit removal, target death, replacement, front insertion, WAIT suspension and inactive queued
removal. Empty queues and WAIT successors must stop when the callback clears the
old goal; MOVE successors must remain queued and continue. The callback checks
the old tag and reason and observes the successor before it executes.

`UnitCommandEnded` supplements `UnitCmdDone`; it does not change its legacy
notification contract or install a default stop policy. `completed` means the
CAI finished the command, not necessarily that its gameplay objective succeeded.
The hook covers shared FinishCommand and explicit queue editing paths; internal
CAI subcommand insertion is not a general suspension/resumption notification.

## Dispatch ownership and invalidation

Install `dispatch_rules.lua` and `dispatch_later.lua` in the isolated game's gadget directory and
`dispatch_gaia.lua` as that game's `LuaGaia/main.lua`. Quicksilver has map-side
LuaGaia entry points; the mod-first loader selects this opt-in fixture. Run
`dispatch.txt` through `run.py`. Expected: `PASS dispatch=6`. The fixture checks
that true stops dispatch across LuaRules/LuaGaia, false/nil reaches Gaia once,
and replacing the command or destroying its target/owner prevents further
dispatch, both to a later LuaRules gadget and to LuaGaia. `dispatch_later.lua`
uses BAR's ascending layer order. Games with their own handler must carry the
invalidation guard from the engine's `gadgetHandler:AttackCommandMovement`;
checking only the literal true return is insufficient. The candidate and
movement APIs reject a context invalidated by Lua.
Remove the fixture from any game intended for actual play.

## Manual review and remaining parity gate

In an isolated graphical game with the companion handler enabled, place a
stationary shooter, an enemy target and an allied building between them. Inspect
the current-position result and two candidate positions with
`TestUnitAttackMovementPosition` inside the movement callback. Confirm the
building's ID and a clear candidate, then repeat with a mobile ally, both
blockers, terrain, ground attacks, and a cannon/curved missile. Querying alone
must not move the shooter or rotate its model. Enable a BAR movement decision
only in a separate policy change, after the identical Lua policy passes parity.

Neither replay digests nor these focused tests certify 100% identical gameplay.
Record the frozen parent/no-call-in/false-fallback/Lua results, replay identities,
binary/policy hashes and timing repetitions for the revision being reviewed.
Human API review and interactive gameplay verification remain necessary.
OpenAI Codex assisted the API implementation, fixtures and documentation;
automated headless checks do not substitute for human gameplay verification.
