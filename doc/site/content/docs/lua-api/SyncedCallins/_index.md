---
title: "SyncedCallins"
permalink: "docs/lua-api/SyncedCallins"
---
# class SyncedCallins





Functions called by the Engine (Synced).







---

## methods
---

### SyncedCallins.CommandFallback
---
```lua
function SyncedCallins.CommandFallback(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  cmdID: integer,
  cmdParams: number[],
  cmdOptions: CommandOptions,
  cmdTag: number
) -> whether boolean
```

@return `whether` - to remove the command from the queue





Called when the unit reaches an unknown command in its queue (i.e. one not handled by the engine).








### SyncedCallins.AllowCommand
---
```lua
function SyncedCallins.AllowCommand(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  cmdID: integer,
  cmdParams: number[],
  cmdOptions: CommandOptions,
  cmdTag: number,
  synced: boolean,
  fromLua: boolean
) -> whether boolean
```

@return `whether` - it should be let into the queue.





Called when the command is given, before the unit's queue is altered.

The queue remains untouched when a command is blocked, whether it would be queued or replace the queue.








### SyncedCallins.AllowUnitCreation
---
```lua
function SyncedCallins.AllowUnitCreation(
  unitDefID: integer,
  builderID: integer,
  builderTeam: integer,
  x: number,
  y: number,
  z: number,
  facing: FacingInteger
)
 -> allow boolean
 -> dropOrder boolean

```





Called just before unit is created.








### SyncedCallins.AllowUnitTransfer
---
```lua
function SyncedCallins.AllowUnitTransfer(
  unitID: integer,
  unitDefID: integer,
  oldTeam: integer,
  newTeam: integer,
  capture: boolean
) -> whether boolean
```

@return `whether` - or not the transfer is permitted.





Called just before a unit is transferred to a different team.








### SyncedCallins.AllowUnitBuildStep
---
```lua
function SyncedCallins.AllowUnitBuildStep(
  builderID: integer,
  builderTeam: integer,
  unitID: integer,
  unitDefID: integer,
  part: number
) -> whether boolean
```

@return `whether` - or not the build makes progress.





Called just before a unit progresses its build percentage.








### SyncedCallins.AllowUnitCaptureStep
---
```lua
function SyncedCallins.AllowUnitCaptureStep(
  builderID: integer,
  builderTeam: integer,
  unitID: integer,
  unitDefID: integer,
  part: number
) -> whether boolean
```

@return `whether` - or not the capture makes progress.












### SyncedCallins.AllowUnitTransport
---
```lua
function SyncedCallins.AllowUnitTransport(
  transporterID: integer,
  transporterUnitDefID: integer,
  transporterTeam: integer,
  transporteeID: integer,
  transporteeUnitDefID: integer,
  transporteeTeam: integer
) -> whether boolean
```

@return `whether` - or not the transport is allowed












### SyncedCallins.AllowUnitTransportLoad
---
```lua
function SyncedCallins.AllowUnitTransportLoad(
  transporterID: integer,
  transporterUnitDefID: integer,
  transporterTeam: integer,
  transporteeID: integer,
  transporteeUnitDefID: integer,
  transporteeTeam: integer,
  x: number,
  y: number,
  z: number
) -> whether boolean
```

@return `whether` - or not the transport load is allowed












### SyncedCallins.AllowUnitTransportUnload
---
```lua
function SyncedCallins.AllowUnitTransportUnload(
  transporterID: integer,
  transporterUnitDefID: integer,
  transporterTeam: integer,
  transporteeID: integer,
  transporteeUnitDefID: integer,
  transporteeTeam: integer,
  x: number,
  y: number,
  z: number
) -> whether boolean
```

@return `whether` - or not the transport unload is allowed












### SyncedCallins.AllowUnitCloak
---
```lua
function SyncedCallins.AllowUnitCloak(
  unitID: integer,
  enemyID: integer?
) -> whether boolean
```

@return `whether` - unit is allowed to cloak












### SyncedCallins.AllowUnitCloak
---
```lua
function SyncedCallins.AllowUnitCloak(
  unitID: integer,
  objectID: integer?,
  weaponNum: number?
) -> whether boolean
```

@return `whether` - unit is allowed to decloak












### SyncedCallins.AllowUnitKamikaze
---
```lua
function SyncedCallins.AllowUnitKamikaze(
  unitID: integer,
  targetID: integer
) -> whether boolean
```

@return `whether` - unit is allowed to selfd












### SyncedCallins.AllowFeatureCreation
---
```lua
function SyncedCallins.AllowFeatureCreation(
  featureDefID: integer,
  teamID: integer,
  x: number,
  y: number,
  z: number
) -> whether boolean
```

@return `whether` - or not the creation is permitted





Called just before feature is created.








### SyncedCallins.AllowFeatureBuildStep
---
```lua
function SyncedCallins.AllowFeatureBuildStep(
  builderID: integer,
  builderTeam: integer,
  featureID: integer,
  featureDefID: integer,
  part: number
) -> whether boolean
```

@return `whether` - or not the change is permitted





Called just before a feature changes its build percentage.

Note that this is also called for resurrecting features, and for refilling features with resources before resurrection.
On reclaim the part values are negative, and on refill and resurrect they are positive.
Part is the percentage the feature be built or reclaimed per frame.
Eg. for a 30 workertime builder, that's a build power of 1 per frame.
For a 50 buildtime feature reclaimed by this builder, part will be 100/-50(/1) = -2%, or -0.02 numerically.








### SyncedCallins.AllowResourceLevel
---
```lua
function SyncedCallins.AllowResourceLevel(
  teamID: integer,
  res: string,
  level: number
) -> whether boolean
```

@return `whether` - or not the sharing level is permitted





Called when a team sets the sharing level of a resource.








### SyncedCallins.AllowResourceTransfer
---
```lua
function SyncedCallins.AllowResourceTransfer(
  oldTeamID: integer,
  newTeamID: integer,
  res: string,
  amount: number
) -> whether boolean
```

@return `whether` - or not the transfer is permitted.





Called just before resources are transferred between players.








### SyncedCallins.AllowDirectUnitControl
---
```lua
function SyncedCallins.AllowDirectUnitControl(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  playerID: integer
) -> allow boolean
```





Determines if this unit can be controlled directly in FPS view.








### SyncedCallins.AllowBuilderHoldFire
---
```lua
function SyncedCallins.AllowBuilderHoldFire(
  unitID: integer,
  unitDefID: integer,
  action: unknown
) -> actionAllowed boolean
```
@param `action` - One of the following:
- `-1` build,
- `CMD.REPAIR` repair,
- `CMD.RECLAIM` reclaim,
- `CMD.RESTORE` restore,
- `CMD.RESURRECT` resurrect, or
- `CMD.CAPTURE` capture.






Called when a construction unit wants to "use his nano beams".








### SyncedCallins.AllowStartPosition
---
```lua
function SyncedCallins.AllowStartPosition(
  playerID: integer,
  teamID: integer,
  readyState: number,
  clampedX: number,
  clampedY: number,
  clampedZ: number,
  rawX: number,
  rawY: number,
  rawZ: number
) -> allow boolean
```





Whether a start position should be allowed

clamped{X,Y,Z} are the coordinates clamped into start-boxes, raw is where player tried to place their marker.

The readyState can be any one of:

0 - player picked a position,
1 - player clicked ready,
2 - player pressed ready OR the game was force-started (player did not click ready, but is now forcibly readied) or
3 - the player failed to load.
The default 'failed to choose' start-position is the north-west point of their startbox, or (0,0,0) if they do not have a startbox.








### SyncedCallins.MoveCtrlNotify
---
```lua
function SyncedCallins.MoveCtrlNotify(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  data: number
) -> whether boolean
```
@param `data` - was supposed to indicate the type of notification but currently never has a value other than 1 ("unit hit the ground").


@return `whether` - or not the unit should remain script-controlled (false) or return to engine controlled movement (true).





Enable both Spring.MoveCtrl.SetCollideStop and Spring.MoveCtrl.SetTrackGround to enable this call-in.








### SyncedCallins.TerraformComplete
---
```lua
function SyncedCallins.TerraformComplete(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  buildUnitID: integer,
  buildUnitDefID: integer,
  buildUnitTeam: integer
) -> if boolean
```

@return `if` - true the current build order is terminated





Called when pre-building terrain levelling terraforms are completed (c.f. levelGround)








### SyncedCallins.UnitPreDamaged
---
```lua
function SyncedCallins.UnitPreDamaged(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  damage: number,
  paralyzer: boolean,
  weaponDefID: integer?,
  projectileID: integer?,
  attackerID: integer?,
  attackerDefID: integer?,
  attackerTeam: integer?
)
 -> newDamage number
 -> impulseMult number

```
@param `weaponDefID` - Synced Only

@param `projectileID` - Synced Only

@param `attackerID` - Synced Only

@param `attackerDefID` - Synced Only

@param `attackerTeam` - Synced Only






Called before damage is applied to the unit, allows fine control over how much damage and impulse is applied.

Called after every damage modification (even `HitByWeaponId`) but before the damage is applied

expects two numbers returned by lua code:
1st is stored under *newDamage if newDamage != NULL
2nd is stored under *impulseMult if impulseMult != NULL








### SyncedCallins.FeaturePreDamaged
---
```lua
function SyncedCallins.FeaturePreDamaged(
  featureID: integer,
  featureDefID: integer,
  featureTeam: integer,
  damage: number,
  weaponDefID: integer,
  projectileID: integer,
  attackerID: integer,
  attackerDefID: integer,
  attackerTeam: integer
)
 -> newDamage number
 -> impulseMult number

```





Called before damage is applied to the feature.

Allows fine control over how much damage and impulse is applied.








### SyncedCallins.ShieldPreDamaged
---
```lua
function SyncedCallins.ShieldPreDamaged(
  projectileID: integer,
  projectileOwnerID: integer,
  shieldWeaponNum: integer,
  shieldCarrierID: integer,
  bounceProjectile: boolean,
  beamEmitterWeaponNum: integer,
  beamEmitterUnitID: integer,
  startX: number,
  startY: number,
  startZ: number,
  hitX: number,
  hitY: number,
  hitZ: number
) -> if boolean
```

@return `if` - true the gadget handles the collision event and the engine does not remove the projectile





Called before any engine shield-vs-projectile logic executes.

If the weapon is a hitscan type (BeamLaser or LightningCanon) then proID is nil and beamEmitterWeaponNum and beamEmitterUnitID are populated instead.








### SyncedCallins.AllowWeaponTargetCheck
---
```lua
function SyncedCallins.AllowWeaponTargetCheck(
  attackerID: integer,
  attackerWeaponNum: integer,
  attackerWeaponDefID: integer
)
 -> allowCheck boolean
 -> ignoreCheck boolean

```





Determines if this weapon can automatically generate targets itself. See also commandFire weaponDef tag.








### SyncedCallins.AllowWeaponTarget
---
```lua
function SyncedCallins.AllowWeaponTarget(
  attackerID: integer,
  targetID: integer,
  attackerWeaponNum: integer,
  attackerWeaponDefID: integer,
  defPriority: number
)
 -> allowed boolean
 -> the number

```

@return `the` - new priority for this target (if you don't want to change it, return defPriority). Lower priority targets are targeted first.





Controls blocking of a specific target from being considered during a weapon's periodic auto-targeting sweep.








### SyncedCallins.AllowWeaponInterceptTarget
---
```lua
function SyncedCallins.AllowWeaponInterceptTarget(
  interceptorUnitID: integer,
  interceptorWeaponID: integer,
  targetProjectileID: integer
) -> allowed boolean
```





Controls blocking of a specific intercept target from being considered during an interceptor weapon's periodic auto-targeting sweep.

Only called for weaponDefIDs registered via Script.SetWatchWeapon.








### SyncedCallins.CommandFallback
---
```lua
function SyncedCallins.CommandFallback(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  cmdID: integer,
  cmdParams: number[],
  cmdOptions: CommandOptions,
  cmdTag: number
) -> whether boolean
```

@return `whether` - to remove the command from the queue





Called when the unit reaches an unknown command in its queue (i.e. one not handled by the engine).








### SyncedCallins.AllowCommand
---
```lua
function SyncedCallins.AllowCommand(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  cmdID: integer,
  cmdParams: number[],
  cmdOptions: CommandOptions,
  cmdTag: number,
  synced: boolean,
  fromLua: boolean
) -> whether boolean
```

@return `whether` - it should be let into the queue.





Called when the command is given, before the unit's queue is altered.

The queue remains untouched when a command is blocked, whether it would be queued or replace the queue.








### SyncedCallins.AllowUnitCreation
---
```lua
function SyncedCallins.AllowUnitCreation(
  unitDefID: integer,
  builderID: integer,
  builderTeam: integer,
  x: number,
  y: number,
  z: number,
  facing: FacingInteger
)
 -> allow boolean
 -> dropOrder boolean

```





Called just before unit is created.








### SyncedCallins.AllowUnitTransfer
---
```lua
function SyncedCallins.AllowUnitTransfer(
  unitID: integer,
  unitDefID: integer,
  oldTeam: integer,
  newTeam: integer,
  capture: boolean
) -> whether boolean
```

@return `whether` - or not the transfer is permitted.





Called just before a unit is transferred to a different team.








### SyncedCallins.AllowUnitBuildStep
---
```lua
function SyncedCallins.AllowUnitBuildStep(
  builderID: integer,
  builderTeam: integer,
  unitID: integer,
  unitDefID: integer,
  part: number
) -> whether boolean
```

@return `whether` - or not the build makes progress.





Called just before a unit progresses its build percentage.








### SyncedCallins.AllowUnitCaptureStep
---
```lua
function SyncedCallins.AllowUnitCaptureStep(
  builderID: integer,
  builderTeam: integer,
  unitID: integer,
  unitDefID: integer,
  part: number
) -> whether boolean
```

@return `whether` - or not the capture makes progress.












### SyncedCallins.AllowUnitTransport
---
```lua
function SyncedCallins.AllowUnitTransport(
  transporterID: integer,
  transporterUnitDefID: integer,
  transporterTeam: integer,
  transporteeID: integer,
  transporteeUnitDefID: integer,
  transporteeTeam: integer
) -> whether boolean
```

@return `whether` - or not the transport is allowed












### SyncedCallins.AllowUnitTransportLoad
---
```lua
function SyncedCallins.AllowUnitTransportLoad(
  transporterID: integer,
  transporterUnitDefID: integer,
  transporterTeam: integer,
  transporteeID: integer,
  transporteeUnitDefID: integer,
  transporteeTeam: integer,
  x: number,
  y: number,
  z: number
) -> whether boolean
```

@return `whether` - or not the transport load is allowed












### SyncedCallins.AllowUnitTransportUnload
---
```lua
function SyncedCallins.AllowUnitTransportUnload(
  transporterID: integer,
  transporterUnitDefID: integer,
  transporterTeam: integer,
  transporteeID: integer,
  transporteeUnitDefID: integer,
  transporteeTeam: integer,
  x: number,
  y: number,
  z: number
) -> whether boolean
```

@return `whether` - or not the transport unload is allowed












### SyncedCallins.AllowUnitCloak
---
```lua
function SyncedCallins.AllowUnitCloak(
  unitID: integer,
  enemyID: integer?
) -> whether boolean
```

@return `whether` - unit is allowed to cloak












### SyncedCallins.AllowUnitCloak
---
```lua
function SyncedCallins.AllowUnitCloak(
  unitID: integer,
  objectID: integer?,
  weaponNum: number?
) -> whether boolean
```

@return `whether` - unit is allowed to decloak












### SyncedCallins.AllowUnitKamikaze
---
```lua
function SyncedCallins.AllowUnitKamikaze(
  unitID: integer,
  targetID: integer
) -> whether boolean
```

@return `whether` - unit is allowed to selfd












### SyncedCallins.AllowFeatureCreation
---
```lua
function SyncedCallins.AllowFeatureCreation(
  featureDefID: integer,
  teamID: integer,
  x: number,
  y: number,
  z: number
) -> whether boolean
```

@return `whether` - or not the creation is permitted





Called just before feature is created.








### SyncedCallins.AllowFeatureBuildStep
---
```lua
function SyncedCallins.AllowFeatureBuildStep(
  builderID: integer,
  builderTeam: integer,
  featureID: integer,
  featureDefID: integer,
  part: number
) -> whether boolean
```

@return `whether` - or not the change is permitted





Called just before a feature changes its build percentage.

Note that this is also called for resurrecting features, and for refilling features with resources before resurrection.
On reclaim the part values are negative, and on refill and resurrect they are positive.
Part is the percentage the feature be built or reclaimed per frame.
Eg. for a 30 workertime builder, that's a build power of 1 per frame.
For a 50 buildtime feature reclaimed by this builder, part will be 100/-50(/1) = -2%, or -0.02 numerically.








### SyncedCallins.AllowResourceLevel
---
```lua
function SyncedCallins.AllowResourceLevel(
  teamID: integer,
  res: string,
  level: number
) -> whether boolean
```

@return `whether` - or not the sharing level is permitted





Called when a team sets the sharing level of a resource.








### SyncedCallins.AllowResourceTransfer
---
```lua
function SyncedCallins.AllowResourceTransfer(
  oldTeamID: integer,
  newTeamID: integer,
  res: string,
  amount: number
) -> whether boolean
```

@return `whether` - or not the transfer is permitted.





Called just before resources are transferred between players.








### SyncedCallins.AllowDirectUnitControl
---
```lua
function SyncedCallins.AllowDirectUnitControl(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  playerID: integer
) -> allow boolean
```





Determines if this unit can be controlled directly in FPS view.








### SyncedCallins.AllowBuilderHoldFire
---
```lua
function SyncedCallins.AllowBuilderHoldFire(
  unitID: integer,
  unitDefID: integer,
  action: unknown
) -> actionAllowed boolean
```
@param `action` - One of the following:
- `-1` build,
- `CMD.REPAIR` repair,
- `CMD.RECLAIM` reclaim,
- `CMD.RESTORE` restore,
- `CMD.RESURRECT` resurrect, or
- `CMD.CAPTURE` capture.






Called when a construction unit wants to "use his nano beams".








### SyncedCallins.AllowStartPosition
---
```lua
function SyncedCallins.AllowStartPosition(
  playerID: integer,
  teamID: integer,
  readyState: number,
  clampedX: number,
  clampedY: number,
  clampedZ: number,
  rawX: number,
  rawY: number,
  rawZ: number
) -> allow boolean
```





Whether a start position should be allowed

clamped{X,Y,Z} are the coordinates clamped into start-boxes, raw is where player tried to place their marker.

The readyState can be any one of:

0 - player picked a position,
1 - player clicked ready,
2 - player pressed ready OR the game was force-started (player did not click ready, but is now forcibly readied) or
3 - the player failed to load.
The default 'failed to choose' start-position is the north-west point of their startbox, or (0,0,0) if they do not have a startbox.








### SyncedCallins.MoveCtrlNotify
---
```lua
function SyncedCallins.MoveCtrlNotify(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  data: number
) -> whether boolean
```
@param `data` - was supposed to indicate the type of notification but currently never has a value other than 1 ("unit hit the ground").


@return `whether` - or not the unit should remain script-controlled (false) or return to engine controlled movement (true).





Enable both Spring.MoveCtrl.SetCollideStop and Spring.MoveCtrl.SetTrackGround to enable this call-in.








### SyncedCallins.TerraformComplete
---
```lua
function SyncedCallins.TerraformComplete(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  buildUnitID: integer,
  buildUnitDefID: integer,
  buildUnitTeam: integer
) -> if boolean
```

@return `if` - true the current build order is terminated





Called when pre-building terrain levelling terraforms are completed (c.f. levelGround)








### SyncedCallins.UnitPreDamaged
---
```lua
function SyncedCallins.UnitPreDamaged(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer,
  damage: number,
  paralyzer: boolean,
  weaponDefID: integer?,
  projectileID: integer?,
  attackerID: integer?,
  attackerDefID: integer?,
  attackerTeam: integer?
)
 -> newDamage number
 -> impulseMult number

```
@param `weaponDefID` - Synced Only

@param `projectileID` - Synced Only

@param `attackerID` - Synced Only

@param `attackerDefID` - Synced Only

@param `attackerTeam` - Synced Only






Called before damage is applied to the unit, allows fine control over how much damage and impulse is applied.

Called after every damage modification (even `HitByWeaponId`) but before the damage is applied

expects two numbers returned by lua code:
1st is stored under *newDamage if newDamage != NULL
2nd is stored under *impulseMult if impulseMult != NULL








### SyncedCallins.FeaturePreDamaged
---
```lua
function SyncedCallins.FeaturePreDamaged(
  featureID: integer,
  featureDefID: integer,
  featureTeam: integer,
  damage: number,
  weaponDefID: integer,
  projectileID: integer,
  attackerID: integer,
  attackerDefID: integer,
  attackerTeam: integer
)
 -> newDamage number
 -> impulseMult number

```





Called before damage is applied to the feature.

Allows fine control over how much damage and impulse is applied.








### SyncedCallins.ShieldPreDamaged
---
```lua
function SyncedCallins.ShieldPreDamaged(
  projectileID: integer,
  projectileOwnerID: integer,
  shieldWeaponNum: integer,
  shieldCarrierID: integer,
  bounceProjectile: boolean,
  beamEmitterWeaponNum: integer,
  beamEmitterUnitID: integer,
  startX: number,
  startY: number,
  startZ: number,
  hitX: number,
  hitY: number,
  hitZ: number
) -> if boolean
```

@return `if` - true the gadget handles the collision event and the engine does not remove the projectile





Called before any engine shield-vs-projectile logic executes.

If the weapon is a hitscan type (BeamLaser or LightningCanon) then proID is nil and beamEmitterWeaponNum and beamEmitterUnitID are populated instead.








### SyncedCallins.AllowWeaponTargetCheck
---
```lua
function SyncedCallins.AllowWeaponTargetCheck(
  attackerID: integer,
  attackerWeaponNum: integer,
  attackerWeaponDefID: integer
)
 -> allowCheck boolean
 -> ignoreCheck boolean

```





Determines if this weapon can automatically generate targets itself. See also commandFire weaponDef tag.








### SyncedCallins.AllowWeaponTarget
---
```lua
function SyncedCallins.AllowWeaponTarget(
  attackerID: integer,
  targetID: integer,
  attackerWeaponNum: integer,
  attackerWeaponDefID: integer,
  defPriority: number
)
 -> allowed boolean
 -> the number

```

@return `the` - new priority for this target (if you don't want to change it, return defPriority). Lower priority targets are targeted first.





Controls blocking of a specific target from being considered during a weapon's periodic auto-targeting sweep.








### SyncedCallins.AllowWeaponInterceptTarget
---
```lua
function SyncedCallins.AllowWeaponInterceptTarget(
  interceptorUnitID: integer,
  interceptorWeaponID: integer,
  targetProjectileID: integer
) -> allowed boolean
```





Controls blocking of a specific intercept target from being considered during an interceptor weapon's periodic auto-targeting sweep.

Only called for weaponDefIDs registered via Script.SetWatchWeapon.











