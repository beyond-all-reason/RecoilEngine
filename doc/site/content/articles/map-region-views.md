---
title: Map regions
---

Games can restrict native sensors to regions of a single map.

## Synced boundaries

Set these during game initialization. Rectangles are half-open, disjoint, and inside the map. Touching edges are allowed. Invalid geometry returns `false` and malformed arguments raise a Lua error. The configuration persists in saves. Reapplying an identical configuration is allowed during a match. Empty configurations use the usual whole-map behavior.

### Sensors

Check `Engine.FeatureSupport.sensorRegions` and call `Spring.SetSensorRegions(rectangles)` before any sensor instance exists. Align the rectangles to the coarsest configured sensor cell.

Each emitter covers only its region. Emitters in a gap provide no coverage. Off-map emitters use the region at the nearest map position. Proximity decloaking and no-LOS nearest-enemy searches use the same region lookup. Global LOS, always-visible units, and spectator visibility still apply.
