+++
title = "Release 2026.07"
aliases = ['/changelogs/changelog-2026-07']
+++

This is the changelog since version 2025.07 until **version 2025.07.01**, which was released on 2026-07-29.

## Caveats
- Barbarian AI updated to version 1.6.28

## Build
- Fixes for MSVC build
- Added warning about unsynced git submodules [PR 2902](https://github.com/beyond-all-reason/RecoilEngine/pull/2902)
- Fixed compilation warnings in CFrontTexture.cpp
- Fixed many compiler warnings
- Fixed sse2neon/streflop macro redefinition warning
- Build/compile changes to lead to eventual support for MacOS
- Added GetPrevFrameChecksum() to the Lua API
- Add multi-platform sync testing (amd64-windows/linux + arm64-linux) [PR 2921](https://github.com/beyond-all-reason/RecoilEngine/pull/2921)
- Optimize LuaPushNamedFoo using compile time key hashing [PR 2986](https://github.com/beyond-all-reason/RecoilEngine/pull/2986)
- Fixed (synctest): workaround widget timing nondeterminism [PR 3124](https://github.com/beyond-all-reason/RecoilEngine/pull/3124)
- Remove refrences to 32-bit support [PR 3033](https://github.com/beyond-all-reason/RecoilEngine/pull/3033)
- Improved setup foir ASAN build [PR 2663](https://github.com/beyond-all-reason/RecoilEngine/pull/2663)

## Documentation
- Added 'First Steps with the Engine' guide for game developers
- Fixed spGetUnitNearestEnemy docs
- UnitDestroyed attacker not always available [PR 2572](https://github.com/beyond-all-reason/RecoilEngine/pull/2572)
- Fixed wupget:DefaultCommand missing cmdID [PR 2984](https://github.com/beyond-all-reason/RecoilEngine/pull/2984)
- Fixed SplinterFaction card link format [PR 2999](https://github.com/beyond-all-reason/RecoilEngine/pull/2999)
- Added ENGINE_PERFORMANCE.md with some notes on engine internals [PR 2919](https://github.com/beyond-all-reason/RecoilEngine/pull/2919)
- Added BACKWARDS_COMPATIBILITY.md guidance doc for coding agents
- Improved running/testing instructions in AGENTS.md [PR 2917](https://github.com/beyond-all-reason/RecoilEngine/pull/2917)
- Spelling correction pass across site articles

## Lua
- LuaSocket no longer inits before sandboxing
- Extracted synced lib loader to a helper function
- Unsynced wrapper handles its sandboxing
- VFS.GetAvailableAIs: add isLuaAI
- Enabled IO, OS and debug libraries for unsynced gadgets [PR 2858](https://github.com/beyond-all-reason/RecoilEngine/pull/2858)
- Added Lua API to get current replay file paths
- Fixed memory barrier bitfield now pulls from correct parameter [PR 2557](https://github.com/beyond-all-reason/RecoilEngine/pull/2557)
- Implement DrawBuildSquare callin and add example GL4 widget [PR 2938](https://github.com/beyond-all-reason/RecoilEngine/pull/2938)
- Added Platform.architecture [PR 2825](https://github.com/beyond-all-reason/RecoilEngine/pull/2825)
- SolLua: use sol::lua_nil instead of the sol::nil alias
- LuaTextures: log glTexImage failures instead of returning nil silently
- Added Lua trace ray functions [PR 1624](https://github.com/beyond-all-reason/RecoilEngine/pull/1624)
- Added type checking to ADD_FOO Lua defs macros
- Fixed LuaSocket VFS mode error in LuaMenu
- Updated Spring.SetSkyBoxTexture to perform setup if needed
- LuaParser: run Spring.TimeCheck's callback under unitsync/dedicated
- Added debug.emulateFoo input emulation API [PR 3097](https://github.com/beyond-all-reason/RecoilEngine/pull/3097)
- Added gadget:ResourceExcess(excessTable) -> bool
- Added spAddTeamResourceExcessStats
- Added debug.emulateMouseWheel input emulation
- Added cancelcommand action
- Added GetPrevFrameChecksum() to the Lua API [PR 2922](https://github.com/beyond-all-reason/RecoilEngine/pull/2922)
- Fixes to Spring.SetMapShader [PR 3127](https://github.com/beyond-all-reason/RecoilEngine/pull/3127)
- MouseHandler: route XButtons (Mouse4/5) as keybinds instead of mouse ownership [PR 2613](https://github.com/beyond-all-reason/RecoilEngine/pull/2613)
- Fixed Lua EmmyLua type annotations [PR 2888](https://github.com/beyond-all-reason/RecoilEngine/pull/2888)

## Misc
- Restored lowercasing in FileSystem::GetExtension
- Demo handlers improvements: keep file paths absolute until needed and do not add a fake 'unnamed.sdfz' name to recordings
- Fixed missing cstdint include in ChatMessage.h [PR 2968](https://github.com/beyond-all-reason/RecoilEngine/pull/2968)
- Fixed the Nonus backlash scancode spelling mistake. [Issue 2978](https://github.com/beyond-all-reason/RecoilEngine/issues/2978)
- Fixed FileSystem::FindFiles so it should return relative paths [PR 3007](https://github.com/beyond-all-reason/RecoilEngine/pull/3007)
- float3: drop redundant direct streflop_cond.h include
- SafeUtil: include <type_traits> and <memory> directly [PR 3025](https://github.com/beyond-all-reason/RecoilEngine/pull/3025)
- Logging fix: updated section min-level in place instead of appending duplicates [PR 3052](https://github.com/beyond-all-reason/RecoilEngine/pull/3052)
- Converted u8string_view to string using explicit size
- Fixde inconsistent class/struct forward declarations
- Fixed truncated sync checksum in demotool dump
- Improve cross-platform portability in archive handlers
- Removed legacy engine options
- SpringMath: add Catmull-Rom bicubic interpolation helpers
- Moved float3/float4/Matrix44f str() out of line
- Fixed unbindaction not clearing scancode bindings
- AIFloat3: Remove non-trivial copy constructor
- Fix /unbind for keychains
- Fix stale spGetActionHotKeys returns [PR 3082](https://github.com/beyond-all-reason/RecoilEngine/pull/3082)
- Use a portable type cast in MemPoolTypes logging
- Support alternate file extensions for replays [PR 2975](https://github.com/beyond-all-reason/RecoilEngine/pull/2975)
- Throw error and stop processing if modrules parsing fails

## Rendering
- Added sorting icon names before adding to atlas so insertion order is consistent across runs.
- Replaced non-deterministic GL texture IDs with stable insertion-order indices for UniqueSubTexture naming.
- Fixed stableIdx assignment to be per-file at first insertion time.
- Also sorted icon rendering to use deterministic ProjectileDrawer iteration.
- Added specifying output format (e.g. "dumpatlas proj tga") instead of hardcoded .png.
- Fixed icon sliding artifact and add edge-pixel padding to atlas
- Disabled runniung atlas/iconhandler in headless
- Fixed a rare crash in UnitDrawer / IconsDrawer caused by inconsistent LOS settings set from Lua.
- Added Custom Color Palette [PR 2945](https://github.com/beyond-all-reason/RecoilEngine/pull/2945)
- Fixed to icons bleeding: adjust CTextureRenderAtlas so it always works with the original/biggest lod
- Removed Java AI bindings
- Debug logs for loading splat normals
- Fix SMF DNTS gating and fallback textures
- Add mapoptions for blank map splats
- Performance improvements with drawing ghosted buildings [PR 3110](https://github.com/beyond-all-reason/RecoilEngine/pull/3110)
- Changed ghosted buildings are drawn based on the last team was seen with and so which team ownership doesn't show automatically on the ghost. [PR 3108](https://github.com/beyond-all-reason/RecoilEngine/pull/3108)

## Simulation
- Sanitize NaNs in CHoverAirMoveType::UpdateMoveRate()
- Set a minimum camera controller height
- Make invalid buildoption warning more verbose
- Early block check for build commands [PR 2557](https://github.com/beyond-all-reason/RecoilEngine/pull/2557)
- Fixed to guard against already-dead reclaim targets [PR 3020](https://github.com/beyond-all-reason/RecoilEngine/pull/3020)
- Fixed edge scrolling threshold [Issue 2987](https://github.com/beyond-all-reason/RecoilEngine/issues/2987)
- Dump state handles resource packs
- Fix units having the wrong path id after loading a save game. [PR 3120](https://github.com/beyond-all-reason/RecoilEngine/pull/3120)
- Avoid UB in float-to-short angle casts (fixes arm64/x86 desync) [PR 3075](https://github.com/beyond-all-reason/RecoilEngine/pull/3075)
- Send gameprogress packet on connection initialization [PR 2872](https://github.com/beyond-all-reason/RecoilEngine/pull/2872)
- Separate and make sonar and RADAR jamming function as they logicall should. [PR 2980](https://github.com/beyond-all-reason/RecoilEngine/pull/2980)
