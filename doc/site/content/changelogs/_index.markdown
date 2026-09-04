+++
title = "Running changelog"
[cascade]
  [cascade.params]
    type = "docs"
+++

This is the bleeding-edge changelog since version 2026.07, for **pre-release 2026.08**.

# Fixes
* The heading passed to `AimWeapon` is now derived from the horizontal part of the
shot direction. Since 2026.06 it was the unit-frame azimuth of the full launch vector,
which for steep (high-trajectory) shots on tilted ground swung by tens of degrees, so
scripts with a yaw arc check rejected targets the engine considered in arc and the
unit never fired (e.g. a Wolverine stopping on a slope during a ground attack).

# Additions
* `AimWeapon` additionally receives the unit-frame heading of the full shot direction,
the value 2026.06 passed as `heading`. It is the exact turret yaw that points the barrel
along a steep shot on tilted ground, but it depends on hull tilt and launch pitch, so
use it for turning the turret and keep arc checks on `heading`. LUS scripts get it as
an optional fourth argument, `AimWeapon(weaponNum, heading, pitch, launchHeading)`
(radians); COB scripts via `get WEAPON_LAUNCH_HEADING(weaponNum)` (141, COB angle
units). Existing scripts need no change.
