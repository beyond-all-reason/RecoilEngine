---
layout: default
title: gl
parent: Lua API
permalink: lua-api/globals/gl
---

{% raw %}

# global gl


Lua OpenGL API

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L86-L89" target="_blank">source</a>]







---

## methods
---

### gl.HasExtension
---
```lua
function gl.HasExtension(ext: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1145-L1149" target="_blank">source</a>]








### gl.GetNumber
---
```lua
function gl.GetNumber(
  pname: GL,
  count: integer?
) ->  number ...
```
@param `count` - (Default: `1`) Number of values to return, in range [1, 64].






Get the value or values of a selected parameter.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1157-L1164" target="_blank">source</a>]








### gl.GetString
---
```lua
function gl.GetString(pname: GL)
```





Get a string describing the current OpenGL connection.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1182-L1186" target="_blank">source</a>]








### gl.GetScreenViewTrans
---
```lua
function gl.GetScreenViewTrans()
 -> x number
 -> y number
 -> z number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1202-L1207" target="_blank">source</a>]








### gl.GetViewSizes
---
```lua
function gl.GetViewSizes()
 -> x number
 -> y number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1217-L1221" target="_blank">source</a>]








### gl.GetViewRange
---
```lua
function gl.GetViewRange()
 -> nearPlaneDist number
 -> farPlaneDist number
 -> minViewRange number
 -> maxViewRange number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1230-L1236" target="_blank">source</a>]








### gl.SlaveMiniMap
---
```lua
function gl.SlaveMiniMap(newMode: boolean)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1252-L1255" target="_blank">source</a>]








### gl.ConfigMiniMap
---
```lua
function gl.ConfigMiniMap(
  px: integer,
  py: integer,
  sx: integer,
  sy: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1267-L1273" target="_blank">source</a>]








### gl.DrawMiniMap
---
```lua
function gl.DrawMiniMap(defaultTransform: boolean?)
```
@param `defaultTransform` - (Default: `true`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1289-L1292" target="_blank">source</a>]








### gl.BeginText
---
```lua
function gl.BeginText(userDefinedBlending: boolean?)
```
@param `userDefinedBlending` - When `true` doesn't set the gl.BlendFunc automatically. Defaults to `false`.






Begin a block of text commands.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1329-L1343" target="_blank">source</a>]

Text can be drawn without Start/End, but when doing several operations it's more optimal
if done inside a block.

Also allows disabling automatic setting of the blend mode. Otherwise the font will always print
with `BlendFunc(GL.SRC_ALPHA, GL.ONE_MINUS_SRC_ALPHA)`.





@see gl.BlendFuncSeparate




### gl.EndText
---
```lua
function gl.EndText()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1353-L1355" target="_blank">source</a>]








### gl.Text
---
```lua
function gl.Text(
  text: string,
  x: number,
  y: number,
  size: number,
  options: string?
) ->  nil
```
@param `options` - concatenated string of option characters.

- horizontal alignment:
- 'c' = center
- 'r' = right
- vertical alignment:
- 'a' = ascender
- 't' = top
- 'v' = vertical center
- 'x' = baseline
- 'b' = bottom
- 'd' = descender
- decorations:
- 'o' = black outline
- 'O' = white outline
- 's' = shadow
- other:
- 'n' = don't round vertex coords to nearest integer (font may get blurry)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1363-L1388" target="_blank">source</a>]








### gl.GetTextWidth
---
```lua
function gl.GetTextWidth(text: string) -> width number
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1444-L1448" target="_blank">source</a>]








### gl.GetTextHeight
---
```lua
function gl.GetTextHeight(text: string)
 -> height number
 -> descender number
 -> lines integer

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1459-L1465" target="_blank">source</a>]








### gl.Unit
---
```lua
function gl.Unit(
  unitID: integer,
  doRawDraw: boolean?,
  useLuaMat: integer?,
  noLuaCall: boolean?,
  fullModel: boolean?
)
```
@param `doRawDraw` - (Default: `false`)

@param `noLuaCall` - (Default: `false`) Skip the `DrawUnit` callin.

@param `fullModel` - (Default: `true`)






Draw the unit, applying transform.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1660-L1669" target="_blank">source</a>]








### gl.UnitRaw
---
```lua
function gl.UnitRaw(
  unitID: integer,
  doRawDraw: boolean?,
  useLuaMat: integer?,
  noLuaCall: boolean?,
  fullModel: boolean?
)
```
@param `doRawDraw` - (Default: `false`)

@param `noLuaCall` - (Default: `true`) Skip the `DrawUnit` callin.

@param `fullModel` - (Default: `true`)






Draw the unit without applying transform.

Also skips the `DrawUnit` callin by default so any
recursion is blocked.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1672-L1684" target="_blank">source</a>]








### gl.UnitTextures
---
```lua
function gl.UnitTextures(
  unitID: integer,
  push: boolean
)
```
@param `push` - If `true`, push the render state; if `false`, pop it.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1687-L1691" target="_blank">source</a>]








### gl.UnitShape
---
```lua
function gl.UnitShape(
  unitDefID: integer,
  teamID: integer,
  rawState: boolean?,
  toScreen: boolean?,
  opaque: boolean?
)
```
@param `rawState` - (Default: `true`)

@param `toScreen` - (Default: `false`)

@param `opaque` - (Default: `true`) If `true`, draw opaque; if `false`, draw alpha.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1699-L1706" target="_blank">source</a>]








### gl.UnitShapeTextures
---
```lua
function gl.UnitShapeTextures(
  unitDefID: integer,
  push: boolean
)
```
@param `push` - If `true`, push the render state; if `false`, pop it.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1714-L1718" target="_blank">source</a>]








### gl.UnitMultMatrix
---
```lua
function gl.UnitMultMatrix(unitID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1727-L1730" target="_blank">source</a>]








### gl.UnitPiece
---
```lua
function gl.UnitPiece(
  unitID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1745-L1749" target="_blank">source</a>]








### gl.UnitPieceMatrix
---
```lua
function gl.UnitPieceMatrix(
  unitID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1756-L1760" target="_blank">source</a>]








### gl.UnitPieceMultMatrix
---
```lua
function gl.UnitPieceMultMatrix(
  unitID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1763-L1767" target="_blank">source</a>]








### gl.Feature
---
```lua
function gl.Feature(
  featureID: integer,
  doRawDraw: boolean?,
  useLuaMat: integer?,
  noLuaCall: boolean?
)
```
@param `doRawDraw` - (Default: `false`)

@param `noLuaCall` - (Default: `false`) Skip the `DrawFeature` callin.






Draw the feature, applying transform.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1828-L1836" target="_blank">source</a>]








### gl.FeatureRaw
---
```lua
function gl.FeatureRaw(
  featureID: integer,
  doRawDraw: boolean?,
  useLuaMat: integer?,
  noLuaCall: boolean?
)
```
@param `doRawDraw` - (Default: `false`)

@param `noLuaCall` - (Default: `true`) Skip the `DrawFeature` callin.






Draw the unit without applying transform.

Also skips the `DrawFeature` callin by default so any
recursion is blocked.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1839-L1849" target="_blank">source</a>]








### gl.FeatureTextures
---
```lua
function gl.FeatureTextures(
  featureID: integer,
  push: boolean
)
```
@param `push` - If `true`, push the render state; if `false`, pop it.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1852-L1856" target="_blank">source</a>]








### gl.FeatureShape
---
```lua
function gl.FeatureShape(
  featureDefID: integer,
  teamID: integer,
  rawState: boolean?,
  toScreen: boolean?,
  opaque: boolean?
)
```
@param `rawState` - (Default: `true`)

@param `toScreen` - (Default: `false`)

@param `opaque` - (Default: `true`) If `true`, draw opaque; if `false`, draw alpha.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1864-L1871" target="_blank">source</a>]








### gl.FeatureShapeTextures
---
```lua
function gl.FeatureShapeTextures(
  featureDefID: integer,
  push: boolean
)
```
@param `push` - If `true`, push the render state; if `false`, pop it.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1879-L1883" target="_blank">source</a>]








### gl.FeatureMultMatrix
---
```lua
function gl.FeatureMultMatrix(featureID: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1892-L1895" target="_blank">source</a>]








### gl.FeaturePiece
---
```lua
function gl.FeaturePiece(
  featureID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1910-L1914" target="_blank">source</a>]








### gl.FeaturePieceMatrix
---
```lua
function gl.FeaturePieceMatrix(
  featureID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1922-L1926" target="_blank">source</a>]








### gl.FeaturePieceMultMatrix
---
```lua
function gl.FeaturePieceMultMatrix(
  featureID: integer,
  pieceID: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1930-L1934" target="_blank">source</a>]








### gl.DrawListAtUnit
---
```lua
function gl.DrawListAtUnit(
  unitID: integer,
  listIndex: integer,
  useMidPos: boolean?,
  scaleX: number?,
  scaleY: number?,
  scaleZ: number?,
  degrees: number?,
  rotX: number?,
  rotY: number?,
  rotZ: number?
)
```
@param `useMidPos` - (Default: `true`)

@param `scaleX` - (Default: `1.0`)

@param `scaleY` - (Default: `1.0`)

@param `scaleZ` - (Default: `1.0`)

@param `degrees` - (Default: `0.0`)

@param `rotX` - (Default: `0.0`)

@param `rotY` - (Default: `1.0`)

@param `rotZ` - (Default: `0.0`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L1947-L1959" target="_blank">source</a>]








### gl.DrawFuncAtUnit
---
```lua
function gl.DrawFuncAtUnit(
  unitID: integer,
  useMidPos: boolean?,
  fun: unknown,
  ...: any
)
```
@param `useMidPos` - (Default: `true`)

@param `...` - Arguments passed to function.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2004-L2010" target="_blank">source</a>]








### gl.DrawGroundCircle
---
```lua
function gl.DrawGroundCircle(
  posX: number,
  posY: number,
  posZ: number,
  radius: number,
  resolution: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2047-L2054" target="_blank">source</a>]








### gl.DrawGroundCircle
---
```lua
function gl.DrawGroundCircle(
  posX: number,
  posY: number,
  posZ: number,
  radius: number,
  resolution: integer,
  slope: number,
  gravity: number,
  weaponDefID: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2055-L2065" target="_blank">source</a>]








### gl.DrawGroundCircle
---
```lua
function gl.DrawGroundCircle(
  x0: number,
  z0: number,
  x1: number,
  z1: number,
  useNorm: nil,
  useTxcd: boolean?
)
```
@param `useNorm` - No longer used.

@param `useTxcd` - (Default: `false`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2106-L2114" target="_blank">source</a>]








### gl.DrawGroundCircle
---
```lua
function gl.DrawGroundCircle(
  x0: number,
  z0: number,
  x1: number,
  z1: number,
  useNorm: nil,
  tu0: number,
  tv0: number,
  tu1: number,
  tv1: number
)
```
@param `useNorm` - No longer used.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2115-L2126" target="_blank">source</a>]








### gl.Shape
---
```lua
function gl.Shape(
  type: GL,
  vertices: VertexData[]
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2315-L2319" target="_blank">source</a>]








### gl.BeginEnd
---
```lua
function gl.BeginEnd(
  primMode: GL,
  fun: unknown,
  ...: any
)
```
@param `...` - Arguments passed to function.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2361-L2366" target="_blank">source</a>]








### gl.Vertex
---
```lua
function gl.Vertex(v: xy)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2398-L2401" target="_blank">source</a>]








### gl.Vertex
---
```lua
function gl.Vertex(v: xyz)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2402-L2405" target="_blank">source</a>]








### gl.Vertex
---
```lua
function gl.Vertex(v: xyzw)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2406-L2409" target="_blank">source</a>]








### gl.Vertex
---
```lua
function gl.Vertex(
  x: number,
  y: number,
  z: number?,
  w: number?
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2410-L2416" target="_blank">source</a>]








### gl.Normal
---
```lua
function gl.Normal(v: xyz)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2480-L2483" target="_blank">source</a>]








### gl.Normal
---
```lua
function gl.Normal(
  x: number,
  y: number,
  z: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2484-L2489" target="_blank">source</a>]








### gl.TexCoord
---
```lua
function gl.TexCoord(coord: (number))
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2528-L2531" target="_blank">source</a>]








### gl.TexCoord
---
```lua
function gl.TexCoord(coord: xy)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2532-L2535" target="_blank">source</a>]








### gl.TexCoord
---
```lua
function gl.TexCoord(coord: xyz)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2536-L2539" target="_blank">source</a>]








### gl.TexCoord
---
```lua
function gl.TexCoord(coord: xyzw)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2540-L2543" target="_blank">source</a>]








### gl.TexCoord
---
```lua
function gl.TexCoord(
  s: number,
  t: number?,
  r: number?,
  q: number?
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2544-L2550" target="_blank">source</a>]








### gl.MultiTexCoord
---
```lua
function gl.MultiTexCoord(
  texNum: integer,
  coord: (number)
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2619-L2623" target="_blank">source</a>]








### gl.MultiTexCoord
---
```lua
function gl.MultiTexCoord(
  texNum: integer,
  coord: xy
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2624-L2628" target="_blank">source</a>]








### gl.MultiTexCoord
---
```lua
function gl.MultiTexCoord(
  texNum: integer,
  coord: xyz
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2629-L2633" target="_blank">source</a>]








### gl.MultiTexCoord
---
```lua
function gl.MultiTexCoord(
  texNum: integer,
  coord: xyzw
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2634-L2638" target="_blank">source</a>]








### gl.MultiTexCoord
---
```lua
function gl.MultiTexCoord(
  texNum: integer,
  s: number,
  t: number?,
  r: number?,
  q: number?
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2639-L2646" target="_blank">source</a>]








### gl.SecondaryColor
---
```lua
function gl.SecondaryColor(color: rgb)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2721-L2724" target="_blank">source</a>]








### gl.SecondaryColor
---
```lua
function gl.SecondaryColor(
  r: number,
  g: number,
  b: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2725-L2730" target="_blank">source</a>]








### gl.FogCoord
---
```lua
function gl.FogCoord(coord: number)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2769-L2772" target="_blank">source</a>]








### gl.EdgeFlag
---
```lua
function gl.EdgeFlag(flag: boolean)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2784-L2787" target="_blank">source</a>]








### gl.Rect
---
```lua
function gl.Rect(
  x1: number,
  y1: number,
  x2: number,
  y2: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2802-L2808" target="_blank">source</a>]








### gl.Rect
---
```lua
function gl.Rect(
  x1: number,
  y1: number,
  x2: number,
  y2: number,
  flipSCoords: boolean?,
  flipTCoords: boolean?
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2821-L2829" target="_blank">source</a>]








### gl.Rect
---
```lua
function gl.Rect(
  x1: number,
  y1: number,
  x2: number,
  y2: number,
  s1: number,
  t1: number,
  s2: number,
  t2: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2830-L2840" target="_blank">source</a>]








### gl.DispatchCompute
---
```lua
function gl.DispatchCompute(
  numGroupX: integer,
  numGroupY: integer,
  numGroupZ: integer,
  barriers: integer?
)
```
@param `barriers` - (Default: `4`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2896-L2902" target="_blank">source</a>]








### gl.MemoryBarrier
---
```lua
function gl.MemoryBarrier(barriers: integer?)
```
@param `barriers` - (Default: `4`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2935-L2938" target="_blank">source</a>]








### gl.Color
---
```lua
function gl.Color(
  r: number,
  g: number,
  b: number,
  a: number?
)
```
@param `r` - Red.

@param `g` - Green.

@param `b` - Blue.

@param `a` - (Default: `1.0`) Alpha.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2956-L2962" target="_blank">source</a>]








### gl.Color
---
```lua
function gl.Color(color: rgba)
```
@param `color` - Color with alpha.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2963-L2966" target="_blank">source</a>]








### gl.Color
---
```lua
function gl.Color(color: rgb)
```
@param `color` - Color.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L2967-L2970" target="_blank">source</a>]








### gl.Material
---
```lua
function gl.Material(material: Material)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3018-L3021" target="_blank">source</a>]








### gl.ResetState
---
```lua
function gl.ResetState()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3091-L3093" target="_blank">source</a>]








### gl.ResetMatrices
---
```lua
function gl.ResetMatrices()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3102-L3104" target="_blank">source</a>]








### gl.Lighting
---
```lua
function gl.Lighting(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3120-L3123" target="_blank">source</a>]








### gl.ShadeModel
---
```lua
function gl.ShadeModel(model: GL)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3137-L3140" target="_blank">source</a>]








### gl.Scissor
---
```lua
function gl.Scissor(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3149-L3152" target="_blank">source</a>]








### gl.Scissor
---
```lua
function gl.Scissor(
  x: integer,
  y: integer,
  w: integer,
  h: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3153-L3159" target="_blank">source</a>]








### gl.Viewport
---
```lua
function gl.Viewport(
  x: integer,
  y: integer,
  w: integer,
  h: integer
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3190-L3196" target="_blank">source</a>]








### gl.ColorMask
---
```lua
function gl.ColorMask(rgba: boolean)
```





Enable or disable writing of frame buffer color components.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3213-L3217" target="_blank">source</a>]








### gl.ColorMask
---
```lua
function gl.ColorMask(
  red: boolean,
  green: boolean,
  blue: boolean,
  alpha: boolean
)
```





Enable or disable writing of frame buffer color components.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3218-L3225" target="_blank">source</a>]








### gl.DepthMask
---
```lua
function gl.DepthMask(enable: boolean)
```





Enable or disable writing into the depth buffer.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3249-L3253" target="_blank">source</a>]








### gl.DepthTest
---
```lua
function gl.DepthTest(enable: boolean)
```





Enable or disable depth test.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3266-L3270" target="_blank">source</a>]








### gl.DepthTest
---
```lua
function gl.DepthTest(depthFunction: GL)
```
@param `depthFunction` - Symbolic constants `GL.NEVER`, `GL.LESS`, `GL.EQUAL`, `GL.LEQUAL`,
`GL.GREATER`, `GL.NOTEQUAL`, `GL.GEQUAL`, and `GL.ALWAYS` are accepted.
The initial value is `GL.LESS`.






Enable depth test and specify the depth comparison function.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3271-L3280" target="_blank">source</a>]








### gl.DepthClamp
---
```lua
function gl.DepthClamp(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3308-L3311" target="_blank">source</a>]








### gl.Culling
---
```lua
function gl.Culling(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3325-L3328" target="_blank">source</a>]








### gl.Culling
---
```lua
function gl.Culling(mode: GL)
```
@param `mode` - Specifies whether front- or back-facing facets are candidates for culling.
Symbolic constants `GL.FRONT`, `GL.BACK`, and `GL.FRONT_AND_BACK` are accepted. The
initial value is `GL.BACK`.






Enable culling and set culling mode.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3329-L3337" target="_blank">source</a>]








### gl.LogicOp
---
```lua
function gl.LogicOp(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3366-L3369" target="_blank">source</a>]








### gl.LogicOp
---
```lua
function gl.LogicOp(opCode: GL)
```
@param `opCode` - Specifies a symbolic constant that selects a logical operation. The following
symbols are accepted: `GL.CLEAR`, `GL.SET`, `GL.COPY`, `GL.COPY_INVERTED`,
`GL.NOOP`, `GL.INVERT`, `GL.AND`, `GL.NAND`, `GL.OR`, `GL.NOR`, `GL.XOR`,
`GL.EQUIV`, `GL.AND_REVERSE`, `GL.AND_INVERTED`, `GL.OR_REVERSE`, and
`GL.OR_INVERTED`.  The initial value is `GL.COPY`.






Specify a logical pixel operation for rendering.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3370-L3382" target="_blank">source</a>]








### gl.Fog
---
```lua
function gl.Fog(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3410-L3413" target="_blank">source</a>]








### gl.Blending
---
```lua
function gl.Blending(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3428-L3431" target="_blank">source</a>]








### gl.Blending
---
```lua
function gl.Blending(mode: ("add"|"alpha_add"|"alpha"|"reset"|"color"|"modulate"...))
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3432-L3435" target="_blank">source</a>]








### gl.Blending
---
```lua
function gl.Blending(
  src: GL,
  dst: GL
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3436-L3440" target="_blank">source</a>]








### gl.BlendEquation
---
```lua
function gl.BlendEquation(mode: GL)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3503-L3506" target="_blank">source</a>]








### gl.BlendFunc
---
```lua
function gl.BlendFunc(
  src: GL,
  dst: GL
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3516-L3520" target="_blank">source</a>]








### gl.BlendEquationSeparate
---
```lua
function gl.BlendEquationSeparate(
  modeRGB: GL,
  modeAlpha: GL
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3531-L3535" target="_blank">source</a>]








### gl.BlendFuncSeparate
---
```lua
function gl.BlendFuncSeparate(
  srcRGB: GL,
  dstRGB: GL,
  srcAlpha: GL,
  dstAlpha: GL
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3546-L3552" target="_blank">source</a>]








### gl.AlphaTest
---
```lua
function gl.AlphaTest(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3565-L3568" target="_blank">source</a>]








### gl.AlphaTest
---
```lua
function gl.AlphaTest(
  func: GL,
  ref: number
)
```
@param `func` - Specifies the alpha comparison function. Symbolic constants `GL.NEVER`,
`GL.LESS`, `GL.EQUAL`, `GL.LEQUAL`, `GL.GREATER`, `GL.NOTEQUAL`, `GL.GEQUAL`,
and `GL.ALWAYS` are accepted. The initial value is `GL.ALWAYS`.

@param `ref` - Specifies the reference value that incoming alpha values are compared to.
This value is clamped to the range `[0, 1]`, where `0` represents the lowest
possible alpha value and `1` the highest possible value. The initial reference
value is `0`.






Specify the alpha test function.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3569-L3582" target="_blank">source</a>]








### gl.AlphaToCoverage
---
```lua
function gl.AlphaToCoverage(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3606-L3609" target="_blank">source</a>]








### gl.PolygonMode
---
```lua
function gl.PolygonMode(
  face: GL,
  mode: GL
)
```
@param `face` - Specifies the polygons that mode applies to. Must be `GL.FRONT` for
front-facing polygons, `GL.BACK` for back-facing polygons, or `GL.FRONT_AND_BACK`
for front- and back-facing polygons.

@param `mode` - Specifies how polygons will be rasterized. Accepted values are `GL.POINT`,
`GL.LINE`, and `GL.FILL`. The initial value is `GL.FILL` for both front- and
back-facing polygons.






Select polygon rasterization mode.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3627-L3643" target="_blank">source</a>]








### gl.PolygonOffset
---
```lua
function gl.PolygonOffset(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3654-L3657" target="_blank">source</a>]








### gl.PolygonOffset
---
```lua
function gl.PolygonOffset(
  factor: number,
  units: number
)
```
@param `factor` - Specifies a scale factor that is used to create a variable depth offset for
each polygon. The initial value is `0`.

@param `units` - Is multiplied by an implementation-specific value to create a constant depth
offset. The initial value is `0`.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3658-L3669" target="_blank">source</a>]








### gl.StencilTest
---
```lua
function gl.StencilTest(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3701-L3704" target="_blank">source</a>]








### gl.StencilMask
---
```lua
function gl.StencilMask(mask: integer)
```
@param `mask` - Specifies a bit mask to enable and disable writing of individual bits in the stencil planes. Initially, the mask is all `1`'s.






Control the front and back writing of individual bits in the stencil planes.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3718-L3722" target="_blank">source</a>]








### gl.StencilFunc
---
```lua
function gl.StencilFunc(
  func: GL,
  ref: integer,
  mask: integer
)
```
@param `func` - Specifies the test function. Eight symbolic constants are valid: `GL.NEVER`, `GL.LESS`, `GL.EQUAL`, `GL.LEQUAL`, `GL.GREATER`, `GL.NOTEQUAL`, `GL.GEQUAL`, and `GL.ALWAYS`. The initial value is `GL.ALWAYS`.

@param `ref` - Specifies the reference value for the stencil test. `ref` is clamped to the range `[0, 2^n - 1]`, where `n` is the number of bitplanes in the stencil buffer. The initial value is `0`.

@param `mask` - Specifies a mask that is ANDed with both the reference value and the stored stencil value when the test is done. The initial value is all `1`'s.






Set front and back function and reference value for stencil testing.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3732-L3738" target="_blank">source</a>]








### gl.StencilOp
---
```lua
function gl.StencilOp(
  fail: GL,
  zfail: GL,
  zpass: GL
)
```
@param `fail` - Specifies the action to take when the stencil test fails. Eight symbolic constants are valid: `GL.KEEP`, `GL.ZERO`, `GL.REPLACE`, `GL.INCR`, `GL.INCR_WRAP`, `GL.DECR`, `GL.DECR_WRAP`, and `GL.INVERT`. The initial value is `GL.KEEP`.

@param `zfail` - Specifies the stencil action when the stencil test passes, but the depth test fails. The initial value is `GL.KEEP`.

@param `zpass` - Specifies the stencil action when both the stencil test and the depth test pass, or when the stencil test passes and either there is no depth buffer or depth testing is not enabled. The initial value is `GL.KEEP`.






Set front and back stencil test actions.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3750-L3756" target="_blank">source</a>]








### gl.StencilMaskSeparate
---
```lua
function gl.StencilMaskSeparate(
  face: GL,
  mask: integer
)
```
@param `face` - Specifies whether the front and/or back stencil writemask is updated. Three symbolic constants are accepted: `GL.FRONT`, `GL.BACK`, and `GL.FRONT_AND_BACK`. The initial value is `GL.FRONT_AND_BACK`.

@param `mask` - Specifies a bit mask to enable and disable writing of individual bits in the stencil planes. Initially, the mask is all `1`'s.






Control the front and back writing of individual bits in the stencil planes.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3768-L3773" target="_blank">source</a>]








### gl.StencilFuncSeparate
---
```lua
function gl.StencilFuncSeparate(
  face: GL,
  func: GL,
  ref: integer,
  mask: integer
)
```
@param `face` - Specifies whether front and/or back stencil state is updated. Three symbolic constants are accepted: `GL.FRONT`, `GL.BACK`, and `GL.FRONT_AND_BACK`. The initial value is `GL.FRONT_AND_BACK`.

@param `func` - Specifies the test function. Eight symbolic constants are valid: `GL.NEVER`, `GL.LESS`, `GL.EQUAL`, `GL.LEQUAL`, `GL.GREATER`, `GL.NOTEQUAL`, `GL.GEQUAL`, and `GL.ALWAYS`. The initial value is `GL.ALWAYS`.

@param `ref` - Specifies the reference value for the stencil test. `ref` is clamped to the range `[0, 2^n - 1]`, where `n` is the number of bitplanes in the stencil buffer. The initial value is `0`.

@param `mask` - Specifies a mask that is ANDed with both the reference value and the stored stencil value when the test is done. The initial value is all `1`'s.






Set front and/or back function and reference value for stencil testing.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3784-L3791" target="_blank">source</a>]








### gl.StencilOpSeparate
---
```lua
function gl.StencilOpSeparate(
  face: GL,
  fail: GL,
  zfail: GL,
  zpass: GL
)
```
@param `face` - Specifies whether front and/or back stencil state is updated. Three symbolic constants are accepted: `GL.FRONT`, `GL.BACK`, and `GL.FRONT_AND_BACK`. The initial value is `GL.FRONT_AND_BACK`.

@param `fail` - Specifies the action to take when the stencil test fails. Eight symbolic constants are valid: `GL.KEEP`, `GL.ZERO`, `GL.REPLACE`, `GL.INCR`, `GL.INCR_WRAP`, `GL.DECR`, `GL.DECR_WRAP`, and `GL.INVERT`. The initial value is `GL.KEEP`.

@param `zfail` - Specifies the stencil action when the stencil test passes, but the depth test fails. The initial value is `GL.KEEP`.

@param `zpass` - Specifies the stencil action when both the stencil test and the depth test pass, or when the stencil test passes and either there is no depth buffer or depth testing is not enabled. The initial value is `GL.KEEP`.






Set front and/or back stencil test actions.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3804-L3811" target="_blank">source</a>]








### gl.LineStipple
---
```lua
function gl.LineStipple(enable: boolean)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3826-L3829" target="_blank">source</a>]








### gl.LineStipple
---
```lua
function gl.LineStipple(ignoredString: string)
```
@param `ignoredString` - The value of this string is ignored, but it still does something.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3830-L3833" target="_blank">source</a>]








### gl.LineStipple
---
```lua
function gl.LineStipple(
  factor: integer,
  pattern: integer,
  shift: integer?
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3834-L3839" target="_blank">source</a>]








### gl.LineWidth
---
```lua
function gl.LineWidth(width: number)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3889-L3892" target="_blank">source</a>]








### gl.PointSize
---
```lua
function gl.PointSize(size: number)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3903-L3906" target="_blank">source</a>]








### gl.PointSprite
---
```lua
function gl.PointSprite(
  enable: boolean,
  enableCoordReplace: boolean?,
  coordOrigin: boolean?
)
```
@param `coordOrigin` - `true` for upper left, `false` for lower left, otherwise no change.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3917-L3922" target="_blank">source</a>]








### gl.PointParameter
---
```lua
function gl.PointParameter(
  atten0: number,
  atten1: number,
  atten2: number,
  sizeMin: number?,
  sizeMax: number?,
  sizeFade: number?
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3951-L3959" target="_blank">source</a>]








### gl.Texture
---
```lua
function gl.Texture(
  texNum: integer,
  enable: boolean?
) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3987-L3992" target="_blank">source</a>]








### gl.Texture
---
```lua
function gl.Texture(enable: boolean) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3993-L3997" target="_blank">source</a>]








### gl.Texture
---
```lua
function gl.Texture(
  texNum: integer,
  image: string
) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L3998-L4003" target="_blank">source</a>]








### gl.Texture
---
```lua
function gl.Texture(image: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4004-L4008" target="_blank">source</a>]








### gl.CreateTexture
---
```lua
function gl.CreateTexture(
  xsize: integer,
  ysize: integer,
  texture: Texture
) -> texName string?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4137-L4143" target="_blank">source</a>]








### gl.CreateTexture
---
```lua
function gl.CreateTexture(
  xsize: integer,
  ysize: integer,
  zsize: integer,
  texture: Texture
) -> texName string?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4144-L4151" target="_blank">source</a>]








### gl.ChangeTextureParams
---
```lua
function gl.ChangeTextureParams(
  texName: string,
  params: Texture
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4219-L4223" target="_blank">source</a>]








### gl.DeleteTexture
---
```lua
function gl.DeleteTexture(texName: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4250-L4254" target="_blank">source</a>]








### gl.DeleteTextureFBO
---
```lua
function gl.DeleteTextureFBO(texName: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4271-L4275" target="_blank">source</a>]








### gl.TextureInfo
---
```lua
function gl.TextureInfo(texName: string) -> textureInfo TextureInfo
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4296-L4300" target="_blank">source</a>]








### gl.CopyToTexture
---
```lua
function gl.CopyToTexture(
  texName: string,
  xoff: integer,
  yoff: integer,
  x: integer,
  y: integer,
  w: integer,
  h: integer,
  target: GL?,
  level: GL?
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4322-L4333" target="_blank">source</a>]








### gl.RenderToTexture
---
```lua
function gl.RenderToTexture(
  texName: string,
  fun: unknown,
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4370-L4375" target="_blank">source</a>]








### gl.GenerateMipmap
---
```lua
function gl.GenerateMipmap(texName: string)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4424-L4427" target="_blank">source</a>]








### gl.ActiveTexture
---
```lua
function gl.ActiveTexture(
  texNum: integer,
  func: function,
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4449-L4454" target="_blank">source</a>]








### gl.TextEnv
---
```lua
function gl.TextEnv(
  target: GL,
  pname: GL,
  value: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4483-L4488" target="_blank">source</a>]








### gl.TextEnv
---
```lua
function gl.TextEnv(
  target: GL,
  pname: GL,
  r: number?,
  g: number?,
  b: number?,
  a: number?
)
```
@param `r` - (Default: `0.0`)

@param `g` - (Default: `0.0`)

@param `b` - (Default: `0.0`)

@param `a` - (Default: `0.0`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4489-L4497" target="_blank">source</a>]








### gl.MultiTexEnv
---
```lua
function gl.MultiTexEnv(
  texNum: integer,
  target: GL,
  pname: GL,
  value: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4527-L4533" target="_blank">source</a>]








### gl.MultiTexEnv
---
```lua
function gl.MultiTexEnv(
  texNum: integer,
  target: GL,
  pname: GL,
  r: number?,
  g: number?,
  b: number?,
  a: number?
)
```
@param `r` - (Default: `0.0`)

@param `g` - (Default: `0.0`)

@param `b` - (Default: `0.0`)

@param `a` - (Default: `0.0`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4534-L4543" target="_blank">source</a>]








### gl.TexGen
---
```lua
function gl.TexGen(
  target: GL,
  state: boolean
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4594-L4598" target="_blank">source</a>]








### gl.TexGen
---
```lua
function gl.TexGen(
  target: GL,
  pname: GL,
  value: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4599-L4604" target="_blank">source</a>]








### gl.TexGen
---
```lua
function gl.TexGen(
  target: GL,
  pname: GL,
  r: number?,
  g: number?,
  b: number?,
  a: number?
)
```
@param `r` - (Default: `0.0`)

@param `g` - (Default: `0.0`)

@param `b` - (Default: `0.0`)

@param `a` - (Default: `0.0`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4605-L4613" target="_blank">source</a>]








### gl.MultiTexGen
---
```lua
function gl.MultiTexGen(
  texNum: integer,
  target: GL,
  state: boolean
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4652-L4657" target="_blank">source</a>]








### gl.MultiTexGen
---
```lua
function gl.MultiTexGen(
  texNum: integer,
  target: GL,
  pname: GL,
  value: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4658-L4664" target="_blank">source</a>]








### gl.MultiTexGen
---
```lua
function gl.MultiTexGen(
  texNum: integer,
  target: GL,
  pname: GL,
  r: number?,
  g: number?,
  b: number?,
  a: number?
)
```
@param `r` - (Default: `0.0`)

@param `g` - (Default: `0.0`)

@param `b` - (Default: `0.0`)

@param `a` - (Default: `0.0`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4665-L4674" target="_blank">source</a>]








### gl.BindImageTexture
---
```lua
function gl.BindImageTexture(
  unit: integer,
  texID: string?,
  level: integer?,
  layer: integer?,
  access: integer?,
  format: integer?
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4723-L4731" target="_blank">source</a>]








### gl.CreateTextureAtlas
---
```lua
function gl.CreateTextureAtlas(
  xsize: integer,
  ysize: integer,
  allocType: integer?
) -> texName string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4843-L4849" target="_blank">source</a>]








### gl.FinalizeTextureAtlas
---
```lua
function gl.FinalizeTextureAtlas(texName: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4874-L4878" target="_blank">source</a>]








### gl.DeleteTextureAtlas
---
```lua
function gl.DeleteTextureAtlas(texName: string) ->  boolean
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4894-L4898" target="_blank">source</a>]








### gl.AddAtlasTexture
---
```lua
function gl.AddAtlasTexture(
  texName: string,
  subAtlasTexName: string
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4910-L4914" target="_blank">source</a>]








### gl.GetAtlasTexture
---
```lua
function gl.GetAtlasTexture(
  texName: string,
  subAtlasTexName: string
)
 -> x1 number
 -> x2 number
 -> y1 number
 -> y2 number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4961-L4969" target="_blank">source</a>]








### gl.GetEngineAtlasTextures
---
```lua
function gl.GetEngineAtlasTextures(atlasName: ("$explosions"|"$groundfx")) -> atlasTextures table<string,float4>
```

@return `atlasTextures` - Table of x1,x2,y1,y2 coordinates by texture name.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L4994-L4998" target="_blank">source</a>]








### gl.Clear
---
```lua
function gl.Clear(
  bits: GL,
  val: number
)
```
@param `bits` - `GL.DEPTH_BUFFER_BIT` or `GL.STENCIL_BUFFER_BIT`.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5048-L5052" target="_blank">source</a>]








### gl.Clear
---
```lua
function gl.Clear(
  bits: GL,
  r: number,
  g: number,
  b: number,
  a: number
)
```
@param `bits` - `GL.COLOR_BUFFER_BIT` or `GL.ACCUM_BUFFER_BIT`.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5053-L5060" target="_blank">source</a>]








### gl.SwapBuffers
---
```lua
function gl.SwapBuffers()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5099-L5101" target="_blank">source</a>]








### gl.Translate
---
```lua
function gl.Translate(
  x: number,
  y: number,
  z: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5116-L5121" target="_blank">source</a>]








### gl.Scale
---
```lua
function gl.Scale(
  x: number,
  y: number,
  z: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5134-L5139" target="_blank">source</a>]








### gl.Rotate
---
```lua
function gl.Rotate(
  r: number,
  x: number,
  y: number,
  z: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5152-L5158" target="_blank">source</a>]








### gl.Ortho
---
```lua
function gl.Ortho(
  left: number,
  right: number,
  bottom: number,
  top: number,
  near: number,
  far: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5172-L5180" target="_blank">source</a>]








### gl.Frustum
---
```lua
function gl.Frustum(
  left: number,
  right: number,
  bottom: number,
  top: number,
  near: number,
  far: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5196-L5204" target="_blank">source</a>]








### gl.Billboard
---
```lua
function gl.Billboard()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5220-L5222" target="_blank">source</a>]








### gl.Light
---
```lua
function gl.Light(
  light: integer,
  enable: boolean
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5234-L5238" target="_blank">source</a>]








### gl.Light
---
```lua
function gl.Light(
  light: integer,
  pname: GL,
  param: GL
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5239-L5244" target="_blank">source</a>]








### gl.Light
---
```lua
function gl.Light(
  light: integer,
  pname: GL,
  r: number,
  g: number,
  b: number,
  a: number?
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5245-L5253" target="_blank">source</a>]








### gl.ClipPlane
---
```lua
function gl.ClipPlane(
  plane: integer,
  enable: boolean
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5304-L5308" target="_blank">source</a>]








### gl.ClipPlane
---
```lua
function gl.ClipPlane(
  plane: integer,
  equation0: number,
  equation1: number,
  equation2: number,
  equation3: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5309-L5316" target="_blank">source</a>]








### gl.ClipDistance
---
```lua
function gl.ClipDistance(
  clipId: integer,
  enable: boolean
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5346-L5350" target="_blank">source</a>]








### gl.MatrixMode
---
```lua
function gl.MatrixMode(mode: GL)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5379-L5382" target="_blank">source</a>]








### gl.LoadIdentity
---
```lua
function gl.LoadIdentity()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5395-L5397" target="_blank">source</a>]








### gl.LoadMatrix
---
```lua
function gl.LoadMatrix(matrix: string)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5432-L5435" target="_blank">source</a>]








### gl.LoadMatrix
---
```lua
function gl.LoadMatrix(matrix: Matrix4x4)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5436-L5439" target="_blank">source</a>]








### gl.LoadMatrix
---
```lua
function gl.LoadMatrix()
 -> m11 number
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





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5440-L5458" target="_blank">source</a>]








### gl.MultMatrix
---
```lua
function gl.MultMatrix(matrixName: string)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5491-L5494" target="_blank">source</a>]








### gl.MultMatrix
---
```lua
function gl.MultMatrix(matrix: Matrix4x4)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5495-L5498" target="_blank">source</a>]








### gl.MultMatrix
---
```lua
function gl.MultMatrix(
  m11: number,
  m12: number,
  m13: number,
  m14: number,
  m21: number,
  m22: number,
  m23: number,
  m24: number,
  m31: number,
  m32: number,
  m33: number,
  m34: number,
  m41: number,
  m42: number,
  m43: number,
  m44: number
)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5499-L5517" target="_blank">source</a>]








### gl.PushMatrix
---
```lua
function gl.PushMatrix()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5550-L5552" target="_blank">source</a>]








### gl.PopMatrix
---
```lua
function gl.PopMatrix()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5571-L5573" target="_blank">source</a>]








### gl.PushPopMatrix
---
```lua
function gl.PushPopMatrix(
  matMode1: GL,
  func: fun(),
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5591-L5596" target="_blank">source</a>]








### gl.PushPopMatrix
---
```lua
function gl.PushPopMatrix(
  func: fun(),
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5597-L5601" target="_blank">source</a>]








### gl.GetMatrixData
---
```lua
function gl.GetMatrixData(
  type: GL,
  index: integer
) -> The number
```
@param `type` - Matrix type (`GL.PROJECTION`, `GL.MODELVIEW`, `GL.TEXTURE`).

@param `index` - Matrix index in range `[1, 16]`.


@return `The` - value.





Get value at index of matrix.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5649-L5656" target="_blank">source</a>]








### gl.GetMatrixData
---
```lua
function gl.GetMatrixData(type: GL) -> The Matrix4x4
```
@param `type` - Matrix type (`GL.PROJECTION`, `GL.MODELVIEW`, `GL.TEXTURE`).


@return `The` - matrix.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5657-L5661" target="_blank">source</a>]








### gl.GetMatrixData
---
```lua
function gl.GetMatrixData(index: integer) -> The number
```
@param `index` - Matrix index in range `[1, 16]`.


@return `The` - value.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5662-L5666" target="_blank">source</a>]








### gl.GetMatrixData
---
```lua
function gl.GetMatrixData(name: MatrixName) -> The Matrix4x4
```
@param `name` - The matrix name.


@return `The` - matrix.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5667-L5671" target="_blank">source</a>]








### gl.PushAttrib
---
```lua
function gl.PushAttrib(mask: GL?)
```
@param `mask` - (Default: `GL.ALL_ATTRIB_BITS`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5734-L5737" target="_blank">source</a>]








### gl.PopAttrib
---
```lua
function gl.PopAttrib()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5751-L5753" target="_blank">source</a>]








### gl.UnsafeState
---
```lua
function gl.UnsafeState(
  state: GL,
  func: fun(),
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5762-L5767" target="_blank">source</a>]








### gl.UnsafeState
---
```lua
function gl.UnsafeState(
  state: GL,
  reverse: boolean,
  func: fun(),
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5768-L5774" target="_blank">source</a>]








### gl.GetFixedState
---
```lua
function gl.GetFixedState(
  param: string,
  toStr: boolean?
)
 -> enabled boolean
 -> values any ...

```
@param `toStr` - (Default: `false`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L5801-L5807" target="_blank">source</a>]








### gl.CreateList
---
```lua
function gl.CreateList(
  func: fun(),
  ...: any
)
```
@param `...` - Arguments to the function.






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6089-L6093" target="_blank">source</a>]








### gl.CallList
---
```lua
function gl.CallList(listIndex: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6141-L6144" target="_blank">source</a>]








### gl.DeleteList
---
```lua
function gl.DeleteList(listIndex: integer)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6165-L6168" target="_blank">source</a>]








### gl.Flush
---
```lua
function gl.Flush()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6188-L6190" target="_blank">source</a>]








### gl.Finish
---
```lua
function gl.Finish()
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6199-L6201" target="_blank">source</a>]








### gl.ReadPixels
---
```lua
function gl.ReadPixels(
  x: integer,
  y: integer,
  w: 1,
  h: 1,
  format: GL?
) -> Color number ...
```
@param `format` - (Default: `GL.RGBA`)


@return `Color` - values (color size based on format).





Get single pixel.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6257-L6266" target="_blank">source</a>]








### gl.ReadPixels
---
```lua
function gl.ReadPixels(
  x: integer,
  y: integer,
  w: 1,
  h: integer,
  format: GL?
) -> Column number[][]
```
@param `format` - (Default: `GL.RGBA`)


@return `Column` - of color values (color size based on format).





Get column of pixels.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6267-L6276" target="_blank">source</a>]








### gl.ReadPixels
---
```lua
function gl.ReadPixels(
  x: integer,
  y: integer,
  w: integer,
  h: 1,
  format: GL?
) -> Row number[][]
```
@param `format` - (Default: `GL.RGBA`)


@return `Row` - of color values (color size based on format).





Get row of pixels.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6277-L6286" target="_blank">source</a>]








### gl.ReadPixels
---
```lua
function gl.ReadPixels(
  x: integer,
  y: integer,
  w: integer,
  h: integer,
  format: GL?
) -> Array number[][][]
```
@param `format` - (Default: `GL.RGBA`)


@return `Array` - of columns of color values (color size based on format).





Get row of pixels.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6287-L6296" target="_blank">source</a>]








### gl.SaveImage
---
```lua
function gl.SaveImage(
  x: integer,
  y: integer,
  width: integer,
  height: integer,
  filename: string,
  options: SaveImageOptions?
) -> success boolean?
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6376-L6385" target="_blank">source</a>]








### gl.CreateQuery
---
```lua
function gl.CreateQuery() -> query any
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6459-L6462" target="_blank">source</a>]








### gl.DeleteQuery
---
```lua
function gl.DeleteQuery(query: any)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6483-L6486" target="_blank">source</a>]








### gl.RunQuery
---
```lua
function gl.RunQuery(query: any)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6510-L6513" target="_blank">source</a>]








### gl.GetQuery
---
```lua
function gl.GetQuery(query: any) -> count integer
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6550-L6554" target="_blank">source</a>]








### gl.GetGlobalTexNames
---
```lua
function gl.GetGlobalTexNames() -> List string[]
```

@return `List` - of texture names.





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6577-L6580" target="_blank">source</a>]








### gl.GetGlobalTexCoords
---
```lua
function gl.GetGlobalTexCoords()
 -> xstart number
 -> ystart number
 -> xend number
 -> yend number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6596-L6602" target="_blank">source</a>]








### gl.GetShadowMapParams
---
```lua
function gl.GetShadowMapParams()
 -> x number
 -> y number
 -> z number
 -> w number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6619-L6625" target="_blank">source</a>]








### gl.GetAtmosphere
---
```lua
function gl.GetAtmosphere()
 -> lightDirX number
 -> lightDirY number
 -> lightDirZ number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6635-L6640" target="_blank">source</a>]








### gl.GetAtmosphere
---
```lua
function gl.GetAtmosphere(param: ("fogStart"|"fogEnd"|"pos"|"fogColor"|"skyColor"|"sunColor"...)) ->  any ...
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6641-L6645" target="_blank">source</a>]








### gl.GetSun
---
```lua
function gl.GetSun()
 -> lightDirX number
 -> lightDirY number
 -> lightDirZ number

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6698-L6703" target="_blank">source</a>]








### gl.GetSun
---
```lua
function gl.GetSun(
  param: ("pos"|"dir"|"specularExponent"|"shadowDensity"|"diffuse"|"ambient"...),
  mode: ("ground"|"unit")
)
 -> data1 number?
 -> data2 number?
 -> data3 number?

```
@param `mode` - (Default: `"ground"`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6704-L6711" target="_blank">source</a>]








### gl.GetWaterRendering
---
```lua
function gl.GetWaterRendering(key: string) -> value any ...
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6777-L6781" target="_blank">source</a>]








### gl.GetMapRendering
---
```lua
function gl.GetMapRendering(key: string) -> value any ...
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L6959-L6963" target="_blank">source</a>]








### gl.ObjectLabel
---
```lua
function gl.ObjectLabel(
  objectTypeIdentifier: GL,
  objectID: integer,
  label: string
)
```
@param `objectTypeIdentifier` - Specifies the type of object being labeled.

@param `objectID` - Specifies the name or ID of the object to label.

@param `label` - A string containing the label to be assigned to the object.






Labels an object for use with debugging tools.
May be unavailable and `nil` if the platform doesn't support the feature.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L7003-L7011" target="_blank">source</a>]








### gl.PushDebugGroup
---
```lua
function gl.PushDebugGroup(
  id: integer,
  message: string,
  sourceIsThirdParty: boolean
) ->  nil
```
@param `id` - A numeric identifier for the group, can be any unique number.

@param `message` - A human-readable string describing the debug group. Will be truncated if longer than driver-specific limit

@param `sourceIsThirdParty` - Set the source tag, true for GL_DEBUG_SOURCE_THIRD_PARTY, false for GL_DEBUG_SOURCE_APPLICATION. default false






Pushes a debug marker for debugging tools such as `nVidia nSight 2024.04`,
see https://registry.khronos.org/OpenGL-Refpages/gl4/html/glPushDebugGroup.xhtml .

May be unavailable and `nil` if the platform doesn't support the feature.

Groups are basically named scopes similar to tracy's, and are pushed/popped independently
from GL attribute/matrix push/pop (though of course makes sense to put them together).

Tools are known to struggle to see the annotation for FBOs if they are raw bound.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L7040-L7056" target="_blank">source</a>]








### gl.PopDebugGroup
---
```lua
function gl.PopDebugGroup() ->  nil
```





Pops the most recent GL debug group from the stack (does NOT take the numerical ID from push).
May be unavailable and `nil` if the platform doesn't support the feature.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaOpenGL.cpp#L7078-L7085" target="_blank">source</a>]








### gl.CreateFBO
---
```lua
function gl.CreateFBO(fboDesc: FBODescription) -> fbo FBO
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaFBOs.cpp#L467-L471" target="_blank">source</a>]








### gl.DeleteFBO
---
```lua
function gl.DeleteFBO(fbo: FBO)
```





This doesn't delete the attached objects!

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaFBOs.cpp#L542-L547" target="_blank">source</a>]








### gl.IsValidFBO
---
```lua
function gl.IsValidFBO(
  fbo: FBO,
  target: GL?
)
 -> valid boolean
 -> status number?

```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaFBOs.cpp#L560-L566" target="_blank">source</a>]








### gl.ActiveFBO
---
```lua
function gl.ActiveFBO(
  fbo: FBO,
  func: fun(...),
  ...: any
)
```
@param `...` - args






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaFBOs.cpp#L601-L606" target="_blank">source</a>]








### gl.ActiveFBO
---
```lua
function gl.ActiveFBO(
  fbo: FBO,
  target: GL?,
  func: fun(...),
  ...: any
)
```
@param `...` - args






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaFBOs.cpp#L607-L613" target="_blank">source</a>]








### gl.RawBindFBO
---
```lua
function gl.RawBindFBO(
  fbo: nil,
  target: GL?,
  rawFboId: integer?
) ->  nil
```
@param `target` - (Default: `GL_FRAMEBUFFER_EXT`)

@param `rawFboId` - (Default: `0`)






Bind default or specified via rawFboId numeric id of FBO

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaFBOs.cpp#L677-L685" target="_blank">source</a>]








### gl.RawBindFBO
---
```lua
function gl.RawBindFBO(
  fbo: FBO,
  target: GL?
) -> previouslyBoundRawFboId number
```
@param `target` - (Default: `fbo.target`)






[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaFBOs.cpp#L686-L691" target="_blank">source</a>]








### gl.BlitFBO
---
```lua
function gl.BlitFBO(
  x0Src: number,
  y0Src: number,
  x1Src: number,
  y1Src: number,
  x0Dst: number,
  y0Dst: number,
  x1Dst: number,
  y1Dst: number,
  mask: number?,
  filter: number?
)
```
@param `mask` - (Default: GL_COLOR_BUFFER_BIT)

@param `filter` - (Default: GL_NEAREST)






needs `GLAD_GL_EXT_framebuffer_blit`

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaFBOs.cpp#L719-L732" target="_blank">source</a>]








### gl.BlitFBO
---
```lua
function gl.BlitFBO(
  fboSrc: FBO,
  x0Src: number,
  y0Src: number,
  x1Src: number,
  y1Src: number,
  fboDst: FBO,
  x0Dst: number,
  y0Dst: number,
  x1Dst: number,
  y1Dst: number,
  mask: number?,
  filter: number?
)
```
@param `mask` - (Default: GL_COLOR_BUFFER_BIT)

@param `filter` - (Default: GL_NEAREST)






needs `GLAD_GL_EXT_framebuffer_blit`

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaFBOs.cpp#L733-L748" target="_blank">source</a>]








### gl.ClearAttachmentFBO
---
```lua
function gl.ClearAttachmentFBO(
  target: number?,
  attachment: (GL|Attachment),
  clearValue0: number?,
  clearValue1: number?,
  clearValue2: number?,
  clearValue3: number?
) -> success boolean
```
@param `target` - (Default: `GL.FRAMEBUFFER`)

@param `attachment` - (e.g. `"color0"` or `GL.COLOR_ATTACHMENT0`)

@param `clearValue0` - (Default: `0`)

@param `clearValue1` - (Default: `0`)

@param `clearValue2` - (Default: `0`)

@param `clearValue3` - (Default: `0`)






needs `Platform.glVersionNum >= 30`
Clears the "attachment" of the currently bound FBO type "target" with "clearValues"

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaFBOs.cpp#L818-L829" target="_blank">source</a>]








### gl.GetVAO
---
```lua
function gl.GetVAO() -> vao VAO?
```

@return `vao` - The VAO ref on success, else `nil`





Example:
```
local myVAO = gl.GetVAO()
if myVAO == nil then Spring.Echo("Failed to get VAO") end
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVAO.cpp#L75-L84" target="_blank">source</a>]








### gl.GetVBO
---
```lua
function gl.GetVBO(
  bufferType: GL?,
  freqUpdated: boolean?
) -> VBO VBO?
```
@param `bufferType` - (Default: `GL.ARRAY_BUFFER`) The buffer type to use.

Accepts the following:
- `GL.ARRAY_BUFFER` for vertex data.
- `GL.ELEMENT_ARRAY_BUFFER` for vertex indices.
- `GL.UNIFORM_BUFFER`
- `GL.SHADER_STORAGE_BUFFER`

@param `freqUpdated` - (Default: `true`)

`true` to updated frequently, `false` to update only once.


@return `VBO` - The VBO ref on success, or nil if not supported or an error occurred.





Example:

```lua
local myVBO = gl.GetVBO()
if myVBO == nil then Spring.Echo("Failed to get VBO") end
```

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaVBO.cpp#L124-L151" target="_blank">source</a>]





@see GL.OpenGL_Buffer_Types




### gl.AddFallbackFont
---
```lua
function gl.AddFallbackFont(filePath: string) -> success boolean
```
@param `filePath` - VFS path to the file, for example "fonts/myfont.ttf". Uses VFS.RAW_FIRST access mode.






Adds a fallback font for the font rendering engine.

Fonts added first will have higher priority.
When a glyph isn't found when rendering a font, the fallback fonts
will be searched first, otherwise os fonts will be used.

The application should listen for the unsynced 'FontsChanged' callin so
modules can clear any already reserved display lists or other relevant
caches.

Note the callin won't be executed at the time of calling this method,
but later, on the Update cycle (before other Update and Draw callins).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaFonts.cpp#L219-L235" target="_blank">source</a>]








### gl.ClearFallbackFonts
---
```lua
function gl.ClearFallbackFonts() ->  nil
```





Clears all fallback fonts.

See the note at 'AddFallbackFont' about the 'FontsChanged' callin,
it also applies when calling this method.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaFonts.cpp#L247-L254" target="_blank">source</a>]








### gl.GetShaderLog
---
```lua
function gl.GetShaderLog() -> infoLog string
```





Returns the shader compilation error log. This is empty if the shader linking failed, in that case, check your in/out blocks and ensure they match.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L198-L202" target="_blank">source</a>]








### gl.CreateShader
---
```lua
function gl.CreateShader(shaderParams: ShaderParams) -> shaderID integer
```





Create a shader.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L621-L627" target="_blank">source</a>]








### gl.DeleteShader
---
```lua
function gl.DeleteShader(shaderID: integer)
```





Deletes a shader identified by shaderID

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L789-L793" target="_blank">source</a>]








### gl.UseShader
---
```lua
function gl.UseShader(shaderID: integer) -> linked boolean
```





Binds a shader program identified by shaderID. Pass 0 to disable the shader. Returns whether the shader was successfully bound.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L806-L811" target="_blank">source</a>]








### gl.ActiveShader
---
```lua
function gl.ActiveShader(
  shaderID: integer,
  func: function,
  ...: any
)
```
@param `...` - Arguments






Binds a shader program identified by shaderID, and calls the Lua func with
the specified arguments.

Can be used in NON-drawing events (to update uniforms etc.)!

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L839-L849" target="_blank">source</a>]








### gl.GetActiveUniforms
---
```lua
function gl.GetActiveUniforms(shaderID: integer) -> activeUniforms ActiveUniform[]
```





Query the active (actually used) uniforms of a shader and identify their
names, types (float, int, uint) and sizes (float, vec4, ...).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L931-L938" target="_blank">source</a>]








### gl.GetUniformLocation
---
```lua
function gl.GetUniformLocation(
  shaderID: integer,
  name: string
) -> locationID GL
```





Returns the locationID of a shaders uniform. Needed for changing uniform
values with function `gl.Uniform`.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L966-L974" target="_blank">source</a>]








### gl.Uniform
---
```lua
function gl.Uniform(
  locationID: (GL|string),
  f1: number,
  f2: number?,
  f3: number?,
  f4: number?
)
```
@param `locationID` - uniformName






Sets the uniform float value at the locationID for the currently active
shader. Shader must be activated before setting uniforms.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L1045-L1055" target="_blank">source</a>]








### gl.UniformInt
---
```lua
function gl.UniformInt(
  locationID: (integer|string),
  int1: integer,
  int2: integer?,
  int3: integer?,
  int4: integer?
)
```
@param `locationID` - uniformName






Sets the uniform int value at the locationID for the currently active shader.
Shader must be activated before setting uniforms.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L1086-L1096" target="_blank">source</a>]








### gl.UniformArray
---
```lua
function gl.UniformArray(
  locationID: (integer|string),
  type: UniformArrayType,
  uniforms: number[]
)
```
@param `locationID` - uniformName

@param `uniforms` - Array up to 1024 elements






Sets the an array of uniform values at the locationID for the currently
active shader.

Shader must be activated before setting uniforms.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L1156-L1166" target="_blank">source</a>]








### gl.UniformMatrix
---
```lua
function gl.UniformMatrix(
  locationID: (integer|string),
  matrix: ("shadows"|"camera"|"caminv"|"camprj")
)
```
@param `locationID` - uniformName

@param `matrix` - Name of common matrix.






Sets the a uniform mat4 locationID for the currently active shader.

Shader must be activated before setting uniforms.

Can set one one common matrix like shadow, or by passing 16 additional
numbers for the matrix.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L1205-L1216" target="_blank">source</a>]








### gl.UniformMatrix
---
```lua
function gl.UniformMatrix(
  locationID: (integer|string),
  matrix: number[]
)
```
@param `locationID` - uniformName

@param `matrix` - A 2x2, 3x3 or 4x4 matrix.






Sets the a uniform mat4 locationID for the currently active shader.

Shader must be activated before setting uniforms.

Can set one one common matrix like shadow, or by passing 16 additional
numbers for the matrix.

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L1218-L1229" target="_blank">source</a>]








### gl.GetEngineUniformBufferDef
---
```lua
function gl.GetEngineUniformBufferDef(index: number) -> glslDefinition string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L1306-L1314" target="_blank">source</a>]

Return the GLSL compliant definition of UniformMatricesBuffer(idx=0) or UniformParamsBuffer(idx=1) structure.








### gl.GetEngineModelUniformDataDef
---
```lua
function gl.GetEngineModelUniformDataDef(index: number) -> glslDefinition string
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L1328-L1336" target="_blank">source</a>]

Return the GLSL compliant definition of ModelUniformData structure (per Unit/Feature buffer available on GPU)








### gl.SetGeometryShaderParameter
---
```lua
function gl.SetGeometryShaderParameter(
  shaderID: integer,
  param: number,
  number: number
) ->  nil
```





Sets the Geometry shader parameters for shaderID. Needed by geometry shader programs (check the opengl GL_ARB_geometry_shader4 extension for glProgramParameteri)

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L1346-L1353" target="_blank">source</a>]








### gl.SetTesselationShaderParameter
---
```lua
function gl.SetTesselationShaderParameter(
  param: integer,
  value: integer
) ->  nil
```





Sets the tesselation shader parameters for `shaderID`.

Needed by tesselation shader programs. (Check the opengl
`GL_ARB_tessellation_shader` extension for `glProgramParameteri`).

[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaShaders.cpp#L1374-L1384" target="_blank">source</a>]








### gl.CreateRBO
---
```lua
function gl.CreateRBO(
  xsize: integer,
  ysize: integer,
  data: CreateRBOData
) ->  RBO
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaRBOs.cpp#L157-L163" target="_blank">source</a>]








### gl.DeleteRBO
---
```lua
function gl.DeleteRBO(rbo: RBO)
```





[<a href="https://github.com/beyond-all-reason/RecoilEngine/blob/b29554ca8a91605fa235eafe60ad740783359665/rts/Lua/LuaRBOs.cpp#L233-L236" target="_blank">source</a>]












{% endraw %}