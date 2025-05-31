---
layout: default
title: Script
parent: Lua API
permalink: lua-api/globals/Script
---

{% raw %}

# global Script








---

## methods
---

### Script.IsEngineMinVersion
---
```lua
function Script.IsEngineMinVersion(
  minMajorVer: integer,
  minMinorVer: integer?,
  minCommits: integer?
) -> satisfiesMin boolean
```
@param `minMinorVer` - (Default: `0`)

@param `minCommits` - (Default: `0`)


@return `satisfiesMin` - `true` if the engine version is greater or equal to the specified version, otherwise `false`.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaUtils.cpp#L527-L533" target="_blank">source</a>]








### Script.GetWatchUnit
---
```lua
function Script.GetWatchUnit(unitDefID: integer) -> watched boolean
```

@return `watched` - Watch status.





Query whether any callins are registered for a unitDefID.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L2170-L2178" target="_blank">source</a>]





@see Script.SetWatchUnit




### Script.GetWatchFeature
---
```lua
function Script.GetWatchFeature(featureDefID: integer) -> watched boolean
```

@return `watched` - `true` if callins are registered, otherwise `false`.





Query whether any callins are registered for a featureDefID.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L2183-L2191" target="_blank">source</a>]





@see Script.SetWatchFeature




### Script.GetWatchWeapon
---
```lua
function Script.GetWatchWeapon(weaponDefID: integer) -> watched boolean
```

@return `watched` - True if watch is enabled for any weaponDefID callins.





Query whether any callins are registered for a weaponDefID.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L2196-L2209" target="_blank">source</a>]

Same as calling:
```lua
Script.GetWatchExplosion(weaponDefID) or Script.GetWatchProjectile(weaponDefID) or Script.GetWatchAllowTarget(weaponDefID)
```





@see Script.SetWatchWeapon




### Script.GetWatchExplosion
---
```lua
function Script.GetWatchExplosion(weaponDefID: integer) -> watched boolean
```

@return `watched` - `true` if callins are registered, otherwise `false`.





Query whether explosion callins are registered for a weaponDefID.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L2211-L2219" target="_blank">source</a>]





@see Script.SetWatchExplosion




### Script.GetWatchProjectile
---
```lua
function Script.GetWatchProjectile(weaponDefID: integer) -> watched boolean
```

@return `watched` - `true` if callins are registered, otherwise `false`.





Query whether projectile callins are registered for a weaponDefID.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L2224-L2232" target="_blank">source</a>]





@see Script.SetWatchProjectile




### Script.GetWatchAllowTarget
---
```lua
function Script.GetWatchAllowTarget(weaponDefID: integer) -> watched boolean
```

@return `watched` - `true` if callins are registered, otherwise `false`.





Query whether weapon targeting callins are registered for a weaponDefID.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L2237-L2245" target="_blank">source</a>]





@see Script.SetWatchAllowTarget




### Script.SetWatchUnit
---
```lua
function Script.SetWatchUnit(
  unitDefID: integer,
  watch: boolean
)
```
@param `watch` - Whether to register or deregister.






Register or deregister unitDefID for expensive callins.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L2250-L2261" target="_blank">source</a>]





@see Callins:UnitMoveFailed




### Script.SetWatchFeature
---
```lua
function Script.SetWatchFeature(
  featureDefID: integer,
  watch: boolean
)
```
@param `watch` - Whether to register or deregister.






Register or deregister featureDefID for expensive callins.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L2266-L2275" target="_blank">source</a>]





@see Callins:UnitFeatureCollision




### Script.SetWatchWeapon
---
```lua
function Script.SetWatchWeapon(
  weaponDefID: integer,
  watch: boolean
)
```
@param `watch` - Whether to register or deregister.






Register or deregister weaponDefID for all expensive callins.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L2280-L2301" target="_blank">source</a>]

Equivalent to calling:

```lua
Script.SetWatchExplosion(weaponDefID)
Script.SetWatchProjectile(weaponDefID)
Script.SetWatchAllowTarget(weaponDefID)
```

Generally it's better to use those methods to avoid registering uneeded callins.





@see Script.SetWatchAllowTarget




### Script.SetWatchExplosion
---
```lua
function Script.SetWatchExplosion(
  weaponDefID: integer,
  watch: boolean
)
```
@param `watch` - Whether to register or deregister.






Register or deregister weaponDefID for explosion callins.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L2303-L2312" target="_blank">source</a>]





@see Callins:Explosion




### Script.SetWatchProjectile
---
```lua
function Script.SetWatchProjectile(
  weaponDefID: integer,
  watch: boolean
)
```
@param `weaponDefID` - weaponDefID for weapons or -1 to watch for debris.

@param `watch` - Whether to register or deregister.






Register or deregister weaponDefID for expensive projectile callins.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L2317-L2327" target="_blank">source</a>]





@see Callins:ProjectileDestroyed




### Script.SetWatchAllowTarget
---
```lua
function Script.SetWatchAllowTarget(
  weaponDefID: integer,
  watch: boolean
)
```
@param `watch` - Whether to register or deregister.






Register or deregister weaponDefID for weapon targeting callins.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandleSynced.cpp#L2332-L2343" target="_blank">source</a>]





@see SyncedCallins:AllowWeaponInterceptTarget




### Script.Kill
---
```lua
function Script.Kill(killMessage: string?)
```
@param `killMessage` - Kill message.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L213-L216" target="_blank">source</a>]








### Script.UpdateCallin
---
```lua
function Script.UpdateCallin(name: string)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L640-L643" target="_blank">source</a>]








### Script.GetName
---
```lua
function Script.GetName() -> name string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L4068-L4071" target="_blank">source</a>]








### Script.GetSynced
---
```lua
function Script.GetSynced() -> synced boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L4079-L4082" target="_blank">source</a>]








### Script.GetFullCtrl
---
```lua
function Script.GetFullCtrl() -> fullCtrl boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L4090-L4093" target="_blank">source</a>]








### Script.GetFullRead
---
```lua
function Script.GetFullRead() -> fullRead boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L4101-L4104" target="_blank">source</a>]








### Script.GetCtrlTeam
---
```lua
function Script.GetCtrlTeam() -> teamID integer
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L4112-L4115" target="_blank">source</a>]








### Script.GetReadTeam
---
```lua
function Script.GetReadTeam() -> teamID integer
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L4123-L4126" target="_blank">source</a>]








### Script.GetReadAllyTeam
---
```lua
function Script.GetReadAllyTeam() -> allyTeamID integer
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L4134-L4137" target="_blank">source</a>]








### Script.GetSelectTeam
---
```lua
function Script.GetSelectTeam() -> teamID integer
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L4145-L4148" target="_blank">source</a>]








### Script.GetGlobal
---
```lua
function Script.GetGlobal() -> global integer?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L4156-L4159" target="_blank">source</a>]








### Script.GetRegistry
---
```lua
function Script.GetRegistry() -> registry integer?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L4170-L4173" target="_blank">source</a>]








### Script.DelayByFrames
---
```lua
function Script.DelayByFrames(
  frameDelay: integer,
  fun: unknown
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L4190-L4194" target="_blank">source</a>]











## fields
---

### Script.NO_ACCESS_TEAM
---
```lua
Script.NO_ACCESS_TEAM: integer = -1
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L4048-L4048" target="_blank">source</a>]








### Script.ALL_ACCESS_TEAM
---
```lua
Script.ALL_ACCESS_TEAM: integer = -2
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L4050-L4050" target="_blank">source</a>]










{% endraw %}