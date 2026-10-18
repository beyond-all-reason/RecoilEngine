# Test start scripts for the macOS renderer

Start scripts the engine accepts directly, so a renderer change can be checked against a real game without a lobby. Kept here rather than in a scratch directory so the setup is not lost between sessions.

Run one with the capped runner, after checking `memory_pressure`:

```bash
cmake --build build-macos -j 10 --target engine-legacy
SCRIPT=coding-agents/test-scripts/<script>.tdf coding-agents/test-scripts/run-capped.sh 120 /tmp/run.log
```

`run-capped.sh` runs `./spring` in `build-macos` with `--isolation-dir ~/dev/spring-testdata --window`. It kills the engine above `MAX_RSS_KB` of phys_footprint (default 11010048, 10.5 GiB) and after the time limit. It checks footprint every 0.5 s and writes a full rss, top and footprint line to `<logfile>.mem` about every 4 s.

`--isolation-dir` is not optional, and `run-capped.sh` passes it for you. Without it the engine picks up `~/.spring`, whose `base/springcontent.sdz` is from January 2025 and is missing shaders this engine asks for, and the terrain draws black. That is a stale content problem wearing a renderer costume.

## The scripts

| script | game | what it is for |
|---|---|---|
| `empty-mod.txt` | Empty Mod | an empty game, so a loose `LuaIntro/main.lua` in the data dir is the only thing that draws. The controlled harness for Lua drawing bugs. |
| `empty-mod-valles.tdf` | Empty Mod | the same, on the SplinterFaction script's map, so a run differs in content only. Measures what the map alone costs. |
| `splinterfaction-fixed-start.tdf` | SplinterFaction 0.1.78 | a real game with a real LuaUI. Sets `side` so the faction is the same every run. |
| `metal-factions-valles.tdf` | Metal Factions v2.58 | a second real game on the same map, with an unrelated LuaUI. Sets `side=aven` so the faction is the same every run. |
| `bar-benchmark.tdf` | Beyond All Reason pr-8942-30929-09be885 | BAR's 650-unit benchmark on Starwatcher 1.0. Needs that game and map in the data dir. The result prints in the log after "Benchmark Results". Disable BAR's "Mission Info" widget first, see the header of the file. |

`run-capped.sh` takes `SCRIPT=<path>` to pick one. **Name new ones `.tdf`, not `.txt`.** The format needs semicolons on every line and the humanize hook rejects those in a `.txt`, so a `.txt` script has to be derived with a shell redirect and cannot carry a comment explaining itself.

## Which engine and which Mesa a run actually uses

The engine loads Mesa itself, so `run-capped.sh` runs `./spring` directly. It looks for Mesa in `<exe dir>/mesa` first, so `build-macos/mesa` is a symlink to a Mesa install prefix and repointing it picks the Mesa build for the next run. See `coding-agents/MACOS_BUILD.md` for the lookup order and the environment variables the engine sets.

Use Mesa 26.3.0-rc1 (`~/dev/mesa-install-26.3.0-rc1`, unpatched). Do not use 26.2.5, which leaks during load and reached a 9.7 GiB footprint 15 s into SplinterFaction.

`LAUNCHER=` overrides `./spring`, which is also how a packaged engine gets tested: point it at the `spring` launcher inside the archive.

## Rules that cost time to learn

**Set `side`.** Without it a game with factions picks one at random per run, so two runs differ in commander model, unit set and the overlays drawn around the selection. That confounded a compiled-versus-deferred display list comparison on 2026-08-04, where the two crops being compared turned out to be two different commanders. The value is the `name` field from the game's `sidedata.lua`, not the `startunit`.

**`startpostype=0`.** Fixed start positions. Anything else lands in a placement screen, which for SplinterFaction is its own LuaUI screen rather than the engine's selector, so it is a different code path and a poor thing to measure against.

**`gametype` is the modinfo `name`, not the `game` field.** For `empty_mod.sdz` that is `Empty Mod`.

**The archives must be findable.** `~/dev/spring-testdata/games/` symlinks to whatever is in `~/.spring/games/`, and the map lives under `~/dev/spring-testdata/maps/`. A version bump in an installed game breaks the `gametype` line here.

## Games available to test against

SplinterFaction and Metal Factions both run, and their LuaUIs have little in common, so a LuaUI finding no longer rests on one game's content.

Metal Factions needs unpacking. It ships as `metal_factions-v2.58.sdz` and the probes have to be written into the game, so it lives beside SplinterFaction as `metal_factions-v2.58.sdd`, 141M unpacked. Point the installer at it with `GAME=`.

MCL is installed and unverified. XTA and Balanced Annihilation may not run either. Confirming which of them start is worth doing on its own, before any of them is used as a control.

## Measuring

Three scripts, in the order you use them.

```bash
./install-probe.sh                      # freeze the scene, log the profiler
./run-measured.sh 60 ~/dev/spring-testdata/logs/run.log
./install-probe.sh --remove             # leave the game unmodified
```

Two flags change what a run measures, and both rewrite a constant in the installed copy and then read it back, because three results in one session were mislabelled by patches that silently did not apply.

`--luaui-off <seconds>` issues `luaui disable` that long after the freeze, which removes every widget and the probe with it. The scene stays frozen, because pause is engine state. The intervals either side of the switch are the two sides, so one run holds both.

`--shots <seconds>[,...]` installs a LuaRules gadget that calls the engine's screenshot action at those times. It is a gadget rather than a widget so it survives `luaui disable`, which is the frame that most needs proving. Files land in `<data dir>/screenshots/` and the log says which shot was taken and what the unit count was.

`--move` sends the starting unit to the middle of the map and puts the camera on it instead of pausing. **Frame rates in this mode are void**, because a live scene varies by a factor of two where a frozen one reads to 2.5%. Use it for hunting artefacts, where it shows what a frozen scene cannot: the move line, the waypoint marker, and line of sight sweeping over new ground. It also removes the faction from the framing, since a locked camera otherwise frames whatever position that faction's commander started at. Tracking is engine state, so it survives `luaui disable` and the unit keeps walking after every widget is gone.

Run a different game with `GAME=<path to a .sdd> ./install-probe.sh ...` and `SCRIPT=<path to a .tdf>`.

Change the window without touching the shared config by passing `-config <file>` through to the engine. The engine rewrites whatever config it is given on exit, and a stale non-default left in `springsettings.cfg` is the shape of contaminated control that has already cost this project a session.

`run-measured.sh` wraps `run-capped.sh`, so the memory ceiling and time limit still apply, and adds focus. It brings the window to the front and then reads frontmost back to confirm it, because `osascript ... set frontmost` succeeds against a process that has no window yet and the engine has none for the first several seconds of loading. It re-asserts focus on a lost poll and voids the run above one lost poll in twenty. It cannot check occlusion, so keep the window uncovered too.

`widget_perf_probe.lua` freezes the scene at sim frame 90 and re-applies one camera state every frame, which also holds the camera against edge scroll and a knocked mouse. It runs `debug 1 0`, enabling the profiler with the on-screen overlay off, and echoes the top timers every five seconds. A frozen scene reads to about 2.5% between intervals where a live one varies by a factor of two.

**Only trust deltas measured in one run.** Absolute frame rates move about 10% between runs even frozen and focused. Mouse position is one uncontrolled variable, since hovering changes what the UI draws.

GL timer queries do not work here, so do not reach for them. Zink on KosmicKrisp does not advertise `GL_ARB_timer_query` and `glQueryCounter` silently returns zeros.
