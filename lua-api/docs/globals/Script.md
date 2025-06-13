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

### Script.GetWatchUnit
---
```lua
function Script.GetWatchUnit(unitDefID: integer) -> watched boolean
```

@return `watched` - Watch status.





Query whether any callins are registered for a unitDefID.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L2172-L2180" target="_blank">source</a>]





@see Script.SetWatchUnit




### Script.GetWatchFeature
---
```lua
function Script.GetWatchFeature(featureDefID: integer) -> watched boolean
```

@return `watched` - `true` if callins are registered, otherwise `false`.





Query whether any callins are registered for a featureDefID.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L2185-L2193" target="_blank">source</a>]





@see Script.SetWatchFeature




### Script.GetWatchWeapon
---
```lua
function Script.GetWatchWeapon(weaponDefID: integer) -> watched boolean
```

@return `watched` - True if watch is enabled for any weaponDefID callins.





Query whether any callins are registered for a weaponDefID.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L2198-L2211" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L2213-L2221" target="_blank">source</a>]





@see Script.SetWatchExplosion




### Script.GetWatchProjectile
---
```lua
function Script.GetWatchProjectile(weaponDefID: integer) -> watched boolean
```

@return `watched` - `true` if callins are registered, otherwise `false`.





Query whether projectile callins are registered for a weaponDefID.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L2226-L2234" target="_blank">source</a>]





@see Script.SetWatchProjectile




### Script.GetWatchAllowTarget
---
```lua
function Script.GetWatchAllowTarget(weaponDefID: integer) -> watched boolean
```

@return `watched` - `true` if callins are registered, otherwise `false`.





Query whether weapon targeting callins are registered for a weaponDefID.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L2239-L2247" target="_blank">source</a>]





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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L2252-L2263" target="_blank">source</a>]





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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L2268-L2277" target="_blank">source</a>]





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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L2282-L2303" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L2305-L2314" target="_blank">source</a>]





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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L2319-L2329" target="_blank">source</a>]





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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandleSynced.cpp#L2334-L2345" target="_blank">source</a>]





@see SyncedCallins:AllowWeaponInterceptTarget




### Script.Kill
---
```lua
function Script.Kill(killMessage: string?)
```
@param `killMessage` - Kill message.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L215-L218" target="_blank">source</a>]








### Script.UpdateCallin
---
```lua
function Script.UpdateCallin(name: string)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L671-L674" target="_blank">source</a>]








### Script.GetName
---
```lua
function Script.GetName() -> name string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L4105-L4108" target="_blank">source</a>]








### Script.GetSynced
---
```lua
function Script.GetSynced() -> synced boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L4116-L4119" target="_blank">source</a>]








### Script.GetFullCtrl
---
```lua
function Script.GetFullCtrl() -> fullCtrl boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L4127-L4130" target="_blank">source</a>]








### Script.GetFullRead
---
```lua
function Script.GetFullRead() -> fullRead boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L4138-L4141" target="_blank">source</a>]








### Script.GetCtrlTeam
---
```lua
function Script.GetCtrlTeam() -> teamID integer
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L4149-L4152" target="_blank">source</a>]








### Script.GetReadTeam
---
```lua
function Script.GetReadTeam() -> teamID integer
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L4160-L4163" target="_blank">source</a>]








### Script.GetReadAllyTeam
---
```lua
function Script.GetReadAllyTeam() -> allyTeamID integer
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L4171-L4174" target="_blank">source</a>]








### Script.GetSelectTeam
---
```lua
function Script.GetSelectTeam() -> teamID integer
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L4182-L4185" target="_blank">source</a>]








### Script.GetGlobal
---
```lua
function Script.GetGlobal() -> global integer?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L4193-L4196" target="_blank">source</a>]








### Script.GetRegistry
---
```lua
function Script.GetRegistry() -> registry integer?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L4207-L4210" target="_blank">source</a>]








### Script.DelayByFrames
---
```lua
function Script.DelayByFrames(
  frameDelay: integer,
  fun: unknown
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L4227-L4231" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUtils.cpp#L527-L533" target="_blank">source</a>]











## fields
---

### Script.NO_ACCESS_TEAM
---
```lua
Script.NO_ACCESS_TEAM: integer = -1
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L4079-L4079" target="_blank">source</a>]








### Script.ALL_ACCESS_TEAM
---
```lua
Script.ALL_ACCESS_TEAM: integer = -2
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L4081-L4081" target="_blank">source</a>]










{% endraw %}