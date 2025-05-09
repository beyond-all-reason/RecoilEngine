---
title: "Spring"
permalink: "docs/lua-api/Spring"
---
# global Spring








---

## methods
---

### Spring.IsCheatingEnabled
---
```lua
function Spring.IsCheatingEnabled() -> enabled boolean
```












### Spring.IsGodModeEnabled
---
```lua
function Spring.IsGodModeEnabled() -> enabled boolean
```












### Spring.IsDevLuaEnabled
---
```lua
function Spring.IsDevLuaEnabled() -> enabled boolean
```












### Spring.IsEditDefsEnabled
---
```lua
function Spring.IsEditDefsEnabled() -> enabled boolean
```












### Spring.IsNoCostEnabled
---
```lua
function Spring.IsNoCostEnabled() -> enabled boolean
```












### Spring.GetGlobalLos
---
```lua
function Spring.GetGlobalLos(teamID: integer?) -> enabled boolean
```












### Spring.AreHelperAIsEnabled
---
```lua
function Spring.AreHelperAIsEnabled() -> enabled boolean
```












### Spring.FixedAllies
---
```lua
function Spring.FixedAllies() -> enabled boolean?
```












### Spring.IsGameOver
---
```lua
function Spring.IsGameOver() -> isGameOver boolean
```












### Spring.GetGameFrame
---
```lua
function Spring.GetGameFrame()
 -> t1 number
 -> t2 number

```

@return `t1` - frameNum % dayFrames

@return `t2` - frameNum / dayFrames












### Spring.GetGameSeconds
---
```lua
function Spring.GetGameSeconds() -> seconds number
```












### Spring.GetTidal
---
```lua
function Spring.GetTidal() -> tidalStrength number
```












### Spring.GetWind
---
```lua
function Spring.GetWind() -> windStrength number
```












### Spring.GetGameRulesParams
---
```lua
function Spring.GetGameRulesParams() -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values












### Spring.GetTeamRulesParams
---
```lua
function Spring.GetTeamRulesParams(teamID: integer) -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values












### Spring.GetPlayerRulesParams
---
```lua
function Spring.GetPlayerRulesParams(playerID: integer) -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values












### Spring.GetUnitRulesParams
---
```lua
function Spring.GetUnitRulesParams(unitID: integer) -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values












### Spring.GetFeatureRulesParams
---
```lua
function Spring.GetFeatureRulesParams(featureID: integer) -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values












### Spring.GetGameRulesParam
---
```lua
function Spring.GetGameRulesParam(ruleRef: (number|string)) ->  number?
```
@param `ruleRef` - the rule index or name


@return  - |string value












### Spring.GetTeamRulesParam
---
```lua
function Spring.GetTeamRulesParam(
  teamID: integer,
  ruleRef: (number|string)
) -> value (number|string)?
```
@param `ruleRef` - the rule index or name













### Spring.GetPlayerRulesParam
---
```lua
function Spring.GetPlayerRulesParam(
  playerID: integer,
  ruleRef: (number|string)
) -> value (number|string)?
```
@param `ruleRef` - the rule index or name













### Spring.GetUnitRulesParam
---
```lua
function Spring.GetUnitRulesParam(
  unitID: integer,
  ruleRef: (number|string)
) -> value (number|string)?
```
@param `ruleRef` - the rule index or name













### Spring.GetFeatureRulesParam
---
```lua
function Spring.GetFeatureRulesParam(
  featureID: integer,
  ruleRef: (number|string)
) -> value (number|string)?
```
@param `ruleRef` - the rule index or name













### Spring.GetMapOption
---
```lua
function Spring.GetMapOption(mapOption: string) -> value string
```

@return `value` - Value of `modOption`.












### Spring.GetMapOptions
---
```lua
function Spring.GetMapOptions() -> mapOptions table<string,string>
```

@return `mapOptions` - Table with options names as keys and values as values.












### Spring.GetModOption
---
```lua
function Spring.GetModOption(modOption: string) -> value string
```

@return `value` - Value of `modOption`.












### Spring.GetModOptions
---
```lua
function Spring.GetModOptions() -> modOptions table<string,string>
```

@return `modOptions` - Table with options names as keys and values as values.












### Spring.GetHeadingFromVector
---
```lua
function Spring.GetHeadingFromVector(
  x: number,
  z: number
) -> heading number
```












### Spring.GetVectorFromHeading
---
```lua
function Spring.GetVectorFromHeading(heading: number)
 -> x number
 -> z number

```












### Spring.GetFacingFromHeading
---
```lua
function Spring.GetFacingFromHeading(heading: number) -> facing FacingInteger
```












### Spring.GetHeadingFromFacing
---
```lua
function Spring.GetHeadingFromFacing(facing: FacingInteger) -> heading number
```












### Spring.GetSideData
---
```lua
function Spring.GetSideData(sideName: string)
 -> startUnit string?
 -> caseSensitiveSideName string

```












### Spring.GetSideData
---
```lua
function Spring.GetSideData(sideID: integer)
 -> sideName string?
 -> startUnit string
 -> caseSensitiveSideName string

```












### Spring.GetSideData
---
```lua
function Spring.GetSideData() -> sideArray SideSpec[]
```












### Spring.GetGaiaTeamID
---
```lua
function Spring.GetGaiaTeamID() -> teamID integer
```












### Spring.GetAllyTeamStartBox
---
```lua
function Spring.GetAllyTeamStartBox(allyID: integer)
 -> xMin number?
 -> zMin number?
 -> xMax number?
 -> zMax number?

```












### Spring.GetTeamStartPosition
---
```lua
function Spring.GetTeamStartPosition(teamID: integer)
 -> x number?
 -> y number?
 -> x number?

```












### Spring.GetMapStartPositions
---
```lua
function Spring.GetMapStartPositions() -> array float3[]
```

@return `array` - of positions indexed by teamID












### Spring.GetAllyTeamList
---
```lua
function Spring.GetAllyTeamList() -> allyTeamIDs integer[]
```












### Spring.GetTeamList
---
```lua
function Spring.GetTeamList(allyTeamID: unknown) -> teamIDs number[]
```
@param `allyTeamID` - (Default: `-1`)


@return `teamIDs` - List of team IDs.





Get all team IDs.








### Spring.GetTeamList
---
```lua
function Spring.GetTeamList(allyTeamID: integer) -> teamIDs number[]?
```
@param `allyTeamID` - The ally team ID to filter teams by. A value less than 0 will return all teams.


@return `teamIDs` - List of team IDs or `nil` if `allyTeamID` is invalid.





Get team IDs in a specific ally team.








### Spring.GetPlayerList
---
```lua
function Spring.GetPlayerList(
  teamID: integer?,
  active: boolean?
) -> list number[]?
```
@param `teamID` - (Default: `-1`) to filter by when >= 0

@param `active` - (Default: `false`) whether to filter only active teams


@return `list` - of playerIDs












### Spring.GetTeamInfo
---
```lua
function Spring.GetTeamInfo(
  teamID: integer,
  getTeamKeys: boolean?
)
 -> teamID integer?
 -> leader number
 -> isDead number
 -> hasAI number
 -> side string
 -> allyTeam number
 -> incomeMultiplier number
 -> customTeamKeys table<string,string>

```
@param `getTeamKeys` - (Default: `true`) whether to return the customTeamKeys table


@return `customTeamKeys` - when getTeamKeys is true, otherwise nil












### Spring.GetTeamAllyTeamID
---
```lua
function Spring.GetTeamAllyTeamID(teamID: integer) -> allyTeamID integer?
```












### Spring.GetTeamResources
---
```lua
function Spring.GetTeamResources(
  teamID: integer,
  resource: ResourceName
)
 -> currentLevel number?
 -> storage number
 -> pull number
 -> income number
 -> expense number
 -> share number
 -> sent number
 -> received number
 -> excess number

```












### Spring.GetTeamUnitStats
---
```lua
function Spring.GetTeamUnitStats(teamID: integer)
 -> killed number?
 -> died number
 -> capturedBy number
 -> capturedFrom number
 -> received number
 -> sent number

```












### Spring.GetTeamResourceStats
---
```lua
function Spring.GetTeamResourceStats(
  teamID: integer,
  resource: ResourceName
)
 -> used number?
 -> produced number
 -> excessed number
 -> received number
 -> sent number

```












### Spring.GetTeamDamageStats
---
```lua
function Spring.GetTeamDamageStats(teamID: integer)
 -> damageDealt number
 -> damageReceived number

```





Gets team damage dealt/received totals

Returns a team's damage stats. Note that all damage is counted,
including self-inflicted and unconfirmed out-of-sight.








### Spring.GetTeamStatsHistory
---
```lua
function Spring.GetTeamStatsHistory(teamID: integer) -> historyCount integer?
```

@return `historyCount` - The number of history entries, or `nil` if unable to resolve team.





Get the number of history entries.








### Spring.GetTeamStatsHistory
---
```lua
function Spring.GetTeamStatsHistory(
  teamID: integer,
  startIndex: integer,
  endIndex: integer?
) -> The TeamStats[]
```
@param `endIndex` - (Default: startIndex)


@return `The` - team stats history, or `nil` if unable to resolve team.





Get team stats history.








### Spring.GetTeamLuaAI
---
```lua
function Spring.GetTeamLuaAI(teamID: integer) ->  string
```












### Spring.GetTeamMaxUnits
---
```lua
function Spring.GetTeamMaxUnits(teamID: integer)
 -> maxUnits number
 -> currentUnits number?

```





Returns a team's unit cap.

Also returns the current unit count for readable teams as the 2nd value.








### Spring.GetPlayerInfo
---
```lua
function Spring.GetPlayerInfo(
  playerID: integer,
  getPlayerOpts: boolean?
)
 -> name string
 -> active boolean
 -> spectator boolean
 -> teamID integer
 -> allyTeamID integer
 -> pingTime number
 -> cpuUsage number
 -> country string
 -> rank number
 -> hasSkirmishAIsInTeam boolean
 -> playerOpts { , [string]: string }
 -> desynced boolean

```
@param `getPlayerOpts` - (Default: `true`) whether to return custom player options


@return `playerOpts` - when playerOpts is true












### Spring.GetPlayerControlledUnit
---
```lua
function Spring.GetPlayerControlledUnit(playerID: integer) ->  number?
```





Returns unit controlled by player on FPS mode








### Spring.GetAIInfo
---
```lua
function Spring.GetAIInfo(teamID: integer)
 -> skirmishAIID integer
 -> name string
 -> hostingPlayerID integer
 -> shortName string
 -> version string
 -> options table<string,string>

```

@return `shortName` - When synced `"SYNCED_NOSHORTNAME"`, otherwise the AI shortname or `"UNKNOWN"`.

@return `version` - When synced `"SYNCED_NOVERSION"`, otherwise the AI version or `"UNKNOWN"`.












### Spring.GetAllyTeamInfo
---
```lua
function Spring.GetAllyTeamInfo(allyTeamID: integer) ->  table<string,string>?
```












### Spring.AreTeamsAllied
---
```lua
function Spring.AreTeamsAllied(
  teamID1: number,
  teamID2: number
) ->  boolean?
```












### Spring.ArePlayersAllied
---
```lua
function Spring.ArePlayersAllied(
  playerID1: number,
  playerID2: number
) ->  boolean?
```












### Spring.GetAllUnits
---
```lua
function Spring.GetAllUnits() -> unitIDs number[]
```





Get a list of all unitIDs

Note that when called from a widget, this also returns units that are only
radar blips.

For units that are radar blips, you may want to check if they are in los,
as GetUnitDefID() will still return true if they have previously been seen.





@see UnsyncedRead.GetVisibleUnits




### Spring.GetTeamUnits
---
```lua
function Spring.GetTeamUnits(teamID: integer) -> unitIDs number[]?
```












### Spring.GetTeamUnitsSorted
---
```lua
function Spring.GetTeamUnitsSorted(teamID: integer) -> unitsByDef table<integer,integer>
```

@return `unitsByDef` - A table where keys are unitDefIDs and values are unitIDs












### Spring.GetTeamUnitsCounts
---
```lua
function Spring.GetTeamUnitsCounts(teamID: integer) -> countByUnit table<number,number>?
```

@return `countByUnit` - A table where keys are unitDefIDs and values are counts.












### Spring.GetTeamUnitsByDefs
---
```lua
function Spring.GetTeamUnitsByDefs(
  teamID: integer,
  unitDefIDs: (number|number[])
) -> unitIDs number[]?
```












### Spring.GetTeamUnitDefCount
---
```lua
function Spring.GetTeamUnitDefCount(
  teamID: integer,
  unitDefID: integer
) -> count number?
```












### Spring.GetTeamUnitCount
---
```lua
function Spring.GetTeamUnitCount(teamID: integer) -> count number?
```












### Spring.GetUnitsInRectangle
---
```lua
function Spring.GetUnitsInRectangle(
  xmin: number,
  zmin: number,
  xmax: number,
  zmax: number,
  allegiance: number?
) -> unitIDs number[]
```












### Spring.GetUnitsInBox
---
```lua
function Spring.GetUnitsInBox(
  xmin: number,
  ymin: number,
  zmin: number,
  xmax: number,
  ymax: number,
  zmax: number,
  allegiance: number?
) -> unitIDs number[]
```












### Spring.GetUnitsInCylinder
---
```lua
function Spring.GetUnitsInCylinder(
  x: number,
  z: number,
  radius: number
) -> unitIDs number[]
```












### Spring.GetUnitsInSphere
---
```lua
function Spring.GetUnitsInSphere(
  x: number,
  y: number,
  z: number,
  radius: number
) -> unitIDs number[]
```












### Spring.GetUnitsInPlanes
---
```lua
function Spring.GetUnitsInPlanes(
  planes: Plane[],
  allegiance: integer?
) -> unitIDs integer[]
```





Plane normals point towards accepted space, so the acceptance criteria for each plane is:

```
radius     = unit radius
px, py, pz = unit position
[(nx * px) + (ny * py) + (nz * pz) + (d - radius)]  <=  0
```








### Spring.GetUnitArrayCentroid
---
```lua
function Spring.GetUnitArrayCentroid(units: table)
 -> centerX number
 -> centerY number
 -> centerZ number

```
@param `units` - { unitID, unitID, ... }






Returns the centroid of an array of units

Returns nil for an empty array








### Spring.GetUnitMapCentroid
---
```lua
function Spring.GetUnitMapCentroid(units: table)
 -> centerX number
 -> centerY number
 -> centerZ number

```
@param `units` - { [unitID] = true, [unitID] = true, ... }






Returns the centroid of a map of units

Returns nil for an empty map








### Spring.GetUnitNearestAlly
---
```lua
function Spring.GetUnitNearestAlly(
  unitID: integer,
  range: number?
) -> unitID integer?
```
@param `range` - (Default: `1.0e9`)













### Spring.GetUnitNearestEnemy
---
```lua
function Spring.GetUnitNearestEnemy(
  unitID: integer,
  range: number?,
  useLOS: boolean?
) -> unitID integer?
```
@param `range` - (Default: `1.0e9`)

@param `useLOS` - (Default: `true`)













### Spring.GetFeaturesInRectangle
---
```lua
function Spring.GetFeaturesInRectangle(
  xmin: number,
  zmin: number,
  xmax: number,
  zmax: number
) -> featureIDs number[]
```












### Spring.GetFeaturesInSphere
---
```lua
function Spring.GetFeaturesInSphere(
  x: number,
  y: number,
  z: number,
  radius: number
) -> featureIDs number[]
```












### Spring.GetFeaturesInCylinder
---
```lua
function Spring.GetFeaturesInCylinder(
  x: number,
  z: number,
  radius: number,
  allegiance: number?
) -> featureIDs number[]
```












### Spring.GetProjectilesInRectangle
---
```lua
function Spring.GetProjectilesInRectangle(
  xmin: number,
  zmin: number,
  xmax: number,
  zmax: number,
  excludeWeaponProjectiles: boolean?,
  excludePieceProjectiles: boolean?
) -> projectileIDs number[]
```
@param `excludeWeaponProjectiles` - (Default: `false`)

@param `excludePieceProjectiles` - (Default: `false`)













### Spring.GetProjectilesInSphere
---
```lua
function Spring.GetProjectilesInSphere(
  x: number,
  y: number,
  z: number,
  radius: number,
  excludeWeaponProjectiles: boolean?,
  excludePieceProjectiles: boolean?
) -> projectileIDs number[]
```
@param `excludeWeaponProjectiles` - (Default: false)

@param `excludePieceProjectiles` - (Default: false)













### Spring.ValidUnitID
---
```lua
function Spring.ValidUnitID(unitID: integer) ->  boolean
```












### Spring.GetUnitStates
---
```lua
function Spring.GetUnitStates(unitID: integer) ->  UnitState
```












### Spring.GetUnitArmored
---
```lua
function Spring.GetUnitArmored(unitID: integer)
 -> armored boolean?
 -> armorMultiple number

```












### Spring.GetUnitIsActive
---
```lua
function Spring.GetUnitIsActive(unitID: integer) -> isActive boolean?
```












### Spring.GetUnitIsCloaked
---
```lua
function Spring.GetUnitIsCloaked(unitID: integer) -> isCloaked boolean?
```












### Spring.GetUnitSeismicSignature
---
```lua
function Spring.GetUnitSeismicSignature(unitID: integer) -> seismicSignature number?
```












### Spring.GetUnitSelfDTime
---
```lua
function Spring.GetUnitSelfDTime(unitID: integer) -> selfDTime integer?
```












### Spring.GetUnitStockpile
---
```lua
function Spring.GetUnitStockpile(unitID: integer)
 -> numStockpiled integer?
 -> numStockpileQued integer?
 -> buildPercent number?

```












### Spring.GetUnitSensorRadius
---
```lua
function Spring.GetUnitSensorRadius(
  unitID: integer,
  type: string
) -> radius number?
```
@param `type` - one of los, airLos, radar, sonar, seismic, radarJammer, sonarJammer













### Spring.GetUnitPosErrorParams
---
```lua
function Spring.GetUnitPosErrorParams(
  unitID: integer,
  allyTeamID: integer?
)
 -> posErrorVectorX number?
 -> posErrorVectorY number
 -> posErrorVectorZ number
 -> posErrorDeltaX number
 -> posErrorDeltaY number
 -> posErrorDeltaZ number
 -> nextPosErrorUpdatebaseErrorMult number
 -> posErrorBit boolean

```












### Spring.GetUnitTooltip
---
```lua
function Spring.GetUnitTooltip(unitID: integer) ->  string?
```












### Spring.GetUnitDefID
---
```lua
function Spring.GetUnitDefID(unitID: integer) ->  number?
```












### Spring.GetUnitTeam
---
```lua
function Spring.GetUnitTeam(unitID: integer) ->  number?
```












### Spring.GetUnitAllyTeam
---
```lua
function Spring.GetUnitAllyTeam(unitID: integer) ->  number?
```












### Spring.GetUnitNeutral
---
```lua
function Spring.GetUnitNeutral(unitID: integer) ->  boolean?
```





Checks if a unit is neutral (NOT Gaia!)

Note that a "neutral" unit can belong to any ally-team (ally, enemy, Gaia).
To check if a unit is Gaia, check its owner team.








### Spring.GetUnitHealth
---
```lua
function Spring.GetUnitHealth(unitID: integer)
 -> health number?
 -> maxHealth number
 -> paralyzeDamage number
 -> captureProgress number
 -> buildProgress number

```

@return `buildProgress` - between 0.0-1.0












### Spring.GetUnitIsDead
---
```lua
function Spring.GetUnitIsDead(unitID: integer) ->  boolean?
```












### Spring.GetUnitIsStunned
---
```lua
function Spring.GetUnitIsStunned(unitID: integer)
 -> stunnedOrBuilt boolean?
 -> stunned boolean
 -> beingBuilt boolean

```

@return `stunnedOrBuilt` - unit is disabled

@return `stunned` - unit is either stunned via EMP or being transported by a non-fireplatform

@return `beingBuilt` - unit is under construction





Checks whether a unit is disabled and can't act

The first return value is a simple OR of the following ones,
any of those conditions is sufficient to disable the unit.

Note that EMP and being transported are mechanically the same and thus lumped together.
Use other callouts to differentiate them if you need to.








### Spring.GetUnitIsBeingBuilt
---
```lua
function Spring.GetUnitIsBeingBuilt(unitID: integer)
 -> beingBuilt boolean
 -> buildProgress number

```












### Spring.GetUnitResources
---
```lua
function Spring.GetUnitResources(unitID: integer)
 -> metalMake number?
 -> metalUse number
 -> energyMake number
 -> energyUse number

```












### Spring.GetUnitStorage
---
```lua
function Spring.GetUnitStorage(unitID: integer)
 -> Unit number
 -> Unit number

```

@return `Unit` - 's metal storage

@return `Unit` - 's energy storage












### Spring.GetUnitCosts
---
```lua
function Spring.GetUnitCosts(unitID: integer)
 -> buildTime number?
 -> metalCost number
 -> energyCost number

```












### Spring.GetUnitCostTable
---
```lua
function Spring.GetUnitCostTable(unitID: integer)
 -> cost ResourceCost?
 -> buildTime number?

```

@return `cost` - The cost of the unit, or `nil` if invalid.

@return `buildTime` - The build time the unit, or `nil` if invalid.












### Spring.GetUnitMetalExtraction
---
```lua
function Spring.GetUnitMetalExtraction(unitID: integer) -> metalExtraction number?
```












### Spring.GetUnitExperience
---
```lua
function Spring.GetUnitExperience(unitID: integer)
 -> xp number
 -> limXp number

```

@return `xp` - [0.0; +∞)

@return `limXp` - [0.0; 1.0) as experience approaches infinity












### Spring.GetUnitHeight
---
```lua
function Spring.GetUnitHeight(unitID: integer) ->  number?
```












### Spring.GetUnitRadius
---
```lua
function Spring.GetUnitRadius(unitID: integer) ->  number?
```












### Spring.GetUnitBuildeeRadius
---
```lua
function Spring.GetUnitBuildeeRadius(unitID: integer) ->  number?
```





Gets the unit's radius for when targeted by build, repair, reclaim-type commands.








### Spring.GetUnitMass
---
```lua
function Spring.GetUnitMass(unitID: integer) ->  number?
```












### Spring.GetUnitPosition
---
```lua
function Spring.GetUnitPosition(
  unitID: integer,
  midPos: boolean?,
  aimPos: boolean?
)
 -> basePointX number?
 -> basePointY number
 -> basePointZ number
 -> midPointX number?
 -> midPointY number
 -> midPointZ number
 -> aimPointX number?
 -> aimPointY number
 -> aimPointZ number

```
@param `midPos` - (Default: `false`) return midpoint as well

@param `aimPos` - (Default: `false`) return aimpoint as well













### Spring.GetUnitBasePosition
---
```lua
function Spring.GetUnitBasePosition(unitID: integer)
 -> posX number?
 -> posY number
 -> posZ number

```












### Spring.GetUnitVectors
---
```lua
function Spring.GetUnitVectors(unitID: integer)
 -> front float3?
 -> up float3
 -> right float3

```












### Spring.GetUnitRotation
---
```lua
function Spring.GetUnitRotation(unitID: integer)
 -> pitch number
 -> yaw number
 -> roll number

```

@return `pitch` - Rotation in X axis

@return `yaw` - Rotation in Y axis

@return `roll` - Rotation in Z axis





Note: PYR order








### Spring.GetUnitDirection
---
```lua
function Spring.GetUnitDirection(unitID: integer)
 -> frontDirX number
 -> frontDirY number
 -> frontDirZ number
 -> rightDirX number
 -> rightDirY number
 -> rightDirZ number
 -> upDirX number
 -> upDirY number
 -> upDirZ number

```












### Spring.GetUnitHeading
---
```lua
function Spring.GetUnitHeading(
  unitID: integer,
  convertToRadians: boolean?
) -> heading number
```
@param `convertToRadians` - (Default: `false`)













### Spring.GetUnitVelocity
---
```lua
function Spring.GetUnitVelocity(unitID: integer)
```












### Spring.GetUnitBuildFacing
---
```lua
function Spring.GetUnitBuildFacing(unitID: integer)
```












### Spring.GetUnitIsBuilding
---
```lua
function Spring.GetUnitIsBuilding(unitID: integer) -> buildeeUnitID integer
```

@return `buildeeUnitID` - or nil





Checks whether a unit is currently building another (NOT for checking if it's a structure)

Works for both mobile builders and factories.








### Spring.GetUnitWorkerTask
---
```lua
function Spring.GetUnitWorkerTask(unitID: integer)
 -> cmdID integer
 -> targetID integer

```

@return `cmdID` - of the relevant command

@return `targetID` - if applicable (all except RESTORE)





Checks a builder's current task

Checks what a builder is currently doing. This is not the same as `Spring.GetUnitCurrentCommand`,
because you can have a command at the front of the queue and not be doing it (for example because
the target is still too far away), and on the other hand you can also be doing a task despite not
having it in front of the queue (for example you're Guarding another builder who does). Also, it
resolves the Repair command into either actual repair, or construction assist (in which case it
returns the appropriate "build" command). Only build-related commands are returned (no Move or any
custom commands).

The possible commands returned are repair, reclaim, resurrect, capture, restore,
and build commands (negative buildee unitDefID).








### Spring.GetUnitEffectiveBuildRange
---
```lua
function Spring.GetUnitEffectiveBuildRange(
  unitID: integer,
  buildeeDefID: integer
) -> effectiveBuildRange number
```
@param `buildeeDefID` - or nil


@return `effectiveBuildRange` - counted to the center of prospective buildee; buildRange if buildee nil





Useful for setting move goals manually.








### Spring.GetUnitCurrentBuildPower
---
```lua
function Spring.GetUnitCurrentBuildPower(unitID: integer)
```












### Spring.GetUnitHarvestStorage
---
```lua
function Spring.GetUnitHarvestStorage(unitID: integer)
 -> storedMetal number
 -> maxStoredMetal number
 -> storedEnergy number
 -> maxStoredEnergy number

```





Get a unit's carried resources

Checks resources being carried internally by the unit.








### Spring.GetUnitBuildParams
---
```lua
function Spring.GetUnitBuildParams(unitID: integer)
```












### Spring.GetUnitInBuildStance
---
```lua
function Spring.GetUnitInBuildStance(unitID: integer) -> inBuildStance boolean
```





Is builder in build stance

Checks if a builder is in build stance, i.e. can create nanoframes.
Returns nil for non-builders.








### Spring.GetUnitNanoPieces
---
```lua
function Spring.GetUnitNanoPieces(unitID: integer) -> pieceArray integer[]
```





Get construction FX attachment points

Returns an array of pieces which represent construction
points. Default engine construction FX (nano spray) will
originate there.

Only works on builders and factories, returns nil (NOT empty table)
for other units.








### Spring.GetUnitTransporter
---
```lua
function Spring.GetUnitTransporter(unitID: integer) -> transportUnitID integer?
```





Get the transport carrying the unit

Returns the unit ID of the transport, if any.
Returns nil if the unit is not being transported.








### Spring.GetUnitIsTransporting
---
```lua
function Spring.GetUnitIsTransporting(unitID: integer) -> transporteeArray integer[]?
```

@return `transporteeArray` - An array of unitIDs being transported by this unit, or `nil` if not a transport.





Get units being transported








### Spring.GetUnitShieldState
---
```lua
function Spring.GetUnitShieldState(
  unitID: integer,
  weaponNum: number?
)
 -> isEnabled number
 -> currentPower number

```
@param `weaponNum` - Optional if the unit has just one shield


@return `isEnabled` - Warning, number not boolean. 0 or 1












### Spring.GetUnitFlanking
---
```lua
function Spring.GetUnitFlanking(unitID: integer)
```












### Spring.GetUnitMaxRange
---
```lua
function Spring.GetUnitMaxRange(unitID: integer) -> maxRange number
```





Get a unit's engagement range

Returns the range at which a unit will stop to engage.
By default this is the highest among the unit's weapon ranges (hence name),
but can be changed dynamically. Also note that unarmed units ignore this.








### Spring.GetUnitWeaponState
---
```lua
function Spring.GetUnitWeaponState(
  unitID: integer,
  weaponNum: number,
  stateName: string
) -> stateValue number
```





Check the state of a unit's weapon

Available states to poll:
"reloadFrame" (frame on which the weapon will be ready to fire),
"reloadSpeed" (reload time in seconds),
"range" (in elmos),
"autoTargetRangeBoost" (predictive aiming range buffer, in elmos),
"projectileSpeed" (in elmos/frame),
"reloadTimeXP" (reload time after XP bonus, in seconds),
"reaimTime" (frames between AimWeapon calls),
"burst" (shots in a burst),
"burstRate" (delay between shots in a burst, in seconds),
"projectiles" (projectiles per shot),
"salvoLeft" (shots remaining in ongoing burst),
"nextSalvo" (simframe of the next shot in an ongoing burst),
"accuracy" (INaccuracy after XP bonus),
"sprayAngle" (spray angle after XP bonus),
"targetMoveError" (extra inaccuracy against moving targets, after XP bonus)
"avoidFlags" (bitmask for targeting avoidance),
"ttl" (number of seconds a projectile should live)
"collisionFlags" (bitmask for collisions).

The state "salvoError" is an exception and returns a table: {x, y, z},
which represents the inaccuracy error of the ongoing burst.








### Spring.GetUnitWeaponDamages
---
```lua
function Spring.GetUnitWeaponDamages(unitID: integer)
```












### Spring.GetUnitWeaponVectors
---
```lua
function Spring.GetUnitWeaponVectors(unitID: integer)
```












### Spring.GetUnitWeaponTryTarget
---
```lua
function Spring.GetUnitWeaponTryTarget(unitID: integer)
```












### Spring.GetUnitWeaponTestTarget
---
```lua
function Spring.GetUnitWeaponTestTarget(unitID: integer)
```












### Spring.GetUnitWeaponTestRange
---
```lua
function Spring.GetUnitWeaponTestRange(unitID: integer)
```












### Spring.GetUnitWeaponHaveFreeLineOfFire
---
```lua
function Spring.GetUnitWeaponHaveFreeLineOfFire(unitID: integer)
```












### Spring.GetUnitWeaponCanFire
---
```lua
function Spring.GetUnitWeaponCanFire(unitID: integer)
```












### Spring.GetUnitWeaponTarget
---
```lua
function Spring.GetUnitWeaponTarget(
  unitID: integer,
  weaponNum: integer
)
 -> TargetType 0
 -> isUserTarget boolean

```

@return `TargetType` - none





Checks a weapon's target

Note that this doesn't need to reflect the unit's Attack orders or such, and
that weapons can aim individually unless slaved.








### Spring.GetUnitWeaponTarget
---
```lua
function Spring.GetUnitWeaponTarget(
  unitID: integer,
  weaponNum: integer
)
 -> TargetType 1
 -> isUserTarget boolean
 -> targetUnitID integer

```

@return `TargetType` - unit





Checks a weapon's target

Note that this doesn't need to reflect the unit's Attack orders or such, and
that weapons can aim individually unless slaved.








### Spring.GetUnitWeaponTarget
---
```lua
function Spring.GetUnitWeaponTarget(
  unitID: integer,
  weaponNum: integer
)
 -> TargetType 2
 -> isUserTarget boolean
 -> targetPosition float3

```

@return `TargetType` - position





Checks a weapon's target

Note that this doesn't need to reflect the unit's Attack orders or such, and
that weapons can aim individually unless slaved.








### Spring.GetUnitWeaponTarget
---
```lua
function Spring.GetUnitWeaponTarget(
  unitID: integer,
  weaponNum: integer
)
 -> TargetType 3
 -> isUserTarget boolean
 -> targetProjectileId integer

```

@return `TargetType` - projectileID





Checks a weapon's target

Note that this doesn't need to reflect the unit's Attack orders or such, and
that weapons can aim individually unless slaved.








### Spring.GetUnitEstimatedPath
---
```lua
function Spring.GetUnitEstimatedPath(unitID: integer)
```












### Spring.GetUnitLastAttacker
---
```lua
function Spring.GetUnitLastAttacker(unitID: integer)
```












### Spring.GetUnitLastAttackedPiece
---
```lua
function Spring.GetUnitLastAttackedPiece(unitID: integer)
```












### Spring.GetUnitCollisionVolumeData
---
```lua
function Spring.GetUnitCollisionVolumeData(unitID: integer)
```












### Spring.GetUnitSeparation
---
```lua
function Spring.GetUnitSeparation(
  unitID1: number,
  unitID2: number,
  direction: boolean?,
  subtractRadii: boolean?
) ->  number?
```
@param `direction` - (Default: `false`) to subtract from, default unitID1 - unitID2

@param `subtractRadii` - (Default: `false`) whether units radii should be subtracted from the total













### Spring.GetUnitFeatureSeparation
---
```lua
function Spring.GetUnitFeatureSeparation(unitID: integer)
```












### Spring.GetUnitDefDimensions
---
```lua
function Spring.GetUnitDefDimensions(unitDefID: integer) -> dimensions UnitDefDimensions?
```












### Spring.GetCEGID
---
```lua
function Spring.GetCEGID()
```












### Spring.GetUnitBlocking
---
```lua
function Spring.GetUnitBlocking(unitID: integer)
 -> isBlocking boolean?
 -> isSolidObjectCollidable boolean
 -> isProjectileCollidable boolean
 -> isRaySegmentCollidable boolean
 -> crushable boolean
 -> blockEnemyPushing boolean
 -> blockHeightChanges boolean

```












### Spring.GetUnitMoveTypeData
---
```lua
function Spring.GetUnitMoveTypeData(unitID: integer)
```












### Spring.GetUnitCurrentCommand
---
```lua
function Spring.GetUnitCurrentCommand(
  unitID: integer,
  cmdIndex: integer
)
 -> cmdID CMD
 -> options (integer|CommandOptionBit)
 -> tag integer
 -> Command number ...

```
@param `unitID` - Unit id.

@param `cmdIndex` - Command index to get. If negative will count from the end of the queue,
for example -1 will be the last command.


@return `Command` - parameters.












### Spring.GetUnitCommands
---
```lua
function Spring.GetUnitCommands(
  unitID: integer,
  count: integer
) -> commands Command[]
```
@param `count` - Number of commands to return, `-1` returns all commands, `0` returns command count.






Get the commands for a unit.

Same as `Spring.GetCommandQueue`








### Spring.GetUnitCommands
---
```lua
function Spring.GetUnitCommands(
  unitID: integer,
  count: 0
) -> The integer
```
@param `count` - Returns the number of commands in the units queue.


@return `The` - number of commands in the unit queue.





Get the count of commands for a unit.

Same as `Spring.GetCommandQueue`








### Spring.GetFactoryCommands
---
```lua
function Spring.GetFactoryCommands(
  unitID: integer,
  count: number
) -> commands (number|Command[])
```
@param `count` - when 0 returns the number of commands in the units queue, when -1 returns all commands, number of commands to return otherwise






Get the number or list of commands for a factory








### Spring.GetUnitCommandCount
---
```lua
function Spring.GetUnitCommandCount(unitID: integer) -> The integer
```

@return `The` - number of commands in the unit's queue.





Get the number of commands in a unit's queue.








### Spring.GetFactoryBuggerOff
---
```lua
function Spring.GetFactoryBuggerOff(unitID: integer)
```












### Spring.GetFactoryCounts
---
```lua
function Spring.GetFactoryCounts(
  unitID: integer,
  count: integer?,
  addCmds: boolean?
) -> counts table<number,number>?
```
@param `count` - (Default: `-1`) Number of commands to retrieve, `-1` for all.

@param `addCmds` - (Default: `false`) Retrieve commands other than buildunit


@return `counts` - Build queue count by `unitDefID` or `-cmdID`, or `nil` if unit is not found.





Gets the build queue of a factory








### Spring.GetCommandQueue
---
```lua
function Spring.GetCommandQueue(
  unitID: integer,
  count: integer
) -> commands Command[]
```
@param `count` - Number of commands to return, `-1` returns all commands, `0` returns command count.






Get the commands for a unit.

Same as `Spring.GetUnitCommands`








### Spring.GetCommandQueue
---
```lua
function Spring.GetCommandQueue(
  unitID: integer,
  count: 0
) -> The integer
```
@param `count` - Returns the number of commands in the units queue.


@return `The` - number of commands in the unit queue.





Get the count of commands for a unit.

Same as `Spring.GetUnitCommands`








### Spring.GetFullBuildQueue
---
```lua
function Spring.GetFullBuildQueue(unitID: integer) -> buildqueue table<number,number>?
```

@return `buildqueue` - indexed by unitDefID with count values





Returns the build queue








### Spring.GetRealBuildQueue
---
```lua
function Spring.GetRealBuildQueue(unitID: integer) -> buildqueue table<number,number>?
```

@return `buildqueue` - indexed by unitDefID with count values





Returns the build queue cleaned of things the unit can't build itself








### Spring.GetUnitCmdDescs
---
```lua
function Spring.GetUnitCmdDescs(unitID: integer)
```












### Spring.FindUnitCmdDesc
---
```lua
function Spring.FindUnitCmdDesc(unitID: integer)
```












### Spring.ValidFeatureID
---
```lua
function Spring.ValidFeatureID(featureID: integer) ->  boolean
```












### Spring.GetAllFeatures
---
```lua
function Spring.GetAllFeatures() -> featureIDs integer[]
```












### Spring.GetFeatureDefID
---
```lua
function Spring.GetFeatureDefID(featureID: integer) ->  number?
```












### Spring.GetFeatureTeam
---
```lua
function Spring.GetFeatureTeam(featureID: integer) ->  number?
```












### Spring.GetFeatureAllyTeam
---
```lua
function Spring.GetFeatureAllyTeam(featureID: integer) ->  number?
```












### Spring.GetFeatureHealth
---
```lua
function Spring.GetFeatureHealth(featureID: integer)
 -> health number?
 -> defHealth number
 -> resurrectProgress number

```












### Spring.GetFeatureHeight
---
```lua
function Spring.GetFeatureHeight(featureID: integer) ->  number?
```












### Spring.GetFeatureRadius
---
```lua
function Spring.GetFeatureRadius(featureID: integer) ->  number?
```












### Spring.GetFeatureMass
---
```lua
function Spring.GetFeatureMass(featureID: integer) ->  number?
```












### Spring.GetFeaturePosition
---
```lua
function Spring.GetFeaturePosition(featureID: integer)
```












### Spring.GetFeatureSeparation
---
```lua
function Spring.GetFeatureSeparation(
  featureID1: number,
  featureID2: number,
  direction: boolean?
) ->  number?
```
@param `direction` - (Default: `false`) to subtract from, default featureID1 - featureID2













### Spring.GetFeatureRotation
---
```lua
function Spring.GetFeatureRotation(featureID: integer)
 -> pitch number
 -> yaw number
 -> roll number

```

@return `pitch` - Rotation in X axis

@return `yaw` - Rotation in Y axis

@return `roll` - Rotation in Z axis





Note: PYR order








### Spring.GetFeatureDirection
---
```lua
function Spring.GetFeatureDirection(featureID: integer)
 -> frontDirX number
 -> frontDirY number
 -> frontDirZ number
 -> rightDirX number
 -> rightDirY number
 -> rightDirZ number
 -> upDirX number
 -> upDirY number
 -> upDirZ number

```












### Spring.GetFeatureVelocity
---
```lua
function Spring.GetFeatureVelocity(featureID: integer)
```












### Spring.GetFeatureHeading
---
```lua
function Spring.GetFeatureHeading(featureID: integer)
```












### Spring.GetFeatureResources
---
```lua
function Spring.GetFeatureResources(featureID: integer)
 -> metal number?
 -> defMetal number
 -> energy number
 -> defEnergy number
 -> reclaimLeft number
 -> reclaimTime number

```












### Spring.GetFeatureBlocking
---
```lua
function Spring.GetFeatureBlocking(featureID: integer)
 -> isBlocking boolean?
 -> isSolidObjectCollidable boolean
 -> isProjectileCollidable boolean
 -> isRaySegmentCollidable boolean
 -> crushable boolean
 -> blockEnemyPushing boolean
 -> blockHeightChanges boolean

```












### Spring.GetFeatureNoSelect
---
```lua
function Spring.GetFeatureNoSelect(featureID: integer) ->  boolean?
```












### Spring.GetFeatureResurrect
---
```lua
function Spring.GetFeatureResurrect(featureID: integer)
```












### Spring.GetFeatureLastAttackedPiece
---
```lua
function Spring.GetFeatureLastAttackedPiece(featureID: integer)
```












### Spring.GetFeatureCollisionVolumeData
---
```lua
function Spring.GetFeatureCollisionVolumeData(featureID: integer)
```












### Spring.GetFeaturePieceCollisionVolumeData
---
```lua
function Spring.GetFeaturePieceCollisionVolumeData(featureID: integer)
```












### Spring.GetProjectilePosition
---
```lua
function Spring.GetProjectilePosition(projectileID: integer)
 -> posX number?
 -> posY number
 -> posZ number

```












### Spring.GetProjectileDirection
---
```lua
function Spring.GetProjectileDirection(projectileID: integer)
 -> dirX number?
 -> dirY number
 -> dirZ number

```












### Spring.GetProjectileVelocity
---
```lua
function Spring.GetProjectileVelocity(projectileID: integer)
 -> velX number?
 -> velY number
 -> velZ number
 -> velW number

```












### Spring.GetProjectileGravity
---
```lua
function Spring.GetProjectileGravity(projectileID: integer) ->  number?
```












### Spring.GetPieceProjectileParams
---
```lua
function Spring.GetPieceProjectileParams(projectileID: integer)
 -> explosionFlags number?
 -> spinAngle number
 -> spinSpeed number
 -> spinVectorX number
 -> spinVectorY number
 -> spinVectorZ number

```

@return `explosionFlags` - encoded bitwise with SHATTER = 1, EXPLODE = 2, EXPLODE_ON_HIT = 2, FALL = 4, SMOKE = 8, FIRE = 16, NONE = 32, NO_CEG_TRAIL = 64, NO_HEATCLOUD = 128












### Spring.GetProjectileTarget
---
```lua
function Spring.GetProjectileTarget(projectileID: integer)
 -> targetTypeInt number?
 -> target (number|float3)

```

@return `targetTypeInt` - where
string.byte('g') := GROUND
string.byte('u') := UNIT
string.byte('f') := FEATURE
string.byte('p') := PROJECTILE

@return `target` - targetID or targetPos when targetTypeInt == string.byte('g')












### Spring.GetProjectileIsIntercepted
---
```lua
function Spring.GetProjectileIsIntercepted(projectileID: integer) ->  boolean?
```












### Spring.GetProjectileTimeToLive
---
```lua
function Spring.GetProjectileTimeToLive(projectileID: integer) ->  number?
```












### Spring.GetProjectileOwnerID
---
```lua
function Spring.GetProjectileOwnerID(projectileID: integer) ->  number?
```












### Spring.GetProjectileTeamID
---
```lua
function Spring.GetProjectileTeamID(projectileID: integer) ->  number?
```












### Spring.GetProjectileAllyTeamID
---
```lua
function Spring.GetProjectileAllyTeamID(projectileID: integer) ->  number?
```












### Spring.GetProjectileType
---
```lua
function Spring.GetProjectileType(projectileID: integer)
 -> weapon boolean?
 -> piece boolean

```












### Spring.GetProjectileDefID
---
```lua
function Spring.GetProjectileDefID(projectileID: integer) ->  number?
```





Using this to get a weaponDefID is HIGHLY preferred to indexing WeaponDefNames via GetProjectileName








### Spring.GetProjectileDamages
---
```lua
function Spring.GetProjectileDamages(
  projectileID: integer,
  tag: string
) ->  number?
```
@param `tag` - one of:
"paralyzeDamageTime"
"impulseFactor"
"impulseBoost"
"craterMult"
"craterBoost"
"dynDamageExp"
"dynDamageMin"
"dynDamageRange"
"dynDamageInverted"
"craterAreaOfEffect"
"damageAreaOfEffect"
"edgeEffectiveness"
"explosionSpeed"
- or -
an armor type index to get the damage against it.













### Spring.IsPosInMap
---
```lua
function Spring.IsPosInMap(
  x: number,
  z: number
)
 -> inPlayArea boolean
 -> inMap boolean

```

@return `inPlayArea` - whether the position is in the active play area

@return `inMap` - whether the position is in the full map area (currently this is the same as above)












### Spring.GetGroundHeight
---
```lua
function Spring.GetGroundHeight(
  x: number,
  z: number
) ->  number
```





Get ground height

On sea, this returns the negative depth of the seafloor








### Spring.GetWaterPlaneLevel
---
```lua
function Spring.GetWaterPlaneLevel() -> waterPlaneLevel number
```





Get water plane height

Water may at some point become shaped (rivers etc) but for now it is always a flat plane.
Use this function instead of GetWaterLevel to denote you are relying on that assumption.





@see Spring.GetWaterLevel




### Spring.GetWaterLevel
---
```lua
function Spring.GetWaterLevel(
  x: number,
  z: number
) -> waterLevel number
```





Get water level in a specific position

Water is currently a flat plane, so this returns the same value regardless of XZ.
However water may become more dynamic at some point so by using this you are future-proof.








### Spring.GetGroundOrigHeight
---
```lua
function Spring.GetGroundOrigHeight(
  x: number,
  z: number
) ->  number
```





Get ground height as it was at game start

Returns the original height before the ground got deformed








### Spring.GetGroundNormal
---
```lua
function Spring.GetGroundNormal(
  x: number,
  z: number,
  smoothed: boolean?
)
 -> normalX number
 -> normalY number
 -> normalZ number
 -> slope number

```
@param `smoothed` - (Default: `false`) raw or smoothed center normal













### Spring.GetGroundInfo
---
```lua
function Spring.GetGroundInfo(
  x: number,
  z: number
)
 -> ix number
 -> iz number
 -> terrainTypeIndex number
 -> name string
 -> metalExtraction number
 -> hardness number
 -> tankSpeed number
 -> kbotSpeed number
 -> hoverSpeed number
 -> shipSpeed number
 -> receiveTracks boolean

```












### Spring.GetGroundBlocked
---
```lua
function Spring.GetGroundBlocked()
```












### Spring.GetGroundExtremes
---
```lua
function Spring.GetGroundExtremes()
 -> initMinHeight number
 -> initMaxHeight number
 -> currMinHeight number
 -> currMaxHeight number

```












### Spring.GetTerrainTypeData
---
```lua
function Spring.GetTerrainTypeData(terrainTypeInfo: number)
 -> index number
 -> name string
 -> hardness number
 -> tankSpeed number
 -> kbotSpeed number
 -> hoverSpeed number
 -> shipSpeed number
 -> receiveTracks boolean

```












### Spring.GetGrass
---
```lua
function Spring.GetGrass(
  x: number,
  z: number
) ->  number
```












### Spring.GetSmoothMeshHeight
---
```lua
function Spring.GetSmoothMeshHeight(
  x: number,
  z: number
) -> height number
```












### Spring.TestMoveOrder
---
```lua
function Spring.TestMoveOrder(
  unitDefID: integer,
  posX: number,
  posY: number,
  posZ: number,
  dirX: number?,
  dirY: number?,
  dirZ: number?,
  testTerrain: boolean?,
  testObjects: boolean?,
  centerOnly: boolean?
) ->  boolean
```
@param `dirX` - (Default: `0.0`)

@param `dirY` - (Default: `0.0`)

@param `dirZ` - (Default: `0.0`)

@param `testTerrain` - (Default: `true`)

@param `testObjects` - (Default: `true`)

@param `centerOnly` - (Default: `false`)













### Spring.TestBuildOrder
---
```lua
function Spring.TestBuildOrder(
  unitDefID: integer,
  x: number,
  y: number,
  z: number,
  facing: Facing
)
 -> blocking BuildOrderBlockedStatus
 -> featureID integer?

```

@return `featureID` - A reclaimable feature in the way.












### Spring.Pos2BuildPos
---
```lua
function Spring.Pos2BuildPos(
  unitDefID: integer,
  posX: number,
  posY: number,
  posZ: number,
  buildFacing: number?
)
 -> buildPosX number
 -> buildPosY number
 -> buildPosZ number

```
@param `buildFacing` - (Default: `0`) one of SOUTH = 0, EAST = 1, NORTH = 2, WEST  = 3






Snaps a position to the building grid








### Spring.ClosestBuildPos
---
```lua
function Spring.ClosestBuildPos(
  teamID: integer,
  unitDefID: integer,
  posX: number,
  posY: number,
  posZ: number,
  searchRadius: number,
  minDistance: number,
  buildFacing: number
)
 -> buildPosX number
 -> buildPosY number
 -> buildPosZ number

```
@param `buildFacing` - one of SOUTH = 0, EAST = 1, NORTH = 2, WEST  = 3













### Spring.GetPositionLosState
---
```lua
function Spring.GetPositionLosState(
  posX: number,
  posY: number,
  posZ: number,
  allyTeamID: integer?
)
 -> inLosOrRadar boolean
 -> inLos boolean
 -> inRadar boolean
 -> inJammer boolean

```












### Spring.IsPosInLos
---
```lua
function Spring.IsPosInLos(
  posX: number,
  posY: number,
  posZ: number,
  allyTeamID: integer?
) ->  boolean
```












### Spring.IsPosInRadar
---
```lua
function Spring.IsPosInRadar(
  posX: number,
  posY: number,
  posZ: number,
  allyTeamID: integer?
) ->  boolean
```












### Spring.IsPosInAirLos
---
```lua
function Spring.IsPosInAirLos(
  posX: number,
  posY: number,
  posZ: number,
  allyTeamID: integer?
) ->  boolean
```












### Spring.GetUnitLosState
---
```lua
function Spring.GetUnitLosState(
  unitID: integer,
  allyTeamID: integer?,
  raw: true
) -> bitmask integer?
```
@param `raw` - Return a bitmask.


@return `bitmask` - A bitmask integer, or `nil` if `unitID` is invalid.

Bitmask bits:
- `1`: `LOS_INLOS` the unit is currently in the los of the allyteam,
- `2`: `LOS_INRADAR` the unit is currently in radar from the allyteam,
- `4`: `LOS_PREVLOS` the unit has previously been in los from the allyteam,
- `8`: `LOS_CONTRADAR` the unit has continuously been in radar since it was last inlos by the allyteam












### Spring.GetUnitLosState
---
```lua
function Spring.GetUnitLosState(
  unitID: integer,
  allyTeamID: integer?,
  raw: false?
) -> los { los: boolean,radar: boolean, ... }?
```
@param `raw` - Return a bitmask.


@return `los` - A table of LOS state, or `nil` if `unitID` is invalid.












### Spring.IsUnitInLos
---
```lua
function Spring.IsUnitInLos(
  unitID: integer,
  allyTeamID: integer
) -> inLos boolean
```












### Spring.IsUnitInAirLos
---
```lua
function Spring.IsUnitInAirLos(
  unitID: integer,
  allyTeamID: integer
) -> inAirLos boolean
```












### Spring.IsUnitInRadar
---
```lua
function Spring.IsUnitInRadar(
  unitID: integer,
  allyTeamID: integer
) -> inRadar boolean
```












### Spring.IsUnitInJammer
---
```lua
function Spring.IsUnitInJammer(
  unitID: integer,
  allyTeamID: integer
) -> inJammer boolean
```












### Spring.GetModelRootPiece
---
```lua
function Spring.GetModelRootPiece(modelName: string) -> index number
```

@return `index` - of the root piece












### Spring.GetModelPieceMap
---
```lua
function Spring.GetModelPieceMap(modelName: string) -> pieceInfos table<string,number>?
```

@return `pieceInfos` - where keys are piece names and values are indices












### Spring.GetModelPieceList
---
```lua
function Spring.GetModelPieceList(modelName: string) -> pieceNames string[]?
```












### Spring.GetUnitRootPiece
---
```lua
function Spring.GetUnitRootPiece(unitID: integer) -> index number
```

@return `index` - of the root piece












### Spring.GetUnitPieceMap
---
```lua
function Spring.GetUnitPieceMap(unitID: integer) -> pieceInfos table<string,number>?
```

@return `pieceInfos` - where keys are piece names and values are indices












### Spring.GetUnitPieceList
---
```lua
function Spring.GetUnitPieceList(unitID: integer) -> pieceNames string[]
```












### Spring.GetUnitPieceInfo
---
```lua
function Spring.GetUnitPieceInfo(
  unitID: integer,
  pieceIndex: integer
) -> pieceInfo PieceInfo?
```












### Spring.GetUnitPiecePosDir
---
```lua
function Spring.GetUnitPiecePosDir(
  unitID: integer,
  pieceIndex: integer
)
 -> posX number?
 -> posY number
 -> posZ number
 -> dirX number
 -> dirY number
 -> dirZ number

```












### Spring.GetUnitPiecePosition
---
```lua
function Spring.GetUnitPiecePosition(
  unitID: integer,
  pieceIndex: integer
)
 -> posX number?
 -> posY number
 -> posZ number

```












### Spring.GetUnitPieceDirection
---
```lua
function Spring.GetUnitPieceDirection(
  unitID: integer,
  pieceIndex: integer
)
 -> dirX number?
 -> dirY number
 -> dirZ number

```












### Spring.GetUnitPieceMatrix
---
```lua
function Spring.GetUnitPieceMatrix(unitID: integer)
 -> m11 number?
 -> m12 number
 -> m13 number
 -> m14 number
 -> m21 number
 -> m22 number
 -> m23 number
 -> m24 number
 -> m31 number
 -> m32 number
 -> m33 number
 -> m34 number
 -> m41 number
 -> m42 number
 -> m43 number
 -> m44 number

```












### Spring.GetFeatureRootPiece
---
```lua
function Spring.GetFeatureRootPiece(featureID: integer) -> index number
```

@return `index` - of the root piece












### Spring.GetFeaturePieceMap
---
```lua
function Spring.GetFeaturePieceMap(featureID: integer) -> pieceInfos table<string,number>
```

@return `pieceInfos` - where keys are piece names and values are indices












### Spring.GetFeaturePieceList
---
```lua
function Spring.GetFeaturePieceList(featureID: integer) -> pieceNames string[]
```












### Spring.GetFeaturePieceInfo
---
```lua
function Spring.GetFeaturePieceInfo(
  featureID: integer,
  pieceIndex: integer
) -> pieceInfo PieceInfo?
```












### Spring.GetFeaturePiecePosDir
---
```lua
function Spring.GetFeaturePiecePosDir(
  featureID: integer,
  pieceIndex: integer
)
 -> posX number?
 -> posY number
 -> posZ number
 -> dirX number
 -> dirY number
 -> dirZ number

```












### Spring.GetFeaturePiecePosition
---
```lua
function Spring.GetFeaturePiecePosition(
  featureID: integer,
  pieceIndex: integer
)
 -> posX number?
 -> posY number
 -> posZ number

```












### Spring.GetFeaturePieceDirection
---
```lua
function Spring.GetFeaturePieceDirection(
  featureID: integer,
  pieceIndex: integer
)
 -> dirX number?
 -> dirY number
 -> dirZ number

```












### Spring.GetFeaturePieceMatrix
---
```lua
function Spring.GetFeaturePieceMatrix(featureID: integer)
 -> m11 number?
 -> m12 number
 -> m13 number
 -> m14 number
 -> m21 number
 -> m22 number
 -> m23 number
 -> m24 number
 -> m31 number
 -> m32 number
 -> m33 number
 -> m34 number
 -> m41 number
 -> m42 number
 -> m43 number
 -> m44 number

```












### Spring.GetUnitScriptPiece
---
```lua
function Spring.GetUnitScriptPiece(unitID: integer) -> pieceIndices integer[]
```












### Spring.GetUnitScriptPiece
---
```lua
function Spring.GetUnitScriptPiece(
  unitID: integer,
  scriptPiece: integer
) -> pieceIndex integer
```












### Spring.GetUnitScriptNames
---
```lua
function Spring.GetUnitScriptNames(unitID: integer) -> where table<string,number>
```

@return `where` - keys are piece names and values are piece indices












### Spring.TraceRayGroundInDirection
---
```lua
function Spring.TraceRayGroundInDirection(
  posX: number,
  posY: number,
  posZ: number,
  dirX: number,
  dirY: number,
  dirZ: number,
  testWater: boolean?
)
 -> rayLength number
 -> posX number
 -> posY number
 -> posZ number

```
@param `testWater` - (Default: `true`)






Checks for a ground collision in given direction

Checks if there is surface (ground, optionally water) towards a vector
and returns the distance to the closest hit and its position, if any.








### Spring.TraceRayGroundBetweenPositions
---
```lua
function Spring.TraceRayGroundBetweenPositions(
  startX: number,
  startY: number,
  startZ: number,
  endX: number,
  endY: number,
  endZ: number,
  testWater: boolean?
)
 -> rayLength number
 -> posX number
 -> posY number
 -> posZ number

```
@param `testWater` - (Default: `true`)






Checks for a ground collision between two positions

Checks if there is surface (ground, optionally water) between two positions
and returns the distance to the closest hit and its position, if any.








### Spring.GetRadarErrorParams
---
```lua
function Spring.GetRadarErrorParams(allyTeamID: integer)
 -> radarErrorSize number?
 -> baseRadarErrorSize number
 -> baseRadarErrorMult number

```

@return `radarErrorSize` - actual radar error size (when allyTeamID is allied to current team) or base radar error size












### Spring.SetAlly
---
```lua
function Spring.SetAlly(
  firstAllyTeamID: integer,
  secondAllyTeamID: integer,
  ally: boolean
) ->  nil
```





Changes the value of the (one-sided) alliance between: firstAllyTeamID -> secondAllyTeamID.








### Spring.SetAllyTeamStartBox
---
```lua
function Spring.SetAllyTeamStartBox(
  allyTeamID: integer,
  xMin: number,
  zMin: number,
  xMax: number,
  zMax: number
) ->  nil
```
@param `xMin` - left start box boundary (elmos)

@param `zMin` - top start box boundary (elmos)

@param `xMax` - right start box boundary (elmos)

@param `zMax` - bottom start box boundary (elmos)






Changes the start box position of an allyTeam.








### Spring.AssignPlayerToTeam
---
```lua
function Spring.AssignPlayerToTeam(
  playerID: integer,
  teamID: integer
) ->  nil
```





Assigns a player to a team.








### Spring.SetGlobalLos
---
```lua
function Spring.SetGlobalLos(
  allyTeamID: integer,
  globallos: boolean
) ->  nil
```





Changes access to global line of sight for a team and its allies.








### Spring.KillTeam
---
```lua
function Spring.KillTeam(teamID: integer) ->  nil
```





Will declare a team to be dead (no further orders can be assigned to such teams units).

Gaia team cannot be killed.








### Spring.GameOver
---
```lua
function Spring.GameOver(winningAllyTeamIDs: integer[])
```
@param `winningAllyTeamIDs` - A list of winning ally team IDs. Pass
multiple winners to declare a draw. Pass no arguments if undecided (e.g.
when dropped from the host).






Declare game over.







@*returns* integer Number of accepted (valid) ally teams.


### Spring.SetTidal
---
```lua
function Spring.SetTidal(strength: number) ->  nil
```





Set tidal strength








### Spring.SetWind
---
```lua
function Spring.SetWind(
  minStrength: number,
  maxStrength: number
) ->  nil
```





Set wind strength








### Spring.AddTeamResource
---
```lua
function Spring.AddTeamResource(
  teamID: integer,
  type: ResourceName,
  amount: number
) ->  nil
```





Adds metal or energy resources to the specified team.








### Spring.UseTeamResource
---
```lua
function Spring.UseTeamResource(
  teamID: integer,
  type: ResourceName,
  amount: number
) -> hadEnough boolean
```
@param `type` - Resource type.

@param `amount` - Amount of resource to use.


@return `hadEnough` - True if enough of the resource type was available and was consumed, otherwise false.





Consumes metal or energy resources of the specified team.








### Spring.UseTeamResource
---
```lua
function Spring.UseTeamResource(
  teamID: integer,
  amount: ResourceUsage
) -> hadEnough boolean
```

@return `hadEnough` - True if enough of the resource type(s) were available and was consumed, otherwise false.





Consumes metal and/or energy resources of the specified team.








### Spring.SetTeamResource
---
```lua
function Spring.SetTeamResource(
  teamID: integer,
  resource: (ResourceName|StorageName),
  amount: number
) ->  nil
```












### Spring.SetTeamShareLevel
---
```lua
function Spring.SetTeamShareLevel(
  teamID: integer,
  type: ResourceName,
  amount: number
) ->  nil
```





Changes the resource amount for a team beyond which resources aren't stored but transferred to other allied teams if possible.








### Spring.ShareTeamResource
---
```lua
function Spring.ShareTeamResource(
  teamID_src: integer,
  teamID_recv: integer,
  type: ResourceName,
  amount: number
) ->  nil
```





Transfers resources between two teams.








### Spring.SetGameRulesParam
---
```lua
function Spring.SetGameRulesParam(
  paramName: string,
  paramValue: (number|string)?,
  losAccess: losAccess?
) ->  nil
```
@param `paramValue` - numeric paramValues in quotes will be converted to number.













### Spring.SetTeamRulesParam
---
```lua
function Spring.SetTeamRulesParam(
  teamID: integer,
  paramName: string,
  paramValue: (number|string)?,
  losAccess: losAccess?
) ->  nil
```
@param `paramValue` - numeric paramValues in quotes will be converted to number.













### Spring.SetPlayerRulesParam
---
```lua
function Spring.SetPlayerRulesParam(
  playerID: integer,
  paramName: string,
  paramValue: (number|string)?,
  losAccess: losAccess?
) ->  nil
```
@param `paramValue` - numeric paramValues in quotes will be converted to number.













### Spring.SetUnitRulesParam
---
```lua
function Spring.SetUnitRulesParam(
  unitID: integer,
  paramName: string,
  paramValue: (number|string)?,
  losAccess: losAccess?
) ->  nil
```
@param `paramValue` - numeric paramValues in quotes will be converted to number.













### Spring.SetFeatureRulesParam
---
```lua
function Spring.SetFeatureRulesParam(
  featureID: integer,
  paramName: string,
  paramValue: (number|string)?,
  losAccess: losAccess?
) ->  nil
```
@param `paramValue` - numeric paramValues in quotes will be converted to number.













### Spring.CallCOBScript
---
```lua
function Spring.CallCOBScript(
  unitID: integer,
  funcName: (integer|string)?,
  retArgs: integer,
  ...: any
) ->  number ...
```
@param `funcName` - Function ID or name.

@param `retArgs` - Number of values to return.

@param `...` - Arguments













### Spring.GetCOBScriptID
---
```lua
function Spring.GetCOBScriptID(
  unitID: integer,
  funcName: string
) -> funcID integer?
```












### Spring.CreateUnit
---
```lua
function Spring.CreateUnit(
  unitDef: (string|integer),
  posX: number,
  posY: number,
  posZ: number,
  facing: Facing,
  teamID: integer,
  build: boolean?,
  flattenGround: boolean?,
  unitID: integer?,
  builderID: integer?
) -> unitID integer?
```
@param `unitDef` - UnitDef name or ID.

@param `build` - (Default: `false`) The unit is created in "being built" state with zero `buildProgress`.

@param `flattenGround` - (Default: `true`) The unit flattens ground, if it normally does so.

@param `unitID` - Request a specific unitID.


@return `unitID` - The ID of the created unit, or `nil` if the unit could not be created.









@see Spring.DestroyUnit




### Spring.DestroyUnit
---
```lua
function Spring.DestroyUnit(
  unitID: integer,
  selfd: boolean?,
  reclaimed: boolean?,
  attackerID: integer?,
  cleanupImmediately: boolean?
) ->  nil
```
@param `selfd` - (Default: `false`) makes the unit act like it self-destructed.

@param `reclaimed` - (Default: `false`) don't show any DeathSequences, don't leave a wreckage. This does not give back the resources to the team!

@param `cleanupImmediately` - (Default: `false`) stronger version of reclaimed, removes the unit unconditionally and makes its ID available for immediate reuse (otherwise it takes a few frames)










@see Spring.CreateUnit




### Spring.TransferUnit
---
```lua
function Spring.TransferUnit(
  unitID: integer,
  newTeamID: integer,
  given: boolean?
) ->  nil
```
@param `given` - (Default: `true`) if false, the unit is captured.













### Spring.SetUnitCosts
---
```lua
function Spring.SetUnitCosts(
  unitID: integer,
  where: table<number,number>
) ->  nil
```
@param `where` - keys and values are, respectively and in this order: buildTime=amount, metalCost=amount, energyCost=amount













### Spring.SetUnitResourcing
---
```lua
function Spring.SetUnitResourcing(
  unitID: integer,
  res: string,
  amount: number
) ->  nil
```












### Spring.SetUnitResourcing
---
```lua
function Spring.SetUnitResourcing(
  unitID: integer,
  res: table<string,number>
) ->  nil
```
@param `res` - keys are: "[u|c][u|m][m|e]" unconditional | conditional, use | make, metal | energy. Values are amounts













### Spring.SetUnitStorage
---
```lua
function Spring.SetUnitStorage(
  unitID: integer,
  res: string,
  amount: number
)
```












### Spring.SetUnitStorage
---
```lua
function Spring.SetUnitStorage(
  unitID: integer,
  res: ResourceUsage
)
```
@param `res` - keys are: "[m|e]" metal | energy. Values are amounts













### Spring.SetUnitTooltip
---
```lua
function Spring.SetUnitTooltip(
  unitID: integer,
  tooltip: string
) ->  nil
```












### Spring.SetUnitHealth
---
```lua
function Spring.SetUnitHealth(
  unitID: integer,
  health: (number|SetUnitHealthAmounts)
) ->  nil
```
@param `health` - If a number, sets the units health
to that value. Pass a table to update health, capture progress, paralyze
damage, and build progress.






Note, if your game's custom shading framework doesn't support reverting into nanoframes
then reverting into nanoframes via the "build" tag will fail to render properly.





@see SetUnitHealthAmounts




### Spring.SetUnitMaxHealth
---
```lua
function Spring.SetUnitMaxHealth(
  unitID: integer,
  maxHealth: number
) ->  nil
```












### Spring.SetUnitStockpile
---
```lua
function Spring.SetUnitStockpile(
  unitID: integer,
  stockpile: number?,
  buildPercent: number?
) ->  nil
```












### Spring.SetUnitUseWeapons
---
```lua
function Spring.SetUnitUseWeapons(
  unitID: integer,
  forceUseWeapons: number?,
  allowUseWeapons: number?
) ->  nil
```












### Spring.SetUnitWeaponState
---
```lua
function Spring.SetUnitWeaponState(
  unitID: integer,
  weaponNum: number,
  states: WeaponState
) ->  nil
```












### Spring.SetUnitWeaponState
---
```lua
function Spring.SetUnitWeaponState(
  unitID: integer,
  weaponNum: number,
  key: string,
  value: number
) ->  nil
```












### Spring.SetUnitWeaponDamages
---
```lua
function Spring.SetUnitWeaponDamages(
  unitID: integer,
  weaponNum: (number|"selfDestruct"|"explode"),
  damages: WeaponDamages
) ->  nil
```












### Spring.SetUnitWeaponDamages
---
```lua
function Spring.SetUnitWeaponDamages(
  unitID: integer,
  weaponNum: (number|"selfDestruct"|"explode"),
  key: string,
  value: number
) ->  nil
```












### Spring.SetUnitMaxRange
---
```lua
function Spring.SetUnitMaxRange(
  unitID: integer,
  maxRange: number
) ->  nil
```












### Spring.SetUnitExperience
---
```lua
function Spring.SetUnitExperience(
  unitID: integer,
  experience: number
) ->  nil
```









@see Spring.GetUnitExperience




### Spring.AddUnitExperience
---
```lua
function Spring.AddUnitExperience(
  unitID: integer,
  deltaExperience: number
) ->  nil
```
@param `deltaExperience` - Can be negative to subtract, but the unit will never have negative total afterwards










@see Spring.GetUnitExperience




### Spring.SetUnitArmored
---
```lua
function Spring.SetUnitArmored(
  unitID: integer,
  armored: boolean?,
  armorMultiple: number?
) ->  nil
```












### Spring.SetUnitLosMask
---
```lua
function Spring.SetUnitLosMask(
  unitID: integer,
  allyTeam: number,
  losTypes: (number|table)
) ->  nil
```





The 3rd argument is either the bit-and combination of the following numbers:

LOS_INLOS = 1
LOS_INRADAR = 2
LOS_PREVLOS = 4
LOS_CONTRADAR = 8

or a table of the following form:

losTypes = {
[los = boolean,]
[radar = boolean,]
[prevLos = boolean,]
[contRadar = boolean]
}








### Spring.SetUnitLosState
---
```lua
function Spring.SetUnitLosState(
  unitID: integer,
  allyTeam: number,
  los: (number|table)
) ->  nil
```












### Spring.SetUnitCloak
---
```lua
function Spring.SetUnitCloak(
  unitID: integer,
  cloak: (boolean|number),
  cloakArg: (boolean|number)
) ->  nil
```





If the 2nd argument is a number, the value works like this:
1:=normal cloak
2:=for free cloak (cost no E)
3:=for free + no decloaking (except the unit is stunned)
4:=ultimate cloak (no ecost, no decloaking, no stunned decloak)

The decloak distance is only changed:
- if the 3th argument is a number or a boolean.
- if the boolean is false it takes the default decloak distance for that unitdef,
- if the boolean is true it takes the absolute value of it.








### Spring.SetUnitStealth
---
```lua
function Spring.SetUnitStealth(
  unitID: integer,
  stealth: boolean
) ->  nil
```












### Spring.SetUnitSonarStealth
---
```lua
function Spring.SetUnitSonarStealth(
  unitID: integer,
  sonarStealth: boolean
) ->  nil
```












### Spring.SetUnitSeismicSignature
---
```lua
function Spring.SetUnitSeismicSignature(
  unitID: integer,
  seismicSignature: number
) ->  nil
```












### Spring.SetUnitAlwaysVisible
---
```lua
function Spring.SetUnitAlwaysVisible(
  unitID: integer,
  alwaysVisible: boolean
) ->  nil
```












### Spring.SetUnitUseAirLos
---
```lua
function Spring.SetUnitUseAirLos(
  unitID: integer,
  useAirLos: boolean
) ->  nil
```












### Spring.SetUnitMetalExtraction
---
```lua
function Spring.SetUnitMetalExtraction(
  unitID: integer,
  depth: number,
  range: number?
) ->  nil
```
@param `depth` - corresponds to metal extraction rate

@param `range` - similar to "extractsMetal" in unitDefs.













### Spring.SetUnitHarvestStorage
---
```lua
function Spring.SetUnitHarvestStorage(
  unitID: integer,
  metal: number
) ->  nil
```





See also harvestStorage UnitDef tag.








### Spring.SetUnitBuildParams
---
```lua
function Spring.SetUnitBuildParams(
  unitID: integer,
  paramName: string,
  value: (number|boolean)
) ->  nil
```
@param `paramName` - one of `buildRange`|`buildDistance`|`buildRange3D`

@param `value` - boolean when `paramName` is `buildRange3D`, otherwise number.













### Spring.SetUnitBuildSpeed
---
```lua
function Spring.SetUnitBuildSpeed(
  builderID: integer,
  buildSpeed: number,
  repairSpeed: number?,
  reclaimSpeed: number?,
  captureSpeed: number?,
  terraformSpeed: number?
) ->  nil
```












### Spring.SetUnitNanoPieces
---
```lua
function Spring.SetUnitNanoPieces(
  builderID: integer,
  pieces: table
) ->  nil
```





This saves a lot of engine calls, by replacing: function script.QueryNanoPiece() return currentpiece end
Use it!








### Spring.SetUnitBlocking
---
```lua
function Spring.SetUnitBlocking(
  unitID: integer,
  isBlocking: boolean?,
  isSolidObjectCollidable: boolean?,
  isProjectileCollidable: boolean?,
  isRaySegmentCollidable: boolean?,
  crushable: boolean?,
  blockEnemyPushing: boolean?,
  blockHeightChanges: boolean?
) -> isBlocking boolean
```
@param `isBlocking` - If `true` add this unit to the `GroundBlockingMap`, but only if it collides with solid objects (or is being set to collide with the `isSolidObjectCollidable` argument). If `false`, remove this unit from the `GroundBlockingMap`. No change if `nil`.

@param `isSolidObjectCollidable` - Enable or disable collision with solid objects, or no change if `nil`.

@param `isProjectileCollidable` - Enable or disable collision with projectiles, or no change if `nil`.

@param `isRaySegmentCollidable` - Enable or disable collision with ray segments, or no change if `nil`.

@param `crushable` - Enable or disable crushable, or no change if `nil`.

@param `blockEnemyPushing` - Enable or disable blocking enemy pushing, or no change if `nil`.

@param `blockHeightChanges` - Enable or disable blocking height changes, or no change if `nil`.













### Spring.SetUnitCrashing
---
```lua
function Spring.SetUnitCrashing(
  unitID: integer,
  crashing: boolean
) -> success boolean
```












### Spring.SetUnitShieldState
---
```lua
function Spring.SetUnitShieldState(
  unitID: integer,
  weaponID: integer?,
  enabled: boolean?,
  power: number?
) ->  nil
```
@param `weaponID` - (Default: `-1`)













### Spring.SetUnitShieldRechargeDelay
---
```lua
function Spring.SetUnitShieldRechargeDelay(
  unitID: integer,
  weaponID: integer?,
  rechargeTime: number?
) ->  nil
```
@param `weaponID` - (optional if the unit only has one shield)

@param `rechargeTime` - (in seconds; emulates a regular hit if nil)













### Spring.SetUnitFlanking
---
```lua
function Spring.SetUnitFlanking(
  unitID: integer,
  type: string,
  arg1: number,
  y: number?,
  z: number?
) ->  nil
```
@param `type` - "dir"|"minDamage"|"maxDamage"|"moveFactor"|"mode"

@param `arg1` - x|minDamage|maxDamage|moveFactor|mode

@param `y` - only when type is "dir"

@param `z` - only when type is "dir"













### Spring.SetUnitPhysicalStateBit
---
```lua
function Spring.SetUnitPhysicalStateBit(
  unitID: integer,
  Physical
) ->  nil
```












### Spring.GetUnitPhysicalState
---
```lua
function Spring.GetUnitPhysicalState(unitID: integer) -> Unit number
```

@return `Unit` - 's PhysicalState bitmask












### Spring.SetUnitNeutral
---
```lua
function Spring.SetUnitNeutral(
  unitID: integer,
  neutral: boolean
) -> setNeutral boolean?
```












### Spring.SetUnitTarget
---
```lua
function Spring.SetUnitTarget(
  unitID: integer,
  enemyUnitID: integer?,
  dgun: boolean?,
  userTarget: boolean?,
  weaponNum: number?
) -> success boolean
```
@param `enemyUnitID` - when nil drops the units current target.

@param `dgun` - (Default: `false`)

@param `userTarget` - (Default: `false`)

@param `weaponNum` - (Default: `-1`)






Defines a unit's target.








### Spring.SetUnitTarget
---
```lua
function Spring.SetUnitTarget(
  unitID: integer,
  x: number?,
  y: number?,
  z: number?,
  dgun: boolean?,
  userTarget: boolean?,
  weaponNum: number?
) -> success boolean
```
@param `x` - when nil or not passed it will drop target and ignore other parameters

@param `dgun` - (Default: `false`)

@param `userTarget` - (Default: `false`)

@param `weaponNum` - (Default: `-1`)













### Spring.SetUnitMidAndAimPos
---
```lua
function Spring.SetUnitMidAndAimPos(
  unitID: integer,
  mpX: number,
  mpY: number,
  mpZ: number,
  apX: number,
  apY: number,
  apZ: number,
  relative: boolean?
) -> success boolean
```
@param `mpX` - new middle positionX of unit

@param `mpY` - new middle positionY of unit

@param `mpZ` - new middle positionZ of unit

@param `apX` - new positionX that enemies aim at on this unit

@param `apY` - new positionY that enemies aim at on this unit

@param `apZ` - new positionZ that enemies aim at on this unit

@param `relative` - (Default: `false`) are the new coordinates relative to world (false) or unit (true) coordinates? Also, note that apy is inverted!













### Spring.SetUnitRadiusAndHeight
---
```lua
function Spring.SetUnitRadiusAndHeight(
  unitID: integer,
  radius: number,
  height: number
) -> success boolean
```












### Spring.SetUnitBuildeeRadius
---
```lua
function Spring.SetUnitBuildeeRadius(
  unitID: integer,
  build: number
) ->  nil
```
@param `build` - radius for when targeted by build, repair, reclaim-type commands.






Sets the unit's radius for when targeted by build, repair, reclaim-type commands.








### Spring.SetUnitPieceParent
---
```lua
function Spring.SetUnitPieceParent(
  unitID: integer,
  AlteredPiece: number,
  ParentPiece: number
) ->  nil
```





Changes the pieces hierarchy of a unit by attaching a piece to a new parent.








### Spring.SetUnitPieceMatrix
---
```lua
function Spring.SetUnitPieceMatrix(
  unitID: integer,
  pieceNum: number,
  matrix: number[]
) ->  nil
```
@param `matrix` - an array of 16 floats






Sets the local (i.e. parent-relative) matrix of the given piece.

If any of the first three elements are non-zero, and also blocks all script animations from modifying it until {0, 0, 0} is passed.








### Spring.SetUnitCollisionVolumeData
---
```lua
function Spring.SetUnitCollisionVolumeData(
  unitID: integer,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  vType: number,
  tType: number,
  Axis: number
) -> enum nil
```

@return `enum` - COLVOL_TYPES {
COLVOL_TYPE_DISABLED = -1,
COLVOL_TYPE_ELLIPSOID = 0,
COLVOL_TYPE_CYLINDER,
COLVOL_TYPE_BOX,
COLVOL_TYPE_SPHERE,
COLVOL_NUM_TYPES       // number of non-disabled collision volume types
};
enum COLVOL_TESTS {
COLVOL_TEST_DISC = 0,
COLVOL_TEST_CONT = 1,
COLVOL_NUM_TESTS = 2   // number of tests
};
enum COLVOL_AXES {
COLVOL_AXIS_X   = 0,
COLVOL_AXIS_Y   = 1,
COLVOL_AXIS_Z   = 2,
COLVOL_NUM_AXES = 3    // number of collision volume axes
};












### Spring.SetUnitPieceCollisionVolumeData
---
```lua
function Spring.SetUnitPieceCollisionVolumeData(
  unitID: integer,
  pieceIndex: number,
  enable: boolean,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  volumeType: number?,
  primaryAxis: number?
) ->  nil
```












### Spring.SetUnitPieceVisible
---
```lua
function Spring.SetUnitPieceVisible(
  unitID: integer,
  pieceIndex: number,
  visible: boolean
) ->  nil
```












### Spring.SetUnitSensorRadius
---
```lua
function Spring.SetUnitSensorRadius(
  unitID: integer,
  type: ("los"|"airLos"|"radar"|"sonar"|"seismic"|"radarJammer"...),
  radius: number
) -> New number?
```

@return `New` - radius, or `nil` if unit is invalid.












### Spring.SetUnitPosErrorParams
---
```lua
function Spring.SetUnitPosErrorParams(
  unitID: integer,
  posErrorVectorX: number,
  posErrorVectorY: number,
  posErrorVectorZ: number,
  posErrorDeltaX: number,
  posErrorDeltaY: number,
  posErrorDeltaZ: number,
  nextPosErrorUpdate: number?
) ->  nil
```





Sets a unit's radar wobble

Controls how much a unit's radar dot will wobble. Note that setting
this above the allyTeam's default wobble may result in the edgemost
dot positions failing to register in ray traces, i.e. things like
native "is under cursor" checks and some Lua interfaces.








### Spring.SetUnitMoveGoal
---
```lua
function Spring.SetUnitMoveGoal(
  unitID: integer,
  goalX: number,
  goalY: number,
  goalZ: number,
  goalRadius: number?,
  moveSpeed: number?,
  moveRaw: boolean?
) ->  nil
```





Used by default commands to get in build-, attackrange etc.








### Spring.SetUnitLandGoal
---
```lua
function Spring.SetUnitLandGoal(
  unitID: integer,
  goalX: number,
  goalY: number,
  goalZ: number,
  goalRadius: number?
) ->  nil
```





Used in conjunction with Spring.UnitAttach et al. to re-implement old airbase & fuel system in Lua.








### Spring.ClearUnitGoal
---
```lua
function Spring.ClearUnitGoal(unitID: integer) ->  nil
```












### Spring.SetUnitPhysics
---
```lua
function Spring.SetUnitPhysics(
  unitID: integer,
  posX: number,
  posY: number,
  posZ: number,
  velX: number,
  velY: number,
  velZ: number,
  rotX: number,
  rotY: number,
  rotZ: number,
  dragX: number,
  dragY: number,
  dragZ: number
) ->  nil
```












### Spring.SetUnitMass
---
```lua
function Spring.SetUnitMass(
  unitID: integer,
  mass: number
) ->  nil
```












### Spring.SetUnitPosition
---
```lua
function Spring.SetUnitPosition(
  unitID: integer,
  x: number,
  z: number,
  floating: boolean?
) ->  nil
```
@param `floating` - (Default: `false`) If true, over water the position is on surface. If false, on seafloor.






Set unit position (2D)

Sets a unit's position in 2D, at terrain height.








### Spring.SetUnitPosition
---
```lua
function Spring.SetUnitPosition(
  unitID: integer,
  x: number,
  y: number,
  z: number
) ->  nil
```





Set unit position (3D)

Sets a unit's position in 3D, at an arbitrary height.








### Spring.SetUnitRotation
---
```lua
function Spring.SetUnitRotation(
  unitID: integer,
  pitch: number,
  yaw: number,
  roll: number
) ->  nil
```
@param `pitch` - Rotation in X axis

@param `yaw` - Rotation in Y axis

@param `roll` - Rotation in Z axis






Note: PYR order








### Spring.SetUnitDirection
---
```lua
function Spring.SetUnitDirection(
  unitID: integer,
  frontx: number,
  fronty: number,
  frontz: number
) ->  nil
```





Set unit front direction vector. The vector is normalized in
the engine.



@deprecated It's strongly that you use the overload that accepts
a right direction as `frontDir` alone doesn't define object orientation.






### Spring.SetUnitDirection
---
```lua
function Spring.SetUnitDirection(
  unitID: integer,
  frontx: number,
  fronty: number,
  frontz: number,
  rightx: number,
  righty: number,
  rightz: number
) ->  nil
```





Set unit front and right direction vectors.

Both vectors will be normalized in the engine.








### Spring.SetUnitHeadingAndUpDir
---
```lua
function Spring.SetUnitHeadingAndUpDir(
  unitID: integer,
  heading: Heading,
  upx: number,
  upy: number,
  upz: number
) ->  nil
```





Use this call to set up unit direction in a robust way. If unit was
completely upright, new `{upx, upy, upz}` direction will be used as new "up"
vector, the rotation set by "heading" will remain preserved.








### Spring.SetUnitVelocity
---
```lua
function Spring.SetUnitVelocity(
  unitID: integer,
  velX: number,
  velY: number,
  velZ: number
) ->  nil
```












### Spring.SetFactoryBuggerOff
---
```lua
function Spring.SetFactoryBuggerOff(
  unitID: integer,
  buggerOff: boolean?,
  offset: number?,
  radius: number?,
  relHeading: Heading?,
  spherical: boolean?,
  forced: boolean?
) -> buggerOff number?
```












### Spring.BuggerOff
---
```lua
function Spring.BuggerOff(
  x: number,
  y: number,
  z: number?,
  radius: number,
  teamID: integer,
  spherical: boolean?,
  forced: boolean?,
  excludeUnitID: integer?,
  excludeUnitDefIDs: number[]?
) ->  nil
```
@param `z` - uses ground height when unspecified

@param `spherical` - (Default: `true`)

@param `forced` - (Default: `true`)













### Spring.AddUnitDamage
---
```lua
function Spring.AddUnitDamage(
  unitID: integer,
  damage: number,
  paralyze: number?,
  attackerID: integer?,
  weaponID: integer?,
  impulseX: number?,
  impulseY: number?,
  impulseZ: number?
) ->  nil
```
@param `paralyze` - (Default: `0`) equals to the paralyzetime in the WeaponDef.

@param `attackerID` - (Default: `-1`)

@param `weaponID` - (Default: `-1`)













### Spring.AddUnitImpulse
---
```lua
function Spring.AddUnitImpulse(
  unitID: integer,
  x: number,
  y: number,
  z: number,
  decayRate: number?
) ->  nil
```












### Spring.AddUnitSeismicPing
---
```lua
function Spring.AddUnitSeismicPing(
  unitID: integer,
  pindSize: number
) ->  nil
```












### Spring.AddUnitResource
---
```lua
function Spring.AddUnitResource(
  unitID: integer,
  resource: string,
  amount: number
) ->  nil
```
@param `resource` - "m" | "e"













### Spring.UseUnitResource
---
```lua
function Spring.UseUnitResource(
  unitID: integer,
  resource: ResourceName,
  amount: number
) -> okay boolean?
```












### Spring.UseUnitResource
---
```lua
function Spring.UseUnitResource(
  unitID: integer,
  resources: ResourceUsage
) -> okay boolean?
```












### Spring.AddObjectDecal
---
```lua
function Spring.AddObjectDecal(unitID: integer) ->  nil
```












### Spring.RemoveObjectDecal
---
```lua
function Spring.RemoveObjectDecal(unitID: integer) ->  nil
```












### Spring.AddGrass
---
```lua
function Spring.AddGrass(
  x: number,
  z: number
) ->  nil
```












### Spring.RemoveGrass
---
```lua
function Spring.RemoveGrass(
  x: number,
  z: number
) ->  nil
```












### Spring.CreateFeature
---
```lua
function Spring.CreateFeature(
  featureDef: (string|integer),
  x: number,
  y: number,
  z: number,
  heading: Heading?,
  AllyTeamID: integer?,
  featureID: integer?
) -> featureID integer
```
@param `featureDef` - name or id













### Spring.DestroyFeature
---
```lua
function Spring.DestroyFeature(featureDefID: integer) ->  nil
```












### Spring.TransferFeature
---
```lua
function Spring.TransferFeature(
  featureDefID: integer,
  teamID: integer
) ->  nil
```





Feature Control








### Spring.SetFeatureAlwaysVisible
---
```lua
function Spring.SetFeatureAlwaysVisible(
  featureID: integer,
  enable: boolean
) ->  nil
```












### Spring.SetFeatureUseAirLos
---
```lua
function Spring.SetFeatureUseAirLos(
  featureID: integer,
  useAirLos: boolean
) ->  nil
```












### Spring.SetFeatureHealth
---
```lua
function Spring.SetFeatureHealth(
  featureID: integer,
  health: number
) ->  nil
```












### Spring.SetFeatureMaxHealth
---
```lua
function Spring.SetFeatureMaxHealth(
  featureID: integer,
  maxHealth: number
) ->  nil
```
@param `maxHealth` - minimum 0.1













### Spring.SetFeatureReclaim
---
```lua
function Spring.SetFeatureReclaim(
  featureID: integer,
  reclaimLeft: number
) ->  nil
```












### Spring.SetFeatureResources
---
```lua
function Spring.SetFeatureResources(
  featureID: integer,
  metal: number,
  energy: number,
  reclaimTime: number?,
  reclaimLeft: number?,
  featureDefMetal: number?,
  featureDefEnergy: number?
) ->  nil
```












### Spring.SetFeatureResurrect
---
```lua
function Spring.SetFeatureResurrect(
  featureID: integer,
  unitDef: (string|integer),
  facing: Facing?,
  progress: number?
) ->  nil
```
@param `unitDef` - Can be a number id or a string name, this allows cancelling resurrection by passing `-1`.

@param `facing` - (Default: `"south"`)

@param `progress` - Set the level of progress.













### Spring.SetFeatureMoveCtrl
---
```lua
function Spring.SetFeatureMoveCtrl(
  featureID: integer,
  enabled: true,
  initialVelocityX: number?,
  initialVelocityY: number?,
  initialVelocityZ: number?,
  accelerationX: number?,
  accelerationY: number?,
  accelerationZ: number?
)
```
@param `enabled` - Enable feature movement.

@param `initialVelocityX` - Initial velocity on X axis, or `nil` for no change.

@param `initialVelocityY` - Initial velocity on Y axis, or `nil` for no change.

@param `initialVelocityZ` - Initial velocity on Z axis, or `nil` for no change.

@param `accelerationX` - Acceleration per frame on X axis, or `nil` for no change.

@param `accelerationY` - Acceleration per frame on Y axis, or `nil` for no change.

@param `accelerationZ` - Acceleration per frame on Z axis, or `nil` for no change.






Enable feature movement control.








### Spring.SetFeatureMoveCtrl
---
```lua
function Spring.SetFeatureMoveCtrl(
  featureID: integer,
  enabled: false,
  velocityMaskX: number?,
  velocityMaskY: number?,
  velocityMaskZ: number?,
  impulseMaskX: number?,
  impulseMaskY: number?,
  impulseMaskZ: number?,
  movementMaskX: number?,
  movementMaskY: number?,
  movementMaskZ: number?
)
```
@param `enabled` - Disable feature movement.

@param `velocityMaskX` - Lock velocity change in X dimension when not using `MoveCtrl`. `0` to lock, non-zero to allow, or `nil` to for no change.

@param `velocityMaskY` - Lock velocity change in Y dimension when not using `MoveCtrl`. `0` to lock, non-zero to allow, or `nil` to for no change.

@param `velocityMaskZ` - Lock velocity change in Z dimension when not using `MoveCtrl`. `0` to lock, non-zero to allow, or `nil` to for no change.

@param `impulseMaskX` - Lock impulse in X dimension when not using `MoveCtrl`. `0` to lock, non-zero to allow, or `nil` to for no change.

@param `impulseMaskY` - Lock impulse in Y dimension when not using `MoveCtrl`. `0` to lock, non-zero to allow, or `nil` to for no change.

@param `impulseMaskZ` - Lock impulse in Z dimension when not using `MoveCtrl`. `0` to lock, non-zero to allow, or `nil` to for no change.

@param `movementMaskX` - Lock move in X dimension when not using `MoveCtrl`. `0` to lock the axis, non-zero to allow, or `nil` for no change.

@param `movementMaskY` - Lock move in Y dimension when not using `MoveCtrl`. `0` to lock the axis, non-zero to allow, or `nil` for no change.

@param `movementMaskZ` - Lock move in Z dimension when not using `MoveCtrl`. `0` to lock the axis, non-zero to allow, or `nil` for no change.






Disable feature movement control.

Optional parameter allow physics vectors to build when not using `MoveCtrl`.

It is necessary to unlock feature movement on x, z axis before changing
feature physics.

For example:

```lua
-- Unlock all movement before setting velocity.
Spring.SetFeatureMoveCtrl(featureID,false,1,1,1,1,1,1,1,1,1)

-- Set velocity.
Spring.SetFeatureVelocity(featureID,10,0,10)
```








### Spring.SetFeaturePhysics
---
```lua
function Spring.SetFeaturePhysics(
  featureID: integer,
  posX: number,
  posY: number,
  posZ: number,
  velX: number,
  velY: number,
  velZ: number,
  rotX: number,
  rotY: number,
  rotZ: number,
  dragX: number,
  dragY: number,
  dragZ: number
) ->  nil
```












### Spring.SetFeatureMass
---
```lua
function Spring.SetFeatureMass(
  featureID: integer,
  mass: number
) ->  nil
```












### Spring.SetFeaturePosition
---
```lua
function Spring.SetFeaturePosition(
  featureID: integer,
  x: number,
  y: number,
  z: number,
  snapToGround: boolean?
) ->  nil
```












### Spring.SetFeatureRotation
---
```lua
function Spring.SetFeatureRotation(
  featureID: integer,
  pitch: number,
  yaw: number,
  roll: number
) ->  nil
```
@param `pitch` - Rotation in X axis

@param `yaw` - Rotation in Y axis

@param `roll` - Rotation in Z axis






Note: PYR order








### Spring.SetFeatureDirection
---
```lua
function Spring.SetFeatureDirection(
  featureID: integer,
  frontx: number,
  fronty: number,
  frontz: number
) ->  nil
```





Set feature front direction vector. The vector is normalized in
the engine.



@deprecated It's strongly that you use the overload that accepts
a right direction as `frontDir` alone doesn't define object orientation.






### Spring.SetFeatureDirection
---
```lua
function Spring.SetFeatureDirection(
  featureID: integer,
  frontx: number,
  fronty: number,
  frontz: number,
  rightx: number,
  righty: number,
  rightz: number
) ->  nil
```





Set feature front and right direction vectors.

Both vectors will be normalized in the engine.








### Spring.SetFeatureHeadingAndUpDir
---
```lua
function Spring.SetFeatureHeadingAndUpDir(
  featureID: integer,
  heading: Heading,
  upx: number,
  upy: number,
  upz: number
) ->  nil
```





Use this call to set up feature direction in a robust way. If feature was
completely upright, new `{upx, upy, upz}` direction will be used as new "up"
vector, the rotation set by "heading" will remain preserved.








### Spring.SetFeatureVelocity
---
```lua
function Spring.SetFeatureVelocity(
  featureID: integer,
  velX: number,
  velY: number,
  velZ: number
) ->  nil
```












### Spring.SetFeatureBlocking
---
```lua
function Spring.SetFeatureBlocking(
  featureID: integer,
  isBlocking: boolean?,
  isSolidObjectCollidable: boolean?,
  isProjectileCollidable: boolean?,
  isRaySegmentCollidable: boolean?,
  crushable: boolean?,
  blockEnemyPushing: boolean?,
  blockHeightChanges: boolean?
) -> isBlocking boolean
```
@param `isBlocking` - If `true` add this feature to the `GroundBlockingMap`, but only if it collides with solid objects (or is being set to collide with the `isSolidObjectCollidable` argument). If `false`, remove this feature from the `GroundBlockingMap`. No change if `nil`.

@param `isSolidObjectCollidable` - Enable or disable collision with solid objects, or no change if `nil`.

@param `isProjectileCollidable` - Enable or disable collision with projectiles, or no change if `nil`.

@param `isRaySegmentCollidable` - Enable or disable collision with ray segments, or no change if `nil`.

@param `crushable` - Enable or disable crushable, or no change if `nil`.

@param `blockEnemyPushing` - Enable or disable blocking enemy pushing, or no change if `nil`.

@param `blockHeightChanges` - Enable or disable blocking height changes, or no change if `nil`.













### Spring.SetFeatureNoSelect
---
```lua
function Spring.SetFeatureNoSelect(
  featureID: integer,
  noSelect: boolean
) ->  nil
```












### Spring.SetFeatureMidAndAimPos
---
```lua
function Spring.SetFeatureMidAndAimPos(
  featureID: integer,
  mpX: number,
  mpY: number,
  mpZ: number,
  apX: number,
  apY: number,
  apZ: number,
  relative: boolean?
) -> success boolean
```





Check `Spring.SetUnitMidAndAimPos` for further explanation of the arguments.








### Spring.SetFeatureRadiusAndHeight
---
```lua
function Spring.SetFeatureRadiusAndHeight(
  featureID: integer,
  radius: number,
  height: number
) -> success boolean
```












### Spring.SetFeatureCollisionVolumeData
---
```lua
function Spring.SetFeatureCollisionVolumeData(
  featureID: integer,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  vType: number,
  tType: number,
  Axis: number
) ->  nil
```





Check `Spring.SetUnitCollisionVolumeData` for further explanation of the arguments.








### Spring.SetFeaturePieceCollisionVolumeData
---
```lua
function Spring.SetFeaturePieceCollisionVolumeData(
  featureID: integer,
  pieceIndex: number,
  enable: boolean,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  Axis: number,
  volumeType: number,
  primaryAxis: number?
) ->  nil
```












### Spring.SetFeaturePieceVisible
---
```lua
function Spring.SetFeaturePieceVisible(
  featureID: integer,
  pieceIndex: number,
  visible: boolean
) ->  nil
```












### Spring.SetProjectileAlwaysVisible
---
```lua
function Spring.SetProjectileAlwaysVisible(
  projectileID: integer,
  alwaysVisible: boolean
) ->  nil
```












### Spring.SetProjectileUseAirLos
---
```lua
function Spring.SetProjectileUseAirLos(
  projectileID: integer,
  useAirLos: boolean
) ->  nil
```












### Spring.SetProjectileMoveControl
---
```lua
function Spring.SetProjectileMoveControl(
  projectileID: integer,
  enable: boolean
) ->  nil
```





Disables engine movecontrol, so lua can fully control the physics.








### Spring.SetProjectilePosition
---
```lua
function Spring.SetProjectilePosition(
  projectileID: integer,
  posX: number?,
  posY: number?,
  posZ: number?
) ->  nil
```
@param `posX` - (Default: `0`)

@param `posY` - (Default: `0`)

@param `posZ` - (Default: `0`)













### Spring.SetProjectileVelocity
---
```lua
function Spring.SetProjectileVelocity(
  projectileID: integer,
  velX: number?,
  velY: number?,
  velZ: number?
) ->  nil
```
@param `velX` - (Default: `0`)

@param `velY` - (Default: `0`)

@param `velZ` - (Default: `0`)













### Spring.SetProjectileCollision
---
```lua
function Spring.SetProjectileCollision(projectileID: integer) ->  nil
```












### Spring.SetProjectileTarget
---
```lua
function Spring.SetProjectileTarget(
  projectileID: integer,
  arg1: number?,
  arg2: number?,
  posZ: number?
) -> validTarget boolean?
```
@param `arg1` - (Default: `0`) targetID or posX

@param `arg2` - (Default: `0`) targetType or posY

@param `posZ` - (Default: `0`)






targetTypeStr can be one of:
'u' - unit
'f' - feature
'p' - projectile
while targetTypeInt is one of:
string.byte('g') := GROUND
string.byte('u') := UNIT
string.byte('f') := FEATURE
string.byte('p') := PROJECTILE








### Spring.SetProjectileTimeToLive
---
```lua
function Spring.SetProjectileTimeToLive(
  projectileID: integer,
  ttl: number
) ->  nil
```
@param `ttl` - Remaining time to live in frames













### Spring.SetProjectileIsIntercepted
---
```lua
function Spring.SetProjectileIsIntercepted(projectileID: integer) ->  nil
```












### Spring.SetProjectileDamages
---
```lua
function Spring.SetProjectileDamages(
  unitID: integer,
  weaponNum: number,
  key: string,
  value: number
) ->  nil
```












### Spring.SetProjectileIgnoreTrackingError
---
```lua
function Spring.SetProjectileIgnoreTrackingError(
  projectileID: integer,
  ignore: boolean
) ->  nil
```












### Spring.SetProjectileGravity
---
```lua
function Spring.SetProjectileGravity(
  projectileID: integer,
  grav: number?
) ->  nil
```
@param `grav` - (Default: `0`)













### Spring.SetPieceProjectileParams
---
```lua
function Spring.SetPieceProjectileParams(
  projectileID: integer,
  explosionFlags: number?,
  spinAngle: number?,
  spinSpeed: number?,
  spinVectorX: number?,
  spinVectorY: number?,
  spinVectorZ: number?
) ->  nil
```












### Spring.SetProjectileCEG
---
```lua
function Spring.SetProjectileCEG(
  projectileID: integer,
  ceg_name: string
) ->  nil
```












### Spring.UnitFinishCommand
---
```lua
function Spring.UnitFinishCommand(unitID: integer)
```












### Spring.GiveOrderToUnit
---
```lua
function Spring.GiveOrderToUnit(
  unitID: integer,
  cmdID: (CMD|integer),
  params: CreateCommandParams?,
  options: CreateCommandOptions?,
  timeout: integer?
) -> unitOrdered boolean
```
@param `cmdID` - The command ID.

@param `params` - Parameters for the given command.













### Spring.GiveOrderToUnitMap
---
```lua
function Spring.GiveOrderToUnitMap(
  unitMap: table<integer,any>,
  cmdID: (CMD|integer),
  params: CreateCommandParams?,
  options: CreateCommandOptions?,
  timeout: integer?
) -> unitsOrdered integer
```
@param `unitMap` - A table with unit IDs as keys.

@param `cmdID` - The command ID.

@param `params` - Parameters for the given command.


@return `unitsOrdered` - The number of units ordered.





Give order to multiple units, specified by table keys.








### Spring.GiveOrderToUnitArray
---
```lua
function Spring.GiveOrderToUnitArray(
  unitIDs: integer[],
  cmdID: (CMD|integer),
  params: CreateCommandParams?,
  options: CreateCommandOptions?,
  timeout: integer?
) -> unitsOrdered integer
```
@param `unitIDs` - An array of unit IDs.

@param `cmdID` - The command ID.

@param `params` - Parameters for the given command.


@return `unitsOrdered` - The number of units ordered.












### Spring.GiveOrderArrayToUnit
---
```lua
function Spring.GiveOrderArrayToUnit(
  unitID: integer,
  commands: CreateCommand[]
) -> ordersGiven boolean
```












### Spring.GiveOrderArrayToUnitMap
---
```lua
function Spring.GiveOrderArrayToUnitMap(
  unitMap: table<integer,any>,
  commands: CreateCommand[]
) -> unitsOrdered integer
```
@param `unitMap` - A table with unit IDs as keys.


@return `unitsOrdered` - The number of units ordered.












### Spring.GiveOrderArrayToUnitArray
---
```lua
function Spring.GiveOrderArrayToUnitArray(
  unitIDs: integer[],
  commands: CreateCommand[],
  pairwise: boolean?
) -> unitsOrdered integer
```
@param `unitIDs` - Array of unit IDs.

@param `pairwise` - (Default: `false`) When `false`, assign all commands to each unit.

When `true`, assign commands according to index between units and cmds arrays.

If `len(unitArray) < len(cmdArray)` only the first `len(unitArray)` commands
will be assigned, and vice-versa.


@return `unitsOrdered` - The number of units ordered.












### Spring.LevelHeightMap
---
```lua
function Spring.LevelHeightMap(
  x1: number,
  z1: number,
  x2_height: number,
  z2: number?,
  height: number?
) ->  nil
```
@param `x2_height` - if y2 and height are nil then this parameter is the height






Set a certain height to a point or rectangle area on the world








### Spring.AdjustHeightMap
---
```lua
function Spring.AdjustHeightMap(
  x1: number,
  y1: number,
  x2_height: number,
  y2: number?,
  height: number?
) ->  nil
```
@param `x2_height` - if y2 and height are nil then this parameter is the height






Add a certain height to a point or rectangle area on the world








### Spring.RevertHeightMap
---
```lua
function Spring.RevertHeightMap(
  x1: number,
  y1: number,
  x2_factor: number,
  y2: number?,
  factor: number?
) ->  nil
```
@param `x2_factor` - if y2 and factor are nil then this parameter is the factor






Restore original map height to a point or rectangle area on the world








### Spring.AddHeightMap
---
```lua
function Spring.AddHeightMap(
  x: number,
  z: number,
  height: number
) -> newHeight integer?
```





Can only be called in `Spring.SetHeightMapFunc`








### Spring.SetHeightMap
---
```lua
function Spring.SetHeightMap(
  x: number,
  z: number,
  height: number,
  terraform: number?
) -> absHeightDiff integer?
```
@param `terraform` - (Default: `1`) Scaling factor.


@return `absHeightDiff` - If `0`, nothing will be changed (the terraform starts), if `1` the terraform will be finished.





Can only be called in `Spring.SetHeightMapFunc`. The terraform argument is








### Spring.SetHeightMapFunc
---
```lua
function Spring.SetHeightMapFunc(
  luaFunction: function,
  arg: number,
  ...: number
) -> absTotalHeightMapAmountChanged integer?
```





Example code:

```lua
function Spring.SetHeightMapFunc(function()
  for z=0,Game.mapSizeZ, Game.squareSize do
    for x=0,Game.mapSizeX, Game.squareSize do
      Spring.SetHeightMap( x, z, 200 + 20 * math.cos((x + z) / 90) )
    end
  end
end)
```








### Spring.LevelOriginalHeightMap
---
```lua
function Spring.LevelOriginalHeightMap(
  x1: number,
  y1: number,
  x2_height: number,
  y2: number?,
  height: number?
) ->  nil
```
@param `x2_height` - if y2 and height are nil then this parameter is the height






Set a height to a point or rectangle area to the original map height cache








### Spring.AdjustOriginalHeightMap
---
```lua
function Spring.AdjustOriginalHeightMap(
  x1: number,
  y1: number,
  x2_height: number,
  y2: number?,
  height: number?
) ->  nil
```
@param `x2_height` - if y2 and height are nil then this parameter is the height






Add height to a point or rectangle area to the original map height cache








### Spring.RevertOriginalHeightMap
---
```lua
function Spring.RevertOriginalHeightMap(
  x1: number,
  y1: number,
  x2_factor: number,
  y2: number?,
  factor: number?
) ->  nil
```
@param `x2_factor` - if y2 and factor are nil then this parameter is the factor






Restore original map height cache to a point or rectangle area on the world








### Spring.AddOriginalHeightMap
---
```lua
function Spring.AddOriginalHeightMap(
  x: number,
  y: number,
  height: number
) ->  nil
```





Can only be called in `Spring.SetOriginalHeightMapFunc`








### Spring.SetOriginalHeightMap
---
```lua
function Spring.SetOriginalHeightMap(
  x: number,
  y: number,
  height: number,
  factor: number?
) ->  nil
```





Can only be called in `Spring.SetOriginalHeightMapFunc`








### Spring.SetOriginalHeightMapFunc
---
```lua
function Spring.SetOriginalHeightMapFunc(heightMapFunc: function) ->  nil
```





Cannot recurse on itself








### Spring.RebuildSmoothMesh
---
```lua
function Spring.RebuildSmoothMesh() ->  nil
```





Heightmap changes normally take up to 25s to propagate to the smooth mesh.
Use to force a mapwide update immediately.








### Spring.LevelSmoothMesh
---
```lua
function Spring.LevelSmoothMesh(
  x1: number,
  z1: number,
  x2: number?,
  z2: number?,
  height: number
) ->  nil
```












### Spring.AdjustSmoothMesh
---
```lua
function Spring.AdjustSmoothMesh(
  x1: number,
  z1: number,
  x2: number?,
  z2: number?,
  height: number
) ->  nil
```












### Spring.RevertSmoothMesh
---
```lua
function Spring.RevertSmoothMesh(
  x1: number,
  z1: number,
  x2: number?,
  z2: number?,
  origFactor: number
) ->  nil
```












### Spring.AddSmoothMesh
---
```lua
function Spring.AddSmoothMesh(
  x: number,
  z: number,
  height: number
) -> height number?
```

@return `height` - The new height, or `nil` if coordinates are invalid.





Can only be called in `Spring.SetSmoothMeshFunc`.








### Spring.SetSmoothMesh
---
```lua
function Spring.SetSmoothMesh(
  x: number,
  z: number,
  height: number,
  terraform: number?
) -> The number?
```
@param `terraform` - (Default: `1`)


@return `The` - absolute height difference, or `nil` if coordinates are invalid.





Can only be called in `Spring.SetSmoothMeshFunc`.








### Spring.SetSmoothMeshFunc
---
```lua
function Spring.SetSmoothMeshFunc(
  luaFunction: function,
  arg: any,
  ...: any
) -> absTotalHeightMapAmountChanged number?
```












### Spring.SetMapSquareTerrainType
---
```lua
function Spring.SetMapSquareTerrainType(
  x: number,
  z: number,
  newType: number
) -> oldType integer?
```












### Spring.SetTerrainTypeData
---
```lua
function Spring.SetTerrainTypeData(
  typeIndex: number,
  speedTanks: number?,
  speedKBOts: number?,
  speedHovers: number?,
  speedShips: number?
) ->  boolean?
```
@param `speedTanks` - (Default: nil)

@param `speedKBOts` - (Default: nil)

@param `speedHovers` - (Default: nil)

@param `speedShips` - (Default: nil)


@return  - true












### Spring.SetSquareBuildingMask
---
```lua
function Spring.SetSquareBuildingMask(
  x: number,
  z: number,
  mask: number
) -> See nil
```

@return `See` - also buildingMask unitdef tag.












### Spring.UnitWeaponFire
---
```lua
function Spring.UnitWeaponFire(
  unitID: integer,
  weaponID: integer
) ->  nil
```












### Spring.UnitWeaponHoldFire
---
```lua
function Spring.UnitWeaponHoldFire(
  unitID: integer,
  weaponID: integer
) ->  nil
```












### Spring.ForceUnitCollisionUpdate
---
```lua
function Spring.ForceUnitCollisionUpdate(unitID: integer) ->  nil
```





Prevent collision checks from working on outdated data

There's a rare edge case that requires units to be in specific positions
and being shot by specific weapons but which can result in shots ghosting
through the unit. This is because the unit's collision volume is stale.
The `movement.unitQuadPositionUpdateRate` modrule controls this behaviour
and can guarantee 100% correctness if set to 1, but the default value is 3
and large-scale games generally don't want to set it so low. This function
lets you guarantee success for important weapons regardless of how high
the normal update rate is set.








### Spring.UnitAttach
---
```lua
function Spring.UnitAttach(
  transporterID: integer,
  passengerID: integer,
  pieceNum: number
) ->  nil
```












### Spring.UnitDetach
---
```lua
function Spring.UnitDetach(passengerID: integer) ->  nil
```












### Spring.UnitDetachFromAir
---
```lua
function Spring.UnitDetachFromAir(passengerID: integer) ->  nil
```












### Spring.SetUnitLoadingTransport
---
```lua
function Spring.SetUnitLoadingTransport(
  passengerID: integer,
  transportID: integer
) ->  nil
```





Disables collisions between the two units to allow colvol intersection during the approach.








### Spring.SpawnProjectile
---
```lua
function Spring.SpawnProjectile(
  weaponDefID: integer,
  projectileParams: ProjectileParams
) -> projectileID integer?
```












### Spring.DeleteProjectile
---
```lua
function Spring.DeleteProjectile(projectileID: integer) ->  nil
```





Silently removes projectiles (no explosion).








### Spring.SpawnExplosion
---
```lua
function Spring.SpawnExplosion(
  posX: number?,
  posY: number?,
  posZ: number?,
  dirX: number?,
  dirY: number?,
  dirZ: number?,
  explosionParams: ExplosionParams
) ->  nil
```
@param `posX` - (Default: `0`)

@param `posY` - (Default: `0`)

@param `posZ` - (Default: `0`)

@param `dirX` - (Default: `0`)

@param `dirY` - (Default: `0`)

@param `dirZ` - (Default: `0`)













### Spring.SpawnCEG
---
```lua
function Spring.SpawnCEG(
  cegname: string,
  posX: number?,
  posY: number?,
  posZ: number?,
  dirX: number?,
  dirY: number?,
  dirZ: number?,
  radius: number?,
  damage: number?
)
 -> success boolean?
 -> cegID integer

```
@param `posX` - (Default: `0`)

@param `posY` - (Default: `0`)

@param `posZ` - (Default: `0`)

@param `dirX` - (Default: `0`)

@param `dirY` - (Default: `0`)

@param `dirZ` - (Default: `0`)

@param `radius` - (Default: `0`)

@param `damage` - (Default: `0`)













### Spring.SpawnSFX
---
```lua
function Spring.SpawnSFX(
  unitID: integer?,
  sfxID: integer?,
  posX: number?,
  posY: number?,
  posZ: number?,
  dirX: number?,
  dirY: number?,
  dirZ: number?,
  radius: number?,
  damage: number?,
  absolute: boolean?
) -> success boolean?
```
@param `unitID` - (Default: `0`)

@param `sfxID` - (Default: `0`)

@param `posX` - (Default: `0`)

@param `posY` - (Default: `0`)

@param `posZ` - (Default: `0`)

@param `dirX` - (Default: `0`)

@param `dirY` - (Default: `0`)

@param `dirZ` - (Default: `0`)

@param `radius` - (Default: `0`)

@param `damage` - (Default: `0`)






Equal to the UnitScript versions of EmitSFX, but takes position and direction arguments (in either unit- or piece-space) instead of a piece index.








### Spring.SetNoPause
---
```lua
function Spring.SetNoPause(noPause: boolean) ->  nil
```












### Spring.SetExperienceGrade
---
```lua
function Spring.SetExperienceGrade(
  expGrade: number,
  ExpPowerScale: number?,
  ExpHealthScale: number?,
  ExpReloadScale: number?
) ->  nil
```





Defines how often `Callins.UnitExperience` will be called.








### Spring.SetRadarErrorParams
---
```lua
function Spring.SetRadarErrorParams(
  allyTeamID: integer,
  allyteamErrorSize: number,
  baseErrorSize: number?,
  baseErrorMult: number?
) ->  nil
```












### Spring.EditUnitCmdDesc
---
```lua
function Spring.EditUnitCmdDesc(
  unitID: integer,
  cmdDescID: integer,
  cmdArray: CommandDescription
)
```












### Spring.InsertUnitCmdDesc
---
```lua
function Spring.InsertUnitCmdDesc(
  unitID: integer,
  index: integer,
  cmdDesc: CommandDescription
)
```





Insert a command description at a specific index.








### Spring.InsertUnitCmdDesc
---
```lua
function Spring.InsertUnitCmdDesc(
  unitID: integer,
  cmdDesc: CommandDescription
)
```





Insert a command description at the last position.








### Spring.RemoveUnitCmdDesc
---
```lua
function Spring.RemoveUnitCmdDesc(
  unitID: integer,
  cmdDescID: integer?
)
```












### Spring.IsReplay
---
```lua
function Spring.IsReplay() -> isReplay boolean?
```












### Spring.GetReplayLength
---
```lua
function Spring.GetReplayLength() -> timeInSeconds number?
```












### Spring.GetGameName
---
```lua
function Spring.GetGameName() -> name string
```












### Spring.GetMenuName
---
```lua
function Spring.GetMenuName() -> name string
```

@return `name` - name .. version from Modinfo.lua. E.g. "Spring: 1944 test-5640-ac2d15b".












### Spring.GetProfilerTimeRecord
---
```lua
function Spring.GetProfilerTimeRecord(
  profilerName: string,
  frameData: boolean?
)
 -> total number
 -> current number
 -> max_dt number
 -> time_pct number
 -> peak_pct number
 -> frameData table<number,number>?

```
@param `frameData` - (Default: `false`)


@return `total` - in ms

@return `current` - in ms

@return `frameData` - Table where key is the frame index and value is duration.












### Spring.GetProfilerRecordNames
---
```lua
function Spring.GetProfilerRecordNames() -> profilerNames string[]
```












### Spring.GetLuaMemUsage
---
```lua
function Spring.GetLuaMemUsage()
 -> luaHandleAllocedMem number
 -> luaHandleNumAllocs number
 -> luaGlobalAllocedMem number
 -> luaGlobalNumAllocs number
 -> luaUnsyncedGlobalAllocedMem number
 -> luaUnsyncedGlobalNumAllocs number
 -> luaSyncedGlobalAllocedMem number
 -> luaSyncedGlobalNumAllocs number

```

@return `luaHandleAllocedMem` - in kilobytes

@return `luaHandleNumAllocs` - divided by 1000

@return `luaGlobalAllocedMem` - in kilobytes

@return `luaGlobalNumAllocs` - divided by 1000

@return `luaUnsyncedGlobalAllocedMem` - in kilobytes

@return `luaUnsyncedGlobalNumAllocs` - divided by 1000

@return `luaSyncedGlobalAllocedMem` - in kilobytes

@return `luaSyncedGlobalNumAllocs` - divided by 1000












### Spring.GetVidMemUsage
---
```lua
function Spring.GetVidMemUsage()
 -> usedMem number
 -> availableMem number

```

@return `usedMem` - in MB

@return `availableMem` - in MB












### Spring.GetTimer
---
```lua
function Spring.GetTimer() ->  integer
```





Get a timer with millisecond resolution








### Spring.GetTimerMicros
---
```lua
function Spring.GetTimerMicros() ->  integer
```





Get a timer with microsecond resolution








### Spring.GetFrameTimer
---
```lua
function Spring.GetFrameTimer(lastFrameTime: boolean?) ->  integer
```
@param `lastFrameTime` - (Default: `false`) whether to use last frame time instead of last frame start






Get a timer for the start of the frame

This should give better results for camera interpolations








### Spring.DiffTimers
---
```lua
function Spring.DiffTimers(
  endTimer: integer,
  startTimer: integer,
  returnMs: boolean?,
  fromMicroSecs: boolean?
) -> timeAmount number
```
@param `returnMs` - (Default: `false`) whether to return `timeAmount` in milliseconds as opposed to seconds

@param `fromMicroSecs` - (Default: `false`) whether timers are in microseconds instead of milliseconds













### Spring.GetNumDisplays
---
```lua
function Spring.GetNumDisplays() -> numDisplays number
```

@return `numDisplays` - as returned by `SDL_GetNumVideoDisplays`












### Spring.GetViewGeometry
---
```lua
function Spring.GetViewGeometry()
 -> viewSizeX number
 -> viewSizeY number
 -> viewPosX number
 -> viewPosY number

```

@return `viewSizeX` - in px

@return `viewSizeY` - in px

@return `viewPosX` - offset from leftmost screen left border in px

@return `viewPosY` - offset from bottommost screen bottom border in px





Get main view geometry (map and game rendering)








### Spring.GetDualViewGeometry
---
```lua
function Spring.GetDualViewGeometry()
 -> dualViewSizeX number
 -> dualViewSizeY number
 -> dualViewPosX number
 -> dualViewPosY number

```

@return `dualViewSizeX` - in px

@return `dualViewSizeY` - in px

@return `dualViewPosX` - offset from leftmost screen left border in px

@return `dualViewPosY` - offset from bottommost screen bottom border in px





Get dual view geometry (minimap when enabled)








### Spring.GetWindowGeometry
---
```lua
function Spring.GetWindowGeometry()
 -> winSizeX number
 -> winSizeY number
 -> winPosX number
 -> winPosY number
 -> windowBorderTop number
 -> windowBorderLeft number
 -> windowBorderBottom number
 -> windowBorderRight number

```

@return `winSizeX` - in px

@return `winSizeY` - in px

@return `winPosX` - in px

@return `winPosY` - in px

@return `windowBorderTop` - in px

@return `windowBorderLeft` - in px

@return `windowBorderBottom` - in px

@return `windowBorderRight` - in px





Get main window geometry








### Spring.GetWindowDisplayMode
---
```lua
function Spring.GetWindowDisplayMode()
 -> width number
 -> height number
 -> bits number
 -> refresh number

```

@return `width` - in px

@return `height` - in px

@return `bits` - per pixel

@return `refresh` - rate in Hz





Get main window display mode








### Spring.GetScreenGeometry
---
```lua
function Spring.GetScreenGeometry(
  displayIndex: number?,
  queryUsable: boolean?
)
 -> screenSizeX number
 -> screenSizeY number
 -> screenPosX number
 -> screenPosY number
 -> windowBorderTop number
 -> windowBorderLeft number
 -> windowBorderBottom number
 -> windowBorderRight number
 -> screenUsableSizeX number?
 -> screenUsableSizeY number?
 -> screenUsablePosX number?
 -> screenUsablePosY number?

```
@param `displayIndex` - (Default: `-1`)

@param `queryUsable` - (Default: `false`)


@return `screenSizeX` - in px

@return `screenSizeY` - in px

@return `screenPosX` - in px

@return `screenPosY` - in px

@return `windowBorderTop` - in px

@return `windowBorderLeft` - in px

@return `windowBorderBottom` - in px

@return `windowBorderRight` - in px

@return `screenUsableSizeX` - in px

@return `screenUsableSizeY` - in px

@return `screenUsablePosX` - in px

@return `screenUsablePosY` - in px





Get screen geometry








### Spring.GetMiniMapGeometry
---
```lua
function Spring.GetMiniMapGeometry()
 -> minimapPosX number
 -> minimapPosY number
 -> minimapSizeX number
 -> minimapSizeY number
 -> minimized boolean
 -> maximized boolean

```

@return `minimapPosX` - in px

@return `minimapPosY` - in px

@return `minimapSizeX` - in px

@return `minimapSizeY` - in px





Get minimap geometry








### Spring.GetMiniMapRotation
---
```lua
function Spring.GetMiniMapRotation() -> amount number
```

@return `amount` - in radians





Get minimap rotation








### Spring.GetMiniMapDualScreen
---
```lua
function Spring.GetMiniMapDualScreen() -> position ("left"|"right"|false)
```

@return `position` - `"left"` or `"right"` when dual screen is enabled, otherwise `false`.












### Spring.GetSelectionBox
---
```lua
function Spring.GetSelectionBox()
 -> left number?
 -> top number?
 -> right number?
 -> bottom number?

```





Get vertices from currently active selection box

Returns nil when selection box is inactive





@see Spring.GetUnitsInScreenRectangle




### Spring.GetDrawSelectionInfo
---
```lua
function Spring.GetDrawSelectionInfo() ->  boolean
```












### Spring.IsAboveMiniMap
---
```lua
function Spring.IsAboveMiniMap(
  x: number,
  y: number
) -> isAbove boolean
```












### Spring.GetDrawFrame
---
```lua
function Spring.GetDrawFrame()
 -> low_16bit number
 -> high_16bit number

```












### Spring.GetFrameTimeOffset
---
```lua
function Spring.GetFrameTimeOffset() -> offset number?
```

@return `offset` - of the current draw frame from the last sim frame, expressed in fractions of a frame





Ideally, when running 30hz sim, and 60hz rendering, the draw frames should
have and offset of either 0.0 frames, or 0.5 frames.

When draw frames are not integer multiples of sim frames, some interpolation
happens, and this timeoffset shows how far along it is.








### Spring.GetLastUpdateSeconds
---
```lua
function Spring.GetLastUpdateSeconds() -> lastUpdateSeconds number?
```












### Spring.GetVideoCapturingMode
---
```lua
function Spring.GetVideoCapturingMode() -> allowRecord boolean
```












### Spring.IsUnitAllied
---
```lua
function Spring.IsUnitAllied(unitID: integer) -> isAllied boolean?
```

@return `isAllied` - nil with unitID cannot be parsed












### Spring.IsUnitSelected
---
```lua
function Spring.IsUnitSelected(unitID: integer) -> isSelected boolean?
```

@return `isSelected` - nil when unitID cannot be parsed












### Spring.GetUnitLuaDraw
---
```lua
function Spring.GetUnitLuaDraw(unitID: integer) -> draw boolean?
```

@return `draw` - nil when unitID cannot be parsed












### Spring.GetUnitNoDraw
---
```lua
function Spring.GetUnitNoDraw(unitID: integer) -> nil boolean?
```

@return `nil` - when unitID cannot be parsed












### Spring.GetUnitEngineDrawMask
---
```lua
function Spring.GetUnitEngineDrawMask(unitID: integer) -> nil boolean?
```

@return `nil` - when unitID cannot be parsed












### Spring.GetUnitAlwaysUpdateMatrix
---
```lua
function Spring.GetUnitAlwaysUpdateMatrix(unitID: integer) -> nil boolean?
```

@return `nil` - when unitID cannot be parsed












### Spring.GetUnitDrawFlag
---
```lua
function Spring.GetUnitDrawFlag(unitID: integer) -> nil number?
```

@return `nil` - when unitID cannot be parsed












### Spring.GetUnitNoMinimap
---
```lua
function Spring.GetUnitNoMinimap(unitID: integer) -> nil boolean?
```

@return `nil` - when unitID cannot be parsed












### Spring.GetUnitNoGroup
---
```lua
function Spring.GetUnitNoGroup(unitID: integer) -> noGroup boolean?
```

@return `noGroup` - `true` if the unit is not allowed to be added to a group, `false` if it is allowed to be added to a group, or `nil` when `unitID` is not valid.





Check if a unit is not allowed to be added to a group by a player.








### Spring.GetUnitNoSelect
---
```lua
function Spring.GetUnitNoSelect(unitID: integer) -> noSelect boolean?
```

@return `noSelect` - `nil` when `unitID` cannot be parsed.












### Spring.UnitIconGetDraw
---
```lua
function Spring.UnitIconGetDraw(unitID: integer) -> drawIcon boolean?
```

@return `drawIcon` - `true` if icon is being drawn, `nil` when unitID is invalid, otherwise `false`.












### Spring.GetUnitSelectionVolumeData
---
```lua
function Spring.GetUnitSelectionVolumeData(unitID: integer)
 -> scaleX number?
 -> scaleY number
 -> scaleZ number
 -> offsetX number
 -> offsetY number
 -> offsetZ number
 -> volumeType number
 -> useContHitTest number
 -> getPrimaryAxis number
 -> ignoreHits boolean

```

@return `scaleX` - nil when unitID cannot be parsed












### Spring.GetFeatureLuaDraw
---
```lua
function Spring.GetFeatureLuaDraw(featureID: integer) -> nil boolean?
```

@return `nil` - when featureID cannot be parsed












### Spring.GetFeatureNoDraw
---
```lua
function Spring.GetFeatureNoDraw(featureID: integer) -> nil boolean?
```

@return `nil` - when featureID cannot be parsed












### Spring.GetFeatureEngineDrawMask
---
```lua
function Spring.GetFeatureEngineDrawMask(featureID: integer) -> nil boolean?
```

@return `nil` - when featureID cannot be parsed












### Spring.GetFeatureAlwaysUpdateMatrix
---
```lua
function Spring.GetFeatureAlwaysUpdateMatrix(featureID: integer) -> nil boolean?
```

@return `nil` - when featureID cannot be parsed












### Spring.GetFeatureDrawFlag
---
```lua
function Spring.GetFeatureDrawFlag(featureID: integer) -> nil number?
```

@return `nil` - when featureID cannot be parsed












### Spring.GetFeatureSelectionVolumeData
---
```lua
function Spring.GetFeatureSelectionVolumeData(featureID: integer)
 -> scaleX number?
 -> scaleY number
 -> scaleZ number
 -> offsetX number
 -> offsetY number
 -> offsetZ number
 -> volumeType number
 -> useContHitTest number
 -> getPrimaryAxis number
 -> ignoreHits boolean

```

@return `scaleX` - nil when unitID cannot be parsed












### Spring.GetUnitTransformMatrix
---
```lua
function Spring.GetUnitTransformMatrix(unitID: integer)
 -> m11 number?
 -> m12 number
 -> m13 number
 -> m14 number
 -> m21 number
 -> m22 number
 -> m23 number
 -> m24 number
 -> m31 number
 -> m32 number
 -> m33 number
 -> m34 number
 -> m41 number
 -> m42 number
 -> m43 number
 -> m44 number

```

@return `m11` - nil when unitID cannot be parsed












### Spring.GetFeatureTransformMatrix
---
```lua
function Spring.GetFeatureTransformMatrix(featureID: integer)
 -> m11 number?
 -> m12 number
 -> m13 number
 -> m14 number
 -> m21 number
 -> m22 number
 -> m23 number
 -> m24 number
 -> m31 number
 -> m32 number
 -> m33 number
 -> m34 number
 -> m41 number
 -> m42 number
 -> m43 number
 -> m44 number

```

@return `m11` - nil when featureID cannot be parsed












### Spring.IsUnitInView
---
```lua
function Spring.IsUnitInView(unitID: integer) -> inView boolean?
```

@return `inView` - nil when unitID cannot be parsed












### Spring.IsUnitVisible
---
```lua
function Spring.IsUnitVisible(
  unitID: integer,
  radius: number?,
  checkIcon: boolean
) -> isVisible boolean?
```
@param `radius` - unitRadius when not specified


@return `isVisible` - nil when unitID cannot be parsed












### Spring.IsUnitIcon
---
```lua
function Spring.IsUnitIcon(unitID: integer) -> isUnitIcon boolean?
```

@return `isUnitIcon` - nil when unitID cannot be parsed












### Spring.IsAABBInView
---
```lua
function Spring.IsAABBInView(
  minX: number,
  minY: number,
  minZ: number,
  maxX: number,
  maxY: number,
  maxZ: number
) -> inView boolean
```












### Spring.IsSphereInView
---
```lua
function Spring.IsSphereInView(
  posX: number,
  posY: number,
  posZ: number,
  radius: number?
) -> inView boolean
```
@param `radius` - (Default: `0`)













### Spring.GetUnitViewPosition
---
```lua
function Spring.GetUnitViewPosition(
  unitID: integer,
  midPos: boolean?
)
 -> x number?
 -> y number
 -> z number

```
@param `midPos` - (Default: `false`)


@return `x` - nil when unitID cannot be parsed












### Spring.GetVisibleUnits
---
```lua
function Spring.GetVisibleUnits(
  teamID: integer?,
  radius: number?,
  icons: boolean?
) -> unitIDs number[]?
```
@param `teamID` - (Default: `-1`)

@param `radius` - (Default: `30`)

@param `icons` - (Default: `true`)













### Spring.GetVisibleFeatures
---
```lua
function Spring.GetVisibleFeatures(
  teamID: integer?,
  radius: number?,
  icons: boolean?,
  geos: boolean?
) -> featureIDs number[]?
```
@param `teamID` - (Default: `-1`)

@param `radius` - (Default: `30`)

@param `icons` - (Default: `true`)

@param `geos` - (Default: `true`)













### Spring.GetVisibleProjectiles
---
```lua
function Spring.GetVisibleProjectiles(
  allyTeamID: integer?,
  addSyncedProjectiles: boolean?,
  addWeaponProjectiles: boolean?,
  addPieceProjectiles: boolean?
) -> projectileIDs number[]?
```
@param `allyTeamID` - (Default: `-1`)

@param `addSyncedProjectiles` - (Default: `true`)

@param `addWeaponProjectiles` - (Default: `true`)

@param `addPieceProjectiles` - (Default: `true`)













### Spring.GetRenderUnits
---
```lua
function Spring.GetRenderUnits()
```












### Spring.GetRenderUnitsDrawFlagChanged
---
```lua
function Spring.GetRenderUnitsDrawFlagChanged()
```












### Spring.GetRenderFeatures
---
```lua
function Spring.GetRenderFeatures()
```












### Spring.GetRenderFeaturesDrawFlagChanged
---
```lua
function Spring.GetRenderFeaturesDrawFlagChanged()
```












### Spring.ClearUnitsPreviousDrawFlag
---
```lua
function Spring.ClearUnitsPreviousDrawFlag() ->  nil
```












### Spring.ClearFeaturesPreviousDrawFlag
---
```lua
function Spring.ClearFeaturesPreviousDrawFlag() ->  nil
```












### Spring.GetUnitsInScreenRectangle
---
```lua
function Spring.GetUnitsInScreenRectangle(
  left: number,
  top: number,
  right: number,
  bottom: number,
  allegiance: number?
) -> unitIDs number[]?
```
@param `allegiance` - (Default: `-1`) teamID when > 0, when < 0 one of AllUnits = -1, MyUnits = -2, AllyUnits = -3, EnemyUnits = -4






Get units inside a rectangle area on the map








### Spring.GetFeaturesInScreenRectangle
---
```lua
function Spring.GetFeaturesInScreenRectangle(
  left: number,
  top: number,
  right: number,
  bottom: number
) -> featureIDs number[]?
```





Get features inside a rectangle area on the map








### Spring.GetLocalPlayerID
---
```lua
function Spring.GetLocalPlayerID() -> playerID integer
```












### Spring.GetLocalTeamID
---
```lua
function Spring.GetLocalTeamID() -> teamID integer
```












### Spring.GetLocalAllyTeamID
---
```lua
function Spring.GetLocalAllyTeamID() -> allyTeamID integer
```












### Spring.GetSpectatingState
---
```lua
function Spring.GetSpectatingState()
 -> spectating boolean
 -> spectatingFullView boolean
 -> spectatingFullSelect boolean

```












### Spring.GetSelectedUnits
---
```lua
function Spring.GetSelectedUnits() -> unitIDs number[]
```












### Spring.GetSelectedUnitsSorted
---
```lua
function Spring.GetSelectedUnitsSorted()
 -> where table<number,number[]>
 -> the integer

```

@return `where` - keys are unitDefIDs and values are unitIDs

@return `the` - number of unitDefIDs





Get selected units aggregated by unitDefID








### Spring.GetSelectedUnitsCounts
---
```lua
function Spring.GetSelectedUnitsCounts()
 -> unitsCounts table<number,number>
 -> the integer

```

@return `unitsCounts` - where keys are unitDefIDs and values are counts

@return `the` - number of unitDefIDs





Get an aggregate count of selected units per unitDefID








### Spring.GetSelectedUnitsCount
---
```lua
function Spring.GetSelectedUnitsCount() -> selectedUnitsCount number
```





Returns the amount of selected units








### Spring.GetBoxSelectionByEngine
---
```lua
function Spring.GetBoxSelectionByEngine() -> isHandledByEngine boolean
```

@return `isHandledByEngine` - `true` if the engine will select units inside selection box on release, otherwise `false`.





Get if selection box is handled by engine.





@see Spring.SetBoxSelectionByEngine




### Spring.IsGUIHidden
---
```lua
function Spring.IsGUIHidden() ->  boolean
```












### Spring.HaveShadows
---
```lua
function Spring.HaveShadows() -> shadowsLoaded boolean
```












### Spring.HaveAdvShading
---
```lua
function Spring.HaveAdvShading()
 -> useAdvShading boolean
 -> groundUseAdvShading boolean

```












### Spring.GetWaterMode
---
```lua
function Spring.GetWaterMode()
 -> waterRendererID integer
 -> waterRendererName string

```









@see rts/Rendering/Env/IWater.h




### Spring.GetMapDrawMode
---
```lua
function Spring.GetMapDrawMode() ->  ("normal"|"height"|"metal"|"pathTraversability"|"los")
```












### Spring.GetMapSquareTexture
---
```lua
function Spring.GetMapSquareTexture(
  texSquareX: number,
  texSquareY: number,
  lodMin: number,
  luaTexName: string,
  lodMax: number?
) -> success boolean?
```
@param `lodMax` - (Default: lodMin)













### Spring.GetLosViewColors
---
```lua
function Spring.GetLosViewColors()
 -> always rgb
 -> LOS rgb
 -> radar rgb
 -> jam rgb
 -> radar2 rgb

```












### Spring.GetNanoProjectileParams
---
```lua
function Spring.GetNanoProjectileParams()
 -> rotVal number
 -> rotVel number
 -> rotAcc number
 -> rotValRng number
 -> rotVelRng number
 -> rotAccRng number

```

@return `rotVal` - in degrees

@return `rotVel` - in degrees

@return `rotAcc` - in degrees

@return `rotValRng` - in degrees

@return `rotVelRng` - in degrees

@return `rotAccRng` - in degrees












### Spring.GetCameraNames
---
```lua
function Spring.GetCameraNames() -> indexByName table<string,integer>
```

@return `indexByName` - Table where where keys are names and values are indices.





Get available cameras.








### Spring.GetCameraState
---
```lua
function Spring.GetCameraState(useReturns: false) -> cameraState CameraState
```












### Spring.GetCameraState
---
```lua
function Spring.GetCameraState(useReturns: true?)
 -> name CameraName
 -> Fields any

```
@param `useReturns` - (Default: `true`) Return multiple values instead of a table.


@return `Fields` - depending on current controller mode.












### Spring.GetCameraPosition
---
```lua
function Spring.GetCameraPosition()
 -> posX number
 -> posY number
 -> posZ number

```












### Spring.GetCameraDirection
---
```lua
function Spring.GetCameraDirection()
 -> dirX number
 -> dirY number
 -> dirZ number

```












### Spring.GetCameraRotation
---
```lua
function Spring.GetCameraRotation()
 -> rotX number
 -> rotY number
 -> rotZ number

```

@return `rotX` - Rotation around X axis in radians.

@return `rotY` - Rotation around Y axis in radians.

@return `rotZ` - Rotation around Z axis in radians.





Get camera rotation in radians.








### Spring.GetCameraFOV
---
```lua
function Spring.GetCameraFOV()
 -> vFOV number
 -> hFOV number

```












### Spring.GetCameraVectors
---
```lua
function Spring.GetCameraVectors() ->  CameraVectors
```












### Spring.WorldToScreenCoords
---
```lua
function Spring.WorldToScreenCoords(
  x: number,
  y: number,
  z: number
)
 -> viewPortX number
 -> viewPortY number
 -> viewPortZ number

```












### Spring.TraceScreenRay
---
```lua
function Spring.TraceScreenRay(
  screenX: number,
  screenY: number,
  onlyCoords: boolean?,
  useMinimap: boolean?,
  includeSky: boolean?,
  ignoreWater: boolean?,
  heightOffset: number?
)
 -> description string?
 -> unitID (number|string|xyz)?
 -> featureID (number|string)?
 -> coords xyz?

```
@param `screenX` - position on x axis in mouse coordinates (origin on left border of view)

@param `screenY` - position on y axis in mouse coordinates (origin on top border of view)

@param `onlyCoords` - (Default: `false`) return only description (1st return value) and coordinates (2nd return value)

@param `useMinimap` - (Default: `false`) if position arguments are contained by minimap, use the minimap corresponding world position

@param `includeSky` - (Default: `false`)

@param `ignoreWater` - (Default: `false`)

@param `heightOffset` - (Default: `0`)


@return `description` - of traced position

@return `unitID` - or feature, position triple when onlyCoords=true

@return `featureID` - or ground





Get information about a ray traced from screen to world position

Extended to allow a custom plane, parameters are (0, 1, 0, D=0) where D is the offset D can be specified in the third argument (if all the bools are false) or in the seventh (as shown).

Intersection coordinates are returned in t[4],t[5],t[6] when the ray goes offmap and includeSky is true), or when no unit or feature is hit (or onlyCoords is true).

This will only work for units & objects with the default collision sphere. Per Piece collision and custom collision objects are not supported.

The unit must be selectable, to appear to a screen trace ray.








### Spring.GetPixelDir
---
```lua
function Spring.GetPixelDir(
  x: number,
  y: number
)
 -> dirX number
 -> dirY number
 -> dirZ number

```












### Spring.GetTeamColor
---
```lua
function Spring.GetTeamColor(teamID: integer)
 -> r number?
 -> g number?
 -> b number?
 -> a number?

```

@return `r` - factor from 0 to 1

@return `g` - factor from 0 to 1

@return `b` - factor from 0 to 1

@return `a` - factor from 0 to 1












### Spring.GetTeamOrigColor
---
```lua
function Spring.GetTeamOrigColor(teamID: integer)
 -> r number?
 -> g number?
 -> b number?
 -> a number?

```

@return `r` - factor from 0 to 1

@return `g` - factor from 0 to 1

@return `b` - factor from 0 to 1

@return `a` - factor from 0 to 1












### Spring.GetDrawSeconds
---
```lua
function Spring.GetDrawSeconds() -> time integer
```

@return `time` - Time in seconds.












### Spring.GetSoundDevices
---
```lua
function Spring.GetSoundDevices() -> devices SoundDeviceSpec[]
```

@return `devices` - Sound devices.












### Spring.GetSoundStreamTime
---
```lua
function Spring.GetSoundStreamTime()
 -> playTime number
 -> time number

```












### Spring.GetSoundEffectParams
---
```lua
function Spring.GetSoundEffectParams()
```












### Spring.GetFPS
---
```lua
function Spring.GetFPS() -> fps number
```












### Spring.GetGameSpeed
---
```lua
function Spring.GetGameSpeed()
 -> wantedSpeedFactor number
 -> speedFactor number
 -> paused boolean

```












### Spring.GetGameState
---
```lua
function Spring.GetGameState(maxLatency: number?)
 -> doneLoading boolean
 -> isSavedGame boolean
 -> isClientPaused boolean
 -> isSimLagging boolean

```
@param `maxLatency` - (Default: `500`) used for `isSimLagging` return parameter













### Spring.GetActiveCommand
---
```lua
function Spring.GetActiveCommand()
 -> cmdIndex number?
 -> cmdID integer?
 -> cmdType number?
 -> cmdName string?

```












### Spring.GetDefaultCommand
---
```lua
function Spring.GetDefaultCommand()
 -> cmdIndex integer?
 -> cmdID integer?
 -> cmdType integer?
 -> cmdName string?

```












### Spring.GetActiveCmdDescs
---
```lua
function Spring.GetActiveCmdDescs() -> cmdDescs CommandDescription[]
```












### Spring.GetActiveCmdDesc
---
```lua
function Spring.GetActiveCmdDesc(cmdIndex: integer) ->  CommandDescription?
```












### Spring.GetCmdDescIndex
---
```lua
function Spring.GetCmdDescIndex(cmdID: integer) -> cmdDescIndex integer?
```












### Spring.GetBuildFacing
---
```lua
function Spring.GetBuildFacing() -> buildFacing FacingInteger
```












### Spring.GetBuildSpacing
---
```lua
function Spring.GetBuildSpacing() -> buildSpacing number
```












### Spring.GetGatherMode
---
```lua
function Spring.GetGatherMode() -> gatherMode number
```












### Spring.GetActivePage
---
```lua
function Spring.GetActivePage()
 -> activePage number
 -> maxPage number

```












### Spring.GetMouseState
---
```lua
function Spring.GetMouseState()
 -> x number
 -> y number
 -> lmbPressed number
 -> mmbPressed number
 -> rmbPressed number
 -> offscreen boolean
 -> mmbScroll boolean

```

@return `lmbPressed` - left mouse button pressed

@return `mmbPressed` - middle mouse button pressed

@return `rmbPressed` - right mouse button pressed












### Spring.GetMouseCursor
---
```lua
function Spring.GetMouseCursor()
 -> cursorName string
 -> cursorScale number

```












### Spring.GetMouseStartPosition
---
```lua
function Spring.GetMouseStartPosition(button: number)
 -> x number
 -> y number
 -> camPosX number
 -> camPosY number
 -> camPosZ number
 -> dirX number
 -> dirY number
 -> dirZ number

```












### Spring.GetClipboard
---
```lua
function Spring.GetClipboard() -> text string
```












### Spring.IsUserWriting
---
```lua
function Spring.IsUserWriting() ->  boolean
```












### Spring.GetLastMessagePositions
---
```lua
function Spring.GetLastMessagePositions() -> message xyz[]
```

@return `message` - positions












### Spring.GetConsoleBuffer
---
```lua
function Spring.GetConsoleBuffer(maxLines: number) -> buffer { text: string,priority: integer }[]
```












### Spring.GetCurrentTooltip
---
```lua
function Spring.GetCurrentTooltip() -> tooltip string
```












### Spring.GetKeyFromScanSymbol
---
```lua
function Spring.GetKeyFromScanSymbol(scanSymbol: string) -> keyName string
```












### Spring.GetKeyState
---
```lua
function Spring.GetKeyState(keyCode: number) -> pressed boolean
```












### Spring.GetModKeyState
---
```lua
function Spring.GetModKeyState()
 -> alt boolean
 -> ctrl boolean
 -> meta boolean
 -> shift boolean

```












### Spring.GetPressedKeys
---
```lua
function Spring.GetPressedKeys() -> where table<(number|string),true>
```

@return `where` - keys are keyCodes or key names












### Spring.GetPressedScans
---
```lua
function Spring.GetPressedScans() -> where table<(number|string),true>
```

@return `where` - keys are scanCodes or scan names












### Spring.GetInvertQueueKey
---
```lua
function Spring.GetInvertQueueKey() -> queueKey number?
```












### Spring.GetKeyCode
---
```lua
function Spring.GetKeyCode(keySym: string) -> keyCode number
```












### Spring.GetKeySymbol
---
```lua
function Spring.GetKeySymbol(keyCode: number)
 -> keyCodeName string
 -> keyCodeDefaultName string

```

@return `keyCodeDefaultName` - name when there are not aliases












### Spring.GetScanSymbol
---
```lua
function Spring.GetScanSymbol(scanCode: number)
 -> scanCodeName string
 -> scanCodeDefaultName string

```

@return `scanCodeDefaultName` - name when there are not aliases












### Spring.GetKeyBindings
---
```lua
function Spring.GetKeyBindings(
  keySet1: string?,
  keySet2: string?
) ->  KeyBinding[]
```
@param `keySet1` - filters keybindings bound to this keyset

@param `keySet2` - OR bound to this keyset













### Spring.GetActionHotKeys
---
```lua
function Spring.GetActionHotKeys(actionName: string) -> hotkeys string[]?
```












### Spring.GetGroupList
---
```lua
function Spring.GetGroupList() -> where table<number,number>?
```

@return `where` - keys are groupIDs and values are counts












### Spring.GetSelectedGroup
---
```lua
function Spring.GetSelectedGroup() -> groupID integer
```












### Spring.GetUnitGroup
---
```lua
function Spring.GetUnitGroup(unitID: integer) -> groupID integer?
```












### Spring.GetGroupUnits
---
```lua
function Spring.GetGroupUnits(groupID: integer) -> unitIDs number[]?
```












### Spring.GetGroupUnitsSorted
---
```lua
function Spring.GetGroupUnitsSorted(groupID: integer) -> where table<number,number[]>?
```

@return `where` - keys are unitDefIDs and values are unitIDs












### Spring.GetGroupUnitsCounts
---
```lua
function Spring.GetGroupUnitsCounts(groupID: integer) -> where table<number,number>?
```

@return `where` - keys are unitDefIDs and values are counts












### Spring.GetGroupUnitsCount
---
```lua
function Spring.GetGroupUnitsCount(groupID: integer) -> groupSize number?
```












### Spring.GetPlayerRoster
---
```lua
function Spring.GetPlayerRoster(
  sortType: number?,
  showPathingPlayers: boolean?
) -> playerTable Roster[]?
```
@param `sortType` - return unsorted if unspecified. Disabled = 0, Allies = 1, TeamID = 2, PlayerName = 3, PlayerCPU = 4, PlayerPing = 5

@param `showPathingPlayers` - (Default: `false`)













### Spring.GetPlayerTraffic
---
```lua
function Spring.GetPlayerTraffic(
  playerID: integer,
  packetID: integer?
) -> traffic number
```












### Spring.GetPlayerStatistics
---
```lua
function Spring.GetPlayerStatistics(playerID: integer)
 -> mousePixels number?
 -> mouseClicks number
 -> keyPresses number
 -> numCommands number
 -> unitCommands number

```

@return `mousePixels` - nil when invalid playerID












### Spring.GetConfigParams
---
```lua
function Spring.GetConfigParams() ->  Configuration[]
```












### Spring.GetConfigInt
---
```lua
function Spring.GetConfigInt(
  name: string,
  default: number?
) -> configInt number?
```
@param `default` - (Default: `0`)













### Spring.GetConfigFloat
---
```lua
function Spring.GetConfigFloat(
  name: string,
  default: number?
) -> configFloat number?
```
@param `default` - (Default: `0`)













### Spring.GetConfigString
---
```lua
function Spring.GetConfigString(
  name: string,
  default: string?
) -> configString number?
```
@param `default` - (Default: `""`)













### Spring.GetLogSections
---
```lua
function Spring.GetLogSections() -> sections table<string,number>
```

@return `sections` - where keys are names and loglevel are values. E.g. `{ "KeyBindings" = LOG.INFO, "Font" = LOG.INFO, "Sound" = LOG.WARNING, ... }`












### Spring.GetAllGroundDecals
---
```lua
function Spring.GetAllGroundDecals() -> decalIDs number[]
```












### Spring.GetGroundDecalMiddlePos
---
```lua
function Spring.GetGroundDecalMiddlePos(decalID: integer)
 -> posX number?
 -> posZ number

```












### Spring.GetDecalQuadPos
---
```lua
function Spring.GetDecalQuadPos(decalID: integer)
 -> posTL.x number?
 -> posTL.z number
 -> posTR.x number
 -> posTR.z number
 -> posBR.x number
 -> posBR.z number
 -> posBL.x number
 -> posBL.z number

```












### Spring.GetGroundDecalSizeAndHeight
---
```lua
function Spring.GetGroundDecalSizeAndHeight(decalID: integer)
 -> sizeX number?
 -> sizeY number
 -> projCubeHeight number

```












### Spring.GetGroundDecalRotation
---
```lua
function Spring.GetGroundDecalRotation(decalID: integer) -> rotation number?
```

@return `rotation` - Rotation in radians.












### Spring.GetGroundDecalTexture
---
```lua
function Spring.GetGroundDecalTexture(
  decalID: integer,
  isMainTex: boolean?
) -> texture string?
```
@param `isMainTex` - (Default: `true`) If `false`, return the normal/glow map.













### Spring.GetDecalTextures
---
```lua
function Spring.GetDecalTextures(isMainTex: boolean?) -> textureNames string[]
```
@param `isMainTex` - (Default: `true`) If `false`, return the texture for normal/glow maps.


@return `textureNames` - All textures on the atlas and available for use in `SetGroundDecalTexture`.









@see Spring.GetGroundDecalTexture




### Spring.SetGroundDecalTextureParams
---
```lua
function Spring.SetGroundDecalTextureParams(decalID: integer)
 -> texWrapDistance number?
 -> texTraveledDistance number

```

@return `texWrapDistance` - If non-zero, sets the mode to repeat the texture along the left-right direction of the decal every texWrapFactor elmos.

@return `texTraveledDistance` - Shifts the texture repetition defined by texWrapFactor so the texture of a next line in the continuous multiline can start where the previous finished. For that it should collect all elmo lengths of the previously set multiline segments.












### Spring.GetGroundDecalAlpha
---
```lua
function Spring.GetGroundDecalAlpha(decalID: integer)
 -> alpha number?
 -> alphaFalloff number

```

@return `alpha` - Between 0 and 1

@return `alphaFalloff` - Between 0 and 1, per second












### Spring.GetGroundDecalNormal
---
```lua
function Spring.GetGroundDecalNormal(decalID: integer)
 -> normal.x number?
 -> normal.y number
 -> normal.z number

```





If all three equal 0, the decal follows the normals of ground at midpoint








### Spring.GetGroundDecalTint
---
```lua
function Spring.GetGroundDecalTint(decalID: integer)
 -> tintR number?
 -> tintG number
 -> tintB number
 -> tintA number

```





Gets the tint of the ground decal.
A color of (0.5, 0.5, 0.5, 0.5) is effectively no tint








### Spring.GetGroundDecalMisc
---
```lua
function Spring.GetGroundDecalMisc(decalID: integer)
 -> dotElimExp number?
 -> refHeight number
 -> minHeight number
 -> maxHeight number
 -> forceHeightMode number

```





Returns less important parameters of a ground decal








### Spring.GetGroundDecalCreationFrame
---
```lua
function Spring.GetGroundDecalCreationFrame(decalID: integer)
 -> creationFrameMin number?
 -> creationFrameMax number

```





Min can be not equal to max for "gradient" style decals, e.g. unit tracks








### Spring.GetGroundDecalOwner
---
```lua
function Spring.GetGroundDecalOwner(decalID: integer) -> value integer?
```

@return `value` - If owner is a unit, then this is `unitID`, if owner is
a feature it is `featureID + MAX_UNITS`. If there is no owner, then `nil`.












### Spring.GetGroundDecalType
---
```lua
function Spring.GetGroundDecalType(decalID: integer) -> type string?
```

@return `type` - "explosion"|"plate"|"lua"|"track"|"unknown"












### Spring.GetSyncedGCInfo
---
```lua
function Spring.GetSyncedGCInfo(collectGC: boolean?) -> GC number?
```
@param `collectGC` - (Default: `false`) collect before returning metric


@return `GC` - values are expressed in Kbytes: #bytes/2^10












### Spring.SolveNURBSCurve
---
```lua
function Spring.SolveNURBSCurve(groupID: integer) -> unitIDs number[]?
```












### Spring.Ping
---
```lua
function Spring.Ping(pingTag: number) ->  nil
```





Send a ping request to the server








### Spring.SendCommands
---
```lua
function Spring.SendCommands(commands: string[])
```












### Spring.SendCommands
---
```lua
function Spring.SendCommands(
  command: string,
  ...: string
) ->  nil
```
@param `...` - additional commands













### Spring.SendMessage
---
```lua
function Spring.SendMessage(message: string) ->  nil
```












### Spring.SendMessageToSpectators
---
```lua
function Spring.SendMessageToSpectators(message: string) ->  nil
```
@param `message` - ``"`<PLAYER#>`"`` where `#` is a player ID.

This will be replaced with the player's name. e.g.
```lua
Spring.SendMessage("`<PLAYER1>` did something") -- "ProRusher did something"
```













### Spring.SendMessageToPlayer
---
```lua
function Spring.SendMessageToPlayer(
  playerID: integer,
  message: string
) ->  nil
```












### Spring.SendMessageToTeam
---
```lua
function Spring.SendMessageToTeam(
  teamID: integer,
  message: string
) ->  nil
```












### Spring.SendMessageToAllyTeam
---
```lua
function Spring.SendMessageToAllyTeam(
  allyID: integer,
  message: string
) ->  nil
```












### Spring.LoadSoundDef
---
```lua
function Spring.LoadSoundDef(soundfile: string) -> success boolean
```





Loads a SoundDefs file, the format is the same as in `gamedata/sounds.lua`.








### Spring.PlaySoundFile
---
```lua
function Spring.PlaySoundFile(
  soundfile: string,
  volume: number?,
  posx: number?,
  posy: number?,
  posz: number?,
  speedx: number?,
  speedy: number?,
  speedz: number?,
  channel: SoundChannel?
) -> playSound boolean
```
@param `volume` - (Default: 1.0)

@param `channel` - (Default: `0|"general"`)













### Spring.PlaySoundStream
---
```lua
function Spring.PlaySoundStream(
  oggfile: string,
  volume: number?,
  enqueue: boolean?
) -> success boolean
```
@param `volume` - (Default: 1.0)






Allows to play an Ogg Vorbis (.OGG) and mp3 compressed sound file.

Multiple sound streams may be played at once.








### Spring.StopSoundStream
---
```lua
function Spring.StopSoundStream() ->  nil
```





Terminates any SoundStream currently running.








### Spring.PauseSoundStream
---
```lua
function Spring.PauseSoundStream() ->  nil
```





Pause any SoundStream currently running.








### Spring.SetSoundStreamVolume
---
```lua
function Spring.SetSoundStreamVolume(volume: number) ->  nil
```





Set volume for SoundStream








### Spring.SetSoundEffectParams
---
```lua
function Spring.SetSoundEffectParams()
```












### Spring.AddWorldIcon
---
```lua
function Spring.AddWorldIcon(
  cmdID: integer,
  posX: number,
  posY: number,
  posZ: number
) ->  nil
```












### Spring.AddWorldText
---
```lua
function Spring.AddWorldText(
  text: string,
  posX: number,
  posY: number,
  posZ: number
) ->  nil
```












### Spring.AddWorldUnit
---
```lua
function Spring.AddWorldUnit(
  unitDefID: integer,
  posX: number,
  posY: number,
  posZ: number,
  teamID: integer,
  facing: FacingInteger
) ->  nil
```












### Spring.DrawUnitCommands
---
```lua
function Spring.DrawUnitCommands(unitID: integer)
```












### Spring.DrawUnitCommands
---
```lua
function Spring.DrawUnitCommands(
  unitIDs: integer[],
  tableOrArray: false?
)
```
@param `unitIDs` - Unit ids.

@param `tableOrArray` - Set to `true` if the unit IDs should be read from the keys of `unitIDs`.













### Spring.DrawUnitCommands
---
```lua
function Spring.DrawUnitCommands(
  unitIDs: table<integer,any>,
  tableOrArray: true
) ->  nil
```
@param `unitIDs` - Table with unit IDs as keys.

@param `tableOrArray` - Set to `false` if the unit IDs should be read from the values of `unitIDs`.













### Spring.SetCameraTarget
---
```lua
function Spring.SetCameraTarget(
  x: number,
  y: number,
  z: number,
  transTime: number?
) ->  nil
```





For Spring Engine XZ represents horizontal, from north west corner of map and Y vertical, from water level and rising.








### Spring.SetCameraOffset
---
```lua
function Spring.SetCameraOffset(
  posX: number?,
  posY: number?,
  posZ: number?,
  tiltX: number?,
  tiltY: number?,
  tiltZ: number?
) ->  nil
```
@param `posX` - (Default: `0`)

@param `posY` - (Default: `0`)

@param `posZ` - (Default: `0`)

@param `tiltX` - (Default: `0`)

@param `tiltY` - (Default: `0`)

@param `tiltZ` - (Default: `0`)













### Spring.SetCameraState
---
```lua
function Spring.SetCameraState(
  cameraState: CameraState,
  transitionTime: number?,
  transitionTimeFactor: number?,
  transitionTimeExponent: number?
) -> set boolean
```
@param `cameraState` - The fields must be consistent with the name/mode and current/new camera mode.

@param `transitionTime` - (Default: `0`) in nanoseconds

@param `transitionTimeFactor` - Multiplicative factor applied to this and all subsequent transition times for
this camera mode.

Defaults to "CamTimeFactor" springsetting unless set previously.

@param `transitionTimeExponent` - Tween factor applied to this and all subsequent transitions for this camera
mode.

Defaults to "CamTimeExponent" springsetting unless set previously.


@return `set` - `true` when applied without errors, otherwise `false`.





Set camera state.








### Spring.RunDollyCamera
---
```lua
function Spring.RunDollyCamera(runtime: number) ->  nil
```
@param `runtime` - Runtime in milliseconds.






Runs Dolly Camera








### Spring.PauseDollyCamera
---
```lua
function Spring.PauseDollyCamera(fraction: number) ->  nil
```
@param `fraction` - Fraction of the total runtime to pause at, 0 to 1 inclusive. A null value pauses at current percent






Pause Dolly Camera








### Spring.ResumeDollyCamera
---
```lua
function Spring.ResumeDollyCamera() ->  nil
```





Resume Dolly Camera








### Spring.SetDollyCameraPosition
---
```lua
function Spring.SetDollyCameraPosition(
  x: number,
  y: number,
  z: number
) ->  nil
```





Sets Dolly Camera Position








### Spring.SetDollyCameraCurve
---
```lua
function Spring.SetDollyCameraCurve(
  degree: number,
  cpoints: ControlPoint[],
  knots: table
) ->  nil
```
@param `cpoints` - NURBS control point positions.






Sets Dolly Camera movement Curve








### Spring.SetDollyCameraMode
---
```lua
function Spring.SetDollyCameraMode(mode: (1|2)) ->  nil
```
@param `mode` - `1` static position, `2` nurbs curve






Sets Dolly Camera movement mode








### Spring.SetDollyCameraRelativeMode
---
```lua
function Spring.SetDollyCameraRelativeMode(relativeMode: number) ->  nil
```
@param `relativeMode` - `1` world, `2` look target






Sets Dolly Camera movement curve to world relative or look target relative








### Spring.SetDollyCameraLookCurve
---
```lua
function Spring.SetDollyCameraLookCurve(
  degree: number,
  cpoints: ControlPoint[],
  knots: table
) ->  nil
```
@param `cpoints` - NURBS control point positions.






Sets Dolly Camera Look Curve








### Spring.SetDollyCameraLookPosition
---
```lua
function Spring.SetDollyCameraLookPosition(
  x: number,
  y: number,
  z: number
) ->  nil
```





Sets Dolly Camera Look Position








### Spring.SetDollyCameraLookUnit
---
```lua
function Spring.SetDollyCameraLookUnit(unitID: integer) ->  nil
```
@param `unitID` - The unit to look at.






Sets target unit for Dolly Camera to look towards








### Spring.SelectUnit
---
```lua
function Spring.SelectUnit(
  unitID: integer?,
  append: boolean?
) ->  nil
```
@param `append` - (Default: `false`) Append to current selection.






Selects a single unit








### Spring.DeselectUnit
---
```lua
function Spring.DeselectUnit(unitID: integer) ->  nil
```












### Spring.DeselectUnitArray
---
```lua
function Spring.DeselectUnitArray(unitIDs: integer[]) ->  nil
```
@param `unitIDs` - Table with unit IDs as values.






Deselects multiple units.








### Spring.DeselectUnitMap
---
```lua
function Spring.DeselectUnitMap(unitMap: table<integer,any>) ->  nil
```
@param `unitMap` - Table with unit IDs as keys.






Deselects multiple units.








### Spring.SelectUnitArray
---
```lua
function Spring.SelectUnitArray(
  unitIDs: integer[],
  append: boolean?
) ->  nil
```
@param `unitIDs` - Table with unit IDs as values.

@param `append` - (Default: `false`) append to current selection






Selects multiple units, or appends to selection. Accepts a table with unitIDs as values








### Spring.SelectUnitMap
---
```lua
function Spring.SelectUnitMap(
  unitMap: table<integer,any>,
  append: boolean?
) ->  nil
```
@param `unitMap` - Table with unit IDs as keys.

@param `append` - (Default: `false`) append to current selection






Selects multiple units, or appends to selection. Accepts a table with unitIDs as keys








### Spring.AddMapLight
---
```lua
function Spring.AddMapLight(lightParams: LightParams) -> lightHandle integer
```





requires MaxDynamicMapLights > 0








### Spring.AddModelLight
---
```lua
function Spring.AddModelLight(lightParams: LightParams) -> lightHandle number
```





requires MaxDynamicMapLights > 0








### Spring.UpdateMapLight
---
```lua
function Spring.UpdateMapLight(
  lightHandle: number,
  lightParams: LightParams
) -> success boolean
```












### Spring.UpdateModelLight
---
```lua
function Spring.UpdateModelLight(
  lightHandle: number,
  lightParams: LightParams
) -> success boolean
```












### Spring.AddLightTrackingTarget
---
```lua
function Spring.AddLightTrackingTarget()
```












### Spring.SetMapLightTrackingState
---
```lua
function Spring.SetMapLightTrackingState(
  lightHandle: number,
  unitOrProjectileID: integer,
  enableTracking: boolean,
  unitOrProjectile: boolean
) -> success boolean
```





Set a map-illuminating light to start/stop tracking the position of a moving object (unit or projectile)








### Spring.SetModelLightTrackingState
---
```lua
function Spring.SetModelLightTrackingState(
  lightHandle: number,
  unitOrProjectileID: integer,
  enableTracking: boolean,
  unitOrProjectile: boolean
) -> success boolean
```





Set a model-illuminating light to start/stop tracking the position of a moving object (unit or projectile)








### Spring.SetMapShader
---
```lua
function Spring.SetMapShader(
  standardShaderID: integer,
  deferredShaderID: integer
) ->  nil
```





The ID's must refer to valid programs returned by `gl.CreateShader`.
Passing in a value of 0 will cause the respective shader to revert back to its engine default.
Custom map shaders that declare a uniform ivec2 named "texSquare" can sample from the default diffuse texture(s), which are always bound to TU 0.








### Spring.SetMapSquareTexture
---
```lua
function Spring.SetMapSquareTexture(
  texSqrX: number,
  texSqrY: number,
  luaTexName: string
) -> success boolean
```












### Spring.SetMapShadingTexture
---
```lua
function Spring.SetMapShadingTexture(
  texType: string,
  texName: string
) -> success boolean
```











@*usage* Spring.SetMapShadingTexture("$ssmf_specular", "name_of_my_shiny_texture")


### Spring.SetSkyBoxTexture
---
```lua
function Spring.SetSkyBoxTexture(texName: string) ->  nil
```












### Spring.SetUnitNoDraw
---
```lua
function Spring.SetUnitNoDraw(
  unitID: integer,
  noDraw: boolean
) ->  nil
```












### Spring.SetUnitEngineDrawMask
---
```lua
function Spring.SetUnitEngineDrawMask(
  unitID: integer,
  drawMask: number
) ->  nil
```












### Spring.SetUnitAlwaysUpdateMatrix
---
```lua
function Spring.SetUnitAlwaysUpdateMatrix(
  unitID: integer,
  alwaysUpdateMatrix: boolean
) ->  nil
```












### Spring.SetUnitNoMinimap
---
```lua
function Spring.SetUnitNoMinimap(
  unitID: integer,
  unitNoMinimap: boolean
) ->  nil
```












### Spring.SetMiniMapRotation
---
```lua
function Spring.SetMiniMapRotation(rotation: number) ->  nil
```
@param `rotation` - amount in radians













### Spring.SetUnitNoGroup
---
```lua
function Spring.SetUnitNoGroup(
  unitID: integer,
  unitNoGroup: boolean
)
```
@param `unitNoGroup` - Whether unit can be added to selection groups













### Spring.SetUnitNoSelect
---
```lua
function Spring.SetUnitNoSelect(
  unitID: integer,
  unitNoSelect: boolean
) ->  nil
```
@param `unitNoSelect` - whether unit can be selected or not













### Spring.SetUnitLeaveTracks
---
```lua
function Spring.SetUnitLeaveTracks(
  unitID: integer,
  unitLeaveTracks: boolean
) ->  nil
```
@param `unitLeaveTracks` - whether unit leaves tracks on movement













### Spring.SetUnitSelectionVolumeData
---
```lua
function Spring.SetUnitSelectionVolumeData(
  unitID: integer,
  featureID: integer,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  vType: number,
  tType: number,
  Axis: number
) ->  nil
```












### Spring.SetFeatureNoDraw
---
```lua
function Spring.SetFeatureNoDraw(
  featureID: integer,
  noDraw: boolean
) ->  nil
```












### Spring.SetFeatureEngineDrawMask
---
```lua
function Spring.SetFeatureEngineDrawMask(
  featureID: integer,
  engineDrawMask: number
) ->  nil
```












### Spring.SetFeatureAlwaysUpdateMatrix
---
```lua
function Spring.SetFeatureAlwaysUpdateMatrix(
  featureID: integer,
  alwaysUpdateMat: number
) ->  nil
```












### Spring.SetFeatureFade
---
```lua
function Spring.SetFeatureFade(
  featureID: integer,
  allow: boolean
) ->  nil
```





Control whether a feature will fade or not when zoomed out.








### Spring.SetFeatureSelectionVolumeData
---
```lua
function Spring.SetFeatureSelectionVolumeData(
  featureID: integer,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  vType: number,
  tType: number,
  Axis: number
) ->  nil
```












### Spring.AddUnitIcon
---
```lua
function Spring.AddUnitIcon(
  iconName: string,
  texFile: string,
  size: number?,
  dist: number?,
  radAdjust: number?
) -> added boolean
```












### Spring.FreeUnitIcon
---
```lua
function Spring.FreeUnitIcon(iconName: string) -> freed boolean?
```












### Spring.UnitIconSetDraw
---
```lua
function Spring.UnitIconSetDraw(
  unitID: integer,
  drawIcon: boolean
) ->  nil
```





Use Spring.SetUnitIconDraw instead.








### Spring.SetUnitIconDraw
---
```lua
function Spring.SetUnitIconDraw(
  unitID: integer,
  drawIcon: boolean
) ->  nil
```












### Spring.SetUnitDefIcon
---
```lua
function Spring.SetUnitDefIcon(
  unitDefID: integer,
  iconName: string
) ->  nil
```












### Spring.SetUnitDefImage
---
```lua
function Spring.SetUnitDefImage(
  unitDefID: integer,
  image: string
) ->  nil
```
@param `image` - luaTexture|texFile













### Spring.ExtractModArchiveFile
---
```lua
function Spring.ExtractModArchiveFile(modfile: string) -> extracted boolean
```












### Spring.CreateDir
---
```lua
function Spring.CreateDir(path: string) -> dirCreated boolean?
```












### Spring.SetActiveCommand
---
```lua
function Spring.SetActiveCommand(
  action: string,
  actionExtra: string?
) -> commandSet boolean?
```












### Spring.SetActiveCommand
---
```lua
function Spring.SetActiveCommand(
  cmdIndex: number,
  button: number?,
  leftClick: boolean?,
  rightClick: boolean?,
  alt: boolean?,
  ctrl: boolean?,
  meta: boolean?,
  shift: boolean?
) -> commandSet boolean?
```
@param `button` - (Default: `1`)













### Spring.LoadCmdColorsConfig
---
```lua
function Spring.LoadCmdColorsConfig(config: string) ->  nil
```












### Spring.LoadCtrlPanelConfig
---
```lua
function Spring.LoadCtrlPanelConfig(config: string) ->  nil
```












### Spring.ForceLayoutUpdate
---
```lua
function Spring.ForceLayoutUpdate() ->  nil
```












### Spring.SetDrawSelectionInfo
---
```lua
function Spring.SetDrawSelectionInfo(enable: boolean) ->  nil
```





Disables the "Selected Units x" box in the GUI.








### Spring.SetBoxSelectionByEngine
---
```lua
function Spring.SetBoxSelectionByEngine(state: boolean) ->  nil
```












### Spring.SetTeamColor
---
```lua
function Spring.SetTeamColor(
  teamID: integer,
  r: number,
  g: number,
  b: number
) ->  nil
```












### Spring.AssignMouseCursor
---
```lua
function Spring.AssignMouseCursor(
  cmdName: string,
  iconFileName: string,
  overwrite: boolean?,
  hotSpotTopLeft: boolean?
) -> assigned boolean?
```
@param `iconFileName` - not the full filename, instead it is like this:
Wanted filename: Anims/cursorattack_0.bmp
=> iconFileName: cursorattack

@param `overwrite` - (Default: `true`)

@param `hotSpotTopLeft` - (Default: `false`)






Changes/creates the cursor of a single CursorCmd.








### Spring.ReplaceMouseCursor
---
```lua
function Spring.ReplaceMouseCursor(
  oldFileName: string,
  newFileName: string,
  hotSpotTopLeft: boolean?
) -> assigned boolean?
```
@param `hotSpotTopLeft` - (Default: `false`)






Mass replace all occurrences of the cursor in all CursorCmds.








### Spring.SetCustomCommandDrawData
---
```lua
function Spring.SetCustomCommandDrawData(
  cmdID: integer,
  cmdReference: (string|integer)?,
  color: rgba?,
  showArea: boolean?
) ->  nil
```
@param `cmdReference` - The name or ID of an icon for command. Pass `nil` to clear draw data for command.

@param `color` - (Default: white)

@param `showArea` - (Default: `false`)






Register your custom cmd so it gets visible in the unit's cmd queue








### Spring.WarpMouse
---
```lua
function Spring.WarpMouse(
  x: number,
  y: number
) ->  nil
```












### Spring.SetMouseCursor
---
```lua
function Spring.SetMouseCursor(
  cursorName: string,
  cursorScale: number?
) ->  nil
```
@param `cursorScale` - (Default: `1.0`)













### Spring.SetLosViewColors
---
```lua
function Spring.SetLosViewColors(
  always: rgb,
  LOS: rgb,
  radar: rgb,
  jam: rgb,
  radar2: rgb
) ->  nil
```












### Spring.SetNanoProjectileParams
---
```lua
function Spring.SetNanoProjectileParams(
  rotVal: number?,
  rotVel: number?,
  rotAcc: number?,
  rotValRng: number?,
  rotVelRng: number?,
  rotAccRng: number?
) ->  nil
```
@param `rotVal` - (Default: `0`) in degrees

@param `rotVel` - (Default: `0`) in degrees

@param `rotAcc` - (Default: `0`) in degrees

@param `rotValRng` - (Default: `0`) in degrees

@param `rotVelRng` - (Default: `0`) in degrees

@param `rotAccRng` - (Default: `0`) in degrees













### Spring.SetConfigInt
---
```lua
function Spring.SetConfigInt(
  name: string,
  value: integer,
  useOverlay: boolean?
) ->  nil
```
@param `useOverlay` - (Default: `false`) If `true`, the value will only be set in memory, and not be restored for the next game.













### Spring.SetConfigFloat
---
```lua
function Spring.SetConfigFloat(
  name: string,
  value: number,
  useOverla: boolean?
) ->  nil
```
@param `useOverla` - (Default: `false`) If `true`, the value will only be set in memory, and not be restored for the next game.y













### Spring.SetConfigString
---
```lua
function Spring.SetConfigString(
  name: string,
  value: string,
  useOverlay: boolean?
) ->  nil
```
@param `useOverlay` - (Default: `false`) If `true`, the value will only be set in memory, and not be restored for the next game.













### Spring.Quit
---
```lua
function Spring.Quit() ->  nil
```





Closes the application








### Spring.SetUnitGroup
---
```lua
function Spring.SetUnitGroup(
  unitID: integer,
  groupID: integer
) ->  nil
```
@param `groupID` - the group number to be assigned, or -1 for deassignment













### Spring.GiveOrder
---
```lua
function Spring.GiveOrder(
  cmdID: (CMD|integer),
  params: CreateCommandParams,
  options: CreateCommandOptions?,
  timeout: integer?
) ->  boolean
```
@param `cmdID` - The command ID.

@param `params` - Parameters for the given command.






Give order to selected units.








### Spring.GiveOrderToUnit
---
```lua
function Spring.GiveOrderToUnit(
  unitID: integer,
  cmdID: (CMD|integer),
  params: CreateCommandParams?,
  options: CreateCommandOptions?,
  timeout: integer?
) ->  boolean
```
@param `cmdID` - The command ID.

@param `params` - Parameters for the given command.






Give order to specific unit.








### Spring.GiveOrderToUnitMap
---
```lua
function Spring.GiveOrderToUnitMap(
  unitMap: table<integer,any>,
  cmdID: (CMD|integer),
  params: CreateCommandParams?,
  options: CreateCommandOptions?,
  timeout: integer?
) -> orderGiven boolean
```
@param `unitMap` - A table with unit IDs as keys.

@param `cmdID` - The command ID.

@param `params` - Parameters for the given command.






Give order to multiple units, specified by table keys.








### Spring.GiveOrderToUnitArray
---
```lua
function Spring.GiveOrderToUnitArray(
  unitIDs: integer[],
  cmdID: (CMD|integer),
  params: CreateCommandParams?,
  options: CreateCommandOptions?,
  timeout: integer?
) -> ordersGiven boolean
```
@param `unitIDs` - Array of unit IDs.

@param `cmdID` - The command ID.

@param `params` - Parameters for the given command.


@return `ordersGiven` - `true` if any orders were sent, otherwise `false`.





Give order to an array of units.








### Spring.GiveOrderArrayToUnit
---
```lua
function Spring.GiveOrderArrayToUnit(
  unitID: integer,
  commands: CreateCommand[]
) -> ordersGiven boolean
```
@param `unitID` - Unit ID.


@return `ordersGiven` - `true` if any orders were sent, otherwise `false`.












### Spring.GiveOrderArrayToUnitMap
---
```lua
function Spring.GiveOrderArrayToUnitMap(
  unitMap: table<integer,any>,
  commands: CreateCommand[]
) -> ordersGiven boolean
```
@param `unitMap` - A table with unit IDs as keys.


@return `ordersGiven` - `true` if any orders were sent, otherwise `false`.












### Spring.GiveOrderArrayToUnitArray
---
```lua
function Spring.GiveOrderArrayToUnitArray(
  unitIDs: integer[],
  commands: CreateCommand[],
  pairwise: boolean?
) -> ordersGiven boolean
```
@param `unitIDs` - Array of unit IDs.

@param `pairwise` - (Default: `false`) When `false`, assign all commands to each unit.

When `true`, assign commands according to index between units and cmds arrays.

If `len(unitArray) < len(cmdArray)` only the first `len(unitArray)` commands
will be assigned, and vice-versa.


@return `ordersGiven` - `true` if any orders were sent, otherwise `false`.












### Spring.SetBuildSpacing
---
```lua
function Spring.SetBuildSpacing(spacing: number) ->  nil
```












### Spring.SetBuildFacing
---
```lua
function Spring.SetBuildFacing(facing: FacingInteger) ->  nil
```












### Spring.SendLuaUIMsg
---
```lua
function Spring.SendLuaUIMsg(
  message: string,
  mode: string
) ->  nil
```
@param `mode` - "s"/"specs" | "a"/"allies"













### Spring.SendLuaGaiaMsg
---
```lua
function Spring.SendLuaGaiaMsg(message: string) ->  nil
```












### Spring.SendLuaRulesMsg
---
```lua
function Spring.SendLuaRulesMsg(message: string) ->  nil
```












### Spring.SendLuaMenuMsg
---
```lua
function Spring.SendLuaMenuMsg(msg: string)
```












### Spring.SetShareLevel
---
```lua
function Spring.SetShareLevel(
  resource: string,
  shareLevel: number
) ->  nil
```
@param `resource` - metal | energy













### Spring.ShareResources
---
```lua
function Spring.ShareResources(
  teamID: integer,
  units: string
) ->  nil
```












### Spring.ShareResources
---
```lua
function Spring.ShareResources(
  teamID: integer,
  resource: string,
  amount: number
) ->  nil
```
@param `resource` - metal | energy













### Spring.SetLastMessagePosition
---
```lua
function Spring.SetLastMessagePosition(
  x: number,
  y: number,
  z: number
) ->  nil
```












### Spring.MarkerAddPoint
---
```lua
function Spring.MarkerAddPoint(
  x: number,
  y: number,
  z: number,
  text: string?,
  localOnly: boolean?
) ->  nil
```
@param `text` - (Default: `""`)













### Spring.MarkerAddLine
---
```lua
function Spring.MarkerAddLine(
  x1: number,
  y1: number,
  z1: number,
  x2: number,
  y2: number,
  z2: number,
  localOnly: boolean?,
  playerId: number?
) ->  nil
```
@param `localOnly` - (Default: `false`)













### Spring.MarkerErasePosition
---
```lua
function Spring.MarkerErasePosition(
  x: number,
  y: number,
  z: number,
  unused: nil,
  localOnly: boolean?,
  playerId: number?,
  alwaysErase: boolean?
) ->  nil
```
@param `unused` - This argument is ignored.

@param `localOnly` - (Default: `false`) do not issue a network message, erase only for the current player

@param `playerId` - when not specified it uses the issuer playerId

@param `alwaysErase` - (Default: `false`) erase any marker when `localOnly` and current player is spectating. Allows spectators to erase players markers locally






Issue an erase command for markers on the map.








### Spring.SetAtmosphere
---
```lua
function Spring.SetAtmosphere(params: AtmosphereParams)
```





It can be used to modify the following atmosphere parameters

Usage:
```lua
Spring.SetAtmosphere({ fogStart = 0, fogEnd = 0.5, fogColor = { 0.7, 0.2, 0.2, 1 }})
```








### Spring.SetSunDirection
---
```lua
function Spring.SetSunDirection(
  dirX: number,
  dirY: number,
  dirZ: number,
  intensity: number?
) ->  nil
```
@param `intensity` - (Default: `1.0`)













### Spring.SetSunLighting
---
```lua
function Spring.SetSunLighting(params: { groundAmbientColor: rgb,groundDiffuseColor: rgb })
```





Modify sun lighting parameters.

```lua
Spring.SetSunLighting({groundAmbientColor = {1, 0.1, 1}, groundDiffuseColor = {1, 0.1, 1} })
```








### Spring.SetMapRenderingParams
---
```lua
function Spring.SetMapRenderingParams(params: MapRenderingParams) ->  nil
```





Allows to change map rendering params at runtime.








### Spring.ForceTesselationUpdate
---
```lua
function Spring.ForceTesselationUpdate(
  normal: boolean?,
  shadow: boolean?
) -> updated boolean
```
@param `normal` - (Default: `true`)

@param `shadow` - (Default: `false`)













### Spring.SendSkirmishAIMessage
---
```lua
function Spring.SendSkirmishAIMessage(
  aiTeam: number,
  message: string
) -> ai_processed boolean?
```












### Spring.SetLogSectionFilterLevel
---
```lua
function Spring.SetLogSectionFilterLevel(
  sectionName: string,
  logLevel: (string|number)?
) ->  nil
```












### Spring.GarbageCollectCtrl
---
```lua
function Spring.GarbageCollectCtrl(
  itersPerBatch: integer?,
  numStepsPerIter: integer?,
  minStepsPerIter: integer?,
  maxStepsPerIter: integer?,
  minLoopRunTime: number?,
  maxLoopRunTime: number?,
  baseRunTimeMult: number?,
  baseMemLoadMult: number?
) ->  nil
```












### Spring.SetAutoShowMetal
---
```lua
function Spring.SetAutoShowMetal(autoShow: boolean) ->  nil
```












### Spring.SetDrawSky
---
```lua
function Spring.SetDrawSky(drawSky: boolean) ->  nil
```












### Spring.SetDrawWater
---
```lua
function Spring.SetDrawWater(drawWater: boolean) ->  nil
```












### Spring.SetDrawGround
---
```lua
function Spring.SetDrawGround(drawGround: boolean) ->  nil
```












### Spring.SetDrawGroundDeferred
---
```lua
function Spring.SetDrawGroundDeferred(
  drawGroundDeferred: boolean,
  drawGroundForward: boolean?
) ->  nil
```
@param `drawGroundForward` - allows disabling of the forward pass













### Spring.SetDrawModelsDeferred
---
```lua
function Spring.SetDrawModelsDeferred(
  drawUnitsDeferred: boolean,
  drawFeaturesDeferred: boolean,
  drawUnitsForward: boolean?,
  drawFeaturesForward: boolean?
) ->  nil
```
@param `drawUnitsForward` - allows disabling of the respective forward passes

@param `drawFeaturesForward` - allows disabling of the respective forward passes













### Spring.SetVideoCapturingMode
---
```lua
function Spring.SetVideoCapturingMode(allowCaptureMode: boolean) ->  nil
```





This doesn't actually record the game in any way, it just regulates the framerate and interpolations.








### Spring.SetVideoCapturingTimeOffset
---
```lua
function Spring.SetVideoCapturingTimeOffset(timeOffset: boolean) ->  nil
```












### Spring.SetWaterParams
---
```lua
function Spring.SetWaterParams(waterParams: WaterParams) ->  nil
```





Does not need cheating enabled.

Allows to change water params (mostly `BumpWater` ones) at runtime. You may
want to set `BumpWaterUseUniforms` in your `springrc` to 1, then you don't even
need to restart `BumpWater` via `/water 4`.








### Spring.PreloadUnitDefModel
---
```lua
function Spring.PreloadUnitDefModel(unitDefID: integer) ->  nil
```





Allow the engine to load the unit's model (and texture) in a background thread.
Wreckages and buildOptions of a unit are automatically preloaded.








### Spring.PreloadFeatureDefModel
---
```lua
function Spring.PreloadFeatureDefModel(featureDefID: integer) ->  nil
```












### Spring.PreloadSoundItem
---
```lua
function Spring.PreloadSoundItem(name: string) ->  nil
```












### Spring.LoadModelTextures
---
```lua
function Spring.LoadModelTextures(modelName: string) -> success boolean?
```












### Spring.CreateGroundDecal
---
```lua
function Spring.CreateGroundDecal() -> decalID number?
```












### Spring.DestroyGroundDecal
---
```lua
function Spring.DestroyGroundDecal(decalID: integer) -> delSuccess boolean
```












### Spring.SetGroundDecalPosAndDims
---
```lua
function Spring.SetGroundDecalPosAndDims(
  decalID: integer,
  midPosX: number?,
  midPosZ: number?,
  sizeX: number?,
  sizeZ: number?,
  projCubeHeight: number?
) -> decalSet boolean
```
@param `midPosX` - (Default: currMidPosX)

@param `midPosZ` - (Default: currMidPosZ)

@param `sizeX` - (Default: currSizeX)

@param `sizeZ` - (Default: currSizeZ)

@param `projCubeHeight` - (Default: calculateProjCubeHeight)













### Spring.SetGroundDecalQuadPosAndHeight
---
```lua
function Spring.SetGroundDecalQuadPosAndHeight(
  decalID: integer,
  posTL: xz?,
  posTR: xz?,
  posBR: xz?,
  posBL: xz?,
  projCubeHeight: number?
) -> decalSet boolean
```
@param `posTL` - (Default: currPosTL)

@param `posTR` - (Default: currPosTR)

@param `posBR` - (Default: currPosBR)

@param `posBL` - (Default: currPosBL)

@param `projCubeHeight` - (Default: calculateProjCubeHeight)






Use for non-rectangular decals








### Spring.SetGroundDecalRotation
---
```lua
function Spring.SetGroundDecalRotation(
  decalID: integer,
  rot: number?
) -> decalSet boolean
```
@param `rot` - (Default: random) in radians













### Spring.SetGroundDecalTexture
---
```lua
function Spring.SetGroundDecalTexture(
  decalID: integer,
  textureName: string,
  isMainTex: boolean?
) -> decalSet boolean?
```
@param `textureName` - The texture has to be on the atlas which seems to mean it's defined as an explosion, unit tracks, or building plate decal on some unit already (no arbitrary textures)

@param `isMainTex` - (Default: `true`) If false, it sets the normals/glow map













### Spring.SetGroundDecalTextureParams
---
```lua
function Spring.SetGroundDecalTextureParams(
  decalID: integer,
  texWrapDistance: number?,
  texTraveledDistance: number?
) -> decalSet boolean?
```
@param `texWrapDistance` - (Default: currTexWrapDistance) if non-zero sets the mode to repeat the texture along the left-right direction of the decal every texWrapFactor elmos

@param `texTraveledDistance` - (Default: currTexTraveledDistance) shifts the texture repetition defined by texWrapFactor so the texture of a next line in the continuous multiline can start where the previous finished. For that it should collect all elmo lengths of the previously set multiline segments.













### Spring.SetGroundDecalAlpha
---
```lua
function Spring.SetGroundDecalAlpha(
  decalID: integer,
  alpha: number?,
  alphaFalloff: number?
) -> decalSet boolean
```
@param `alpha` - (Default: currAlpha) Between 0 and 1

@param `alphaFalloff` - (Default: currAlphaFalloff) Between 0 and 1, per second













### Spring.SetGroundDecalNormal
---
```lua
function Spring.SetGroundDecalNormal(
  decalID: integer,
  normalX: number?,
  normalY: number?,
  normalZ: number?
) -> decalSet boolean
```
@param `normalX` - (Default: `0`)

@param `normalY` - (Default: `0`)

@param `normalZ` - (Default: `0`)






Sets projection cube normal to orient in 3D space.
In case the normal (0,0,0) then normal is picked from the terrain








### Spring.SetGroundDecalTint
---
```lua
function Spring.SetGroundDecalTint(
  decalID: integer,
  tintColR: number?,
  tintColG: number?,
  tintColB: number?,
  tintColA: number?
) -> decalSet boolean
```
@param `tintColR` - (Default: curTintColR)

@param `tintColG` - (Default: curTintColG)

@param `tintColB` - (Default: curTintColB)

@param `tintColA` - (Default: curTintColA)






Sets the tint of the ground decal. Color = 2 * textureColor * tintColor
Respectively a color of (0.5, 0.5, 0.5, 0.5) is effectively no tint








### Spring.SetGroundDecalMisc
---
```lua
function Spring.SetGroundDecalMisc(
  decalID: integer,
  dotElimExp: number?,
  refHeight: number?,
  minHeight: number?,
  maxHeight: number?,
  forceHeightMode: number?
) -> decalSet boolean
```
@param `dotElimExp` - (Default: curValue) pow(max(dot(decalProjVector, SurfaceNormal), 0.0), dotElimExp), used to reduce decal artifacts on surfaces non-collinear with the projection vector

@param `refHeight` - (Default: curValue)

@param `minHeight` - (Default: curValue)

@param `maxHeight` - (Default: curValue)

@param `forceHeightMode` - (Default: curValue) in case forceHeightMode==1.0 ==> force relative height: midPoint.y = refHeight + clamp(midPoint.y - refHeight, minHeight); forceHeightMode==2.0 ==> force absolute height: midPoint.y = midPoint.y, clamp(midPoint.y, minHeight, maxHeight); other forceHeightMode values do not enforce the height of the center position






Sets varios secondary parameters of a decal








### Spring.SetGroundDecalCreationFrame
---
```lua
function Spring.SetGroundDecalCreationFrame(
  decalID: integer,
  creationFrameMin: number?,
  creationFrameMax: number?
) -> decalSet boolean
```
@param `creationFrameMin` - (Default: currCreationFrameMin)

@param `creationFrameMax` - (Default: currCreationFrameMax)






Use separate min and max for "gradient" style decals such as tank tracks








### Spring.SDLSetTextInputRect
---
```lua
function Spring.SDLSetTextInputRect(
  x: number,
  y: number,
  width: number,
  height: number
) ->  nil
```












### Spring.SDLStartTextInput
---
```lua
function Spring.SDLStartTextInput() ->  nil
```












### Spring.SDLStopTextInput
---
```lua
function Spring.SDLStopTextInput() ->  nil
```












### Spring.SetWindowGeometry
---
```lua
function Spring.SetWindowGeometry(
  displayIndex: number,
  winRelPosX: number,
  winRelPosY: number,
  winSizeX: number,
  winSizeY: number,
  fullScreen: boolean,
  borderless: boolean
) ->  nil
```












### Spring.SetWindowMinimized
---
```lua
function Spring.SetWindowMinimized() -> minimized boolean
```












### Spring.SetWindowMaximized
---
```lua
function Spring.SetWindowMaximized() -> maximized boolean
```












### Spring.Reload
---
```lua
function Spring.Reload(startScript: string) ->  nil
```
@param `startScript` - the CONTENT of the script.txt spring should use to start.













### Spring.Restart
---
```lua
function Spring.Restart(
  commandline_args: string,
  startScript: string
) ->  nil
```
@param `commandline_args` - commandline arguments passed to spring executable.






If this call returns, something went wrong








### Spring.Start
---
```lua
function Spring.Start(
  commandline_args: string,
  startScript: string
) ->  nil
```
@param `commandline_args` - commandline arguments passed to spring executable.

@param `startScript` - the CONTENT of the script.txt spring should use to start (if empty, no start-script is added, you can still point spring to your custom script.txt when you add the file-path to commandline_args.






Launches a new Spring instance without terminating the existing one.

If this call returns, something went wrong








### Spring.SetWMIcon
---
```lua
function Spring.SetWMIcon(iconFileName: string) ->  nil
```





Sets the icon for the process which is seen in the OS task-bar and other places (default: spring-logo).

Note: has to be 24bit or 32bit.
Note: on windows, it has to be 32x32 pixels in size (recommended for cross-platform)
Note: *.bmp images have to be in BGR format (default for m$ ones).
Note: *.ico images are not supported.








### Spring.ClearWatchDogTimer
---
```lua
function Spring.ClearWatchDogTimer(threadName: string?) ->  nil
```
@param `threadName` - (Default: main)













### Spring.SetClipboard
---
```lua
function Spring.SetClipboard(text: string) ->  nil
```












### Spring.Yield
---
```lua
function Spring.Yield() -> when boolean
```

@return `when` - true caller should continue calling `Spring.Yield` during the widgets/gadgets load, when false it shouldn't call it any longer.





Relinquish control of the game loading thread and OpenGL context back to the UI (LuaIntro).

Should be called after each widget/unsynced gadget is loaded in widget/gadget handler. Use it to draw screen updates and process windows events.







@*usage* local wantYield = Spring.Yield and Spring.Yield() -- nil check: not present in synced
for wupget in pairs(wupgetsToLoad) do
loadWupget(wupget)
wantYield = wantYield and Spring.Yield()
end


### Spring.GetMetalMapSize
---
```lua
function Spring.GetMetalMapSize()
 -> x integer
 -> z integer

```

@return `x` - X coordinate in worldspace / `Game.metalMapSquareSize`.

@return `z` - Z coordinate in worldspace / `Game.metalMapSquareSize`.












### Spring.GetMetalAmount
---
```lua
function Spring.GetMetalAmount(
  x: integer,
  z: integer
) -> amount number
```
@param `x` - X coordinate in worldspace / `Game.metalMapSquareSize`.

@param `z` - Z coordinate in worldspace / `Game.metalMapSquareSize`.






Returns the amount of metal on a single square.








### Spring.SetMetalAmount
---
```lua
function Spring.SetMetalAmount(
  x: integer,
  z: integer,
  metalAmount: number
) ->  nil
```
@param `x` - X coordinate in worldspace / `Game.metalMapSquareSize`.

@param `z` - Z coordinate in worldspace / `Game.metalMapSquareSize`.

@param `metalAmount` - must be between 0 and 255*maxMetal (with maxMetal from the .smd or mapinfo.lua).






Sets the amount of metal on a single square.








### Spring.GetMetalExtraction
---
```lua
function Spring.GetMetalExtraction(
  x: integer,
  z: integer
) -> extraction number
```
@param `x` - X coordinate in worldspace / `Game.metalMapSquareSize`.

@param `z` - Z coordinate in worldspace / `Game.metalMapSquareSize`.













### Spring.Echo
---
```lua
function Spring.Echo(
  arg: any,
  ...: any
) ->  nil
```





Prints values in the spring chat console. Useful for debugging.

Hint: the default print() writes to STDOUT.








### Spring.Log
---
```lua
function Spring.Log(
  section: string,
  logLevel: (LogLevel|LOG)?,
  ...: string
)
```
@param `section` - Sets an arbitrary section. Level filtering can be applied per-section

@param `logLevel` - (Default: `"notice"`)

@param `...` - messages






Logs a message to the logfile/console.








### Spring.IsCheatingEnabled
---
```lua
function Spring.IsCheatingEnabled() -> enabled boolean
```












### Spring.IsGodModeEnabled
---
```lua
function Spring.IsGodModeEnabled() -> enabled boolean
```












### Spring.IsDevLuaEnabled
---
```lua
function Spring.IsDevLuaEnabled() -> enabled boolean
```












### Spring.IsEditDefsEnabled
---
```lua
function Spring.IsEditDefsEnabled() -> enabled boolean
```












### Spring.IsNoCostEnabled
---
```lua
function Spring.IsNoCostEnabled() -> enabled boolean
```












### Spring.GetGlobalLos
---
```lua
function Spring.GetGlobalLos(teamID: integer?) -> enabled boolean
```












### Spring.AreHelperAIsEnabled
---
```lua
function Spring.AreHelperAIsEnabled() -> enabled boolean
```












### Spring.FixedAllies
---
```lua
function Spring.FixedAllies() -> enabled boolean?
```












### Spring.IsGameOver
---
```lua
function Spring.IsGameOver() -> isGameOver boolean
```












### Spring.GetGameFrame
---
```lua
function Spring.GetGameFrame()
 -> t1 number
 -> t2 number

```

@return `t1` - frameNum % dayFrames

@return `t2` - frameNum / dayFrames












### Spring.GetGameSeconds
---
```lua
function Spring.GetGameSeconds() -> seconds number
```












### Spring.GetTidal
---
```lua
function Spring.GetTidal() -> tidalStrength number
```












### Spring.GetWind
---
```lua
function Spring.GetWind() -> windStrength number
```












### Spring.GetGameRulesParams
---
```lua
function Spring.GetGameRulesParams() -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values












### Spring.GetTeamRulesParams
---
```lua
function Spring.GetTeamRulesParams(teamID: integer) -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values












### Spring.GetPlayerRulesParams
---
```lua
function Spring.GetPlayerRulesParams(playerID: integer) -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values












### Spring.GetUnitRulesParams
---
```lua
function Spring.GetUnitRulesParams(unitID: integer) -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values












### Spring.GetFeatureRulesParams
---
```lua
function Spring.GetFeatureRulesParams(featureID: integer) -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values












### Spring.GetGameRulesParam
---
```lua
function Spring.GetGameRulesParam(ruleRef: (number|string)) ->  number?
```
@param `ruleRef` - the rule index or name


@return  - |string value












### Spring.GetTeamRulesParam
---
```lua
function Spring.GetTeamRulesParam(
  teamID: integer,
  ruleRef: (number|string)
) -> value (number|string)?
```
@param `ruleRef` - the rule index or name













### Spring.GetPlayerRulesParam
---
```lua
function Spring.GetPlayerRulesParam(
  playerID: integer,
  ruleRef: (number|string)
) -> value (number|string)?
```
@param `ruleRef` - the rule index or name













### Spring.GetUnitRulesParam
---
```lua
function Spring.GetUnitRulesParam(
  unitID: integer,
  ruleRef: (number|string)
) -> value (number|string)?
```
@param `ruleRef` - the rule index or name













### Spring.GetFeatureRulesParam
---
```lua
function Spring.GetFeatureRulesParam(
  featureID: integer,
  ruleRef: (number|string)
) -> value (number|string)?
```
@param `ruleRef` - the rule index or name













### Spring.GetMapOption
---
```lua
function Spring.GetMapOption(mapOption: string) -> value string
```

@return `value` - Value of `modOption`.












### Spring.GetMapOptions
---
```lua
function Spring.GetMapOptions() -> mapOptions table<string,string>
```

@return `mapOptions` - Table with options names as keys and values as values.












### Spring.GetModOption
---
```lua
function Spring.GetModOption(modOption: string) -> value string
```

@return `value` - Value of `modOption`.












### Spring.GetModOptions
---
```lua
function Spring.GetModOptions() -> modOptions table<string,string>
```

@return `modOptions` - Table with options names as keys and values as values.












### Spring.GetHeadingFromVector
---
```lua
function Spring.GetHeadingFromVector(
  x: number,
  z: number
) -> heading number
```












### Spring.GetVectorFromHeading
---
```lua
function Spring.GetVectorFromHeading(heading: number)
 -> x number
 -> z number

```












### Spring.GetFacingFromHeading
---
```lua
function Spring.GetFacingFromHeading(heading: number) -> facing FacingInteger
```












### Spring.GetHeadingFromFacing
---
```lua
function Spring.GetHeadingFromFacing(facing: FacingInteger) -> heading number
```












### Spring.GetSideData
---
```lua
function Spring.GetSideData(sideName: string)
 -> startUnit string?
 -> caseSensitiveSideName string

```












### Spring.GetSideData
---
```lua
function Spring.GetSideData(sideID: integer)
 -> sideName string?
 -> startUnit string
 -> caseSensitiveSideName string

```












### Spring.GetSideData
---
```lua
function Spring.GetSideData() -> sideArray SideSpec[]
```












### Spring.GetGaiaTeamID
---
```lua
function Spring.GetGaiaTeamID() -> teamID integer
```












### Spring.GetAllyTeamStartBox
---
```lua
function Spring.GetAllyTeamStartBox(allyID: integer)
 -> xMin number?
 -> zMin number?
 -> xMax number?
 -> zMax number?

```












### Spring.GetTeamStartPosition
---
```lua
function Spring.GetTeamStartPosition(teamID: integer)
 -> x number?
 -> y number?
 -> x number?

```












### Spring.GetMapStartPositions
---
```lua
function Spring.GetMapStartPositions() -> array float3[]
```

@return `array` - of positions indexed by teamID












### Spring.GetAllyTeamList
---
```lua
function Spring.GetAllyTeamList() -> allyTeamIDs integer[]
```












### Spring.GetTeamList
---
```lua
function Spring.GetTeamList(allyTeamID: unknown) -> teamIDs number[]
```
@param `allyTeamID` - (Default: `-1`)


@return `teamIDs` - List of team IDs.





Get all team IDs.








### Spring.GetTeamList
---
```lua
function Spring.GetTeamList(allyTeamID: integer) -> teamIDs number[]?
```
@param `allyTeamID` - The ally team ID to filter teams by. A value less than 0 will return all teams.


@return `teamIDs` - List of team IDs or `nil` if `allyTeamID` is invalid.





Get team IDs in a specific ally team.








### Spring.GetPlayerList
---
```lua
function Spring.GetPlayerList(
  teamID: integer?,
  active: boolean?
) -> list number[]?
```
@param `teamID` - (Default: `-1`) to filter by when >= 0

@param `active` - (Default: `false`) whether to filter only active teams


@return `list` - of playerIDs












### Spring.GetTeamInfo
---
```lua
function Spring.GetTeamInfo(
  teamID: integer,
  getTeamKeys: boolean?
)
 -> teamID integer?
 -> leader number
 -> isDead number
 -> hasAI number
 -> side string
 -> allyTeam number
 -> incomeMultiplier number
 -> customTeamKeys table<string,string>

```
@param `getTeamKeys` - (Default: `true`) whether to return the customTeamKeys table


@return `customTeamKeys` - when getTeamKeys is true, otherwise nil












### Spring.GetTeamAllyTeamID
---
```lua
function Spring.GetTeamAllyTeamID(teamID: integer) -> allyTeamID integer?
```












### Spring.GetTeamResources
---
```lua
function Spring.GetTeamResources(
  teamID: integer,
  resource: ResourceName
)
 -> currentLevel number?
 -> storage number
 -> pull number
 -> income number
 -> expense number
 -> share number
 -> sent number
 -> received number
 -> excess number

```












### Spring.GetTeamUnitStats
---
```lua
function Spring.GetTeamUnitStats(teamID: integer)
 -> killed number?
 -> died number
 -> capturedBy number
 -> capturedFrom number
 -> received number
 -> sent number

```












### Spring.GetTeamResourceStats
---
```lua
function Spring.GetTeamResourceStats(
  teamID: integer,
  resource: ResourceName
)
 -> used number?
 -> produced number
 -> excessed number
 -> received number
 -> sent number

```












### Spring.GetTeamDamageStats
---
```lua
function Spring.GetTeamDamageStats(teamID: integer)
 -> damageDealt number
 -> damageReceived number

```





Gets team damage dealt/received totals

Returns a team's damage stats. Note that all damage is counted,
including self-inflicted and unconfirmed out-of-sight.








### Spring.GetTeamStatsHistory
---
```lua
function Spring.GetTeamStatsHistory(teamID: integer) -> historyCount integer?
```

@return `historyCount` - The number of history entries, or `nil` if unable to resolve team.





Get the number of history entries.








### Spring.GetTeamStatsHistory
---
```lua
function Spring.GetTeamStatsHistory(
  teamID: integer,
  startIndex: integer,
  endIndex: integer?
) -> The TeamStats[]
```
@param `endIndex` - (Default: startIndex)


@return `The` - team stats history, or `nil` if unable to resolve team.





Get team stats history.








### Spring.GetTeamLuaAI
---
```lua
function Spring.GetTeamLuaAI(teamID: integer) ->  string
```












### Spring.GetTeamMaxUnits
---
```lua
function Spring.GetTeamMaxUnits(teamID: integer)
 -> maxUnits number
 -> currentUnits number?

```





Returns a team's unit cap.

Also returns the current unit count for readable teams as the 2nd value.








### Spring.GetPlayerInfo
---
```lua
function Spring.GetPlayerInfo(
  playerID: integer,
  getPlayerOpts: boolean?
)
 -> name string
 -> active boolean
 -> spectator boolean
 -> teamID integer
 -> allyTeamID integer
 -> pingTime number
 -> cpuUsage number
 -> country string
 -> rank number
 -> hasSkirmishAIsInTeam boolean
 -> playerOpts { , [string]: string }
 -> desynced boolean

```
@param `getPlayerOpts` - (Default: `true`) whether to return custom player options


@return `playerOpts` - when playerOpts is true












### Spring.GetPlayerControlledUnit
---
```lua
function Spring.GetPlayerControlledUnit(playerID: integer) ->  number?
```





Returns unit controlled by player on FPS mode








### Spring.GetAIInfo
---
```lua
function Spring.GetAIInfo(teamID: integer)
 -> skirmishAIID integer
 -> name string
 -> hostingPlayerID integer
 -> shortName string
 -> version string
 -> options table<string,string>

```

@return `shortName` - When synced `"SYNCED_NOSHORTNAME"`, otherwise the AI shortname or `"UNKNOWN"`.

@return `version` - When synced `"SYNCED_NOVERSION"`, otherwise the AI version or `"UNKNOWN"`.












### Spring.GetAllyTeamInfo
---
```lua
function Spring.GetAllyTeamInfo(allyTeamID: integer) ->  table<string,string>?
```












### Spring.AreTeamsAllied
---
```lua
function Spring.AreTeamsAllied(
  teamID1: number,
  teamID2: number
) ->  boolean?
```












### Spring.ArePlayersAllied
---
```lua
function Spring.ArePlayersAllied(
  playerID1: number,
  playerID2: number
) ->  boolean?
```












### Spring.GetAllUnits
---
```lua
function Spring.GetAllUnits() -> unitIDs number[]
```





Get a list of all unitIDs

Note that when called from a widget, this also returns units that are only
radar blips.

For units that are radar blips, you may want to check if they are in los,
as GetUnitDefID() will still return true if they have previously been seen.





@see UnsyncedRead.GetVisibleUnits




### Spring.GetTeamUnits
---
```lua
function Spring.GetTeamUnits(teamID: integer) -> unitIDs number[]?
```












### Spring.GetTeamUnitsSorted
---
```lua
function Spring.GetTeamUnitsSorted(teamID: integer) -> unitsByDef table<integer,integer>
```

@return `unitsByDef` - A table where keys are unitDefIDs and values are unitIDs












### Spring.GetTeamUnitsCounts
---
```lua
function Spring.GetTeamUnitsCounts(teamID: integer) -> countByUnit table<number,number>?
```

@return `countByUnit` - A table where keys are unitDefIDs and values are counts.












### Spring.GetTeamUnitsByDefs
---
```lua
function Spring.GetTeamUnitsByDefs(
  teamID: integer,
  unitDefIDs: (number|number[])
) -> unitIDs number[]?
```












### Spring.GetTeamUnitDefCount
---
```lua
function Spring.GetTeamUnitDefCount(
  teamID: integer,
  unitDefID: integer
) -> count number?
```












### Spring.GetTeamUnitCount
---
```lua
function Spring.GetTeamUnitCount(teamID: integer) -> count number?
```












### Spring.GetUnitsInRectangle
---
```lua
function Spring.GetUnitsInRectangle(
  xmin: number,
  zmin: number,
  xmax: number,
  zmax: number,
  allegiance: number?
) -> unitIDs number[]
```












### Spring.GetUnitsInBox
---
```lua
function Spring.GetUnitsInBox(
  xmin: number,
  ymin: number,
  zmin: number,
  xmax: number,
  ymax: number,
  zmax: number,
  allegiance: number?
) -> unitIDs number[]
```












### Spring.GetUnitsInCylinder
---
```lua
function Spring.GetUnitsInCylinder(
  x: number,
  z: number,
  radius: number
) -> unitIDs number[]
```












### Spring.GetUnitsInSphere
---
```lua
function Spring.GetUnitsInSphere(
  x: number,
  y: number,
  z: number,
  radius: number
) -> unitIDs number[]
```












### Spring.GetUnitsInPlanes
---
```lua
function Spring.GetUnitsInPlanes(
  planes: Plane[],
  allegiance: integer?
) -> unitIDs integer[]
```





Plane normals point towards accepted space, so the acceptance criteria for each plane is:

```
radius     = unit radius
px, py, pz = unit position
[(nx * px) + (ny * py) + (nz * pz) + (d - radius)]  <=  0
```








### Spring.GetUnitArrayCentroid
---
```lua
function Spring.GetUnitArrayCentroid(units: table)
 -> centerX number
 -> centerY number
 -> centerZ number

```
@param `units` - { unitID, unitID, ... }






Returns the centroid of an array of units

Returns nil for an empty array








### Spring.GetUnitMapCentroid
---
```lua
function Spring.GetUnitMapCentroid(units: table)
 -> centerX number
 -> centerY number
 -> centerZ number

```
@param `units` - { [unitID] = true, [unitID] = true, ... }






Returns the centroid of a map of units

Returns nil for an empty map








### Spring.GetUnitNearestAlly
---
```lua
function Spring.GetUnitNearestAlly(
  unitID: integer,
  range: number?
) -> unitID integer?
```
@param `range` - (Default: `1.0e9`)













### Spring.GetUnitNearestEnemy
---
```lua
function Spring.GetUnitNearestEnemy(
  unitID: integer,
  range: number?,
  useLOS: boolean?
) -> unitID integer?
```
@param `range` - (Default: `1.0e9`)

@param `useLOS` - (Default: `true`)













### Spring.GetFeaturesInRectangle
---
```lua
function Spring.GetFeaturesInRectangle(
  xmin: number,
  zmin: number,
  xmax: number,
  zmax: number
) -> featureIDs number[]
```












### Spring.GetFeaturesInSphere
---
```lua
function Spring.GetFeaturesInSphere(
  x: number,
  y: number,
  z: number,
  radius: number
) -> featureIDs number[]
```












### Spring.GetFeaturesInCylinder
---
```lua
function Spring.GetFeaturesInCylinder(
  x: number,
  z: number,
  radius: number,
  allegiance: number?
) -> featureIDs number[]
```












### Spring.GetProjectilesInRectangle
---
```lua
function Spring.GetProjectilesInRectangle(
  xmin: number,
  zmin: number,
  xmax: number,
  zmax: number,
  excludeWeaponProjectiles: boolean?,
  excludePieceProjectiles: boolean?
) -> projectileIDs number[]
```
@param `excludeWeaponProjectiles` - (Default: `false`)

@param `excludePieceProjectiles` - (Default: `false`)













### Spring.GetProjectilesInSphere
---
```lua
function Spring.GetProjectilesInSphere(
  x: number,
  y: number,
  z: number,
  radius: number,
  excludeWeaponProjectiles: boolean?,
  excludePieceProjectiles: boolean?
) -> projectileIDs number[]
```
@param `excludeWeaponProjectiles` - (Default: false)

@param `excludePieceProjectiles` - (Default: false)













### Spring.ValidUnitID
---
```lua
function Spring.ValidUnitID(unitID: integer) ->  boolean
```












### Spring.GetUnitStates
---
```lua
function Spring.GetUnitStates(unitID: integer) ->  UnitState
```












### Spring.GetUnitArmored
---
```lua
function Spring.GetUnitArmored(unitID: integer)
 -> armored boolean?
 -> armorMultiple number

```












### Spring.GetUnitIsActive
---
```lua
function Spring.GetUnitIsActive(unitID: integer) -> isActive boolean?
```












### Spring.GetUnitIsCloaked
---
```lua
function Spring.GetUnitIsCloaked(unitID: integer) -> isCloaked boolean?
```












### Spring.GetUnitSeismicSignature
---
```lua
function Spring.GetUnitSeismicSignature(unitID: integer) -> seismicSignature number?
```












### Spring.GetUnitSelfDTime
---
```lua
function Spring.GetUnitSelfDTime(unitID: integer) -> selfDTime integer?
```












### Spring.GetUnitStockpile
---
```lua
function Spring.GetUnitStockpile(unitID: integer)
 -> numStockpiled integer?
 -> numStockpileQued integer?
 -> buildPercent number?

```












### Spring.GetUnitSensorRadius
---
```lua
function Spring.GetUnitSensorRadius(
  unitID: integer,
  type: string
) -> radius number?
```
@param `type` - one of los, airLos, radar, sonar, seismic, radarJammer, sonarJammer













### Spring.GetUnitPosErrorParams
---
```lua
function Spring.GetUnitPosErrorParams(
  unitID: integer,
  allyTeamID: integer?
)
 -> posErrorVectorX number?
 -> posErrorVectorY number
 -> posErrorVectorZ number
 -> posErrorDeltaX number
 -> posErrorDeltaY number
 -> posErrorDeltaZ number
 -> nextPosErrorUpdatebaseErrorMult number
 -> posErrorBit boolean

```












### Spring.GetUnitTooltip
---
```lua
function Spring.GetUnitTooltip(unitID: integer) ->  string?
```












### Spring.GetUnitDefID
---
```lua
function Spring.GetUnitDefID(unitID: integer) ->  number?
```












### Spring.GetUnitTeam
---
```lua
function Spring.GetUnitTeam(unitID: integer) ->  number?
```












### Spring.GetUnitAllyTeam
---
```lua
function Spring.GetUnitAllyTeam(unitID: integer) ->  number?
```












### Spring.GetUnitNeutral
---
```lua
function Spring.GetUnitNeutral(unitID: integer) ->  boolean?
```





Checks if a unit is neutral (NOT Gaia!)

Note that a "neutral" unit can belong to any ally-team (ally, enemy, Gaia).
To check if a unit is Gaia, check its owner team.








### Spring.GetUnitHealth
---
```lua
function Spring.GetUnitHealth(unitID: integer)
 -> health number?
 -> maxHealth number
 -> paralyzeDamage number
 -> captureProgress number
 -> buildProgress number

```

@return `buildProgress` - between 0.0-1.0












### Spring.GetUnitIsDead
---
```lua
function Spring.GetUnitIsDead(unitID: integer) ->  boolean?
```












### Spring.GetUnitIsStunned
---
```lua
function Spring.GetUnitIsStunned(unitID: integer)
 -> stunnedOrBuilt boolean?
 -> stunned boolean
 -> beingBuilt boolean

```

@return `stunnedOrBuilt` - unit is disabled

@return `stunned` - unit is either stunned via EMP or being transported by a non-fireplatform

@return `beingBuilt` - unit is under construction





Checks whether a unit is disabled and can't act

The first return value is a simple OR of the following ones,
any of those conditions is sufficient to disable the unit.

Note that EMP and being transported are mechanically the same and thus lumped together.
Use other callouts to differentiate them if you need to.








### Spring.GetUnitIsBeingBuilt
---
```lua
function Spring.GetUnitIsBeingBuilt(unitID: integer)
 -> beingBuilt boolean
 -> buildProgress number

```












### Spring.GetUnitResources
---
```lua
function Spring.GetUnitResources(unitID: integer)
 -> metalMake number?
 -> metalUse number
 -> energyMake number
 -> energyUse number

```












### Spring.GetUnitStorage
---
```lua
function Spring.GetUnitStorage(unitID: integer)
 -> Unit number
 -> Unit number

```

@return `Unit` - 's metal storage

@return `Unit` - 's energy storage












### Spring.GetUnitCosts
---
```lua
function Spring.GetUnitCosts(unitID: integer)
 -> buildTime number?
 -> metalCost number
 -> energyCost number

```












### Spring.GetUnitCostTable
---
```lua
function Spring.GetUnitCostTable(unitID: integer)
 -> cost ResourceCost?
 -> buildTime number?

```

@return `cost` - The cost of the unit, or `nil` if invalid.

@return `buildTime` - The build time the unit, or `nil` if invalid.












### Spring.GetUnitMetalExtraction
---
```lua
function Spring.GetUnitMetalExtraction(unitID: integer) -> metalExtraction number?
```












### Spring.GetUnitExperience
---
```lua
function Spring.GetUnitExperience(unitID: integer)
 -> xp number
 -> limXp number

```

@return `xp` - [0.0; +∞)

@return `limXp` - [0.0; 1.0) as experience approaches infinity












### Spring.GetUnitHeight
---
```lua
function Spring.GetUnitHeight(unitID: integer) ->  number?
```












### Spring.GetUnitRadius
---
```lua
function Spring.GetUnitRadius(unitID: integer) ->  number?
```












### Spring.GetUnitBuildeeRadius
---
```lua
function Spring.GetUnitBuildeeRadius(unitID: integer) ->  number?
```





Gets the unit's radius for when targeted by build, repair, reclaim-type commands.








### Spring.GetUnitMass
---
```lua
function Spring.GetUnitMass(unitID: integer) ->  number?
```












### Spring.GetUnitPosition
---
```lua
function Spring.GetUnitPosition(
  unitID: integer,
  midPos: boolean?,
  aimPos: boolean?
)
 -> basePointX number?
 -> basePointY number
 -> basePointZ number
 -> midPointX number?
 -> midPointY number
 -> midPointZ number
 -> aimPointX number?
 -> aimPointY number
 -> aimPointZ number

```
@param `midPos` - (Default: `false`) return midpoint as well

@param `aimPos` - (Default: `false`) return aimpoint as well













### Spring.GetUnitBasePosition
---
```lua
function Spring.GetUnitBasePosition(unitID: integer)
 -> posX number?
 -> posY number
 -> posZ number

```












### Spring.GetUnitVectors
---
```lua
function Spring.GetUnitVectors(unitID: integer)
 -> front float3?
 -> up float3
 -> right float3

```












### Spring.GetUnitRotation
---
```lua
function Spring.GetUnitRotation(unitID: integer)
 -> pitch number
 -> yaw number
 -> roll number

```

@return `pitch` - Rotation in X axis

@return `yaw` - Rotation in Y axis

@return `roll` - Rotation in Z axis





Note: PYR order








### Spring.GetUnitDirection
---
```lua
function Spring.GetUnitDirection(unitID: integer)
 -> frontDirX number
 -> frontDirY number
 -> frontDirZ number
 -> rightDirX number
 -> rightDirY number
 -> rightDirZ number
 -> upDirX number
 -> upDirY number
 -> upDirZ number

```












### Spring.GetUnitHeading
---
```lua
function Spring.GetUnitHeading(
  unitID: integer,
  convertToRadians: boolean?
) -> heading number
```
@param `convertToRadians` - (Default: `false`)













### Spring.GetUnitVelocity
---
```lua
function Spring.GetUnitVelocity(unitID: integer)
```












### Spring.GetUnitBuildFacing
---
```lua
function Spring.GetUnitBuildFacing(unitID: integer)
```












### Spring.GetUnitIsBuilding
---
```lua
function Spring.GetUnitIsBuilding(unitID: integer) -> buildeeUnitID integer
```

@return `buildeeUnitID` - or nil





Checks whether a unit is currently building another (NOT for checking if it's a structure)

Works for both mobile builders and factories.








### Spring.GetUnitWorkerTask
---
```lua
function Spring.GetUnitWorkerTask(unitID: integer)
 -> cmdID integer
 -> targetID integer

```

@return `cmdID` - of the relevant command

@return `targetID` - if applicable (all except RESTORE)





Checks a builder's current task

Checks what a builder is currently doing. This is not the same as `Spring.GetUnitCurrentCommand`,
because you can have a command at the front of the queue and not be doing it (for example because
the target is still too far away), and on the other hand you can also be doing a task despite not
having it in front of the queue (for example you're Guarding another builder who does). Also, it
resolves the Repair command into either actual repair, or construction assist (in which case it
returns the appropriate "build" command). Only build-related commands are returned (no Move or any
custom commands).

The possible commands returned are repair, reclaim, resurrect, capture, restore,
and build commands (negative buildee unitDefID).








### Spring.GetUnitEffectiveBuildRange
---
```lua
function Spring.GetUnitEffectiveBuildRange(
  unitID: integer,
  buildeeDefID: integer
) -> effectiveBuildRange number
```
@param `buildeeDefID` - or nil


@return `effectiveBuildRange` - counted to the center of prospective buildee; buildRange if buildee nil





Useful for setting move goals manually.








### Spring.GetUnitCurrentBuildPower
---
```lua
function Spring.GetUnitCurrentBuildPower(unitID: integer)
```












### Spring.GetUnitHarvestStorage
---
```lua
function Spring.GetUnitHarvestStorage(unitID: integer)
 -> storedMetal number
 -> maxStoredMetal number
 -> storedEnergy number
 -> maxStoredEnergy number

```





Get a unit's carried resources

Checks resources being carried internally by the unit.








### Spring.GetUnitBuildParams
---
```lua
function Spring.GetUnitBuildParams(unitID: integer)
```












### Spring.GetUnitInBuildStance
---
```lua
function Spring.GetUnitInBuildStance(unitID: integer) -> inBuildStance boolean
```





Is builder in build stance

Checks if a builder is in build stance, i.e. can create nanoframes.
Returns nil for non-builders.








### Spring.GetUnitNanoPieces
---
```lua
function Spring.GetUnitNanoPieces(unitID: integer) -> pieceArray integer[]
```





Get construction FX attachment points

Returns an array of pieces which represent construction
points. Default engine construction FX (nano spray) will
originate there.

Only works on builders and factories, returns nil (NOT empty table)
for other units.








### Spring.GetUnitTransporter
---
```lua
function Spring.GetUnitTransporter(unitID: integer) -> transportUnitID integer?
```





Get the transport carrying the unit

Returns the unit ID of the transport, if any.
Returns nil if the unit is not being transported.








### Spring.GetUnitIsTransporting
---
```lua
function Spring.GetUnitIsTransporting(unitID: integer) -> transporteeArray integer[]?
```

@return `transporteeArray` - An array of unitIDs being transported by this unit, or `nil` if not a transport.





Get units being transported








### Spring.GetUnitShieldState
---
```lua
function Spring.GetUnitShieldState(
  unitID: integer,
  weaponNum: number?
)
 -> isEnabled number
 -> currentPower number

```
@param `weaponNum` - Optional if the unit has just one shield


@return `isEnabled` - Warning, number not boolean. 0 or 1












### Spring.GetUnitFlanking
---
```lua
function Spring.GetUnitFlanking(unitID: integer)
```












### Spring.GetUnitMaxRange
---
```lua
function Spring.GetUnitMaxRange(unitID: integer) -> maxRange number
```





Get a unit's engagement range

Returns the range at which a unit will stop to engage.
By default this is the highest among the unit's weapon ranges (hence name),
but can be changed dynamically. Also note that unarmed units ignore this.








### Spring.GetUnitWeaponState
---
```lua
function Spring.GetUnitWeaponState(
  unitID: integer,
  weaponNum: number,
  stateName: string
) -> stateValue number
```





Check the state of a unit's weapon

Available states to poll:
"reloadFrame" (frame on which the weapon will be ready to fire),
"reloadSpeed" (reload time in seconds),
"range" (in elmos),
"autoTargetRangeBoost" (predictive aiming range buffer, in elmos),
"projectileSpeed" (in elmos/frame),
"reloadTimeXP" (reload time after XP bonus, in seconds),
"reaimTime" (frames between AimWeapon calls),
"burst" (shots in a burst),
"burstRate" (delay between shots in a burst, in seconds),
"projectiles" (projectiles per shot),
"salvoLeft" (shots remaining in ongoing burst),
"nextSalvo" (simframe of the next shot in an ongoing burst),
"accuracy" (INaccuracy after XP bonus),
"sprayAngle" (spray angle after XP bonus),
"targetMoveError" (extra inaccuracy against moving targets, after XP bonus)
"avoidFlags" (bitmask for targeting avoidance),
"ttl" (number of seconds a projectile should live)
"collisionFlags" (bitmask for collisions).

The state "salvoError" is an exception and returns a table: {x, y, z},
which represents the inaccuracy error of the ongoing burst.








### Spring.GetUnitWeaponDamages
---
```lua
function Spring.GetUnitWeaponDamages(unitID: integer)
```












### Spring.GetUnitWeaponVectors
---
```lua
function Spring.GetUnitWeaponVectors(unitID: integer)
```












### Spring.GetUnitWeaponTryTarget
---
```lua
function Spring.GetUnitWeaponTryTarget(unitID: integer)
```












### Spring.GetUnitWeaponTestTarget
---
```lua
function Spring.GetUnitWeaponTestTarget(unitID: integer)
```












### Spring.GetUnitWeaponTestRange
---
```lua
function Spring.GetUnitWeaponTestRange(unitID: integer)
```












### Spring.GetUnitWeaponHaveFreeLineOfFire
---
```lua
function Spring.GetUnitWeaponHaveFreeLineOfFire(unitID: integer)
```












### Spring.GetUnitWeaponCanFire
---
```lua
function Spring.GetUnitWeaponCanFire(unitID: integer)
```












### Spring.GetUnitWeaponTarget
---
```lua
function Spring.GetUnitWeaponTarget(
  unitID: integer,
  weaponNum: integer
)
 -> TargetType 0
 -> isUserTarget boolean

```

@return `TargetType` - none





Checks a weapon's target

Note that this doesn't need to reflect the unit's Attack orders or such, and
that weapons can aim individually unless slaved.








### Spring.GetUnitWeaponTarget
---
```lua
function Spring.GetUnitWeaponTarget(
  unitID: integer,
  weaponNum: integer
)
 -> TargetType 1
 -> isUserTarget boolean
 -> targetUnitID integer

```

@return `TargetType` - unit





Checks a weapon's target

Note that this doesn't need to reflect the unit's Attack orders or such, and
that weapons can aim individually unless slaved.








### Spring.GetUnitWeaponTarget
---
```lua
function Spring.GetUnitWeaponTarget(
  unitID: integer,
  weaponNum: integer
)
 -> TargetType 2
 -> isUserTarget boolean
 -> targetPosition float3

```

@return `TargetType` - position





Checks a weapon's target

Note that this doesn't need to reflect the unit's Attack orders or such, and
that weapons can aim individually unless slaved.








### Spring.GetUnitWeaponTarget
---
```lua
function Spring.GetUnitWeaponTarget(
  unitID: integer,
  weaponNum: integer
)
 -> TargetType 3
 -> isUserTarget boolean
 -> targetProjectileId integer

```

@return `TargetType` - projectileID





Checks a weapon's target

Note that this doesn't need to reflect the unit's Attack orders or such, and
that weapons can aim individually unless slaved.








### Spring.GetUnitEstimatedPath
---
```lua
function Spring.GetUnitEstimatedPath(unitID: integer)
```












### Spring.GetUnitLastAttacker
---
```lua
function Spring.GetUnitLastAttacker(unitID: integer)
```












### Spring.GetUnitLastAttackedPiece
---
```lua
function Spring.GetUnitLastAttackedPiece(unitID: integer)
```












### Spring.GetUnitCollisionVolumeData
---
```lua
function Spring.GetUnitCollisionVolumeData(unitID: integer)
```












### Spring.GetUnitSeparation
---
```lua
function Spring.GetUnitSeparation(
  unitID1: number,
  unitID2: number,
  direction: boolean?,
  subtractRadii: boolean?
) ->  number?
```
@param `direction` - (Default: `false`) to subtract from, default unitID1 - unitID2

@param `subtractRadii` - (Default: `false`) whether units radii should be subtracted from the total













### Spring.GetUnitFeatureSeparation
---
```lua
function Spring.GetUnitFeatureSeparation(unitID: integer)
```












### Spring.GetUnitDefDimensions
---
```lua
function Spring.GetUnitDefDimensions(unitDefID: integer) -> dimensions UnitDefDimensions?
```












### Spring.GetCEGID
---
```lua
function Spring.GetCEGID()
```












### Spring.GetUnitBlocking
---
```lua
function Spring.GetUnitBlocking(unitID: integer)
 -> isBlocking boolean?
 -> isSolidObjectCollidable boolean
 -> isProjectileCollidable boolean
 -> isRaySegmentCollidable boolean
 -> crushable boolean
 -> blockEnemyPushing boolean
 -> blockHeightChanges boolean

```












### Spring.GetUnitMoveTypeData
---
```lua
function Spring.GetUnitMoveTypeData(unitID: integer)
```












### Spring.GetUnitCurrentCommand
---
```lua
function Spring.GetUnitCurrentCommand(
  unitID: integer,
  cmdIndex: integer
)
 -> cmdID CMD
 -> options (integer|CommandOptionBit)
 -> tag integer
 -> Command number ...

```
@param `unitID` - Unit id.

@param `cmdIndex` - Command index to get. If negative will count from the end of the queue,
for example -1 will be the last command.


@return `Command` - parameters.












### Spring.GetUnitCommands
---
```lua
function Spring.GetUnitCommands(
  unitID: integer,
  count: integer
) -> commands Command[]
```
@param `count` - Number of commands to return, `-1` returns all commands, `0` returns command count.






Get the commands for a unit.

Same as `Spring.GetCommandQueue`








### Spring.GetUnitCommands
---
```lua
function Spring.GetUnitCommands(
  unitID: integer,
  count: 0
) -> The integer
```
@param `count` - Returns the number of commands in the units queue.


@return `The` - number of commands in the unit queue.





Get the count of commands for a unit.

Same as `Spring.GetCommandQueue`








### Spring.GetFactoryCommands
---
```lua
function Spring.GetFactoryCommands(
  unitID: integer,
  count: number
) -> commands (number|Command[])
```
@param `count` - when 0 returns the number of commands in the units queue, when -1 returns all commands, number of commands to return otherwise






Get the number or list of commands for a factory








### Spring.GetUnitCommandCount
---
```lua
function Spring.GetUnitCommandCount(unitID: integer) -> The integer
```

@return `The` - number of commands in the unit's queue.





Get the number of commands in a unit's queue.








### Spring.GetFactoryBuggerOff
---
```lua
function Spring.GetFactoryBuggerOff(unitID: integer)
```












### Spring.GetFactoryCounts
---
```lua
function Spring.GetFactoryCounts(
  unitID: integer,
  count: integer?,
  addCmds: boolean?
) -> counts table<number,number>?
```
@param `count` - (Default: `-1`) Number of commands to retrieve, `-1` for all.

@param `addCmds` - (Default: `false`) Retrieve commands other than buildunit


@return `counts` - Build queue count by `unitDefID` or `-cmdID`, or `nil` if unit is not found.





Gets the build queue of a factory








### Spring.GetCommandQueue
---
```lua
function Spring.GetCommandQueue(
  unitID: integer,
  count: integer
) -> commands Command[]
```
@param `count` - Number of commands to return, `-1` returns all commands, `0` returns command count.






Get the commands for a unit.

Same as `Spring.GetUnitCommands`








### Spring.GetCommandQueue
---
```lua
function Spring.GetCommandQueue(
  unitID: integer,
  count: 0
) -> The integer
```
@param `count` - Returns the number of commands in the units queue.


@return `The` - number of commands in the unit queue.





Get the count of commands for a unit.

Same as `Spring.GetUnitCommands`








### Spring.GetFullBuildQueue
---
```lua
function Spring.GetFullBuildQueue(unitID: integer) -> buildqueue table<number,number>?
```

@return `buildqueue` - indexed by unitDefID with count values





Returns the build queue








### Spring.GetRealBuildQueue
---
```lua
function Spring.GetRealBuildQueue(unitID: integer) -> buildqueue table<number,number>?
```

@return `buildqueue` - indexed by unitDefID with count values





Returns the build queue cleaned of things the unit can't build itself








### Spring.GetUnitCmdDescs
---
```lua
function Spring.GetUnitCmdDescs(unitID: integer)
```












### Spring.FindUnitCmdDesc
---
```lua
function Spring.FindUnitCmdDesc(unitID: integer)
```












### Spring.ValidFeatureID
---
```lua
function Spring.ValidFeatureID(featureID: integer) ->  boolean
```












### Spring.GetAllFeatures
---
```lua
function Spring.GetAllFeatures() -> featureIDs integer[]
```












### Spring.GetFeatureDefID
---
```lua
function Spring.GetFeatureDefID(featureID: integer) ->  number?
```












### Spring.GetFeatureTeam
---
```lua
function Spring.GetFeatureTeam(featureID: integer) ->  number?
```












### Spring.GetFeatureAllyTeam
---
```lua
function Spring.GetFeatureAllyTeam(featureID: integer) ->  number?
```












### Spring.GetFeatureHealth
---
```lua
function Spring.GetFeatureHealth(featureID: integer)
 -> health number?
 -> defHealth number
 -> resurrectProgress number

```












### Spring.GetFeatureHeight
---
```lua
function Spring.GetFeatureHeight(featureID: integer) ->  number?
```












### Spring.GetFeatureRadius
---
```lua
function Spring.GetFeatureRadius(featureID: integer) ->  number?
```












### Spring.GetFeatureMass
---
```lua
function Spring.GetFeatureMass(featureID: integer) ->  number?
```












### Spring.GetFeaturePosition
---
```lua
function Spring.GetFeaturePosition(featureID: integer)
```












### Spring.GetFeatureSeparation
---
```lua
function Spring.GetFeatureSeparation(
  featureID1: number,
  featureID2: number,
  direction: boolean?
) ->  number?
```
@param `direction` - (Default: `false`) to subtract from, default featureID1 - featureID2













### Spring.GetFeatureRotation
---
```lua
function Spring.GetFeatureRotation(featureID: integer)
 -> pitch number
 -> yaw number
 -> roll number

```

@return `pitch` - Rotation in X axis

@return `yaw` - Rotation in Y axis

@return `roll` - Rotation in Z axis





Note: PYR order








### Spring.GetFeatureDirection
---
```lua
function Spring.GetFeatureDirection(featureID: integer)
 -> frontDirX number
 -> frontDirY number
 -> frontDirZ number
 -> rightDirX number
 -> rightDirY number
 -> rightDirZ number
 -> upDirX number
 -> upDirY number
 -> upDirZ number

```












### Spring.GetFeatureVelocity
---
```lua
function Spring.GetFeatureVelocity(featureID: integer)
```












### Spring.GetFeatureHeading
---
```lua
function Spring.GetFeatureHeading(featureID: integer)
```












### Spring.GetFeatureResources
---
```lua
function Spring.GetFeatureResources(featureID: integer)
 -> metal number?
 -> defMetal number
 -> energy number
 -> defEnergy number
 -> reclaimLeft number
 -> reclaimTime number

```












### Spring.GetFeatureBlocking
---
```lua
function Spring.GetFeatureBlocking(featureID: integer)
 -> isBlocking boolean?
 -> isSolidObjectCollidable boolean
 -> isProjectileCollidable boolean
 -> isRaySegmentCollidable boolean
 -> crushable boolean
 -> blockEnemyPushing boolean
 -> blockHeightChanges boolean

```












### Spring.GetFeatureNoSelect
---
```lua
function Spring.GetFeatureNoSelect(featureID: integer) ->  boolean?
```












### Spring.GetFeatureResurrect
---
```lua
function Spring.GetFeatureResurrect(featureID: integer)
```












### Spring.GetFeatureLastAttackedPiece
---
```lua
function Spring.GetFeatureLastAttackedPiece(featureID: integer)
```












### Spring.GetFeatureCollisionVolumeData
---
```lua
function Spring.GetFeatureCollisionVolumeData(featureID: integer)
```












### Spring.GetFeaturePieceCollisionVolumeData
---
```lua
function Spring.GetFeaturePieceCollisionVolumeData(featureID: integer)
```












### Spring.GetProjectilePosition
---
```lua
function Spring.GetProjectilePosition(projectileID: integer)
 -> posX number?
 -> posY number
 -> posZ number

```












### Spring.GetProjectileDirection
---
```lua
function Spring.GetProjectileDirection(projectileID: integer)
 -> dirX number?
 -> dirY number
 -> dirZ number

```












### Spring.GetProjectileVelocity
---
```lua
function Spring.GetProjectileVelocity(projectileID: integer)
 -> velX number?
 -> velY number
 -> velZ number
 -> velW number

```












### Spring.GetProjectileGravity
---
```lua
function Spring.GetProjectileGravity(projectileID: integer) ->  number?
```












### Spring.GetPieceProjectileParams
---
```lua
function Spring.GetPieceProjectileParams(projectileID: integer)
 -> explosionFlags number?
 -> spinAngle number
 -> spinSpeed number
 -> spinVectorX number
 -> spinVectorY number
 -> spinVectorZ number

```

@return `explosionFlags` - encoded bitwise with SHATTER = 1, EXPLODE = 2, EXPLODE_ON_HIT = 2, FALL = 4, SMOKE = 8, FIRE = 16, NONE = 32, NO_CEG_TRAIL = 64, NO_HEATCLOUD = 128












### Spring.GetProjectileTarget
---
```lua
function Spring.GetProjectileTarget(projectileID: integer)
 -> targetTypeInt number?
 -> target (number|float3)

```

@return `targetTypeInt` - where
string.byte('g') := GROUND
string.byte('u') := UNIT
string.byte('f') := FEATURE
string.byte('p') := PROJECTILE

@return `target` - targetID or targetPos when targetTypeInt == string.byte('g')












### Spring.GetProjectileIsIntercepted
---
```lua
function Spring.GetProjectileIsIntercepted(projectileID: integer) ->  boolean?
```












### Spring.GetProjectileTimeToLive
---
```lua
function Spring.GetProjectileTimeToLive(projectileID: integer) ->  number?
```












### Spring.GetProjectileOwnerID
---
```lua
function Spring.GetProjectileOwnerID(projectileID: integer) ->  number?
```












### Spring.GetProjectileTeamID
---
```lua
function Spring.GetProjectileTeamID(projectileID: integer) ->  number?
```












### Spring.GetProjectileAllyTeamID
---
```lua
function Spring.GetProjectileAllyTeamID(projectileID: integer) ->  number?
```












### Spring.GetProjectileType
---
```lua
function Spring.GetProjectileType(projectileID: integer)
 -> weapon boolean?
 -> piece boolean

```












### Spring.GetProjectileDefID
---
```lua
function Spring.GetProjectileDefID(projectileID: integer) ->  number?
```





Using this to get a weaponDefID is HIGHLY preferred to indexing WeaponDefNames via GetProjectileName








### Spring.GetProjectileDamages
---
```lua
function Spring.GetProjectileDamages(
  projectileID: integer,
  tag: string
) ->  number?
```
@param `tag` - one of:
"paralyzeDamageTime"
"impulseFactor"
"impulseBoost"
"craterMult"
"craterBoost"
"dynDamageExp"
"dynDamageMin"
"dynDamageRange"
"dynDamageInverted"
"craterAreaOfEffect"
"damageAreaOfEffect"
"edgeEffectiveness"
"explosionSpeed"
- or -
an armor type index to get the damage against it.













### Spring.IsPosInMap
---
```lua
function Spring.IsPosInMap(
  x: number,
  z: number
)
 -> inPlayArea boolean
 -> inMap boolean

```

@return `inPlayArea` - whether the position is in the active play area

@return `inMap` - whether the position is in the full map area (currently this is the same as above)












### Spring.GetGroundHeight
---
```lua
function Spring.GetGroundHeight(
  x: number,
  z: number
) ->  number
```





Get ground height

On sea, this returns the negative depth of the seafloor








### Spring.GetWaterPlaneLevel
---
```lua
function Spring.GetWaterPlaneLevel() -> waterPlaneLevel number
```





Get water plane height

Water may at some point become shaped (rivers etc) but for now it is always a flat plane.
Use this function instead of GetWaterLevel to denote you are relying on that assumption.





@see Spring.GetWaterLevel




### Spring.GetWaterLevel
---
```lua
function Spring.GetWaterLevel(
  x: number,
  z: number
) -> waterLevel number
```





Get water level in a specific position

Water is currently a flat plane, so this returns the same value regardless of XZ.
However water may become more dynamic at some point so by using this you are future-proof.








### Spring.GetGroundOrigHeight
---
```lua
function Spring.GetGroundOrigHeight(
  x: number,
  z: number
) ->  number
```





Get ground height as it was at game start

Returns the original height before the ground got deformed








### Spring.GetGroundNormal
---
```lua
function Spring.GetGroundNormal(
  x: number,
  z: number,
  smoothed: boolean?
)
 -> normalX number
 -> normalY number
 -> normalZ number
 -> slope number

```
@param `smoothed` - (Default: `false`) raw or smoothed center normal













### Spring.GetGroundInfo
---
```lua
function Spring.GetGroundInfo(
  x: number,
  z: number
)
 -> ix number
 -> iz number
 -> terrainTypeIndex number
 -> name string
 -> metalExtraction number
 -> hardness number
 -> tankSpeed number
 -> kbotSpeed number
 -> hoverSpeed number
 -> shipSpeed number
 -> receiveTracks boolean

```












### Spring.GetGroundBlocked
---
```lua
function Spring.GetGroundBlocked()
```












### Spring.GetGroundExtremes
---
```lua
function Spring.GetGroundExtremes()
 -> initMinHeight number
 -> initMaxHeight number
 -> currMinHeight number
 -> currMaxHeight number

```












### Spring.GetTerrainTypeData
---
```lua
function Spring.GetTerrainTypeData(terrainTypeInfo: number)
 -> index number
 -> name string
 -> hardness number
 -> tankSpeed number
 -> kbotSpeed number
 -> hoverSpeed number
 -> shipSpeed number
 -> receiveTracks boolean

```












### Spring.GetGrass
---
```lua
function Spring.GetGrass(
  x: number,
  z: number
) ->  number
```












### Spring.GetSmoothMeshHeight
---
```lua
function Spring.GetSmoothMeshHeight(
  x: number,
  z: number
) -> height number
```












### Spring.TestMoveOrder
---
```lua
function Spring.TestMoveOrder(
  unitDefID: integer,
  posX: number,
  posY: number,
  posZ: number,
  dirX: number?,
  dirY: number?,
  dirZ: number?,
  testTerrain: boolean?,
  testObjects: boolean?,
  centerOnly: boolean?
) ->  boolean
```
@param `dirX` - (Default: `0.0`)

@param `dirY` - (Default: `0.0`)

@param `dirZ` - (Default: `0.0`)

@param `testTerrain` - (Default: `true`)

@param `testObjects` - (Default: `true`)

@param `centerOnly` - (Default: `false`)













### Spring.TestBuildOrder
---
```lua
function Spring.TestBuildOrder(
  unitDefID: integer,
  x: number,
  y: number,
  z: number,
  facing: Facing
)
 -> blocking BuildOrderBlockedStatus
 -> featureID integer?

```

@return `featureID` - A reclaimable feature in the way.












### Spring.Pos2BuildPos
---
```lua
function Spring.Pos2BuildPos(
  unitDefID: integer,
  posX: number,
  posY: number,
  posZ: number,
  buildFacing: number?
)
 -> buildPosX number
 -> buildPosY number
 -> buildPosZ number

```
@param `buildFacing` - (Default: `0`) one of SOUTH = 0, EAST = 1, NORTH = 2, WEST  = 3






Snaps a position to the building grid








### Spring.ClosestBuildPos
---
```lua
function Spring.ClosestBuildPos(
  teamID: integer,
  unitDefID: integer,
  posX: number,
  posY: number,
  posZ: number,
  searchRadius: number,
  minDistance: number,
  buildFacing: number
)
 -> buildPosX number
 -> buildPosY number
 -> buildPosZ number

```
@param `buildFacing` - one of SOUTH = 0, EAST = 1, NORTH = 2, WEST  = 3













### Spring.GetPositionLosState
---
```lua
function Spring.GetPositionLosState(
  posX: number,
  posY: number,
  posZ: number,
  allyTeamID: integer?
)
 -> inLosOrRadar boolean
 -> inLos boolean
 -> inRadar boolean
 -> inJammer boolean

```












### Spring.IsPosInLos
---
```lua
function Spring.IsPosInLos(
  posX: number,
  posY: number,
  posZ: number,
  allyTeamID: integer?
) ->  boolean
```












### Spring.IsPosInRadar
---
```lua
function Spring.IsPosInRadar(
  posX: number,
  posY: number,
  posZ: number,
  allyTeamID: integer?
) ->  boolean
```












### Spring.IsPosInAirLos
---
```lua
function Spring.IsPosInAirLos(
  posX: number,
  posY: number,
  posZ: number,
  allyTeamID: integer?
) ->  boolean
```












### Spring.GetUnitLosState
---
```lua
function Spring.GetUnitLosState(
  unitID: integer,
  allyTeamID: integer?,
  raw: true
) -> bitmask integer?
```
@param `raw` - Return a bitmask.


@return `bitmask` - A bitmask integer, or `nil` if `unitID` is invalid.

Bitmask bits:
- `1`: `LOS_INLOS` the unit is currently in the los of the allyteam,
- `2`: `LOS_INRADAR` the unit is currently in radar from the allyteam,
- `4`: `LOS_PREVLOS` the unit has previously been in los from the allyteam,
- `8`: `LOS_CONTRADAR` the unit has continuously been in radar since it was last inlos by the allyteam












### Spring.GetUnitLosState
---
```lua
function Spring.GetUnitLosState(
  unitID: integer,
  allyTeamID: integer?,
  raw: false?
) -> los { los: boolean,radar: boolean, ... }?
```
@param `raw` - Return a bitmask.


@return `los` - A table of LOS state, or `nil` if `unitID` is invalid.












### Spring.IsUnitInLos
---
```lua
function Spring.IsUnitInLos(
  unitID: integer,
  allyTeamID: integer
) -> inLos boolean
```












### Spring.IsUnitInAirLos
---
```lua
function Spring.IsUnitInAirLos(
  unitID: integer,
  allyTeamID: integer
) -> inAirLos boolean
```












### Spring.IsUnitInRadar
---
```lua
function Spring.IsUnitInRadar(
  unitID: integer,
  allyTeamID: integer
) -> inRadar boolean
```












### Spring.IsUnitInJammer
---
```lua
function Spring.IsUnitInJammer(
  unitID: integer,
  allyTeamID: integer
) -> inJammer boolean
```












### Spring.GetModelRootPiece
---
```lua
function Spring.GetModelRootPiece(modelName: string) -> index number
```

@return `index` - of the root piece












### Spring.GetModelPieceMap
---
```lua
function Spring.GetModelPieceMap(modelName: string) -> pieceInfos table<string,number>?
```

@return `pieceInfos` - where keys are piece names and values are indices












### Spring.GetModelPieceList
---
```lua
function Spring.GetModelPieceList(modelName: string) -> pieceNames string[]?
```












### Spring.GetUnitRootPiece
---
```lua
function Spring.GetUnitRootPiece(unitID: integer) -> index number
```

@return `index` - of the root piece












### Spring.GetUnitPieceMap
---
```lua
function Spring.GetUnitPieceMap(unitID: integer) -> pieceInfos table<string,number>?
```

@return `pieceInfos` - where keys are piece names and values are indices












### Spring.GetUnitPieceList
---
```lua
function Spring.GetUnitPieceList(unitID: integer) -> pieceNames string[]
```












### Spring.GetUnitPieceInfo
---
```lua
function Spring.GetUnitPieceInfo(
  unitID: integer,
  pieceIndex: integer
) -> pieceInfo PieceInfo?
```












### Spring.GetUnitPiecePosDir
---
```lua
function Spring.GetUnitPiecePosDir(
  unitID: integer,
  pieceIndex: integer
)
 -> posX number?
 -> posY number
 -> posZ number
 -> dirX number
 -> dirY number
 -> dirZ number

```












### Spring.GetUnitPiecePosition
---
```lua
function Spring.GetUnitPiecePosition(
  unitID: integer,
  pieceIndex: integer
)
 -> posX number?
 -> posY number
 -> posZ number

```












### Spring.GetUnitPieceDirection
---
```lua
function Spring.GetUnitPieceDirection(
  unitID: integer,
  pieceIndex: integer
)
 -> dirX number?
 -> dirY number
 -> dirZ number

```












### Spring.GetUnitPieceMatrix
---
```lua
function Spring.GetUnitPieceMatrix(unitID: integer)
 -> m11 number?
 -> m12 number
 -> m13 number
 -> m14 number
 -> m21 number
 -> m22 number
 -> m23 number
 -> m24 number
 -> m31 number
 -> m32 number
 -> m33 number
 -> m34 number
 -> m41 number
 -> m42 number
 -> m43 number
 -> m44 number

```












### Spring.GetFeatureRootPiece
---
```lua
function Spring.GetFeatureRootPiece(featureID: integer) -> index number
```

@return `index` - of the root piece












### Spring.GetFeaturePieceMap
---
```lua
function Spring.GetFeaturePieceMap(featureID: integer) -> pieceInfos table<string,number>
```

@return `pieceInfos` - where keys are piece names and values are indices












### Spring.GetFeaturePieceList
---
```lua
function Spring.GetFeaturePieceList(featureID: integer) -> pieceNames string[]
```












### Spring.GetFeaturePieceInfo
---
```lua
function Spring.GetFeaturePieceInfo(
  featureID: integer,
  pieceIndex: integer
) -> pieceInfo PieceInfo?
```












### Spring.GetFeaturePiecePosDir
---
```lua
function Spring.GetFeaturePiecePosDir(
  featureID: integer,
  pieceIndex: integer
)
 -> posX number?
 -> posY number
 -> posZ number
 -> dirX number
 -> dirY number
 -> dirZ number

```












### Spring.GetFeaturePiecePosition
---
```lua
function Spring.GetFeaturePiecePosition(
  featureID: integer,
  pieceIndex: integer
)
 -> posX number?
 -> posY number
 -> posZ number

```












### Spring.GetFeaturePieceDirection
---
```lua
function Spring.GetFeaturePieceDirection(
  featureID: integer,
  pieceIndex: integer
)
 -> dirX number?
 -> dirY number
 -> dirZ number

```












### Spring.GetFeaturePieceMatrix
---
```lua
function Spring.GetFeaturePieceMatrix(featureID: integer)
 -> m11 number?
 -> m12 number
 -> m13 number
 -> m14 number
 -> m21 number
 -> m22 number
 -> m23 number
 -> m24 number
 -> m31 number
 -> m32 number
 -> m33 number
 -> m34 number
 -> m41 number
 -> m42 number
 -> m43 number
 -> m44 number

```












### Spring.GetUnitScriptPiece
---
```lua
function Spring.GetUnitScriptPiece(unitID: integer) -> pieceIndices integer[]
```












### Spring.GetUnitScriptPiece
---
```lua
function Spring.GetUnitScriptPiece(
  unitID: integer,
  scriptPiece: integer
) -> pieceIndex integer
```












### Spring.GetUnitScriptNames
---
```lua
function Spring.GetUnitScriptNames(unitID: integer) -> where table<string,number>
```

@return `where` - keys are piece names and values are piece indices












### Spring.TraceRayGroundInDirection
---
```lua
function Spring.TraceRayGroundInDirection(
  posX: number,
  posY: number,
  posZ: number,
  dirX: number,
  dirY: number,
  dirZ: number,
  testWater: boolean?
)
 -> rayLength number
 -> posX number
 -> posY number
 -> posZ number

```
@param `testWater` - (Default: `true`)






Checks for a ground collision in given direction

Checks if there is surface (ground, optionally water) towards a vector
and returns the distance to the closest hit and its position, if any.








### Spring.TraceRayGroundBetweenPositions
---
```lua
function Spring.TraceRayGroundBetweenPositions(
  startX: number,
  startY: number,
  startZ: number,
  endX: number,
  endY: number,
  endZ: number,
  testWater: boolean?
)
 -> rayLength number
 -> posX number
 -> posY number
 -> posZ number

```
@param `testWater` - (Default: `true`)






Checks for a ground collision between two positions

Checks if there is surface (ground, optionally water) between two positions
and returns the distance to the closest hit and its position, if any.








### Spring.GetRadarErrorParams
---
```lua
function Spring.GetRadarErrorParams(allyTeamID: integer)
 -> radarErrorSize number?
 -> baseRadarErrorSize number
 -> baseRadarErrorMult number

```

@return `radarErrorSize` - actual radar error size (when allyTeamID is allied to current team) or base radar error size












### Spring.SetAlly
---
```lua
function Spring.SetAlly(
  firstAllyTeamID: integer,
  secondAllyTeamID: integer,
  ally: boolean
) ->  nil
```





Changes the value of the (one-sided) alliance between: firstAllyTeamID -> secondAllyTeamID.








### Spring.SetAllyTeamStartBox
---
```lua
function Spring.SetAllyTeamStartBox(
  allyTeamID: integer,
  xMin: number,
  zMin: number,
  xMax: number,
  zMax: number
) ->  nil
```
@param `xMin` - left start box boundary (elmos)

@param `zMin` - top start box boundary (elmos)

@param `xMax` - right start box boundary (elmos)

@param `zMax` - bottom start box boundary (elmos)






Changes the start box position of an allyTeam.








### Spring.AssignPlayerToTeam
---
```lua
function Spring.AssignPlayerToTeam(
  playerID: integer,
  teamID: integer
) ->  nil
```





Assigns a player to a team.








### Spring.SetGlobalLos
---
```lua
function Spring.SetGlobalLos(
  allyTeamID: integer,
  globallos: boolean
) ->  nil
```





Changes access to global line of sight for a team and its allies.








### Spring.KillTeam
---
```lua
function Spring.KillTeam(teamID: integer) ->  nil
```





Will declare a team to be dead (no further orders can be assigned to such teams units).

Gaia team cannot be killed.








### Spring.GameOver
---
```lua
function Spring.GameOver(winningAllyTeamIDs: integer[])
```
@param `winningAllyTeamIDs` - A list of winning ally team IDs. Pass
multiple winners to declare a draw. Pass no arguments if undecided (e.g.
when dropped from the host).






Declare game over.







@*returns* integer Number of accepted (valid) ally teams.


### Spring.SetTidal
---
```lua
function Spring.SetTidal(strength: number) ->  nil
```





Set tidal strength








### Spring.SetWind
---
```lua
function Spring.SetWind(
  minStrength: number,
  maxStrength: number
) ->  nil
```





Set wind strength








### Spring.AddTeamResource
---
```lua
function Spring.AddTeamResource(
  teamID: integer,
  type: ResourceName,
  amount: number
) ->  nil
```





Adds metal or energy resources to the specified team.








### Spring.UseTeamResource
---
```lua
function Spring.UseTeamResource(
  teamID: integer,
  type: ResourceName,
  amount: number
) -> hadEnough boolean
```
@param `type` - Resource type.

@param `amount` - Amount of resource to use.


@return `hadEnough` - True if enough of the resource type was available and was consumed, otherwise false.





Consumes metal or energy resources of the specified team.








### Spring.UseTeamResource
---
```lua
function Spring.UseTeamResource(
  teamID: integer,
  amount: ResourceUsage
) -> hadEnough boolean
```

@return `hadEnough` - True if enough of the resource type(s) were available and was consumed, otherwise false.





Consumes metal and/or energy resources of the specified team.








### Spring.SetTeamResource
---
```lua
function Spring.SetTeamResource(
  teamID: integer,
  resource: (ResourceName|StorageName),
  amount: number
) ->  nil
```












### Spring.SetTeamShareLevel
---
```lua
function Spring.SetTeamShareLevel(
  teamID: integer,
  type: ResourceName,
  amount: number
) ->  nil
```





Changes the resource amount for a team beyond which resources aren't stored but transferred to other allied teams if possible.








### Spring.ShareTeamResource
---
```lua
function Spring.ShareTeamResource(
  teamID_src: integer,
  teamID_recv: integer,
  type: ResourceName,
  amount: number
) ->  nil
```





Transfers resources between two teams.








### Spring.SetGameRulesParam
---
```lua
function Spring.SetGameRulesParam(
  paramName: string,
  paramValue: (number|string)?,
  losAccess: losAccess?
) ->  nil
```
@param `paramValue` - numeric paramValues in quotes will be converted to number.













### Spring.SetTeamRulesParam
---
```lua
function Spring.SetTeamRulesParam(
  teamID: integer,
  paramName: string,
  paramValue: (number|string)?,
  losAccess: losAccess?
) ->  nil
```
@param `paramValue` - numeric paramValues in quotes will be converted to number.













### Spring.SetPlayerRulesParam
---
```lua
function Spring.SetPlayerRulesParam(
  playerID: integer,
  paramName: string,
  paramValue: (number|string)?,
  losAccess: losAccess?
) ->  nil
```
@param `paramValue` - numeric paramValues in quotes will be converted to number.













### Spring.SetUnitRulesParam
---
```lua
function Spring.SetUnitRulesParam(
  unitID: integer,
  paramName: string,
  paramValue: (number|string)?,
  losAccess: losAccess?
) ->  nil
```
@param `paramValue` - numeric paramValues in quotes will be converted to number.













### Spring.SetFeatureRulesParam
---
```lua
function Spring.SetFeatureRulesParam(
  featureID: integer,
  paramName: string,
  paramValue: (number|string)?,
  losAccess: losAccess?
) ->  nil
```
@param `paramValue` - numeric paramValues in quotes will be converted to number.













### Spring.CallCOBScript
---
```lua
function Spring.CallCOBScript(
  unitID: integer,
  funcName: (integer|string)?,
  retArgs: integer,
  ...: any
) ->  number ...
```
@param `funcName` - Function ID or name.

@param `retArgs` - Number of values to return.

@param `...` - Arguments













### Spring.GetCOBScriptID
---
```lua
function Spring.GetCOBScriptID(
  unitID: integer,
  funcName: string
) -> funcID integer?
```












### Spring.CreateUnit
---
```lua
function Spring.CreateUnit(
  unitDef: (string|integer),
  posX: number,
  posY: number,
  posZ: number,
  facing: Facing,
  teamID: integer,
  build: boolean?,
  flattenGround: boolean?,
  unitID: integer?,
  builderID: integer?
) -> unitID integer?
```
@param `unitDef` - UnitDef name or ID.

@param `build` - (Default: `false`) The unit is created in "being built" state with zero `buildProgress`.

@param `flattenGround` - (Default: `true`) The unit flattens ground, if it normally does so.

@param `unitID` - Request a specific unitID.


@return `unitID` - The ID of the created unit, or `nil` if the unit could not be created.









@see Spring.DestroyUnit




### Spring.DestroyUnit
---
```lua
function Spring.DestroyUnit(
  unitID: integer,
  selfd: boolean?,
  reclaimed: boolean?,
  attackerID: integer?,
  cleanupImmediately: boolean?
) ->  nil
```
@param `selfd` - (Default: `false`) makes the unit act like it self-destructed.

@param `reclaimed` - (Default: `false`) don't show any DeathSequences, don't leave a wreckage. This does not give back the resources to the team!

@param `cleanupImmediately` - (Default: `false`) stronger version of reclaimed, removes the unit unconditionally and makes its ID available for immediate reuse (otherwise it takes a few frames)










@see Spring.CreateUnit




### Spring.TransferUnit
---
```lua
function Spring.TransferUnit(
  unitID: integer,
  newTeamID: integer,
  given: boolean?
) ->  nil
```
@param `given` - (Default: `true`) if false, the unit is captured.













### Spring.SetUnitCosts
---
```lua
function Spring.SetUnitCosts(
  unitID: integer,
  where: table<number,number>
) ->  nil
```
@param `where` - keys and values are, respectively and in this order: buildTime=amount, metalCost=amount, energyCost=amount













### Spring.SetUnitResourcing
---
```lua
function Spring.SetUnitResourcing(
  unitID: integer,
  res: string,
  amount: number
) ->  nil
```












### Spring.SetUnitResourcing
---
```lua
function Spring.SetUnitResourcing(
  unitID: integer,
  res: table<string,number>
) ->  nil
```
@param `res` - keys are: "[u|c][u|m][m|e]" unconditional | conditional, use | make, metal | energy. Values are amounts













### Spring.SetUnitStorage
---
```lua
function Spring.SetUnitStorage(
  unitID: integer,
  res: string,
  amount: number
)
```












### Spring.SetUnitStorage
---
```lua
function Spring.SetUnitStorage(
  unitID: integer,
  res: ResourceUsage
)
```
@param `res` - keys are: "[m|e]" metal | energy. Values are amounts













### Spring.SetUnitTooltip
---
```lua
function Spring.SetUnitTooltip(
  unitID: integer,
  tooltip: string
) ->  nil
```












### Spring.SetUnitHealth
---
```lua
function Spring.SetUnitHealth(
  unitID: integer,
  health: (number|SetUnitHealthAmounts)
) ->  nil
```
@param `health` - If a number, sets the units health
to that value. Pass a table to update health, capture progress, paralyze
damage, and build progress.






Note, if your game's custom shading framework doesn't support reverting into nanoframes
then reverting into nanoframes via the "build" tag will fail to render properly.





@see SetUnitHealthAmounts




### Spring.SetUnitMaxHealth
---
```lua
function Spring.SetUnitMaxHealth(
  unitID: integer,
  maxHealth: number
) ->  nil
```












### Spring.SetUnitStockpile
---
```lua
function Spring.SetUnitStockpile(
  unitID: integer,
  stockpile: number?,
  buildPercent: number?
) ->  nil
```












### Spring.SetUnitUseWeapons
---
```lua
function Spring.SetUnitUseWeapons(
  unitID: integer,
  forceUseWeapons: number?,
  allowUseWeapons: number?
) ->  nil
```












### Spring.SetUnitWeaponState
---
```lua
function Spring.SetUnitWeaponState(
  unitID: integer,
  weaponNum: number,
  states: WeaponState
) ->  nil
```












### Spring.SetUnitWeaponState
---
```lua
function Spring.SetUnitWeaponState(
  unitID: integer,
  weaponNum: number,
  key: string,
  value: number
) ->  nil
```












### Spring.SetUnitWeaponDamages
---
```lua
function Spring.SetUnitWeaponDamages(
  unitID: integer,
  weaponNum: (number|"selfDestruct"|"explode"),
  damages: WeaponDamages
) ->  nil
```












### Spring.SetUnitWeaponDamages
---
```lua
function Spring.SetUnitWeaponDamages(
  unitID: integer,
  weaponNum: (number|"selfDestruct"|"explode"),
  key: string,
  value: number
) ->  nil
```












### Spring.SetUnitMaxRange
---
```lua
function Spring.SetUnitMaxRange(
  unitID: integer,
  maxRange: number
) ->  nil
```












### Spring.SetUnitExperience
---
```lua
function Spring.SetUnitExperience(
  unitID: integer,
  experience: number
) ->  nil
```









@see Spring.GetUnitExperience




### Spring.AddUnitExperience
---
```lua
function Spring.AddUnitExperience(
  unitID: integer,
  deltaExperience: number
) ->  nil
```
@param `deltaExperience` - Can be negative to subtract, but the unit will never have negative total afterwards










@see Spring.GetUnitExperience




### Spring.SetUnitArmored
---
```lua
function Spring.SetUnitArmored(
  unitID: integer,
  armored: boolean?,
  armorMultiple: number?
) ->  nil
```












### Spring.SetUnitLosMask
---
```lua
function Spring.SetUnitLosMask(
  unitID: integer,
  allyTeam: number,
  losTypes: (number|table)
) ->  nil
```





The 3rd argument is either the bit-and combination of the following numbers:

LOS_INLOS = 1
LOS_INRADAR = 2
LOS_PREVLOS = 4
LOS_CONTRADAR = 8

or a table of the following form:

losTypes = {
[los = boolean,]
[radar = boolean,]
[prevLos = boolean,]
[contRadar = boolean]
}








### Spring.SetUnitLosState
---
```lua
function Spring.SetUnitLosState(
  unitID: integer,
  allyTeam: number,
  los: (number|table)
) ->  nil
```












### Spring.SetUnitCloak
---
```lua
function Spring.SetUnitCloak(
  unitID: integer,
  cloak: (boolean|number),
  cloakArg: (boolean|number)
) ->  nil
```





If the 2nd argument is a number, the value works like this:
1:=normal cloak
2:=for free cloak (cost no E)
3:=for free + no decloaking (except the unit is stunned)
4:=ultimate cloak (no ecost, no decloaking, no stunned decloak)

The decloak distance is only changed:
- if the 3th argument is a number or a boolean.
- if the boolean is false it takes the default decloak distance for that unitdef,
- if the boolean is true it takes the absolute value of it.








### Spring.SetUnitStealth
---
```lua
function Spring.SetUnitStealth(
  unitID: integer,
  stealth: boolean
) ->  nil
```












### Spring.SetUnitSonarStealth
---
```lua
function Spring.SetUnitSonarStealth(
  unitID: integer,
  sonarStealth: boolean
) ->  nil
```












### Spring.SetUnitSeismicSignature
---
```lua
function Spring.SetUnitSeismicSignature(
  unitID: integer,
  seismicSignature: number
) ->  nil
```












### Spring.SetUnitAlwaysVisible
---
```lua
function Spring.SetUnitAlwaysVisible(
  unitID: integer,
  alwaysVisible: boolean
) ->  nil
```












### Spring.SetUnitUseAirLos
---
```lua
function Spring.SetUnitUseAirLos(
  unitID: integer,
  useAirLos: boolean
) ->  nil
```












### Spring.SetUnitMetalExtraction
---
```lua
function Spring.SetUnitMetalExtraction(
  unitID: integer,
  depth: number,
  range: number?
) ->  nil
```
@param `depth` - corresponds to metal extraction rate

@param `range` - similar to "extractsMetal" in unitDefs.













### Spring.SetUnitHarvestStorage
---
```lua
function Spring.SetUnitHarvestStorage(
  unitID: integer,
  metal: number
) ->  nil
```





See also harvestStorage UnitDef tag.








### Spring.SetUnitBuildParams
---
```lua
function Spring.SetUnitBuildParams(
  unitID: integer,
  paramName: string,
  value: (number|boolean)
) ->  nil
```
@param `paramName` - one of `buildRange`|`buildDistance`|`buildRange3D`

@param `value` - boolean when `paramName` is `buildRange3D`, otherwise number.













### Spring.SetUnitBuildSpeed
---
```lua
function Spring.SetUnitBuildSpeed(
  builderID: integer,
  buildSpeed: number,
  repairSpeed: number?,
  reclaimSpeed: number?,
  captureSpeed: number?,
  terraformSpeed: number?
) ->  nil
```












### Spring.SetUnitNanoPieces
---
```lua
function Spring.SetUnitNanoPieces(
  builderID: integer,
  pieces: table
) ->  nil
```





This saves a lot of engine calls, by replacing: function script.QueryNanoPiece() return currentpiece end
Use it!








### Spring.SetUnitBlocking
---
```lua
function Spring.SetUnitBlocking(
  unitID: integer,
  isBlocking: boolean?,
  isSolidObjectCollidable: boolean?,
  isProjectileCollidable: boolean?,
  isRaySegmentCollidable: boolean?,
  crushable: boolean?,
  blockEnemyPushing: boolean?,
  blockHeightChanges: boolean?
) -> isBlocking boolean
```
@param `isBlocking` - If `true` add this unit to the `GroundBlockingMap`, but only if it collides with solid objects (or is being set to collide with the `isSolidObjectCollidable` argument). If `false`, remove this unit from the `GroundBlockingMap`. No change if `nil`.

@param `isSolidObjectCollidable` - Enable or disable collision with solid objects, or no change if `nil`.

@param `isProjectileCollidable` - Enable or disable collision with projectiles, or no change if `nil`.

@param `isRaySegmentCollidable` - Enable or disable collision with ray segments, or no change if `nil`.

@param `crushable` - Enable or disable crushable, or no change if `nil`.

@param `blockEnemyPushing` - Enable or disable blocking enemy pushing, or no change if `nil`.

@param `blockHeightChanges` - Enable or disable blocking height changes, or no change if `nil`.













### Spring.SetUnitCrashing
---
```lua
function Spring.SetUnitCrashing(
  unitID: integer,
  crashing: boolean
) -> success boolean
```












### Spring.SetUnitShieldState
---
```lua
function Spring.SetUnitShieldState(
  unitID: integer,
  weaponID: integer?,
  enabled: boolean?,
  power: number?
) ->  nil
```
@param `weaponID` - (Default: `-1`)













### Spring.SetUnitShieldRechargeDelay
---
```lua
function Spring.SetUnitShieldRechargeDelay(
  unitID: integer,
  weaponID: integer?,
  rechargeTime: number?
) ->  nil
```
@param `weaponID` - (optional if the unit only has one shield)

@param `rechargeTime` - (in seconds; emulates a regular hit if nil)













### Spring.SetUnitFlanking
---
```lua
function Spring.SetUnitFlanking(
  unitID: integer,
  type: string,
  arg1: number,
  y: number?,
  z: number?
) ->  nil
```
@param `type` - "dir"|"minDamage"|"maxDamage"|"moveFactor"|"mode"

@param `arg1` - x|minDamage|maxDamage|moveFactor|mode

@param `y` - only when type is "dir"

@param `z` - only when type is "dir"













### Spring.SetUnitPhysicalStateBit
---
```lua
function Spring.SetUnitPhysicalStateBit(
  unitID: integer,
  Physical
) ->  nil
```












### Spring.GetUnitPhysicalState
---
```lua
function Spring.GetUnitPhysicalState(unitID: integer) -> Unit number
```

@return `Unit` - 's PhysicalState bitmask












### Spring.SetUnitNeutral
---
```lua
function Spring.SetUnitNeutral(
  unitID: integer,
  neutral: boolean
) -> setNeutral boolean?
```












### Spring.SetUnitTarget
---
```lua
function Spring.SetUnitTarget(
  unitID: integer,
  enemyUnitID: integer?,
  dgun: boolean?,
  userTarget: boolean?,
  weaponNum: number?
) -> success boolean
```
@param `enemyUnitID` - when nil drops the units current target.

@param `dgun` - (Default: `false`)

@param `userTarget` - (Default: `false`)

@param `weaponNum` - (Default: `-1`)






Defines a unit's target.








### Spring.SetUnitTarget
---
```lua
function Spring.SetUnitTarget(
  unitID: integer,
  x: number?,
  y: number?,
  z: number?,
  dgun: boolean?,
  userTarget: boolean?,
  weaponNum: number?
) -> success boolean
```
@param `x` - when nil or not passed it will drop target and ignore other parameters

@param `dgun` - (Default: `false`)

@param `userTarget` - (Default: `false`)

@param `weaponNum` - (Default: `-1`)













### Spring.SetUnitMidAndAimPos
---
```lua
function Spring.SetUnitMidAndAimPos(
  unitID: integer,
  mpX: number,
  mpY: number,
  mpZ: number,
  apX: number,
  apY: number,
  apZ: number,
  relative: boolean?
) -> success boolean
```
@param `mpX` - new middle positionX of unit

@param `mpY` - new middle positionY of unit

@param `mpZ` - new middle positionZ of unit

@param `apX` - new positionX that enemies aim at on this unit

@param `apY` - new positionY that enemies aim at on this unit

@param `apZ` - new positionZ that enemies aim at on this unit

@param `relative` - (Default: `false`) are the new coordinates relative to world (false) or unit (true) coordinates? Also, note that apy is inverted!













### Spring.SetUnitRadiusAndHeight
---
```lua
function Spring.SetUnitRadiusAndHeight(
  unitID: integer,
  radius: number,
  height: number
) -> success boolean
```












### Spring.SetUnitBuildeeRadius
---
```lua
function Spring.SetUnitBuildeeRadius(
  unitID: integer,
  build: number
) ->  nil
```
@param `build` - radius for when targeted by build, repair, reclaim-type commands.






Sets the unit's radius for when targeted by build, repair, reclaim-type commands.








### Spring.SetUnitPieceParent
---
```lua
function Spring.SetUnitPieceParent(
  unitID: integer,
  AlteredPiece: number,
  ParentPiece: number
) ->  nil
```





Changes the pieces hierarchy of a unit by attaching a piece to a new parent.








### Spring.SetUnitPieceMatrix
---
```lua
function Spring.SetUnitPieceMatrix(
  unitID: integer,
  pieceNum: number,
  matrix: number[]
) ->  nil
```
@param `matrix` - an array of 16 floats






Sets the local (i.e. parent-relative) matrix of the given piece.

If any of the first three elements are non-zero, and also blocks all script animations from modifying it until {0, 0, 0} is passed.








### Spring.SetUnitCollisionVolumeData
---
```lua
function Spring.SetUnitCollisionVolumeData(
  unitID: integer,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  vType: number,
  tType: number,
  Axis: number
) -> enum nil
```

@return `enum` - COLVOL_TYPES {
COLVOL_TYPE_DISABLED = -1,
COLVOL_TYPE_ELLIPSOID = 0,
COLVOL_TYPE_CYLINDER,
COLVOL_TYPE_BOX,
COLVOL_TYPE_SPHERE,
COLVOL_NUM_TYPES       // number of non-disabled collision volume types
};
enum COLVOL_TESTS {
COLVOL_TEST_DISC = 0,
COLVOL_TEST_CONT = 1,
COLVOL_NUM_TESTS = 2   // number of tests
};
enum COLVOL_AXES {
COLVOL_AXIS_X   = 0,
COLVOL_AXIS_Y   = 1,
COLVOL_AXIS_Z   = 2,
COLVOL_NUM_AXES = 3    // number of collision volume axes
};












### Spring.SetUnitPieceCollisionVolumeData
---
```lua
function Spring.SetUnitPieceCollisionVolumeData(
  unitID: integer,
  pieceIndex: number,
  enable: boolean,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  volumeType: number?,
  primaryAxis: number?
) ->  nil
```












### Spring.SetUnitPieceVisible
---
```lua
function Spring.SetUnitPieceVisible(
  unitID: integer,
  pieceIndex: number,
  visible: boolean
) ->  nil
```












### Spring.SetUnitSensorRadius
---
```lua
function Spring.SetUnitSensorRadius(
  unitID: integer,
  type: ("los"|"airLos"|"radar"|"sonar"|"seismic"|"radarJammer"...),
  radius: number
) -> New number?
```

@return `New` - radius, or `nil` if unit is invalid.












### Spring.SetUnitPosErrorParams
---
```lua
function Spring.SetUnitPosErrorParams(
  unitID: integer,
  posErrorVectorX: number,
  posErrorVectorY: number,
  posErrorVectorZ: number,
  posErrorDeltaX: number,
  posErrorDeltaY: number,
  posErrorDeltaZ: number,
  nextPosErrorUpdate: number?
) ->  nil
```





Sets a unit's radar wobble

Controls how much a unit's radar dot will wobble. Note that setting
this above the allyTeam's default wobble may result in the edgemost
dot positions failing to register in ray traces, i.e. things like
native "is under cursor" checks and some Lua interfaces.








### Spring.SetUnitMoveGoal
---
```lua
function Spring.SetUnitMoveGoal(
  unitID: integer,
  goalX: number,
  goalY: number,
  goalZ: number,
  goalRadius: number?,
  moveSpeed: number?,
  moveRaw: boolean?
) ->  nil
```





Used by default commands to get in build-, attackrange etc.








### Spring.SetUnitLandGoal
---
```lua
function Spring.SetUnitLandGoal(
  unitID: integer,
  goalX: number,
  goalY: number,
  goalZ: number,
  goalRadius: number?
) ->  nil
```





Used in conjunction with Spring.UnitAttach et al. to re-implement old airbase & fuel system in Lua.








### Spring.ClearUnitGoal
---
```lua
function Spring.ClearUnitGoal(unitID: integer) ->  nil
```












### Spring.SetUnitPhysics
---
```lua
function Spring.SetUnitPhysics(
  unitID: integer,
  posX: number,
  posY: number,
  posZ: number,
  velX: number,
  velY: number,
  velZ: number,
  rotX: number,
  rotY: number,
  rotZ: number,
  dragX: number,
  dragY: number,
  dragZ: number
) ->  nil
```












### Spring.SetUnitMass
---
```lua
function Spring.SetUnitMass(
  unitID: integer,
  mass: number
) ->  nil
```












### Spring.SetUnitPosition
---
```lua
function Spring.SetUnitPosition(
  unitID: integer,
  x: number,
  z: number,
  floating: boolean?
) ->  nil
```
@param `floating` - (Default: `false`) If true, over water the position is on surface. If false, on seafloor.






Set unit position (2D)

Sets a unit's position in 2D, at terrain height.








### Spring.SetUnitPosition
---
```lua
function Spring.SetUnitPosition(
  unitID: integer,
  x: number,
  y: number,
  z: number
) ->  nil
```





Set unit position (3D)

Sets a unit's position in 3D, at an arbitrary height.








### Spring.SetUnitRotation
---
```lua
function Spring.SetUnitRotation(
  unitID: integer,
  pitch: number,
  yaw: number,
  roll: number
) ->  nil
```
@param `pitch` - Rotation in X axis

@param `yaw` - Rotation in Y axis

@param `roll` - Rotation in Z axis






Note: PYR order








### Spring.SetUnitDirection
---
```lua
function Spring.SetUnitDirection(
  unitID: integer,
  frontx: number,
  fronty: number,
  frontz: number
) ->  nil
```





Set unit front direction vector. The vector is normalized in
the engine.



@deprecated It's strongly that you use the overload that accepts
a right direction as `frontDir` alone doesn't define object orientation.






### Spring.SetUnitDirection
---
```lua
function Spring.SetUnitDirection(
  unitID: integer,
  frontx: number,
  fronty: number,
  frontz: number,
  rightx: number,
  righty: number,
  rightz: number
) ->  nil
```





Set unit front and right direction vectors.

Both vectors will be normalized in the engine.








### Spring.SetUnitHeadingAndUpDir
---
```lua
function Spring.SetUnitHeadingAndUpDir(
  unitID: integer,
  heading: Heading,
  upx: number,
  upy: number,
  upz: number
) ->  nil
```





Use this call to set up unit direction in a robust way. If unit was
completely upright, new `{upx, upy, upz}` direction will be used as new "up"
vector, the rotation set by "heading" will remain preserved.








### Spring.SetUnitVelocity
---
```lua
function Spring.SetUnitVelocity(
  unitID: integer,
  velX: number,
  velY: number,
  velZ: number
) ->  nil
```












### Spring.SetFactoryBuggerOff
---
```lua
function Spring.SetFactoryBuggerOff(
  unitID: integer,
  buggerOff: boolean?,
  offset: number?,
  radius: number?,
  relHeading: Heading?,
  spherical: boolean?,
  forced: boolean?
) -> buggerOff number?
```












### Spring.BuggerOff
---
```lua
function Spring.BuggerOff(
  x: number,
  y: number,
  z: number?,
  radius: number,
  teamID: integer,
  spherical: boolean?,
  forced: boolean?,
  excludeUnitID: integer?,
  excludeUnitDefIDs: number[]?
) ->  nil
```
@param `z` - uses ground height when unspecified

@param `spherical` - (Default: `true`)

@param `forced` - (Default: `true`)













### Spring.AddUnitDamage
---
```lua
function Spring.AddUnitDamage(
  unitID: integer,
  damage: number,
  paralyze: number?,
  attackerID: integer?,
  weaponID: integer?,
  impulseX: number?,
  impulseY: number?,
  impulseZ: number?
) ->  nil
```
@param `paralyze` - (Default: `0`) equals to the paralyzetime in the WeaponDef.

@param `attackerID` - (Default: `-1`)

@param `weaponID` - (Default: `-1`)













### Spring.AddUnitImpulse
---
```lua
function Spring.AddUnitImpulse(
  unitID: integer,
  x: number,
  y: number,
  z: number,
  decayRate: number?
) ->  nil
```












### Spring.AddUnitSeismicPing
---
```lua
function Spring.AddUnitSeismicPing(
  unitID: integer,
  pindSize: number
) ->  nil
```












### Spring.AddUnitResource
---
```lua
function Spring.AddUnitResource(
  unitID: integer,
  resource: string,
  amount: number
) ->  nil
```
@param `resource` - "m" | "e"













### Spring.UseUnitResource
---
```lua
function Spring.UseUnitResource(
  unitID: integer,
  resource: ResourceName,
  amount: number
) -> okay boolean?
```












### Spring.UseUnitResource
---
```lua
function Spring.UseUnitResource(
  unitID: integer,
  resources: ResourceUsage
) -> okay boolean?
```












### Spring.AddObjectDecal
---
```lua
function Spring.AddObjectDecal(unitID: integer) ->  nil
```












### Spring.RemoveObjectDecal
---
```lua
function Spring.RemoveObjectDecal(unitID: integer) ->  nil
```












### Spring.AddGrass
---
```lua
function Spring.AddGrass(
  x: number,
  z: number
) ->  nil
```












### Spring.RemoveGrass
---
```lua
function Spring.RemoveGrass(
  x: number,
  z: number
) ->  nil
```












### Spring.CreateFeature
---
```lua
function Spring.CreateFeature(
  featureDef: (string|integer),
  x: number,
  y: number,
  z: number,
  heading: Heading?,
  AllyTeamID: integer?,
  featureID: integer?
) -> featureID integer
```
@param `featureDef` - name or id













### Spring.DestroyFeature
---
```lua
function Spring.DestroyFeature(featureDefID: integer) ->  nil
```












### Spring.TransferFeature
---
```lua
function Spring.TransferFeature(
  featureDefID: integer,
  teamID: integer
) ->  nil
```





Feature Control








### Spring.SetFeatureAlwaysVisible
---
```lua
function Spring.SetFeatureAlwaysVisible(
  featureID: integer,
  enable: boolean
) ->  nil
```












### Spring.SetFeatureUseAirLos
---
```lua
function Spring.SetFeatureUseAirLos(
  featureID: integer,
  useAirLos: boolean
) ->  nil
```












### Spring.SetFeatureHealth
---
```lua
function Spring.SetFeatureHealth(
  featureID: integer,
  health: number
) ->  nil
```












### Spring.SetFeatureMaxHealth
---
```lua
function Spring.SetFeatureMaxHealth(
  featureID: integer,
  maxHealth: number
) ->  nil
```
@param `maxHealth` - minimum 0.1













### Spring.SetFeatureReclaim
---
```lua
function Spring.SetFeatureReclaim(
  featureID: integer,
  reclaimLeft: number
) ->  nil
```












### Spring.SetFeatureResources
---
```lua
function Spring.SetFeatureResources(
  featureID: integer,
  metal: number,
  energy: number,
  reclaimTime: number?,
  reclaimLeft: number?,
  featureDefMetal: number?,
  featureDefEnergy: number?
) ->  nil
```












### Spring.SetFeatureResurrect
---
```lua
function Spring.SetFeatureResurrect(
  featureID: integer,
  unitDef: (string|integer),
  facing: Facing?,
  progress: number?
) ->  nil
```
@param `unitDef` - Can be a number id or a string name, this allows cancelling resurrection by passing `-1`.

@param `facing` - (Default: `"south"`)

@param `progress` - Set the level of progress.













### Spring.SetFeatureMoveCtrl
---
```lua
function Spring.SetFeatureMoveCtrl(
  featureID: integer,
  enabled: true,
  initialVelocityX: number?,
  initialVelocityY: number?,
  initialVelocityZ: number?,
  accelerationX: number?,
  accelerationY: number?,
  accelerationZ: number?
)
```
@param `enabled` - Enable feature movement.

@param `initialVelocityX` - Initial velocity on X axis, or `nil` for no change.

@param `initialVelocityY` - Initial velocity on Y axis, or `nil` for no change.

@param `initialVelocityZ` - Initial velocity on Z axis, or `nil` for no change.

@param `accelerationX` - Acceleration per frame on X axis, or `nil` for no change.

@param `accelerationY` - Acceleration per frame on Y axis, or `nil` for no change.

@param `accelerationZ` - Acceleration per frame on Z axis, or `nil` for no change.






Enable feature movement control.








### Spring.SetFeatureMoveCtrl
---
```lua
function Spring.SetFeatureMoveCtrl(
  featureID: integer,
  enabled: false,
  velocityMaskX: number?,
  velocityMaskY: number?,
  velocityMaskZ: number?,
  impulseMaskX: number?,
  impulseMaskY: number?,
  impulseMaskZ: number?,
  movementMaskX: number?,
  movementMaskY: number?,
  movementMaskZ: number?
)
```
@param `enabled` - Disable feature movement.

@param `velocityMaskX` - Lock velocity change in X dimension when not using `MoveCtrl`. `0` to lock, non-zero to allow, or `nil` to for no change.

@param `velocityMaskY` - Lock velocity change in Y dimension when not using `MoveCtrl`. `0` to lock, non-zero to allow, or `nil` to for no change.

@param `velocityMaskZ` - Lock velocity change in Z dimension when not using `MoveCtrl`. `0` to lock, non-zero to allow, or `nil` to for no change.

@param `impulseMaskX` - Lock impulse in X dimension when not using `MoveCtrl`. `0` to lock, non-zero to allow, or `nil` to for no change.

@param `impulseMaskY` - Lock impulse in Y dimension when not using `MoveCtrl`. `0` to lock, non-zero to allow, or `nil` to for no change.

@param `impulseMaskZ` - Lock impulse in Z dimension when not using `MoveCtrl`. `0` to lock, non-zero to allow, or `nil` to for no change.

@param `movementMaskX` - Lock move in X dimension when not using `MoveCtrl`. `0` to lock the axis, non-zero to allow, or `nil` for no change.

@param `movementMaskY` - Lock move in Y dimension when not using `MoveCtrl`. `0` to lock the axis, non-zero to allow, or `nil` for no change.

@param `movementMaskZ` - Lock move in Z dimension when not using `MoveCtrl`. `0` to lock the axis, non-zero to allow, or `nil` for no change.






Disable feature movement control.

Optional parameter allow physics vectors to build when not using `MoveCtrl`.

It is necessary to unlock feature movement on x, z axis before changing
feature physics.

For example:

```lua
-- Unlock all movement before setting velocity.
Spring.SetFeatureMoveCtrl(featureID,false,1,1,1,1,1,1,1,1,1)

-- Set velocity.
Spring.SetFeatureVelocity(featureID,10,0,10)
```








### Spring.SetFeaturePhysics
---
```lua
function Spring.SetFeaturePhysics(
  featureID: integer,
  posX: number,
  posY: number,
  posZ: number,
  velX: number,
  velY: number,
  velZ: number,
  rotX: number,
  rotY: number,
  rotZ: number,
  dragX: number,
  dragY: number,
  dragZ: number
) ->  nil
```












### Spring.SetFeatureMass
---
```lua
function Spring.SetFeatureMass(
  featureID: integer,
  mass: number
) ->  nil
```












### Spring.SetFeaturePosition
---
```lua
function Spring.SetFeaturePosition(
  featureID: integer,
  x: number,
  y: number,
  z: number,
  snapToGround: boolean?
) ->  nil
```












### Spring.SetFeatureRotation
---
```lua
function Spring.SetFeatureRotation(
  featureID: integer,
  pitch: number,
  yaw: number,
  roll: number
) ->  nil
```
@param `pitch` - Rotation in X axis

@param `yaw` - Rotation in Y axis

@param `roll` - Rotation in Z axis






Note: PYR order








### Spring.SetFeatureDirection
---
```lua
function Spring.SetFeatureDirection(
  featureID: integer,
  frontx: number,
  fronty: number,
  frontz: number
) ->  nil
```





Set feature front direction vector. The vector is normalized in
the engine.



@deprecated It's strongly that you use the overload that accepts
a right direction as `frontDir` alone doesn't define object orientation.






### Spring.SetFeatureDirection
---
```lua
function Spring.SetFeatureDirection(
  featureID: integer,
  frontx: number,
  fronty: number,
  frontz: number,
  rightx: number,
  righty: number,
  rightz: number
) ->  nil
```





Set feature front and right direction vectors.

Both vectors will be normalized in the engine.








### Spring.SetFeatureHeadingAndUpDir
---
```lua
function Spring.SetFeatureHeadingAndUpDir(
  featureID: integer,
  heading: Heading,
  upx: number,
  upy: number,
  upz: number
) ->  nil
```





Use this call to set up feature direction in a robust way. If feature was
completely upright, new `{upx, upy, upz}` direction will be used as new "up"
vector, the rotation set by "heading" will remain preserved.








### Spring.SetFeatureVelocity
---
```lua
function Spring.SetFeatureVelocity(
  featureID: integer,
  velX: number,
  velY: number,
  velZ: number
) ->  nil
```












### Spring.SetFeatureBlocking
---
```lua
function Spring.SetFeatureBlocking(
  featureID: integer,
  isBlocking: boolean?,
  isSolidObjectCollidable: boolean?,
  isProjectileCollidable: boolean?,
  isRaySegmentCollidable: boolean?,
  crushable: boolean?,
  blockEnemyPushing: boolean?,
  blockHeightChanges: boolean?
) -> isBlocking boolean
```
@param `isBlocking` - If `true` add this feature to the `GroundBlockingMap`, but only if it collides with solid objects (or is being set to collide with the `isSolidObjectCollidable` argument). If `false`, remove this feature from the `GroundBlockingMap`. No change if `nil`.

@param `isSolidObjectCollidable` - Enable or disable collision with solid objects, or no change if `nil`.

@param `isProjectileCollidable` - Enable or disable collision with projectiles, or no change if `nil`.

@param `isRaySegmentCollidable` - Enable or disable collision with ray segments, or no change if `nil`.

@param `crushable` - Enable or disable crushable, or no change if `nil`.

@param `blockEnemyPushing` - Enable or disable blocking enemy pushing, or no change if `nil`.

@param `blockHeightChanges` - Enable or disable blocking height changes, or no change if `nil`.













### Spring.SetFeatureNoSelect
---
```lua
function Spring.SetFeatureNoSelect(
  featureID: integer,
  noSelect: boolean
) ->  nil
```












### Spring.SetFeatureMidAndAimPos
---
```lua
function Spring.SetFeatureMidAndAimPos(
  featureID: integer,
  mpX: number,
  mpY: number,
  mpZ: number,
  apX: number,
  apY: number,
  apZ: number,
  relative: boolean?
) -> success boolean
```





Check `Spring.SetUnitMidAndAimPos` for further explanation of the arguments.








### Spring.SetFeatureRadiusAndHeight
---
```lua
function Spring.SetFeatureRadiusAndHeight(
  featureID: integer,
  radius: number,
  height: number
) -> success boolean
```












### Spring.SetFeatureCollisionVolumeData
---
```lua
function Spring.SetFeatureCollisionVolumeData(
  featureID: integer,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  vType: number,
  tType: number,
  Axis: number
) ->  nil
```





Check `Spring.SetUnitCollisionVolumeData` for further explanation of the arguments.








### Spring.SetFeaturePieceCollisionVolumeData
---
```lua
function Spring.SetFeaturePieceCollisionVolumeData(
  featureID: integer,
  pieceIndex: number,
  enable: boolean,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  Axis: number,
  volumeType: number,
  primaryAxis: number?
) ->  nil
```












### Spring.SetFeaturePieceVisible
---
```lua
function Spring.SetFeaturePieceVisible(
  featureID: integer,
  pieceIndex: number,
  visible: boolean
) ->  nil
```












### Spring.SetProjectileAlwaysVisible
---
```lua
function Spring.SetProjectileAlwaysVisible(
  projectileID: integer,
  alwaysVisible: boolean
) ->  nil
```












### Spring.SetProjectileUseAirLos
---
```lua
function Spring.SetProjectileUseAirLos(
  projectileID: integer,
  useAirLos: boolean
) ->  nil
```












### Spring.SetProjectileMoveControl
---
```lua
function Spring.SetProjectileMoveControl(
  projectileID: integer,
  enable: boolean
) ->  nil
```





Disables engine movecontrol, so lua can fully control the physics.








### Spring.SetProjectilePosition
---
```lua
function Spring.SetProjectilePosition(
  projectileID: integer,
  posX: number?,
  posY: number?,
  posZ: number?
) ->  nil
```
@param `posX` - (Default: `0`)

@param `posY` - (Default: `0`)

@param `posZ` - (Default: `0`)













### Spring.SetProjectileVelocity
---
```lua
function Spring.SetProjectileVelocity(
  projectileID: integer,
  velX: number?,
  velY: number?,
  velZ: number?
) ->  nil
```
@param `velX` - (Default: `0`)

@param `velY` - (Default: `0`)

@param `velZ` - (Default: `0`)













### Spring.SetProjectileCollision
---
```lua
function Spring.SetProjectileCollision(projectileID: integer) ->  nil
```












### Spring.SetProjectileTarget
---
```lua
function Spring.SetProjectileTarget(
  projectileID: integer,
  arg1: number?,
  arg2: number?,
  posZ: number?
) -> validTarget boolean?
```
@param `arg1` - (Default: `0`) targetID or posX

@param `arg2` - (Default: `0`) targetType or posY

@param `posZ` - (Default: `0`)






targetTypeStr can be one of:
'u' - unit
'f' - feature
'p' - projectile
while targetTypeInt is one of:
string.byte('g') := GROUND
string.byte('u') := UNIT
string.byte('f') := FEATURE
string.byte('p') := PROJECTILE








### Spring.SetProjectileTimeToLive
---
```lua
function Spring.SetProjectileTimeToLive(
  projectileID: integer,
  ttl: number
) ->  nil
```
@param `ttl` - Remaining time to live in frames













### Spring.SetProjectileIsIntercepted
---
```lua
function Spring.SetProjectileIsIntercepted(projectileID: integer) ->  nil
```












### Spring.SetProjectileDamages
---
```lua
function Spring.SetProjectileDamages(
  unitID: integer,
  weaponNum: number,
  key: string,
  value: number
) ->  nil
```












### Spring.SetProjectileIgnoreTrackingError
---
```lua
function Spring.SetProjectileIgnoreTrackingError(
  projectileID: integer,
  ignore: boolean
) ->  nil
```












### Spring.SetProjectileGravity
---
```lua
function Spring.SetProjectileGravity(
  projectileID: integer,
  grav: number?
) ->  nil
```
@param `grav` - (Default: `0`)













### Spring.SetPieceProjectileParams
---
```lua
function Spring.SetPieceProjectileParams(
  projectileID: integer,
  explosionFlags: number?,
  spinAngle: number?,
  spinSpeed: number?,
  spinVectorX: number?,
  spinVectorY: number?,
  spinVectorZ: number?
) ->  nil
```












### Spring.SetProjectileCEG
---
```lua
function Spring.SetProjectileCEG(
  projectileID: integer,
  ceg_name: string
) ->  nil
```












### Spring.UnitFinishCommand
---
```lua
function Spring.UnitFinishCommand(unitID: integer)
```












### Spring.GiveOrderToUnit
---
```lua
function Spring.GiveOrderToUnit(
  unitID: integer,
  cmdID: (CMD|integer),
  params: CreateCommandParams?,
  options: CreateCommandOptions?,
  timeout: integer?
) -> unitOrdered boolean
```
@param `cmdID` - The command ID.

@param `params` - Parameters for the given command.













### Spring.GiveOrderToUnitMap
---
```lua
function Spring.GiveOrderToUnitMap(
  unitMap: table<integer,any>,
  cmdID: (CMD|integer),
  params: CreateCommandParams?,
  options: CreateCommandOptions?,
  timeout: integer?
) -> unitsOrdered integer
```
@param `unitMap` - A table with unit IDs as keys.

@param `cmdID` - The command ID.

@param `params` - Parameters for the given command.


@return `unitsOrdered` - The number of units ordered.





Give order to multiple units, specified by table keys.








### Spring.GiveOrderToUnitArray
---
```lua
function Spring.GiveOrderToUnitArray(
  unitIDs: integer[],
  cmdID: (CMD|integer),
  params: CreateCommandParams?,
  options: CreateCommandOptions?,
  timeout: integer?
) -> unitsOrdered integer
```
@param `unitIDs` - An array of unit IDs.

@param `cmdID` - The command ID.

@param `params` - Parameters for the given command.


@return `unitsOrdered` - The number of units ordered.












### Spring.GiveOrderArrayToUnit
---
```lua
function Spring.GiveOrderArrayToUnit(
  unitID: integer,
  commands: CreateCommand[]
) -> ordersGiven boolean
```












### Spring.GiveOrderArrayToUnitMap
---
```lua
function Spring.GiveOrderArrayToUnitMap(
  unitMap: table<integer,any>,
  commands: CreateCommand[]
) -> unitsOrdered integer
```
@param `unitMap` - A table with unit IDs as keys.


@return `unitsOrdered` - The number of units ordered.












### Spring.GiveOrderArrayToUnitArray
---
```lua
function Spring.GiveOrderArrayToUnitArray(
  unitIDs: integer[],
  commands: CreateCommand[],
  pairwise: boolean?
) -> unitsOrdered integer
```
@param `unitIDs` - Array of unit IDs.

@param `pairwise` - (Default: `false`) When `false`, assign all commands to each unit.

When `true`, assign commands according to index between units and cmds arrays.

If `len(unitArray) < len(cmdArray)` only the first `len(unitArray)` commands
will be assigned, and vice-versa.


@return `unitsOrdered` - The number of units ordered.












### Spring.LevelHeightMap
---
```lua
function Spring.LevelHeightMap(
  x1: number,
  z1: number,
  x2_height: number,
  z2: number?,
  height: number?
) ->  nil
```
@param `x2_height` - if y2 and height are nil then this parameter is the height






Set a certain height to a point or rectangle area on the world








### Spring.AdjustHeightMap
---
```lua
function Spring.AdjustHeightMap(
  x1: number,
  y1: number,
  x2_height: number,
  y2: number?,
  height: number?
) ->  nil
```
@param `x2_height` - if y2 and height are nil then this parameter is the height






Add a certain height to a point or rectangle area on the world








### Spring.RevertHeightMap
---
```lua
function Spring.RevertHeightMap(
  x1: number,
  y1: number,
  x2_factor: number,
  y2: number?,
  factor: number?
) ->  nil
```
@param `x2_factor` - if y2 and factor are nil then this parameter is the factor






Restore original map height to a point or rectangle area on the world








### Spring.AddHeightMap
---
```lua
function Spring.AddHeightMap(
  x: number,
  z: number,
  height: number
) -> newHeight integer?
```





Can only be called in `Spring.SetHeightMapFunc`








### Spring.SetHeightMap
---
```lua
function Spring.SetHeightMap(
  x: number,
  z: number,
  height: number,
  terraform: number?
) -> absHeightDiff integer?
```
@param `terraform` - (Default: `1`) Scaling factor.


@return `absHeightDiff` - If `0`, nothing will be changed (the terraform starts), if `1` the terraform will be finished.





Can only be called in `Spring.SetHeightMapFunc`. The terraform argument is








### Spring.SetHeightMapFunc
---
```lua
function Spring.SetHeightMapFunc(
  luaFunction: function,
  arg: number,
  ...: number
) -> absTotalHeightMapAmountChanged integer?
```





Example code:

```lua
function Spring.SetHeightMapFunc(function()
  for z=0,Game.mapSizeZ, Game.squareSize do
    for x=0,Game.mapSizeX, Game.squareSize do
      Spring.SetHeightMap( x, z, 200 + 20 * math.cos((x + z) / 90) )
    end
  end
end)
```








### Spring.LevelOriginalHeightMap
---
```lua
function Spring.LevelOriginalHeightMap(
  x1: number,
  y1: number,
  x2_height: number,
  y2: number?,
  height: number?
) ->  nil
```
@param `x2_height` - if y2 and height are nil then this parameter is the height






Set a height to a point or rectangle area to the original map height cache








### Spring.AdjustOriginalHeightMap
---
```lua
function Spring.AdjustOriginalHeightMap(
  x1: number,
  y1: number,
  x2_height: number,
  y2: number?,
  height: number?
) ->  nil
```
@param `x2_height` - if y2 and height are nil then this parameter is the height






Add height to a point or rectangle area to the original map height cache








### Spring.RevertOriginalHeightMap
---
```lua
function Spring.RevertOriginalHeightMap(
  x1: number,
  y1: number,
  x2_factor: number,
  y2: number?,
  factor: number?
) ->  nil
```
@param `x2_factor` - if y2 and factor are nil then this parameter is the factor






Restore original map height cache to a point or rectangle area on the world








### Spring.AddOriginalHeightMap
---
```lua
function Spring.AddOriginalHeightMap(
  x: number,
  y: number,
  height: number
) ->  nil
```





Can only be called in `Spring.SetOriginalHeightMapFunc`








### Spring.SetOriginalHeightMap
---
```lua
function Spring.SetOriginalHeightMap(
  x: number,
  y: number,
  height: number,
  factor: number?
) ->  nil
```





Can only be called in `Spring.SetOriginalHeightMapFunc`








### Spring.SetOriginalHeightMapFunc
---
```lua
function Spring.SetOriginalHeightMapFunc(heightMapFunc: function) ->  nil
```





Cannot recurse on itself








### Spring.RebuildSmoothMesh
---
```lua
function Spring.RebuildSmoothMesh() ->  nil
```





Heightmap changes normally take up to 25s to propagate to the smooth mesh.
Use to force a mapwide update immediately.








### Spring.LevelSmoothMesh
---
```lua
function Spring.LevelSmoothMesh(
  x1: number,
  z1: number,
  x2: number?,
  z2: number?,
  height: number
) ->  nil
```












### Spring.AdjustSmoothMesh
---
```lua
function Spring.AdjustSmoothMesh(
  x1: number,
  z1: number,
  x2: number?,
  z2: number?,
  height: number
) ->  nil
```












### Spring.RevertSmoothMesh
---
```lua
function Spring.RevertSmoothMesh(
  x1: number,
  z1: number,
  x2: number?,
  z2: number?,
  origFactor: number
) ->  nil
```












### Spring.AddSmoothMesh
---
```lua
function Spring.AddSmoothMesh(
  x: number,
  z: number,
  height: number
) -> height number?
```

@return `height` - The new height, or `nil` if coordinates are invalid.





Can only be called in `Spring.SetSmoothMeshFunc`.








### Spring.SetSmoothMesh
---
```lua
function Spring.SetSmoothMesh(
  x: number,
  z: number,
  height: number,
  terraform: number?
) -> The number?
```
@param `terraform` - (Default: `1`)


@return `The` - absolute height difference, or `nil` if coordinates are invalid.





Can only be called in `Spring.SetSmoothMeshFunc`.








### Spring.SetSmoothMeshFunc
---
```lua
function Spring.SetSmoothMeshFunc(
  luaFunction: function,
  arg: any,
  ...: any
) -> absTotalHeightMapAmountChanged number?
```












### Spring.SetMapSquareTerrainType
---
```lua
function Spring.SetMapSquareTerrainType(
  x: number,
  z: number,
  newType: number
) -> oldType integer?
```












### Spring.SetTerrainTypeData
---
```lua
function Spring.SetTerrainTypeData(
  typeIndex: number,
  speedTanks: number?,
  speedKBOts: number?,
  speedHovers: number?,
  speedShips: number?
) ->  boolean?
```
@param `speedTanks` - (Default: nil)

@param `speedKBOts` - (Default: nil)

@param `speedHovers` - (Default: nil)

@param `speedShips` - (Default: nil)


@return  - true












### Spring.SetSquareBuildingMask
---
```lua
function Spring.SetSquareBuildingMask(
  x: number,
  z: number,
  mask: number
) -> See nil
```

@return `See` - also buildingMask unitdef tag.












### Spring.UnitWeaponFire
---
```lua
function Spring.UnitWeaponFire(
  unitID: integer,
  weaponID: integer
) ->  nil
```












### Spring.UnitWeaponHoldFire
---
```lua
function Spring.UnitWeaponHoldFire(
  unitID: integer,
  weaponID: integer
) ->  nil
```












### Spring.ForceUnitCollisionUpdate
---
```lua
function Spring.ForceUnitCollisionUpdate(unitID: integer) ->  nil
```





Prevent collision checks from working on outdated data

There's a rare edge case that requires units to be in specific positions
and being shot by specific weapons but which can result in shots ghosting
through the unit. This is because the unit's collision volume is stale.
The `movement.unitQuadPositionUpdateRate` modrule controls this behaviour
and can guarantee 100% correctness if set to 1, but the default value is 3
and large-scale games generally don't want to set it so low. This function
lets you guarantee success for important weapons regardless of how high
the normal update rate is set.








### Spring.UnitAttach
---
```lua
function Spring.UnitAttach(
  transporterID: integer,
  passengerID: integer,
  pieceNum: number
) ->  nil
```












### Spring.UnitDetach
---
```lua
function Spring.UnitDetach(passengerID: integer) ->  nil
```












### Spring.UnitDetachFromAir
---
```lua
function Spring.UnitDetachFromAir(passengerID: integer) ->  nil
```












### Spring.SetUnitLoadingTransport
---
```lua
function Spring.SetUnitLoadingTransport(
  passengerID: integer,
  transportID: integer
) ->  nil
```





Disables collisions between the two units to allow colvol intersection during the approach.








### Spring.SpawnProjectile
---
```lua
function Spring.SpawnProjectile(
  weaponDefID: integer,
  projectileParams: ProjectileParams
) -> projectileID integer?
```












### Spring.DeleteProjectile
---
```lua
function Spring.DeleteProjectile(projectileID: integer) ->  nil
```





Silently removes projectiles (no explosion).








### Spring.SpawnExplosion
---
```lua
function Spring.SpawnExplosion(
  posX: number?,
  posY: number?,
  posZ: number?,
  dirX: number?,
  dirY: number?,
  dirZ: number?,
  explosionParams: ExplosionParams
) ->  nil
```
@param `posX` - (Default: `0`)

@param `posY` - (Default: `0`)

@param `posZ` - (Default: `0`)

@param `dirX` - (Default: `0`)

@param `dirY` - (Default: `0`)

@param `dirZ` - (Default: `0`)













### Spring.SpawnCEG
---
```lua
function Spring.SpawnCEG(
  cegname: string,
  posX: number?,
  posY: number?,
  posZ: number?,
  dirX: number?,
  dirY: number?,
  dirZ: number?,
  radius: number?,
  damage: number?
)
 -> success boolean?
 -> cegID integer

```
@param `posX` - (Default: `0`)

@param `posY` - (Default: `0`)

@param `posZ` - (Default: `0`)

@param `dirX` - (Default: `0`)

@param `dirY` - (Default: `0`)

@param `dirZ` - (Default: `0`)

@param `radius` - (Default: `0`)

@param `damage` - (Default: `0`)













### Spring.SpawnSFX
---
```lua
function Spring.SpawnSFX(
  unitID: integer?,
  sfxID: integer?,
  posX: number?,
  posY: number?,
  posZ: number?,
  dirX: number?,
  dirY: number?,
  dirZ: number?,
  radius: number?,
  damage: number?,
  absolute: boolean?
) -> success boolean?
```
@param `unitID` - (Default: `0`)

@param `sfxID` - (Default: `0`)

@param `posX` - (Default: `0`)

@param `posY` - (Default: `0`)

@param `posZ` - (Default: `0`)

@param `dirX` - (Default: `0`)

@param `dirY` - (Default: `0`)

@param `dirZ` - (Default: `0`)

@param `radius` - (Default: `0`)

@param `damage` - (Default: `0`)






Equal to the UnitScript versions of EmitSFX, but takes position and direction arguments (in either unit- or piece-space) instead of a piece index.








### Spring.SetNoPause
---
```lua
function Spring.SetNoPause(noPause: boolean) ->  nil
```












### Spring.SetExperienceGrade
---
```lua
function Spring.SetExperienceGrade(
  expGrade: number,
  ExpPowerScale: number?,
  ExpHealthScale: number?,
  ExpReloadScale: number?
) ->  nil
```





Defines how often `Callins.UnitExperience` will be called.








### Spring.SetRadarErrorParams
---
```lua
function Spring.SetRadarErrorParams(
  allyTeamID: integer,
  allyteamErrorSize: number,
  baseErrorSize: number?,
  baseErrorMult: number?
) ->  nil
```












### Spring.EditUnitCmdDesc
---
```lua
function Spring.EditUnitCmdDesc(
  unitID: integer,
  cmdDescID: integer,
  cmdArray: CommandDescription
)
```












### Spring.InsertUnitCmdDesc
---
```lua
function Spring.InsertUnitCmdDesc(
  unitID: integer,
  index: integer,
  cmdDesc: CommandDescription
)
```





Insert a command description at a specific index.








### Spring.InsertUnitCmdDesc
---
```lua
function Spring.InsertUnitCmdDesc(
  unitID: integer,
  cmdDesc: CommandDescription
)
```





Insert a command description at the last position.








### Spring.RemoveUnitCmdDesc
---
```lua
function Spring.RemoveUnitCmdDesc(
  unitID: integer,
  cmdDescID: integer?
)
```












### Spring.IsReplay
---
```lua
function Spring.IsReplay() -> isReplay boolean?
```












### Spring.GetReplayLength
---
```lua
function Spring.GetReplayLength() -> timeInSeconds number?
```












### Spring.GetGameName
---
```lua
function Spring.GetGameName() -> name string
```












### Spring.GetMenuName
---
```lua
function Spring.GetMenuName() -> name string
```

@return `name` - name .. version from Modinfo.lua. E.g. "Spring: 1944 test-5640-ac2d15b".












### Spring.GetProfilerTimeRecord
---
```lua
function Spring.GetProfilerTimeRecord(
  profilerName: string,
  frameData: boolean?
)
 -> total number
 -> current number
 -> max_dt number
 -> time_pct number
 -> peak_pct number
 -> frameData table<number,number>?

```
@param `frameData` - (Default: `false`)


@return `total` - in ms

@return `current` - in ms

@return `frameData` - Table where key is the frame index and value is duration.












### Spring.GetProfilerRecordNames
---
```lua
function Spring.GetProfilerRecordNames() -> profilerNames string[]
```












### Spring.GetLuaMemUsage
---
```lua
function Spring.GetLuaMemUsage()
 -> luaHandleAllocedMem number
 -> luaHandleNumAllocs number
 -> luaGlobalAllocedMem number
 -> luaGlobalNumAllocs number
 -> luaUnsyncedGlobalAllocedMem number
 -> luaUnsyncedGlobalNumAllocs number
 -> luaSyncedGlobalAllocedMem number
 -> luaSyncedGlobalNumAllocs number

```

@return `luaHandleAllocedMem` - in kilobytes

@return `luaHandleNumAllocs` - divided by 1000

@return `luaGlobalAllocedMem` - in kilobytes

@return `luaGlobalNumAllocs` - divided by 1000

@return `luaUnsyncedGlobalAllocedMem` - in kilobytes

@return `luaUnsyncedGlobalNumAllocs` - divided by 1000

@return `luaSyncedGlobalAllocedMem` - in kilobytes

@return `luaSyncedGlobalNumAllocs` - divided by 1000












### Spring.GetVidMemUsage
---
```lua
function Spring.GetVidMemUsage()
 -> usedMem number
 -> availableMem number

```

@return `usedMem` - in MB

@return `availableMem` - in MB












### Spring.GetTimer
---
```lua
function Spring.GetTimer() ->  integer
```





Get a timer with millisecond resolution








### Spring.GetTimerMicros
---
```lua
function Spring.GetTimerMicros() ->  integer
```





Get a timer with microsecond resolution








### Spring.GetFrameTimer
---
```lua
function Spring.GetFrameTimer(lastFrameTime: boolean?) ->  integer
```
@param `lastFrameTime` - (Default: `false`) whether to use last frame time instead of last frame start






Get a timer for the start of the frame

This should give better results for camera interpolations








### Spring.DiffTimers
---
```lua
function Spring.DiffTimers(
  endTimer: integer,
  startTimer: integer,
  returnMs: boolean?,
  fromMicroSecs: boolean?
) -> timeAmount number
```
@param `returnMs` - (Default: `false`) whether to return `timeAmount` in milliseconds as opposed to seconds

@param `fromMicroSecs` - (Default: `false`) whether timers are in microseconds instead of milliseconds













### Spring.GetNumDisplays
---
```lua
function Spring.GetNumDisplays() -> numDisplays number
```

@return `numDisplays` - as returned by `SDL_GetNumVideoDisplays`












### Spring.GetViewGeometry
---
```lua
function Spring.GetViewGeometry()
 -> viewSizeX number
 -> viewSizeY number
 -> viewPosX number
 -> viewPosY number

```

@return `viewSizeX` - in px

@return `viewSizeY` - in px

@return `viewPosX` - offset from leftmost screen left border in px

@return `viewPosY` - offset from bottommost screen bottom border in px





Get main view geometry (map and game rendering)








### Spring.GetDualViewGeometry
---
```lua
function Spring.GetDualViewGeometry()
 -> dualViewSizeX number
 -> dualViewSizeY number
 -> dualViewPosX number
 -> dualViewPosY number

```

@return `dualViewSizeX` - in px

@return `dualViewSizeY` - in px

@return `dualViewPosX` - offset from leftmost screen left border in px

@return `dualViewPosY` - offset from bottommost screen bottom border in px





Get dual view geometry (minimap when enabled)








### Spring.GetWindowGeometry
---
```lua
function Spring.GetWindowGeometry()
 -> winSizeX number
 -> winSizeY number
 -> winPosX number
 -> winPosY number
 -> windowBorderTop number
 -> windowBorderLeft number
 -> windowBorderBottom number
 -> windowBorderRight number

```

@return `winSizeX` - in px

@return `winSizeY` - in px

@return `winPosX` - in px

@return `winPosY` - in px

@return `windowBorderTop` - in px

@return `windowBorderLeft` - in px

@return `windowBorderBottom` - in px

@return `windowBorderRight` - in px





Get main window geometry








### Spring.GetWindowDisplayMode
---
```lua
function Spring.GetWindowDisplayMode()
 -> width number
 -> height number
 -> bits number
 -> refresh number

```

@return `width` - in px

@return `height` - in px

@return `bits` - per pixel

@return `refresh` - rate in Hz





Get main window display mode








### Spring.GetScreenGeometry
---
```lua
function Spring.GetScreenGeometry(
  displayIndex: number?,
  queryUsable: boolean?
)
 -> screenSizeX number
 -> screenSizeY number
 -> screenPosX number
 -> screenPosY number
 -> windowBorderTop number
 -> windowBorderLeft number
 -> windowBorderBottom number
 -> windowBorderRight number
 -> screenUsableSizeX number?
 -> screenUsableSizeY number?
 -> screenUsablePosX number?
 -> screenUsablePosY number?

```
@param `displayIndex` - (Default: `-1`)

@param `queryUsable` - (Default: `false`)


@return `screenSizeX` - in px

@return `screenSizeY` - in px

@return `screenPosX` - in px

@return `screenPosY` - in px

@return `windowBorderTop` - in px

@return `windowBorderLeft` - in px

@return `windowBorderBottom` - in px

@return `windowBorderRight` - in px

@return `screenUsableSizeX` - in px

@return `screenUsableSizeY` - in px

@return `screenUsablePosX` - in px

@return `screenUsablePosY` - in px





Get screen geometry








### Spring.GetMiniMapGeometry
---
```lua
function Spring.GetMiniMapGeometry()
 -> minimapPosX number
 -> minimapPosY number
 -> minimapSizeX number
 -> minimapSizeY number
 -> minimized boolean
 -> maximized boolean

```

@return `minimapPosX` - in px

@return `minimapPosY` - in px

@return `minimapSizeX` - in px

@return `minimapSizeY` - in px





Get minimap geometry








### Spring.GetMiniMapRotation
---
```lua
function Spring.GetMiniMapRotation() -> amount number
```

@return `amount` - in radians





Get minimap rotation








### Spring.GetMiniMapDualScreen
---
```lua
function Spring.GetMiniMapDualScreen() -> position ("left"|"right"|false)
```

@return `position` - `"left"` or `"right"` when dual screen is enabled, otherwise `false`.












### Spring.GetSelectionBox
---
```lua
function Spring.GetSelectionBox()
 -> left number?
 -> top number?
 -> right number?
 -> bottom number?

```





Get vertices from currently active selection box

Returns nil when selection box is inactive





@see Spring.GetUnitsInScreenRectangle




### Spring.GetDrawSelectionInfo
---
```lua
function Spring.GetDrawSelectionInfo() ->  boolean
```












### Spring.IsAboveMiniMap
---
```lua
function Spring.IsAboveMiniMap(
  x: number,
  y: number
) -> isAbove boolean
```












### Spring.GetDrawFrame
---
```lua
function Spring.GetDrawFrame()
 -> low_16bit number
 -> high_16bit number

```












### Spring.GetFrameTimeOffset
---
```lua
function Spring.GetFrameTimeOffset() -> offset number?
```

@return `offset` - of the current draw frame from the last sim frame, expressed in fractions of a frame





Ideally, when running 30hz sim, and 60hz rendering, the draw frames should
have and offset of either 0.0 frames, or 0.5 frames.

When draw frames are not integer multiples of sim frames, some interpolation
happens, and this timeoffset shows how far along it is.








### Spring.GetLastUpdateSeconds
---
```lua
function Spring.GetLastUpdateSeconds() -> lastUpdateSeconds number?
```












### Spring.GetVideoCapturingMode
---
```lua
function Spring.GetVideoCapturingMode() -> allowRecord boolean
```












### Spring.IsUnitAllied
---
```lua
function Spring.IsUnitAllied(unitID: integer) -> isAllied boolean?
```

@return `isAllied` - nil with unitID cannot be parsed












### Spring.IsUnitSelected
---
```lua
function Spring.IsUnitSelected(unitID: integer) -> isSelected boolean?
```

@return `isSelected` - nil when unitID cannot be parsed












### Spring.GetUnitLuaDraw
---
```lua
function Spring.GetUnitLuaDraw(unitID: integer) -> draw boolean?
```

@return `draw` - nil when unitID cannot be parsed












### Spring.GetUnitNoDraw
---
```lua
function Spring.GetUnitNoDraw(unitID: integer) -> nil boolean?
```

@return `nil` - when unitID cannot be parsed












### Spring.GetUnitEngineDrawMask
---
```lua
function Spring.GetUnitEngineDrawMask(unitID: integer) -> nil boolean?
```

@return `nil` - when unitID cannot be parsed












### Spring.GetUnitAlwaysUpdateMatrix
---
```lua
function Spring.GetUnitAlwaysUpdateMatrix(unitID: integer) -> nil boolean?
```

@return `nil` - when unitID cannot be parsed












### Spring.GetUnitDrawFlag
---
```lua
function Spring.GetUnitDrawFlag(unitID: integer) -> nil number?
```

@return `nil` - when unitID cannot be parsed












### Spring.GetUnitNoMinimap
---
```lua
function Spring.GetUnitNoMinimap(unitID: integer) -> nil boolean?
```

@return `nil` - when unitID cannot be parsed












### Spring.GetUnitNoGroup
---
```lua
function Spring.GetUnitNoGroup(unitID: integer) -> noGroup boolean?
```

@return `noGroup` - `true` if the unit is not allowed to be added to a group, `false` if it is allowed to be added to a group, or `nil` when `unitID` is not valid.





Check if a unit is not allowed to be added to a group by a player.








### Spring.GetUnitNoSelect
---
```lua
function Spring.GetUnitNoSelect(unitID: integer) -> noSelect boolean?
```

@return `noSelect` - `nil` when `unitID` cannot be parsed.












### Spring.UnitIconGetDraw
---
```lua
function Spring.UnitIconGetDraw(unitID: integer) -> drawIcon boolean?
```

@return `drawIcon` - `true` if icon is being drawn, `nil` when unitID is invalid, otherwise `false`.












### Spring.GetUnitSelectionVolumeData
---
```lua
function Spring.GetUnitSelectionVolumeData(unitID: integer)
 -> scaleX number?
 -> scaleY number
 -> scaleZ number
 -> offsetX number
 -> offsetY number
 -> offsetZ number
 -> volumeType number
 -> useContHitTest number
 -> getPrimaryAxis number
 -> ignoreHits boolean

```

@return `scaleX` - nil when unitID cannot be parsed












### Spring.GetFeatureLuaDraw
---
```lua
function Spring.GetFeatureLuaDraw(featureID: integer) -> nil boolean?
```

@return `nil` - when featureID cannot be parsed












### Spring.GetFeatureNoDraw
---
```lua
function Spring.GetFeatureNoDraw(featureID: integer) -> nil boolean?
```

@return `nil` - when featureID cannot be parsed












### Spring.GetFeatureEngineDrawMask
---
```lua
function Spring.GetFeatureEngineDrawMask(featureID: integer) -> nil boolean?
```

@return `nil` - when featureID cannot be parsed












### Spring.GetFeatureAlwaysUpdateMatrix
---
```lua
function Spring.GetFeatureAlwaysUpdateMatrix(featureID: integer) -> nil boolean?
```

@return `nil` - when featureID cannot be parsed












### Spring.GetFeatureDrawFlag
---
```lua
function Spring.GetFeatureDrawFlag(featureID: integer) -> nil number?
```

@return `nil` - when featureID cannot be parsed












### Spring.GetFeatureSelectionVolumeData
---
```lua
function Spring.GetFeatureSelectionVolumeData(featureID: integer)
 -> scaleX number?
 -> scaleY number
 -> scaleZ number
 -> offsetX number
 -> offsetY number
 -> offsetZ number
 -> volumeType number
 -> useContHitTest number
 -> getPrimaryAxis number
 -> ignoreHits boolean

```

@return `scaleX` - nil when unitID cannot be parsed












### Spring.GetUnitTransformMatrix
---
```lua
function Spring.GetUnitTransformMatrix(unitID: integer)
 -> m11 number?
 -> m12 number
 -> m13 number
 -> m14 number
 -> m21 number
 -> m22 number
 -> m23 number
 -> m24 number
 -> m31 number
 -> m32 number
 -> m33 number
 -> m34 number
 -> m41 number
 -> m42 number
 -> m43 number
 -> m44 number

```

@return `m11` - nil when unitID cannot be parsed












### Spring.GetFeatureTransformMatrix
---
```lua
function Spring.GetFeatureTransformMatrix(featureID: integer)
 -> m11 number?
 -> m12 number
 -> m13 number
 -> m14 number
 -> m21 number
 -> m22 number
 -> m23 number
 -> m24 number
 -> m31 number
 -> m32 number
 -> m33 number
 -> m34 number
 -> m41 number
 -> m42 number
 -> m43 number
 -> m44 number

```

@return `m11` - nil when featureID cannot be parsed












### Spring.IsUnitInView
---
```lua
function Spring.IsUnitInView(unitID: integer) -> inView boolean?
```

@return `inView` - nil when unitID cannot be parsed












### Spring.IsUnitVisible
---
```lua
function Spring.IsUnitVisible(
  unitID: integer,
  radius: number?,
  checkIcon: boolean
) -> isVisible boolean?
```
@param `radius` - unitRadius when not specified


@return `isVisible` - nil when unitID cannot be parsed












### Spring.IsUnitIcon
---
```lua
function Spring.IsUnitIcon(unitID: integer) -> isUnitIcon boolean?
```

@return `isUnitIcon` - nil when unitID cannot be parsed












### Spring.IsAABBInView
---
```lua
function Spring.IsAABBInView(
  minX: number,
  minY: number,
  minZ: number,
  maxX: number,
  maxY: number,
  maxZ: number
) -> inView boolean
```












### Spring.IsSphereInView
---
```lua
function Spring.IsSphereInView(
  posX: number,
  posY: number,
  posZ: number,
  radius: number?
) -> inView boolean
```
@param `radius` - (Default: `0`)













### Spring.GetUnitViewPosition
---
```lua
function Spring.GetUnitViewPosition(
  unitID: integer,
  midPos: boolean?
)
 -> x number?
 -> y number
 -> z number

```
@param `midPos` - (Default: `false`)


@return `x` - nil when unitID cannot be parsed












### Spring.GetVisibleUnits
---
```lua
function Spring.GetVisibleUnits(
  teamID: integer?,
  radius: number?,
  icons: boolean?
) -> unitIDs number[]?
```
@param `teamID` - (Default: `-1`)

@param `radius` - (Default: `30`)

@param `icons` - (Default: `true`)













### Spring.GetVisibleFeatures
---
```lua
function Spring.GetVisibleFeatures(
  teamID: integer?,
  radius: number?,
  icons: boolean?,
  geos: boolean?
) -> featureIDs number[]?
```
@param `teamID` - (Default: `-1`)

@param `radius` - (Default: `30`)

@param `icons` - (Default: `true`)

@param `geos` - (Default: `true`)













### Spring.GetVisibleProjectiles
---
```lua
function Spring.GetVisibleProjectiles(
  allyTeamID: integer?,
  addSyncedProjectiles: boolean?,
  addWeaponProjectiles: boolean?,
  addPieceProjectiles: boolean?
) -> projectileIDs number[]?
```
@param `allyTeamID` - (Default: `-1`)

@param `addSyncedProjectiles` - (Default: `true`)

@param `addWeaponProjectiles` - (Default: `true`)

@param `addPieceProjectiles` - (Default: `true`)













### Spring.GetRenderUnits
---
```lua
function Spring.GetRenderUnits()
```












### Spring.GetRenderUnitsDrawFlagChanged
---
```lua
function Spring.GetRenderUnitsDrawFlagChanged()
```












### Spring.GetRenderFeatures
---
```lua
function Spring.GetRenderFeatures()
```












### Spring.GetRenderFeaturesDrawFlagChanged
---
```lua
function Spring.GetRenderFeaturesDrawFlagChanged()
```












### Spring.ClearUnitsPreviousDrawFlag
---
```lua
function Spring.ClearUnitsPreviousDrawFlag() ->  nil
```












### Spring.ClearFeaturesPreviousDrawFlag
---
```lua
function Spring.ClearFeaturesPreviousDrawFlag() ->  nil
```












### Spring.GetUnitsInScreenRectangle
---
```lua
function Spring.GetUnitsInScreenRectangle(
  left: number,
  top: number,
  right: number,
  bottom: number,
  allegiance: number?
) -> unitIDs number[]?
```
@param `allegiance` - (Default: `-1`) teamID when > 0, when < 0 one of AllUnits = -1, MyUnits = -2, AllyUnits = -3, EnemyUnits = -4






Get units inside a rectangle area on the map








### Spring.GetFeaturesInScreenRectangle
---
```lua
function Spring.GetFeaturesInScreenRectangle(
  left: number,
  top: number,
  right: number,
  bottom: number
) -> featureIDs number[]?
```





Get features inside a rectangle area on the map








### Spring.GetLocalPlayerID
---
```lua
function Spring.GetLocalPlayerID() -> playerID integer
```












### Spring.GetLocalTeamID
---
```lua
function Spring.GetLocalTeamID() -> teamID integer
```












### Spring.GetLocalAllyTeamID
---
```lua
function Spring.GetLocalAllyTeamID() -> allyTeamID integer
```












### Spring.GetSpectatingState
---
```lua
function Spring.GetSpectatingState()
 -> spectating boolean
 -> spectatingFullView boolean
 -> spectatingFullSelect boolean

```












### Spring.GetSelectedUnits
---
```lua
function Spring.GetSelectedUnits() -> unitIDs number[]
```












### Spring.GetSelectedUnitsSorted
---
```lua
function Spring.GetSelectedUnitsSorted()
 -> where table<number,number[]>
 -> the integer

```

@return `where` - keys are unitDefIDs and values are unitIDs

@return `the` - number of unitDefIDs





Get selected units aggregated by unitDefID








### Spring.GetSelectedUnitsCounts
---
```lua
function Spring.GetSelectedUnitsCounts()
 -> unitsCounts table<number,number>
 -> the integer

```

@return `unitsCounts` - where keys are unitDefIDs and values are counts

@return `the` - number of unitDefIDs





Get an aggregate count of selected units per unitDefID








### Spring.GetSelectedUnitsCount
---
```lua
function Spring.GetSelectedUnitsCount() -> selectedUnitsCount number
```





Returns the amount of selected units








### Spring.GetBoxSelectionByEngine
---
```lua
function Spring.GetBoxSelectionByEngine() -> isHandledByEngine boolean
```

@return `isHandledByEngine` - `true` if the engine will select units inside selection box on release, otherwise `false`.





Get if selection box is handled by engine.





@see Spring.SetBoxSelectionByEngine




### Spring.IsGUIHidden
---
```lua
function Spring.IsGUIHidden() ->  boolean
```












### Spring.HaveShadows
---
```lua
function Spring.HaveShadows() -> shadowsLoaded boolean
```












### Spring.HaveAdvShading
---
```lua
function Spring.HaveAdvShading()
 -> useAdvShading boolean
 -> groundUseAdvShading boolean

```












### Spring.GetWaterMode
---
```lua
function Spring.GetWaterMode()
 -> waterRendererID integer
 -> waterRendererName string

```









@see rts/Rendering/Env/IWater.h




### Spring.GetMapDrawMode
---
```lua
function Spring.GetMapDrawMode() ->  ("normal"|"height"|"metal"|"pathTraversability"|"los")
```












### Spring.GetMapSquareTexture
---
```lua
function Spring.GetMapSquareTexture(
  texSquareX: number,
  texSquareY: number,
  lodMin: number,
  luaTexName: string,
  lodMax: number?
) -> success boolean?
```
@param `lodMax` - (Default: lodMin)













### Spring.GetLosViewColors
---
```lua
function Spring.GetLosViewColors()
 -> always rgb
 -> LOS rgb
 -> radar rgb
 -> jam rgb
 -> radar2 rgb

```












### Spring.GetNanoProjectileParams
---
```lua
function Spring.GetNanoProjectileParams()
 -> rotVal number
 -> rotVel number
 -> rotAcc number
 -> rotValRng number
 -> rotVelRng number
 -> rotAccRng number

```

@return `rotVal` - in degrees

@return `rotVel` - in degrees

@return `rotAcc` - in degrees

@return `rotValRng` - in degrees

@return `rotVelRng` - in degrees

@return `rotAccRng` - in degrees












### Spring.GetCameraNames
---
```lua
function Spring.GetCameraNames() -> indexByName table<string,integer>
```

@return `indexByName` - Table where where keys are names and values are indices.





Get available cameras.








### Spring.GetCameraState
---
```lua
function Spring.GetCameraState(useReturns: false) -> cameraState CameraState
```












### Spring.GetCameraState
---
```lua
function Spring.GetCameraState(useReturns: true?)
 -> name CameraName
 -> Fields any

```
@param `useReturns` - (Default: `true`) Return multiple values instead of a table.


@return `Fields` - depending on current controller mode.












### Spring.GetCameraPosition
---
```lua
function Spring.GetCameraPosition()
 -> posX number
 -> posY number
 -> posZ number

```












### Spring.GetCameraDirection
---
```lua
function Spring.GetCameraDirection()
 -> dirX number
 -> dirY number
 -> dirZ number

```












### Spring.GetCameraRotation
---
```lua
function Spring.GetCameraRotation()
 -> rotX number
 -> rotY number
 -> rotZ number

```

@return `rotX` - Rotation around X axis in radians.

@return `rotY` - Rotation around Y axis in radians.

@return `rotZ` - Rotation around Z axis in radians.





Get camera rotation in radians.








### Spring.GetCameraFOV
---
```lua
function Spring.GetCameraFOV()
 -> vFOV number
 -> hFOV number

```












### Spring.GetCameraVectors
---
```lua
function Spring.GetCameraVectors() ->  CameraVectors
```












### Spring.WorldToScreenCoords
---
```lua
function Spring.WorldToScreenCoords(
  x: number,
  y: number,
  z: number
)
 -> viewPortX number
 -> viewPortY number
 -> viewPortZ number

```












### Spring.TraceScreenRay
---
```lua
function Spring.TraceScreenRay(
  screenX: number,
  screenY: number,
  onlyCoords: boolean?,
  useMinimap: boolean?,
  includeSky: boolean?,
  ignoreWater: boolean?,
  heightOffset: number?
)
 -> description string?
 -> unitID (number|string|xyz)?
 -> featureID (number|string)?
 -> coords xyz?

```
@param `screenX` - position on x axis in mouse coordinates (origin on left border of view)

@param `screenY` - position on y axis in mouse coordinates (origin on top border of view)

@param `onlyCoords` - (Default: `false`) return only description (1st return value) and coordinates (2nd return value)

@param `useMinimap` - (Default: `false`) if position arguments are contained by minimap, use the minimap corresponding world position

@param `includeSky` - (Default: `false`)

@param `ignoreWater` - (Default: `false`)

@param `heightOffset` - (Default: `0`)


@return `description` - of traced position

@return `unitID` - or feature, position triple when onlyCoords=true

@return `featureID` - or ground





Get information about a ray traced from screen to world position

Extended to allow a custom plane, parameters are (0, 1, 0, D=0) where D is the offset D can be specified in the third argument (if all the bools are false) or in the seventh (as shown).

Intersection coordinates are returned in t[4],t[5],t[6] when the ray goes offmap and includeSky is true), or when no unit or feature is hit (or onlyCoords is true).

This will only work for units & objects with the default collision sphere. Per Piece collision and custom collision objects are not supported.

The unit must be selectable, to appear to a screen trace ray.








### Spring.GetPixelDir
---
```lua
function Spring.GetPixelDir(
  x: number,
  y: number
)
 -> dirX number
 -> dirY number
 -> dirZ number

```












### Spring.GetTeamColor
---
```lua
function Spring.GetTeamColor(teamID: integer)
 -> r number?
 -> g number?
 -> b number?
 -> a number?

```

@return `r` - factor from 0 to 1

@return `g` - factor from 0 to 1

@return `b` - factor from 0 to 1

@return `a` - factor from 0 to 1












### Spring.GetTeamOrigColor
---
```lua
function Spring.GetTeamOrigColor(teamID: integer)
 -> r number?
 -> g number?
 -> b number?
 -> a number?

```

@return `r` - factor from 0 to 1

@return `g` - factor from 0 to 1

@return `b` - factor from 0 to 1

@return `a` - factor from 0 to 1












### Spring.GetDrawSeconds
---
```lua
function Spring.GetDrawSeconds() -> time integer
```

@return `time` - Time in seconds.












### Spring.GetSoundDevices
---
```lua
function Spring.GetSoundDevices() -> devices SoundDeviceSpec[]
```

@return `devices` - Sound devices.












### Spring.GetSoundStreamTime
---
```lua
function Spring.GetSoundStreamTime()
 -> playTime number
 -> time number

```












### Spring.GetSoundEffectParams
---
```lua
function Spring.GetSoundEffectParams()
```












### Spring.GetFPS
---
```lua
function Spring.GetFPS() -> fps number
```












### Spring.GetGameSpeed
---
```lua
function Spring.GetGameSpeed()
 -> wantedSpeedFactor number
 -> speedFactor number
 -> paused boolean

```












### Spring.GetGameState
---
```lua
function Spring.GetGameState(maxLatency: number?)
 -> doneLoading boolean
 -> isSavedGame boolean
 -> isClientPaused boolean
 -> isSimLagging boolean

```
@param `maxLatency` - (Default: `500`) used for `isSimLagging` return parameter













### Spring.GetActiveCommand
---
```lua
function Spring.GetActiveCommand()
 -> cmdIndex number?
 -> cmdID integer?
 -> cmdType number?
 -> cmdName string?

```












### Spring.GetDefaultCommand
---
```lua
function Spring.GetDefaultCommand()
 -> cmdIndex integer?
 -> cmdID integer?
 -> cmdType integer?
 -> cmdName string?

```












### Spring.GetActiveCmdDescs
---
```lua
function Spring.GetActiveCmdDescs() -> cmdDescs CommandDescription[]
```












### Spring.GetActiveCmdDesc
---
```lua
function Spring.GetActiveCmdDesc(cmdIndex: integer) ->  CommandDescription?
```












### Spring.GetCmdDescIndex
---
```lua
function Spring.GetCmdDescIndex(cmdID: integer) -> cmdDescIndex integer?
```












### Spring.GetBuildFacing
---
```lua
function Spring.GetBuildFacing() -> buildFacing FacingInteger
```












### Spring.GetBuildSpacing
---
```lua
function Spring.GetBuildSpacing() -> buildSpacing number
```












### Spring.GetGatherMode
---
```lua
function Spring.GetGatherMode() -> gatherMode number
```












### Spring.GetActivePage
---
```lua
function Spring.GetActivePage()
 -> activePage number
 -> maxPage number

```












### Spring.GetMouseState
---
```lua
function Spring.GetMouseState()
 -> x number
 -> y number
 -> lmbPressed number
 -> mmbPressed number
 -> rmbPressed number
 -> offscreen boolean
 -> mmbScroll boolean

```

@return `lmbPressed` - left mouse button pressed

@return `mmbPressed` - middle mouse button pressed

@return `rmbPressed` - right mouse button pressed












### Spring.GetMouseCursor
---
```lua
function Spring.GetMouseCursor()
 -> cursorName string
 -> cursorScale number

```












### Spring.GetMouseStartPosition
---
```lua
function Spring.GetMouseStartPosition(button: number)
 -> x number
 -> y number
 -> camPosX number
 -> camPosY number
 -> camPosZ number
 -> dirX number
 -> dirY number
 -> dirZ number

```












### Spring.GetClipboard
---
```lua
function Spring.GetClipboard() -> text string
```












### Spring.IsUserWriting
---
```lua
function Spring.IsUserWriting() ->  boolean
```












### Spring.GetLastMessagePositions
---
```lua
function Spring.GetLastMessagePositions() -> message xyz[]
```

@return `message` - positions












### Spring.GetConsoleBuffer
---
```lua
function Spring.GetConsoleBuffer(maxLines: number) -> buffer { text: string,priority: integer }[]
```












### Spring.GetCurrentTooltip
---
```lua
function Spring.GetCurrentTooltip() -> tooltip string
```












### Spring.GetKeyFromScanSymbol
---
```lua
function Spring.GetKeyFromScanSymbol(scanSymbol: string) -> keyName string
```












### Spring.GetKeyState
---
```lua
function Spring.GetKeyState(keyCode: number) -> pressed boolean
```












### Spring.GetModKeyState
---
```lua
function Spring.GetModKeyState()
 -> alt boolean
 -> ctrl boolean
 -> meta boolean
 -> shift boolean

```












### Spring.GetPressedKeys
---
```lua
function Spring.GetPressedKeys() -> where table<(number|string),true>
```

@return `where` - keys are keyCodes or key names












### Spring.GetPressedScans
---
```lua
function Spring.GetPressedScans() -> where table<(number|string),true>
```

@return `where` - keys are scanCodes or scan names












### Spring.GetInvertQueueKey
---
```lua
function Spring.GetInvertQueueKey() -> queueKey number?
```












### Spring.GetKeyCode
---
```lua
function Spring.GetKeyCode(keySym: string) -> keyCode number
```












### Spring.GetKeySymbol
---
```lua
function Spring.GetKeySymbol(keyCode: number)
 -> keyCodeName string
 -> keyCodeDefaultName string

```

@return `keyCodeDefaultName` - name when there are not aliases












### Spring.GetScanSymbol
---
```lua
function Spring.GetScanSymbol(scanCode: number)
 -> scanCodeName string
 -> scanCodeDefaultName string

```

@return `scanCodeDefaultName` - name when there are not aliases












### Spring.GetKeyBindings
---
```lua
function Spring.GetKeyBindings(
  keySet1: string?,
  keySet2: string?
) ->  KeyBinding[]
```
@param `keySet1` - filters keybindings bound to this keyset

@param `keySet2` - OR bound to this keyset













### Spring.GetActionHotKeys
---
```lua
function Spring.GetActionHotKeys(actionName: string) -> hotkeys string[]?
```












### Spring.GetGroupList
---
```lua
function Spring.GetGroupList() -> where table<number,number>?
```

@return `where` - keys are groupIDs and values are counts












### Spring.GetSelectedGroup
---
```lua
function Spring.GetSelectedGroup() -> groupID integer
```












### Spring.GetUnitGroup
---
```lua
function Spring.GetUnitGroup(unitID: integer) -> groupID integer?
```












### Spring.GetGroupUnits
---
```lua
function Spring.GetGroupUnits(groupID: integer) -> unitIDs number[]?
```












### Spring.GetGroupUnitsSorted
---
```lua
function Spring.GetGroupUnitsSorted(groupID: integer) -> where table<number,number[]>?
```

@return `where` - keys are unitDefIDs and values are unitIDs












### Spring.GetGroupUnitsCounts
---
```lua
function Spring.GetGroupUnitsCounts(groupID: integer) -> where table<number,number>?
```

@return `where` - keys are unitDefIDs and values are counts












### Spring.GetGroupUnitsCount
---
```lua
function Spring.GetGroupUnitsCount(groupID: integer) -> groupSize number?
```












### Spring.GetPlayerRoster
---
```lua
function Spring.GetPlayerRoster(
  sortType: number?,
  showPathingPlayers: boolean?
) -> playerTable Roster[]?
```
@param `sortType` - return unsorted if unspecified. Disabled = 0, Allies = 1, TeamID = 2, PlayerName = 3, PlayerCPU = 4, PlayerPing = 5

@param `showPathingPlayers` - (Default: `false`)













### Spring.GetPlayerTraffic
---
```lua
function Spring.GetPlayerTraffic(
  playerID: integer,
  packetID: integer?
) -> traffic number
```












### Spring.GetPlayerStatistics
---
```lua
function Spring.GetPlayerStatistics(playerID: integer)
 -> mousePixels number?
 -> mouseClicks number
 -> keyPresses number
 -> numCommands number
 -> unitCommands number

```

@return `mousePixels` - nil when invalid playerID












### Spring.GetConfigParams
---
```lua
function Spring.GetConfigParams() ->  Configuration[]
```












### Spring.GetConfigInt
---
```lua
function Spring.GetConfigInt(
  name: string,
  default: number?
) -> configInt number?
```
@param `default` - (Default: `0`)













### Spring.GetConfigFloat
---
```lua
function Spring.GetConfigFloat(
  name: string,
  default: number?
) -> configFloat number?
```
@param `default` - (Default: `0`)













### Spring.GetConfigString
---
```lua
function Spring.GetConfigString(
  name: string,
  default: string?
) -> configString number?
```
@param `default` - (Default: `""`)













### Spring.GetLogSections
---
```lua
function Spring.GetLogSections() -> sections table<string,number>
```

@return `sections` - where keys are names and loglevel are values. E.g. `{ "KeyBindings" = LOG.INFO, "Font" = LOG.INFO, "Sound" = LOG.WARNING, ... }`












### Spring.GetAllGroundDecals
---
```lua
function Spring.GetAllGroundDecals() -> decalIDs number[]
```












### Spring.GetGroundDecalMiddlePos
---
```lua
function Spring.GetGroundDecalMiddlePos(decalID: integer)
 -> posX number?
 -> posZ number

```












### Spring.GetDecalQuadPos
---
```lua
function Spring.GetDecalQuadPos(decalID: integer)
 -> posTL.x number?
 -> posTL.z number
 -> posTR.x number
 -> posTR.z number
 -> posBR.x number
 -> posBR.z number
 -> posBL.x number
 -> posBL.z number

```












### Spring.GetGroundDecalSizeAndHeight
---
```lua
function Spring.GetGroundDecalSizeAndHeight(decalID: integer)
 -> sizeX number?
 -> sizeY number
 -> projCubeHeight number

```












### Spring.GetGroundDecalRotation
---
```lua
function Spring.GetGroundDecalRotation(decalID: integer) -> rotation number?
```

@return `rotation` - Rotation in radians.












### Spring.GetGroundDecalTexture
---
```lua
function Spring.GetGroundDecalTexture(
  decalID: integer,
  isMainTex: boolean?
) -> texture string?
```
@param `isMainTex` - (Default: `true`) If `false`, return the normal/glow map.













### Spring.GetDecalTextures
---
```lua
function Spring.GetDecalTextures(isMainTex: boolean?) -> textureNames string[]
```
@param `isMainTex` - (Default: `true`) If `false`, return the texture for normal/glow maps.


@return `textureNames` - All textures on the atlas and available for use in `SetGroundDecalTexture`.









@see Spring.GetGroundDecalTexture




### Spring.SetGroundDecalTextureParams
---
```lua
function Spring.SetGroundDecalTextureParams(decalID: integer)
 -> texWrapDistance number?
 -> texTraveledDistance number

```

@return `texWrapDistance` - If non-zero, sets the mode to repeat the texture along the left-right direction of the decal every texWrapFactor elmos.

@return `texTraveledDistance` - Shifts the texture repetition defined by texWrapFactor so the texture of a next line in the continuous multiline can start where the previous finished. For that it should collect all elmo lengths of the previously set multiline segments.












### Spring.GetGroundDecalAlpha
---
```lua
function Spring.GetGroundDecalAlpha(decalID: integer)
 -> alpha number?
 -> alphaFalloff number

```

@return `alpha` - Between 0 and 1

@return `alphaFalloff` - Between 0 and 1, per second












### Spring.GetGroundDecalNormal
---
```lua
function Spring.GetGroundDecalNormal(decalID: integer)
 -> normal.x number?
 -> normal.y number
 -> normal.z number

```





If all three equal 0, the decal follows the normals of ground at midpoint








### Spring.GetGroundDecalTint
---
```lua
function Spring.GetGroundDecalTint(decalID: integer)
 -> tintR number?
 -> tintG number
 -> tintB number
 -> tintA number

```





Gets the tint of the ground decal.
A color of (0.5, 0.5, 0.5, 0.5) is effectively no tint








### Spring.GetGroundDecalMisc
---
```lua
function Spring.GetGroundDecalMisc(decalID: integer)
 -> dotElimExp number?
 -> refHeight number
 -> minHeight number
 -> maxHeight number
 -> forceHeightMode number

```





Returns less important parameters of a ground decal








### Spring.GetGroundDecalCreationFrame
---
```lua
function Spring.GetGroundDecalCreationFrame(decalID: integer)
 -> creationFrameMin number?
 -> creationFrameMax number

```





Min can be not equal to max for "gradient" style decals, e.g. unit tracks








### Spring.GetGroundDecalOwner
---
```lua
function Spring.GetGroundDecalOwner(decalID: integer) -> value integer?
```

@return `value` - If owner is a unit, then this is `unitID`, if owner is
a feature it is `featureID + MAX_UNITS`. If there is no owner, then `nil`.












### Spring.GetGroundDecalType
---
```lua
function Spring.GetGroundDecalType(decalID: integer) -> type string?
```

@return `type` - "explosion"|"plate"|"lua"|"track"|"unknown"












### Spring.GetSyncedGCInfo
---
```lua
function Spring.GetSyncedGCInfo(collectGC: boolean?) -> GC number?
```
@param `collectGC` - (Default: `false`) collect before returning metric


@return `GC` - values are expressed in Kbytes: #bytes/2^10












### Spring.SolveNURBSCurve
---
```lua
function Spring.SolveNURBSCurve(groupID: integer) -> unitIDs number[]?
```












### Spring.Ping
---
```lua
function Spring.Ping(pingTag: number) ->  nil
```





Send a ping request to the server








### Spring.SendCommands
---
```lua
function Spring.SendCommands(commands: string[])
```












### Spring.SendCommands
---
```lua
function Spring.SendCommands(
  command: string,
  ...: string
) ->  nil
```
@param `...` - additional commands













### Spring.SendMessage
---
```lua
function Spring.SendMessage(message: string) ->  nil
```












### Spring.SendMessageToSpectators
---
```lua
function Spring.SendMessageToSpectators(message: string) ->  nil
```
@param `message` - ``"`<PLAYER#>`"`` where `#` is a player ID.

This will be replaced with the player's name. e.g.
```lua
Spring.SendMessage("`<PLAYER1>` did something") -- "ProRusher did something"
```













### Spring.SendMessageToPlayer
---
```lua
function Spring.SendMessageToPlayer(
  playerID: integer,
  message: string
) ->  nil
```












### Spring.SendMessageToTeam
---
```lua
function Spring.SendMessageToTeam(
  teamID: integer,
  message: string
) ->  nil
```












### Spring.SendMessageToAllyTeam
---
```lua
function Spring.SendMessageToAllyTeam(
  allyID: integer,
  message: string
) ->  nil
```












### Spring.LoadSoundDef
---
```lua
function Spring.LoadSoundDef(soundfile: string) -> success boolean
```





Loads a SoundDefs file, the format is the same as in `gamedata/sounds.lua`.








### Spring.PlaySoundFile
---
```lua
function Spring.PlaySoundFile(
  soundfile: string,
  volume: number?,
  posx: number?,
  posy: number?,
  posz: number?,
  speedx: number?,
  speedy: number?,
  speedz: number?,
  channel: SoundChannel?
) -> playSound boolean
```
@param `volume` - (Default: 1.0)

@param `channel` - (Default: `0|"general"`)













### Spring.PlaySoundStream
---
```lua
function Spring.PlaySoundStream(
  oggfile: string,
  volume: number?,
  enqueue: boolean?
) -> success boolean
```
@param `volume` - (Default: 1.0)






Allows to play an Ogg Vorbis (.OGG) and mp3 compressed sound file.

Multiple sound streams may be played at once.








### Spring.StopSoundStream
---
```lua
function Spring.StopSoundStream() ->  nil
```





Terminates any SoundStream currently running.








### Spring.PauseSoundStream
---
```lua
function Spring.PauseSoundStream() ->  nil
```





Pause any SoundStream currently running.








### Spring.SetSoundStreamVolume
---
```lua
function Spring.SetSoundStreamVolume(volume: number) ->  nil
```





Set volume for SoundStream








### Spring.SetSoundEffectParams
---
```lua
function Spring.SetSoundEffectParams()
```












### Spring.AddWorldIcon
---
```lua
function Spring.AddWorldIcon(
  cmdID: integer,
  posX: number,
  posY: number,
  posZ: number
) ->  nil
```












### Spring.AddWorldText
---
```lua
function Spring.AddWorldText(
  text: string,
  posX: number,
  posY: number,
  posZ: number
) ->  nil
```












### Spring.AddWorldUnit
---
```lua
function Spring.AddWorldUnit(
  unitDefID: integer,
  posX: number,
  posY: number,
  posZ: number,
  teamID: integer,
  facing: FacingInteger
) ->  nil
```












### Spring.DrawUnitCommands
---
```lua
function Spring.DrawUnitCommands(unitID: integer)
```












### Spring.DrawUnitCommands
---
```lua
function Spring.DrawUnitCommands(
  unitIDs: integer[],
  tableOrArray: false?
)
```
@param `unitIDs` - Unit ids.

@param `tableOrArray` - Set to `true` if the unit IDs should be read from the keys of `unitIDs`.













### Spring.DrawUnitCommands
---
```lua
function Spring.DrawUnitCommands(
  unitIDs: table<integer,any>,
  tableOrArray: true
) ->  nil
```
@param `unitIDs` - Table with unit IDs as keys.

@param `tableOrArray` - Set to `false` if the unit IDs should be read from the values of `unitIDs`.













### Spring.SetCameraTarget
---
```lua
function Spring.SetCameraTarget(
  x: number,
  y: number,
  z: number,
  transTime: number?
) ->  nil
```





For Spring Engine XZ represents horizontal, from north west corner of map and Y vertical, from water level and rising.








### Spring.SetCameraOffset
---
```lua
function Spring.SetCameraOffset(
  posX: number?,
  posY: number?,
  posZ: number?,
  tiltX: number?,
  tiltY: number?,
  tiltZ: number?
) ->  nil
```
@param `posX` - (Default: `0`)

@param `posY` - (Default: `0`)

@param `posZ` - (Default: `0`)

@param `tiltX` - (Default: `0`)

@param `tiltY` - (Default: `0`)

@param `tiltZ` - (Default: `0`)













### Spring.SetCameraState
---
```lua
function Spring.SetCameraState(
  cameraState: CameraState,
  transitionTime: number?,
  transitionTimeFactor: number?,
  transitionTimeExponent: number?
) -> set boolean
```
@param `cameraState` - The fields must be consistent with the name/mode and current/new camera mode.

@param `transitionTime` - (Default: `0`) in nanoseconds

@param `transitionTimeFactor` - Multiplicative factor applied to this and all subsequent transition times for
this camera mode.

Defaults to "CamTimeFactor" springsetting unless set previously.

@param `transitionTimeExponent` - Tween factor applied to this and all subsequent transitions for this camera
mode.

Defaults to "CamTimeExponent" springsetting unless set previously.


@return `set` - `true` when applied without errors, otherwise `false`.





Set camera state.








### Spring.RunDollyCamera
---
```lua
function Spring.RunDollyCamera(runtime: number) ->  nil
```
@param `runtime` - Runtime in milliseconds.






Runs Dolly Camera








### Spring.PauseDollyCamera
---
```lua
function Spring.PauseDollyCamera(fraction: number) ->  nil
```
@param `fraction` - Fraction of the total runtime to pause at, 0 to 1 inclusive. A null value pauses at current percent






Pause Dolly Camera








### Spring.ResumeDollyCamera
---
```lua
function Spring.ResumeDollyCamera() ->  nil
```





Resume Dolly Camera








### Spring.SetDollyCameraPosition
---
```lua
function Spring.SetDollyCameraPosition(
  x: number,
  y: number,
  z: number
) ->  nil
```





Sets Dolly Camera Position








### Spring.SetDollyCameraCurve
---
```lua
function Spring.SetDollyCameraCurve(
  degree: number,
  cpoints: ControlPoint[],
  knots: table
) ->  nil
```
@param `cpoints` - NURBS control point positions.






Sets Dolly Camera movement Curve








### Spring.SetDollyCameraMode
---
```lua
function Spring.SetDollyCameraMode(mode: (1|2)) ->  nil
```
@param `mode` - `1` static position, `2` nurbs curve






Sets Dolly Camera movement mode








### Spring.SetDollyCameraRelativeMode
---
```lua
function Spring.SetDollyCameraRelativeMode(relativeMode: number) ->  nil
```
@param `relativeMode` - `1` world, `2` look target






Sets Dolly Camera movement curve to world relative or look target relative








### Spring.SetDollyCameraLookCurve
---
```lua
function Spring.SetDollyCameraLookCurve(
  degree: number,
  cpoints: ControlPoint[],
  knots: table
) ->  nil
```
@param `cpoints` - NURBS control point positions.






Sets Dolly Camera Look Curve








### Spring.SetDollyCameraLookPosition
---
```lua
function Spring.SetDollyCameraLookPosition(
  x: number,
  y: number,
  z: number
) ->  nil
```





Sets Dolly Camera Look Position








### Spring.SetDollyCameraLookUnit
---
```lua
function Spring.SetDollyCameraLookUnit(unitID: integer) ->  nil
```
@param `unitID` - The unit to look at.






Sets target unit for Dolly Camera to look towards








### Spring.SelectUnit
---
```lua
function Spring.SelectUnit(
  unitID: integer?,
  append: boolean?
) ->  nil
```
@param `append` - (Default: `false`) Append to current selection.






Selects a single unit








### Spring.DeselectUnit
---
```lua
function Spring.DeselectUnit(unitID: integer) ->  nil
```












### Spring.DeselectUnitArray
---
```lua
function Spring.DeselectUnitArray(unitIDs: integer[]) ->  nil
```
@param `unitIDs` - Table with unit IDs as values.






Deselects multiple units.








### Spring.DeselectUnitMap
---
```lua
function Spring.DeselectUnitMap(unitMap: table<integer,any>) ->  nil
```
@param `unitMap` - Table with unit IDs as keys.






Deselects multiple units.








### Spring.SelectUnitArray
---
```lua
function Spring.SelectUnitArray(
  unitIDs: integer[],
  append: boolean?
) ->  nil
```
@param `unitIDs` - Table with unit IDs as values.

@param `append` - (Default: `false`) append to current selection






Selects multiple units, or appends to selection. Accepts a table with unitIDs as values








### Spring.SelectUnitMap
---
```lua
function Spring.SelectUnitMap(
  unitMap: table<integer,any>,
  append: boolean?
) ->  nil
```
@param `unitMap` - Table with unit IDs as keys.

@param `append` - (Default: `false`) append to current selection






Selects multiple units, or appends to selection. Accepts a table with unitIDs as keys








### Spring.AddMapLight
---
```lua
function Spring.AddMapLight(lightParams: LightParams) -> lightHandle integer
```





requires MaxDynamicMapLights > 0








### Spring.AddModelLight
---
```lua
function Spring.AddModelLight(lightParams: LightParams) -> lightHandle number
```





requires MaxDynamicMapLights > 0








### Spring.UpdateMapLight
---
```lua
function Spring.UpdateMapLight(
  lightHandle: number,
  lightParams: LightParams
) -> success boolean
```












### Spring.UpdateModelLight
---
```lua
function Spring.UpdateModelLight(
  lightHandle: number,
  lightParams: LightParams
) -> success boolean
```












### Spring.AddLightTrackingTarget
---
```lua
function Spring.AddLightTrackingTarget()
```












### Spring.SetMapLightTrackingState
---
```lua
function Spring.SetMapLightTrackingState(
  lightHandle: number,
  unitOrProjectileID: integer,
  enableTracking: boolean,
  unitOrProjectile: boolean
) -> success boolean
```





Set a map-illuminating light to start/stop tracking the position of a moving object (unit or projectile)








### Spring.SetModelLightTrackingState
---
```lua
function Spring.SetModelLightTrackingState(
  lightHandle: number,
  unitOrProjectileID: integer,
  enableTracking: boolean,
  unitOrProjectile: boolean
) -> success boolean
```





Set a model-illuminating light to start/stop tracking the position of a moving object (unit or projectile)








### Spring.SetMapShader
---
```lua
function Spring.SetMapShader(
  standardShaderID: integer,
  deferredShaderID: integer
) ->  nil
```





The ID's must refer to valid programs returned by `gl.CreateShader`.
Passing in a value of 0 will cause the respective shader to revert back to its engine default.
Custom map shaders that declare a uniform ivec2 named "texSquare" can sample from the default diffuse texture(s), which are always bound to TU 0.








### Spring.SetMapSquareTexture
---
```lua
function Spring.SetMapSquareTexture(
  texSqrX: number,
  texSqrY: number,
  luaTexName: string
) -> success boolean
```












### Spring.SetMapShadingTexture
---
```lua
function Spring.SetMapShadingTexture(
  texType: string,
  texName: string
) -> success boolean
```











@*usage* Spring.SetMapShadingTexture("$ssmf_specular", "name_of_my_shiny_texture")


### Spring.SetSkyBoxTexture
---
```lua
function Spring.SetSkyBoxTexture(texName: string) ->  nil
```












### Spring.SetUnitNoDraw
---
```lua
function Spring.SetUnitNoDraw(
  unitID: integer,
  noDraw: boolean
) ->  nil
```












### Spring.SetUnitEngineDrawMask
---
```lua
function Spring.SetUnitEngineDrawMask(
  unitID: integer,
  drawMask: number
) ->  nil
```












### Spring.SetUnitAlwaysUpdateMatrix
---
```lua
function Spring.SetUnitAlwaysUpdateMatrix(
  unitID: integer,
  alwaysUpdateMatrix: boolean
) ->  nil
```












### Spring.SetUnitNoMinimap
---
```lua
function Spring.SetUnitNoMinimap(
  unitID: integer,
  unitNoMinimap: boolean
) ->  nil
```












### Spring.SetMiniMapRotation
---
```lua
function Spring.SetMiniMapRotation(rotation: number) ->  nil
```
@param `rotation` - amount in radians













### Spring.SetUnitNoGroup
---
```lua
function Spring.SetUnitNoGroup(
  unitID: integer,
  unitNoGroup: boolean
)
```
@param `unitNoGroup` - Whether unit can be added to selection groups













### Spring.SetUnitNoSelect
---
```lua
function Spring.SetUnitNoSelect(
  unitID: integer,
  unitNoSelect: boolean
) ->  nil
```
@param `unitNoSelect` - whether unit can be selected or not













### Spring.SetUnitLeaveTracks
---
```lua
function Spring.SetUnitLeaveTracks(
  unitID: integer,
  unitLeaveTracks: boolean
) ->  nil
```
@param `unitLeaveTracks` - whether unit leaves tracks on movement













### Spring.SetUnitSelectionVolumeData
---
```lua
function Spring.SetUnitSelectionVolumeData(
  unitID: integer,
  featureID: integer,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  vType: number,
  tType: number,
  Axis: number
) ->  nil
```












### Spring.SetFeatureNoDraw
---
```lua
function Spring.SetFeatureNoDraw(
  featureID: integer,
  noDraw: boolean
) ->  nil
```












### Spring.SetFeatureEngineDrawMask
---
```lua
function Spring.SetFeatureEngineDrawMask(
  featureID: integer,
  engineDrawMask: number
) ->  nil
```












### Spring.SetFeatureAlwaysUpdateMatrix
---
```lua
function Spring.SetFeatureAlwaysUpdateMatrix(
  featureID: integer,
  alwaysUpdateMat: number
) ->  nil
```












### Spring.SetFeatureFade
---
```lua
function Spring.SetFeatureFade(
  featureID: integer,
  allow: boolean
) ->  nil
```





Control whether a feature will fade or not when zoomed out.








### Spring.SetFeatureSelectionVolumeData
---
```lua
function Spring.SetFeatureSelectionVolumeData(
  featureID: integer,
  scaleX: number,
  scaleY: number,
  scaleZ: number,
  offsetX: number,
  offsetY: number,
  offsetZ: number,
  vType: number,
  tType: number,
  Axis: number
) ->  nil
```












### Spring.AddUnitIcon
---
```lua
function Spring.AddUnitIcon(
  iconName: string,
  texFile: string,
  size: number?,
  dist: number?,
  radAdjust: number?
) -> added boolean
```












### Spring.FreeUnitIcon
---
```lua
function Spring.FreeUnitIcon(iconName: string) -> freed boolean?
```












### Spring.UnitIconSetDraw
---
```lua
function Spring.UnitIconSetDraw(
  unitID: integer,
  drawIcon: boolean
) ->  nil
```





Use Spring.SetUnitIconDraw instead.








### Spring.SetUnitIconDraw
---
```lua
function Spring.SetUnitIconDraw(
  unitID: integer,
  drawIcon: boolean
) ->  nil
```












### Spring.SetUnitDefIcon
---
```lua
function Spring.SetUnitDefIcon(
  unitDefID: integer,
  iconName: string
) ->  nil
```












### Spring.SetUnitDefImage
---
```lua
function Spring.SetUnitDefImage(
  unitDefID: integer,
  image: string
) ->  nil
```
@param `image` - luaTexture|texFile













### Spring.ExtractModArchiveFile
---
```lua
function Spring.ExtractModArchiveFile(modfile: string) -> extracted boolean
```












### Spring.CreateDir
---
```lua
function Spring.CreateDir(path: string) -> dirCreated boolean?
```












### Spring.SetActiveCommand
---
```lua
function Spring.SetActiveCommand(
  action: string,
  actionExtra: string?
) -> commandSet boolean?
```












### Spring.SetActiveCommand
---
```lua
function Spring.SetActiveCommand(
  cmdIndex: number,
  button: number?,
  leftClick: boolean?,
  rightClick: boolean?,
  alt: boolean?,
  ctrl: boolean?,
  meta: boolean?,
  shift: boolean?
) -> commandSet boolean?
```
@param `button` - (Default: `1`)













### Spring.LoadCmdColorsConfig
---
```lua
function Spring.LoadCmdColorsConfig(config: string) ->  nil
```












### Spring.LoadCtrlPanelConfig
---
```lua
function Spring.LoadCtrlPanelConfig(config: string) ->  nil
```












### Spring.ForceLayoutUpdate
---
```lua
function Spring.ForceLayoutUpdate() ->  nil
```












### Spring.SetDrawSelectionInfo
---
```lua
function Spring.SetDrawSelectionInfo(enable: boolean) ->  nil
```





Disables the "Selected Units x" box in the GUI.








### Spring.SetBoxSelectionByEngine
---
```lua
function Spring.SetBoxSelectionByEngine(state: boolean) ->  nil
```












### Spring.SetTeamColor
---
```lua
function Spring.SetTeamColor(
  teamID: integer,
  r: number,
  g: number,
  b: number
) ->  nil
```












### Spring.AssignMouseCursor
---
```lua
function Spring.AssignMouseCursor(
  cmdName: string,
  iconFileName: string,
  overwrite: boolean?,
  hotSpotTopLeft: boolean?
) -> assigned boolean?
```
@param `iconFileName` - not the full filename, instead it is like this:
Wanted filename: Anims/cursorattack_0.bmp
=> iconFileName: cursorattack

@param `overwrite` - (Default: `true`)

@param `hotSpotTopLeft` - (Default: `false`)






Changes/creates the cursor of a single CursorCmd.








### Spring.ReplaceMouseCursor
---
```lua
function Spring.ReplaceMouseCursor(
  oldFileName: string,
  newFileName: string,
  hotSpotTopLeft: boolean?
) -> assigned boolean?
```
@param `hotSpotTopLeft` - (Default: `false`)






Mass replace all occurrences of the cursor in all CursorCmds.








### Spring.SetCustomCommandDrawData
---
```lua
function Spring.SetCustomCommandDrawData(
  cmdID: integer,
  cmdReference: (string|integer)?,
  color: rgba?,
  showArea: boolean?
) ->  nil
```
@param `cmdReference` - The name or ID of an icon for command. Pass `nil` to clear draw data for command.

@param `color` - (Default: white)

@param `showArea` - (Default: `false`)






Register your custom cmd so it gets visible in the unit's cmd queue








### Spring.WarpMouse
---
```lua
function Spring.WarpMouse(
  x: number,
  y: number
) ->  nil
```












### Spring.SetMouseCursor
---
```lua
function Spring.SetMouseCursor(
  cursorName: string,
  cursorScale: number?
) ->  nil
```
@param `cursorScale` - (Default: `1.0`)













### Spring.SetLosViewColors
---
```lua
function Spring.SetLosViewColors(
  always: rgb,
  LOS: rgb,
  radar: rgb,
  jam: rgb,
  radar2: rgb
) ->  nil
```












### Spring.SetNanoProjectileParams
---
```lua
function Spring.SetNanoProjectileParams(
  rotVal: number?,
  rotVel: number?,
  rotAcc: number?,
  rotValRng: number?,
  rotVelRng: number?,
  rotAccRng: number?
) ->  nil
```
@param `rotVal` - (Default: `0`) in degrees

@param `rotVel` - (Default: `0`) in degrees

@param `rotAcc` - (Default: `0`) in degrees

@param `rotValRng` - (Default: `0`) in degrees

@param `rotVelRng` - (Default: `0`) in degrees

@param `rotAccRng` - (Default: `0`) in degrees













### Spring.SetConfigInt
---
```lua
function Spring.SetConfigInt(
  name: string,
  value: integer,
  useOverlay: boolean?
) ->  nil
```
@param `useOverlay` - (Default: `false`) If `true`, the value will only be set in memory, and not be restored for the next game.













### Spring.SetConfigFloat
---
```lua
function Spring.SetConfigFloat(
  name: string,
  value: number,
  useOverla: boolean?
) ->  nil
```
@param `useOverla` - (Default: `false`) If `true`, the value will only be set in memory, and not be restored for the next game.y













### Spring.SetConfigString
---
```lua
function Spring.SetConfigString(
  name: string,
  value: string,
  useOverlay: boolean?
) ->  nil
```
@param `useOverlay` - (Default: `false`) If `true`, the value will only be set in memory, and not be restored for the next game.













### Spring.Quit
---
```lua
function Spring.Quit() ->  nil
```





Closes the application








### Spring.SetUnitGroup
---
```lua
function Spring.SetUnitGroup(
  unitID: integer,
  groupID: integer
) ->  nil
```
@param `groupID` - the group number to be assigned, or -1 for deassignment













### Spring.GiveOrder
---
```lua
function Spring.GiveOrder(
  cmdID: (CMD|integer),
  params: CreateCommandParams,
  options: CreateCommandOptions?,
  timeout: integer?
) ->  boolean
```
@param `cmdID` - The command ID.

@param `params` - Parameters for the given command.






Give order to selected units.








### Spring.GiveOrderToUnit
---
```lua
function Spring.GiveOrderToUnit(
  unitID: integer,
  cmdID: (CMD|integer),
  params: CreateCommandParams?,
  options: CreateCommandOptions?,
  timeout: integer?
) ->  boolean
```
@param `cmdID` - The command ID.

@param `params` - Parameters for the given command.






Give order to specific unit.








### Spring.GiveOrderToUnitMap
---
```lua
function Spring.GiveOrderToUnitMap(
  unitMap: table<integer,any>,
  cmdID: (CMD|integer),
  params: CreateCommandParams?,
  options: CreateCommandOptions?,
  timeout: integer?
) -> orderGiven boolean
```
@param `unitMap` - A table with unit IDs as keys.

@param `cmdID` - The command ID.

@param `params` - Parameters for the given command.






Give order to multiple units, specified by table keys.








### Spring.GiveOrderToUnitArray
---
```lua
function Spring.GiveOrderToUnitArray(
  unitIDs: integer[],
  cmdID: (CMD|integer),
  params: CreateCommandParams?,
  options: CreateCommandOptions?,
  timeout: integer?
) -> ordersGiven boolean
```
@param `unitIDs` - Array of unit IDs.

@param `cmdID` - The command ID.

@param `params` - Parameters for the given command.


@return `ordersGiven` - `true` if any orders were sent, otherwise `false`.





Give order to an array of units.








### Spring.GiveOrderArrayToUnit
---
```lua
function Spring.GiveOrderArrayToUnit(
  unitID: integer,
  commands: CreateCommand[]
) -> ordersGiven boolean
```
@param `unitID` - Unit ID.


@return `ordersGiven` - `true` if any orders were sent, otherwise `false`.












### Spring.GiveOrderArrayToUnitMap
---
```lua
function Spring.GiveOrderArrayToUnitMap(
  unitMap: table<integer,any>,
  commands: CreateCommand[]
) -> ordersGiven boolean
```
@param `unitMap` - A table with unit IDs as keys.


@return `ordersGiven` - `true` if any orders were sent, otherwise `false`.












### Spring.GiveOrderArrayToUnitArray
---
```lua
function Spring.GiveOrderArrayToUnitArray(
  unitIDs: integer[],
  commands: CreateCommand[],
  pairwise: boolean?
) -> ordersGiven boolean
```
@param `unitIDs` - Array of unit IDs.

@param `pairwise` - (Default: `false`) When `false`, assign all commands to each unit.

When `true`, assign commands according to index between units and cmds arrays.

If `len(unitArray) < len(cmdArray)` only the first `len(unitArray)` commands
will be assigned, and vice-versa.


@return `ordersGiven` - `true` if any orders were sent, otherwise `false`.












### Spring.SetBuildSpacing
---
```lua
function Spring.SetBuildSpacing(spacing: number) ->  nil
```












### Spring.SetBuildFacing
---
```lua
function Spring.SetBuildFacing(facing: FacingInteger) ->  nil
```












### Spring.SendLuaUIMsg
---
```lua
function Spring.SendLuaUIMsg(
  message: string,
  mode: string
) ->  nil
```
@param `mode` - "s"/"specs" | "a"/"allies"













### Spring.SendLuaGaiaMsg
---
```lua
function Spring.SendLuaGaiaMsg(message: string) ->  nil
```












### Spring.SendLuaRulesMsg
---
```lua
function Spring.SendLuaRulesMsg(message: string) ->  nil
```












### Spring.SendLuaMenuMsg
---
```lua
function Spring.SendLuaMenuMsg(msg: string)
```












### Spring.SetShareLevel
---
```lua
function Spring.SetShareLevel(
  resource: string,
  shareLevel: number
) ->  nil
```
@param `resource` - metal | energy













### Spring.ShareResources
---
```lua
function Spring.ShareResources(
  teamID: integer,
  units: string
) ->  nil
```












### Spring.ShareResources
---
```lua
function Spring.ShareResources(
  teamID: integer,
  resource: string,
  amount: number
) ->  nil
```
@param `resource` - metal | energy













### Spring.SetLastMessagePosition
---
```lua
function Spring.SetLastMessagePosition(
  x: number,
  y: number,
  z: number
) ->  nil
```












### Spring.MarkerAddPoint
---
```lua
function Spring.MarkerAddPoint(
  x: number,
  y: number,
  z: number,
  text: string?,
  localOnly: boolean?
) ->  nil
```
@param `text` - (Default: `""`)













### Spring.MarkerAddLine
---
```lua
function Spring.MarkerAddLine(
  x1: number,
  y1: number,
  z1: number,
  x2: number,
  y2: number,
  z2: number,
  localOnly: boolean?,
  playerId: number?
) ->  nil
```
@param `localOnly` - (Default: `false`)













### Spring.MarkerErasePosition
---
```lua
function Spring.MarkerErasePosition(
  x: number,
  y: number,
  z: number,
  unused: nil,
  localOnly: boolean?,
  playerId: number?,
  alwaysErase: boolean?
) ->  nil
```
@param `unused` - This argument is ignored.

@param `localOnly` - (Default: `false`) do not issue a network message, erase only for the current player

@param `playerId` - when not specified it uses the issuer playerId

@param `alwaysErase` - (Default: `false`) erase any marker when `localOnly` and current player is spectating. Allows spectators to erase players markers locally






Issue an erase command for markers on the map.








### Spring.SetAtmosphere
---
```lua
function Spring.SetAtmosphere(params: AtmosphereParams)
```





It can be used to modify the following atmosphere parameters

Usage:
```lua
Spring.SetAtmosphere({ fogStart = 0, fogEnd = 0.5, fogColor = { 0.7, 0.2, 0.2, 1 }})
```








### Spring.SetSunDirection
---
```lua
function Spring.SetSunDirection(
  dirX: number,
  dirY: number,
  dirZ: number,
  intensity: number?
) ->  nil
```
@param `intensity` - (Default: `1.0`)













### Spring.SetSunLighting
---
```lua
function Spring.SetSunLighting(params: { groundDiffuseColor: rgb,groundAmbientColor: rgb })
```





Modify sun lighting parameters.

```lua
Spring.SetSunLighting({groundAmbientColor = {1, 0.1, 1}, groundDiffuseColor = {1, 0.1, 1} })
```








### Spring.SetMapRenderingParams
---
```lua
function Spring.SetMapRenderingParams(params: MapRenderingParams) ->  nil
```





Allows to change map rendering params at runtime.








### Spring.ForceTesselationUpdate
---
```lua
function Spring.ForceTesselationUpdate(
  normal: boolean?,
  shadow: boolean?
) -> updated boolean
```
@param `normal` - (Default: `true`)

@param `shadow` - (Default: `false`)













### Spring.SendSkirmishAIMessage
---
```lua
function Spring.SendSkirmishAIMessage(
  aiTeam: number,
  message: string
) -> ai_processed boolean?
```












### Spring.SetLogSectionFilterLevel
---
```lua
function Spring.SetLogSectionFilterLevel(
  sectionName: string,
  logLevel: (string|number)?
) ->  nil
```












### Spring.GarbageCollectCtrl
---
```lua
function Spring.GarbageCollectCtrl(
  itersPerBatch: integer?,
  numStepsPerIter: integer?,
  minStepsPerIter: integer?,
  maxStepsPerIter: integer?,
  minLoopRunTime: number?,
  maxLoopRunTime: number?,
  baseRunTimeMult: number?,
  baseMemLoadMult: number?
) ->  nil
```












### Spring.SetAutoShowMetal
---
```lua
function Spring.SetAutoShowMetal(autoShow: boolean) ->  nil
```












### Spring.SetDrawSky
---
```lua
function Spring.SetDrawSky(drawSky: boolean) ->  nil
```












### Spring.SetDrawWater
---
```lua
function Spring.SetDrawWater(drawWater: boolean) ->  nil
```












### Spring.SetDrawGround
---
```lua
function Spring.SetDrawGround(drawGround: boolean) ->  nil
```












### Spring.SetDrawGroundDeferred
---
```lua
function Spring.SetDrawGroundDeferred(
  drawGroundDeferred: boolean,
  drawGroundForward: boolean?
) ->  nil
```
@param `drawGroundForward` - allows disabling of the forward pass













### Spring.SetDrawModelsDeferred
---
```lua
function Spring.SetDrawModelsDeferred(
  drawUnitsDeferred: boolean,
  drawFeaturesDeferred: boolean,
  drawUnitsForward: boolean?,
  drawFeaturesForward: boolean?
) ->  nil
```
@param `drawUnitsForward` - allows disabling of the respective forward passes

@param `drawFeaturesForward` - allows disabling of the respective forward passes













### Spring.SetVideoCapturingMode
---
```lua
function Spring.SetVideoCapturingMode(allowCaptureMode: boolean) ->  nil
```





This doesn't actually record the game in any way, it just regulates the framerate and interpolations.








### Spring.SetVideoCapturingTimeOffset
---
```lua
function Spring.SetVideoCapturingTimeOffset(timeOffset: boolean) ->  nil
```












### Spring.SetWaterParams
---
```lua
function Spring.SetWaterParams(waterParams: WaterParams) ->  nil
```





Does not need cheating enabled.

Allows to change water params (mostly `BumpWater` ones) at runtime. You may
want to set `BumpWaterUseUniforms` in your `springrc` to 1, then you don't even
need to restart `BumpWater` via `/water 4`.








### Spring.PreloadUnitDefModel
---
```lua
function Spring.PreloadUnitDefModel(unitDefID: integer) ->  nil
```





Allow the engine to load the unit's model (and texture) in a background thread.
Wreckages and buildOptions of a unit are automatically preloaded.








### Spring.PreloadFeatureDefModel
---
```lua
function Spring.PreloadFeatureDefModel(featureDefID: integer) ->  nil
```












### Spring.PreloadSoundItem
---
```lua
function Spring.PreloadSoundItem(name: string) ->  nil
```












### Spring.LoadModelTextures
---
```lua
function Spring.LoadModelTextures(modelName: string) -> success boolean?
```












### Spring.CreateGroundDecal
---
```lua
function Spring.CreateGroundDecal() -> decalID number?
```












### Spring.DestroyGroundDecal
---
```lua
function Spring.DestroyGroundDecal(decalID: integer) -> delSuccess boolean
```












### Spring.SetGroundDecalPosAndDims
---
```lua
function Spring.SetGroundDecalPosAndDims(
  decalID: integer,
  midPosX: number?,
  midPosZ: number?,
  sizeX: number?,
  sizeZ: number?,
  projCubeHeight: number?
) -> decalSet boolean
```
@param `midPosX` - (Default: currMidPosX)

@param `midPosZ` - (Default: currMidPosZ)

@param `sizeX` - (Default: currSizeX)

@param `sizeZ` - (Default: currSizeZ)

@param `projCubeHeight` - (Default: calculateProjCubeHeight)













### Spring.SetGroundDecalQuadPosAndHeight
---
```lua
function Spring.SetGroundDecalQuadPosAndHeight(
  decalID: integer,
  posTL: xz?,
  posTR: xz?,
  posBR: xz?,
  posBL: xz?,
  projCubeHeight: number?
) -> decalSet boolean
```
@param `posTL` - (Default: currPosTL)

@param `posTR` - (Default: currPosTR)

@param `posBR` - (Default: currPosBR)

@param `posBL` - (Default: currPosBL)

@param `projCubeHeight` - (Default: calculateProjCubeHeight)






Use for non-rectangular decals








### Spring.SetGroundDecalRotation
---
```lua
function Spring.SetGroundDecalRotation(
  decalID: integer,
  rot: number?
) -> decalSet boolean
```
@param `rot` - (Default: random) in radians













### Spring.SetGroundDecalTexture
---
```lua
function Spring.SetGroundDecalTexture(
  decalID: integer,
  textureName: string,
  isMainTex: boolean?
) -> decalSet boolean?
```
@param `textureName` - The texture has to be on the atlas which seems to mean it's defined as an explosion, unit tracks, or building plate decal on some unit already (no arbitrary textures)

@param `isMainTex` - (Default: `true`) If false, it sets the normals/glow map













### Spring.SetGroundDecalTextureParams
---
```lua
function Spring.SetGroundDecalTextureParams(
  decalID: integer,
  texWrapDistance: number?,
  texTraveledDistance: number?
) -> decalSet boolean?
```
@param `texWrapDistance` - (Default: currTexWrapDistance) if non-zero sets the mode to repeat the texture along the left-right direction of the decal every texWrapFactor elmos

@param `texTraveledDistance` - (Default: currTexTraveledDistance) shifts the texture repetition defined by texWrapFactor so the texture of a next line in the continuous multiline can start where the previous finished. For that it should collect all elmo lengths of the previously set multiline segments.













### Spring.SetGroundDecalAlpha
---
```lua
function Spring.SetGroundDecalAlpha(
  decalID: integer,
  alpha: number?,
  alphaFalloff: number?
) -> decalSet boolean
```
@param `alpha` - (Default: currAlpha) Between 0 and 1

@param `alphaFalloff` - (Default: currAlphaFalloff) Between 0 and 1, per second













### Spring.SetGroundDecalNormal
---
```lua
function Spring.SetGroundDecalNormal(
  decalID: integer,
  normalX: number?,
  normalY: number?,
  normalZ: number?
) -> decalSet boolean
```
@param `normalX` - (Default: `0`)

@param `normalY` - (Default: `0`)

@param `normalZ` - (Default: `0`)






Sets projection cube normal to orient in 3D space.
In case the normal (0,0,0) then normal is picked from the terrain








### Spring.SetGroundDecalTint
---
```lua
function Spring.SetGroundDecalTint(
  decalID: integer,
  tintColR: number?,
  tintColG: number?,
  tintColB: number?,
  tintColA: number?
) -> decalSet boolean
```
@param `tintColR` - (Default: curTintColR)

@param `tintColG` - (Default: curTintColG)

@param `tintColB` - (Default: curTintColB)

@param `tintColA` - (Default: curTintColA)






Sets the tint of the ground decal. Color = 2 * textureColor * tintColor
Respectively a color of (0.5, 0.5, 0.5, 0.5) is effectively no tint








### Spring.SetGroundDecalMisc
---
```lua
function Spring.SetGroundDecalMisc(
  decalID: integer,
  dotElimExp: number?,
  refHeight: number?,
  minHeight: number?,
  maxHeight: number?,
  forceHeightMode: number?
) -> decalSet boolean
```
@param `dotElimExp` - (Default: curValue) pow(max(dot(decalProjVector, SurfaceNormal), 0.0), dotElimExp), used to reduce decal artifacts on surfaces non-collinear with the projection vector

@param `refHeight` - (Default: curValue)

@param `minHeight` - (Default: curValue)

@param `maxHeight` - (Default: curValue)

@param `forceHeightMode` - (Default: curValue) in case forceHeightMode==1.0 ==> force relative height: midPoint.y = refHeight + clamp(midPoint.y - refHeight, minHeight); forceHeightMode==2.0 ==> force absolute height: midPoint.y = midPoint.y, clamp(midPoint.y, minHeight, maxHeight); other forceHeightMode values do not enforce the height of the center position






Sets varios secondary parameters of a decal








### Spring.SetGroundDecalCreationFrame
---
```lua
function Spring.SetGroundDecalCreationFrame(
  decalID: integer,
  creationFrameMin: number?,
  creationFrameMax: number?
) -> decalSet boolean
```
@param `creationFrameMin` - (Default: currCreationFrameMin)

@param `creationFrameMax` - (Default: currCreationFrameMax)






Use separate min and max for "gradient" style decals such as tank tracks








### Spring.SDLSetTextInputRect
---
```lua
function Spring.SDLSetTextInputRect(
  x: number,
  y: number,
  width: number,
  height: number
) ->  nil
```












### Spring.SDLStartTextInput
---
```lua
function Spring.SDLStartTextInput() ->  nil
```












### Spring.SDLStopTextInput
---
```lua
function Spring.SDLStopTextInput() ->  nil
```












### Spring.SetWindowGeometry
---
```lua
function Spring.SetWindowGeometry(
  displayIndex: number,
  winRelPosX: number,
  winRelPosY: number,
  winSizeX: number,
  winSizeY: number,
  fullScreen: boolean,
  borderless: boolean
) ->  nil
```












### Spring.SetWindowMinimized
---
```lua
function Spring.SetWindowMinimized() -> minimized boolean
```












### Spring.SetWindowMaximized
---
```lua
function Spring.SetWindowMaximized() -> maximized boolean
```












### Spring.Reload
---
```lua
function Spring.Reload(startScript: string) ->  nil
```
@param `startScript` - the CONTENT of the script.txt spring should use to start.













### Spring.Restart
---
```lua
function Spring.Restart(
  commandline_args: string,
  startScript: string
) ->  nil
```
@param `commandline_args` - commandline arguments passed to spring executable.






If this call returns, something went wrong








### Spring.Start
---
```lua
function Spring.Start(
  commandline_args: string,
  startScript: string
) ->  nil
```
@param `commandline_args` - commandline arguments passed to spring executable.

@param `startScript` - the CONTENT of the script.txt spring should use to start (if empty, no start-script is added, you can still point spring to your custom script.txt when you add the file-path to commandline_args.






Launches a new Spring instance without terminating the existing one.

If this call returns, something went wrong








### Spring.SetWMIcon
---
```lua
function Spring.SetWMIcon(iconFileName: string) ->  nil
```





Sets the icon for the process which is seen in the OS task-bar and other places (default: spring-logo).

Note: has to be 24bit or 32bit.
Note: on windows, it has to be 32x32 pixels in size (recommended for cross-platform)
Note: *.bmp images have to be in BGR format (default for m$ ones).
Note: *.ico images are not supported.








### Spring.ClearWatchDogTimer
---
```lua
function Spring.ClearWatchDogTimer(threadName: string?) ->  nil
```
@param `threadName` - (Default: main)













### Spring.SetClipboard
---
```lua
function Spring.SetClipboard(text: string) ->  nil
```












### Spring.Yield
---
```lua
function Spring.Yield() -> when boolean
```

@return `when` - true caller should continue calling `Spring.Yield` during the widgets/gadgets load, when false it shouldn't call it any longer.





Relinquish control of the game loading thread and OpenGL context back to the UI (LuaIntro).

Should be called after each widget/unsynced gadget is loaded in widget/gadget handler. Use it to draw screen updates and process windows events.







@*usage* local wantYield = Spring.Yield and Spring.Yield() -- nil check: not present in synced
for wupget in pairs(wupgetsToLoad) do
loadWupget(wupget)
wantYield = wantYield and Spring.Yield()
end


### Spring.GetMetalMapSize
---
```lua
function Spring.GetMetalMapSize()
 -> x integer
 -> z integer

```

@return `x` - X coordinate in worldspace / `Game.metalMapSquareSize`.

@return `z` - Z coordinate in worldspace / `Game.metalMapSquareSize`.












### Spring.GetMetalAmount
---
```lua
function Spring.GetMetalAmount(
  x: integer,
  z: integer
) -> amount number
```
@param `x` - X coordinate in worldspace / `Game.metalMapSquareSize`.

@param `z` - Z coordinate in worldspace / `Game.metalMapSquareSize`.






Returns the amount of metal on a single square.








### Spring.SetMetalAmount
---
```lua
function Spring.SetMetalAmount(
  x: integer,
  z: integer,
  metalAmount: number
) ->  nil
```
@param `x` - X coordinate in worldspace / `Game.metalMapSquareSize`.

@param `z` - Z coordinate in worldspace / `Game.metalMapSquareSize`.

@param `metalAmount` - must be between 0 and 255*maxMetal (with maxMetal from the .smd or mapinfo.lua).






Sets the amount of metal on a single square.








### Spring.GetMetalExtraction
---
```lua
function Spring.GetMetalExtraction(
  x: integer,
  z: integer
) -> extraction number
```
@param `x` - X coordinate in worldspace / `Game.metalMapSquareSize`.

@param `z` - Z coordinate in worldspace / `Game.metalMapSquareSize`.













### Spring.Echo
---
```lua
function Spring.Echo(
  arg: any,
  ...: any
) ->  nil
```





Prints values in the spring chat console. Useful for debugging.

Hint: the default print() writes to STDOUT.








### Spring.Log
---
```lua
function Spring.Log(
  section: string,
  logLevel: (LogLevel|LOG)?,
  ...: string
)
```
@param `section` - Sets an arbitrary section. Level filtering can be applied per-section

@param `logLevel` - (Default: `"notice"`)

@param `...` - messages






Logs a message to the logfile/console.











## fields
---

### Spring.MoveCtrl
---
```lua
Spring.MoveCtrl : MoveCtrl
```










### Spring.MoveCtrl
---
```lua
Spring.MoveCtrl : MoveCtrl
```











