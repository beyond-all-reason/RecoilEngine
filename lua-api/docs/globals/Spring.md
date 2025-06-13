---
layout: default
title: Spring
parent: Lua API
permalink: lua-api/globals/Spring
---

{% raw %}

# global Spring








---

## methods
---

### Spring.IsReplay
---
```lua
function Spring.IsReplay() -> isReplay boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L467-L472" target="_blank">source</a>]








### Spring.GetReplayLength
---
```lua
function Spring.GetReplayLength() -> timeInSeconds number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L480-L485" target="_blank">source</a>]








### Spring.GetGameName
---
```lua
function Spring.GetGameName() -> name string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L500-L505" target="_blank">source</a>]








### Spring.GetMenuName
---
```lua
function Spring.GetMenuName() -> name string
```

@return `name` - name .. version from Modinfo.lua. E.g. "Spring: 1944 test-5640-ac2d15b".





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L512-L517" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L531-L544" target="_blank">source</a>]








### Spring.GetProfilerRecordNames
---
```lua
function Spring.GetProfilerRecordNames() -> profilerNames string[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L568-L573" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L590-L602" target="_blank">source</a>]








### Spring.GetVidMemUsage
---
```lua
function Spring.GetVidMemUsage()
 -> usedMem number
 -> availableMem number

```

@return `usedMem` - in MB

@return `availableMem` - in MB





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L641-L647" target="_blank">source</a>]








### Spring.GetTimer
---
```lua
function Spring.GetTimer() ->  integer
```





Get a timer with millisecond resolution

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L695-L699" target="_blank">source</a>]








### Spring.GetTimerMicros
---
```lua
function Spring.GetTimerMicros() ->  integer
```





Get a timer with microsecond resolution

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L707-L711" target="_blank">source</a>]








### Spring.GetFrameTimer
---
```lua
function Spring.GetFrameTimer(lastFrameTime: boolean?) ->  integer
```
@param `lastFrameTime` - (Default: `false`) whether to use last frame time instead of last frame start






Get a timer for the start of the frame

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L719-L727" target="_blank">source</a>]

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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L739-L747" target="_blank">source</a>]








### Spring.GetNumDisplays
---
```lua
function Spring.GetNumDisplays() -> numDisplays number
```

@return `numDisplays` - as returned by `SDL_GetNumVideoDisplays`





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L797-L802" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L810-L818" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L829-L837" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L848-L860" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L878-L885" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L901-L920" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L958-L968" target="_blank">source</a>]








### Spring.GetMiniMapRotation
---
```lua
function Spring.GetMiniMapRotation() -> amount number
```

@return `amount` - in radians





Get minimap rotation

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L985-L989" target="_blank">source</a>]








### Spring.GetMiniMapDualScreen
---
```lua
function Spring.GetMiniMapDualScreen() -> position ("left"|"right"|false)
```

@return `position` - `"left"` or `"right"` when dual screen is enabled, otherwise `false`.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1001-L1004" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1023-L1035" target="_blank">source</a>]

Returns nil when selection box is inactive





@see Spring.GetUnitsInScreenRectangle




### Spring.GetDrawSelectionInfo
---
```lua
function Spring.GetDrawSelectionInfo() ->  boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1057-L1061" target="_blank">source</a>]








### Spring.IsAboveMiniMap
---
```lua
function Spring.IsAboveMiniMap(
  x: number,
  y: number
) -> isAbove boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1069-L1077" target="_blank">source</a>]








### Spring.GetDrawFrame
---
```lua
function Spring.GetDrawFrame()
 -> low_16bit number
 -> high_16bit number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1101-L1107" target="_blank">source</a>]








### Spring.GetFrameTimeOffset
---
```lua
function Spring.GetFrameTimeOffset() -> offset number?
```

@return `offset` - of the current draw frame from the last sim frame, expressed in fractions of a frame





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1116-L1127" target="_blank">source</a>]

Ideally, when running 30hz sim, and 60hz rendering, the draw frames should
have and offset of either 0.0 frames, or 0.5 frames.

When draw frames are not integer multiples of sim frames, some interpolation
happens, and this timeoffset shows how far along it is.








### Spring.GetLastUpdateSeconds
---
```lua
function Spring.GetLastUpdateSeconds() -> lastUpdateSeconds number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1146-L1151" target="_blank">source</a>]








### Spring.GetVideoCapturingMode
---
```lua
function Spring.GetVideoCapturingMode() -> allowRecord boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1159-L1164" target="_blank">source</a>]








### Spring.IsUnitAllied
---
```lua
function Spring.IsUnitAllied(unitID: integer) -> isAllied boolean?
```

@return `isAllied` - nil with unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1178-L1183" target="_blank">source</a>]








### Spring.IsUnitSelected
---
```lua
function Spring.IsUnitSelected(unitID: integer) -> isSelected boolean?
```

@return `isSelected` - nil when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1202-L1207" target="_blank">source</a>]








### Spring.GetUnitLuaDraw
---
```lua
function Spring.GetUnitLuaDraw(unitID: integer) -> draw boolean?
```

@return `draw` - nil when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1220-L1225" target="_blank">source</a>]








### Spring.GetUnitNoDraw
---
```lua
function Spring.GetUnitNoDraw(unitID: integer) -> nil boolean?
```

@return `nil` - when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1231-L1236" target="_blank">source</a>]








### Spring.GetUnitEngineDrawMask
---
```lua
function Spring.GetUnitEngineDrawMask(unitID: integer) -> nil boolean?
```

@return `nil` - when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1242-L1247" target="_blank">source</a>]








### Spring.GetUnitAlwaysUpdateMatrix
---
```lua
function Spring.GetUnitAlwaysUpdateMatrix(unitID: integer) -> nil boolean?
```

@return `nil` - when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1253-L1258" target="_blank">source</a>]








### Spring.GetUnitDrawFlag
---
```lua
function Spring.GetUnitDrawFlag(unitID: integer) -> nil number?
```

@return `nil` - when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1270-L1275" target="_blank">source</a>]








### Spring.GetUnitNoMinimap
---
```lua
function Spring.GetUnitNoMinimap(unitID: integer) -> nil boolean?
```

@return `nil` - when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1287-L1292" target="_blank">source</a>]








### Spring.GetUnitNoGroup
---
```lua
function Spring.GetUnitNoGroup(unitID: integer) -> noGroup boolean?
```

@return `noGroup` - `true` if the unit is not allowed to be added to a group, `false` if it is allowed to be added to a group, or `nil` when `unitID` is not valid.





Check if a unit is not allowed to be added to a group by a player.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1304-L1310" target="_blank">source</a>]








### Spring.GetUnitNoSelect
---
```lua
function Spring.GetUnitNoSelect(unitID: integer) -> noSelect boolean?
```

@return `noSelect` - `nil` when `unitID` cannot be parsed.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1322-L1327" target="_blank">source</a>]








### Spring.UnitIconGetDraw
---
```lua
function Spring.UnitIconGetDraw(unitID: integer) -> drawIcon boolean?
```

@return `drawIcon` - `true` if icon is being drawn, `nil` when unitID is invalid, otherwise `false`.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1340-L1346" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1358-L1372" target="_blank">source</a>]








### Spring.GetFeatureLuaDraw
---
```lua
function Spring.GetFeatureLuaDraw(featureID: integer) -> nil boolean?
```

@return `nil` - when featureID cannot be parsed





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1385-L1390" target="_blank">source</a>]








### Spring.GetFeatureNoDraw
---
```lua
function Spring.GetFeatureNoDraw(featureID: integer) -> nil boolean?
```

@return `nil` - when featureID cannot be parsed





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1396-L1401" target="_blank">source</a>]








### Spring.GetFeatureEngineDrawMask
---
```lua
function Spring.GetFeatureEngineDrawMask(featureID: integer) -> nil boolean?
```

@return `nil` - when featureID cannot be parsed





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1407-L1412" target="_blank">source</a>]








### Spring.GetFeatureAlwaysUpdateMatrix
---
```lua
function Spring.GetFeatureAlwaysUpdateMatrix(featureID: integer) -> nil boolean?
```

@return `nil` - when featureID cannot be parsed





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1418-L1423" target="_blank">source</a>]








### Spring.GetFeatureDrawFlag
---
```lua
function Spring.GetFeatureDrawFlag(featureID: integer) -> nil number?
```

@return `nil` - when featureID cannot be parsed





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1435-L1440" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1452-L1466" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1501-L1521" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1525-L1545" target="_blank">source</a>]








### Spring.IsUnitInView
---
```lua
function Spring.IsUnitInView(unitID: integer) -> inView boolean?
```

@return `inView` - nil when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1555-L1560" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1573-L1580" target="_blank">source</a>]








### Spring.IsUnitIcon
---
```lua
function Spring.IsUnitIcon(unitID: integer) -> isUnitIcon boolean?
```

@return `isUnitIcon` - nil when unitID cannot be parsed





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1615-L1620" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1633-L1643" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1662-L1670" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1683-L1691" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1778-L1785" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1876-L1884" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L1957-L1965" target="_blank">source</a>]








### Spring.GetRenderUnits
---
```lua
function Spring.GetRenderUnits(
  drawMask: DrawMask,
  sendMask: true
)
 -> featureIDs integer[]
 -> drawFlags DrawFlag[]

```
@param `drawMask` - (Default: `0`) Filter objects by their draw flags.

@param `sendMask` - Whether to send objects draw flags as second return






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2131-L2138" target="_blank">source</a>]








### Spring.GetRenderUnits
---
```lua
function Spring.GetRenderUnits(
  drawMask: DrawMask,
  sendMask: false?
) -> featureIDs integer[]
```
@param `drawMask` - (Default: `0`) Filter objects by their draw flags.

@param `sendMask` - Whether to send objects draw flags as second return






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2140-L2146" target="_blank">source</a>]








### Spring.GetRenderUnitsDrawFlagChanged
---
```lua
function Spring.GetRenderUnitsDrawFlagChanged(sendMask: true)
 -> ids integer[]
 -> unitDrawFlags DrawFlag[]

```
@param `sendMask` - Whether to send objects draw flags as second return.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2152-L2158" target="_blank">source</a>]

Gets a list of IDs of units that have had their draw flags changed, and the corresponding flags.








### Spring.GetRenderUnitsDrawFlagChanged
---
```lua
function Spring.GetRenderUnitsDrawFlagChanged(sendMask: false?) -> ids integer[]
```
@param `sendMask` - Whether to send objects draw flags as second return.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2160-L2165" target="_blank">source</a>]

Gets a list of IDs of units that have had their draw flags changed, and the corresponding flags.








### Spring.GetRenderFeatures
---
```lua
function Spring.GetRenderFeatures(
  drawMask: DrawMask,
  sendMask: true
)
 -> featureIDs integer[]
 -> drawFlags DrawFlag[]

```
@param `drawMask` - (Default: `0`) Filter objects by their draw flags.

@param `sendMask` - Whether to send objects draw flags as second return






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2171-L2178" target="_blank">source</a>]








### Spring.GetRenderFeatures
---
```lua
function Spring.GetRenderFeatures(
  drawMask: DrawMask,
  sendMask: false?
) -> featureIDs integer[]
```
@param `drawMask` - (Default: `0`) Filter objects by their draw flags.

@param `sendMask` - Whether to send objects draw flags as second return






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2180-L2186" target="_blank">source</a>]








### Spring.GetRenderFeaturesDrawFlagChanged
---
```lua
function Spring.GetRenderFeaturesDrawFlagChanged(sendMask: true)
 -> ids integer[]
 -> unitDrawFlags DrawFlag[]

```
@param `sendMask` - Whether to send objects draw flags as second return.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2192-L2198" target="_blank">source</a>]

Gets a list of IDs of features that have had their draw flags changed, and the corresponding flags.








### Spring.GetRenderFeaturesDrawFlagChanged
---
```lua
function Spring.GetRenderFeaturesDrawFlagChanged(sendMask: false?) -> ids integer[]
```
@param `sendMask` - Whether to send objects draw flags as second return.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2200-L2205" target="_blank">source</a>]

Gets a list of IDs of features that have had their draw flags changed, and the corresponding flags.








### Spring.ClearUnitsPreviousDrawFlag
---
```lua
function Spring.ClearUnitsPreviousDrawFlag()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2211-L2214" target="_blank">source</a>]








### Spring.ClearFeaturesPreviousDrawFlag
---
```lua
function Spring.ClearFeaturesPreviousDrawFlag()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2221-L2224" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2231-L2240" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2330-L2338" target="_blank">source</a>]








### Spring.GetLocalPlayerID
---
```lua
function Spring.GetLocalPlayerID() -> playerID integer
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2386-L2390" target="_blank">source</a>]








### Spring.GetLocalTeamID
---
```lua
function Spring.GetLocalTeamID() -> teamID integer
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2398-L2402" target="_blank">source</a>]








### Spring.GetLocalAllyTeamID
---
```lua
function Spring.GetLocalAllyTeamID() -> allyTeamID integer
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2410-L2414" target="_blank">source</a>]








### Spring.GetSpectatingState
---
```lua
function Spring.GetSpectatingState()
 -> spectating boolean
 -> spectatingFullView boolean
 -> spectatingFullSelect boolean

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2422-L2428" target="_blank">source</a>]








### Spring.GetSelectedUnits
---
```lua
function Spring.GetSelectedUnits() -> unitIDs number[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2441-L2445" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2452-L2457" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2467-L2473" target="_blank">source</a>]








### Spring.GetSelectedUnitsCount
---
```lua
function Spring.GetSelectedUnitsCount() -> selectedUnitsCount number
```





Returns the amount of selected units

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2483-L2487" target="_blank">source</a>]








### Spring.GetBoxSelectionByEngine
---
```lua
function Spring.GetBoxSelectionByEngine() -> isHandledByEngine boolean
```

@return `isHandledByEngine` - `true` if the engine will select units inside selection box on release, otherwise `false`.





Get if selection box is handled by engine.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2494-L2502" target="_blank">source</a>]





@see Spring.SetBoxSelectionByEngine




### Spring.IsGUIHidden
---
```lua
function Spring.IsGUIHidden() ->  boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2513-L2517" target="_blank">source</a>]








### Spring.HaveShadows
---
```lua
function Spring.HaveShadows() -> shadowsLoaded boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2525-L2529" target="_blank">source</a>]








### Spring.HaveAdvShading
---
```lua
function Spring.HaveAdvShading()
 -> useAdvShading boolean
 -> groundUseAdvShading boolean

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2537-L2542" target="_blank">source</a>]








### Spring.GetWaterMode
---
```lua
function Spring.GetWaterMode()
 -> waterRendererID integer
 -> waterRendererName string

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2551-L2557" target="_blank">source</a>]





@see rts/Rendering/Env/IWater.h




### Spring.GetMapDrawMode
---
```lua
function Spring.GetMapDrawMode() ->  ("normal"|"height"|"metal"|"pathTraversability"|"los")
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2567-L2571" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2596-L2605" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2653-L2660" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2681-L2690" target="_blank">source</a>]








### Spring.GetCameraNames
---
```lua
function Spring.GetCameraNames() -> indexByName table<string,integer>
```

@return `indexByName` - Table where where keys are names and values are indices.





Get available cameras.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2705-L2710" target="_blank">source</a>]








### Spring.GetCameraState
---
```lua
function Spring.GetCameraState(useReturns: false) -> cameraState CameraState
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2726-L2730" target="_blank">source</a>]








### Spring.GetCameraState
---
```lua
function Spring.GetCameraState(useReturns: true?)
 -> name CameraName
 -> Fields any

```
@param `useReturns` - (Default: `true`) Return multiple values instead of a table.


@return `Fields` - depending on current controller mode.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2731-L2736" target="_blank">source</a>]








### Spring.GetCameraPosition
---
```lua
function Spring.GetCameraPosition()
 -> posX number
 -> posY number
 -> posZ number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2802-L2807" target="_blank">source</a>]








### Spring.GetCameraDirection
---
```lua
function Spring.GetCameraDirection()
 -> dirX number
 -> dirY number
 -> dirZ number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2816-L2821" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2830-L2836" target="_blank">source</a>]








### Spring.GetCameraFOV
---
```lua
function Spring.GetCameraFOV()
 -> vFOV number
 -> hFOV number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2845-L2849" target="_blank">source</a>]








### Spring.GetCameraVectors
---
```lua
function Spring.GetCameraVectors() ->  CameraVectors
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2868-L2871" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2895-L2904" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L2918-L2941" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3044-L3052" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3099-L3107" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3126-L3134" target="_blank">source</a>]








### Spring.GetDrawSeconds
---
```lua
function Spring.GetDrawSeconds() -> time integer
```

@return `time` - Time in seconds.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3153-L3157" target="_blank">source</a>]








### Spring.GetSoundDevices
---
```lua
function Spring.GetSoundDevices() -> devices SoundDeviceSpec[]
```

@return `devices` - Sound devices.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3179-L3183" target="_blank">source</a>]








### Spring.GetSoundStreamTime
---
```lua
function Spring.GetSoundStreamTime()
 -> playTime number
 -> time number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3199-L3204" target="_blank">source</a>]








### Spring.GetSoundEffectParams
---
```lua
function Spring.GetSoundEffectParams()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3213-L3216" target="_blank">source</a>]








### Spring.GetFPS
---
```lua
function Spring.GetFPS() -> fps number
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3303-L3307" target="_blank">source</a>]








### Spring.GetGameSpeed
---
```lua
function Spring.GetGameSpeed()
 -> wantedSpeedFactor number
 -> speedFactor number
 -> paused boolean

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3317-L3323" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3332-L3340" target="_blank">source</a>]








### Spring.GetActiveCommand
---
```lua
function Spring.GetActiveCommand()
 -> cmdIndex number?
 -> cmdID integer?
 -> cmdType number?
 -> cmdName string?

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3359-L3366" target="_blank">source</a>]








### Spring.GetDefaultCommand
---
```lua
function Spring.GetDefaultCommand()
 -> cmdIndex integer?
 -> cmdID integer?
 -> cmdType integer?
 -> cmdName string?

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3388-L3395" target="_blank">source</a>]








### Spring.GetActiveCmdDescs
---
```lua
function Spring.GetActiveCmdDescs() -> cmdDescs CommandDescription[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3417-L3421" target="_blank">source</a>]








### Spring.GetActiveCmdDesc
---
```lua
function Spring.GetActiveCmdDesc(cmdIndex: integer) ->  CommandDescription?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3446-L3451" target="_blank">source</a>]








### Spring.GetCmdDescIndex
---
```lua
function Spring.GetCmdDescIndex(cmdID: integer) -> cmdDescIndex integer?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3470-L3475" target="_blank">source</a>]








### Spring.GetBuildFacing
---
```lua
function Spring.GetBuildFacing() -> buildFacing FacingInteger
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3509-L3513" target="_blank">source</a>]








### Spring.GetBuildSpacing
---
```lua
function Spring.GetBuildSpacing() -> buildSpacing number
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3524-L3528" target="_blank">source</a>]








### Spring.GetGatherMode
---
```lua
function Spring.GetGatherMode() -> gatherMode number
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3539-L3543" target="_blank">source</a>]








### Spring.GetActivePage
---
```lua
function Spring.GetActivePage()
 -> activePage number
 -> maxPage number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3556-L3561" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3579-L3589" target="_blank">source</a>]








### Spring.GetMouseCursor
---
```lua
function Spring.GetMouseCursor()
 -> cursorName string
 -> cursorScale number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3607-L3612" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3623-L3635" target="_blank">source</a>]








### Spring.GetMouseButtonsPressed
---
```lua
function Spring.GetMouseButtonsPressed(
  button1: integer,
  ...: integer
) -> Pressed boolean ...
```
@param `button1` - Index of the first button.

@param `...` - Indices for more buttons.


@return `Pressed` - status for the buttons.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3658-L3667" target="_blank">source</a>]

Get pressed status for specific buttons.








### Spring.GetClipboard
---
```lua
function Spring.GetClipboard() -> text string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3697-L3701" target="_blank">source</a>]








### Spring.IsUserWriting
---
```lua
function Spring.IsUserWriting() ->  boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3713-L3717" target="_blank">source</a>]








### Spring.GetLastMessagePositions
---
```lua
function Spring.GetLastMessagePositions() -> message xyz[]
```

@return `message` - positions





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3731-L3735" target="_blank">source</a>]








### Spring.GetConsoleBuffer
---
```lua
function Spring.GetConsoleBuffer(maxLines: number) -> buffer { priority: integer,text: string }[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3754-L3758" target="_blank">source</a>]








### Spring.GetCurrentTooltip
---
```lua
function Spring.GetCurrentTooltip() -> tooltip string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3790-L3793" target="_blank">source</a>]








### Spring.GetKeyFromScanSymbol
---
```lua
function Spring.GetKeyFromScanSymbol(scanSymbol: string) -> keyName string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3807-L3811" target="_blank">source</a>]








### Spring.GetKeyState
---
```lua
function Spring.GetKeyState(keyCode: number) -> pressed boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3842-L3847" target="_blank">source</a>]








### Spring.GetModKeyState
---
```lua
function Spring.GetModKeyState()
 -> alt boolean
 -> ctrl boolean
 -> meta boolean
 -> shift boolean

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3856-L3863" target="_blank">source</a>]








### Spring.GetPressedKeys
---
```lua
function Spring.GetPressedKeys() -> where table<(number|string),true>
```

@return `where` - keys are keyCodes or key names





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3874-L3878" target="_blank">source</a>]








### Spring.GetPressedScans
---
```lua
function Spring.GetPressedScans() -> where table<(number|string),true>
```

@return `where` - keys are scanCodes or scan names





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3905-L3909" target="_blank">source</a>]








### Spring.GetInvertQueueKey
---
```lua
function Spring.GetInvertQueueKey() -> queueKey number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3936-L3940" target="_blank">source</a>]








### Spring.GetKeyCode
---
```lua
function Spring.GetKeyCode(keySym: string) -> keyCode number
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3951-L3956" target="_blank">source</a>]








### Spring.GetKeySymbol
---
```lua
function Spring.GetKeySymbol(keyCode: number)
 -> keyCodeName string
 -> keyCodeDefaultName string

```

@return `keyCodeDefaultName` - name when there are not aliases





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3964-L3970" target="_blank">source</a>]








### Spring.GetScanSymbol
---
```lua
function Spring.GetScanSymbol(scanCode: number)
 -> scanCodeName string
 -> scanCodeDefaultName string

```

@return `scanCodeDefaultName` - name when there are not aliases





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L3980-L3986" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4008-L4013" target="_blank">source</a>]








### Spring.GetActionHotKeys
---
```lua
function Spring.GetActionHotKeys(actionName: string) -> hotkeys string[]?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4061-L4066" target="_blank">source</a>]








### Spring.GetGroupList
---
```lua
function Spring.GetGroupList() -> where table<number,number>?
```

@return `where` - keys are groupIDs and values are counts





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4087-L4091" target="_blank">source</a>]








### Spring.GetSelectedGroup
---
```lua
function Spring.GetSelectedGroup() -> groupID integer
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4115-L4119" target="_blank">source</a>]








### Spring.GetUnitGroup
---
```lua
function Spring.GetUnitGroup(unitID: integer) -> groupID integer?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4127-L4132" target="_blank">source</a>]








### Spring.GetGroupUnits
---
```lua
function Spring.GetGroupUnits(groupID: integer) -> unitIDs number[]?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4162-L4167" target="_blank">source</a>]








### Spring.GetGroupUnitsSorted
---
```lua
function Spring.GetGroupUnitsSorted(groupID: integer) -> where table<number,number[]>?
```

@return `where` - keys are unitDefIDs and values are unitIDs





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4179-L4184" target="_blank">source</a>]








### Spring.GetGroupUnitsCounts
---
```lua
function Spring.GetGroupUnitsCounts(groupID: integer) -> where table<number,number>?
```

@return `where` - keys are unitDefIDs and values are counts





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4196-L4201" target="_blank">source</a>]








### Spring.GetGroupUnitsCount
---
```lua
function Spring.GetGroupUnitsCount(groupID: integer) -> groupSize number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4213-L4218" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4251-L4257" target="_blank">source</a>]








### Spring.GetPlayerTraffic
---
```lua
function Spring.GetPlayerTraffic(
  playerID: integer,
  packetID: integer?
) -> traffic number
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4287-L4293" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4339-L4348" target="_blank">source</a>]








### Spring.GetConfigParams
---
```lua
function Spring.GetConfigParams() ->  Configuration[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4395-L4399" target="_blank">source</a>]








### Spring.GetConfigInt
---
```lua
function Spring.GetConfigInt(
  name: string,
  default: number?
) -> configInt number?
```
@param `default` - (Default: `0`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4469-L4475" target="_blank">source</a>]








### Spring.GetConfigFloat
---
```lua
function Spring.GetConfigFloat(
  name: string,
  default: number?
) -> configFloat number?
```
@param `default` - (Default: `0`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4490-L4496" target="_blank">source</a>]








### Spring.GetConfigString
---
```lua
function Spring.GetConfigString(
  name: string,
  default: string?
) -> configString number?
```
@param `default` - (Default: `""`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4511-L4517" target="_blank">source</a>]








### Spring.GetLogSections
---
```lua
function Spring.GetLogSections() -> sections table<string,number>
```

@return `sections` - where keys are names and loglevel are values. E.g. `{ "KeyBindings" = LOG.INFO, "Font" = LOG.INFO, "Sound" = LOG.WARNING, ... }`





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4532-L4536" target="_blank">source</a>]








### Spring.GetAllGroundDecals
---
```lua
function Spring.GetAllGroundDecals() -> decalIDs number[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4560-L4565" target="_blank">source</a>]








### Spring.GetGroundDecalMiddlePos
---
```lua
function Spring.GetGroundDecalMiddlePos(decalID: integer)
 -> posX number?
 -> posZ number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4594-L4600" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4615-L4627" target="_blank">source</a>]








### Spring.GetGroundDecalSizeAndHeight
---
```lua
function Spring.GetGroundDecalSizeAndHeight(decalID: integer)
 -> sizeX number?
 -> sizeY number
 -> projCubeHeight number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4648-L4655" target="_blank">source</a>]








### Spring.GetGroundDecalRotation
---
```lua
function Spring.GetGroundDecalRotation(decalID: integer) -> rotation number?
```

@return `rotation` - Rotation in radians.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4672-L4677" target="_blank">source</a>]








### Spring.GetGroundDecalTexture
---
```lua
function Spring.GetGroundDecalTexture(
  decalID: integer,
  isMainTex: boolean?
) -> texture string?
```
@param `isMainTex` - (Default: `true`) If `false`, return the normal/glow map.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4691-L4697" target="_blank">source</a>]








### Spring.GetDecalTextures
---
```lua
function Spring.GetDecalTextures(isMainTex: boolean?) -> textureNames string[]
```
@param `isMainTex` - (Default: `true`) If `false`, return the texture for normal/glow maps.


@return `textureNames` - All textures on the atlas and available for use in `SetGroundDecalTexture`.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4706-L4712" target="_blank">source</a>]





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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4722-L4728" target="_blank">source</a>]








### Spring.GetGroundDecalAlpha
---
```lua
function Spring.GetGroundDecalAlpha(decalID: integer)
 -> alpha number?
 -> alphaFalloff number

```

@return `alpha` - Between 0 and 1

@return `alphaFalloff` - Between 0 and 1, per second





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4743-L4749" target="_blank">source</a>]








### Spring.GetGroundDecalNormal
---
```lua
function Spring.GetGroundDecalNormal(decalID: integer)
 -> normal.x number?
 -> normal.y number
 -> normal.z number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4763-L4773" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4788-L4798" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4815-L4825" target="_blank">source</a>]

Returns less important parameters of a ground decal








### Spring.GetGroundDecalCreationFrame
---
```lua
function Spring.GetGroundDecalCreationFrame(decalID: integer)
 -> creationFrameMin number?
 -> creationFrameMax number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4841-L4850" target="_blank">source</a>]

Min can be not equal to max for "gradient" style decals, e.g. unit tracks








### Spring.GetGroundDecalOwner
---
```lua
function Spring.GetGroundDecalOwner(decalID: integer) -> value integer?
```

@return `value` - If owner is a unit, then this is `unitID`, if owner is
a feature it is `featureID + MAX_UNITS`. If there is no owner, then `nil`.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4865-L4870" target="_blank">source</a>]








### Spring.GetGroundDecalType
---
```lua
function Spring.GetGroundDecalType(decalID: integer) -> type ("explosion"|"plate"|"lua"|"track"|"unknown")?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4886-L4891" target="_blank">source</a>]








### Spring.GetSyncedGCInfo
---
```lua
function Spring.GetSyncedGCInfo(collectGC: boolean?) -> GC number?
```
@param `collectGC` - (Default: `false`) collect before returning metric


@return `GC` - values are expressed in Kbytes: #bytes/2^10





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4928-L4933" target="_blank">source</a>]








### Spring.SolveNURBSCurve
---
```lua
function Spring.SolveNURBSCurve(groupID: integer) -> unitIDs number[]?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedRead.cpp#L4955-L4960" target="_blank">source</a>]








### Spring.IsCheatingEnabled
---
```lua
function Spring.IsCheatingEnabled() -> enabled boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L755-L760" target="_blank">source</a>]








### Spring.IsGodModeEnabled
---
```lua
function Spring.IsGodModeEnabled() -> enabled boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L768-L773" target="_blank">source</a>]








### Spring.IsDevLuaEnabled
---
```lua
function Spring.IsDevLuaEnabled() -> enabled boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L783-L788" target="_blank">source</a>]








### Spring.IsEditDefsEnabled
---
```lua
function Spring.IsEditDefsEnabled() -> enabled boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L796-L801" target="_blank">source</a>]








### Spring.IsNoCostEnabled
---
```lua
function Spring.IsNoCostEnabled() -> enabled boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L809-L814" target="_blank">source</a>]








### Spring.GetGlobalLos
---
```lua
function Spring.GetGlobalLos(teamID: integer?) -> enabled boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L822-L829" target="_blank">source</a>]








### Spring.AreHelperAIsEnabled
---
```lua
function Spring.AreHelperAIsEnabled() -> enabled boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L841-L846" target="_blank">source</a>]








### Spring.FixedAllies
---
```lua
function Spring.FixedAllies() -> enabled boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L854-L859" target="_blank">source</a>]








### Spring.IsGameOver
---
```lua
function Spring.IsGameOver() -> isGameOver boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L867-L872" target="_blank">source</a>]








### Spring.GetGameFrame
---
```lua
function Spring.GetGameFrame()
 -> t1 number
 -> t2 number

```

@return `t1` - frameNum % dayFrames

@return `t2` - frameNum / dayFrames





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L889-L895" target="_blank">source</a>]








### Spring.GetGameSeconds
---
```lua
function Spring.GetGameSeconds() -> seconds number
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L907-L912" target="_blank">source</a>]








### Spring.GetTidal
---
```lua
function Spring.GetTidal() -> tidalStrength number
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L926-L931" target="_blank">source</a>]








### Spring.GetWind
---
```lua
function Spring.GetWind() -> windStrength number
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L939-L944" target="_blank">source</a>]








### Spring.GetGameRulesParams
---
```lua
function Spring.GetGameRulesParams() -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L971-L976" target="_blank">source</a>]








### Spring.GetTeamRulesParams
---
```lua
function Spring.GetTeamRulesParams(teamID: integer) -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L984-L991" target="_blank">source</a>]








### Spring.GetPlayerRulesParams
---
```lua
function Spring.GetPlayerRulesParams(playerID: integer) -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1010-L1017" target="_blank">source</a>]








### Spring.GetUnitRulesParams
---
```lua
function Spring.GetUnitRulesParams(unitID: integer) -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1079-L1086" target="_blank">source</a>]








### Spring.GetFeatureRulesParams
---
```lua
function Spring.GetFeatureRulesParams(featureID: integer) -> rulesParams RulesParams
```

@return `rulesParams` - map with rules names as key and values as values





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1097-L1104" target="_blank">source</a>]








### Spring.GetGameRulesParam
---
```lua
function Spring.GetGameRulesParam(ruleRef: (number|string)) ->  number?
```
@param `ruleRef` - the rule index or name


@return  - |string value





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1133-L1140" target="_blank">source</a>]








### Spring.GetTeamRulesParam
---
```lua
function Spring.GetTeamRulesParam(
  teamID: integer,
  ruleRef: (number|string)
) -> value (number|string)?
```
@param `ruleRef` - the rule index or name






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1148-L1156" target="_blank">source</a>]








### Spring.GetPlayerRulesParam
---
```lua
function Spring.GetPlayerRulesParam(
  playerID: integer,
  ruleRef: (number|string)
) -> value (number|string)?
```
@param `ruleRef` - the rule index or name






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1176-L1184" target="_blank">source</a>]








### Spring.GetUnitRulesParam
---
```lua
function Spring.GetUnitRulesParam(
  unitID: integer,
  ruleRef: (number|string)
) -> value (number|string)?
```
@param `ruleRef` - the rule index or name






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1207-L1215" target="_blank">source</a>]








### Spring.GetFeatureRulesParam
---
```lua
function Spring.GetFeatureRulesParam(
  featureID: integer,
  ruleRef: (number|string)
) -> value (number|string)?
```
@param `ruleRef` - the rule index or name






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1226-L1234" target="_blank">source</a>]








### Spring.GetMapOption
---
```lua
function Spring.GetMapOption(mapOption: string) -> value string
```

@return `value` - Value of `modOption`.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1302-L1309" target="_blank">source</a>]








### Spring.GetMapOptions
---
```lua
function Spring.GetMapOptions() -> mapOptions table<string,string>
```

@return `mapOptions` - Table with options names as keys and values as values.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1314-L1319" target="_blank">source</a>]








### Spring.GetModOption
---
```lua
function Spring.GetModOption(modOption: string) -> value string
```

@return `value` - Value of `modOption`.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1326-L1333" target="_blank">source</a>]








### Spring.GetModOptions
---
```lua
function Spring.GetModOptions() -> modOptions table<string,string>
```

@return `modOptions` - Table with options names as keys and values as values.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1340-L1345" target="_blank">source</a>]








### Spring.GetHeadingFromVector
---
```lua
function Spring.GetHeadingFromVector(
  x: number,
  z: number
) -> heading number
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1359-L1367" target="_blank">source</a>]








### Spring.GetVectorFromHeading
---
```lua
function Spring.GetVectorFromHeading(heading: number)
 -> x number
 -> z number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1378-L1386" target="_blank">source</a>]








### Spring.GetFacingFromHeading
---
```lua
function Spring.GetFacingFromHeading(heading: number) -> facing FacingInteger
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1396-L1400" target="_blank">source</a>]








### Spring.GetHeadingFromFacing
---
```lua
function Spring.GetHeadingFromFacing(facing: FacingInteger) -> heading number
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1407-L1411" target="_blank">source</a>]








### Spring.GetSideData
---
```lua
function Spring.GetSideData(sideName: string)
 -> startUnit string?
 -> caseName string?

```
@param `sideName` - Case-insensitive side name.


@return `caseName` - Original case side name.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1439-L1445" target="_blank">source</a>]








### Spring.GetSideData
---
```lua
function Spring.GetSideData(sideID: integer)
 -> sideName string?
 -> startUnit string?
 -> caseName string?

```

@return `sideName` - Lowercase side name.

@return `caseName` - Original case side name.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1447-L1453" target="_blank">source</a>]








### Spring.GetSideData
---
```lua
function Spring.GetSideData() -> sideArray SideSpec[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1455-L1458" target="_blank">source</a>]








### Spring.GetGaiaTeamID
---
```lua
function Spring.GetGaiaTeamID() -> teamID integer
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1505-L1510" target="_blank">source</a>]








### Spring.GetAllyTeamStartBox
---
```lua
function Spring.GetAllyTeamStartBox(allyID: integer)
 -> xMin number?
 -> zMin number?
 -> xMax number?
 -> zMax number?

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1521-L1531" target="_blank">source</a>]








### Spring.GetTeamStartPosition
---
```lua
function Spring.GetTeamStartPosition(teamID: integer)
 -> x number?
 -> y number?
 -> x number?

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1553-L1562" target="_blank">source</a>]








### Spring.GetMapStartPositions
---
```lua
function Spring.GetMapStartPositions() -> array float3[]
```

@return `array` - of positions indexed by teamID





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1581-L1585" target="_blank">source</a>]








### Spring.GetAllyTeamList
---
```lua
function Spring.GetAllyTeamList() -> allyTeamIDs integer[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1607-L1611" target="_blank">source</a>]








### Spring.GetTeamList
---
```lua
function Spring.GetTeamList(allyTeamID: unknown) -> teamIDs number[]
```
@param `allyTeamID` - (Default: `-1`)


@return `teamIDs` - List of team IDs.





Get all team IDs.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1627-L1633" target="_blank">source</a>]








### Spring.GetTeamList
---
```lua
function Spring.GetTeamList(allyTeamID: integer) -> teamIDs number[]?
```
@param `allyTeamID` - The ally team ID to filter teams by. A value less than 0 will return all teams.


@return `teamIDs` - List of team IDs or `nil` if `allyTeamID` is invalid.





Get team IDs in a specific ally team.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1635-L1641" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1677-L1683" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1731-L1744" target="_blank">source</a>]








### Spring.GetTeamAllyTeamID
---
```lua
function Spring.GetTeamAllyTeamID(teamID: integer) -> allyTeamID integer?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1782-L1787" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1803-L1817" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1862-L1872" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1897-L1907" target="_blank">source</a>]








### Spring.GetTeamDamageStats
---
```lua
function Spring.GetTeamDamageStats(teamID: integer)
 -> damageDealt number
 -> damageReceived number

```





Gets team damage dealt/received totals

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L1946-L1956" target="_blank">source</a>]

Returns a team's damage stats. Note that all damage is counted,
including self-inflicted and unconfirmed out-of-sight.








### Spring.GetTeamStatsHistory
---
```lua
function Spring.GetTeamStatsHistory(teamID: integer) -> historyCount integer?
```

@return `historyCount` - The number of history entries, or `nil` if unable to resolve team.





Get the number of history entries.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2001-L2006" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2007-L2014" target="_blank">source</a>]








### Spring.GetTeamLuaAI
---
```lua
function Spring.GetTeamLuaAI(teamID: integer) ->  string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2100-L2105" target="_blank">source</a>]








### Spring.GetTeamMaxUnits
---
```lua
function Spring.GetTeamMaxUnits(teamID: integer)
 -> maxUnits number
 -> currentUnits number?

```





Returns a team's unit cap.

Also returns the current unit count for readable teams as the 2nd value.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2133-L2141" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2158-L2175" target="_blank">source</a>]








### Spring.GetPlayerControlledUnit
---
```lua
function Spring.GetPlayerControlledUnit(playerID: integer) ->  number?
```





Returns unit controlled by player on FPS mode

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2223-L2228" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2259-L2269" target="_blank">source</a>]








### Spring.GetAllyTeamInfo
---
```lua
function Spring.GetAllyTeamInfo(allyTeamID: integer) ->  table<string,string>?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2318-L2323" target="_blank">source</a>]








### Spring.AreTeamsAllied
---
```lua
function Spring.AreTeamsAllied(
  teamID1: number,
  teamID2: number
) ->  boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2344-L2350" target="_blank">source</a>]








### Spring.ArePlayersAllied
---
```lua
function Spring.ArePlayersAllied(
  playerID1: number,
  playerID2: number
) ->  boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2364-L2370" target="_blank">source</a>]








### Spring.GetAllUnits
---
```lua
function Spring.GetAllUnits() -> unitIDs number[]
```





Get a list of all unitIDs

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2400-L2413" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2438-L2443" target="_blank">source</a>]








### Spring.GetTeamUnitsSorted
---
```lua
function Spring.GetTeamUnitsSorted(teamID: integer) -> unitsByDef table<integer,integer>
```

@return `unitsByDef` - A table where keys are unitDefIDs and values are unitIDs





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2538-L2543" target="_blank">source</a>]








### Spring.GetTeamUnitsCounts
---
```lua
function Spring.GetTeamUnitsCounts(teamID: integer) -> countByUnit table<number,number>?
```

@return `countByUnit` - A table where keys are unitDefIDs and values are counts.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2634-L2639" target="_blank">source</a>]








### Spring.GetTeamUnitsByDefs
---
```lua
function Spring.GetTeamUnitsByDefs(
  teamID: integer,
  unitDefIDs: (number|number[])
) -> unitIDs number[]?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2717-L2723" target="_blank">source</a>]








### Spring.GetTeamUnitDefCount
---
```lua
function Spring.GetTeamUnitDefCount(
  teamID: integer,
  unitDefID: integer
) -> count number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2793-L2799" target="_blank">source</a>]








### Spring.GetTeamUnitCount
---
```lua
function Spring.GetTeamUnitCount(teamID: integer) -> count number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2854-L2859" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L2967-L2976" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3033-L3044" target="_blank">source</a>]








### Spring.GetUnitsInCylinder
---
```lua
function Spring.GetUnitsInCylinder(
  x: number,
  z: number,
  radius: number
) -> unitIDs number[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3103-L3110" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3165-L3173" target="_blank">source</a>]








### Spring.GetUnitsInPlanes
---
```lua
function Spring.GetUnitsInPlanes(
  planes: Plane[],
  allegiance: integer?
) -> unitIDs integer[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3256-L3271" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3390-L3399" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3405-L3414" target="_blank">source</a>]








### Spring.GetUnitNearestAlly
---
```lua
function Spring.GetUnitNearestAlly(
  unitID: integer,
  range: number?
) -> unitID integer?
```
@param `range` - (Default: `1.0e9`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3421-L3427" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3446-L3453" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3517-L3525" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3543-L3551" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3568-L3576" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3636-L3646" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3666-L3676" target="_blank">source</a>]








### Spring.ValidUnitID
---
```lua
function Spring.ValidUnitID(unitID: integer) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3698-L3703" target="_blank">source</a>]








### Spring.GetUnitStates
---
```lua
function Spring.GetUnitStates(unitID: integer) ->  UnitState
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3725-L3730" target="_blank">source</a>]








### Spring.GetUnitArmored
---
```lua
function Spring.GetUnitArmored(unitID: integer)
 -> armored boolean?
 -> armorMultiple number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3818-L3824" target="_blank">source</a>]








### Spring.GetUnitIsActive
---
```lua
function Spring.GetUnitIsActive(unitID: integer) -> isActive boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3837-L3842" target="_blank">source</a>]








### Spring.GetUnitIsCloaked
---
```lua
function Spring.GetUnitIsCloaked(unitID: integer) -> isCloaked boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3854-L3859" target="_blank">source</a>]








### Spring.GetUnitSeismicSignature
---
```lua
function Spring.GetUnitSeismicSignature(unitID: integer) -> seismicSignature number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3871-L3876" target="_blank">source</a>]








### Spring.GetUnitSelfDTime
---
```lua
function Spring.GetUnitSelfDTime(unitID: integer) -> selfDTime integer?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3887-L3892" target="_blank">source</a>]








### Spring.GetUnitStockpile
---
```lua
function Spring.GetUnitStockpile(unitID: integer)
 -> numStockpiled integer?
 -> numStockpileQued integer?
 -> buildPercent number?

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3904-L3911" target="_blank">source</a>]








### Spring.GetUnitSensorRadius
---
```lua
function Spring.GetUnitSensorRadius(
  unitID: integer,
  type: string
) -> radius number?
```
@param `type` - one of los, airLos, radar, sonar, seismic, radarJammer, sonarJammer






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3928-L3934" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L3971-L3984" target="_blank">source</a>]








### Spring.GetUnitTooltip
---
```lua
function Spring.GetUnitTooltip(unitID: integer) ->  string?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4008-L4013" target="_blank">source</a>]








### Spring.GetUnitDefID
---
```lua
function Spring.GetUnitDefID(unitID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4048-L4053" target="_blank">source</a>]








### Spring.GetUnitTeam
---
```lua
function Spring.GetUnitTeam(unitID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4073-L4078" target="_blank">source</a>]








### Spring.GetUnitAllyTeam
---
```lua
function Spring.GetUnitAllyTeam(unitID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4090-L4095" target="_blank">source</a>]








### Spring.GetUnitNeutral
---
```lua
function Spring.GetUnitNeutral(unitID: integer) ->  boolean?
```





Checks if a unit is neutral (NOT Gaia!)

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4107-L4116" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4128-L4137" target="_blank">source</a>]








### Spring.GetUnitIsDead
---
```lua
function Spring.GetUnitIsDead(unitID: integer) ->  boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4167-L4172" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4184-L4197" target="_blank">source</a>]








### Spring.GetUnitIsBeingBuilt
---
```lua
function Spring.GetUnitIsBeingBuilt(unitID: integer)
 -> beingBuilt boolean
 -> buildProgress number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4211-L4217" target="_blank">source</a>]








### Spring.GetUnitResources
---
```lua
function Spring.GetUnitResources(unitID: integer)
 -> metalMake number?
 -> metalUse number
 -> energyMake number
 -> energyUse number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4229-L4237" target="_blank">source</a>]








### Spring.GetUnitStorage
---
```lua
function Spring.GetUnitStorage(unitID: integer)
 -> Unit number
 -> Unit number

```

@return `Unit` - 's metal storage

@return `Unit` - 's energy storage





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4251-L4256" target="_blank">source</a>]








### Spring.GetUnitCosts
---
```lua
function Spring.GetUnitCosts(unitID: integer)
 -> buildTime number?
 -> metalCost number
 -> energyCost number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4269-L4275" target="_blank">source</a>]








### Spring.GetUnitCostTable
---
```lua
function Spring.GetUnitCostTable(unitID: integer)
 -> cost ResourceCost?
 -> buildTime number?

```

@return `cost` - The cost of the unit, or `nil` if invalid.

@return `buildTime` - The build time the unit, or `nil` if invalid.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4294-L4299" target="_blank">source</a>]








### Spring.GetUnitMetalExtraction
---
```lua
function Spring.GetUnitMetalExtraction(unitID: integer) -> metalExtraction number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4317-L4322" target="_blank">source</a>]








### Spring.GetUnitExperience
---
```lua
function Spring.GetUnitExperience(unitID: integer)
 -> xp number
 -> limXp number

```

@return `xp` - [0.0; +∞)

@return `limXp` - [0.0; 1.0) as experience approaches infinity





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4337-L4343" target="_blank">source</a>]








### Spring.GetUnitHeight
---
```lua
function Spring.GetUnitHeight(unitID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4356-L4361" target="_blank">source</a>]








### Spring.GetUnitRadius
---
```lua
function Spring.GetUnitRadius(unitID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4373-L4378" target="_blank">source</a>]








### Spring.GetUnitBuildeeRadius
---
```lua
function Spring.GetUnitBuildeeRadius(unitID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4389-L4395" target="_blank">source</a>]

Gets the unit's radius for when targeted by build, repair, reclaim-type commands.








### Spring.GetUnitMass
---
```lua
function Spring.GetUnitMass(unitID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4406-L4411" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4417-L4432" target="_blank">source</a>]








### Spring.GetUnitBasePosition
---
```lua
function Spring.GetUnitBasePosition(unitID: integer)
 -> posX number?
 -> posY number?
 -> posZ number?

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4438-L4447" target="_blank">source</a>]

The same as `Spring.GetUnitPosition`, but without the optional midpoint calculations.





@see Spring.GetUnitPosition




### Spring.GetUnitVectors
---
```lua
function Spring.GetUnitVectors(unitID: integer)
 -> front float3?
 -> up float3
 -> right float3

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4454-L4461" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4482-L4490" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4497-L4510" target="_blank">source</a>]








### Spring.GetUnitHeading
---
```lua
function Spring.GetUnitHeading(
  unitID: integer,
  convertToRadians: boolean?
) -> heading number
```
@param `convertToRadians` - (Default: `false`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4534-L4540" target="_blank">source</a>]








### Spring.GetUnitVelocity
---
```lua
function Spring.GetUnitVelocity(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4557-L4561" target="_blank">source</a>]








### Spring.GetUnitBuildFacing
---
```lua
function Spring.GetUnitBuildFacing(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4568-L4572" target="_blank">source</a>]








### Spring.GetUnitIsBuilding
---
```lua
function Spring.GetUnitIsBuilding(unitID: integer) -> buildeeUnitID integer
```

@return `buildeeUnitID` - or nil





Checks whether a unit is currently building another (NOT for checking if it's a structure)

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4584-L4592" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4672-L4690" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4706-L4713" target="_blank">source</a>]

Useful for setting move goals manually.








### Spring.GetUnitCurrentBuildPower
---
```lua
function Spring.GetUnitCurrentBuildPower(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4762-L4766" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4795-L4806" target="_blank">source</a>]

Checks resources being carried internally by the unit.








### Spring.GetUnitBuildParams
---
```lua
function Spring.GetUnitBuildParams(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4820-L4824" target="_blank">source</a>]








### Spring.GetUnitInBuildStance
---
```lua
function Spring.GetUnitInBuildStance(unitID: integer) -> inBuildStance boolean
```





Is builder in build stance

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4853-L4862" target="_blank">source</a>]

Checks if a builder is in build stance, i.e. can create nanoframes.
Returns nil for non-builders.








### Spring.GetUnitNanoPieces
---
```lua
function Spring.GetUnitNanoPieces(unitID: integer) -> pieceArray integer[]
```





Get construction FX attachment points

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4879-L4892" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4934-L4943" target="_blank">source</a>]

Returns the unit ID of the transport, if any.
Returns nil if the unit is not being transported.








### Spring.GetUnitIsTransporting
---
```lua
function Spring.GetUnitIsTransporting(unitID: integer) -> transporteeArray integer[]?
```

@return `transporteeArray` - An array of unitIDs being transported by this unit, or `nil` if not a transport.





Get units being transported

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4958-L4965" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L4987-L4994" target="_blank">source</a>]








### Spring.GetUnitFlanking
---
```lua
function Spring.GetUnitFlanking(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5019-L5023" target="_blank">source</a>]








### Spring.GetUnitMaxRange
---
```lua
function Spring.GetUnitMaxRange(unitID: integer) -> maxRange number
```





Get a unit's engagement range

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5078-L5088" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5107-L5138" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5317-L5321" target="_blank">source</a>]








### Spring.GetUnitWeaponVectors
---
```lua
function Spring.GetUnitWeaponVectors(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5356-L5360" target="_blank">source</a>]








### Spring.GetUnitWeaponTryTarget
---
```lua
function Spring.GetUnitWeaponTryTarget(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5396-L5400" target="_blank">source</a>]








### Spring.GetUnitWeaponTestTarget
---
```lua
function Spring.GetUnitWeaponTestTarget(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5441-L5445" target="_blank">source</a>]








### Spring.GetUnitWeaponTestRange
---
```lua
function Spring.GetUnitWeaponTestRange(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5479-L5483" target="_blank">source</a>]








### Spring.GetUnitWeaponHaveFreeLineOfFire
---
```lua
function Spring.GetUnitWeaponHaveFreeLineOfFire(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5517-L5521" target="_blank">source</a>]








### Spring.GetUnitWeaponCanFire
---
```lua
function Spring.GetUnitWeaponCanFire(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5583-L5587" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5616-L5627" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5628-L5640" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5641-L5653" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5654-L5666" target="_blank">source</a>]








### Spring.GetUnitEstimatedPath
---
```lua
function Spring.GetUnitEstimatedPath(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5723-L5727" target="_blank">source</a>]








### Spring.GetUnitLastAttacker
---
```lua
function Spring.GetUnitLastAttacker(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5743-L5747" target="_blank">source</a>]








### Spring.GetUnitLastAttackedPiece
---
```lua
function Spring.GetUnitLastAttackedPiece(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5763-L5767" target="_blank">source</a>]








### Spring.GetUnitCollisionVolumeData
---
```lua
function Spring.GetUnitCollisionVolumeData(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5773-L5777" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5794-L5802" target="_blank">source</a>]








### Spring.GetUnitFeatureSeparation
---
```lua
function Spring.GetUnitFeatureSeparation(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5836-L5840" target="_blank">source</a>]








### Spring.GetUnitDefDimensions
---
```lua
function Spring.GetUnitDefDimensions(unitDefID: integer) -> dimensions UnitDefDimensions?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5885-L5890" target="_blank">source</a>]








### Spring.GetCEGID
---
```lua
function Spring.GetCEGID()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5920-L5923" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5931-L5942" target="_blank">source</a>]








### Spring.GetUnitMoveTypeData
---
```lua
function Spring.GetUnitMoveTypeData(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L5949-L5953" target="_blank">source</a>]








### Spring.GetUnitCurrentCommand
---
```lua
function Spring.GetUnitCurrentCommand(
  unitID: integer,
  cmdIndex: integer?
)
 -> cmdID CMD?
 -> options (integer|CommandOptionBit)?
 -> tag integer?
 -> Command number? ...

```
@param `unitID` - unitID when invalid this function returns nil.

@param `cmdIndex` - (Default: `0`) Command index to get. If negative will count from the end of the queue, e.g. -1 will be the last command.


@return `Command` - parameters.





Get a command from a units command queue.

For factories, this function uses the command queue automatically assigned to new units.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6181-L6196" target="_blank">source</a>]





@see Spring.GetFactoryCommands for getting factory build queue commands




### Spring.GetUnitCommands
---
```lua
function Spring.GetUnitCommands(
  unitID: integer,
  count: integer
) -> commands Command[]
```
@param `count` - Maximum amount of commands to return, `-1` returns all commands.






Get the commands for a unit.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6233-L6243" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6244-L6253" target="_blank">source</a>]



@deprecated This overload is deprecated, use `Spring.GetUnitCommandCount(unitId)` instead.






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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6281-L6288" target="_blank">source</a>]








### Spring.GetUnitCommandCount
---
```lua
function Spring.GetUnitCommandCount(unitID: integer) -> The integer
```

@return `The` - number of commands in the unit's queue.





Get the number of commands in a unit's queue.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6317-L6322" target="_blank">source</a>]








### Spring.GetFactoryBuggerOff
---
```lua
function Spring.GetFactoryBuggerOff(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6340-L6344" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6420-L6428" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6456-L6467" target="_blank">source</a>]

Same as `Spring.GetUnitCommands`



@deprecated Use Spring.GetUnitCommands instead.






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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6468-L6480" target="_blank">source</a>]

Same as `Spring.GetUnitCommands`



@deprecated Use Spring.GetUnitCommandCount instead.






### Spring.GetFullBuildQueue
---
```lua
function Spring.GetFullBuildQueue(unitID: integer) -> buildqueue table<number,number>?
```

@return `buildqueue` - indexed by unitDefID with count values





Returns the build queue

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6561-L6566" target="_blank">source</a>]








### Spring.GetRealBuildQueue
---
```lua
function Spring.GetRealBuildQueue(unitID: integer) -> buildqueue table<number,number>?
```

@return `buildqueue` - indexed by unitDefID with count values





Returns the build queue cleaned of things the unit can't build itself

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6573-L6578" target="_blank">source</a>]








### Spring.GetUnitCmdDescs
---
```lua
function Spring.GetUnitCmdDescs(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6588-L6592" target="_blank">source</a>]








### Spring.FindUnitCmdDesc
---
```lua
function Spring.FindUnitCmdDesc(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6627-L6631" target="_blank">source</a>]








### Spring.ValidFeatureID
---
```lua
function Spring.ValidFeatureID(featureID: integer) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6654-L6659" target="_blank">source</a>]








### Spring.GetAllFeatures
---
```lua
function Spring.GetAllFeatures() -> featureIDs integer[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6667-L6670" target="_blank">source</a>]








### Spring.GetFeatureDefID
---
```lua
function Spring.GetFeatureDefID(featureID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6695-L6700" target="_blank">source</a>]








### Spring.GetFeatureTeam
---
```lua
function Spring.GetFeatureTeam(featureID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6712-L6717" target="_blank">source</a>]








### Spring.GetFeatureAllyTeam
---
```lua
function Spring.GetFeatureAllyTeam(featureID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6733-L6738" target="_blank">source</a>]








### Spring.GetFeatureHealth
---
```lua
function Spring.GetFeatureHealth(featureID: integer)
 -> health number?
 -> defHealth number
 -> resurrectProgress number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6750-L6757" target="_blank">source</a>]








### Spring.GetFeatureHeight
---
```lua
function Spring.GetFeatureHeight(featureID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6771-L6776" target="_blank">source</a>]








### Spring.GetFeatureRadius
---
```lua
function Spring.GetFeatureRadius(featureID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6788-L6793" target="_blank">source</a>]








### Spring.GetFeatureMass
---
```lua
function Spring.GetFeatureMass(featureID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6804-L6809" target="_blank">source</a>]








### Spring.GetFeaturePosition
---
```lua
function Spring.GetFeaturePosition(featureID: integer)
 -> x number?
 -> y number?
 -> z number?

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6815-L6822" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6829-L6836" target="_blank">source</a>]








### Spring.GetFeatureRotation
---
```lua
function Spring.GetFeatureRotation(featureID: integer)
 -> pitch number?
 -> yaw number?
 -> roll number?

```

@return `pitch` - Rotation in X axis

@return `yaw` - Rotation in Y axis

@return `roll` - Rotation in Z axis





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6861-L6869" target="_blank">source</a>]

Note: PYR order








### Spring.GetFeatureDirection
---
```lua
function Spring.GetFeatureDirection(featureID: integer)
 -> frontDirX number?
 -> frontDirY number?
 -> frontDirZ number?
 -> rightDirX number?
 -> rightDirY number?
 -> rightDirZ number?
 -> upDirX number?
 -> upDirY number?
 -> upDirZ number?

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6879-L6892" target="_blank">source</a>]








### Spring.GetFeatureVelocity
---
```lua
function Spring.GetFeatureVelocity(featureID: integer)
 -> x number?
 -> y number?
 -> z number?
 -> w number?

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6920-L6929" target="_blank">source</a>]

Returns nil if no feature found with ID.








### Spring.GetFeatureHeading
---
```lua
function Spring.GetFeatureHeading(featureID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6936-L6940" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6952-L6962" target="_blank">source</a>]








### Spring.GetFeatureBlocking
---
```lua
function Spring.GetFeatureBlocking(featureID: integer)
 -> isBlocking boolean?
 -> isSolidObjectCollidable boolean?
 -> isProjectileCollidable boolean?
 -> isRaySegmentCollidable boolean?
 -> crushable boolean?
 -> blockEnemyPushing boolean?
 -> blockHeightChanges boolean?

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6979-L6990" target="_blank">source</a>]








### Spring.GetFeatureNoSelect
---
```lua
function Spring.GetFeatureNoSelect(featureID: integer) ->  boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L6997-L7002" target="_blank">source</a>]








### Spring.GetFeatureResurrect
---
```lua
function Spring.GetFeatureResurrect(featureID: integer)
 -> featureDefName (string|"")?
 -> buildFacing FacingInteger

```

@return `buildFacing` - facing of footprint, 0 - 3





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7015-L7022" target="_blank">source</a>]

Returns nil if no feature found with ID.








### Spring.GetFeatureLastAttackedPiece
---
```lua
function Spring.GetFeatureLastAttackedPiece(featureID: integer)
 -> Last (string|"")?
 -> frame integer

```

@return `Last` - hit piece name

@return `frame` - it was last hit on





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7041-L7048" target="_blank">source</a>]

Returns nil if no feature found with ID.








### Spring.GetFeatureCollisionVolumeData
---
```lua
function Spring.GetFeatureCollisionVolumeData(featureID: integer) ->  CollisionVolumeData?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7070-L7075" target="_blank">source</a>]








### Spring.GetFeaturePieceCollisionVolumeData
---
```lua
function Spring.GetFeaturePieceCollisionVolumeData(featureID: integer) ->  CollisionVolumeData?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7086-L7091" target="_blank">source</a>]








### Spring.GetProjectilePosition
---
```lua
function Spring.GetProjectilePosition(projectileID: integer)
 -> posX number?
 -> posY number?
 -> posZ number?

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7105-L7112" target="_blank">source</a>]








### Spring.GetProjectileDirection
---
```lua
function Spring.GetProjectileDirection(projectileID: integer)
 -> dirX number?
 -> dirY number?
 -> dirZ number?

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7126-L7133" target="_blank">source</a>]








### Spring.GetProjectileVelocity
---
```lua
function Spring.GetProjectileVelocity(projectileID: integer)
 -> velX number?
 -> velY number?
 -> velZ number?
 -> velW number?

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7147-L7155" target="_blank">source</a>]








### Spring.GetProjectileGravity
---
```lua
function Spring.GetProjectileGravity(projectileID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7162-L7167" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7181-L7191" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7211-L7221" target="_blank">source</a>]








### Spring.GetProjectileIsIntercepted
---
```lua
function Spring.GetProjectileIsIntercepted(projectileID: integer) ->  boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7263-L7268" target="_blank">source</a>]








### Spring.GetProjectileTimeToLive
---
```lua
function Spring.GetProjectileTimeToLive(projectileID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7283-L7288" target="_blank">source</a>]








### Spring.GetProjectileOwnerID
---
```lua
function Spring.GetProjectileOwnerID(projectileID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7303-L7308" target="_blank">source</a>]








### Spring.GetProjectileTeamID
---
```lua
function Spring.GetProjectileTeamID(projectileID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7325-L7330" target="_blank">source</a>]








### Spring.GetProjectileAllyTeamID
---
```lua
function Spring.GetProjectileAllyTeamID(projectileID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7346-L7351" target="_blank">source</a>]








### Spring.GetProjectileType
---
```lua
function Spring.GetProjectileType(projectileID: integer)
 -> weapon boolean?
 -> piece boolean

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7367-L7373" target="_blank">source</a>]








### Spring.GetProjectileDefID
---
```lua
function Spring.GetProjectileDefID(projectileID: integer) ->  number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7387-L7395" target="_blank">source</a>]

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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7416-L7437" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7464-L7471" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7505-L7513" target="_blank">source</a>]








### Spring.GetWaterPlaneLevel
---
```lua
function Spring.GetWaterPlaneLevel() -> waterPlaneLevel number
```





Get water plane height

Water may at some point become shaped (rivers etc) but for now it is always a flat plane.
Use this function instead of GetWaterLevel to denote you are relying on that assumption.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7522-L7530" target="_blank">source</a>]





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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7537-L7546" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7556-L7564" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7574-L7584" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7604-L7620" target="_blank">source</a>]








### Spring.GetGroundBlocked
---
```lua
function Spring.GetGroundBlocked()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7675-L7678" target="_blank">source</a>]








### Spring.GetGroundExtremes
---
```lua
function Spring.GetGroundExtremes()
 -> initMinHeight number
 -> initMaxHeight number
 -> currMinHeight number
 -> currMaxHeight number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7720-L7727" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7738-L7750" target="_blank">source</a>]








### Spring.GetGrass
---
```lua
function Spring.GetGrass(
  x: number,
  z: number
) ->  number
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7762-L7768" target="_blank">source</a>]








### Spring.GetSmoothMeshHeight
---
```lua
function Spring.GetSmoothMeshHeight(
  x: number,
  z: number
) -> height number
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7778-L7784" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7802-L7816" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7867-L7876" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7917-L7928" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L7946-L7960" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8014-L8025" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8054-L8062" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8080-L8088" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8106-L8114" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8132-L8145" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8146-L8153" target="_blank">source</a>]








### Spring.IsUnitInLos
---
```lua
function Spring.IsUnitInLos(
  unitID: integer,
  allyTeamID: integer
) -> inLos boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8196-L8202" target="_blank">source</a>]








### Spring.IsUnitInAirLos
---
```lua
function Spring.IsUnitInAirLos(
  unitID: integer,
  allyTeamID: integer
) -> inAirLos boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8220-L8226" target="_blank">source</a>]








### Spring.IsUnitInRadar
---
```lua
function Spring.IsUnitInRadar(
  unitID: integer,
  allyTeamID: integer
) -> inRadar boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8244-L8250" target="_blank">source</a>]








### Spring.IsUnitInJammer
---
```lua
function Spring.IsUnitInJammer(
  unitID: integer,
  allyTeamID: integer
) -> inJammer boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8268-L8274" target="_blank">source</a>]








### Spring.GetModelRootPiece
---
```lua
function Spring.GetModelRootPiece(modelName: string) -> index number
```

@return `index` - of the root piece





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8567-L8572" target="_blank">source</a>]








### Spring.GetModelPieceMap
---
```lua
function Spring.GetModelPieceMap(modelName: string) -> pieceInfos table<string,number>?
```

@return `pieceInfos` - where keys are piece names and values are indices





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8577-L8582" target="_blank">source</a>]








### Spring.GetModelPieceList
---
```lua
function Spring.GetModelPieceList(modelName: string) -> pieceNames string[]?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8588-L8593" target="_blank">source</a>]








### Spring.GetUnitRootPiece
---
```lua
function Spring.GetUnitRootPiece(unitID: integer) -> index number
```

@return `index` - of the root piece





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8599-L8604" target="_blank">source</a>]








### Spring.GetUnitPieceMap
---
```lua
function Spring.GetUnitPieceMap(unitID: integer) -> pieceInfos table<string,number>?
```

@return `pieceInfos` - where keys are piece names and values are indices





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8609-L8614" target="_blank">source</a>]








### Spring.GetUnitPieceList
---
```lua
function Spring.GetUnitPieceList(unitID: integer) -> pieceNames string[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8620-L8625" target="_blank">source</a>]








### Spring.GetUnitPieceInfo
---
```lua
function Spring.GetUnitPieceInfo(
  unitID: integer,
  pieceIndex: integer
) -> pieceInfo PieceInfo?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8631-L8637" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8643-L8654" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8660-L8668" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8674-L8682" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8688-L8708" target="_blank">source</a>]








### Spring.GetFeatureRootPiece
---
```lua
function Spring.GetFeatureRootPiece(featureID: integer) -> index number
```

@return `index` - of the root piece





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8713-L8718" target="_blank">source</a>]








### Spring.GetFeaturePieceMap
---
```lua
function Spring.GetFeaturePieceMap(featureID: integer) -> pieceInfos table<string,number>
```

@return `pieceInfos` - where keys are piece names and values are indices





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8723-L8728" target="_blank">source</a>]








### Spring.GetFeaturePieceList
---
```lua
function Spring.GetFeaturePieceList(featureID: integer) -> pieceNames string[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8734-L8739" target="_blank">source</a>]








### Spring.GetFeaturePieceInfo
---
```lua
function Spring.GetFeaturePieceInfo(
  featureID: integer,
  pieceIndex: integer
) -> pieceInfo PieceInfo?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8745-L8751" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8757-L8768" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8774-L8782" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8788-L8796" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8802-L8822" target="_blank">source</a>]








### Spring.GetUnitScriptPiece
---
```lua
function Spring.GetUnitScriptPiece(unitID: integer) -> pieceIndices integer[]
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8827-L8833" target="_blank">source</a>]








### Spring.GetUnitScriptPiece
---
```lua
function Spring.GetUnitScriptPiece(
  unitID: integer,
  scriptPiece: integer
) -> pieceIndex integer
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8834-L8841" target="_blank">source</a>]








### Spring.GetUnitScriptNames
---
```lua
function Spring.GetUnitScriptNames(unitID: integer) -> where table<string,number>
```

@return `where` - keys are piece names and values are piece indices





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8873-L8880" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8916-L8934" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8945-L8963" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedRead.cpp#L8983-L8992" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L856-L863" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L879-L888" target="_blank">source</a>]








### Spring.AssignPlayerToTeam
---
```lua
function Spring.AssignPlayerToTeam(
  playerID: integer,
  teamID: integer
) ->  nil
```





Assigns a player to a team.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L911-L917" target="_blank">source</a>]








### Spring.SetGlobalLos
---
```lua
function Spring.SetGlobalLos(
  allyTeamID: integer,
  globallos: boolean
) ->  nil
```





Changes access to global line of sight for a team and its allies.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L939-L945" target="_blank">source</a>]








### Spring.KillTeam
---
```lua
function Spring.KillTeam(teamID: integer) ->  nil
```





Will declare a team to be dead (no further orders can be assigned to such teams units).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L963-L971" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L994-L1002" target="_blank">source</a>]







@*returns* integer Number of accepted (valid) ally teams.


### Spring.SetTidal
---
```lua
function Spring.SetTidal(strength: number) ->  nil
```





Set tidal strength

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1039-L1044" target="_blank">source</a>]








### Spring.SetWind
---
```lua
function Spring.SetWind(
  minStrength: number,
  maxStrength: number
) ->  nil
```





Set wind strength

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1052-L1058" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1065-L1072" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1101-L1110" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1111-L1119" target="_blank">source</a>]








### Spring.SetTeamResource
---
```lua
function Spring.SetTeamResource(
  teamID: integer,
  resource: (ResourceName|StorageName),
  amount: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1197-L1203" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1247-L1254" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1284-L1292" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1442-L1448" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1456-L1463" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1474-L1481" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1497-L1505" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1518-L1525" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1582-L1589" target="_blank">source</a>]








### Spring.GetCOBScriptID
---
```lua
function Spring.GetCOBScriptID(
  unitID: integer,
  funcName: string
) -> funcID integer?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1643-L1648" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1682-L1699" target="_blank">source</a>]





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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1784-L1793" target="_blank">source</a>]





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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1829-L1835" target="_blank">source</a>]








### Spring.SetUnitCosts
---
```lua
function Spring.SetUnitCosts(
  unitID: integer,
  where: table<number,number>
) ->  nil
```
@param `where` - keys and values are, respectively and in this order: buildTime=amount, metalCost=amount, energyCost=amount






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L1874-L1879" target="_blank">source</a>]








### Spring.SetUnitResourcing
---
```lua
function Spring.SetUnitResourcing(
  unitID: integer,
  res: string,
  amount: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2003-L2009" target="_blank">source</a>]








### Spring.SetUnitResourcing
---
```lua
function Spring.SetUnitResourcing(
  unitID: integer,
  res: table<string,number>
) ->  nil
```
@param `res` - keys are: "[u|c][u|m][m|e]" unconditional | conditional, use | make, metal | energy. Values are amounts






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2011-L2016" target="_blank">source</a>]








### Spring.SetUnitStorage
---
```lua
function Spring.SetUnitStorage(
  unitID: integer,
  res: string,
  amount: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2049-L2054" target="_blank">source</a>]








### Spring.SetUnitStorage
---
```lua
function Spring.SetUnitStorage(
  unitID: integer,
  res: ResourceUsage
)
```
@param `res` - keys are: "[m|e]" metal | energy. Values are amounts






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2056-L2060" target="_blank">source</a>]








### Spring.SetUnitTooltip
---
```lua
function Spring.SetUnitTooltip(
  unitID: integer,
  tooltip: string
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2088-L2093" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2119-L2132" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2184-L2189" target="_blank">source</a>]








### Spring.SetUnitStockpile
---
```lua
function Spring.SetUnitStockpile(
  unitID: integer,
  stockpile: number?,
  buildPercent: number?
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2203-L2209" target="_blank">source</a>]








### Spring.SetUnitUseWeapons
---
```lua
function Spring.SetUnitUseWeapons(
  unitID: integer,
  forceUseWeapons: number?,
  allowUseWeapons: number?
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2343-L2350" target="_blank">source</a>]








### Spring.SetUnitWeaponState
---
```lua
function Spring.SetUnitWeaponState(
  unitID: integer,
  weaponNum: number,
  states: WeaponState
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2363-L2369" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2371-L2378" target="_blank">source</a>]








### Spring.SetUnitWeaponDamages
---
```lua
function Spring.SetUnitWeaponDamages(
  unitID: integer,
  weaponNum: (number|"selfDestruct"|"explode"),
  damages: WeaponDamages
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2498-L2504" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2505-L2512" target="_blank">source</a>]








### Spring.SetUnitMaxRange
---
```lua
function Spring.SetUnitMaxRange(
  unitID: integer,
  maxRange: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2558-L2563" target="_blank">source</a>]








### Spring.SetUnitExperience
---
```lua
function Spring.SetUnitExperience(
  unitID: integer,
  experience: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2576-L2583" target="_blank">source</a>]





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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2595-L2602" target="_blank">source</a>]





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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2616-L2622" target="_blank">source</a>]








### Spring.SetUnitLosMask
---
```lua
function Spring.SetUnitLosMask(
  unitID: integer,
  allyTeam: number,
  losTypes: (number|table)
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2690-L2713" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2738-L2744" target="_blank">source</a>]








### Spring.SetUnitCloak
---
```lua
function Spring.SetUnitCloak(
  unitID: integer,
  cloak: (boolean|number),
  cloakArg: (boolean|number)
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2766-L2784" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2815-L2820" target="_blank">source</a>]








### Spring.SetUnitSonarStealth
---
```lua
function Spring.SetUnitSonarStealth(
  unitID: integer,
  sonarStealth: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2833-L2838" target="_blank">source</a>]








### Spring.SetUnitSeismicSignature
---
```lua
function Spring.SetUnitSeismicSignature(
  unitID: integer,
  seismicSignature: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2850-L2855" target="_blank">source</a>]








### Spring.SetUnitAlwaysVisible
---
```lua
function Spring.SetUnitAlwaysVisible(
  unitID: integer,
  alwaysVisible: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2866-L2871" target="_blank">source</a>]








### Spring.SetUnitUseAirLos
---
```lua
function Spring.SetUnitUseAirLos(
  unitID: integer,
  useAirLos: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2878-L2884" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2891-L2897" target="_blank">source</a>]








### Spring.SetUnitHarvestStorage
---
```lua
function Spring.SetUnitHarvestStorage(
  unitID: integer,
  metal: number
) ->  nil
```





See also harvestStorage UnitDef tag.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2918-L2924" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2939-L2946" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L2973-L2982" target="_blank">source</a>]








### Spring.SetUnitNanoPieces
---
```lua
function Spring.SetUnitNanoPieces(
  builderID: integer,
  pieces: table
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3024-L3034" target="_blank">source</a>]

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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3084-L3095" target="_blank">source</a>]








### Spring.SetUnitCrashing
---
```lua
function Spring.SetUnitCrashing(
  unitID: integer,
  crashing: boolean
) -> success boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3102-L3107" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3137-L3144" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3175-L3181" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3212-L3220" target="_blank">source</a>]








### Spring.SetUnitPhysicalStateBit
---
```lua
function Spring.SetUnitPhysicalStateBit(
  unitID: integer,
  Physical
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3259-L3264" target="_blank">source</a>]








### Spring.GetUnitPhysicalState
---
```lua
function Spring.GetUnitPhysicalState(unitID: integer) -> Unit number
```

@return `Unit` - 's PhysicalState bitmask





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3278-L3282" target="_blank">source</a>]








### Spring.SetUnitNeutral
---
```lua
function Spring.SetUnitNeutral(
  unitID: integer,
  neutral: boolean
) -> setNeutral boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3301-L3308" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3321-L3330" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3332-L3342" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3401-L3412" target="_blank">source</a>]








### Spring.SetUnitRadiusAndHeight
---
```lua
function Spring.SetUnitRadiusAndHeight(
  unitID: integer,
  radius: number,
  height: number
) -> success boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3449-L3455" target="_blank">source</a>]








### Spring.SetUnitBuildeeRadius
---
```lua
function Spring.SetUnitBuildeeRadius(
  unitID: integer,
  build: number
) ->  nil
```
@param `build` - radius for when targeted by build, repair, reclaim-type commands.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3485-L3491" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3505-L3512" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3546-L3556" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3582-L3615" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3622-L3636" target="_blank">source</a>]








### Spring.SetUnitPieceVisible
---
```lua
function Spring.SetUnitPieceVisible(
  unitID: integer,
  pieceIndex: number,
  visible: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3643-L3650" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3657-L3663" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3706-L3723" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3747-L3758" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3784-L3793" target="_blank">source</a>]








### Spring.ClearUnitGoal
---
```lua
function Spring.ClearUnitGoal(unitID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3814-L3818" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3831-L3847" target="_blank">source</a>]








### Spring.SetUnitMass
---
```lua
function Spring.SetUnitMass(
  unitID: integer,
  mass: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3853-L3858" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3865-L3875" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3878-L3888" target="_blank">source</a>]

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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3920-L3928" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3935-L3948" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3950-L3964" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L3990-L4002" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4008-L4015" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4022-L4033" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4056-L4069" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4098-L4110" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4150-L4158" target="_blank">source</a>]








### Spring.AddUnitSeismicPing
---
```lua
function Spring.AddUnitSeismicPing(
  unitID: integer,
  pindSize: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4175-L4180" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4195-L4201" target="_blank">source</a>]








### Spring.UseUnitResource
---
```lua
function Spring.UseUnitResource(
  unitID: integer,
  resource: ResourceName,
  amount: number
) -> okay boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4225-L4231" target="_blank">source</a>]








### Spring.UseUnitResource
---
```lua
function Spring.UseUnitResource(
  unitID: integer,
  resources: ResourceUsage
) -> okay boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4233-L4238" target="_blank">source</a>]








### Spring.AddObjectDecal
---
```lua
function Spring.AddObjectDecal(unitID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4303-L4308" target="_blank">source</a>]








### Spring.RemoveObjectDecal
---
```lua
function Spring.RemoveObjectDecal(unitID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4321-L4325" target="_blank">source</a>]








### Spring.AddGrass
---
```lua
function Spring.AddGrass(
  x: number,
  z: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4344-L4349" target="_blank">source</a>]








### Spring.RemoveGrass
---
```lua
function Spring.RemoveGrass(
  x: number,
  z: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4359-L4364" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4380-L4390" target="_blank">source</a>]








### Spring.DestroyFeature
---
```lua
function Spring.DestroyFeature(featureDefID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4468-L4472" target="_blank">source</a>]








### Spring.TransferFeature
---
```lua
function Spring.TransferFeature(
  featureDefID: integer,
  teamID: integer
) ->  nil
```





Feature Control

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4491-L4497" target="_blank">source</a>]








### Spring.SetFeatureAlwaysVisible
---
```lua
function Spring.SetFeatureAlwaysVisible(
  featureID: integer,
  enable: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4514-L4519" target="_blank">source</a>]








### Spring.SetFeatureUseAirLos
---
```lua
function Spring.SetFeatureUseAirLos(
  featureID: integer,
  useAirLos: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4525-L4531" target="_blank">source</a>]








### Spring.SetFeatureHealth
---
```lua
function Spring.SetFeatureHealth(
  featureID: integer,
  health: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4538-L4543" target="_blank">source</a>]








### Spring.SetFeatureMaxHealth
---
```lua
function Spring.SetFeatureMaxHealth(
  featureID: integer,
  maxHealth: number
) ->  nil
```
@param `maxHealth` - minimum 0.1






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4556-L4562" target="_blank">source</a>]








### Spring.SetFeatureReclaim
---
```lua
function Spring.SetFeatureReclaim(
  featureID: integer,
  reclaimLeft: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4576-L4581" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4593-L4603" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4622-L4630" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4658-L4670" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4672-L4702" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4733-L4749" target="_blank">source</a>]








### Spring.SetFeatureMass
---
```lua
function Spring.SetFeatureMass(
  featureID: integer,
  mass: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4756-L4761" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4768-L4776" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4794-L4802" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4809-L4822" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4824-L4838" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4844-L4856" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4862-L4869" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4876-L4887" target="_blank">source</a>]








### Spring.SetFeatureNoSelect
---
```lua
function Spring.SetFeatureNoSelect(
  featureID: integer,
  noSelect: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4894-L4899" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4912-L4926" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4963-L4969" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L4998-L5014" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5021-L5036" target="_blank">source</a>]








### Spring.SetFeaturePieceVisible
---
```lua
function Spring.SetFeaturePieceVisible(
  featureID: integer,
  pieceIndex: number,
  visible: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5042-L5049" target="_blank">source</a>]








### Spring.SetProjectileAlwaysVisible
---
```lua
function Spring.SetProjectileAlwaysVisible(
  projectileID: integer,
  alwaysVisible: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5078-L5083" target="_blank">source</a>]








### Spring.SetProjectileUseAirLos
---
```lua
function Spring.SetProjectileUseAirLos(
  projectileID: integer,
  useAirLos: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5090-L5096" target="_blank">source</a>]








### Spring.SetProjectileMoveControl
---
```lua
function Spring.SetProjectileMoveControl(
  projectileID: integer,
  enable: boolean
) ->  nil
```





Disables engine movecontrol, so lua can fully control the physics.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5103-L5109" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5123-L5130" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5145-L5153" target="_blank">source</a>]








### Spring.SetProjectileCollision
---
```lua
function Spring.SetProjectileCollision(projectileID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5159-L5163" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5175-L5193" target="_blank">source</a>]

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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5264-L5269" target="_blank">source</a>]








### Spring.SetProjectileIsIntercepted
---
```lua
function Spring.SetProjectileIsIntercepted(projectileID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5286-L5290" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5306-L5313" target="_blank">source</a>]








### Spring.SetProjectileIgnoreTrackingError
---
```lua
function Spring.SetProjectileIgnoreTrackingError(
  projectileID: integer,
  ignore: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5343-L5348" target="_blank">source</a>]








### Spring.SetProjectileGravity
---
```lua
function Spring.SetProjectileGravity(
  projectileID: integer,
  grav: number?
) ->  nil
```
@param `grav` - (Default: `0`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5373-L5378" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5396-L5406" target="_blank">source</a>]








### Spring.SetProjectileCEG
---
```lua
function Spring.SetProjectileCEG(
  projectileID: integer,
  ceg_name: string
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5428-L5433" target="_blank">source</a>]








### Spring.UnitFinishCommand
---
```lua
function Spring.UnitFinishCommand(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5467-L5470" target="_blank">source</a>]








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

@param `timeout` - Absolute frame number. The command will be discarded after this frame. Only respected by mobile units.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5486-L5494" target="_blank">source</a>]








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

@param `timeout` - Absolute frame number. The command will be discarded after this frame. Only respected by mobile units.


@return `unitsOrdered` - The number of units ordered.





Give order to multiple units, specified by table keys.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5523-L5533" target="_blank">source</a>]








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

@param `timeout` - Absolute frame number. The command will be discarded after this frame. Only respected by mobile units.


@return `unitsOrdered` - The number of units ordered.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5568-L5577" target="_blank">source</a>]








### Spring.GiveOrderArrayToUnit
---
```lua
function Spring.GiveOrderArrayToUnit(
  unitID: integer,
  commands: CreateCommand[]
) -> ordersGiven boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5614-L5620" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5655-L5660" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5697-L5709" target="_blank">source</a>]








### Spring.LevelHeightMap
---
```lua
function Spring.LevelHeightMap(
  x: number,
  z: number,
  height: number
)
```





Set the height of a point in the world.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5827-L5834" target="_blank">source</a>]








### Spring.LevelHeightMap
---
```lua
function Spring.LevelHeightMap(
  x1: number,
  z1: number,
  x2: number,
  z2: number,
  height: number?
) ->  nil
```





Set the height of a rectangle area in the world.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5835-L5845" target="_blank">source</a>]








### Spring.AdjustHeightMap
---
```lua
function Spring.AdjustHeightMap(
  x: number,
  z: number,
  height: number
)
```





Add height to a point in the world.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5866-L5873" target="_blank">source</a>]








### Spring.AdjustHeightMap
---
```lua
function Spring.AdjustHeightMap(
  x1: number,
  z1: number,
  x2: number,
  z2: number,
  height: number?
) ->  nil
```





Add height to a rectangle in the world.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5875-L5885" target="_blank">source</a>]








### Spring.RevertHeightMap
---
```lua
function Spring.RevertHeightMap(
  x: number,
  z: number,
  height: number
)
```





Restore map height at a point in the world.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5907-L5914" target="_blank">source</a>]








### Spring.RevertHeightMap
---
```lua
function Spring.RevertHeightMap(
  x1: number,
  z1: number,
  x2: number,
  z2: number,
  height: number?
) ->  nil
```





Restore map height of a rectangle area in the world.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5915-L5925" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L5964-L5971" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6009-L6021" target="_blank">source</a>]

Can only be called in `Spring.SetHeightMapFunc`.








### Spring.SetHeightMapFunc
---
```lua
function Spring.SetHeightMapFunc(
  luaFunction: function,
  arg: number,
  ...: number
) -> absTotalHeightMapAmountChanged integer?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6068-L6087" target="_blank">source</a>]

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
  x: number,
  z: number,
  height: number
)
```





Set the height of a point in the original map height cache.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6133-L6140" target="_blank">source</a>]








### Spring.LevelOriginalHeightMap
---
```lua
function Spring.LevelOriginalHeightMap(
  x1: number,
  z1: number,
  x2: number,
  z2: number,
  height: number
)
```





Set the height of a rectangle area in the original map height cache.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6141-L6150" target="_blank">source</a>]








### Spring.AdjustOriginalHeightMap
---
```lua
function Spring.AdjustOriginalHeightMap(
  x: number,
  z: number,
  height: number
)
```





Add height to a point in the original map height cache.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6169-L6176" target="_blank">source</a>]








### Spring.AdjustOriginalHeightMap
---
```lua
function Spring.AdjustOriginalHeightMap(
  x1: number,
  z1: number,
  x2: number,
  z2: number,
  height: number
)
```





Add height to a rectangle area in the original map height cache.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6177-L6186" target="_blank">source</a>]








### Spring.RevertOriginalHeightMap
---
```lua
function Spring.RevertOriginalHeightMap(
  x: number,
  z: number,
  height: number
)
```





Restore original map height at a point in the world.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6208-L6215" target="_blank">source</a>]








### Spring.RevertOriginalHeightMap
---
```lua
function Spring.RevertOriginalHeightMap(
  x1: number,
  z1: number,
  x2: number,
  z2: number,
  height: number?
) ->  nil
```





Restore original map height over a rectangle in the world.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6216-L6226" target="_blank">source</a>]








### Spring.AddOriginalHeightMap
---
```lua
function Spring.AddOriginalHeightMap(
  x: number,
  y: number,
  height: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6264-L6274" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6306-L6317" target="_blank">source</a>]

Can only be called in `Spring.SetOriginalHeightMapFunc`








### Spring.SetOriginalHeightMapFunc
---
```lua
function Spring.SetOriginalHeightMapFunc(heightMapFunc: function) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6358-L6366" target="_blank">source</a>]

Cannot recurse on itself








### Spring.RebuildSmoothMesh
---
```lua
function Spring.RebuildSmoothMesh() ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6410-L6417" target="_blank">source</a>]

Heightmap changes normally take up to 25s to propagate to the smooth mesh.
Use to force a mapwide update immediately.








### Spring.LevelSmoothMesh
---
```lua
function Spring.LevelSmoothMesh(
  x: number,
  z: number,
  height: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6425-L6430" target="_blank">source</a>]








### Spring.LevelSmoothMesh
---
```lua
function Spring.LevelSmoothMesh(
  x1: number,
  z1: number,
  x2: number,
  z2: number,
  height: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6431-L6438" target="_blank">source</a>]








### Spring.AdjustSmoothMesh
---
```lua
function Spring.AdjustSmoothMesh(
  x: number,
  z: number,
  height: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6457-L6462" target="_blank">source</a>]








### Spring.AdjustSmoothMesh
---
```lua
function Spring.AdjustSmoothMesh(
  x1: number,
  z1: number,
  x2: number,
  z2: number,
  height: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6463-L6470" target="_blank">source</a>]








### Spring.RevertSmoothMesh
---
```lua
function Spring.RevertSmoothMesh(
  x: number,
  z: number,
  origFactor: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6487-L6492" target="_blank">source</a>]








### Spring.RevertSmoothMesh
---
```lua
function Spring.RevertSmoothMesh(
  x1: number,
  z1: number,
  x2: number,
  z2: number,
  origFactor: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6493-L6500" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6534-L6541" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6572-L6580" target="_blank">source</a>]








### Spring.SetSmoothMeshFunc
---
```lua
function Spring.SetSmoothMeshFunc(
  luaFunction: function,
  arg: any,
  ...: any
) -> absTotalHeightMapAmountChanged number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6620-L6626" target="_blank">source</a>]








### Spring.SetMapSquareTerrainType
---
```lua
function Spring.SetMapSquareTerrainType(
  x: number,
  z: number,
  newType: number
) -> oldType integer?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6661-L6667" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6691-L6699" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6738-L6746" target="_blank">source</a>]








### Spring.UnitWeaponFire
---
```lua
function Spring.UnitWeaponFire(
  unitID: integer,
  weaponID: integer
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6770-L6775" target="_blank">source</a>]








### Spring.UnitWeaponHoldFire
---
```lua
function Spring.UnitWeaponHoldFire(
  unitID: integer,
  weaponID: integer
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6793-L6798" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6814-L6828" target="_blank">source</a>]








### Spring.UnitAttach
---
```lua
function Spring.UnitAttach(
  transporterID: integer,
  passengerID: integer,
  pieceNum: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6841-L6848" target="_blank">source</a>]








### Spring.UnitDetach
---
```lua
function Spring.UnitDetach(passengerID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6880-L6884" target="_blank">source</a>]








### Spring.UnitDetachFromAir
---
```lua
function Spring.UnitDetachFromAir(passengerID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6902-L6906" target="_blank">source</a>]








### Spring.SetUnitLoadingTransport
---
```lua
function Spring.SetUnitLoadingTransport(
  passengerID: integer,
  transportID: integer
) ->  nil
```





Disables collisions between the two units to allow colvol intersection during the approach.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6934-L6940" target="_blank">source</a>]








### Spring.SpawnProjectile
---
```lua
function Spring.SpawnProjectile(
  weaponDefID: integer,
  projectileParams: ProjectileParams
) -> projectileID integer?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6964-L6970" target="_blank">source</a>]








### Spring.DeleteProjectile
---
```lua
function Spring.DeleteProjectile(projectileID: integer) ->  nil
```





Silently removes projectiles (no explosion).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L6986-L6991" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L7132-L7142" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L7204-L7217" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L7236-L7251" target="_blank">source</a>]








### Spring.SetNoPause
---
```lua
function Spring.SetNoPause(noPause: boolean) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L7276-L7280" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L7294-L7302" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L7325-L7333" target="_blank">source</a>]








### Spring.EditUnitCmdDesc
---
```lua
function Spring.EditUnitCmdDesc(
  unitID: integer,
  cmdDescID: integer,
  cmdArray: CommandDescription
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L7460-L7465" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L7492-L7499" target="_blank">source</a>]








### Spring.InsertUnitCmdDesc
---
```lua
function Spring.InsertUnitCmdDesc(
  unitID: integer,
  cmdDesc: CommandDescription
)
```





Insert a command description at the last position.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L7500-L7506" target="_blank">source</a>]








### Spring.RemoveUnitCmdDesc
---
```lua
function Spring.RemoveUnitCmdDesc(
  unitID: integer,
  cmdDescID: integer?
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L7541-L7545" target="_blank">source</a>]








### Spring.Ping
---
```lua
function Spring.Ping(pingTag: number) ->  nil
```





Send a ping request to the server

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L460-L467" target="_blank">source</a>]








### Spring.SendCommands
---
```lua
function Spring.SendCommands(commands: string[])
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L492-L495" target="_blank">source</a>]








### Spring.SendCommands
---
```lua
function Spring.SendCommands(
  command: string,
  ...: string
) ->  nil
```
@param `...` - additional commands






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L497-L502" target="_blank">source</a>]








### Spring.SendPublicChat
---
```lua
function Spring.SendPublicChat(message: string) ->  nil
```





Sends a chat message to everyone (players and spectators).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L576-L581" target="_blank">source</a>]








### Spring.SendAllyChat
---
```lua
function Spring.SendAllyChat(message: string) ->  nil
```





Sends a chat message to the sender's ally team (if a spectator, to other spectators).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L592-L597" target="_blank">source</a>]








### Spring.SendSpectatorChat
---
```lua
function Spring.SendSpectatorChat(message: string) ->  nil
```





Sends a chat message to spectators. Works even if you're a player.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L607-L612" target="_blank">source</a>]








### Spring.SendPrivateChat
---
```lua
function Spring.SendPrivateChat(
  message: string,
  playerID: integer
) ->  nil
```





Sends a private chat message to a specific player ID.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L622-L628" target="_blank">source</a>]








### Spring.SendMessage
---
```lua
function Spring.SendMessage(message: string) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L652-L655" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L663-L672" target="_blank">source</a>]








### Spring.SendMessageToPlayer
---
```lua
function Spring.SendMessageToPlayer(
  playerID: integer,
  message: string
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L682-L686" target="_blank">source</a>]








### Spring.SendMessageToTeam
---
```lua
function Spring.SendMessageToTeam(
  teamID: integer,
  message: string
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L696-L700" target="_blank">source</a>]








### Spring.SendMessageToAllyTeam
---
```lua
function Spring.SendMessageToAllyTeam(
  allyID: integer,
  message: string
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L710-L714" target="_blank">source</a>]








### Spring.LoadSoundDef
---
```lua
function Spring.LoadSoundDef(soundfile: string) -> success boolean
```





Loads a SoundDefs file, the format is the same as in `gamedata/sounds.lua`.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L730-L735" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L762-L773" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L853-L864" target="_blank">source</a>]

Multiple sound streams may be played at once.








### Spring.StopSoundStream
---
```lua
function Spring.StopSoundStream() ->  nil
```





Terminates any SoundStream currently running.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L877-L881" target="_blank">source</a>]








### Spring.PauseSoundStream
---
```lua
function Spring.PauseSoundStream() ->  nil
```





Pause any SoundStream currently running.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L889-L893" target="_blank">source</a>]








### Spring.SetSoundStreamVolume
---
```lua
function Spring.SetSoundStreamVolume(volume: number) ->  nil
```





Set volume for SoundStream

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L901-L906" target="_blank">source</a>]








### Spring.SetSoundEffectParams
---
```lua
function Spring.SetSoundEffectParams()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L914-L915" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1024-L1032" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1044-L1052" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1064-L1074" target="_blank">source</a>]








### Spring.DrawUnitCommands
---
```lua
function Spring.DrawUnitCommands(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1097-L1100" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1101-L1105" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1106-L1111" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1171-L1180" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1209-L1220" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1234-L1255" target="_blank">source</a>]








### Spring.RunDollyCamera
---
```lua
function Spring.RunDollyCamera(runtime: number) ->  nil
```
@param `runtime` - Runtime in milliseconds.






Runs Dolly Camera

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1278-L1283" target="_blank">source</a>]








### Spring.PauseDollyCamera
---
```lua
function Spring.PauseDollyCamera(fraction: number) ->  nil
```
@param `fraction` - Fraction of the total runtime to pause at, 0 to 1 inclusive. A null value pauses at current percent






Pause Dolly Camera

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1293-L1298" target="_blank">source</a>]








### Spring.ResumeDollyCamera
---
```lua
function Spring.ResumeDollyCamera() ->  nil
```





Resume Dolly Camera

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1308-L1312" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1320-L1327" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1350-L1357" target="_blank">source</a>]








### Spring.SetDollyCameraMode
---
```lua
function Spring.SetDollyCameraMode(mode: (1|2)) ->  nil
```
@param `mode` - `1` static position, `2` nurbs curve






Sets Dolly Camera movement mode

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1373-L1378" target="_blank">source</a>]








### Spring.SetDollyCameraRelativeMode
---
```lua
function Spring.SetDollyCameraRelativeMode(relativeMode: number) ->  nil
```
@param `relativeMode` - `1` world, `2` look target






Sets Dolly Camera movement curve to world relative or look target relative

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1388-L1393" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1404-L1411" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1428-L1435" target="_blank">source</a>]








### Spring.SetDollyCameraLookUnit
---
```lua
function Spring.SetDollyCameraLookUnit(unitID: integer) ->  nil
```
@param `unitID` - The unit to look at.






Sets target unit for Dolly Camera to look towards

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1448-L1453" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1471-L1477" target="_blank">source</a>]








### Spring.DeselectUnit
---
```lua
function Spring.DeselectUnit(unitID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1495-L1500" target="_blank">source</a>]








### Spring.DeselectUnitArray
---
```lua
function Spring.DeselectUnitArray(unitIDs: integer[]) ->  nil
```
@param `unitIDs` - Table with unit IDs as values.






Deselects multiple units.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1537-L1542" target="_blank">source</a>]








### Spring.DeselectUnitMap
---
```lua
function Spring.DeselectUnitMap(unitMap: table<integer,any>) ->  nil
```
@param `unitMap` - Table with unit IDs as keys.






Deselects multiple units.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1548-L1553" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1559-L1565" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1571-L1577" target="_blank">source</a>]








### Spring.AddMapLight
---
```lua
function Spring.AddMapLight(lightParams: LightParams) -> lightHandle integer
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1728-L1735" target="_blank">source</a>]

requires MaxDynamicMapLights > 0








### Spring.AddModelLight
---
```lua
function Spring.AddModelLight(lightParams: LightParams) -> lightHandle number
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1754-L1761" target="_blank">source</a>]

requires MaxDynamicMapLights > 0








### Spring.UpdateMapLight
---
```lua
function Spring.UpdateMapLight(
  lightHandle: number,
  lightParams: LightParams
) -> success boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1780-L1786" target="_blank">source</a>]








### Spring.UpdateModelLight
---
```lua
function Spring.UpdateModelLight(
  lightHandle: number,
  lightParams: LightParams
) -> success boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1802-L1808" target="_blank">source</a>]








### Spring.AddLightTrackingTarget
---
```lua
function Spring.AddLightTrackingTarget()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1824-L1826" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1881-L1890" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1917-L1926" target="_blank">source</a>]








### Spring.SetMapShader
---
```lua
function Spring.SetMapShader(
  standardShaderID: integer,
  deferredShaderID: integer
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1959-L1968" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L1988-L1993" target="_blank">source</a>]








### Spring.SetMapShadingTexture
---
```lua
function Spring.SetMapShadingTexture(
  texType: string,
  texName: string
) -> success boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2085-L2090" target="_blank">source</a>]







@*usage* Spring.SetMapShadingTexture("$ssmf_specular", "name_of_my_shiny_texture")


### Spring.SetSkyBoxTexture
---
```lua
function Spring.SetSkyBoxTexture(texName: string) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2106-L2109" target="_blank">source</a>]








### Spring.SetUnitNoDraw
---
```lua
function Spring.SetUnitNoDraw(
  unitID: integer,
  noDraw: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2128-L2134" target="_blank">source</a>]








### Spring.SetUnitEngineDrawMask
---
```lua
function Spring.SetUnitEngineDrawMask(
  unitID: integer,
  drawMask: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2147-L2153" target="_blank">source</a>]








### Spring.SetUnitAlwaysUpdateMatrix
---
```lua
function Spring.SetUnitAlwaysUpdateMatrix(
  unitID: integer,
  alwaysUpdateMatrix: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2166-L2172" target="_blank">source</a>]








### Spring.SetUnitNoMinimap
---
```lua
function Spring.SetUnitNoMinimap(
  unitID: integer,
  unitNoMinimap: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2185-L2191" target="_blank">source</a>]








### Spring.SetMiniMapRotation
---
```lua
function Spring.SetMiniMapRotation(rotation: number) ->  nil
```
@param `rotation` - amount in radians






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2203-L2207" target="_blank">source</a>]








### Spring.SetUnitNoGroup
---
```lua
function Spring.SetUnitNoGroup(
  unitID: integer,
  unitNoGroup: boolean
)
```
@param `unitNoGroup` - Whether unit can be added to selection groups






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2233-L2238" target="_blank">source</a>]








### Spring.SetUnitNoSelect
---
```lua
function Spring.SetUnitNoSelect(
  unitID: integer,
  unitNoSelect: boolean
) ->  nil
```
@param `unitNoSelect` - whether unit can be selected or not






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2255-L2261" target="_blank">source</a>]








### Spring.SetUnitLeaveTracks
---
```lua
function Spring.SetUnitLeaveTracks(
  unitID: integer,
  unitLeaveTracks: boolean
) ->  nil
```
@param `unitLeaveTracks` - whether unit leaves tracks on movement






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2283-L2289" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2302-L2317" target="_blank">source</a>]








### Spring.SetFeatureNoDraw
---
```lua
function Spring.SetFeatureNoDraw(
  featureID: integer,
  noDraw: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2335-L2343" target="_blank">source</a>]








### Spring.SetFeatureEngineDrawMask
---
```lua
function Spring.SetFeatureEngineDrawMask(
  featureID: integer,
  engineDrawMask: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2356-L2362" target="_blank">source</a>]








### Spring.SetFeatureAlwaysUpdateMatrix
---
```lua
function Spring.SetFeatureAlwaysUpdateMatrix(
  featureID: integer,
  alwaysUpdateMat: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2375-L2381" target="_blank">source</a>]








### Spring.SetFeatureFade
---
```lua
function Spring.SetFeatureFade(
  featureID: integer,
  allow: boolean
) ->  nil
```





Control whether a feature will fade or not when zoomed out.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2394-L2402" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2415-L2430" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2449-L2460" target="_blank">source</a>]








### Spring.FreeUnitIcon
---
```lua
function Spring.FreeUnitIcon(iconName: string) -> freed boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2479-L2486" target="_blank">source</a>]








### Spring.UnitIconSetDraw
---
```lua
function Spring.UnitIconSetDraw(
  unitID: integer,
  drawIcon: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2497-L2505" target="_blank">source</a>]

Use Spring.SetUnitIconDraw instead.








### Spring.SetUnitIconDraw
---
```lua
function Spring.SetUnitIconDraw(
  unitID: integer,
  drawIcon: boolean
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2513-L2519" target="_blank">source</a>]








### Spring.SetUnitDefIcon
---
```lua
function Spring.SetUnitDefIcon(
  unitDefID: integer,
  iconName: string
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2532-L2540" target="_blank">source</a>]








### Spring.SetUnitDefImage
---
```lua
function Spring.SetUnitDefImage(
  unitDefID: integer,
  image: string
) ->  nil
```
@param `image` - luaTexture|texFile






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2572-L2580" target="_blank">source</a>]








### Spring.ExtractModArchiveFile
---
```lua
function Spring.ExtractModArchiveFile(modfile: string) -> extracted boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2624-L2629" target="_blank">source</a>]








### Spring.CreateDir
---
```lua
function Spring.CreateDir(path: string) -> dirCreated boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2688-L2693" target="_blank">source</a>]








### Spring.SetActiveCommand
---
```lua
function Spring.SetActiveCommand(
  action: string,
  actionExtra: string?
) -> commandSet boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2766-L2771" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2773-L2783" target="_blank">source</a>]








### Spring.LoadCmdColorsConfig
---
```lua
function Spring.LoadCmdColorsConfig(config: string) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2805-L2808" target="_blank">source</a>]








### Spring.LoadCtrlPanelConfig
---
```lua
function Spring.LoadCtrlPanelConfig(config: string) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2816-L2819" target="_blank">source</a>]








### Spring.ForceLayoutUpdate
---
```lua
function Spring.ForceLayoutUpdate() ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2830-L2832" target="_blank">source</a>]








### Spring.SetDrawSelectionInfo
---
```lua
function Spring.SetDrawSelectionInfo(enable: boolean) ->  nil
```





Disables the "Selected Units x" box in the GUI.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2843-L2848" target="_blank">source</a>]








### Spring.SetBoxSelectionByEngine
---
```lua
function Spring.SetBoxSelectionByEngine(state: boolean) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2858-L2863" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2872-L2880" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2899-L2910" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2926-L2933" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2951-L2959" target="_blank">source</a>]








### Spring.WarpMouse
---
```lua
function Spring.WarpMouse(
  x: number,
  y: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L2998-L3002" target="_blank">source</a>]








### Spring.SetMouseCursor
---
```lua
function Spring.SetMouseCursor(
  cursorName: string,
  cursorScale: number?
) ->  nil
```
@param `cursorScale` - (Default: `1.0`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3012-L3016" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3033-L3040" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3081-L3091" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3115-L3122" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3143-L3150" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3166-L3173" target="_blank">source</a>]








### Spring.Quit
---
```lua
function Spring.Quit() ->  nil
```





Closes the application

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3244-L3248" target="_blank">source</a>]








### Spring.SetUnitGroup
---
```lua
function Spring.SetUnitGroup(
  unitID: integer,
  groupID: integer
) ->  nil
```
@param `groupID` - the group number to be assigned, or -1 for deassignment






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3260-L3266" target="_blank">source</a>]








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

@param `timeout` - Absolute frame number. The command will be discarded after this frame. Only respected by mobile units.






Give order to selected units.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3356-L3365" target="_blank">source</a>]








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

@param `timeout` - Absolute frame number. The command will be discarded after this frame. Only respected by mobile units.






Give order to specific unit.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3380-L3390" target="_blank">source</a>]








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

@param `timeout` - Absolute frame number. The command will be discarded after this frame. Only respected by mobile units.






Give order to multiple units, specified by table keys.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3414-L3424" target="_blank">source</a>]








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

@param `timeout` - Absolute frame number. The command will be discarded after this frame. Only respected by mobile units.


@return `ordersGiven` - `true` if any orders were sent, otherwise `false`.





Give order to an array of units.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3448-L3458" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3481-L3487" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3515-L3521" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3549-L3561" target="_blank">source</a>]








### Spring.SetBuildSpacing
---
```lua
function Spring.SetBuildSpacing(spacing: number) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3591-L3596" target="_blank">source</a>]








### Spring.SetBuildFacing
---
```lua
function Spring.SetBuildFacing(facing: FacingInteger) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3606-L3611" target="_blank">source</a>]








### Spring.SendLuaUIMsg
---
```lua
function Spring.SendLuaUIMsg(
  message: string,
  mode: string
) ->  nil
```
@param `mode` - "s"/"specs" | "a"/"allies"






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3628-L3632" target="_blank">source</a>]








### Spring.SendLuaGaiaMsg
---
```lua
function Spring.SendLuaGaiaMsg(message: string) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3653-L3656" target="_blank">source</a>]








### Spring.SendLuaRulesMsg
---
```lua
function Spring.SendLuaRulesMsg(message: string) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3672-L3675" target="_blank">source</a>]








### Spring.SendLuaMenuMsg
---
```lua
function Spring.SendLuaMenuMsg(msg: string)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3690-L3695" target="_blank">source</a>]








### Spring.SetShareLevel
---
```lua
function Spring.SetShareLevel(
  resource: string,
  shareLevel: number
) ->  nil
```
@param `resource` - metal | energy






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3711-L3718" target="_blank">source</a>]








### Spring.ShareResources
---
```lua
function Spring.ShareResources(
  teamID: integer,
  units: string
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3742-L3749" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3751-L3759" target="_blank">source</a>]








### Spring.SetLastMessagePosition
---
```lua
function Spring.SetLastMessagePosition(
  x: number,
  y: number,
  z: number
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3807-L3812" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3830-L3837" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3859-L3869" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3893-L3905" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L3945-L3955" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4019-L4027" target="_blank">source</a>]








### Spring.SetSunLighting
---
```lua
function Spring.SetSunLighting(params: { groundAmbientColor: rgb,groundDiffuseColor: rgb })
```





Modify sun lighting parameters.

```lua
Spring.SetSunLighting({groundAmbientColor = {1, 0.1, 1}, groundDiffuseColor = {1, 0.1, 1} })
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4037-L4046" target="_blank">source</a>]








### Spring.SetMapRenderingParams
---
```lua
function Spring.SetMapRenderingParams(params: MapRenderingParams) ->  nil
```





Allows to change map rendering params at runtime.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4095-L4100" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4161-L4167" target="_blank">source</a>]








### Spring.SendSkirmishAIMessage
---
```lua
function Spring.SendSkirmishAIMessage(
  aiTeam: number,
  message: string
) -> ai_processed boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4199-L4203" target="_blank">source</a>]








### Spring.SetLogSectionFilterLevel
---
```lua
function Spring.SetLogSectionFilterLevel(
  sectionName: string,
  logLevel: (string|number)?
) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4233-L4237" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4248-L4259" target="_blank">source</a>]








### Spring.SetAutoShowMetal
---
```lua
function Spring.SetAutoShowMetal(autoShow: boolean) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4280-L4283" target="_blank">source</a>]








### Spring.SetDrawSky
---
```lua
function Spring.SetDrawSky(drawSky: boolean) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4291-L4294" target="_blank">source</a>]








### Spring.SetDrawWater
---
```lua
function Spring.SetDrawWater(drawWater: boolean) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4302-L4305" target="_blank">source</a>]








### Spring.SetDrawGround
---
```lua
function Spring.SetDrawGround(drawGround: boolean) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4313-L4316" target="_blank">source</a>]








### Spring.SetDrawGroundDeferred
---
```lua
function Spring.SetDrawGroundDeferred(
  drawGroundDeferred: boolean,
  drawGroundForward: boolean?
) ->  nil
```
@param `drawGroundForward` - allows disabling of the forward pass






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4324-L4328" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4341-L4347" target="_blank">source</a>]








### Spring.SetVideoCapturingMode
---
```lua
function Spring.SetVideoCapturingMode(allowCaptureMode: boolean) ->  nil
```





This doesn't actually record the game in any way, it just regulates the framerate and interpolations.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4365-L4370" target="_blank">source</a>]








### Spring.SetVideoCapturingTimeOffset
---
```lua
function Spring.SetVideoCapturingTimeOffset(timeOffset: boolean) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4378-L4381" target="_blank">source</a>]








### Spring.SetWaterParams
---
```lua
function Spring.SetWaterParams(waterParams: WaterParams) ->  nil
```





Does not need cheating enabled.

Allows to change water params (mostly `BumpWater` ones) at runtime. You may
want to set `BumpWaterUseUniforms` in your `springrc` to 1, then you don't even
need to restart `BumpWater` via `/water 4`.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4433-L4443" target="_blank">source</a>]








### Spring.PreloadUnitDefModel
---
```lua
function Spring.PreloadUnitDefModel(unitDefID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4654-L4661" target="_blank">source</a>]

Allow the engine to load the unit's model (and texture) in a background thread.
Wreckages and buildOptions of a unit are automatically preloaded.








### Spring.PreloadFeatureDefModel
---
```lua
function Spring.PreloadFeatureDefModel(featureDefID: integer) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4673-L4677" target="_blank">source</a>]








### Spring.PreloadSoundItem
---
```lua
function Spring.PreloadSoundItem(name: string) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4689-L4693" target="_blank">source</a>]








### Spring.LoadModelTextures
---
```lua
function Spring.LoadModelTextures(modelName: string) -> success boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4705-L4709" target="_blank">source</a>]








### Spring.CreateGroundDecal
---
```lua
function Spring.CreateGroundDecal() -> decalID number?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4741-L4745" target="_blank">source</a>]








### Spring.DestroyGroundDecal
---
```lua
function Spring.DestroyGroundDecal(decalID: integer) -> delSuccess boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4757-L4762" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4770-L4780" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4819-L4832" target="_blank">source</a>]

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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4855-L4861" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4877-L4884" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4893-L4900" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4917-L4924" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4940-L4950" target="_blank">source</a>]

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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L4972-L4983" target="_blank">source</a>]

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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L5004-L5015" target="_blank">source</a>]

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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L5034-L5044" target="_blank">source</a>]

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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L5067-L5075" target="_blank">source</a>]








### Spring.SDLStartTextInput
---
```lua
function Spring.SDLStartTextInput() ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L5087-L5091" target="_blank">source</a>]








### Spring.SDLStopTextInput
---
```lua
function Spring.SDLStopTextInput() ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L5098-L5102" target="_blank">source</a>]








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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L5115-L5126" target="_blank">source</a>]








### Spring.SetWindowMinimized
---
```lua
function Spring.SetWindowMinimized() -> minimized boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L5145-L5149" target="_blank">source</a>]








### Spring.SetWindowMaximized
---
```lua
function Spring.SetWindowMaximized() -> maximized boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L5156-L5160" target="_blank">source</a>]








### Spring.Reload
---
```lua
function Spring.Reload(startScript: string) ->  nil
```
@param `startScript` - the CONTENT of the script.txt spring should use to start.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L5174-L5177" target="_blank">source</a>]








### Spring.Restart
---
```lua
function Spring.Restart(
  commandline_args: string,
  startScript: string
) ->  nil
```
@param `commandline_args` - commandline arguments passed to spring executable.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L5184-L5191" target="_blank">source</a>]

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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L5199-L5208" target="_blank">source</a>]

If this call returns, something went wrong








### Spring.SetWMIcon
---
```lua
function Spring.SetWMIcon(iconFileName: string) ->  nil
```





Sets the icon for the process which is seen in the OS task-bar and other places (default: spring-logo).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L5220-L5231" target="_blank">source</a>]

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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L5268-L5271" target="_blank">source</a>]








### Spring.SetClipboard
---
```lua
function Spring.SetClipboard(text: string) ->  nil
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L5289-L5292" target="_blank">source</a>]








### Spring.Yield
---
```lua
function Spring.Yield() -> when boolean
```

@return `when` - true caller should continue calling `Spring.Yield` during the widgets/gadgets load, when false it shouldn't call it any longer.





Relinquish control of the game loading thread and OpenGL context back to the UI (LuaIntro).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUnsyncedCtrl.cpp#L5300-L5314" target="_blank">source</a>]

Should be called after each widget/unsynced gadget is loaded in widget/gadget handler. Use it to draw screen updates and process windows events.







@*usage* local wantYield = Spring.Yield and Spring.Yield() -- nil check: not present in synced
for wupget in pairs(wupgetsToLoad) do
loadWupget(wupget)
wantYield = wantYield and Spring.Yield()
end


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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUtils.cpp#L1388-L1398" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaUtils.cpp#L1468-L1475" target="_blank">source</a>]








### Spring.GetMetalMapSize
---
```lua
function Spring.GetMetalMapSize()
 -> x integer
 -> z integer

```

@return `x` - X coordinate in worldspace / `Game.metalMapSquareSize`.

@return `z` - Z coordinate in worldspace / `Game.metalMapSquareSize`.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaMetalMap.cpp#L34-L38" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaMetalMap.cpp#L47-L53" target="_blank">source</a>]








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

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaMetalMap.cpp#L64-L71" target="_blank">source</a>]








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






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaMetalMap.cpp#L83-L88" target="_blank">source</a>]











## fields
---

### Spring.MoveCtrl
---
```lua
Spring.MoveCtrl : MoveCtrl
```



[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaSyncedCtrl.cpp#L359-L359" target="_blank">source</a>]










{% endraw %}