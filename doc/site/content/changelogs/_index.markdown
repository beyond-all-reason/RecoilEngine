+++
title = "Running changelog"
[cascade]
  [cascade.params]
    type = "docs"
+++

This is the bleeding-edge changelog since version 2025.06, for **pre-release 2026.06**.

## Caveats

- UTF-8 file paths are now supported.
- some file accesses are now case-sensitive.
- rmlUI version used 6.0 → 6.2
- ARM64 architecture builds now have nominal support.
- `/aicontrol` is now blocked by default. Call `/aiCtrl PlayerName` or `/aiCtrlByNum 123` to enable.
- `script:AimWeapon` now receives unit-relative heading and pitch, rather than world-space. This means units angled on slopes will receive different values.
- heading cast to radians will now return \[-pi; +pi) rather than \[0; tau).
- minor Lua env sandboxing changes, see the "Lua environment sandboxing" section below.
- always output logs to stdout.
- removed `CSphereParticleSpawner` particle class. Identical to `CSimpleParticleSystem`.
- archive cache version 20 → 21.

## Features

### RmlUi

- rmlUI version used 6.0 → 6.2
- add datamodel support for pairs: `pairs(dm_handle)`
- add datamodel support for ipairs: `dm_handle:__ipairs()`
- support for accessing the underlying datamodel table with `dm_handle.__raw()`
- allow datamodel self-referential assignments such as `dm_handle.property = dm_handle.another_property`
- support for retrieving datamodel property length: `dm_handle.property.__len()`
- fix datamodel array access
- fix `data-value` binds in rml elements
- added `RmlUi.GetDocumentPathRequests(string docPath) -> {"filePath", "filePath", ...}` which tracks all of the files opened by an RmlUi LoadDocument call
- added `RmlUi.ClearDocumentPathRequests(string docPath) -> nil` to clear tracked LoadDocument files

### Radar icon Lua API
- added `Spring.SetUnitIcon(unitID, string? iconName)`. Pass `nil` to reset to default.
- added `Spring.GetUnitIcon(unitID) → string iconName`.

### Minimap callins
- added `wupget:MiniMapRotationChanged(rotation, previousRotation)` unsynced callin. In radians.
- added `wupget:MiniMapGeometryChanged(x, y, sizeX, sizeY, prevX, prevY, prevSizeX, prevSizeY)` unsynced callin. In pixels.
- added `wupget:MiniMapStateChanged(isMinimized, isMaximized, isSlaved)` unsynced callin.

### Lua environment sandboxing
- LuaSocket no longer inits before Lua sandboxing.
- unsynced LuaRules (incl. unsynced LuaGaia) now has access to `io` and `os` libraries.
- unsynced LuaRules (incl. unsynced LuaGaia) now has access to the `debug` library by default (no longer requires devmode).

### Replay path getters
- add `Spring.GetReplayFilePath() → string?`, returns path of replay being watched.
- add `Spring.GetReplayRecordingFilePath() → string?`, returns path of replay to be produced. Note that this is just a prospective file path (nothing is written until the match ends), and that it possible to record a replay of a replay.

### Build commands
- builders now perform an extra block check immediately when a build command reaches the front of the queue. This is in addition to the existing periodic (≈ 0.4 Hz) block check. A block check cancels a build command if the build site is hard-blocked ("red squares", as opposed to "yellow squares" with reclaimables/mobiles).
- add `Spring.SetEngineBuildSquareRendering(bool) → nil`, for disabling the native rendering of the footprint grid when a build command is selected.
- add `wupget:DrawBuildSquare(unitDefID, x, z, facing, statuses) → nil` unsynced callin, fires when a build command is selected. Statuses is a 1D array for the status of each tile: 0 blocked (red), 1 occupied (yellow), 2 reclaimable (yellow), 3 open (green). This fires even if native drawing is enabled.

### Misc

- UTF-8 file paths are now supported.
- some file accesses are now case-sensitive.
- `/aicontrol` is now blocked by default. Call `/aiCtrl PlayerName` or `/aiCtrlByNum 123` to enable.
- `script:AimWeapon` now receives unit-relative heading and pitch, rather than world-space. This means units angled on slopes will receive different values.
- heading cast to radians will now return \[-pi; +pi) rather than \[0; tau).
- `VFS.GetAvailableAIs()` returned entries now have a new `isLuaAI` boolean.
- add `Platform.architecture`, string. Usually "x86_64", with some ongoing work to support "arm64".
- add `Spring.SetCheatingEnabled(bool)`.
- add `Spring.SetGodMode(bool? controlAllies, bool? controlEnemies)`.
- add `Spring.GetClosestEnemyUnit(x, y, z, range = inf) → unitID?` to LuaUI.
- add `Spring.GetClosestEnemyUnit(x, y, z, range = inf, allyTeamID, bool useLoS = true, bool spherical = false, bool requireEnemyToSeePos = false) → unitID?` to LuaRules.
- large QTPFS perf improvements.
- always output logs to stdout.
- add boolean `Platform.isHeadless`.
- archive cache version 20 → 21.

## Fixes

- fix logs sometimes not getting flushed on exit
- fix `CMD[20]` and `CMD[105]` returning legacy aliases for those commands rather than their standard names.
