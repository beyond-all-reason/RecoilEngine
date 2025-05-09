---
title: "Script"
permalink: "docs/lua-api/Script"
---
# global Script








---

## methods
---

### Script.Kill
---
```lua
function Script.Kill(killMessage: string?)
```
@param `killMessage` - Kill message.













### Script.UpdateCallin
---
```lua
function Script.UpdateCallin(name: string)
```












### Script.GetName
---
```lua
function Script.GetName() -> name string
```












### Script.GetSynced
---
```lua
function Script.GetSynced() -> synced boolean
```












### Script.GetFullCtrl
---
```lua
function Script.GetFullCtrl() -> fullCtrl boolean
```












### Script.GetFullRead
---
```lua
function Script.GetFullRead() -> fullRead boolean
```












### Script.GetCtrlTeam
---
```lua
function Script.GetCtrlTeam() -> teamID integer
```












### Script.GetReadTeam
---
```lua
function Script.GetReadTeam() -> teamID integer
```












### Script.GetReadAllyTeam
---
```lua
function Script.GetReadAllyTeam() -> allyTeamID integer
```












### Script.GetSelectTeam
---
```lua
function Script.GetSelectTeam() -> teamID integer
```












### Script.GetGlobal
---
```lua
function Script.GetGlobal() -> global integer?
```












### Script.GetRegistry
---
```lua
function Script.GetRegistry() -> registry integer?
```












### Script.DelayByFrames
---
```lua
function Script.DelayByFrames(
  frameDelay: integer,
  fun: unknown
)
```












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












### Script.Kill
---
```lua
function Script.Kill(killMessage: string?)
```
@param `killMessage` - Kill message.













### Script.UpdateCallin
---
```lua
function Script.UpdateCallin(name: string)
```












### Script.GetName
---
```lua
function Script.GetName() -> name string
```












### Script.GetSynced
---
```lua
function Script.GetSynced() -> synced boolean
```












### Script.GetFullCtrl
---
```lua
function Script.GetFullCtrl() -> fullCtrl boolean
```












### Script.GetFullRead
---
```lua
function Script.GetFullRead() -> fullRead boolean
```












### Script.GetCtrlTeam
---
```lua
function Script.GetCtrlTeam() -> teamID integer
```












### Script.GetReadTeam
---
```lua
function Script.GetReadTeam() -> teamID integer
```












### Script.GetReadAllyTeam
---
```lua
function Script.GetReadAllyTeam() -> allyTeamID integer
```












### Script.GetSelectTeam
---
```lua
function Script.GetSelectTeam() -> teamID integer
```












### Script.GetGlobal
---
```lua
function Script.GetGlobal() -> global integer?
```












### Script.GetRegistry
---
```lua
function Script.GetRegistry() -> registry integer?
```












### Script.DelayByFrames
---
```lua
function Script.DelayByFrames(
  frameDelay: integer,
  fun: unknown
)
```












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















## fields
---

### Script.NO_ACCESS_TEAM
---
```lua
Script.NO_ACCESS_TEAM: integer = -1
```










### Script.ALL_ACCESS_TEAM
---
```lua
Script.ALL_ACCESS_TEAM: integer = -2
```










### Script.NO_ACCESS_TEAM
---
```lua
Script.NO_ACCESS_TEAM: integer = -1
```










### Script.ALL_ACCESS_TEAM
---
```lua
Script.ALL_ACCESS_TEAM: integer = -2
```











