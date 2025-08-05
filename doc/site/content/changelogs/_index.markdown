+++
title = "Running changelog"
[cascade]
  [cascade.params]
    type = "docs"
+++

This is the bleeding-edge changelog **since version 2025.04**, which is still in the pre-release phase.
See [the 2025.04 page]({{% ref "changelogs/changelog-2025-04" %}}) for the upcoming release.

## Caveats

- unit shaders receive TRS transforms instead of matrices, breaking shaders. See the "Smooth script piece animations" section below.
- removed Python bindings for AI. Apparently unmaintained and unused.
- removed `UpdateWeaponVectorsMT`, `UpdateBoundingVolumeMT`, and `AnimationMT` springsettings. These were just in case, but MT seems safe enough after some time live testing.
- removed `/AdvModelShading` command and the `AdvUnitShading` springsetting, the adv mode is now always on. In practice there wasn't any difference since GLSL became mandatory.
- removed `gl.UnitGL4`, `gl.FeatureGL4`, `gl.FeatureShapeGL4`, and `gl.UnitShapeGL4`. These did not do anything.
- handicap (native income multiplier) no longer applies to reclaim. Use `gadget:Allow{Unit,Feature}BuildStep` to replace, if desired.
  Backwards compatibility feature check: `Engine.FeatureSupport.noHandicapForReclaim`.
- `/group add N` no longer selects the entire group, just adds. `/groupN` (without space) unaffected. Feature support tag: `Engine.FeatureSupport.groupAddDoesntSelect`.
- immobile units that `canKamikaze` no longer disregard the `blocking` unit def tag.
- `BeamLaser` and `LightningCannon` type weapons now pass their actual projectileID to callins rather than `-1`.
- `BeamLaser` and `LightningCannon` now obey ellipsoid and/or cylinder target volumes correctly, this will result in a slightly different effective range volume.
- in-line text color codes: 0x11 and 0x12 (decimal 17 and 18) now work as color code indicators, in addition to 255 and 254 respectively.
  `Engine.textColorCodes` still only lists the old ones.
- default value for `LogRepeatLimit` springsetting 10 → 0, see below.
- area resurrect command now puts a leash on repair sub-commands (behaviour change, but also the repair subcommands now have 5 args instead of 1).
- widgets can now sign up to receive the `widget:Explosion` callin. There are some arcane visibility rules that boil down to it revealing much more info than the existing `widget:ShockFront` callin.
- fully removed support for 32-bit builds.

### Deprecation notice

- `wupget:DrawUnit`, `DrawFeature`, `DrawShield` and `DrawMaterial` are deprecated.
  Nothing changes in behaviour, but they are (and have been for a long time) a bad idea to use as a performance anti-pattern.
  Use shaders as a replacement.
- deprecated `Spring.GetUnitCommands(unitID, 0) → number commandCount`, use `Spring.GetUnitCommandCount(unitID)`.
  The non-0 overloads still return a table of commands and are not affected.
- similarly deprecated `Spring.GetFactoryCommands(unitID, 0)`, use the new `Spring.GetFactoryCommandCount`.
- deprecated `Spring.GetCommandQueue`, use `Spring.GetUnitCommands` (which has always been 100% equivalent), except for the case above.
- deprecated the `ghostedBuildings` engineoption. You can read it manually as a modoption and set every unitdef not to leave ghosts.

## Features

### Camera callins

- added `wupget:CameraRotationChanged(rotX, rotY, rotZ) → nil`.
- added `wupget:CameraPositionChanged(posX, posY, posZ) → nil`.

### Chat via widgets

- added `Spring.SendPublicChat(msg) → nil`, for sending a chat message. Equivalent to `/say`.
- added `Spring.SendAllyChat`, and `Spring.SendSpectatorChat`, same interface as above, for sending chat to allies/spectators.
  Equivalent to using `/say` with the "a:" or "s:" magic destination prefixes.
- added `Spring.SendPrivateChat(msg, playerID) → nil`, for sending a private message (aka whisper) to a specific playerID.
  Almost equivalent to `/wbynum`, but does not support the magic 255 destination.

### GL debugging

- added `DebugGLReportGroups` boolean springsetting, default true. Shows OpenGL push/pop groups in GL debug.
- the `/debugGL` option now takes an optional numerical argument, 0-15.
  0 and 1 control the whole debug view without touching anything else (i.e. work as before).
  Otherwise values 2-15 are treated as a bitmask: 8 controls stacktraces, 4 report groups, 2 the whole debug enabled/disabled state, 1 ignored.

### New Website

- Slashscreen, in an effort to procrastinate from university finals, completely redid the website.
- Reworked the Api docs.
- New guides.
- Documented RmlUi bindings.

### CEG rendering

- adds the following tags to `CBitmapMuzzleFlame` type CEG particles: `particleSpeed`, `particleSpeedSpread`, `airdrag`, `gravity`.
  These work the same as they already do for `CSimpleParticleSystem` type particles.
- added `animParams1` and `animParams2` to CEGs, similar to weapons.
Texture 1 is used for `CSimpleParticleSystem` and the "front" texture of `CBitmapMuzzleFlame`,
texture 2 is used for the "side" texture of muzzle flame.
Other CEG classes don't support this yet.
Numberless `animParams` used as a fallback.
- added `drawSideX` and `drawSizeY` bool tags to `CBitmapMuzzleFlame`, allows to skip some of the two "forward" sprites. X is the one parallel to the ground when shooting horizontally. There is no `drawSideZ` because you can just set that texture to empty.

### Infotex commands

- added `/showLoS`.
- added `/showInfoTex foo`, where `foo` is one of the existing info textures (e.g. `/showInfoTex elevation` is equivalent to `/showElevation`).
- all the `/showFoo` commands can now accept 0/1, in addition to argument-less toggling.

### Smooth script piece animations

- unit shaders receive TRS transforms instead of matrices, breaking shaders.
  This will require gamedevs to update GL4 shaders that used to employ the engine provided transformation matrices SSBO.
- the new format is not directly documented anywhere, but you can look at basecontent shaders.
- added `Engine.FeatureSupport.transformsInGL4` backwards compatibility check.

### Base64

- added `Encoding` global table (like `Spring` or `math` etc), available everywhere. So far contains base64 encoding, more (such as JSON) to be added lated.
- added `Encoding.EncodeBase64(string plain, bool? stripPadding = true) → string encoded`. Can optionally remove the extra `=` at the end.
- added `Encoding.DecodeBase64(string encoded) → string plain`, decodes a base64 string (if there's an invalid character, it parses just the valid prefix).
- added `Encoding.IsValidBase64(string) → bool`.
- added `EncodeBase64Url`, `DecodeBase64Url`, and `IsValidBase64Url` to `Encoding`. Same as above, except uses the base64url character set.

### Duplicate logs

- the `LogRepeatLimit` springsetting, which controls how many duplicate logs in a row can end up in the infolog, can now accept 0 for infinite.
- default value for `LogRepeatLimit`: 10 → 0
- native ingame console no longer filters duplicates (max 1 in a row → infinite)

### GLTF/GLB model format support

- `.gltf` and `.glb` model formats are now supported.
- GLTF parser will insert a few empty pieces as root / sub-root.
- materials, textures and vertex colors embedded in the GLTF itself are not supported. Mostly because GLTF expects PBR workflow, which the standard engine shader doesn't do.
- GLTF can use a Lua metadata file similar to Assimp, custom model attributes in GLTF can also be used to provide the same data. Supports the existing tags: `tex1`, `tex2`, `midpos`, `mins`, `maxs`, `height`, `radius`, `fliptextures`, `invertteamcolor`.
- add a new `s3ocompat` bool attribute, both for GLTF and Assimp models. Enables the same right handed - left handed flip of the model as it's done in s3o. This is experimental and probably very bugged. If your model is converted from s3o and animates incorrectly, give this option a try.
- does not support re-parenting or the baked rotation angles. Use your 3d editor to change them instead. Assimp not affected.

### Ghosts

- add `leavesGhost` unit def tag. Controls whether the unit leaves a ghost. Defaults to true for buildings (maintains previous behaviour).
- add `leavesGhost` UnitDefs entry, same.
- add `Spring.GetUnitLeavesGhost(unitID) → bool`.
- add `Spring.SetUnitLeavesGhost(unitID, bool leavesGhost, bool? existingGhostRemains = false)`. If `leavesGhost` becomes false and the unit already had a ghost, whether the existing ghost is removed or sticks around is controlled by the second arg.
- note that the `ghostedBuildings` engineoption still exists and makes all of the above interfaces no-op, but is scheduled for removal.

### Text coloring

- added `AllowColorFonts` bool springsetting, defaults to false. Allows fonts to be colored instead of monochromatic. In practice this means emojis render as expected, though a fancy enough font could also do something to normal text.
- in-line text color codes: 0x11 and 0x12 (decimal 17 and 18) now work as color code indicators, in addition to 0xFF and 0xFE (decimal 255 and 254) respectively. The significance of this is that 255/254 are normal, printable characters from reasonable languages so treating them as color codes can ruin otherwise normal text by eating characters and setting random colors.
- `Engine.textColorCodes` still only lists the old ones.
- added `TextDisableOldColorIndicators` bool springsetting, defaults to false. If true, the previous 255 and 254 color codes stop working and `Engine.textColorCodes` lists the new ones.

### Explosion event in unsynced

- add `Script.SetWatchExplosion(weaponDefID, bool)`, and the corresponding Get, to unsynced.
- widgets can now receive the `Explosion` callin, same arguments as the gadget one but can't use the return value to block the CEG. There are some arcane visibility rules (that include the visibility of the explosion's CEG) that boil down to it revealing a bit more info than the existing `widget:ShockFront` callin which used to fulfil a similar role.

### Death metal

- add `Spring.CreateUnitWreck(unitID, int? wreckLevel = 1, bool? emitSmoke = true) → featureID?`. Creates a wreck as if the unit died.
- add `Spring.CreateFeatureWreck(featureID, int? wreckLevel = 1, bool? emitSmoke = false) → featureID?`. Creates a wreck as if the unit died.
- add `Spring.AddFeatureDamage` with the same signature as `Spring.AddUnitDamage`. Note that one of the args is paralyze time, but features don't take paralyze damage.
- add a new `checkDeath` bool arg to `Spring.SetFeatureHealth(featureID, health, bool? checkDeath = false)`. It was discovered that the current behaviour is that features set to negative health don't die from HP removal and it takes an instance of damage to "poke" them into death. The default of `false` preserves this, set it to `true` to let features die immediately from HP removal.
- add `Spring.SetFeatureFireTime(featureID, fireTime)`, sets a feature "on fire" (similar to being damaged with a `firestarter` tagged weapon) with that much time to live, in seconds. Set to 0 to cancel.
- add `Spring.SetFeatureSmokeTime(featureID, smokeTime)`, makes the feature emit smoke (similar to freshly killed wrecks) for that many seconds. Note that smoke size is proportional to time remaining.
- add the corresponding getters (`Spring.GetFeatureFireTime`, `Spring.GetFeatureSmokeTime`) for the above.

### Misc

- add `accurateLeading` numerical unit weapon tag (note, not weaponDef). Controls how many extra accuracy iterations are done when calculating shots.
0: current behaviour (single iteration, fails to hit at large speed disparities and/or weird angles).
1: one extra iteration, enough to get a perfect solution for non-ballistic shots and usually good enough for non-extreme ballistic shots too.
2+: extra iterations for ballistic shots, if needed. In practice, shouldn't need to go higher than low single-digit values.
Note that the calculation always stops when 1-frame resolution accuracy is achieved (since unit and projectile movement are all frame-discrete), so setting this arbitrarily high should still be safe and reasonably performant.
- math extras (`math.hypot`, `math.normalize`, bit ops etc) now available in LuaParser envs (modrules, defs parsing, etc).
- added `Spring.GetFactoryCommandCount(unitID) → number` checks the queue length in a factory's build queue (note that `GetUnitCommandCount` is for the rally queue in factories).
- add `ThreadPinPolicy` springsetting to control which CPUs to use. (0) = Off; (1) = System Default; (2) = Exclusive Performance Core; (3) = Share Performance Cores.
- added new CEG impact filters, `shield` and `intercepted`.
- `BeamLaser` and `LightningCannon` type weapons now pass their actual projectileID to callins rather than `-1`.
- `BeamLaser` and `LightningCannon` now obey ellipsoid and/or cylinder target volumes correctly.
- immobile units that `canKamikaze` no longer disregard the `blocking` unit def tag.
- add 'system.nativeExcessSharing' modrule, controls whether the resource sharing level (aka "red slider") applies. Note that this also prevents excess from flowing back to the allyteam. If you want excess to flow to allies and only block changing the share level, use the existing `gadget:AllowResourceLevel`.
- add `MiniMapDrawPings` boolean springsetting, defaults to true. Whether engine renders pulsating white squares on the minimap when a label is placed.
- if a factory "changes" a build order into a build order of the same type (e.g. by using command insert) it no longer resets build progress.
- `/group add N` no longer selects the entire group, just adds. `/groupN` (without space) unaffected. Feature support tag: `Engine.FeatureSupport.groupAddDoesntSelect`.
- expose the rmlUI data model backing table as `__GetTable()`.
- add `gl.GetEngineModelUniformDataSize(number index) → number sizeInElements, number sizeInBytesOnCPU` to grab the size of the model uniforms buffer on the CPU side.
- add `LuaVBO:CopyTo(otherVBO, sizeInBytes) → bool ok` to copy the GPU side content from the current LuaVBO to another.
- add a second bool arg and an optional second return value to `Spring.GetGroundDecalTextures(bool? mainTex, bool? alsoFilenames = false) → string[] textures, string[]? filenames`.
- add `mouse2` to `mouse10` "keys" for mousebuttons that can be bound, ditto `sc_mouse2-10` scancodes.
  LMB (`mouse1`/`sc_mouse1`) is planned to be made bindable later.
- added `DWMFlush` numerical springsetting, for Windows only. Forces Windows Desktop Compositors DWMFlush before each SDL_GL_SwapWindow,
preventing dropped frames (use nVidias FrameView to validate dropped frames, or BARs Jitter Timer widget).
Value of 1 does DWMFlush before SwapBuffers, value of 2 does DWMFlush after swapbuffers.
- enabling dev mode (either via startscript or at runtime) adds `debug.*` functions to synced Lua.
- `/smoothmesh` renderer stays enabled even if you disable cheats.
- trying to set a deprecated/nonexistent springsetting via Lua now produces a warning.
- `/give` now gives a correct hint for @x,y,z usage.
- added `GL.TEXTURE_2D_ARRAY` Lua constant.

## Fixes

- fixed crashing aircraft sometimes bouncing off the ground without dying.
- disabling cheats no longer disables debug airmesh view.
- fix RmlUi crash when reloading stylesheets with documents containing scripts in head section.
- fix some graphical interfaces producing harmless warnings/errors on headless builds.
- fix some RmlUI crashes.
- fix some remaining performance issues with fonts.
- fix some pathing/QTPFS issues.
- fix some jitter on Windows in mostly-idle and/or low speed multiplayer games.
- fix some issues with models rendering at world origin before game start.
