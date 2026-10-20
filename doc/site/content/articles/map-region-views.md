---
title: Map-region views, sensors and craters
---

## Synced sensor regions

Sensor coverage can be partitioned independently of presentation. Check
`Engine.FeatureSupport.sensorRegions`, then configure the rectangles from synced
game initialization, before any unit sensor update:

```lua
assert(Spring.SetSensorRegions({
	{0, 0, 4096, 4096},
	{6144, 0, 10240, 4096},
}))
```

Coordinates are half-open world rectangles: the lower edges belong to the region,
the upper edges do not. Rectangles must be disjoint, inside the physical map and
have positive size. All coordinates must be finite integers aligned to the
coarsest configured sensor cell (its size depends on the game's sensor mip
settings). Touching rectangles are permitted. Invalid geometry returns false and
preserves the previous configuration; malformed argument types raise a Lua error.

Each emitter supplies ground LOS, air LOS, radar, sonar, seismic and radar/sonar
jamming only within its current region. An emitter outside every region supplies
no coverage. Region membership follows movement and teleportation through the
ordinary sensor update; the engine's existing update cadence and delayed removal
rules still apply. Explicit global LOS, always-visible units and spectator
privileges retain their usual meaning. Native proximity decloaking and no-LOS
nearest-enemy queries also consider only enemies in the search position's region.
Candidates are filtered before selecting the nearest, preserving local detection
when a foreign enemy would otherwise be closer.

The configuration is fixed for the match: calls after frame zero or after any
sensor instance exists return false. No arguments, nil or an empty table restores
whole-map coverage only during initialization. This restriction keeps cached
raycasts and delayed coverage removal consistent. Circle row spans and cached
raycast row spans are clipped to the same immutable bounds when adding/removing
coverage. The configuration is serialized with the game and used by newly
created emitters after loading. With no regions configured, sensors retain normal
whole-map behavior.

This API belongs to synced Lua control and is unavailable to LuaUI. It changes
sensor coverage, not movement, terrain deformation, weapons or damage. A game
still needs synced rules for its other region boundaries. The three view setters
above remain local, independent presentation controls.
