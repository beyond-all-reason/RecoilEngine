+++
title = "Running changelog"
[cascade]
  [cascade.params]
    type = "docs"
+++

This is the bleeding-edge changelog since version 2026.07, for **pre-release 2026.08**.

# Fixes
* The heading passed to `AimWeapon` is now derived from the horizontal part of the
shot direction. Since 2026.02 it was the unit-frame azimuth of the full launch vector,
which for steep (high-trajectory) shots on tilted ground swung by tens of degrees, so
scripts with a yaw arc check rejected targets the engine considered in arc and the
unit never fired (e.g. a Wolverine stopping on a slope during a ground attack).
