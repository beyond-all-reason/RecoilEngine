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
jamming only within its current region. An emitter in the separating strip,
outside every region, supplies no coverage. An emitter beyond the physical map
edge, such as an aircraft flying past it, uses the region at the nearest map
position, so aircraft grazing the edge of a region that touches the map border
keep their sensors. Off-map movement alongside the map is not a barrier: a unit
that travels past the end of a strip takes the next region's sensors, so games
keep units out of foreign regions with their own rules. Region membership
follows movement and teleportation through the ordinary sensor update; the
engine's existing update cadence and delayed removal rules still apply. Explicit
global LOS, always-visible units and spectator privileges retain their usual
meaning. Native proximity decloaking and no-LOS nearest-enemy queries also
consider only enemies in the search position's region, using the same edge rule.
Candidates are filtered before selecting the nearest, preserving local detection
when a foreign enemy would otherwise be closer.

The configuration is fixed for the match: changes after frame zero or after any
sensor instance exists return false. Reapplying the current configuration is
accepted at any time and changes nothing, so synced code that runs its setup
again, for example after `/luarules reload`, can repeat the same call. Loading a
saved game restores the configuration with the rest of the synced state. No arguments, nil or an
empty table restores whole-map coverage only during initialization. This
restriction keeps cached raycasts and delayed coverage removal consistent.
Circle row spans and cached raycast row spans are clipped to the same immutable
bounds when adding/removing coverage. The configuration is serialized with the
game and used by newly created emitters after loading. With no regions
configured, sensors retain normal whole-map behavior.

This API belongs to synced Lua control and is unavailable to LuaUI. It changes
sensor coverage, not movement, terrain deformation, weapons or damage. A game
still needs synced rules for its other region boundaries. Local view and draw
bounds are separate, unsynced presentation controls.

## Synced crater regions

Check `Engine.FeatureSupport.mapDamageRegions` and configure
`Spring.SetMapDamageRegions({{x0,z0,x1,z1}, ...})` from synced initialization.
The rectangles use the same half-open convention, but only require alignment to
terrain squares (8 elmos). No arguments, nil or an empty table restores whole-map
deformation. Invalid geometry returns false without changing state; malformed
argument types raise a Lua error. Changes after the first simulation frame begins
or after any native crater is queued are rejected. Reapplying the current
configuration is accepted at any time and changes nothing. Configure before
initialization code creates explosions. This API is unavailable to LuaUI and has
no console command.

A native explosion deforms only height samples strictly inside its impact
region. Height samples lie on square corners, so a corner on a region edge is
shared with the terrain square outside the region; corners on both the lower and
upper edges are left unchanged. Impacts outside every region produce no crater.
The clipped rectangle is stored with the queued explosion, so its delayed
updates cannot modify the separating strip or a neighbor. Buildings are moved
with their flattened ground only when every corner they move lies strictly
inside that region: a footprint that starts on the region's lower edge, or that
belongs to a neighbor or straddles the boundary, is left alone. A footprint
ending on the upper edge only moves interior corners and is moved normally.
The configuration is serialized as synced game state and reset between matches.
No configuration preserves existing whole-map behavior.

These bounds govern native explosion deformation only. Lua heightmap editing,
movement, projectile motion, object damage and script-created effects need their
own game rules. Combining terrain/build masks with those rules, native sensor
bounds and crater bounds permits compact separating strips without relying on
strip width to absorb every explosion. No particular game or packing layout is
encoded in the engine.
