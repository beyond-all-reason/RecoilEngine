# PBR IBL smoke launch: user guide

`run-pbr-ibl-smoke.sh` is a Windows Git Bash launcher for the PBR IBL branch. It runs a BAR checkout that contains the PBR IBL widgets on a chosen Recoil runtime, in an isolated write directory, without touching the normal BAR install. It does not require fixed checkout names or a shared parent directory when explicit paths are supplied.

This guide covers all four tools in this folder that belong to it:

| File | Kind | What it does | Started by |
|---|---|---|---|
| `run-pbr-ibl-smoke.sh` | bash, entry point | prepares the isolated runtime and launches the game, the self-test or the input probe | you, VS Code tasks |
| `pbr-ibl-startscript.txt` | start script | the game to start: Onyx Cauldron 2.2.2, one player, one SimpleAI | the launcher (last engine argument) |
| `pbr-window-to-back.ps1` | PowerShell helper | keeps an unattended game window at the bottom of the window stack | the launcher, background window mode |
| `pbr-viewer-input-probe.ps1` | PowerShell helper | drives the viewer with real Windows mouse and keyboard input and checks the result | the launcher, `--input-probe` |
| `split-debug-symbols.sh` | bash | moves a Debug build's DWARF symbols into `spring.dbg` so Windows can run `spring.exe` | you, after a Debug build |

Each script's header repeats its own usages, expected output and how to extend it.

## 1. Quick start

From the Recoil worktree, in Git Bash:

```bash
cmake --build build/window --target spring
cmake -P build/window/cont/cmake_install.cmake
tools/scripts/split-debug-symbols.sh --runtime-directory build/window
tools/scripts/run-pbr-ibl-smoke.sh --self-test
grep -E '^(PASS|FAIL|KNOWN|GAP|SUMMARY)' test-results/pbr-ibl/pbr-ibl-selftest/report.txt
```

The third line is only needed for Debug builds. The launcher prints what it resolved, then the game runs in a window behind your other windows and quits by itself when the self-test is done (about two minutes for a Debug build):

```text
Engine:  /c/.../RecoilEngine-pbr-rendering-experimental-V1/build/window/spring.exe
Game:    /c/.../Beyond-All-Reason-Private-pbr-rendering-experimental-V1.sdd
Maps:    /c/.../BARS-Install/data/maps
Script:  /c/.../RecoilEngine-pbr-rendering-experimental-V1/tools/scripts/pbr-ibl-startscript.txt
Write:   /c/.../RecoilEngine-pbr-rendering-experimental-V1/test-results/pbr-ibl
Cursor:  software
Window:  background 1280x720 at 40,40
```

The last report line summarises the run, for example `SUMMARY pass=145 fail=0 known=1 gap=3 info=71`.

## 2. Required inputs

- A Recoil runtime containing `spring.exe`, installed runtime files, fonts, and base content.
- A BAR checkout containing `luaui/Widgets/dbg_pbr_ibl_viewer.lua` and `luaui/Widgets/dbg_pbr_ibl_selftest.lua`.
- A directory containing the test map named by `pbr-ibl-startscript.txt` (Onyx Cauldron 2.2.2).

For an in-tree Recoil build, build and install the runtime first:

```bash
cmake --build build/window --target spring
cmake -P build/window/cont/cmake_install.cmake
```

The second command is Recoil's own install rule (`cont/CMakeLists.txt`): it copies the MinGW DLLs, `fonts/`, the base archives and stock `LuaUI/` content beside `spring.exe`. Without the DLLs Windows stops before `main` with `0xc0000135`; without the fonts BAR's `FontFile = FreeSansBold.otf` fails. The launcher checks for both and prints the install command when they are missing.

Debug executables larger than 2 GiB must have their symbols split before Windows can run them. Use the tracked helper (or an equivalent `objcopy` flow):

```bash
tools/scripts/split-debug-symbols.sh --runtime-directory build/window
```

The helper discovers `objcopy`, `objdump`, and `strip` from `PATH`, falls back to `C:/mingw64/bin`, and accepts `PBR_OBJCOPY`, `PBR_OBJDUMP`, and `PBR_STRIP` overrides. See section 9.

## 3. Common tasks, with examples

### 3.1 Look at the viewer by hand

```bash
tools/scripts/run-pbr-ibl-smoke.sh --window-mode foreground
```

The game opens in front, activated, and stops on the pregame start-position screen. Open the viewer with `/luaui enablewidget PBR IBL and Cubemap Viewer` in chat. Quit the game normally; the launcher returns with the engine's exit status. Without `--window-mode foreground` the window opens behind your windows (section 5), and the z-order helper sends it back to the bottom every second even if you bring it forward from the taskbar, so use `foreground` whenever you want to watch or play.

### 3.2 Run the automated self-test

```bash
tools/scripts/run-pbr-ibl-smoke.sh --self-test
```

The launcher arms BAR's "PBR IBL Self-Test" widget (`PBRIBLSelfTest = 1`, `PBRIBLSelfTestQuit = 1`), the widget runs, writes `<write-dir>/pbr-ibl-selftest/report.txt` (also in `infolog.txt` with the prefix `[PBR IBL SelfTest]`) and quits the game. The flags are written on every launch, so a later normal launch is never armed by accident.

The ordinary `--self-test` injects UI events directly into the viewer. It does not send Windows keyboard/mouse events, move your cursor, or take keyboard ownership from another widget. You may use your desktop while it runs; avoid changing the test game's camera or settings, which can still affect image-based checks.

### 3.3 Keep one run's results apart (measurements)

```bash
tools/scripts/run-pbr-ibl-smoke.sh --self-test --write-directory test-results/pbr-perf-sky-only-256
```

A new write directory starts from the config template and an archive cache copied from the newest earlier run beside it (section 6), so nothing else carries over. A relative `--write-directory` is relative to the current directory.

### 3.4 Pin settings for a run (config template)

```bash
cp .vscode/springsettings-debug.cfg /c/temp/vsync-off.cfg
echo "VSync = 0" >> /c/temp/vsync-off.cfg
tools/scripts/run-pbr-ibl-smoke.sh --self-test --config-template /c/temp/vsync-off.cfg --write-directory test-results/pbr-vsync-off
```

The template is copied only when the write directory has no `springsettings.cfg` yet, so use a new write directory. The launcher's own keys (section 6) are applied on top on every launch. Without the option the default template is the worktree's `.vscode/springsettings-debug.cfg` when it exists, otherwise an empty file.

### 3.5 Prepare without launching

```bash
tools/scripts/run-pbr-ibl-smoke.sh --prepare-only
```

This validates the paths, creates the managed game and maps junctions, and writes the isolated settings, then prints `Prepared; not launching (--prepare-only).` The VS Code `build` task ends with this step.

### 3.6 Another checkout, runtime or location

```bash
tools/scripts/run-pbr-ibl-smoke.sh \
  --engine /c/path/to/recoil-runtime/spring.exe \
  --runtime-directory /c/path/to/recoil-runtime \
  --game-directory /d/checkouts/Beyond-All-Reason-Private.sdd \
  --maps-directory /e/games/BAR/maps \
  --write-directory /c/temp/pbr-ibl-user
```

The equivalent environment variables are:

```bash
export PBR_ENGINE=/c/path/to/recoil-runtime/spring.exe
export PBR_RUNTIME_DIRECTORY=/c/path/to/recoil-runtime
export PBR_GAME_DIRECTORY=/d/checkouts/Beyond-All-Reason-Private.sdd
export PBR_MAPS_DIRECTORY=/e/games/BAR/maps
export PBR_WRITE_DIRECTORY=/c/temp/pbr-ibl-user
tools/scripts/run-pbr-ibl-smoke.sh --self-test
```

Paths may be written as `/c/x`, `C:/x` or `C:\x`. An option overrides its environment variable.

When no BAR path is supplied, the launcher first checks the expected experimental sibling name (`Beyond-All-Reason-Private-pbr-rendering-experimental-V1.sdd` next to the Recoil checkout) and then accepts exactly one sibling `Beyond-All-Reason-Private*.sdd` checkout containing both PBR IBL widgets. Ambiguous discovery fails with an actionable message instead of choosing an arbitrary checkout. When no maps path is supplied, it uses a sibling `BARS-Install/data/maps`, then an existing `<runtime>/maps`.

A prebuilt engine elsewhere: `--engine` alone makes the engine's folder the runtime directory; add `--runtime-directory` when the runtime files live somewhere else.

### 3.7 Another game or map (start script)

```bash
cp tools/scripts/pbr-ibl-startscript.txt /c/temp/my-start.txt
# edit mapname= in /c/temp/my-start.txt to the map's internal name (as the game lists it)
tools/scripts/run-pbr-ibl-smoke.sh --start-script /c/temp/my-start.txt
```

The map must be installed in the maps directory. The engine copies the script it was given to `<write-dir>/_script.txt`. See section 8 for every key of the committed script.

### 3.8 Real-input probe (exclusive use of the computer)

```bash
tools/scripts/run-pbr-ibl-smoke.sh --input-probe --window-mode foreground
```

The separate opt-in `--input-probe` deliberately sends real Windows input and should be run only while you are not using the computer: it moves your cursor and types into the game for about a minute. The launcher refuses it in background window mode. Windows delivers synthetic input to whichever window is in front, so if another window comes forward during the run, the remaining keys and clicks go there. Results: `<write-dir>/pbr-ibl-input-probe/report.txt` and six step screenshots; the script returns the probe's status (0 = every check passed). Section 10 describes the checks.

### 3.9 From VS Code

The worktrees' local `.vscode/tasks.json` files (not tracked) call the launcher:

| Worktree | Task | Command |
|---|---|---|
| Recoil | `build` | compile, install runtime, split symbols, then `--prepare-only` |
| Recoil | `Launch PBR IBL` | `run-pbr-ibl-smoke.sh --build-directory build/window` |
| Recoil | `Run PBR IBL self-test` | `--self-test`, then prints the verdict lines of the report |
| BAR | `PBR: launch isolated IBL smoke test`, `PBR: run automated IBL self-test`, `PBR: prepare isolated Recoil runtime` | the same, with `--game-directory "${workspaceFolder}"` |

## 4. Options and environment variables

| Option | Environment | Default | Meaning |
|---|---|---|---|
| `--engine PATH` | `PBR_ENGINE` | `<runtime>/spring.exe` | engine executable |
| `--runtime-directory PATH` (alias `--build-directory`) | `PBR_RUNTIME_DIRECTORY` (legacy `PBR_BUILD_DIRECTORY`) | `build/window` in this repository | isolation root: engine, DLLs, fonts, base content, `games/`, `maps/` |
| `--game-directory PATH` | `PBR_GAME_DIRECTORY` | sibling auto-discovery | BAR checkout with both PBR IBL widgets |
| `--maps-directory PATH` | `PBR_MAPS_DIRECTORY` | sibling `BARS-Install/data/maps`, then `<runtime>/maps` | installed maps |
| `--write-directory PATH` | `PBR_WRITE_DIRECTORY` | `test-results/pbr-ibl` in this repository | settings, logs, caches, screenshots, reports |
| `--start-script PATH` | `PBR_START_SCRIPT` | `tools/scripts/pbr-ibl-startscript.txt` | game to start |
| `--config-template PATH` | `PBR_CONFIG_TEMPLATE` | `.vscode/springsettings-debug.cfg` if present | first-run `springsettings.cfg` |
| `--game-link-name NAME` | `PBR_GAME_LINK_NAME` | `Beyond-All-Reason-Private-pbr-test.sdd` | junction name under `<runtime>/games` (one `.sdd` name, not a path) |
| `--cursor-mode MODE` | `PBR_CURSOR_MODE` | `software` | `software`, `hardware` or `keep` (section 7) |
| `--window-mode MODE` | `PBR_WINDOW_MODE` | `background` | `background`, `foreground` or `keep` (section 5) |
| `--window-size WxH` | `PBR_WINDOW_SIZE` | `1280x720` | windowed size |
| `--window-position X,Y` | `PBR_WINDOW_POSITION` | `40,40` | window position in desktop pixels (may be negative) |
| `--self-test` | | off | run the self-test widget, then quit |
| `--input-probe` | | off | run the real-input probe (needs `--window-mode foreground`) |
| `--prepare-only` | | off | create links and settings, do not launch |
| `-h`, `--help` | | | print the usage text |

Run `tools/scripts/run-pbr-ibl-smoke.sh --help` for the same list. The launcher exits with the engine's status (the probe's status with `--input-probe`), or with 1 and a message when an input is missing or invalid.

## 5. Window behaviour

Test runs are unattended by default (`--window-mode background`, `PBR_WINDOW_MODE`): the launcher writes `Fullscreen = 0`, `WindowBorderless = 0`, `MinimizeOnFocusLoss = 0` and a plain 1280x720 window at 40,40 (`--window-size WxH`, `--window-position X,Y`) into the isolated settings, and exports SDL's `SDL_WINDOW_NO_ACTIVATION_WHEN_SHOWN=1`. SDL reads its hints from environment variables of the same name, so no engine change is needed.

The game window therefore opens without taking keyboard focus. A window shown without activation still lands at the top of the window stack (the "z-order"), and clicking the already-active window behind it does not raise that window. The hidden helper `pbr-window-to-back.ps1` therefore moves the game window to the bottom of the stack every second, without activating it (`SetWindowPos(HWND_BOTTOM)` with `SWP_NOACTIVATE`), so it never covers your windows. It logs to `<write-dir>/window-to-back.log`. The self-test needs no physical mouse or keyboard, and a covered window keeps rendering.

Do not minimize a running test: Recoil draws only about twice a minute while minimized, so the self-test would stall.

A new write directory is seeded with the newest archive cache of an earlier run beside it. Without that cache the engine hashes the BAR checkout for about 9 s without answering window messages, Windows treats the window as hung, and the ghosted window ("Not Responding") takes the focus; after BAR files change this re-hash still happens once.

`--window-mode foreground` opens the same plain window activated, and `--window-mode keep` leaves the config's own window and fullscreen settings untouched. `--input-probe` refuses background mode because it moves your cursor and types into the game.

## 6. What the launcher changes, and where

The launcher never edits the BAR checkout, installed maps, or a normal BAR user directory. Everything else it touches:

| Where | What | Why |
|---|---|---|
| `<runtime>/games/<game-link-name>` | directory junction to the BAR checkout | with `--isolation` the engine finds games only below the runtime; all BAR checkouts share the archive name `Beyond All Reason $VERSION`, so only one may be visible; a stable name means renaming the checkout changes nothing |
| `<runtime>/maps` | directory junction to the maps directory | the start script's map must be found below the runtime |
| `<write-dir>/springsettings.cfg` | copied from the template on the first run, then the keys below | isolated settings; the user's normal settings are never read |
| `<write-dir>/cache/ArchiveCache*.lua` | copied from the newest earlier run beside it, only when missing | avoids the 9 s hang described in section 5 |
| `<write-dir>/LuaUI/Config/BYAR.lua` | with `--self-test`, `["PBR IBL Self-Test"] = 0,` becomes `= 1,` | BAR ignores a widget's default once its saved order has an entry; 0 (switched off in F11) would stop the self-test loading |

A junction is an NTFS folder alias. Unlike a symbolic link it needs neither administrator rights nor Developer Mode. The launcher creates it with `cmd.exe /c mklink /J` (with `MSYS_NO_PATHCONV=1`, so Git Bash does not turn the switches `/d /c /J` into drive paths). An existing path is accepted only if it already points at the same target; anything else is refused rather than replaced. Remove a junction with `rmdir` in cmd: that deletes the link, never the target.

Settings written into the isolated `springsettings.cfg`:

| Key | Value | When | Why |
|---|---|---|---|
| `SpringData` | the runtime directory | every launch | a copied template can carry absolute search paths of another checkout |
| `HardwareCursor` | `0` / `1` | `--cursor-mode software` / `hardware` | section 7 |
| `Fullscreen`, `WindowBorderless` | `0` | window mode not `keep` | plain window that does not cover the desktop |
| `XResolutionWindowed`, `YResolutionWindowed` | from `--window-size` | window mode not `keep` | fixed size, comparable runs |
| `WindowPosX`, `WindowPosY` | from `--window-position` | window mode not `keep` | title bar on screen |
| `MinimizeOnFocusLoss` | `0` | window mode not `keep` | a minimized game stalls |
| `PBREnvironmentCapture` | `hdr-modern` | first run, and every `--self-test` | later the viewer's Capture dropdown owns the value; the self-test needs `hdr-modern` |
| `PBRIBLSelfTest`, `PBRIBLSelfTestQuit` | `1` with `--self-test`, else `0` | every launch | arms the self-test only for that launch |
| `LogFlush` | `1` | `--input-probe` | the probe reads `infolog.txt` while the game runs; the key is not reset afterwards |

The engine rewrites `springsettings.cfg` when it exits; keys equal to their defaults may then no longer be listed. The launcher writes its keys again on every launch.

The write directory default is `test-results/pbr-ibl` at the Recoil repository root. This intentionally lives outside `build/`, so deleting and recreating the build tree does not remove settings, logs, reports, screenshots, or numeric dumps. The repository ignores `test-results/`; copy or archive any evidence that must be retained beyond the checkout itself. Do not run two games at once with the same write directory.

What a run leaves in the write directory:

| Path | Content |
|---|---|
| `infolog.txt` | engine and widget log of the last run |
| `_script.txt` | the start script the engine used |
| `springsettings.cfg` | the isolated settings |
| `pbr-ibl-selftest/` | `report.txt`, `view_*.png` panel images, raw float dumps (`environment_32.txt` and others: one `face row column r g b` per line) |
| `pbr-ibl-input-probe/` | `report.txt` and `01_` to `06_*.png` |
| `pbr-ibl-viewer/layout.txt` | answers to `/pbribllayout` |
| `screenshots/` | engine screenshots (F12) |
| `window-to-back.log` | the z-order helper's log |
| `cache/`, `fontcache/`, `LuaUI/` | engine caches and the isolated LuaUI configuration |

## 7. Cursor mode

The isolated launcher defaults to `--cursor-mode software` (`HardwareCursor = 0`) so OBS Game Capture can include the in-game cursor. Use `--cursor-mode hardware` to force the OS cursor, or `--cursor-mode keep` to retain the isolated config's setting; `PBR_CURSOR_MODE` is the equivalent environment override. This changes only the isolated write directory's settings, not your normal BAR installation. For a manual launch, set `HardwareCursor = 0` in that launch's `springsettings.cfg` and enable cursor capture in OBS. Hardware cursor overlays may not be present in the captured game framebuffer.

## 8. The start script

`pbr-ibl-startscript.txt` is a Recoil start script (TDF format: `[section]`, `{ }`, `key=value;`). The engine takes it as its last argument. It has no comments because the engine copies it unchanged into `_script.txt` for every run; this table explains it instead.

| Key | Value | Meaning |
|---|---|---|
| `gametype` | `Beyond All Reason $VERSION` | archive name of an unpacked BAR checkout (its `modinfo.lua` version is the literal `$VERSION`); resolved through the game junction |
| `mapname` | `Onyx Cauldron 2.2.2` | the map; must be installed in the maps directory |
| `ishost`, `hostip`, `hostport` | `1`, `127.0.0.1`, `0` | a local game hosted by this process on the loopback address; no other client joins |
| `myplayername` | `Player` | must match `[player0] name` |
| `startpostype` | `2` | choose start positions in game; the game therefore waits on the start-position screen, which is all the PBR tests need |
| `gamestartdelay` | `5` | seconds before the match would start |
| `numplayers`, `numusers` | `1`, `2` | one human, two participants (player and AI) |
| `nohelperais` | `0` | helper AIs allowed |
| `[player0]` | team 0 | the local player (`isfromdemo=0`: live, not a replay) |
| `[ai0]` | `SimpleAI`, team 1, hosted by player 0 | an opponent so the game is a valid match |
| `[team0]`, `[team1]` | Armada / Random, blue / orange | sides and colours |
| `[allyteam0]`, `[allyteam1]` | start boxes in opposite corners | as fractions of the map size |

## 9. Debug symbols (`split-debug-symbols.sh`)

A Debug MinGW `spring.exe` with embedded DWARF is about 2.7 GB, and Windows refuses PE images of 2 GiB or more ("Exec format error"). The helper runs `objcopy --only-keep-debug` (symbols to `spring.dbg`), `strip --strip-debug --strip-unneeded` (shrinks `spring.exe`, about 50 MB) and `objcopy --add-gnu-debuglink=spring.dbg` (GDB follows this link and keeps full symbols). Running it again is harmless: an already split executable is only verified.

```bash
tools/scripts/split-debug-symbols.sh --runtime-directory build/window
tools/scripts/split-debug-symbols.sh --verify-only --runtime-directory build/window
tools/scripts/split-debug-symbols.sh --engine /c/path/to/spring.exe
PBR_OBJCOPY=/c/msys64/mingw64/bin/objcopy.exe tools/scripts/split-debug-symbols.sh
```

Expected output of `--verify-only`:

```text
Verified stripped runtime and external symbols:
  Runtime: /c/.../build/window/spring.exe
  Symbols: /c/.../build/window/spring.dbg
```

Every Debug rebuild links the symbols back into `spring.exe`, so run the split after each build (the VS Code `build` task does).

## 10. The real-input probe in detail

`pbr-viewer-input-probe.ps1` waits for the engine window and the `finished loading and is now ingame` line in `infolog.txt`, brings the game to the front, and then:

1. types `/luaui enablewidget PBR IBL and Cubemap Viewer` into chat (the documented path) and checks the viewer opened;
2. clicks the Resource dropdown and its sixth row (the GGX prefilter), presses Up twice while the panel is focused, then F12 and Enter, and checks that no engine screenshot was taken and the mip changed;
3. checks with engine screenshots that a held arrow key does not move the camera while focused, and does after Esc;
4. checks that the wheel over the panel only steps the mip, while the wheel outside the panel zooms the camera;
5. disables the viewer again if it was off, restores the cursor position, writes the report and quits with `/quitforce`.

Control positions and the viewer state come from the viewer's `/pbribllayout <tag>` answers in `pbr-ibl-viewer/layout.txt`. Key events carry scan codes (SDL ignores events without them), text is typed as Unicode input events, and screenshots are taken by the engine because desktop capture of an OpenGL window can be stale. "Camera moved" is decided from the mean RGB difference of a sampled world area right of the panel: below 0.8 counts as still, above 3.0 as moved. It is Windows-only and needs the 64-bit Windows PowerShell.

A passing report ends with `SUMMARY pass=8 fail=0`.

## 11. Troubleshooting

| Symptom | Cause and fix |
|---|---|
| `The engine exceeds Windows' 2 GiB executable limit` | Run `split-debug-symbols.sh` (section 9). |
| `Engine runtime files are not installed` / "Failed to load FontFile" / `0xc0000135` | Run `cmake -P build/window/cont/cmake_install.cmake`. |
| `No sibling BAR checkout contains the PBR IBL widgets` or `Multiple sibling BAR PBR checkouts were found` | Pass `--game-directory`. |
| `No maps directory was found` | Pass `--maps-directory` with a folder holding Onyx Cauldron 2.2.2. |
| `Refusing to replace <path>` | `<runtime>/games/<name>` or `<runtime>/maps` exists and is not a junction to the requested target. Remove that junction with `rmdir` in cmd (check first that it is a junction), or choose another `--game-link-name`. |
| The game window took the keyboard focus at start | The archive cache was rebuilt (first run, or BAR files changed); later runs stay in the background. |
| The game window stays in front | The z-order helper is not running: check `window-to-back.log` for a `keeping window` line. |
| The game window keeps disappearing behind other windows | That is background mode working as intended; launch with `--window-mode foreground` to watch. |
| `--self-test` does nothing | The isolated widget order had the self-test switched off in a format the launcher did not recognise; check `LuaUI/Config/BYAR.lua` and `infolog.txt` for `[PBR IBL SelfTest]`. |
| Self-test stalls | The window was minimized; restore it. |
| Input probe: `no target engine window appeared` | The engine failed to open a window, or the probe was given a process id it cannot find; check `infolog.txt`. |

## 12. Extending

- **A new launcher option** that writes one more isolated setting: follow the worked example in the header of `run-pbr-ibl-smoke.sh` (default from a `PBR_*` variable, a `case` branch, validation, one `set_setting` call, `usage()` and this guide).
- **A new real-input check**: follow the worked example in the header of `pbr-viewer-input-probe.ps1`; read the viewer state through `Layout`, and use `Shot` with `WorldDiff` only to decide whether the camera moved.
- **A different z-order placement** (for example directly behind one window instead of at the bottom): see the header of `pbr-window-to-back.ps1`.
- **Another executable to split**: `split-debug-symbols.sh --binary-name <name>.exe`.

Keep the language split: bash for entry points that launch programs and prepare folders, PowerShell only for Windows window and input calls, always started from the bash launcher.
