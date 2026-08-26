+++
title = "Running changelog"
[cascade]
  [cascade.params]
    type = "docs"
+++

This is the bleeding-edge changelog since version 2026.07, for **pre-release 2026.08**.

## Camera
- Add spring setting option `CamSpringTrackMapHeightMode=2`, this camera mode tracks the smooth air mesh rather than the terrain heightmap and applies a bicubic interpolation to give the camera a smooth transition between height changes as it moves across the map.
- Add spring setting option `CamSpringSmoothMeshBlendMinDist` and `CamSpringSmoothMeshBlendMaxDist`, which takes distance in float elmos. THis is used by camera mode 2 to smooth out the height of the camera when it zooms under the smooth air mesh. The camera's distance from the ground is clamped and blended against these two values (using the smoothstep function), this then provides the strength of how much the camera's height is determined by the real terrain value and the smooth air mesh's. The closer the camera gets to the min distance the more the real terrain height matters - so you follow and zoom in on cliff faces more closely for example.
