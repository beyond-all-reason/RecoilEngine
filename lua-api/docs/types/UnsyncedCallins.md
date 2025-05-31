---
layout: default
title: UnsyncedCallins
parent: Lua API
permalink: lua-api/types/UnsyncedCallins
---

{% raw %}

# class UnsyncedCallins





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L174-L177" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L185-L189" target="_blank">source</a>]








### UnsyncedCallins.DrawUnit
---
```lua
function UnsyncedCallins.DrawUnit(
  unitID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of units

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L216-L223" target="_blank">source</a>]



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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L253-L260" target="_blank">source</a>]



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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L289-L297" target="_blank">source</a>]



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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L328-L335" target="_blank">source</a>]



@deprecated Deprecated






### UnsyncedCallins.DrawMaterial
---
```lua
function UnsyncedCallins.DrawMaterial(
  uuid: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L366-L373" target="_blank">source</a>]



@deprecated Deprecated










{% endraw %}