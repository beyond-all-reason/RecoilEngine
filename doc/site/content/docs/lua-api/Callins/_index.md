---
title: "Callins"
permalink: "docs/lua-api/Callins"
---
# class Callins





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








### Callins.LoadCode
---
```lua
function Callins.LoadCode()
```





Called when the game is (re)loaded.








### Callins.Shutdown
---
```lua
function Callins.Shutdown() ->  nil
```





Called when the addon or the game is shutdown.








### Callins.GotChatMsg
---
```lua
function Callins.GotChatMsg(
  msg: string,
  playerID: integer
)
```





Called when a player issues a UI command e.g. types /foo or /luarules foo.








### Callins.Load
---
```lua
function Callins.Load(zipReader: table)
```





Called after `GamePreload` and before `GameStart`. See Lua_SaveLoad.








### Callins.GamePreload
---
```lua
function Callins.GamePreload()
```





Called before the 0 gameframe.

Is not called when a saved game is loaded.








### Callins.GameStart
---
```lua
function Callins.GameStart()
```





Called upon the start of the game.

Is not called when a saved game is loaded.








### Callins.GameOver
---
```lua
function Callins.GameOver(winningAllyTeams: number[])
```
@param `winningAllyTeams` - list of winning allyTeams, if empty the game result was undecided (like when dropping from an host).






Called when the game ends








### Callins.GamePaused
---
```lua
function Callins.GamePaused(
  playerID: integer,
  paused: boolean
)
```





Called when the game is paused.








### Callins.GameFrame
---
```lua
function Callins.GameFrame(frame: number)
```
@param `frame` - Starts at frame 1






Called for every game simulation frame (30 per second).








### Callins.GameFramePost
---
```lua
function Callins.GameFramePost(frame: number)
```
@param `frame` - Starts at frame 1






Called at the end of every game simulation frame








### Callins.GameID
---
```lua
function Callins.GameID(gameID: string)
```
@param `gameID` - encoded in hex.






Called once to deliver the gameID








### Callins.TeamDied
---
```lua
function Callins.TeamDied(teamID: integer)
```





Called when a team dies (see `Spring.KillTeam`).








### Callins.TeamChanged
---
```lua
function Callins.TeamChanged(teamID: integer)
```












### Callins.PlayerChanged
---
```lua
function Callins.PlayerChanged(playerID: integer)
```





Called whenever a player's status changes e.g. becoming a spectator.








### Callins.PlayerAdded
---
```lua
function Callins.PlayerAdded(playerID: integer)
```





Called whenever a new player joins the game.








### Callins.PlayerRemoved
---
```lua
function Callins.PlayerRemoved(
  playerID: integer,
  reason: string
)
```





Called whenever a player is removed from the game.








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








### Callins.UnitEnteredUnderwater
---
```lua
function Callins.UnitEnteredUnderwater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```












### Callins.UnitEnteredWater
---
```lua
function Callins.UnitEnteredWater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```












### Callins.UnitLeftAir
---
```lua
function Callins.UnitLeftAir(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```












### Callins.UnitLeftUnderwater
---
```lua
function Callins.UnitLeftUnderwater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```












### Callins.UnitLeftWater
---
```lua
function Callins.UnitLeftWater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```












### Callins.UnitEnteredAir
---
```lua
function Callins.UnitEnteredAir(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```












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








### Callins.UnitFeatureCollision
---
```lua
function Callins.UnitFeatureCollision(
  colliderID: integer,
  collideeID: integer
)
```





Called when a unit collides with a feature.

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












### Callins.UnitArrivedAtGoal
---
```lua
function Callins.UnitArrivedAtGoal(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```












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








### Callins.FeatureCreated
---
```lua
function Callins.FeatureCreated(
  featureID: integer,
  allyTeamID: integer
)
```





Called when a feature is created.








### Callins.FeatureDestroyed
---
```lua
function Callins.FeatureDestroyed(
  featureID: integer,
  allyTeamID: integer
)
```





Called when a feature is destroyed.








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

Note that weaponDefID is missing if the projectile is spawned as part of a burst, but `Spring.GetProjectileDefID` and `Spring.GetProjectileName` still work in callin scope using proID.








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








### Callins.RecvLuaMsg
---
```lua
function Callins.RecvLuaMsg(
  msg: string,
  playerID: integer
)
```





Receives messages from unsynced sent via `Spring.SendLuaRulesMsg` or `Spring.SendLuaUIMsg`.








### Callins.Save
---
```lua
function Callins.Save(zip: table)
```
@param `zip` - a userdatum representing the savegame zip file. See Lua_SaveLoad.






Called when a chat command '/save' or '/savegame' is received.








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








### Callins.Update
---
```lua
function Callins.Update(dt: number)
```
@param `dt` - the time since the last update.






Called for every draw frame (including when the game is paused) and at least once per sim frame except when catching up.








### Callins.ViewResize
---
```lua
function Callins.ViewResize(
  viewSizeX: number,
  viewSizeY: number
)
```





Called whenever the window is resized.








### Callins.FontsChanged
---
```lua
function Callins.FontsChanged()
```





Called whenever fonts are updated. Signals the game display lists
and other caches should be discarded.

Gets called before other Update and Draw callins.








### Callins.SunChanged
---
```lua
function Callins.SunChanged()
```












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








### Callins.DrawGenesis
---
```lua
function Callins.DrawGenesis()
```





Use this callin to update textures, shaders, etc.

Doesn't render to screen!
Also available to LuaMenu.








### Callins.DrawWorld
---
```lua
function Callins.DrawWorld()
```





Spring draws command queues, 'map stuff', and map marks.








### Callins.DrawWorldPreUnit
---
```lua
function Callins.DrawWorldPreUnit()
```





Spring draws units, features, some water types, cloaked units, and the sun.








### Callins.DrawPreDecals
---
```lua
function Callins.DrawPreDecals()
```





Called before decals are drawn








### Callins.DrawWaterPost
---
```lua
function Callins.DrawWaterPost()
```












### Callins.DrawShadowPassTransparent
---
```lua
function Callins.DrawShadowPassTransparent()
```





Invoked after semi-transparent shadows pass is about to conclude

This callin has depth and color buffer of shadowmap bound via FBO as well as the FFP state to do "semi-transparent" shadows pass (traditionally only used to draw shadows of shadow casting semi-transparent particles). Can be used to draw nice colored shadows.








### Callins.DrawWorldShadow
---
```lua
function Callins.DrawWorldShadow()
```












### Callins.DrawWorldReflection
---
```lua
function Callins.DrawWorldReflection()
```












### Callins.DrawWorldRefraction
---
```lua
function Callins.DrawWorldRefraction()
```












### Callins.DrawGroundPreForward
---
```lua
function Callins.DrawGroundPreForward()
```





Runs at the start of the forward pass when a custom map shader has been assigned via `Spring.SetMapShader` (convenient for setting uniforms).








### Callins.DrawGroundPostForward
---
```lua
function Callins.DrawGroundPostForward()
```












### Callins.DrawGroundPreDeferred
---
```lua
function Callins.DrawGroundPreDeferred()
```





Runs at the start of the deferred pass when a custom map shader has been assigned via `Spring.SetMapShader` (convenient for setting uniforms).








### Callins.DrawGroundDeferred
---
```lua
function Callins.DrawGroundDeferred()
```












### Callins.DrawGroundPostDeferred
---
```lua
function Callins.DrawGroundPostDeferred()
```





This runs at the end of its respective deferred pass.

Allows proper frame compositing (with ground flashes/decals/foliage/etc, which are drawn between it and `DrawWorldPreUnit`) via `gl.CopyToTexture`.








### Callins.DrawUnitsPostDeferred
---
```lua
function Callins.DrawUnitsPostDeferred()
```





Runs at the end of the unit deferred pass.

Informs Lua code it should make use of the $model_gbuffer_* textures before another pass overwrites them (and to allow proper blending with e.g. cloaked objects which are drawn between these events and DrawWorld via gl.CopyToTexture). N.B. The *PostDeferred events are only sent (and only have a real purpose) if forward drawing is disabled.








### Callins.DrawFeaturesPostDeferred
---
```lua
function Callins.DrawFeaturesPostDeferred()
```





Runs at the end of the feature deferred pass to inform Lua code it should make use of the $model_gbuffer_* textures before another pass overwrites them (and to allow proper blending with e.g. cloaked objects which are drawn between these events and DrawWorld via gl.CopyToTexture). N.B. The *PostDeferred events are only sent (and only have a real purpose) if forward drawing is disabled.








### Callins.DrawShadowUnitsLua
---
```lua
function Callins.DrawShadowUnitsLua()
```












### Callins.DrawShadowFeaturesLua
---
```lua
function Callins.DrawShadowFeaturesLua()
```












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








### Callins.DrawScreen
---
```lua
function Callins.DrawScreen(
  viewSizeX: number,
  viewSizeY: number
)
```





Also available to LuaMenu.








### Callins.DrawScreenEffects
---
```lua
function Callins.DrawScreenEffects(
  viewSizeX: number,
  viewSizeY: number
)
```












### Callins.DrawScreenPost
---
```lua
function Callins.DrawScreenPost(
  viewSizeX: number,
  viewSizeY: number
)
```





Similar to DrawScreenEffects, this can be used to alter the contents of a frame after it has been completely rendered (i.e. World, MiniMap, Menu, UI).








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













### Callins.GameProgress
---
```lua
function Callins.GameProgress(serverFrameNum: integer)
```





Called every 60 frames, calculating delta between `GameFrame` and `GameProgress`.

Can give an ETA about catching up with simulation for mid-game join players.








### Callins.KeyMapChanged
---
```lua
function Callins.KeyMapChanged()
```





Called when the keymap changes

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








### Callins.TextInput
---
```lua
function Callins.TextInput(utf8char: string)
```





Called whenever a key press results in text input.








### Callins.TextEditing
---
```lua
function Callins.TextEditing(
  utf8: string,
  start: number,
  length: number
)
```












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








### Callins.IsAbove
---
```lua
function Callins.IsAbove(
  x: number,
  y: number
) -> isAbove boolean
```





Called every `Update`.

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








### Callins.ActiveCommandChanged
---
```lua
function Callins.ActiveCommandChanged(
  cmdId: integer?,
  cmdType: integer?
)
```





Called when a command is issued.








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








### Callins.AddConsoleLine
---
```lua
function Callins.AddConsoleLine(
  msg: string,
  priority: integer
)
```





Called when text is entered into the console (e.g. `Spring.Echo`).








### Callins.GroupChanged
---
```lua
function Callins.GroupChanged(groupID: integer)
```





Called when a unit is added to or removed from a control group.








### Callins.WorldTooltip
---
```lua
function Callins.WorldTooltip(
  type: "unit",
  unitId: integer
) -> tooltip string
```












### Callins.WorldTooltip
---
```lua
function Callins.WorldTooltip(
  type: "feature",
  featureId: integer
) -> tooltip string
```












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












### Callins.WorldTooltip
---
```lua
function Callins.WorldTooltip(type: "selection") -> tooltip string
```












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












### Callins.RecvSkirmishAIMessage
---
```lua
function Callins.RecvSkirmishAIMessage(
  aiTeam: integer,
  dataStr: string
)
```












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








### Callins.DownloadStarted
---
```lua
function Callins.DownloadStarted(id: integer)
```





Called when a Pr-downloader download is started via VFS.DownloadArchive.








### Callins.DownloadFinished
---
```lua
function Callins.DownloadFinished(id: integer)
```





Called when a Pr-downloader download finishes successfully.








### Callins.DownloadFailed
---
```lua
function Callins.DownloadFailed(
  id: integer,
  errorID: integer
)
```





Called when a Pr-downloader download fails to complete.








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








### Callins.Initialize
---
```lua
function Callins.Initialize()
```





Called when the addon is (re)loaded.








### Callins.LoadCode
---
```lua
function Callins.LoadCode()
```





Called when the game is (re)loaded.








### Callins.Shutdown
---
```lua
function Callins.Shutdown() ->  nil
```





Called when the addon or the game is shutdown.








### Callins.GotChatMsg
---
```lua
function Callins.GotChatMsg(
  msg: string,
  playerID: integer
)
```





Called when a player issues a UI command e.g. types /foo or /luarules foo.








### Callins.Load
---
```lua
function Callins.Load(zipReader: table)
```





Called after `GamePreload` and before `GameStart`. See Lua_SaveLoad.








### Callins.GamePreload
---
```lua
function Callins.GamePreload()
```





Called before the 0 gameframe.

Is not called when a saved game is loaded.








### Callins.GameStart
---
```lua
function Callins.GameStart()
```





Called upon the start of the game.

Is not called when a saved game is loaded.








### Callins.GameOver
---
```lua
function Callins.GameOver(winningAllyTeams: number[])
```
@param `winningAllyTeams` - list of winning allyTeams, if empty the game result was undecided (like when dropping from an host).






Called when the game ends








### Callins.GamePaused
---
```lua
function Callins.GamePaused(
  playerID: integer,
  paused: boolean
)
```





Called when the game is paused.








### Callins.GameFrame
---
```lua
function Callins.GameFrame(frame: number)
```
@param `frame` - Starts at frame 1






Called for every game simulation frame (30 per second).








### Callins.GameFramePost
---
```lua
function Callins.GameFramePost(frame: number)
```
@param `frame` - Starts at frame 1






Called at the end of every game simulation frame








### Callins.GameID
---
```lua
function Callins.GameID(gameID: string)
```
@param `gameID` - encoded in hex.






Called once to deliver the gameID








### Callins.TeamDied
---
```lua
function Callins.TeamDied(teamID: integer)
```





Called when a team dies (see `Spring.KillTeam`).








### Callins.TeamChanged
---
```lua
function Callins.TeamChanged(teamID: integer)
```












### Callins.PlayerChanged
---
```lua
function Callins.PlayerChanged(playerID: integer)
```





Called whenever a player's status changes e.g. becoming a spectator.








### Callins.PlayerAdded
---
```lua
function Callins.PlayerAdded(playerID: integer)
```





Called whenever a new player joins the game.








### Callins.PlayerRemoved
---
```lua
function Callins.PlayerRemoved(
  playerID: integer,
  reason: string
)
```





Called whenever a player is removed from the game.








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








### Callins.UnitEnteredUnderwater
---
```lua
function Callins.UnitEnteredUnderwater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```












### Callins.UnitEnteredWater
---
```lua
function Callins.UnitEnteredWater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```












### Callins.UnitLeftAir
---
```lua
function Callins.UnitLeftAir(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```












### Callins.UnitLeftUnderwater
---
```lua
function Callins.UnitLeftUnderwater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```












### Callins.UnitLeftWater
---
```lua
function Callins.UnitLeftWater(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```












### Callins.UnitEnteredAir
---
```lua
function Callins.UnitEnteredAir(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```












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








### Callins.UnitFeatureCollision
---
```lua
function Callins.UnitFeatureCollision(
  colliderID: integer,
  collideeID: integer
)
```





Called when a unit collides with a feature.

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












### Callins.UnitArrivedAtGoal
---
```lua
function Callins.UnitArrivedAtGoal(
  unitID: integer,
  unitDefID: integer,
  unitTeam: integer
)
```












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








### Callins.FeatureCreated
---
```lua
function Callins.FeatureCreated(
  featureID: integer,
  allyTeamID: integer
)
```





Called when a feature is created.








### Callins.FeatureDestroyed
---
```lua
function Callins.FeatureDestroyed(
  featureID: integer,
  allyTeamID: integer
)
```





Called when a feature is destroyed.








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

Note that weaponDefID is missing if the projectile is spawned as part of a burst, but `Spring.GetProjectileDefID` and `Spring.GetProjectileName` still work in callin scope using proID.








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








### Callins.RecvLuaMsg
---
```lua
function Callins.RecvLuaMsg(
  msg: string,
  playerID: integer
)
```





Receives messages from unsynced sent via `Spring.SendLuaRulesMsg` or `Spring.SendLuaUIMsg`.








### Callins.Save
---
```lua
function Callins.Save(zip: table)
```
@param `zip` - a userdatum representing the savegame zip file. See Lua_SaveLoad.






Called when a chat command '/save' or '/savegame' is received.








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








### Callins.Update
---
```lua
function Callins.Update(dt: number)
```
@param `dt` - the time since the last update.






Called for every draw frame (including when the game is paused) and at least once per sim frame except when catching up.








### Callins.ViewResize
---
```lua
function Callins.ViewResize(
  viewSizeX: number,
  viewSizeY: number
)
```





Called whenever the window is resized.








### Callins.FontsChanged
---
```lua
function Callins.FontsChanged()
```





Called whenever fonts are updated. Signals the game display lists
and other caches should be discarded.

Gets called before other Update and Draw callins.








### Callins.SunChanged
---
```lua
function Callins.SunChanged()
```












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








### Callins.DrawGenesis
---
```lua
function Callins.DrawGenesis()
```





Use this callin to update textures, shaders, etc.

Doesn't render to screen!
Also available to LuaMenu.








### Callins.DrawWorld
---
```lua
function Callins.DrawWorld()
```





Spring draws command queues, 'map stuff', and map marks.








### Callins.DrawWorldPreUnit
---
```lua
function Callins.DrawWorldPreUnit()
```





Spring draws units, features, some water types, cloaked units, and the sun.








### Callins.DrawPreDecals
---
```lua
function Callins.DrawPreDecals()
```





Called before decals are drawn








### Callins.DrawWaterPost
---
```lua
function Callins.DrawWaterPost()
```












### Callins.DrawShadowPassTransparent
---
```lua
function Callins.DrawShadowPassTransparent()
```





Invoked after semi-transparent shadows pass is about to conclude

This callin has depth and color buffer of shadowmap bound via FBO as well as the FFP state to do "semi-transparent" shadows pass (traditionally only used to draw shadows of shadow casting semi-transparent particles). Can be used to draw nice colored shadows.








### Callins.DrawWorldShadow
---
```lua
function Callins.DrawWorldShadow()
```












### Callins.DrawWorldReflection
---
```lua
function Callins.DrawWorldReflection()
```












### Callins.DrawWorldRefraction
---
```lua
function Callins.DrawWorldRefraction()
```












### Callins.DrawGroundPreForward
---
```lua
function Callins.DrawGroundPreForward()
```





Runs at the start of the forward pass when a custom map shader has been assigned via `Spring.SetMapShader` (convenient for setting uniforms).








### Callins.DrawGroundPostForward
---
```lua
function Callins.DrawGroundPostForward()
```












### Callins.DrawGroundPreDeferred
---
```lua
function Callins.DrawGroundPreDeferred()
```





Runs at the start of the deferred pass when a custom map shader has been assigned via `Spring.SetMapShader` (convenient for setting uniforms).








### Callins.DrawGroundDeferred
---
```lua
function Callins.DrawGroundDeferred()
```












### Callins.DrawGroundPostDeferred
---
```lua
function Callins.DrawGroundPostDeferred()
```





This runs at the end of its respective deferred pass.

Allows proper frame compositing (with ground flashes/decals/foliage/etc, which are drawn between it and `DrawWorldPreUnit`) via `gl.CopyToTexture`.








### Callins.DrawUnitsPostDeferred
---
```lua
function Callins.DrawUnitsPostDeferred()
```





Runs at the end of the unit deferred pass.

Informs Lua code it should make use of the $model_gbuffer_* textures before another pass overwrites them (and to allow proper blending with e.g. cloaked objects which are drawn between these events and DrawWorld via gl.CopyToTexture). N.B. The *PostDeferred events are only sent (and only have a real purpose) if forward drawing is disabled.








### Callins.DrawFeaturesPostDeferred
---
```lua
function Callins.DrawFeaturesPostDeferred()
```





Runs at the end of the feature deferred pass to inform Lua code it should make use of the $model_gbuffer_* textures before another pass overwrites them (and to allow proper blending with e.g. cloaked objects which are drawn between these events and DrawWorld via gl.CopyToTexture). N.B. The *PostDeferred events are only sent (and only have a real purpose) if forward drawing is disabled.








### Callins.DrawShadowUnitsLua
---
```lua
function Callins.DrawShadowUnitsLua()
```












### Callins.DrawShadowFeaturesLua
---
```lua
function Callins.DrawShadowFeaturesLua()
```












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








### Callins.DrawScreen
---
```lua
function Callins.DrawScreen(
  viewSizeX: number,
  viewSizeY: number
)
```





Also available to LuaMenu.








### Callins.DrawScreenEffects
---
```lua
function Callins.DrawScreenEffects(
  viewSizeX: number,
  viewSizeY: number
)
```












### Callins.DrawScreenPost
---
```lua
function Callins.DrawScreenPost(
  viewSizeX: number,
  viewSizeY: number
)
```





Similar to DrawScreenEffects, this can be used to alter the contents of a frame after it has been completely rendered (i.e. World, MiniMap, Menu, UI).








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













### Callins.GameProgress
---
```lua
function Callins.GameProgress(serverFrameNum: integer)
```





Called every 60 frames, calculating delta between `GameFrame` and `GameProgress`.

Can give an ETA about catching up with simulation for mid-game join players.








### Callins.KeyMapChanged
---
```lua
function Callins.KeyMapChanged()
```





Called when the keymap changes

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








### Callins.TextInput
---
```lua
function Callins.TextInput(utf8char: string)
```





Called whenever a key press results in text input.








### Callins.TextEditing
---
```lua
function Callins.TextEditing(
  utf8: string,
  start: number,
  length: number
)
```












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








### Callins.IsAbove
---
```lua
function Callins.IsAbove(
  x: number,
  y: number
) -> isAbove boolean
```





Called every `Update`.

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








### Callins.ActiveCommandChanged
---
```lua
function Callins.ActiveCommandChanged(
  cmdId: integer?,
  cmdType: integer?
)
```





Called when a command is issued.








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








### Callins.AddConsoleLine
---
```lua
function Callins.AddConsoleLine(
  msg: string,
  priority: integer
)
```





Called when text is entered into the console (e.g. `Spring.Echo`).








### Callins.GroupChanged
---
```lua
function Callins.GroupChanged(groupID: integer)
```





Called when a unit is added to or removed from a control group.








### Callins.WorldTooltip
---
```lua
function Callins.WorldTooltip(
  type: "unit",
  unitId: integer
) -> tooltip string
```












### Callins.WorldTooltip
---
```lua
function Callins.WorldTooltip(
  type: "feature",
  featureId: integer
) -> tooltip string
```












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












### Callins.WorldTooltip
---
```lua
function Callins.WorldTooltip(type: "selection") -> tooltip string
```












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












### Callins.RecvSkirmishAIMessage
---
```lua
function Callins.RecvSkirmishAIMessage(
  aiTeam: integer,
  dataStr: string
)
```












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








### Callins.DownloadStarted
---
```lua
function Callins.DownloadStarted(id: integer)
```





Called when a Pr-downloader download is started via VFS.DownloadArchive.








### Callins.DownloadFinished
---
```lua
function Callins.DownloadFinished(id: integer)
```





Called when a Pr-downloader download finishes successfully.








### Callins.DownloadFailed
---
```lua
function Callins.DownloadFailed(
  id: integer,
  errorID: integer
)
```





Called when a Pr-downloader download fails to complete.








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











