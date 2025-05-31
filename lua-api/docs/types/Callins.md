---
layout: default
title: Callins
parent: Lua API
permalink: lua-api/types/Callins
---

{% raw %}

# class Callins





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L73-L104" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L507-L510" target="_blank">source</a>]








### Callins.LoadCode
---
```lua
function Callins.LoadCode()
```





Called when the game is (re)loaded.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L512-L515" target="_blank">source</a>]








### Callins.Shutdown
---
```lua
function Callins.Shutdown() ->  nil
```





Called when the addon or the game is shutdown.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L538-L542" target="_blank">source</a>]








### Callins.GotChatMsg
---
```lua
function Callins.GotChatMsg(
  msg: string,
  playerID: integer
)
```





Called when a player issues a UI command e.g. types /foo or /luarules foo.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L560-L565" target="_blank">source</a>]








### Callins.Load
---
```lua
function Callins.Load(zipReader: table)
```





Called after `GamePreload` and before `GameStart`. See Lua_SaveLoad.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L592-L596" target="_blank">source</a>]








### Callins.GamePreload
---
```lua
function Callins.GamePreload()
```





Called before the 0 gameframe.

Is not called when a saved game is loaded.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L659-L664" target="_blank">source</a>]








### Callins.GameStart
---
```lua
function Callins.GameStart()
```





Called upon the start of the game.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L682-L687" target="_blank">source</a>]

Is not called when a saved game is loaded.








### Callins.GameOver
---
```lua
function Callins.GameOver(winningAllyTeams: number[])
```
@param `winningAllyTeams` - list of winning allyTeams, if empty the game result was undecided (like when dropping from an host).






Called when the game ends

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L705-L709" target="_blank">source</a>]








### Callins.GamePaused
---
```lua
function Callins.GamePaused(
  playerID: integer,
  paused: boolean
)
```





Called when the game is paused.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L732-L737" target="_blank">source</a>]








### Callins.GameFrame
---
```lua
function Callins.GameFrame(frame: number)
```
@param `frame` - Starts at frame 1






Called for every game simulation frame (30 per second).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L782-L786" target="_blank">source</a>]








### Callins.GameFramePost
---
```lua
function Callins.GameFramePost(frame: number)
```
@param `frame` - Starts at frame 1






Called at the end of every game simulation frame

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L816-L820" target="_blank">source</a>]








### Callins.GameID
---
```lua
function Callins.GameID(gameID: string)
```
@param `gameID` - encoded in hex.






Called once to deliver the gameID

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L840-L844" target="_blank">source</a>]








### Callins.TeamDied
---
```lua
function Callins.TeamDied(teamID: integer)
```





Called when a team dies (see `Spring.KillTeam`).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L871-L875" target="_blank">source</a>]








### Callins.TeamChanged
---
```lua
function Callins.TeamChanged(teamID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L895-L898" target="_blank">source</a>]








### Callins.PlayerChanged
---
```lua
function Callins.PlayerChanged(playerID: integer)
```





Called whenever a player's status changes e.g. becoming a spectator.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L918-L922" target="_blank">source</a>]








### Callins.PlayerAdded
---
```lua
function Callins.PlayerAdded(playerID: integer)
```





Called whenever a new player joins the game.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L942-L946" target="_blank">source</a>]








### Callins.PlayerRemoved
---
```lua
function Callins.PlayerRemoved(
  playerID: integer,
  reason: string
)
```





Called whenever a player is removed from the game.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L966-L971" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1016-L1023" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1047-L1053" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1061-L1070" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1095-L1101" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1110-L1119" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1143-L1153" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1181-L1188" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1210-L1217" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1239-L1245" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1253-L1263" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1287-L1297" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1317-L1330" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1365-L1372" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1396-L1408" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1432-L1438" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1448-L1461" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1515-L1525" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1534-L1544" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1552-L1564" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1573-L1583" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1597-L1605" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1629-L1637" target="_blank">source</a>]








### Callins.UnitEnteredUnderwater
---
```lua
function Callins.UnitEnteredUnderwater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1667-L1673" target="_blank">source</a>]








### Callins.UnitEnteredWater
---
```lua
function Callins.UnitEnteredWater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1681-L1687" target="_blank">source</a>]








### Callins.UnitLeftAir
---
```lua
function Callins.UnitLeftAir(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1695-L1702" target="_blank">source</a>]








### Callins.UnitLeftUnderwater
---
```lua
function Callins.UnitLeftUnderwater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1710-L1717" target="_blank">source</a>]








### Callins.UnitLeftWater
---
```lua
function Callins.UnitLeftWater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1724-L1731" target="_blank">source</a>]








### Callins.UnitEnteredAir
---
```lua
function Callins.UnitEnteredAir(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1739-L1746" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1754-L1761" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1769-L1776" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1784-L1791" target="_blank">source</a>]








### Callins.UnitFeatureCollision
---
```lua
function Callins.UnitFeatureCollision(
  colliderID: integer,
  collideeID: integer
)
```





Called when a unit collides with a feature.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1835-L1843" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1888-L1895" target="_blank">source</a>]








### Callins.UnitArrivedAtGoal
---
```lua
function Callins.UnitArrivedAtGoal(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1910-L1917" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1927-L1934" target="_blank">source</a>]








### Callins.FeatureCreated
---
```lua
function Callins.FeatureCreated(
  featureID: integer,
  allyTeamID: integer
)
```





Called when a feature is created.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1965-L1971" target="_blank">source</a>]








### Callins.FeatureDestroyed
---
```lua
function Callins.FeatureDestroyed(
  featureID: integer,
  allyTeamID: integer
)
```





Called when a feature is destroyed.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L1992-L1998" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2019-L2032" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2074-L2086" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2126-L2135" target="_blank">source</a>]





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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2182-L2199" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2243-L2253" target="_blank">source</a>]








### Callins.RecvLuaMsg
---
```lua
function Callins.RecvLuaMsg(
  msg: string,
  playerID: integer
)
```





Receives messages from unsynced sent via `Spring.SendLuaRulesMsg` or `Spring.SendLuaUIMsg`.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2278-L2283" target="_blank">source</a>]








### Callins.Save
---
```lua
function Callins.Save(zip: table)
```
@param `zip` - a userdatum representing the savegame zip file. See Lua_SaveLoad.






Called when a chat command '/save' or '/savegame' is received.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2414-L2418" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2440-L2447" target="_blank">source</a>]








### Callins.Update
---
```lua
function Callins.Update(dt: number)
```
@param `dt` - the time since the last update.






Called for every draw frame (including when the game is paused) and at least once per sim frame except when catching up.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2467-L2471" target="_blank">source</a>]








### Callins.ViewResize
---
```lua
function Callins.ViewResize(
  viewSizeX: number,
  viewSizeY: number
)
```





Called whenever the window is resized.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2486-L2491" target="_blank">source</a>]








### Callins.FontsChanged
---
```lua
function Callins.FontsChanged()
```





Called whenever fonts are updated. Signals the game display lists
and other caches should be discarded.

Gets called before other Update and Draw callins.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2525-L2531" target="_blank">source</a>]








### Callins.SunChanged
---
```lua
function Callins.SunChanged()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2546-L2548" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2562-L2567" target="_blank">source</a>]








### Callins.DrawGenesis
---
```lua
function Callins.DrawGenesis()
```





Use this callin to update textures, shaders, etc.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2654-L2660" target="_blank">source</a>]

Doesn't render to screen!
Also available to LuaMenu.








### Callins.DrawWorld
---
```lua
function Callins.DrawWorld()
```





Spring draws command queues, 'map stuff', and map marks.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2663-L2666" target="_blank">source</a>]








### Callins.DrawWorldPreUnit
---
```lua
function Callins.DrawWorldPreUnit()
```





Spring draws units, features, some water types, cloaked units, and the sun.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2669-L2672" target="_blank">source</a>]








### Callins.DrawPreDecals
---
```lua
function Callins.DrawPreDecals()
```





Called before decals are drawn

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2675-L2678" target="_blank">source</a>]








### Callins.DrawWaterPost
---
```lua
function Callins.DrawWaterPost()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2681-L2683" target="_blank">source</a>]








### Callins.DrawShadowPassTransparent
---
```lua
function Callins.DrawShadowPassTransparent()
```





Invoked after semi-transparent shadows pass is about to conclude

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2686-L2690" target="_blank">source</a>]

This callin has depth and color buffer of shadowmap bound via FBO as well as the FFP state to do "semi-transparent" shadows pass (traditionally only used to draw shadows of shadow casting semi-transparent particles). Can be used to draw nice colored shadows.








### Callins.DrawWorldShadow
---
```lua
function Callins.DrawWorldShadow()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2693-L2695" target="_blank">source</a>]








### Callins.DrawWorldReflection
---
```lua
function Callins.DrawWorldReflection()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2698-L2700" target="_blank">source</a>]








### Callins.DrawWorldRefraction
---
```lua
function Callins.DrawWorldRefraction()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2703-L2705" target="_blank">source</a>]








### Callins.DrawGroundPreForward
---
```lua
function Callins.DrawGroundPreForward()
```





Runs at the start of the forward pass when a custom map shader has been assigned via `Spring.SetMapShader` (convenient for setting uniforms).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2708-L2711" target="_blank">source</a>]








### Callins.DrawGroundPostForward
---
```lua
function Callins.DrawGroundPostForward()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2714-L2716" target="_blank">source</a>]








### Callins.DrawGroundPreDeferred
---
```lua
function Callins.DrawGroundPreDeferred()
```





Runs at the start of the deferred pass when a custom map shader has been assigned via `Spring.SetMapShader` (convenient for setting uniforms).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2719-L2722" target="_blank">source</a>]








### Callins.DrawGroundDeferred
---
```lua
function Callins.DrawGroundDeferred()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2725-L2727" target="_blank">source</a>]








### Callins.DrawGroundPostDeferred
---
```lua
function Callins.DrawGroundPostDeferred()
```





This runs at the end of its respective deferred pass.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2730-L2735" target="_blank">source</a>]

Allows proper frame compositing (with ground flashes/decals/foliage/etc, which are drawn between it and `DrawWorldPreUnit`) via `gl.CopyToTexture`.








### Callins.DrawUnitsPostDeferred
---
```lua
function Callins.DrawUnitsPostDeferred()
```





Runs at the end of the unit deferred pass.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2738-L2743" target="_blank">source</a>]

Informs Lua code it should make use of the $model_gbuffer_* textures before another pass overwrites them (and to allow proper blending with e.g. cloaked objects which are drawn between these events and DrawWorld via gl.CopyToTexture). N.B. The *PostDeferred events are only sent (and only have a real purpose) if forward drawing is disabled.








### Callins.DrawFeaturesPostDeferred
---
```lua
function Callins.DrawFeaturesPostDeferred()
```





Runs at the end of the feature deferred pass to inform Lua code it should make use of the $model_gbuffer_* textures before another pass overwrites them (and to allow proper blending with e.g. cloaked objects which are drawn between these events and DrawWorld via gl.CopyToTexture). N.B. The *PostDeferred events are only sent (and only have a real purpose) if forward drawing is disabled.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2746-L2749" target="_blank">source</a>]








### Callins.DrawShadowUnitsLua
---
```lua
function Callins.DrawShadowUnitsLua()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2752-L2754" target="_blank">source</a>]








### Callins.DrawShadowFeaturesLua
---
```lua
function Callins.DrawShadowFeaturesLua()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2757-L2759" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2762-L2771" target="_blank">source</a>]








### Callins.DrawScreen
---
```lua
function Callins.DrawScreen(
  viewSizeX: number,
  viewSizeY: number
)
```





Also available to LuaMenu.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2810-L2815" target="_blank">source</a>]








### Callins.DrawScreenEffects
---
```lua
function Callins.DrawScreenEffects(
  viewSizeX: number,
  viewSizeY: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2827-L2831" target="_blank">source</a>]








### Callins.DrawScreenPost
---
```lua
function Callins.DrawScreenPost(
  viewSizeX: number,
  viewSizeY: number
)
```





Similar to DrawScreenEffects, this can be used to alter the contents of a frame after it has been completely rendered (i.e. World, MiniMap, Menu, UI).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2843-L2851" target="_blank">source</a>]

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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2863-L2868" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2891-L2896" target="_blank">source</a>]








### Callins.GameProgress
---
```lua
function Callins.GameProgress(serverFrameNum: integer)
```





Called every 60 frames, calculating delta between `GameFrame` and `GameProgress`.

Can give an ETA about catching up with simulation for mid-game join players.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L2966-L2972" target="_blank">source</a>]








### Callins.KeyMapChanged
---
```lua
function Callins.KeyMapChanged()
```





Called when the keymap changes

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3010-L3015" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3051-L3065" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3119-L3131" target="_blank">source</a>]








### Callins.TextInput
---
```lua
function Callins.TextInput(utf8char: string)
```





Called whenever a key press results in text input.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3180-L3185" target="_blank">source</a>]








### Callins.TextEditing
---
```lua
function Callins.TextEditing(
  utf8: string,
  start: number,
  length: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3208-L3215" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3238-L3247" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3271-L3281" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3300-L3309" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3335-L3341" target="_blank">source</a>]








### Callins.IsAbove
---
```lua
function Callins.IsAbove(
  x: number,
  y: number
) -> isAbove boolean
```





Called every `Update`.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3362-L3371" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3393-L3399" target="_blank">source</a>]








### Callins.ActiveCommandChanged
---
```lua
function Callins.ActiveCommandChanged(
  cmdId: integer?,
  cmdType: integer?
)
```





Called when a command is issued.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3421-L3426" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3447-L3453" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3470-L3476" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3494-L3501" target="_blank">source</a>]








### Callins.AddConsoleLine
---
```lua
function Callins.AddConsoleLine(
  msg: string,
  priority: integer
)
```





Called when text is entered into the console (e.g. `Spring.Echo`).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3530-L3535" target="_blank">source</a>]








### Callins.GroupChanged
---
```lua
function Callins.GroupChanged(groupID: integer)
```





Called when a unit is added to or removed from a control group.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3553-L3557" target="_blank">source</a>]








### Callins.WorldTooltip
---
```lua
function Callins.WorldTooltip(
  type: "unit",
  unitId: integer
) -> tooltip string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3573-L3578" target="_blank">source</a>]








### Callins.WorldTooltip
---
```lua
function Callins.WorldTooltip(
  type: "feature",
  featureId: integer
) -> tooltip string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3579-L3584" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3585-L3592" target="_blank">source</a>]








### Callins.WorldTooltip
---
```lua
function Callins.WorldTooltip(type: "selection") -> tooltip string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3593-L3597" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3641-L3649" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3650-L3660" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3661-L3669" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3729-L3737" target="_blank">source</a>]








### Callins.RecvSkirmishAIMessage
---
```lua
function Callins.RecvSkirmishAIMessage(
  aiTeam: integer,
  dataStr: string
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3780-L3784" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3825-L3831" target="_blank">source</a>]








### Callins.DownloadStarted
---
```lua
function Callins.DownloadStarted(id: integer)
```





Called when a Pr-downloader download is started via VFS.DownloadArchive.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3853-L3857" target="_blank">source</a>]








### Callins.DownloadFinished
---
```lua
function Callins.DownloadFinished(id: integer)
```





Called when a Pr-downloader download finishes successfully.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3876-L3880" target="_blank">source</a>]








### Callins.DownloadFailed
---
```lua
function Callins.DownloadFailed(
  id: integer,
  errorID: integer
)
```





Called when a Pr-downloader download fails to complete.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3899-L3904" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b276dffed09b3b4c3036a2300d91d66b4a2dd589/rts/Lua/LuaHandle.cpp#L3924-L3930" target="_blank">source</a>]












{% endraw %}