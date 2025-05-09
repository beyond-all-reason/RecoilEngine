---
layout: default
title: CMD
parent: Lua API
permalink: lua-api/types/CMD
---

{% raw %}

# enum CMD
---




### OPT_ALT

```lua
CMD.OPT_ALT = 128
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L30-L30" target="_blank">source</a>]








### OPT_CTRL

```lua
CMD.OPT_CTRL = 64
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L32-L32" target="_blank">source</a>]








### OPT_SHIFT

```lua
CMD.OPT_SHIFT = 32
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L34-L34" target="_blank">source</a>]








### OPT_RIGHT

```lua
CMD.OPT_RIGHT = 16
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L36-L36" target="_blank">source</a>]








### OPT_INTERNAL

```lua
CMD.OPT_INTERNAL = 8
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L38-L38" target="_blank">source</a>]








### OPT_META

```lua
CMD.OPT_META = 4
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L40-L40" target="_blank">source</a>]








### MOVESTATE_NONE

```lua
CMD.MOVESTATE_NONE = -1
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L43-L43" target="_blank">source</a>]








### MOVESTATE_HOLDPOS

```lua
CMD.MOVESTATE_HOLDPOS = 0
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L45-L45" target="_blank">source</a>]








### MOVESTATE_MANEUVER

```lua
CMD.MOVESTATE_MANEUVER = 1
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L47-L47" target="_blank">source</a>]








### MOVESTATE_ROAM

```lua
CMD.MOVESTATE_ROAM = 2
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L49-L49" target="_blank">source</a>]








### FIRESTATE_NONE

```lua
CMD.FIRESTATE_NONE = -1
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L52-L52" target="_blank">source</a>]








### FIRESTATE_HOLDFIRE

```lua
CMD.FIRESTATE_HOLDFIRE = 0
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L54-L54" target="_blank">source</a>]








### FIRESTATE_RETURNFIRE

```lua
CMD.FIRESTATE_RETURNFIRE = 1
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L56-L56" target="_blank">source</a>]








### FIRESTATE_FIREATWILL

```lua
CMD.FIRESTATE_FIREATWILL = 2
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L58-L58" target="_blank">source</a>]








### FIRESTATE_FIREATNEUTRAL

```lua
CMD.FIRESTATE_FIREATNEUTRAL = 3
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L60-L60" target="_blank">source</a>]








### WAITCODE_TIME

```lua
CMD.WAITCODE_TIME = 1
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L63-L63" target="_blank">source</a>]








### WAITCODE_DEATH

```lua
CMD.WAITCODE_DEATH = 2
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L65-L65" target="_blank">source</a>]








### WAITCODE_SQUAD

```lua
CMD.WAITCODE_SQUAD = 3
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L67-L67" target="_blank">source</a>]








### WAITCODE_GATHER

```lua
CMD.WAITCODE_GATHER = 4
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L69-L69" target="_blank">source</a>]








### STOP

```lua
CMD.STOP = 0
```

Stop the current action and clear the unit's command queue.

For factories, this will cancel the new unit orders queue.
For units, this will cancel the current command and queue.

Accepts no parameters.

It won't do anything if used with `CMD.INSERT`, or the `shift` option.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L74-L85" target="_blank">source</a>]








### INSERT

```lua
CMD.INSERT = 1
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L87-L87" target="_blank">source</a>]








### REMOVE

```lua
CMD.REMOVE = 2
```

Remove all commands from a unit's queue matching specific cmdIDs or tags.

## Modes of operation

### Filter by tag

Removes any command with a tag matching those included in params.

- `params` {tag1, tag2 ...} an array of tags to look for.

This is the default mode of operation.

### Filter by id

Removes any command with a `command id` matching those included in params.

- `params` {id1, id2 ...} or {tag1, tag2, ...} an array of ids tags to look for.

To use this mode you need to pass the `alt` option.

## Command Options

- `alt` Tag/Id switch
- `ctrl` Alternative queue selection.
- For factories alternative queue is the factory command queue, default queue is the rally queue.
- For other units no effect.

## Examples

Delete all attack orders from unit, or factory rally queue if factory:
```lua
Spring.GiveOrderToUnit(unitID, CMD.REMOVE, CMD.ATTACK)
```

Delete all attack and fight orders from unit, or factory rally queue if factory:
```lua
Spring.GiveOrderToUnit(unitID, CMD.REMOVE, {CMD.ATTACK, CMD.FIGHT}, CMD.OPT_ALT)
```

Delete commands with specific tags:
```lua
Spring.GiveOrderToUnit(unitID, CMD.REMOVE, {tag1, tag2, tag3})
```

Delete all commands to build units with UnitDef ids unitDefId1 and unitDefId2 from factory queue:
```lua
Spring.GiveOrderToUnit(unitID, CMD.REMOVE, {-unitDefId1, -unitDefId2}, CMD.OPT_ALT + CMD.OPT_CTRL)
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L90-L143" target="_blank">source</a>]








### WAIT

```lua
CMD.WAIT = 5
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L145-L145" target="_blank">source</a>]








### TIMEWAIT

```lua
CMD.TIMEWAIT = 6
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L147-L147" target="_blank">source</a>]








### DEATHWAIT

```lua
CMD.DEATHWAIT = 7
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L149-L149" target="_blank">source</a>]








### SQUADWAIT

```lua
CMD.SQUADWAIT = 8
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L151-L151" target="_blank">source</a>]








### GATHERWAIT

```lua
CMD.GATHERWAIT = 9
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L153-L153" target="_blank">source</a>]








### MOVE

```lua
CMD.MOVE = 10
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L155-L155" target="_blank">source</a>]








### PATROL

```lua
CMD.PATROL = 15
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L157-L157" target="_blank">source</a>]








### FIGHT

```lua
CMD.FIGHT = 16
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L159-L159" target="_blank">source</a>]








### ATTACK

```lua
CMD.ATTACK = 20
```

Attack command. Gives an order to attack some target(s).

The command has different modes of operation, depending on the number
of parameters and options used.

## Modes of operation

### Attack single target

- `params` {unitID}: Attack a unit

The command will end once the target is dead or not valid any more.

### Area attack

Will create a number of `single target` actions by finding targets in a circle.

**Note:** this is different than CMD.AREA_ATTACK, since this initially finds the targets
but then doesn't consider the area any more.

- `params` {x,y,z,r} when radius is greater than 0.
- r: radius
- x,y,z: map position

### Ground attack

- `params` {x,y,z,0} or {x,y,z}
- x,y,z: map position

## Command Options

- `alt` Also target stunned targets. Without this stunned targets will be skipped.
- `meta` Override `manualFire`, and `noAutoTarget` weapon behaviours.

## Other modifiers

- `modInfo.targetableTransportedUnits`: Controls whether transported units are targetable.

## Callins

- UnitCmdDone: Run when the command is finished.

## Examples

Attack unit with id `targetID`.
```lua
Spring.GiveOrderToUnit(unitID, CMD.ATTACK, targetID)
```

Area attack of radius 50 at map position 1000,1000 with height 100:
```lua
Spring.GiveOrderToUnit(unitID, CMD.ATTACK, {1000,100,1000,50})
```

Ground attack at map position 1000,1000 with height 100:
```lua
Spring.GiveOrderToUnit(unitID, CMD.ATTACK, {1000,100,1000})
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L162-L225" target="_blank">source</a>]








### AREA_ATTACK

```lua
CMD.AREA_ATTACK = 21
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L227-L227" target="_blank">source</a>]








### GUARD

```lua
CMD.GUARD = 25
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L229-L229" target="_blank">source</a>]








### GROUPSELECT

```lua
CMD.GROUPSELECT = 35
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L231-L231" target="_blank">source</a>]








### GROUPADD

```lua
CMD.GROUPADD = 36
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L233-L233" target="_blank">source</a>]








### GROUPCLEAR

```lua
CMD.GROUPCLEAR = 37
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L235-L235" target="_blank">source</a>]








### REPAIR

```lua
CMD.REPAIR = 40
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L237-L237" target="_blank">source</a>]








### FIRE_STATE

```lua
CMD.FIRE_STATE = 45
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L239-L239" target="_blank">source</a>]








### MOVE_STATE

```lua
CMD.MOVE_STATE = 50
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L241-L241" target="_blank">source</a>]








### SETBASE

```lua
CMD.SETBASE = 55
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L243-L243" target="_blank">source</a>]








### INTERNAL

```lua
CMD.INTERNAL = 60
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L245-L245" target="_blank">source</a>]








### SELFD

```lua
CMD.SELFD = 65
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L247-L247" target="_blank">source</a>]








### LOAD_UNITS

```lua
CMD.LOAD_UNITS = 75
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L249-L249" target="_blank">source</a>]








### LOAD_ONTO

```lua
CMD.LOAD_ONTO = 76
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L251-L251" target="_blank">source</a>]








### UNLOAD_UNITS

```lua
CMD.UNLOAD_UNITS = 80
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L253-L253" target="_blank">source</a>]








### UNLOAD_UNIT

```lua
CMD.UNLOAD_UNIT = 81
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L255-L255" target="_blank">source</a>]








### ONOFF

```lua
CMD.ONOFF = 85
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L257-L257" target="_blank">source</a>]








### RECLAIM

```lua
CMD.RECLAIM = 90
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L259-L259" target="_blank">source</a>]








### CLOAK

```lua
CMD.CLOAK = 95
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L261-L261" target="_blank">source</a>]








### STOCKPILE

```lua
CMD.STOCKPILE = 100
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L263-L263" target="_blank">source</a>]








### MANUALFIRE

```lua
CMD.MANUALFIRE = 105
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L265-L265" target="_blank">source</a>]








### DGUN

```lua
CMD.DGUN = 105
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L267-L267" target="_blank">source</a>]








### RESTORE

```lua
CMD.RESTORE = 110
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L269-L269" target="_blank">source</a>]








### REPEAT

```lua
CMD.REPEAT = 115
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L271-L271" target="_blank">source</a>]








### TRAJECTORY

```lua
CMD.TRAJECTORY = 120
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L273-L273" target="_blank">source</a>]








### RESURRECT

```lua
CMD.RESURRECT = 125
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L275-L275" target="_blank">source</a>]








### CAPTURE

```lua
CMD.CAPTURE = 130
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L277-L277" target="_blank">source</a>]








### AUTOREPAIRLEVEL

```lua
CMD.AUTOREPAIRLEVEL = 135
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L279-L279" target="_blank">source</a>]








### LOOPBACKATTACK

```lua
CMD.LOOPBACKATTACK = 20
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L281-L281" target="_blank">source</a>]








### IDLEMODE

```lua
CMD.IDLEMODE = 145
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/47cc5a9657a0c3241c95f06dd17047ce4c119d89/rts/Lua/LuaConstCMD.cpp#L283-L283" target="_blank">source</a>]











{% endraw %}