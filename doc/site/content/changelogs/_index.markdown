+++
title = "Running changelog"
[cascade]
  [cascade.params]
    type = "docs"
+++

This is the bleeding-edge changelog since version 2026.09, for **pre-release 2026.11**. This is a minor release that contains mostly capabilities for efficient rendering and a bunch of fixes.

## Caveats

 * hovering a build command over a queued building will now color the build grid outline red, configurable under the `buildBoxCancel` cmdcolors entry.
 * `/unbindall` now immediately binds more keys, aimed at letting you manually type `/bind` commands. See below for details. Use the new `Spring.UnbindAll()` Lua function if you don't want any of that. Don't forget that `/unbindall` ran `/bind enter chat`.

### Deprecation notices

 * None yet

## Features

### Build commands

 * queued build cancellation and rejection can apply yardmaps after their existing rectangular checks, allowing the same compatible overlaps as sequential construction. Set `construction.useYardmapsForQueuedBuildOverlap = true` in `gamedata/modrules.lua` to enable yardmap-aware queue overlap; the default remains the previous rectangle-only behavior.
 * queued build outlines that would be removed by the previewed placement use the configurable `buildBoxCancel` command color, which defaults to red.
 * add `Spring.TestBuildOrderOverlap(queuedBuild, proposedBuild) → boolean overlaps, boolean cancels` to test a queued build against a proposed build. `overlaps` reports any conflict under the queued-build yardmap rules; `cancels` reports that the conflict is also inside the cancellation rectangle. Each build is `{unitDefID, x, y, z, facing}`.

### Misc

 * fixed a beamlaser related crash, likely it has just been downgraded into incorrect behaviour though. The effort is ongoing, there are logs for when this happens.
 * fix overlapping build grid previews causing a crash or visual flickering.
 * added `Spring.UnbindAll() → nil`, unbinds all keys. Unlike the `/unbindall` command it does not leave any "bootstrap" bindings in place.
 * the `/unbindall` command now binds the following afterwards:
     * enter → chat (as before)
     * enter → edit_return
     * escape → edit_escape
     * backspace → edit_backspace
     * delete → edit_delete
     * left → edit_prev_char
     * right → edit_next_char
