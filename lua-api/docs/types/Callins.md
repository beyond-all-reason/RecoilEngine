---
layout: default
title: Callins
parent: Lua API
permalink: lua-api/types/Callins
---

{% raw %}

# class Callins





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L75-L106" target="_blank">source</a>]

Functions called by the Engine.

To use these callins in a widget, prepend `widget:` and, for a gadget,
prepend `gadget:`. For example:

```lua
function widget:UnitCreated(unitID, unitDefID, unitTeam, builderID)
  -- ...
end
```

Some functions may differ between (synced) gadget and widgets. This is
because all information should be available to synced (game logic
controlling) gadgets, but restricted to unsynced gadget/widget. e.g.
information about an enemy unit only detected via radar and not yet in LOS.

In such cases the full (synced) param list is documented.

**Attention:** Some callins will only work on the unsynced portion of the gadget.
Due to the type-unsafe nature of lua parsing, those callins not firing up
might be hard to trace.







---

## methods
---

### Callins.Initialize
---
```lua
function Callins.Initialize()
```





Called when the addon is (re)loaded.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L509-L512" target="_blank">source</a>]








### Callins.LoadCode
---
```lua
function Callins.LoadCode()
```





Called when the game is (re)loaded.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L514-L517" target="_blank">source</a>]








### Callins.Shutdown
---
```lua
function Callins.Shutdown() ->  nil
```





Called when the addon or the game is shutdown.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L569-L573" target="_blank">source</a>]








### Callins.GotChatMsg
---
```lua
function Callins.GotChatMsg(
  msg: string,
  playerID: integer
)
```





Called when a player issues a UI command e.g. types /foo or /luarules foo.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L591-L596" target="_blank">source</a>]








### Callins.Load
---
```lua
function Callins.Load(zipReader: table)
```





Called after `GamePreload` and before `GameStart`. See Lua_SaveLoad.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L623-L627" target="_blank">source</a>]








### Callins.GamePreload
---
```lua
function Callins.GamePreload()
```





Called before the 0 gameframe.

Is not called when a saved game is loaded.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L690-L695" target="_blank">source</a>]








### Callins.GameStart
---
```lua
function Callins.GameStart()
```





Called upon the start of the game.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L713-L718" target="_blank">source</a>]

Is not called when a saved game is loaded.








### Callins.GameOver
---
```lua
function Callins.GameOver(winningAllyTeams: number[])
```
@param `winningAllyTeams` - list of winning allyTeams, if empty the game result was undecided (like when dropping from an host).






Called when the game ends

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L736-L740" target="_blank">source</a>]








### Callins.GamePaused
---
```lua
function Callins.GamePaused(
  playerID: integer,
  paused: boolean
)
```





Called when the game is paused.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L763-L768" target="_blank">source</a>]








### Callins.GameFrame
---
```lua
function Callins.GameFrame(frame: number)
```
@param `frame` - Starts at frame 1






Called for every game simulation frame (30 per second).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L813-L817" target="_blank">source</a>]








### Callins.GameFramePost
---
```lua
function Callins.GameFramePost(frame: number)
```
@param `frame` - Starts at frame 1






Called at the end of every game simulation frame

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L847-L851" target="_blank">source</a>]








### Callins.GameID
---
```lua
function Callins.GameID(gameID: string)
```
@param `gameID` - encoded in hex.






Called once to deliver the gameID

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L871-L875" target="_blank">source</a>]








### Callins.TeamDied
---
```lua
function Callins.TeamDied(teamID: integer)
```





Called when a team dies (see `Spring.KillTeam`).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L902-L906" target="_blank">source</a>]








### Callins.TeamChanged
---
```lua
function Callins.TeamChanged(teamID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L926-L929" target="_blank">source</a>]








### Callins.PlayerChanged
---
```lua
function Callins.PlayerChanged(playerID: integer)
```





Called whenever a player's status changes e.g. becoming a spectator.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L949-L953" target="_blank">source</a>]








### Callins.PlayerAdded
---
```lua
function Callins.PlayerAdded(playerID: integer)
```





Called whenever a new player joins the game.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L973-L977" target="_blank">source</a>]








### Callins.PlayerRemoved
---
```lua
function Callins.PlayerRemoved(
  playerID: integer,
  reason: string
)
```





Called whenever a player is removed from the game.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L997-L1002" target="_blank">source</a>]








### Callins.UnitCreated
---
```lua
function Callins.UnitCreated(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  builderID: integer?
)
```





Called at the moment the unit is created.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1047-L1054" target="_blank">source</a>]








### Callins.UnitFinished
---
```lua
function Callins.UnitFinished(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





Called at the moment the unit is completed.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1078-L1084" target="_blank">source</a>]








### Callins.UnitFromFactory
---
```lua
function Callins.UnitFromFactory(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  factID: integer,
  factDefID: integer,
  userOrders: boolean
)
```





Called when a factory finishes construction of a unit.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1092-L1101" target="_blank">source</a>]








### Callins.UnitReverseBuilt
---
```lua
function Callins.UnitReverseBuilt(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





Called when a living unit becomes a nanoframe again.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1126-L1132" target="_blank">source</a>]








### Callins.UnitConstructionDecayed
---
```lua
function Callins.UnitConstructionDecayed(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  timeSinceLastBuild: number,
  iterationPeriod: number,
  part: number
)
```





Called when a unit being built starts decaying.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1141-L1150" target="_blank">source</a>]








### Callins.UnitDestroyed
---
```lua
function Callins.UnitDestroyed(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  attackerID: integer,
  attackerDefID: integer,
  attackerTeam: number,
  weaponDefID: integer
)
```





Called when a unit is destroyed.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1174-L1184" target="_blank">source</a>]








### Callins.UnitTaken
---
```lua
function Callins.UnitTaken(
  unitID: integer,
  unitDefID: integer,
  oldTeam: number,
  newTeam: number
)
```





Called when a unit is transferred between teams. This is called before `UnitGiven` and in that moment unit is still assigned to the oldTeam.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1212-L1219" target="_blank">source</a>]








### Callins.UnitGiven
---
```lua
function Callins.UnitGiven(
  unitID: integer,
  unitDefID: integer,
  newTeam: number,
  oldTeam: number
)
```





Called when a unit is transferred between teams. This is called after `UnitTaken` and in that moment unit is assigned to the newTeam.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1241-L1248" target="_blank">source</a>]








### Callins.UnitIdle
---
```lua
function Callins.UnitIdle(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





Called when a unit is idle (empty command queue).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1270-L1276" target="_blank">source</a>]








### Callins.UnitCommand
---
```lua
function Callins.UnitCommand(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  cmdID: integer,
  cmdParams: table,
  options: CommandOptions,
  cmdTag: number
)
```





Called after when a unit accepts a command, after `AllowCommand` returns true.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1284-L1294" target="_blank">source</a>]








### Callins.UnitCmdDone
---
```lua
function Callins.UnitCmdDone(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  cmdID: integer,
  cmdParams: table,
  options: CommandOptions,
  cmdTag: number
)
```





Called when a unit completes a command.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1318-L1328" target="_blank">source</a>]








### Callins.UnitDamaged
---
```lua
function Callins.UnitDamaged(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  damage: number,
  paralyzer: number,
  weaponDefID: integer,
  projectileID: integer,
  attackerID: integer,
  attackerDefID: integer,
  attackerTeam: number
)
```





Called when a unit is damaged (after UnitPreDamaged).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1348-L1361" target="_blank">source</a>]








### Callins.UnitStunned
---
```lua
function Callins.UnitStunned(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  stunned: boolean
)
```





Called when a unit changes its stun status.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1396-L1403" target="_blank">source</a>]








### Callins.UnitExperience
---
```lua
function Callins.UnitExperience(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  experience: number,
  oldExperience: number
)
```





Called when a unit gains experience greater or equal to the minimum limit set by calling `Spring.SetExperienceGrade`.

Should be called more reliably with small values of experience grade.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1427-L1439" target="_blank">source</a>]








### Callins.UnitHarvestStorageFull
---
```lua
function Callins.UnitHarvestStorageFull(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





Called when a unit's harvestStorage is full (according to its unitDef's entry).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1463-L1469" target="_blank">source</a>]








### Callins.UnitSeismicPing
---
```lua
function Callins.UnitSeismicPing(
  x: number,
  y: number,
  z: number,
  strength: number,
  allyTeam: integer,
  unitID: integer,
  unitDefID: integer
)
```





Called when a unit emits a seismic ping.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1479-L1492" target="_blank">source</a>]

See `seismicSignature`.








### Callins.UnitEnteredRadar
---
```lua
function Callins.UnitEnteredRadar(
  unitID: integer,
  unitTeam: integer,
  allyTeam: integer,
  unitDefID: integer
)
```





Called when a unit enters radar of an allyteam.

Also called when a unit enters LOS without any radar coverage.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1546-L1556" target="_blank">source</a>]








### Callins.UnitEnteredLos
---
```lua
function Callins.UnitEnteredLos(
  unitID: integer,
  unitTeam: integer,
  allyTeam: integer,
  unitDefID: integer
)
```
@param `allyTeam` - who's LOS the unit entered.






Called when a unit enters LOS of an allyteam.

Its called after the unit is in LOS, so you can query that unit.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1565-L1575" target="_blank">source</a>]








### Callins.UnitLeftRadar
---
```lua
function Callins.UnitLeftRadar(
  unitID: integer,
  unitTeam: integer,
  allyTeam: integer,
  unitDefID: integer
)
```





Called when a unit leaves radar of an allyteam.

Also called when a unit leaves LOS without any radar coverage.
For widgets, this is called just after a unit leaves radar coverage, so
widgets cannot get the position of units that left their radar.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1583-L1595" target="_blank">source</a>]








### Callins.UnitLeftLos
---
```lua
function Callins.UnitLeftLos(
  unitID: integer,
  unitTeam: integer,
  allyTeam: integer,
  unitDefID: integer
)
```





Called when a unit leaves LOS of an allyteam.

For widgets, this one is called just before the unit leaves los, so you can still get the position of a unit that left los.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1604-L1614" target="_blank">source</a>]








### Callins.UnitLoaded
---
```lua
function Callins.UnitLoaded(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  transportID: integer,
  transportTeam: integer
)
```





Called when a unit is loaded by a transport.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1628-L1636" target="_blank">source</a>]








### Callins.UnitUnloaded
---
```lua
function Callins.UnitUnloaded(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  transportID: integer,
  transportTeam: integer
)
```





Called when a unit is unloaded by a transport.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1660-L1668" target="_blank">source</a>]








### Callins.UnitEnteredUnderwater
---
```lua
function Callins.UnitEnteredUnderwater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1698-L1704" target="_blank">source</a>]








### Callins.UnitEnteredWater
---
```lua
function Callins.UnitEnteredWater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1712-L1718" target="_blank">source</a>]








### Callins.UnitLeftAir
---
```lua
function Callins.UnitLeftAir(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1726-L1733" target="_blank">source</a>]








### Callins.UnitLeftUnderwater
---
```lua
function Callins.UnitLeftUnderwater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1741-L1748" target="_blank">source</a>]








### Callins.UnitLeftWater
---
```lua
function Callins.UnitLeftWater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1755-L1762" target="_blank">source</a>]








### Callins.UnitEnteredAir
---
```lua
function Callins.UnitEnteredAir(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1770-L1777" target="_blank">source</a>]








### Callins.UnitCloaked
---
```lua
function Callins.UnitCloaked(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





Called when a unit cloaks.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1785-L1792" target="_blank">source</a>]








### Callins.UnitDecloaked
---
```lua
function Callins.UnitDecloaked(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





Called when a unit decloaks.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1800-L1807" target="_blank">source</a>]








### Callins.UnitUnitCollision
---
```lua
function Callins.UnitUnitCollision(
  colliderID: integer,
  collideeID: integer
)
```





Called when two units collide.

Both units must be registered with `Script.SetWatchUnit`.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1815-L1822" target="_blank">source</a>]








### Callins.UnitFeatureCollision
---
```lua
function Callins.UnitFeatureCollision(
  colliderID: integer,
  collideeID: integer
)
```





Called when a unit collides with a feature.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1866-L1874" target="_blank">source</a>]

The unit must be registered with `Script.SetWatchUnit` and the feature registered with `Script.SetWatchFeature`.








### Callins.UnitMoveFailed
---
```lua
function Callins.UnitMoveFailed(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1919-L1926" target="_blank">source</a>]








### Callins.UnitArrivedAtGoal
---
```lua
function Callins.UnitArrivedAtGoal(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1941-L1948" target="_blank">source</a>]








### Callins.RenderUnitDestroyed
---
```lua
function Callins.RenderUnitDestroyed(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





Called just before a unit is invalid, after it finishes its death animation.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1958-L1965" target="_blank">source</a>]








### Callins.FeatureCreated
---
```lua
function Callins.FeatureCreated(
  featureID: integer,
  allyTeamID: integer
)
```





Called when a feature is created.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L1996-L2002" target="_blank">source</a>]








### Callins.FeatureDestroyed
---
```lua
function Callins.FeatureDestroyed(
  featureID: integer,
  allyTeamID: integer
)
```





Called when a feature is destroyed.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2023-L2029" target="_blank">source</a>]








### Callins.FeatureDamaged
---
```lua
function Callins.FeatureDamaged(
  featureID: integer,
  featureDefID: integer,
  featureTeam: number,
  damage: number,
  weaponDefID: integer,
  projectileID: integer,
  attackerID: integer,
  attackerDefID: integer,
  attackerTeam: number
)
```





Called when a feature is damaged.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2050-L2063" target="_blank">source</a>]








### Callins.ProjectileCreated
---
```lua
function Callins.ProjectileCreated(
  proID: integer,
  proOwnerID: integer,
  weaponDefID: integer
)
```





Called when the projectile is created.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2105-L2117" target="_blank">source</a>]

Note that weaponDefID is missing if the projectile is spawned as part of a burst, but `Spring.GetProjectileDefID` and `Spring.GetProjectileName` still work in callin scope using proID.





@see Script.SetWatchWeapon




### Callins.ProjectileDestroyed
---
```lua
function Callins.ProjectileDestroyed(
  proID: integer,
  ownerID: integer,
  proWeaponDefID: integer
)
```





Called when the projectile is destroyed.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2157-L2166" target="_blank">source</a>]





@see Script.SetWatchWeapon




### Callins.Explosion
---
```lua
function Callins.Explosion(
  weaponDefID: integer,
  px: number,
  py: number,
  pz: number,
  attackerID: integer,
  projectileID: integer
) -> noGfx boolean
```

@return `noGfx` - if then no graphical effects are drawn by the engine for this explosion.





Called when an explosion occurs.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2213-L2230" target="_blank">source</a>]

Only called for weaponDefIDs registered via Script.SetWatchExplosion or Script.SetWatchWeapon.





@see Script.SetWatchWeapon




### Callins.StockpileChanged
---
```lua
function Callins.StockpileChanged(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  weaponNum: integer,
  oldCount: integer,
  newCount: integer
)
```





Called when a units stockpile of weapons increases or decreases.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2274-L2284" target="_blank">source</a>]








### Callins.RecvLuaMsg
---
```lua
function Callins.RecvLuaMsg(
  msg: string,
  playerID: integer
)
```





Receives messages from unsynced sent via `Spring.SendLuaRulesMsg` or `Spring.SendLuaUIMsg`.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2309-L2314" target="_blank">source</a>]








### Callins.Save
---
```lua
function Callins.Save(zip: table)
```
@param `zip` - a userdatum representing the savegame zip file. See Lua_SaveLoad.






Called when a chat command '/save' or '/savegame' is received.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2445-L2449" target="_blank">source</a>]








### Callins.UnsyncedHeightMapUpdate
---
```lua
function Callins.UnsyncedHeightMapUpdate()
 -> x1 number
 -> z1 number
 -> x2 number
 -> z2 number

```





Called when the unsynced copy of the height-map is altered.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2471-L2478" target="_blank">source</a>]








### Callins.Update
---
```lua
function Callins.Update(dt: number)
```
@param `dt` - the time since the last update.






Called for every draw frame (including when the game is paused) and at least once per sim frame except when catching up.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2498-L2502" target="_blank">source</a>]








### Callins.ViewResize
---
```lua
function Callins.ViewResize(
  viewSizeX: number,
  viewSizeY: number
)
```





Called whenever the window is resized.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2517-L2522" target="_blank">source</a>]








### Callins.FontsChanged
---
```lua
function Callins.FontsChanged()
```





Called whenever fonts are updated. Signals the game display lists
and other caches should be discarded.

Gets called before other Update and Draw callins.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2556-L2562" target="_blank">source</a>]








### Callins.SunChanged
---
```lua
function Callins.SunChanged()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2577-L2579" target="_blank">source</a>]








### Callins.DefaultCommand
---
```lua
function Callins.DefaultCommand(
  type: ("unit"|"feature"),
  id: integer
)
```
@param `type` - The type of the object pointed at.

@param `id` - The `unitID` or `featureID`.






Used to set the default command when a unit is selected.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2593-L2598" target="_blank">source</a>]








### Callins.DrawGenesis
---
```lua
function Callins.DrawGenesis()
```





Use this callin to update textures, shaders, etc.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2685-L2691" target="_blank">source</a>]

Doesn't render to screen!
Also available to LuaMenu.








### Callins.DrawWorld
---
```lua
function Callins.DrawWorld()
```





Spring draws command queues, 'map stuff', and map marks.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2694-L2697" target="_blank">source</a>]








### Callins.DrawWorldPreUnit
---
```lua
function Callins.DrawWorldPreUnit()
```





Spring draws units, features, some water types, cloaked units, and the sun.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2700-L2703" target="_blank">source</a>]








### Callins.DrawPreDecals
---
```lua
function Callins.DrawPreDecals()
```





Called before decals are drawn

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2706-L2709" target="_blank">source</a>]








### Callins.DrawWaterPost
---
```lua
function Callins.DrawWaterPost()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2712-L2714" target="_blank">source</a>]








### Callins.DrawShadowPassTransparent
---
```lua
function Callins.DrawShadowPassTransparent()
```





Invoked after semi-transparent shadows pass is about to conclude

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2717-L2721" target="_blank">source</a>]

This callin has depth and color buffer of shadowmap bound via FBO as well as the FFP state to do "semi-transparent" shadows pass (traditionally only used to draw shadows of shadow casting semi-transparent particles). Can be used to draw nice colored shadows.








### Callins.DrawWorldShadow
---
```lua
function Callins.DrawWorldShadow()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2724-L2726" target="_blank">source</a>]








### Callins.DrawWorldReflection
---
```lua
function Callins.DrawWorldReflection()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2729-L2731" target="_blank">source</a>]








### Callins.DrawWorldRefraction
---
```lua
function Callins.DrawWorldRefraction()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2734-L2736" target="_blank">source</a>]








### Callins.DrawGroundPreForward
---
```lua
function Callins.DrawGroundPreForward()
```





Runs at the start of the forward pass when a custom map shader has been assigned via `Spring.SetMapShader` (convenient for setting uniforms).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2739-L2742" target="_blank">source</a>]








### Callins.DrawGroundPostForward
---
```lua
function Callins.DrawGroundPostForward()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2745-L2747" target="_blank">source</a>]








### Callins.DrawGroundPreDeferred
---
```lua
function Callins.DrawGroundPreDeferred()
```





Runs at the start of the deferred pass when a custom map shader has been assigned via `Spring.SetMapShader` (convenient for setting uniforms).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2750-L2753" target="_blank">source</a>]








### Callins.DrawGroundDeferred
---
```lua
function Callins.DrawGroundDeferred()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2756-L2758" target="_blank">source</a>]








### Callins.DrawGroundPostDeferred
---
```lua
function Callins.DrawGroundPostDeferred()
```





This runs at the end of its respective deferred pass.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2761-L2766" target="_blank">source</a>]

Allows proper frame compositing (with ground flashes/decals/foliage/etc, which are drawn between it and `DrawWorldPreUnit`) via `gl.CopyToTexture`.








### Callins.DrawUnitsPostDeferred
---
```lua
function Callins.DrawUnitsPostDeferred()
```





Runs at the end of the unit deferred pass.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2769-L2774" target="_blank">source</a>]

Informs Lua code it should make use of the $model_gbuffer_* textures before another pass overwrites them (and to allow proper blending with e.g. cloaked objects which are drawn between these events and DrawWorld via gl.CopyToTexture). N.B. The *PostDeferred events are only sent (and only have a real purpose) if forward drawing is disabled.








### Callins.DrawFeaturesPostDeferred
---
```lua
function Callins.DrawFeaturesPostDeferred()
```





Runs at the end of the feature deferred pass to inform Lua code it should make use of the $model_gbuffer_* textures before another pass overwrites them (and to allow proper blending with e.g. cloaked objects which are drawn between these events and DrawWorld via gl.CopyToTexture). N.B. The *PostDeferred events are only sent (and only have a real purpose) if forward drawing is disabled.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2777-L2780" target="_blank">source</a>]








### Callins.DrawShadowUnitsLua
---
```lua
function Callins.DrawShadowUnitsLua()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2783-L2785" target="_blank">source</a>]








### Callins.DrawShadowFeaturesLua
---
```lua
function Callins.DrawShadowFeaturesLua()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2788-L2790" target="_blank">source</a>]








### Callins.DrawWorldPreParticles
---
```lua
function Callins.DrawWorldPreParticles(
  drawAboveWater: boolean,
  drawBelowWater: boolean,
  drawReflection: boolean,
  drawRefraction: boolean
)
```





DrawWorldPreParticles is called multiples times per draw frame.
Each call has a different permutation of values for drawAboveWater, drawBelowWater, drawReflection, and drawRefraction.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2793-L2802" target="_blank">source</a>]








### Callins.DrawScreen
---
```lua
function Callins.DrawScreen(
  viewSizeX: number,
  viewSizeY: number
)
```





Also available to LuaMenu.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2841-L2846" target="_blank">source</a>]








### Callins.DrawScreenEffects
---
```lua
function Callins.DrawScreenEffects(
  viewSizeX: number,
  viewSizeY: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2858-L2862" target="_blank">source</a>]








### Callins.DrawScreenPost
---
```lua
function Callins.DrawScreenPost(
  viewSizeX: number,
  viewSizeY: number
)
```





Similar to DrawScreenEffects, this can be used to alter the contents of a frame after it has been completely rendered (i.e. World, MiniMap, Menu, UI).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2874-L2882" target="_blank">source</a>]

Note: This callin is invoked after the software rendered cursor (configuration variable HardwareCursor=0) is drawn.








### Callins.DrawInMiniMap
---
```lua
function Callins.DrawInMiniMap(
  sx: number,
  sy: number
)
```
@param `sx` - relative to the minimap's position and scale.

@param `sy` - relative to the minimap's position and scale.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2894-L2899" target="_blank">source</a>]








### Callins.DrawInMiniMapBackground
---
```lua
function Callins.DrawInMiniMapBackground(
  sx: number,
  sy: number
)
```
@param `sx` - relative to the minimap's position and scale.

@param `sy` - relative to the minimap's position and scale.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2922-L2927" target="_blank">source</a>]








### Callins.GameProgress
---
```lua
function Callins.GameProgress(serverFrameNum: integer)
```





Called every 60 frames, calculating delta between `GameFrame` and `GameProgress`.

Can give an ETA about catching up with simulation for mid-game join players.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L2997-L3003" target="_blank">source</a>]








### Callins.KeyMapChanged
---
```lua
function Callins.KeyMapChanged()
```





Called when the keymap changes

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3041-L3046" target="_blank">source</a>]

Can be caused due to a change in language or keyboard








### Callins.KeyPress
---
```lua
function Callins.KeyPress(
  keyCode: number,
  mods: KeyModifiers,
  isRepeat: boolean,
  label: boolean,
  utf32char: number,
  scanCode: number,
  actionList: table
) -> halt boolean
```
@param `isRepeat` - If you want an action to occur only once check for isRepeat == false.

@param `label` - the name of the key

@param `utf32char` - (deprecated) always 0

@param `actionList` - the list of actions for this keypress


@return `halt` - whether to halt the chain for consumers of the keypress





Called repeatedly when a key is pressed down.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3082-L3096" target="_blank">source</a>]

Return true if you don't want other callins or the engine to also receive this keypress. A list of key codes can be seen at the SDL wiki.








### Callins.KeyRelease
---
```lua
function Callins.KeyRelease(
  keyCode: number,
  mods: KeyModifiers,
  label: boolean,
  utf32char: number,
  scanCode: number,
  actionList: table
) ->  boolean
```
@param `label` - the name of the key

@param `utf32char` - (deprecated) always 0

@param `actionList` - the list of actions for this keyrelease






Called when the key is released.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3150-L3162" target="_blank">source</a>]








### Callins.TextInput
---
```lua
function Callins.TextInput(utf8char: string)
```





Called whenever a key press results in text input.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3211-L3216" target="_blank">source</a>]








### Callins.TextEditing
---
```lua
function Callins.TextEditing(
  utf8: string,
  start: number,
  length: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3239-L3246" target="_blank">source</a>]








### Callins.MousePress
---
```lua
function Callins.MousePress(
  x: number,
  y: number,
  button: number
) -> becomeMouseOwner boolean
```





Called when a mouse button is pressed.

The button parameter supports up to 7 buttons. Must return true for `MouseRelease` and other functions to be called.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3269-L3278" target="_blank">source</a>]








### Callins.MouseRelease
---
```lua
function Callins.MouseRelease(
  x: number,
  y: number,
  button: number
) -> becomeMouseOwner boolean
```





Called when a mouse button is released.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3302-L3312" target="_blank">source</a>]

Please note that in order to have Spring call `Spring.MouseRelease`, you need to have a `Spring.MousePress` call-in in the same addon that returns true.








### Callins.MouseMove
---
```lua
function Callins.MouseMove(
  x: number,
  y: number,
  dx: number,
  dy: number,
  button: number
)
```
@param `x` - final x position

@param `y` - final y position

@param `dx` - distance travelled in x

@param `dy` - distance travelled in y






Called when the mouse is moved.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3331-L3340" target="_blank">source</a>]








### Callins.MouseWheel
---
```lua
function Callins.MouseWheel(
  up: boolean,
  value: number
)
```
@param `up` - the direction

@param `value` - the amount travelled






Called when the mouse wheel is moved.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3366-L3372" target="_blank">source</a>]








### Callins.IsAbove
---
```lua
function Callins.IsAbove(
  x: number,
  y: number
) -> isAbove boolean
```





Called every `Update`.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3393-L3402" target="_blank">source</a>]

Must return true for `Mouse*` events and `Spring.GetToolTip` to be called.








### Callins.GetTooltip
---
```lua
function Callins.GetTooltip(
  x: number,
  y: number
) -> tooltip string
```





Called when `Spring.IsAbove` returns true.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3424-L3430" target="_blank">source</a>]








### Callins.ActiveCommandChanged
---
```lua
function Callins.ActiveCommandChanged(
  cmdId: integer?,
  cmdType: integer?
)
```





Called when a command is issued.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3452-L3457" target="_blank">source</a>]








### Callins.CameraRotationChanged
---
```lua
function Callins.CameraRotationChanged(
  rotX: number,
  rotY: number,
  rotZ: number
)
```
@param `rotX` - Camera rotation around the x axis in radians.

@param `rotY` - Camera rotation around the y axis in radians.

@param `rotZ` - Camera rotation around the z axis in radians.






Called whenever the camera rotation changes

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3478-L3484" target="_blank">source</a>]








### Callins.CameraPositionChanged
---
```lua
function Callins.CameraPositionChanged(
  posX: number,
  posY: number,
  posZ: number
)
```
@param `posX` - Camera position x in world coordinates

@param `posY` - Camera position y in world coordinates

@param `posZ` - Camera position z in world coordinates






Called whenever the camera position changes

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3501-L3507" target="_blank">source</a>]








### Callins.CommandNotify
---
```lua
function Callins.CommandNotify(
  cmdID: integer,
  cmdParams: table,
  options: CommandOptions
) -> Returning boolean
```

@return `Returning` - true deletes the command and does not send it through the network.





Called when a command is issued.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3525-L3532" target="_blank">source</a>]








### Callins.AddConsoleLine
---
```lua
function Callins.AddConsoleLine(
  msg: string,
  priority: integer
)
```





Called when text is entered into the console (e.g. `Spring.Echo`).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3561-L3566" target="_blank">source</a>]








### Callins.GroupChanged
---
```lua
function Callins.GroupChanged(groupID: integer)
```





Called when a unit is added to or removed from a control group.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3584-L3588" target="_blank">source</a>]








### Callins.WorldTooltip
---
```lua
function Callins.WorldTooltip(
  type: "unit",
  unitId: integer
) -> tooltip string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3604-L3609" target="_blank">source</a>]








### Callins.WorldTooltip
---
```lua
function Callins.WorldTooltip(
  type: "feature",
  featureId: integer
) -> tooltip string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3610-L3615" target="_blank">source</a>]








### Callins.WorldTooltip
---
```lua
function Callins.WorldTooltip(
  type: "ground",
  posX: number,
  posY: number,
  posZ: number
) -> tooltip string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3616-L3623" target="_blank">source</a>]








### Callins.WorldTooltip
---
```lua
function Callins.WorldTooltip(type: "selection") -> tooltip string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3624-L3628" target="_blank">source</a>]








### Callins.MapDrawCmd
---
```lua
function Callins.MapDrawCmd(
  playerID: integer,
  type: "point",
  posX: number,
  posY: number,
  posZ: number,
  label: string
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3672-L3680" target="_blank">source</a>]








### Callins.MapDrawCmd
---
```lua
function Callins.MapDrawCmd(
  playerID: integer,
  type: "line",
  pos1X: number,
  pos1Y: number,
  pos1Z: number,
  pos2X: number,
  pos2Y: number,
  pos2Z: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3681-L3691" target="_blank">source</a>]








### Callins.MapDrawCmd
---
```lua
function Callins.MapDrawCmd(
  playerID: integer,
  type: "erase",
  posX: number,
  posY: number,
  posZ: number,
  radius: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3692-L3700" target="_blank">source</a>]








### Callins.GameSetup
---
```lua
function Callins.GameSetup(
  state: string,
  ready: boolean,
  playerStates: table
)
 -> success boolean
 -> newReady boolean

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3760-L3768" target="_blank">source</a>]








### Callins.RecvSkirmishAIMessage
---
```lua
function Callins.RecvSkirmishAIMessage(
  aiTeam: integer,
  dataStr: string
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3811-L3815" target="_blank">source</a>]








### Callins.DownloadQueued
---
```lua
function Callins.DownloadQueued(
  id: integer,
  name: string,
  type: string
)
```





Called when a Pr-downloader download is queued

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3856-L3862" target="_blank">source</a>]








### Callins.DownloadStarted
---
```lua
function Callins.DownloadStarted(id: integer)
```





Called when a Pr-downloader download is started via VFS.DownloadArchive.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3884-L3888" target="_blank">source</a>]








### Callins.DownloadFinished
---
```lua
function Callins.DownloadFinished(id: integer)
```





Called when a Pr-downloader download finishes successfully.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3907-L3911" target="_blank">source</a>]








### Callins.DownloadFailed
---
```lua
function Callins.DownloadFailed(
  id: integer,
  errorID: integer
)
```





Called when a Pr-downloader download fails to complete.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3930-L3935" target="_blank">source</a>]








### Callins.DownloadProgress
---
```lua
function Callins.DownloadProgress(
  id: integer,
  downloaded: integer,
  total: integer
)
```





Called incrementally during a Pr-downloader download.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaHandle.cpp#L3955-L3961" target="_blank">source</a>]












{% endraw %}