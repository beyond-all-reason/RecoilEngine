---
layout: post
title: Running changelog
parent: Changelogs
permalink: changelogs/running-changelog
author: sprunk
---

This is the bleeding-edge changelog **since version 2025.04**, which is still in the pre-release phase.
See [the 2025.04 page]({{ site.baseurl }}{% link changelogs/changelog-2025-03.markdown %}) for the upcoming release.

# Caveats
* unit shaders receive TRS transforms instead of matrices, breaking shaders. See the "Smooth script piece animations" section below.
* removed Python bindings for AI. Apparently unmaintained and unused.
* removed `UpdateWeaponVectorsMT`, `UpdateBoundingVolumeMT`, and `AnimationMT` springsettings. These were just in case, but MT seems safe enough after some time live testing.
* removed `/AdvModelShading` command and the `AdvUnitShading` springsetting, the adv mode is now always on. In practice there wasn't any difference since GLSL became mandatory.
* removed `gl.UnitGL4`, `gl.FeatureGL4`, `gl.FeatureShapeGL4`, and `gl.UnitShapeGL4`. These did not do anything.
* setting a range circle's alpha to 0 in `cmdcolors` completely skips rendering it. Ditto for interfaces like `gl.SurfaceCircle` or `gl.BallisticCircle`.
FIXME does this mean you can no longer do weird stuff via exotic blending modes, or what exactly? commit a067faa41ef4b3222bbf33f757c79bb5e01c054a
* handicap (native income multiplier) no longer applies to reclaim. Use `gadget:Allow{Unit,Feature}BuildStep` to replace, if desired.
Backwards compatibility feature check: `Engine.FeatureSupport.noHandicapForReclaim`.

### Deprecation notice
* `wupget:DrawUnit`, `DrawFeature`, `DrawShield` and `DrawMaterial` are deprecated.
Nothing changes in behaviour, but they are (and have been for a long time) a bad idea to use as a performance anti-pattern.
Use shaders as a replacement.
* deprecated `Spring.GetUnitCommands(unitID, 0) → number commandCount`, use `Spring.GetUnitCommandCount(unitID)`.
The non-0 overloads still return a table of commands and are not affected.
* deprecated `Spring.GetCommandQueue`, use `Spring.GetUnitCommands` (which has always been 100% equivalent), except for the case above.

# Features

### Camera callins
* added `wupget:CameraRotationChanged(rotX, rotY, rotZ) → nil`.
* added `wupget:CameraPositionChanged(posX, posY, posZ) → nil`.

### Chat via widgets
* added `Spring.SendPublicChat(msg) → nil`, for sending a chat message. Equivalent to `/say`.
* added `Spring.SendAllyChat`, and `Spring.SendSpectatorChat`, same interface as above, for sending chat to allies/spectators.
Equivalent to using `/say` with the "a:" or "s:" magic destination prefixes.
* added `Spring.SendPrivateChat(msg, playerID) → nil`, for sending a private message (aka whisper) to a specific playerID.
Almost equivalent to `/wbynum`, but does not support the magic 255 destination.

### GL debugging
* added `DebugGLReportGroups` boolean springsetting, default true. Shows OpenGL push/pop groups in GL debug.
* the `/debugGL` option now takes an optional numerical argument, 0-15.
0 and 1 control the whole debug view without touching anything else (i.e. work as before).
Otherwise values 2-15 are treated as a bitmask: 8 controls stacktraces, 4 report groups, 2 the whole debug enabled/disabled state, 1 ignored.

### CEGs
* adds the following tags to `CBitmapMuzzleFlame` type CEG particles: `particleSpeed`, `particleSpeedSpread`, `airdrag`, `gravity`.
These work the same as they already do for `CSimpleParticleSystem` type particles.
* added `animParams1` and `animParams2` to CEGs, similar to weapons. Texture 2 is only used for the "side" texture of `CBitmapMuzzleFlame`, all other particles (and "front" texture of muzzle flame) use texture 1.
Not all CEG particles types support animations (FIXME: why, can't all textures get support? if not then list them, supported uses AddEffectsQuad<1> in implementation while unsupported is <0>).
Numberless `animParams` used as a fallback.

### Smooth script piece animations
* unit shaders receive TRS transforms instead of matrices, breaking shaders.
This will require gamedevs to update GL4 shaders that used to employ the engine provided transformation matrices SSBO.
* the new format is FIXME, and a more detailed documentation is found at FIXME.
* added `Engine.FeatureSupport.transformsInGL4` backwards compatibility check.

### Misc
* expose the rmlUI data model backing table as `__GetTable()`.
* add `mouse2` to `mouse10` "keys" for mousebuttons that can be bound, ditto `sc_mouse2-10` scancodes.
LMB (`mouse1`/`sc_mouse1`) is planned to be made bindable later.
* unit piece animations using GL4 rendering paths can now use smooth interpolations as opposed to using the fixed synced framerate.
* enabling dev mode (either via startscript or at runtime) adds `debug.*` functions to synced Lua.
* `/smoothmesh` renderer stays enabled even if you disable cheats.
* `/give` now gives a correct hint for @x,y,z usage.

## Fixes
* fix some graphical interfaces producing harmless warnings/errors on headless builds.