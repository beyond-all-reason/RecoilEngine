---
title: Map regions
---

A game can divide one map into rectangular regions. It can crop each player's view and configure synced boundaries for sensors and craters. Movement, orders, damage, and gameplay rules belong to the game.

## Local view

Check `Engine.FeatureSupport.mapRegionViews`. The unsynced setters take `x0, z0, x1, z1` in world elmos.

```lua
Spring.SetCameraBounds(0, 0, 4096, 4096)
Spring.SetMiniMapView(0, 0, 4096, 4096)
Spring.SetWorldDrawBounds(0, 0, 4096, 4096)
```

- `SetCameraBounds` limits camera focus and adjusts overhead zoom and overview to the region.
- `SetMiniMapView` crops the minimap and remaps its input. Widgets that draw in whole-map coordinates must account for the crop.
- `SetWorldDrawBounds` clips terrain and filters native objects and effects. Games handle their own Lua overlays.

Invalid rectangles return `false` and preserve the previous bounds. Call a setter without arguments to clear its bounds. Reapply local bounds when the UI starts after loading a save.

## Synced boundaries

Set these during game initialization. Rectangles are half-open, disjoint, and inside the map. Touching edges are allowed. Invalid geometry returns `false` and malformed arguments raise a Lua error. Both configurations persist in saves. Reapplying an identical configuration is allowed during a match. Empty configurations use the usual whole-map behavior.

### Sensors

Check `Engine.FeatureSupport.sensorRegions` and call `Spring.SetSensorRegions(rectangles)` before any sensor instance exists. Align the rectangles to the coarsest configured sensor cell.

Each emitter covers only its region. Emitters in a gap provide no coverage. Off-map emitters use the region at the nearest map position. Proximity decloaking and no-LOS nearest-enemy searches use the same region lookup. Global LOS, always-visible units, and spectator visibility still apply.

### Craters

Check `Engine.FeatureSupport.mapDamageRegions` and call `Spring.SetMapDamageRegions(rectangles)` before any crater is queued. Align the rectangles to terrain squares (8 elmos).

Native craters deform terrain strictly inside the impact region, leaving shared edge corners alone. They move buildings only when the corners affected by the building footprint are inside that region. Lua heightmap edits use their usual rules.
