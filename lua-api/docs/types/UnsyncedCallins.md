---
layout: default
title: UnsyncedCallins
parent: Lua API
permalink: lua-api/types/UnsyncedCallins
---

{% raw %}

# class UnsyncedCallins





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L175-L178" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L186-L190" target="_blank">source</a>]








### UnsyncedCallins.DrawUnit
---
```lua
function UnsyncedCallins.DrawUnit(
  unitID: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





For custom rendering of units

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L217-L224" target="_blank">source</a>]



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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L254-L261" target="_blank">source</a>]



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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L290-L298" target="_blank">source</a>]



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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L329-L336" target="_blank">source</a>]



@deprecated Deprecated






### UnsyncedCallins.DrawMaterial
---
```lua
function UnsyncedCallins.DrawMaterial(
  uuid: integer,
  drawMode: number
) -> suppressEngineDraw boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L367-L374" target="_blank">source</a>]



@deprecated Deprecated










{% endraw %}