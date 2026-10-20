# Map region fixtures

Use a Linux engine install, Python 3 and a C compiler. The runner generates a map and minimal game. Graphical cases use SDL's offscreen driver and default to 30 FPS.

```sh
python3 test/region_views/run.py --case camera --output /tmp/region-camera
python3 test/region_views/run.py --case draw --drawer 1 --output /tmp/region-draw
python3 test/region_views/run.py --case nano --nano instanced --output /tmp/region-nano
python3 test/region_views/run.py --case sensor --headless --save-load --output /tmp/region-sensors
python3 test/region_views/run.py --case crater --headless --clients 2 --output /tmp/region-craters
```

- **camera** checks camera limits, minimap crop and input across region switches.
- **draw** checks terrain and object clipping. Use drawer 0 for GLSL or 1 for GL4.
- **nano** checks particles emitted during construction. The `--nano` options are `legacy`, `geometry` and `instanced`.
- **sensor** checks coverage for all seven native sensors, including movement and jamming.
- **crater** checks terrain deformation and building movement at region edges.

Sensor and crater cases support `--regions whole-map` for a control, `--clients 2` for sync checks and `--save-load` for a native save round trip. Use `--headless` for simulation checks. The camera fixture's `--direct` option requires uncached minimap drawing support.

Use `--engine-dir` to choose a different install and a fresh `--output` directory for each run.
