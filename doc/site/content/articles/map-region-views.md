---
title: Map-region views, sensors and craters
---

Games can present several regions of a single map as separate worlds. Three
unsynced Lua setters control independent parts of the local view:

```lua
Spring.SetWorldDrawBounds(0, 0, 4096, 4096)
Spring.SetCameraBounds(0, 0, 4096, 4096)
Spring.SetMiniMapView(0, 0, 4096, 4096)
```

Check `Engine.FeatureSupport.mapRegionViews` before enabling a game mode that
requires these commands.

Coordinates are `x0 z0 x1 z1` in world elmos. All values must be finite, ordered,
and within the physical map. Invalid coordinates return false and leave the previous rectangle
unchanged. Each setter accepts no arguments or nil to restore normal whole-map behavior:

```lua
Spring.SetWorldDrawBounds()
Spring.SetCameraBounds()
Spring.SetMiniMapView()
```

`Spring.SetWorldDrawBounds` sets both the terrain and native object/effect bounds. It filters shadows, world icons and command queues by position, clips SMF terrain fragments and covers both native nano particle shader paths. Camera and minimap rectangles remain independent. Large billboards/models are included by their draw position, rather than cut into pieces at the rectangle edge. Lua models, custom shaders, decals and arbitrary widget geometry still require game-side filtering.

Games can give objects more room while keeping a fixed terrain edge. Check `Engine.FeatureSupport.objectDrawBounds`, then override only the object/effect rectangle after setting the world bounds:

```lua
Spring.SetWorldDrawBounds(0, 0, 4096, 4096)
Spring.SetObjectDrawBounds(-256, -256, 4352, 4352)
```

`Spring.SetObjectDrawBounds` changes the native object/effect filter without changing terrain, camera or minimap bounds. This includes unit/feature models, shadows, world icons, selection/command drawing, projectiles, particles, ground flashes and native nano particles. Unlike the other view setters, finite, ordered object coordinates may extend beyond the physical map, so aircraft, overshooting projectiles and displaced units can remain visible there. Invalid input preserves the previous state. A finite margin still has an outer drawing cutoff; games choose its size and must account for neighboring worlds and cubicle barriers.

No arguments or nil clears only the object filter, leaving the terrain edge in place. Calling `Spring.SetWorldDrawBounds` again resets both terrain and object bounds together, preserving its original behavior. The minimap remains independently cropped; this setter does not add edge indicators or alter minimap input. All drawing bounds are local presentation state, cleared during camera initialization and teardown; games reapply them when their UI initializes after loading a save.

`Spring.SetCameraBounds` constrains controller focus and interpolated camera focus, and uses
the region's size/centre for normal overhead zoom limits, reset and overview.
The camera can still look beyond the edge; pair it with terrain/render clipping
or the game's chosen background effect. Clear camera bounds at widget shutdown.

`Spring.SetMiniMapView` crops the background, maps world passes and icons to that rectangle,
and maps input/rays back into its coordinates. Rotation, direct drawing and the
cached minimap path are supported. Native range/command overlays use the minimap
rectangle independently of `Spring.SetWorldDrawBounds`. Lua minimap drawing remains in the
engine's documented callback coordinate spaces; games drawing full-map pixel
charts must transform their own charts for the crop. The engine retains clipping
when Lua resets drawing state. `gl.DrawMiniMapIcons` is independent of the built-in
minimap crop and continues to use its caller-supplied rectangle.

All three setters are local presentation state. They do not partition simulation,
LOS, pathfinding, collision, damage, projectiles, teams or map physics. A game must
implement portals, order routing and interaction isolation itself. Rendering
bounds do not provide a security or information-hiding boundary for Lua clients.
These are intentionally not console commands. Like other unsynced Lua controls,
they are not a protection against modified widgets or clients; rely on synced
rules and LOS for information hiding.
With no bounds set, existing games retain their normal whole-map behavior.

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

## Synced crater regions

Check `Engine.FeatureSupport.mapDamageRegions` and configure
`Spring.SetMapDamageRegions({{x0,z0,x1,z1}, ...})` from synced initialization.
The rectangles use the same half-open convention, but only require alignment to
terrain squares (8 elmos). No arguments, nil or an empty table restores whole-map
deformation. Invalid geometry returns false without changing state; malformed
argument types raise a Lua error. Calls after the first simulation frame begins or after any native crater is
queued are rejected. Configure before initialization code creates explosions. This API is unavailable to LuaUI and has no console command.

A native explosion deforms only height samples within its impact region. Impacts
outside every region produce no crater. The clipped rectangle is stored with the
queued explosion, so its delayed updates cannot modify the separating strip or a
neighbor. Buildings are moved with their flattened ground only when the entire
footprint lies within that region; foreign/straddling footprints are left alone.
The configuration is serialized as synced game state and reset between matches.
No configuration preserves existing whole-map behavior.

These bounds govern native explosion deformation only. Lua heightmap editing,
movement, projectile motion, object damage and script-created effects need their
own game rules. Combining terrain/build masks with those rules, native sensor
bounds and crater bounds permits compact separating strips without relying on
strip width to absorb every explosion. No particular game or packing layout is
encoded in the engine.

See `test/region_views/README.md` for graphical regressions and manual checks.
