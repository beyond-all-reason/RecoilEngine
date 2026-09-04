+++
title = "Running changelog"
[cascade]
  [cascade.params]
    type = "docs"
+++

This is the bleeding-edge changelog since version 2026.07, for **pre-release 2026.08**. This is a minor release with mostly fixes and internal cleanup, and one minor feature.

## Caveats and deprecation notices

None.

### "Spring" camera smoothness

The "spring" camera received changes aimed at improving the smoothmesh tracking mode (`CamSpringTrackMapHeightMode = 2`). The aim is to keep the advantages of smoothmesh when zoomed out and not have the camera get thrown around by every little bump when zoomed in.

 * added `CamSpringSmoothMeshBlendMinDist`, default 150 (in elmos). Below this height above ground, the smoothmesh tracking mode will follow the raw terrain. Takes precedence over the "max" below.
 * added `CamSpringSmoothMeshBlendMaxDist`, default 600 (in elmos). Above this height above ground, the smoothmesh tracking mode will follow the smoothmesh.
 * between these two heights, the camera will track an interpolation between the two relevant meshes.
 * tracking now applied for camera movement via zooming, dragging the window on the minimap, and Lua-based movement (previously only edge scrolling).
 * fixed interpolation issues with the existing tracking (over- and under-shooting, framerate dependence, visible jolts).

### Fixes
 * fixed that attempting loading a save would overflow MacOS' default stack size.
 * fixed `/skip` not doing anything before game start.
 * fixed `pr-downloader` triggering a refresh of rapid repos when downloading non-rapid items like maps or engines.
 * fixed `Spring.SetSunDirection` not notifying lighting consumers, leaving rendered terrain stale until another lighting update.
 * fixed grass not respecting `Spring.SetSunDirection` at all.
 * fixed `MultiScale` and `WaitForScale` missing from the LUS header.
 * fixed corrupted/incompatible saves causing a crash at load (still interrupts load, now with an error popup).
 * fixed `Spring.GetProfilerTimeRecord` crashing if given the 2nd boolean argument.
 * fixed some inaccuracies in the website documentation.
 * fixed `unitsync` leaking memory and possibly other resources on shutdown (Lua state was not cleaned up).
 * fixed `VAO:RemoveFromSubmission` having a huge overhead, no longer dependent on how many indices there are in the submission.
 * fixed atlas-related log spam with a minimized window.

### Internals relevant for engine devs
 * enforce power-of-2 memory alignment for analyzing the engine with Address Sanitizer.
 * CMake: make engine test setup optional via new `BUILD_TESTING` option, default enabled.
 * `DemoTool` now builds again.
 * fix various build issues on OpenBSD and Windows.
 * fix some failing unit tests on aarch64. Involves a creg change for loading yielded Lua coroutines.
