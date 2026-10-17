# AimFrom / Query compatibility audit (#3327)

Opt-in runtime heuristic for games using `Cannon`, `MissileLauncher`, or
`StarburstLauncher`. The standalone gadget calls each candidate weapon's
`QueryWeapon` and `AimFromWeapon`, normalizes script/model piece IDs, and reports
the observed pieces. It supports COB numbered and legacy names, plus generic
and numbered Lua callbacks through `UnitScript.GetScriptEnv`/`CallAsUnit`.
No PR-specific engine API is required.

This tool accompanies #3327. The audit request originated in #3317, which
shares the change from the current muzzle to the AimFrom piece for these three
weapon types. It checks that compatibility concern only: it does not fit or
validate `aimFromEstimate` tuples, nor cover every weapon type using estimates.

## Quick start for another game

1. Copy `dbg_aimfrom_audit.lua` into your game's synced gadget directory,
   commonly `LuaRules/Gadgets/` (BAR: `luarules/gadgets/`).
2. Start a **disposable local development game** with custom modoption
   `aimfromaudit=1`; enable cheats before frame 300. If your framework filters
   custom options, declare this option in the game first.
3. Search `infolog.txt` for `AIMFROM_AUDIT`. Wait for the `complete` row before
   treating the run as finished. Inspect candidate weapon rows and script paths.

Only this one gadget is needed. It is inactive without the option. It spawns
at most eight concurrent Gaia test units, holds fire, controls movement,
suppresses incoming damage to its own units, samples ages 1 and 30 frames,
then reclaims them. Script callbacks can have side effects. **Do not use in a
live game or a user-owned production installation.** Other gameplay gadgets
can interfere; lost/failed samples are reported. Restart rather than hot-reload
an active audit. The generic gadget does not remove other gadgets or quit.

## Read the output

Rows are tab-separated, prefixed `AIMFROM_AUDIT`:

- `begin`: loaded UnitDef count, game version, engine version.
- `unit`: unit name, `unaffected-type`/`candidate`/failure, candidate script path.
- `weapon`: unit, one-based slot, WeaponDef name/type, backend, age, category,
  Query piece, AimFrom piece. Piece fields include names, zero-based script IDs
  and one-based model IDs.
- `complete`: loaded UnitDef count. A completed scan can still have unknown or
  failed samples; inspect the records or use the runner's coverage checks.

Categories:

- **unaffected-type**: no weapon of the three affected types in this loaded
  UnitDef; outside this audit's three-type AimFrom/Query compatibility scope.
- **different**: distinct valid model pieces were returned. Inspect the script;
  this does not prove different gameplay, since pieces can coincide spatially.
- **same**: same valid model piece in the sampled states. **Maybe affected**, not
  proven compatible: aiming, movement, deployment and alternating muzzles can
  change later results.
- **unknown**: missing/erroring/yielding callbacks or invalid/unmapped pieces.
  Inspect manually; engine fallback is not treated as proof of compatibility.

These are two idle/construction-settling snapshots, not static script analysis,
an obstacle/terrain matrix, or a pre-PR/post-PR behavioral comparison. Loaded
definitions depend on the game's options. COB calls read the output parameter,
not the execution-status return. Lua scripts require the game's environment
accessor; games with other frameworks need to provide an equivalent accessor.

## Optional isolated BAR runner

Python 3.10+, Lua, a BAR checkout, development maps, and a runnable headless
engine are sufficient. From the engine repository root:

```sh
python3 tools/aimfrom-audit/run.py \
  --source /path/to/BAR-checkout \
  --engine /path/to/spring-headless \
  --maps /path/to/development/maps \
  --output /tmp/bar-aimfrom-audit
```

The output directory must not already exist. It creates a temporary symlink
overlay, installs the audit and BAR-specific `isolate.lua`, and runs
`Quicksilver Remake 1.24` with `forceallunits=1`. It never modifies the source
checkout, engine installation, maps or production data. Only the Lua unit
script framework and audit remain active after initialization. The harness
enables cheats, accelerates simulation and exits on completion.

Temporary overlays/game links are removed on success, failure or timeout.
Retained output includes `engine.log`, `samples.tsv`, `summary.json`, isolated
config/startscript and `manifest.json` (source revision/status, engine/tool
hashes, command, return code and output-filesystem free-space observations).
The runner rejects Lua runtime tracebacks, incomplete scans, lost/failed units
and missing age-1/30 samples. Explicit unknowns are retained. Aggregate
precedence is `different`, then `unknown`, then `same`.

Tests (also runnable without the engine/game):

```sh
lua tools/aimfrom-audit/test_audit.lua
python3 tools/aimfrom-audit/test_report.py
```

The Lua fixture runs the actual gadget against mocked APIs, covering opt-in and
cheat gates, all three weapon types, mixed slots, COB zero/aliases/status/errors,
Lua dispatch/indexing, failed/lost units, damage protection and cleanup.
The Python tests cover aggregation and rejection of incomplete reports.

## BAR sample results (2026-10-18)

Two isolated runs with engine `2026.09.01`, clean BAR revision
`e867d6678c737c4acc924d47358c6bcad366ed99` and `forceallunits=1` produce identical
reports. All 2,001 loaded definitions are covered: 1,203 have no affected types,
798 are candidates. All candidates survive both samples; both runs exit 0
without Lua runtime tracebacks.

| Weapon type | Different | Same/maybe | Unknown | Total |
| --- | ---: | ---: | ---: | ---: |
| Cannon | 553 | 171 | 102 | 826 |
| MissileLauncher | 241 | 67 | 44 | 352 |
| StarburstLauncher | 60 | 23 | 30 | 113 |
| **Total** | **854** | **261** | **176** | **1,291** |

These include generated variants, auxiliary/dummy weapons, 1,257 COB slots and
34 Lua slots. With the above precedence, unit categories are 584 different,
108 same/maybe and 106 unknown. These are **not counts of broken units**.
For example, `armrock` slot 1 returns `firepoint` versus `aimpoint`; `armstump`
slot 1 returns `flare` versus `sleeve`. `armbanth` slot 3 returns the same
`missleflare` piece, while its auxiliary Cannon slot 4 has missing callbacks
and remains unknown. Manually exercise relevant aiming/firing states before
deciding whether a script needs changes.

AI disclosure: OpenAI Codex assisted with implementation, tests and this guide.
Automated validation is described above; it is not a claim of human gameplay
verification or behavioral compatibility.
