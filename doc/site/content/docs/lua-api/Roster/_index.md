---
title: "Roster"
permalink: "docs/lua-api/Roster"
---
# class Roster





Roster

Contains data about a player







---



## fields
---

### Roster.name
---
```lua
Roster.name : string
```










### Roster.playerID
---
```lua
Roster.playerID : integer
```










### Roster.teamID
---
```lua
Roster.teamID : integer
```










### Roster.allyTeamID
---
```lua
Roster.allyTeamID : integer
```










### Roster.spectator
---
```lua
Roster.spectator : boolean
```










### Roster.cpuUsage
---
```lua
Roster.cpuUsage : number
```



in order to find the progress, use: cpuUsage&0x1 if it's PC or BO, cpuUsage& 0xFE to get path res, (cpuUsage>>8)*1000 for the progress








### Roster.pingTime
---
```lua
Roster.pingTime : number
```



if -1, the player is pathfinding








### Roster.name
---
```lua
Roster.name : string
```










### Roster.playerID
---
```lua
Roster.playerID : integer
```










### Roster.teamID
---
```lua
Roster.teamID : integer
```










### Roster.allyTeamID
---
```lua
Roster.allyTeamID : integer
```










### Roster.spectator
---
```lua
Roster.spectator : boolean
```










### Roster.cpuUsage
---
```lua
Roster.cpuUsage : number
```



in order to find the progress, use: cpuUsage&0x1 if it's PC or BO, cpuUsage& 0xFE to get path res, (cpuUsage>>8)*1000 for the progress








### Roster.pingTime
---
```lua
Roster.pingTime : number
```



if -1, the player is pathfinding









