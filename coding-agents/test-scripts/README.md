# Test start scripts for the macOS renderer

Start scripts the engine accepts directly, so a renderer change can be checked against a real game without a lobby. Kept here rather than in a scratch directory so the setup is not lost between sessions.

Run one with:

```bash
cmake --build build-macos-legacy -j 10 --target engine-legacy
cd build-macos-legacy
./run-macos.sh --isolation-dir ~/dev/spring-testdata --window ../coding-agents/test-scripts/<script>.txt
```

`--isolation-dir` is not optional. Without it the engine picks up `~/.spring`, whose `base/springcontent.sdz` is from January 2025 and is missing shaders this engine asks for, and the terrain draws black. That is a stale content problem wearing a renderer costume.

## The scripts

| script | game | what it is for |
|---|---|---|
| `empty-mod.txt` | Empty Mod | an empty game, so a loose `LuaIntro/main.lua` in the data dir is the only thing that draws. The controlled harness for Lua drawing bugs. |
| `empty-mod-valles.tdf` | Empty Mod | the same, on the SplinterFaction script's map, so a run differs in content only. Measures what the map alone costs. |
| `splinterfaction-fixed-start.tdf` | SplinterFaction 0.1.78 | a real game with a real LuaUI. Sets `side` so the faction is the same every run. |
| `metal-factions-valles.tdf` | Metal Factions v2.58 | a second real game on the same map, with an unrelated LuaUI. Sets `side=aven` so the faction is the same every run. |

`run-capped.sh` takes `SCRIPT=<path>` to pick one. **Name new ones `.tdf`, not `.txt`.** The format needs semicolons on every line and the humanize hook rejects those in a `.txt`, so a `.txt` script has to be derived with a shell redirect and cannot carry a comment explaining itself.

## Which engine and which Mesa a run actually uses

**`run-capped.sh` defaults to `run-macos-premtl4.sh`, not `run-macos.sh`.** So a run uses `spring-premtl4`, a copy of `spring` relinked against `~/dev/mesa-install-premtl4`, and not the `spring` you just built against `~/dev/mesa-install`. Your code changes do reach it, because the launcher refreshes the copy whenever `spring` is newer, but the Mesa underneath is a different build.

That distinction is easy to miss and has already produced one wrong conclusion: a packaged engine was built against `mesa-install` on the reasoning that "this is what the launcher uses", which was true of a launcher nothing runs. It also means memory figures in the plan are pre-Metal4 numbers and say nothing about rc3.

| prefix | gallium | what it is |
|---|---|---|
| `~/dev/mesa-install-premtl4` | `26.2.0-devel` | pinned before "kk: Move to Metal4 command encoding", the one every measurement uses |
| `~/dev/mesa-install` | `26.2.0-rc3` | carries the Metal4 change, which never returns render pass memory |
| `~/dev/mesa-install-staging` | | scratch, not used by any launcher |

`LAUNCHER=` overrides it, which is also how a packaged engine gets tested: point it at the `spring` launcher inside the archive.

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

`SPRING_FPS_LOG=<seconds>` makes the engine log its present rate, which is how an unattended run reports a frame rate at all.

Two flags change what a run measures, and both rewrite a constant in the installed copy and then read it back, because three results in one session were mislabelled by patches that silently did not apply.

`--luaui-off <seconds>` issues `luaui disable` that long after the freeze, which removes every widget and the probe with it. The scene stays frozen, because pause is engine state. The intervals either side of the switch are the two sides, so one run holds both.

`--shots <seconds>[,...]` installs a LuaRules gadget that calls the engine's screenshot action at those times. It is a gadget rather than a widget so it survives `luaui disable`, which is the frame that most needs proving. Files land in `<data dir>/screenshots/` and the log says which shot was taken and what the unit count was.

`--move` sends the starting unit to the middle of the map and puts the camera on it instead of pausing. **Frame rates in this mode are void**, because a live scene varies by a factor of two where a frozen one reads to 2.5%. Use it for hunting artefacts, where it shows what a frozen scene cannot: the move line, the waypoint marker, and line of sight sweeping over new ground. It also removes the faction from the framing, since a locked camera otherwise frames whatever position that faction's commander started at. Tracking is engine state, so it survives `luaui disable` and the unit keeps walking after every widget is gone.

`--loops <n>` installs the artefact amplifier, which draws that many identical `LINE_LOOP` batches a frame so batch merging goes from about 8% of frames to all of them. Score it with `minimap_score.py <log> --amp`.

`--alt` alternates the amplifier's colour by batch index, which measures a different defect with the same grid. Identical batches are what makes merging visible and exactly what hides a batch inheriting the previous one's attributes, since an inherited attribute is then the same attribute. With `--alt` a batch that takes the previous batch's colour draws a whole circle wrong, about 40 pixels against a threshold of 10. Score it with `--alt`. It varies colour only, so it cannot see an inherited texture coordinate or normal, which need a bound texture or lighting to show.

Run a different game with `GAME=<path to a .sdd> ./install-probe.sh ...` and `SCRIPT=<path to a .tdf>`.

Change the window without touching the shared config by passing `-config <file>` through to the engine. The engine rewrites whatever config it is given on exit, and a stale non-default left in `springsettings.cfg` is the shape of contaminated control that has already cost this project a session.

`run-measured.sh` wraps `run-capped.sh`, so the memory ceiling and time limit still apply, and adds focus. It brings the window to the front and then reads frontmost back to confirm it, because `osascript ... set frontmost` succeeds against a process that has no window yet and the engine has none for the first several seconds of loading. It re-asserts focus on a lost poll and voids the run above one lost poll in twenty. It cannot check occlusion, so keep the window uncovered too.

`widget_perf_probe.lua` freezes the scene at sim frame 90 and re-applies one camera state every frame, which also holds the camera against edge scroll and a knocked mouse. It runs `debug 1 0`, enabling the profiler with the on-screen overlay off, and echoes the top timers every five seconds. A frozen scene reads to about 2.5% between intervals where a live one varies by a factor of two.

**Only trust deltas measured in one run.** Absolute frame rates move about 10% between runs even frozen and focused. Mouse position is one uncontrolled variable, since hovering changes what the UI draws.

## Standalone probes

These need no engine run, so use them first where they can answer the question.

| probe | what it settles |
|---|---|
| `fill_probe.c` | the driver's fill rate, and the cost of a render pass break. A break is a full attachment store and reload at memory bandwidth, 0.266ms at 3024x1832 |
| `kk_mipmap_leak.c` | the KosmicKrisp memory leak, attached to the upstream issue |
| `batch_merge_probe.c` | whether a defect that affects `glBegin` batches also affects `glDrawArrays`. Carries a positive control, so a run that fails to reproduce the known artefact says so instead of reading as a clean result |

GL timer queries do not work here, so do not reach for them. Zink on KosmicKrisp does not advertise `GL_ARB_timer_query` and `glQueryCounter` silently returns zeros.

Build and run them against the same Mesa the engine uses. Each carries its own command in a comment at the top.

**Put the Mesa prefix ahead of Homebrew in `DYLD_LIBRARY_PATH`.** Zink needs `/opt/homebrew/lib` to find `libvulkan.1.dylib`, but Homebrew also ships its own `libEGL.dylib` and `libGL.dylib`, currently Mesa 26.2.1. On macOS `DYLD_LIBRARY_PATH` beats the path recorded in the binary, so a bare `DYLD_LIBRARY_PATH=/opt/homebrew/lib` silently runs Homebrew's Mesa however carefully the probe was linked. Use `DYLD_LIBRARY_PATH=$PREFIX/lib:/opt/homebrew/lib` and check the `GL_VERSION` a probe prints before believing its numbers. The pinned prefix reports `26.2.0-devel (git-56588ef066)` on Vulkan 1.3, Homebrew reports `26.2.1` on Vulkan 1.4.

Engine runs are not affected. `run-macos-premtl4.sh` rewrites the recorded path with `install_name_tool` and sets no `DYLD_LIBRARY_PATH` at all.

## Capturing what happened

`SPRING_PHASE_DUMP=1 SPRING_PHASE_DUMP_DIR=<dir>` writes a quarter-size PPM between draw phases, so a stray polygon can be attributed to the phase that drew it. Convert with `python3 ~/dev/macos-probes/ppm2png.py <file.ppm>`.

Note the phase dumps stop at `6-screenpost`, the last point in `CGame::Draw`. Anything that appears on screen but not in that dump is being drawn somewhere else. The engine's own screenshot action captures the finished frame and is the control for exactly that case.
