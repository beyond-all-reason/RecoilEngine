# Local map-region views

The graphical regression generates a native 10×8 flat SMF/SMT map and a minimal
game using the engine's bundled tree model. No downloads, game checkout, native
module, or generated-map extension is required. Run from a normal source checkout
with its own completed engine install and SDL's GPU offscreen driver. Python 3,
Pillow, and a C compiler are required. The Linux runner builds a temporary swap
limiter and caps drawing at 30 FPS; `--render-fps 0` disables the limit:

```sh
python3 test/region_views/run.py --case camera --output /tmp/view-cached
python3 test/region_views/run.py --case camera --direct --output /tmp/view-direct
python3 test/region_views/run.py --case draw --drawer 0 --output /tmp/draw-glsl
python3 test/region_views/run.py --case draw --drawer 1 --output /tmp/draw-gl4
```

Use `--engine-dir /path/to/install` for a different build. Output folders must not
already exist. Each run records its configuration, log, exit code and result.
Use `--case nano --nano geometry` and `--case nano --nano instanced` for real
construction emission, native lifecycle event assertions and pixels before,
during, after switching and after clearing the world crop. The draw case separately
exercises legacy CEGs. It also overrides object bounds independently of terrain, checks invalid input and both clearing behaviors, and places a native unit/projectile beyond the physical map edge. Their draw flags and pixels must return with padded object bounds while the terrain crop stays fixed. Both unit drawers run the same case.

The camera case covers all seven ordinary controllers, out-of-range camera
states/targets, zoom limits and reset keys, disabled optional map-clamp settings,
invalid input, clearing, switching, quarter-turn minimap rays, cropped background
pixels, icon filtering and clipping of oversized Lua minimap charts. Cached and
uncached minimap paths are separate runs. The draw case compares pixels and draw
flags before clipping, after each region switch, and after clearing: unit/feature
models, shadows, projectiles, simple particles and ground flashes. It also checks
that simulation objects survive unchanged. `test_ViewRectangle` covers finite/ordered/in-map geometry validation; the Lua
fixtures check invalid setters preserve the previous rectangle. The `flame` case
checks negative-size muzzle-flame culling, and `layout` checks nested Lua UI layout.
These two cases exercise the independent fixes in [#3579](https://github.com/beyond-all-reason/RecoilEngine/pull/3579) and [#3578](https://github.com/beyond-all-reason/RecoilEngine/pull/3578). They do not use region APIs and can fail on an otherwise correct region build without those companion fixes; they are not required to validate the region features.

The sensor fixture configures four disjoint regions and exercises all seven native
sensor types, including real seismic pings and jamming. It checks invalid/late
configuration, emitter movement through regions and the separating strip, removal
of old coverage, and absence of the synced setter from LuaUI. The whole-map run
is a control with the partition API unused:

```sh
python3 test/region_views/run.py --case sensor --output /tmp/sensors-isolated
python3 test/region_views/run.py --case sensor --sensors whole-map --output /tmp/sensors-control
python3 test/region_views/run.py --case sensor --clients 2 --output /tmp/sensors-network
python3 test/region_views/run.py --case sensor --save-load --output /tmp/sensors-load
```

Two-client mode launches separate engine processes with different local render
and minimap views. It compares bounded synced-state witnesses and requires clean
completion from both clients without native sync errors. Save/load mode saves the
minimal game, starts a fresh process, creates a new emitter and verifies that the
restored sensor partition still restricts its coverage. This is a native sensor
roundtrip, not full save/load coverage of any downstream game. Output contains all
process logs, witnesses, exit codes and the combined result.

Manual ZK/other-game check: in a combined map, set all three rectangles to one
region, select units, give move/fight/attack orders, pan/zoom and switch regions.
Verify terrain, units, particles, shadows, minimap icons and minimap orders use the
right region, then clear all three and verify the normal whole-map view returns.
Repeat with `MiniMapRenderToTexture=0`, both unit drawers, and native nano particles
while units construct. Custom Lua/GL4 drawing must apply the game's own filtering;
the engine cannot infer which arbitrary Lua geometry belongs to a region.

The `crater` case checks actual native terrain deformation and building movement
across a 512-elmo strip. Local terrain/buildings must change, while the strip,
foreign terrain and foreign/straddling flattened footprints must stay unchanged.
An impact in the strip must produce no deformation. The whole-map control proves
that the same native explosions would otherwise affect those locations.

```sh
python3 test/region_views/run.py --case crater --output /tmp/craters-isolated
python3 test/region_views/run.py --case crater --regions whole-map --output /tmp/craters-control
python3 test/region_views/run.py --case crater --clients 2 --output /tmp/craters-network
python3 test/region_views/run.py --case crater --save-load --output /tmp/craters-load
```

Crater save/load creates another explosion in the loaded process, testing restored
boundaries rather than only a saved heightmap. As with sensors, this minimal native
roundtrip does not establish complete downstream-game persistence.

The engine also has a generic CREG serialization unit test in `test/engine/System/LoadSave/testCregLoadSave.cpp`. That checks the serializer itself; the two `--save-load` fixtures above exercise the region configuration through real `.ssf` saves and separate native processes. No desktop game or downstream game installation is needed. The loaded sensor fixture creates a new emitter; the loaded crater fixture creates a new explosion and checks terrain/building effects on both sides of the boundary. These focused tests do not establish complete save/load correctness for BAR or Zero-K.
