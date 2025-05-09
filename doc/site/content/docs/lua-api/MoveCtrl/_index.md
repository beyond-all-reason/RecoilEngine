---
title: "MoveCtrl"
permalink: "docs/lua-api/MoveCtrl"
---
# class MoveCtrl





Accessed via `Spring.MoveCtrl`.







---

## methods
---

### MoveCtrl.IsEnabled
---
```lua
function MoveCtrl.IsEnabled(unitID: integer) -> isEnabled boolean?
```












### MoveCtrl.Enable
---
```lua
function MoveCtrl.Enable(unitID: integer)
```












### MoveCtrl.Disable
---
```lua
function MoveCtrl.Disable(unitID: integer)
```












### MoveCtrl.SetTag
---
```lua
function MoveCtrl.SetTag(
  unitID: integer,
  tag: integer
)
```












### MoveCtrl.GetTag
---
```lua
function MoveCtrl.GetTag(tag: integer?)
```












### MoveCtrl.SetProgressState
---
```lua
function MoveCtrl.SetProgressState(
  unitID: integer,
  state: (0|1|2|"done"|"active"|"failed")
)
```












### MoveCtrl.SetExtrapolate
---
```lua
function MoveCtrl.SetExtrapolate(
  unitID: integer,
  extrapolate: boolean
)
```












### MoveCtrl.SetPhysics
---
```lua
function MoveCtrl.SetPhysics(
  unitID: integer,
  posX: number,
  posY: number,
  posZ: number,
  velX: number,
  velY: number,
  velZ: number,
  rotX: number,
  rotY: number,
  rotZ: number
)
```
@param `posX` - Position X component.

@param `posY` - Position Y component.

@param `posZ` - Position Z component.

@param `velX` - Velocity X component.

@param `velY` - Velocity Y component.

@param `velZ` - Velocity Z component.

@param `rotX` - Rotation X component.

@param `rotY` - Rotation Y component.

@param `rotZ` - Rotation Z component.













### MoveCtrl.SetPosition
---
```lua
function MoveCtrl.SetPosition(
  unitID: integer,
  posX: number,
  posY: number,
  posZ: number
)
```
@param `posX` - Position X component.

@param `posY` - Position Y component.

@param `posZ` - Position Z component.













### MoveCtrl.SetVelocity
---
```lua
function MoveCtrl.SetVelocity(
  unitID: integer,
  velX: number,
  velY: number,
  velZ: number
)
```
@param `velX` - Velocity X component.

@param `velY` - Velocity Y component.

@param `velZ` - Velocity Z component.













### MoveCtrl.SetRelativeVelocity
---
```lua
function MoveCtrl.SetRelativeVelocity(
  unitID: integer,
  relVelX: number,
  relVelY: number,
  relVelZ: number
)
```
@param `relVelX` - Relative velocity X component.

@param `relVelY` - Relative velocity Y component.

@param `relVelZ` - Relative velocity Z component.













### MoveCtrl.SetRotation
---
```lua
function MoveCtrl.SetRotation(
  unitID: integer,
  rotX: number,
  rotY: number,
  rotZ: number
)
```
@param `rotX` - Rotation X component.

@param `rotY` - Rotation Y component.

@param `rotZ` - Rotation Z component.













### MoveCtrl.SetRotationOffset
---
```lua
function MoveCtrl.SetRotationOffset()
```







@deprecated Deprecated






### MoveCtrl.SetRotationVelocity
---
```lua
function MoveCtrl.SetRotationVelocity(
  unitID: integer,
  rotVelX: number,
  rotVelY: number,
  rotVelZ: number
)
```
@param `rotVelX` - Rotation velocity X component.

@param `rotVelY` - Rotation velocity Y component.

@param `rotVelZ` - Rotation velocity Z component.













### MoveCtrl.SetHeading
---
```lua
function MoveCtrl.SetHeading(
  unitID: integer,
  heading: Heading
)
```












### MoveCtrl.SetTrackSlope
---
```lua
function MoveCtrl.SetTrackSlope(
  unitID: integer,
  trackSlope: boolean
)
```












### MoveCtrl.SetTrackGround
---
```lua
function MoveCtrl.SetTrackGround(
  unitID: integer,
  trackGround: boolean
)
```












### MoveCtrl.SetTrackLimits
---
```lua
function MoveCtrl.SetTrackLimits(
  unitID: integer,
  trackLimits: boolean
)
```












### MoveCtrl.SetGroundOffset
---
```lua
function MoveCtrl.SetGroundOffset(
  unitID: integer,
  groundOffset: number
)
```












### MoveCtrl.SetGravity
---
```lua
function MoveCtrl.SetGravity(
  unitID: integer,
  gravityFactor: number
)
```












### MoveCtrl.SetDrag
---
```lua
function MoveCtrl.SetDrag(
  unitID: integer,
  drag: number
)
```












### MoveCtrl.SetWindFactor
---
```lua
function MoveCtrl.SetWindFactor(
  unitID: integer,
  windFactor: number
)
```












### MoveCtrl.SetLimits
---
```lua
function MoveCtrl.SetLimits(
  unitID: integer,
  minX: number,
  minY: number,
  minZ: number,
  maxX: number,
  maxY: number,
  maxZ: number
)
```
@param `minX` - Minimum position X component.

@param `minY` - Minimum position Y component.

@param `minZ` - Minimum position Z component.

@param `maxX` - Maximum position X component.

@param `maxY` - Maximum position Y component.

@param `maxZ` - Maximum position Z component.













### MoveCtrl.SetNoBlocking
---
```lua
function MoveCtrl.SetNoBlocking(
  unitID: integer,
  noBlocking: boolean
)
```












### MoveCtrl.SetCollideStop
---
```lua
function MoveCtrl.SetCollideStop(
  unitID: integer,
  collideStop: boolean
)
```












### MoveCtrl.SetLimitsStop
---
```lua
function MoveCtrl.SetLimitsStop(
  unitID: integer,
  limitsStop: boolean
)
```












### MoveCtrl.SetGunshipMoveTypeData
---
```lua
function MoveCtrl.SetGunshipMoveTypeData(
  unitID: integer,
  moveType: HoverAirMoveType
) -> numAssignedValues number
```












### MoveCtrl.SetGunshipMoveTypeData
---
```lua
function MoveCtrl.SetGunshipMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeBooleanKey|"collide"|"dontLand"|"airStrafe"|"useSmoothMesh"|"bankingAllowed"),
  value: boolean
) -> numAssignedValues number
```












### MoveCtrl.SetGunshipMoveTypeData
---
```lua
function MoveCtrl.SetGunshipMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeNumberKey|"wantedHeight"|"accRate"|"decRate"|"turnRate"|"altitudeRate"|"currentBank"|"currentPitch"...),
  value: number
) -> numAssignedValues number
```












### MoveCtrl.SetAirMoveTypeData
---
```lua
function MoveCtrl.SetAirMoveTypeData(
  unitID: integer,
  moveType: StrafeAirMoveType
) -> numAssignedValues number
```












### MoveCtrl.SetAirMoveTypeData
---
```lua
function MoveCtrl.SetAirMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeBooleanKey|"collide"|"useSmoothMesh"|"loopbackAttack"),
  value: boolean
) -> numAssignedValues number
```












### MoveCtrl.SetAirMoveTypeData
---
```lua
function MoveCtrl.SetAirMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeNumberKey|"wantedHeight"|"turnRadius"|"accRate"|"decRate"|"maxAcc"|"maxDec"|"maxBank"...),
  value: number
) -> numAssignedValues number
```












### MoveCtrl.SetAirMoveTypeData
---
```lua
function MoveCtrl.SetAirMoveTypeData(
  unitID: integer,
  key: ("maneuverBlockTime"),
  value: integer
) -> numAssignedValues number
```












### MoveCtrl.SetGroundMoveTypeData
---
```lua
function MoveCtrl.SetGroundMoveTypeData(
  unitID: integer,
  moveType: GroundMoveType
) -> numAssignedValues number
```












### MoveCtrl.SetGroundMoveTypeData
---
```lua
function MoveCtrl.SetGroundMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeBooleanKey|"atGoal"|"atEndOfPath"|"pushResistant"),
  value: boolean
) -> numAssignedValues number
```












### MoveCtrl.SetGroundMoveTypeData
---
```lua
function MoveCtrl.SetGroundMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeNumberKey|"turnRate"|"turnAccel"|"accRate"|"decRate"|"myGravity"|"maxReverseDist"|"minReverseAngle"...),
  value: number
) -> numAssignedValues number
```












### MoveCtrl.SetGroundMoveTypeData
---
```lua
function MoveCtrl.SetGroundMoveTypeData(
  unitID: integer,
  key: ("minScriptChangeHeading"),
  value: integer
) -> numAssignedValues number
```












### MoveCtrl.SetMoveDef
---
```lua
function MoveCtrl.SetMoveDef(
  unitID: integer,
  moveDef: (integer|string)
) -> success boolean
```
@param `moveDef` - Name or path type of the MoveDef.


@return `success` - `true` if the `MoveDef` was set, `false` if `unitID` or `moveDef` were invalid, or if the unit does not support a `MoveDef`.












### MoveCtrl.IsEnabled
---
```lua
function MoveCtrl.IsEnabled(unitID: integer) -> isEnabled boolean?
```












### MoveCtrl.Enable
---
```lua
function MoveCtrl.Enable(unitID: integer)
```












### MoveCtrl.Disable
---
```lua
function MoveCtrl.Disable(unitID: integer)
```












### MoveCtrl.SetTag
---
```lua
function MoveCtrl.SetTag(
  unitID: integer,
  tag: integer
)
```












### MoveCtrl.GetTag
---
```lua
function MoveCtrl.GetTag(tag: integer?)
```












### MoveCtrl.SetProgressState
---
```lua
function MoveCtrl.SetProgressState(
  unitID: integer,
  state: (0|1|2|"done"|"active"|"failed")
)
```












### MoveCtrl.SetExtrapolate
---
```lua
function MoveCtrl.SetExtrapolate(
  unitID: integer,
  extrapolate: boolean
)
```












### MoveCtrl.SetPhysics
---
```lua
function MoveCtrl.SetPhysics(
  unitID: integer,
  posX: number,
  posY: number,
  posZ: number,
  velX: number,
  velY: number,
  velZ: number,
  rotX: number,
  rotY: number,
  rotZ: number
)
```
@param `posX` - Position X component.

@param `posY` - Position Y component.

@param `posZ` - Position Z component.

@param `velX` - Velocity X component.

@param `velY` - Velocity Y component.

@param `velZ` - Velocity Z component.

@param `rotX` - Rotation X component.

@param `rotY` - Rotation Y component.

@param `rotZ` - Rotation Z component.













### MoveCtrl.SetPosition
---
```lua
function MoveCtrl.SetPosition(
  unitID: integer,
  posX: number,
  posY: number,
  posZ: number
)
```
@param `posX` - Position X component.

@param `posY` - Position Y component.

@param `posZ` - Position Z component.













### MoveCtrl.SetVelocity
---
```lua
function MoveCtrl.SetVelocity(
  unitID: integer,
  velX: number,
  velY: number,
  velZ: number
)
```
@param `velX` - Velocity X component.

@param `velY` - Velocity Y component.

@param `velZ` - Velocity Z component.













### MoveCtrl.SetRelativeVelocity
---
```lua
function MoveCtrl.SetRelativeVelocity(
  unitID: integer,
  relVelX: number,
  relVelY: number,
  relVelZ: number
)
```
@param `relVelX` - Relative velocity X component.

@param `relVelY` - Relative velocity Y component.

@param `relVelZ` - Relative velocity Z component.













### MoveCtrl.SetRotation
---
```lua
function MoveCtrl.SetRotation(
  unitID: integer,
  rotX: number,
  rotY: number,
  rotZ: number
)
```
@param `rotX` - Rotation X component.

@param `rotY` - Rotation Y component.

@param `rotZ` - Rotation Z component.













### MoveCtrl.SetRotationOffset
---
```lua
function MoveCtrl.SetRotationOffset()
```







@deprecated Deprecated






### MoveCtrl.SetRotationVelocity
---
```lua
function MoveCtrl.SetRotationVelocity(
  unitID: integer,
  rotVelX: number,
  rotVelY: number,
  rotVelZ: number
)
```
@param `rotVelX` - Rotation velocity X component.

@param `rotVelY` - Rotation velocity Y component.

@param `rotVelZ` - Rotation velocity Z component.













### MoveCtrl.SetHeading
---
```lua
function MoveCtrl.SetHeading(
  unitID: integer,
  heading: Heading
)
```












### MoveCtrl.SetTrackSlope
---
```lua
function MoveCtrl.SetTrackSlope(
  unitID: integer,
  trackSlope: boolean
)
```












### MoveCtrl.SetTrackGround
---
```lua
function MoveCtrl.SetTrackGround(
  unitID: integer,
  trackGround: boolean
)
```












### MoveCtrl.SetTrackLimits
---
```lua
function MoveCtrl.SetTrackLimits(
  unitID: integer,
  trackLimits: boolean
)
```












### MoveCtrl.SetGroundOffset
---
```lua
function MoveCtrl.SetGroundOffset(
  unitID: integer,
  groundOffset: number
)
```












### MoveCtrl.SetGravity
---
```lua
function MoveCtrl.SetGravity(
  unitID: integer,
  gravityFactor: number
)
```












### MoveCtrl.SetDrag
---
```lua
function MoveCtrl.SetDrag(
  unitID: integer,
  drag: number
)
```












### MoveCtrl.SetWindFactor
---
```lua
function MoveCtrl.SetWindFactor(
  unitID: integer,
  windFactor: number
)
```












### MoveCtrl.SetLimits
---
```lua
function MoveCtrl.SetLimits(
  unitID: integer,
  minX: number,
  minY: number,
  minZ: number,
  maxX: number,
  maxY: number,
  maxZ: number
)
```
@param `minX` - Minimum position X component.

@param `minY` - Minimum position Y component.

@param `minZ` - Minimum position Z component.

@param `maxX` - Maximum position X component.

@param `maxY` - Maximum position Y component.

@param `maxZ` - Maximum position Z component.













### MoveCtrl.SetNoBlocking
---
```lua
function MoveCtrl.SetNoBlocking(
  unitID: integer,
  noBlocking: boolean
)
```












### MoveCtrl.SetCollideStop
---
```lua
function MoveCtrl.SetCollideStop(
  unitID: integer,
  collideStop: boolean
)
```












### MoveCtrl.SetLimitsStop
---
```lua
function MoveCtrl.SetLimitsStop(
  unitID: integer,
  limitsStop: boolean
)
```












### MoveCtrl.SetGunshipMoveTypeData
---
```lua
function MoveCtrl.SetGunshipMoveTypeData(
  unitID: integer,
  moveType: HoverAirMoveType
) -> numAssignedValues number
```












### MoveCtrl.SetGunshipMoveTypeData
---
```lua
function MoveCtrl.SetGunshipMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeBooleanKey|"collide"|"dontLand"|"airStrafe"|"useSmoothMesh"|"bankingAllowed"),
  value: boolean
) -> numAssignedValues number
```












### MoveCtrl.SetGunshipMoveTypeData
---
```lua
function MoveCtrl.SetGunshipMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeNumberKey|"wantedHeight"|"accRate"|"decRate"|"turnRate"|"altitudeRate"|"currentBank"|"currentPitch"...),
  value: number
) -> numAssignedValues number
```












### MoveCtrl.SetAirMoveTypeData
---
```lua
function MoveCtrl.SetAirMoveTypeData(
  unitID: integer,
  moveType: StrafeAirMoveType
) -> numAssignedValues number
```












### MoveCtrl.SetAirMoveTypeData
---
```lua
function MoveCtrl.SetAirMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeBooleanKey|"collide"|"useSmoothMesh"|"loopbackAttack"),
  value: boolean
) -> numAssignedValues number
```












### MoveCtrl.SetAirMoveTypeData
---
```lua
function MoveCtrl.SetAirMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeNumberKey|"wantedHeight"|"turnRadius"|"accRate"|"decRate"|"maxAcc"|"maxDec"|"maxBank"...),
  value: number
) -> numAssignedValues number
```












### MoveCtrl.SetAirMoveTypeData
---
```lua
function MoveCtrl.SetAirMoveTypeData(
  unitID: integer,
  key: ("maneuverBlockTime"),
  value: integer
) -> numAssignedValues number
```












### MoveCtrl.SetGroundMoveTypeData
---
```lua
function MoveCtrl.SetGroundMoveTypeData(
  unitID: integer,
  moveType: GroundMoveType
) -> numAssignedValues number
```












### MoveCtrl.SetGroundMoveTypeData
---
```lua
function MoveCtrl.SetGroundMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeBooleanKey|"atGoal"|"atEndOfPath"|"pushResistant"),
  value: boolean
) -> numAssignedValues number
```












### MoveCtrl.SetGroundMoveTypeData
---
```lua
function MoveCtrl.SetGroundMoveTypeData(
  unitID: integer,
  key: (GenericMoveTypeNumberKey|"turnRate"|"turnAccel"|"accRate"|"decRate"|"myGravity"|"maxReverseDist"|"minReverseAngle"...),
  value: number
) -> numAssignedValues number
```












### MoveCtrl.SetGroundMoveTypeData
---
```lua
function MoveCtrl.SetGroundMoveTypeData(
  unitID: integer,
  key: ("minScriptChangeHeading"),
  value: integer
) -> numAssignedValues number
```












### MoveCtrl.SetMoveDef
---
```lua
function MoveCtrl.SetMoveDef(
  unitID: integer,
  moveDef: (integer|string)
) -> success boolean
```
@param `moveDef` - Name or path type of the MoveDef.


@return `success` - `true` if the `MoveDef` was set, `false` if `unitID` or `moveDef` were invalid, or if the unit does not support a `MoveDef`.















