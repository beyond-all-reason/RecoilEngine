+++
title = "Running changelog"
[cascade]
  [cascade.params]
    type = "docs"
+++

This is the bleeding-edge changelog since version 2026.07, for **pre-release 2026.08**.

## Features

### Unit icon draw ordering
- add optional `drawOrder` number (default 0) to `gamedata/icontypes.lua` entries; when icon sorting is enabled, icons with higher `drawOrder` are drawn on top.
- add `UnitIconsSorted` config bool (default false). When enabled, unit icons in the world, screen ("icons as UI") and minimap draw paths are sorted by their icon's `drawOrder`, then back-to-front, so overlapping icons resolve in a deterministic, prioritized order instead of unit creation order.
- add optional 10th parameter `drawOrder` (default 0) to `Spring.AddUnitIcon`.
- `Spring.GetUnitIconData`, `Spring.GetIconData` and `Spring.GetAllIconDataArray` now include `drawOrder` when called with `fullData`.
