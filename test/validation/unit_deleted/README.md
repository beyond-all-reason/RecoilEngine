# UnitPreDelete architecture prototype

This is an uncommitted experiment based on Recoil PR #3454 at
`916b59dc1d8f8bd1b9a349eb53631bd8c8ef96b6`, not a published replacement API.

## Lifecycle contract

`UnitDestroyed` and the existing unsynced `RenderUnitDestroyed` remain in their
existing locations. At the final deletion boundary, the unit's incoming death
dependencies are notified explicitly before its destructor. Synced, full-read
clients then receive `UnitPreDelete(unitID, unitDefID, unitTeam)` while the unit
remains readable through the normal synced read API. ID invalidation, entity
destruction and memory release follow the callin.

New native object-target commands, including CMD.INSERT, are rejected once
notification begins. New death dependencies and explicit attack/weapon targets
are also rejected. LuaSyncedCtrl normally treats a finalizing unit as unavailable
for mutation. Single-unit order APIs return false for orders to a finalizing
unit instead of reporting an invalid ID. Recursive self-deletion is ignored;
nested deletion of another unit must not invalidate the outer callback's unit.

Incoming notifications run at most once. Outgoing dependency teardown remains
in the destructor. The standalone ObjectFinalization unit tests exercise these
boundaries. Neutral-target removal remains a separate callin and code path.

## Focused validation

Run this same runner against the original PR binary with `--callin UnitDeleted`,
and against the prototype with its default `--callin UnitPreDelete`:

```sh
python3 test/validation/unit_deleted/run.py \
  --engine /path/to/spring-headless \
  --game /path/to/unmodified/development/BAR \
  --maps /home/aron/Projects/bar/bar-dev/data/maps \
  --output /path/to/new/isolated/results
```

Use `--compare /path/to/baseline/trace.json` to require identical frame-by-frame
native position, velocity, heading, command queues and command-completion events.
`--fire-at-killed` enables the corresponding modrule; `--ground` replaces the main
fighter/bomber pair with a tank/turret pair. Supply the same map and options to
both sides. Generated game overlays and map links are removed after each run.

The fixture also checks valid IDs and readable metadata, native queue cleanup
before the lifecycle callin, virtual-list pruning, rejected new orders/targets,
self-deletion, nested deletion with ID reuse, affected-unit-only neutralization,
returned command-tag erasure and absence of unsynced lifecycle delivery.

The separate `run_compact.py` fixture exercises the real BAR #8935 runtime from
commit `ed615ccc55126d032ba9c70cdb2e547e0a01bd51`. Its gadget handler and controller
receive only the mechanical `UnitDeleted` to `UnitPreDelete` rename; the target
store and Set Target gadget are unchanged. Other game assets come from the frozen
development controller checkout `958be567af54ab616c7c9649d5cea5bfeb2988b0`.
Both compared engines include the same command-registration dependency from
#3431 at `210580878cb1f5b4a6de6bfc0268d77d72a1e73e`. This is a focused integration,
not a full-current-BAR checkout or replay benchmark.
Moving notifications ahead of destruction also moves them ahead of wreck creation,
unblocking and subsystem teardown. Ground-pathing, transports, construction,
rendering, save/load and other games therefore still need broader validation.

OpenAI Codex assisted implementation and automated validation. Human gameplay
verification and API review are pending. No commits or remote PR updates were made.

## Initial observations, 2026-10-13

- The isolated Linux RELWITHDEBINFO headless build succeeds. ObjectFinalization
  passes all four test cases / twelve assertions. `git diff --check` is clean.
- The final `guarded-air`, `guarded-fire-at-killed` and `guarded-ground` runs pass
  every lifecycle assertion, including readable IDs, recursive deletion,
  nested ID reuse, forbidden commands/targets and neutral-target removal.
- Each final run matches its baseline's 90 per-frame native movement and queue
  observations, and target-removal timing. Ground and fireAtKilled also match
  the complete event trace.
- The final ordinary aircraft run does NOT pass complete trace equality: two
  early UnitCmdDone events (ATTACK and STOP) occur at frame 3 instead of frame 2.
  An earlier repeat with the same candidate binary matched the baseline exactly;
  another fireAtKilled comparison showed the opposite 3-to-2 shift. The tested
  target removal and subsequent completion remain at frames 31 and 45. The
  cause was not isolated in this initial investigation; see the follow-up below.
- These initial tests use a minimal virtual list, not BAR #8935's compact controller.
  No full replay, graphical gameplay, other-game, save/load or performance
  validation was performed.

Evidence is retained in
`/home/aron/Projects/bar/attack-comparison/unit-predelete-prototype-20261013/`:
`comparison.json`, per-run logs/results/traces, build scripts/logs and binary
hashes. Failed exploratory runs are retained as well; their errors must not be
confused with the final lifecycle results. Test overlays have been removed.

Workspace filesystem available space decreased from 184,309,305,344 to
177,675,186,176 bytes during this experiment (6,634,119,168 bytes, approximately
6.18 GiB). This is a filesystem-wide free-space measurement, not a `du` total or
an exclusively attributable allocation count. The separate build is retained.
The attempted reflink copy on tmpfs was unsupported; its incomplete destination
was removed and a clean separate build was used, without a full-copy fallback.

## Follow-up: startup event timing

The extra completions are caused by external UI input, not a change to the
deletion boundary. `LuaUI=0` in the old settings did not disable BAR's widgets.
The enabled `luaui/Widgets/cmd_holdfire_fix.lua` reacts to the fixture's Hold Fire
command by sending an unsynced `CMD.INSERT {0, CMD.STOP, 0}`. Its arrival at frame
2 or 3 finishes the current Attack for front insertion and then finishes STOP.
`unit_set_fighters_fly.lua` also sends an unsynced IDLEMODE command.
The diagnostic log records these exact orders, tags and completion parameters.

The corrected harness overlays an inert `luaui.lua` and verifies that no widgets
load. `--with-ui` retains the previous environment for reproducing the issue.
No completion events are filtered from the original lifecycle comparison.
All 18 corrected runs (three repetitions of each engine for aircraft, ground
units and fireAtKilled) pass lifecycle checks and complete trace equality,
including command completion timing. Their evidence is in `isolated-*`.

## Real controller comparison

For a fresh output directory, run:

```sh
python3 test/validation/unit_deleted/run_compact.py \
  --engine /path/to/engine-with-3431/spring-headless \
  --game /path/to/frozen/development/controller-game \
  --bar-repo /path/to/BAR/repository \
  --maps /home/aron/Projects/bar/bar-dev/data/maps \
  --callin UnitPreDelete --compact --scenario active \
  --output /path/to/new/results
```

Use `--callin UnitDeleted` for the baseline and omit `--compact` for ordinary
native Attacks. `--ground` and `--fire-at-killed` select the other variants;
`--scenario pending|neutral|recycle` exercises the corresponding lifecycle case.
`--scenario combat` enables real weapons and natural target destruction, with
600 aircraft frames or 1,200 ground frames (including the approach). Other cases
run for 150 frames with weapons held in reload. `--frames` overrides the duration.
The fixture asserts that compact references actually occur, so silent fallback
to native commands cannot count as a passing controller test.

`behavior.json` records exact nine-decimal per-frame position/velocity, heading,
active command/parameters, weapon target, native Attack/Move completions and
lifecycle timing. `raw.json` additionally retains all command completions,
queue entries, tags and option bits. Raw queues are compared between engines
within the same representation, not between native lists and compact references.
The native-vs-compact behavior comparison excludes only representation-specific
tags/options and internal custom-command completions; they remain in raw data.

The initial neutral fixture incorrectly required `Spring.SetUnitTarget` to return
true even for distant targets. `CUnit::AttackUnit` assigns its current target and
death dependence before weapon `TryTarget` acceptance, so a false weapon result
does not mean neutralization has no affected unit. The corrected fixture instead
asserts the actual affected-unit callback, the pruned next target for that unit,
and retention of the same target for the unaffected unit. Original failed runs
are retained in `integration-*-neutral-*`; corrected runs use `followup-*`.

Ground live-combat trials initially retained the generic fixture's elevated
target placement and then ran too briefly to include a kill after approach.
Neither qualified as passing combat coverage. The final ground combat places
turrets on terrain and runs 1,200 frames; earlier trials remain in `followup-*`
and `live-*`, and final ground combat evidence uses `combat-final-*`.

Aircraft live combat exposes an intentional representation difference: between
two materialized Attacks the compact queue can have an ATTACK_TARGETS reference
at its head where the native queue has the next Attack. `behavior.json` retains
this difference rather than silently normalizing it away. Position, velocity,
heading, actual weapon target and the complete non-frame event sequence are
also compared separately. Thus native-vs-compact queue-API identity and physical
combat parity are explicitly different claims.

## Final follow-up results, 2026-10-13

All 15 integration cases pass in all four configurations (original/native,
original/compact, pre-delete/native, pre-delete/compact): **60 passing runs**.
Each case uses two attackers and three targets, fixed RNG seed 1, and isolated
inputs. Aircraft, ground and fireAtKilled each cover active-target deletion,
pending-target deletion, affected-unit-only neutralization, pending-ID reuse
and live combat.

| Comparison | Result |
| --- | --- |
| Original versus pre-delete, native orders | All behavior and raw traces exactly equal |
| Original versus pre-delete, real compact controller | All behavior and raw traces exactly equal |
| Native versus compact movement, heading and weapon targets | Exactly equal in all 15 cases |
| Native versus compact non-frame events, including shots/damage/deletion | Exactly equal in all 15 cases |
| Native versus compact sampled command-queue head | Equal in 13 cases; expected reference/Attack differences in both aircraft combat cases |

Each aircraft combat comparison has 10 differing queue-head observations out of
1,200 per-unit observations. The same differences exist on the original engine;
the pre-delete architecture introduces none. Each 600-frame aircraft combat run
records 20 projectiles, 17 damage events and one target deletion. The 1,200-frame
ground run records 25 projectiles, 18 damage events and two target deletions.
These event counts and their individual times/values match, not merely totals.

Together with the 18 isolated lifecycle repetitions and the four C++ test cases
(12 assertions), this supports the explicit pre-deletion boundary as a viable
behavior-preserving replacement for the tested #3454/#8935 use cases. It does
not establish identical native queue representation, exhaustive gameplay parity,
multiplayer compatibility between different builds, or merge readiness.
Construction/transports, broader pathing scenarios, graphical clients, save/load,
other games and long replays still need review and validation.

Authoritative follow-up evidence is `verification.json` and
`integration-comparison.json` in the experiment directory. The old
`comparison.json` intentionally retains the earlier UI-contaminated comparisons.
Recompute the complete verdict with:

```sh
python3 /home/aron/Projects/bar/attack-comparison/unit-predelete-prototype-20261013/compare-integration.py --require-complete
```

`registration-dependency.patch` records the common #3431 addition;
`stacked-binary-hashes.json`, runtime-file hashes and observer snapshots retain
provenance. Both extra build/source copies use successful mandatory reflinks;
their engine-code difference is exactly the prototype, not different dependency
versions. No additional engine behavior fix was needed during this follow-up.
All temporary game overlays and map links are removed. Builds, scripts, traces,
patches and exploratory failures remain for review. No commits or pushes were
made, and neither the main checkouts nor the production installation was changed.

During this follow-up, filesystem available space changed from 177,671,163,904
to 172,599,013,376 bytes: a decrease of 5,072,150,528 bytes (about 4.72 GiB).
This is an observed filesystem-wide change, not a `du` total and not an exclusive
attribution to this task; reflink-shared build totals must not be added together.
