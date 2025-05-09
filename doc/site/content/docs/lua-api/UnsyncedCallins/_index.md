---
title: "UnsyncedCallins"
permalink: "docs/lua-api/UnsyncedCallins"
---
# class UnsyncedCallins





Functions called by the Engine (Unsynced).







---

## methods
---

### UnsyncedCallins.RecvFromSynced
---
```lua
function UnsyncedCallins.RecvFromSynced(...: any)
```





Receives data sent via `SendToUnsynced` callout.








### UnsyncedCallins.DrawUnit
---
```lua
function UnsyncedCallins.DrawUnit(
  unitID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of units



@deprecated Deprecated






### UnsyncedCallins.DrawFeature
---
```lua
function UnsyncedCallins.DrawFeature(
  featureID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of features



@deprecated Deprecated






### UnsyncedCallins.DrawShield
---
```lua
function UnsyncedCallins.DrawShield(
  featureID: integer,
  weaponID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of shields.



@deprecated Deprecated






### UnsyncedCallins.DrawProjectile
---
```lua
function UnsyncedCallins.DrawProjectile(
  projectileID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of weapon (& other) projectiles



@deprecated Deprecated






### UnsyncedCallins.DrawMaterial
---
```lua
function UnsyncedCallins.DrawMaterial(
  uuid: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```







@deprecated Deprecated






### UnsyncedCallins.RecvFromSynced
---
```lua
function UnsyncedCallins.RecvFromSynced(...: any)
```





Receives data sent via `SendToUnsynced` callout.








### UnsyncedCallins.DrawUnit
---
```lua
function UnsyncedCallins.DrawUnit(
  unitID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of units



@deprecated Deprecated






### UnsyncedCallins.DrawFeature
---
```lua
function UnsyncedCallins.DrawFeature(
  featureID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of features



@deprecated Deprecated






### UnsyncedCallins.DrawShield
---
```lua
function UnsyncedCallins.DrawShield(
  featureID: integer,
  weaponID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of shields.



@deprecated Deprecated






### UnsyncedCallins.DrawProjectile
---
```lua
function UnsyncedCallins.DrawProjectile(
  projectileID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of weapon (& other) projectiles



@deprecated Deprecated






### UnsyncedCallins.DrawMaterial
---
```lua
function UnsyncedCallins.DrawMaterial(
  uuid: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```







@deprecated Deprecated









