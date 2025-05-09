---
title: "gl"
permalink: "docs/lua-api/gl"
---
# global gl


Lua OpenGL API







---

## methods
---

### gl.CreateRBO
---
```lua
function gl.CreateRBO(
  xsize: integer,
  ysize: integer,
  data: CreateRBOData
) ->  RBO
```












### gl.DeleteRBO
---
```lua
function gl.DeleteRBO(rbo: RBO)
```












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








### gl.ClearFallbackFonts
---
```lua
function gl.ClearFallbackFonts() ->  nil
```





Clears all fallback fonts.

See the note at 'AddFallbackFont' about the 'FontsChanged' callin,
it also applies when calling this method.








### gl.HasExtension
---
```lua
function gl.HasExtension(ext: string) ->  boolean
```












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








### gl.GetString
---
```lua
function gl.GetString(pname: GL)
```





Get a string describing the current OpenGL connection.








### gl.GetScreenViewTrans
---
```lua
function gl.GetScreenViewTrans()
 -> x number
 -> y number
 -> z number

```












### gl.GetViewSizes
---
```lua
function gl.GetViewSizes()
 -> x number
 -> y number

```












### gl.GetViewRange
---
```lua
function gl.GetViewRange()
 -> nearPlaneDist number
 -> farPlaneDist number
 -> minViewRange number
 -> maxViewRange number

```












### gl.SetSlaveMode
---
```lua
function gl.SetSlaveMode(newMode: boolean)
```












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












### gl.DrawMiniMap
---
```lua
function gl.DrawMiniMap(defaultTransform: boolean?)
```
@param `defaultTransform` - (Default: `true`)













### gl.BeginText
---
```lua
function gl.BeginText()
```












### gl.EndText
---
```lua
function gl.EndText()
```












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













### gl.GetTextWidth
---
```lua
function gl.GetTextWidth(text: string) -> width number
```












### gl.GetTextHeight
---
```lua
function gl.GetTextHeight(text: string)
 -> height number
 -> descender number
 -> lines integer

```












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








### gl.UnitTextures
---
```lua
function gl.UnitTextures(
  unitID: integer,
  push: boolean
)
```
@param `push` - If `true`, push the render state; if `false`, pop it.













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













### gl.UnitShapeTextures
---
```lua
function gl.UnitShapeTextures(
  unitDefID: integer,
  push: boolean
)
```
@param `push` - If `true`, push the render state; if `false`, pop it.













### gl.UnitMultMatrix
---
```lua
function gl.UnitMultMatrix(unitID: integer)
```












### gl.UnitPiece
---
```lua
function gl.UnitPiece(
  unitID: integer,
  pieceID: integer
)
```












### gl.UnitPieceMatrix
---
```lua
function gl.UnitPieceMatrix(
  unitID: integer,
  pieceID: integer
)
```












### gl.UnitPieceMultMatrix
---
```lua
function gl.UnitPieceMultMatrix(
  unitID: integer,
  pieceID: integer
)
```












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








### gl.FeatureTextures
---
```lua
function gl.FeatureTextures(
  featureID: integer,
  push: boolean
)
```
@param `push` - If `true`, push the render state; if `false`, pop it.













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













### gl.FeatureShapeTextures
---
```lua
function gl.FeatureShapeTextures(
  featureDefID: integer,
  push: boolean
)
```
@param `push` - If `true`, push the render state; if `false`, pop it.













### gl.FeatureMultMatrix
---
```lua
function gl.FeatureMultMatrix(featureID: integer)
```












### gl.FeaturePiece
---
```lua
function gl.FeaturePiece(
  featureID: integer,
  pieceID: integer
)
```












### gl.FeaturePieceMatrix
---
```lua
function gl.FeaturePieceMatrix(
  featureID: integer,
  pieceID: integer
)
```












### gl.FeaturePieceMultMatrix
---
```lua
function gl.FeaturePieceMultMatrix(
  featureID: integer,
  pieceID: integer
)
```












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













### gl.Shape
---
```lua
function gl.Shape(
  type: GL,
  vertices: VertexData[]
)
```












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













### gl.Vertex
---
```lua
function gl.Vertex(v: xy)
```












### gl.Vertex
---
```lua
function gl.Vertex(v: xyz)
```












### gl.Vertex
---
```lua
function gl.Vertex(v: xyzw)
```












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












### gl.Normal
---
```lua
function gl.Normal(v: xyz)
```












### gl.Normal
---
```lua
function gl.Normal(
  x: number,
  y: number,
  z: number
)
```












### gl.TexCoord
---
```lua
function gl.TexCoord(coord: (number))
```












### gl.TexCoord
---
```lua
function gl.TexCoord(coord: xy)
```












### gl.TexCoord
---
```lua
function gl.TexCoord(coord: xyz)
```












### gl.TexCoord
---
```lua
function gl.TexCoord(coord: xyzw)
```












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












### gl.MultiTexCoord
---
```lua
function gl.MultiTexCoord(
  texNum: integer,
  coord: (number)
)
```












### gl.MultiTexCoord
---
```lua
function gl.MultiTexCoord(
  texNum: integer,
  coord: xy
)
```












### gl.MultiTexCoord
---
```lua
function gl.MultiTexCoord(
  texNum: integer,
  coord: xyz
)
```












### gl.MultiTexCoord
---
```lua
function gl.MultiTexCoord(
  texNum: integer,
  coord: xyzw
)
```












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












### gl.SecondaryColor
---
```lua
function gl.SecondaryColor(color: rgb)
```












### gl.SecondaryColor
---
```lua
function gl.SecondaryColor(
  r: number,
  g: number,
  b: number
)
```












### gl.FogCoord
---
```lua
function gl.FogCoord(coord: number)
```












### gl.EdgeFlag
---
```lua
function gl.EdgeFlag(flag: boolean)
```












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













### gl.MemoryBarrier
---
```lua
function gl.MemoryBarrier(barriers: integer?)
```
@param `barriers` - (Default: `4`)













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













### gl.Color
---
```lua
function gl.Color(color: rgba)
```
@param `color` - Color with alpha.













### gl.Color
---
```lua
function gl.Color(color: rgb)
```
@param `color` - Color.













### gl.Material
---
```lua
function gl.Material(material: Material)
```












### gl.ResetState
---
```lua
function gl.ResetState()
```












### gl.ResetMatrices
---
```lua
function gl.ResetMatrices()
```












### gl.Lighting
---
```lua
function gl.Lighting(enable: boolean)
```












### gl.ShadeModel
---
```lua
function gl.ShadeModel(model: GL)
```












### gl.Scissor
---
```lua
function gl.Scissor(enable: boolean)
```












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












### gl.ColorMask
---
```lua
function gl.ColorMask(rgba: boolean)
```





Enable or disable writing of frame buffer color components.








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








### gl.DepthMask
---
```lua
function gl.DepthMask(enable: boolean)
```





Enable or disable writing into the depth buffer.








### gl.DepthTest
---
```lua
function gl.DepthTest(enable: boolean)
```





Enable or disable depth test.








### gl.DepthTest
---
```lua
function gl.DepthTest(depthFunction: GL)
```
@param `depthFunction` - Symbolic constants `GL.NEVER`, `GL.LESS`, `GL.EQUAL`, `GL.LEQUAL`,
`GL.GREATER`, `GL.NOTEQUAL`, `GL.GEQUAL`, and `GL.ALWAYS` are accepted.
The initial value is `GL.LESS`.






Enable depth test and specify the depth comparison function.








### gl.DepthClamp
---
```lua
function gl.DepthClamp(enable: boolean)
```












### gl.Culling
---
```lua
function gl.Culling(enable: boolean)
```












### gl.Culling
---
```lua
function gl.Culling(mode: GL)
```
@param `mode` - Specifies whether front- or back-facing facets are candidates for culling.
Symbolic constants `GL.FRONT`, `GL.BACK`, and `GL.FRONT_AND_BACK` are accepted. The
initial value is `GL.BACK`.






Enable culling and set culling mode.








### gl.LogicOp
---
```lua
function gl.LogicOp(enable: boolean)
```












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








### gl.Fog
---
```lua
function gl.Fog(enable: boolean)
```












### gl.Blending
---
```lua
function gl.Blending(enable: boolean)
```












### gl.Blending
---
```lua
function gl.Blending(mode: ("add"|"alpha_add"|"alpha"|"reset"|"color"|"modulate"...))
```












### gl.Blending
---
```lua
function gl.Blending(
  src: GL,
  dst: GL
)
```












### gl.BlendEquation
---
```lua
function gl.BlendEquation(mode: GL)
```












### gl.BlendFunc
---
```lua
function gl.BlendFunc(
  src: GL,
  dst: GL
)
```












### gl.BlendEquationSeparate
---
```lua
function gl.BlendEquationSeparate(
  modeRGB: GL,
  modeAlpha: GL
)
```












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












### gl.AlphaTest
---
```lua
function gl.AlphaTest(enable: boolean)
```












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








### gl.AlphaToCoverage
---
```lua
function gl.AlphaToCoverage(enable: boolean)
```












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








### gl.PolygonOffset
---
```lua
function gl.PolygonOffset(enable: boolean)
```












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













### gl.StencilTest
---
```lua
function gl.StencilTest(enable: boolean)
```












### gl.StencilMask
---
```lua
function gl.StencilMask(mask: integer)
```
@param `mask` - Specifies a bit mask to enable and disable writing of individual bits in the stencil planes. Initially, the mask is all `1`'s.






Control the front and back writing of individual bits in the stencil planes.








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








### gl.LineStipple
---
```lua
function gl.LineStipple(enable: boolean)
```












### gl.LineStipple
---
```lua
function gl.LineStipple(ignoredString: string)
```
@param `ignoredString` - The value of this string is ignored, but it still does something.













### gl.LineStipple
---
```lua
function gl.LineStipple(
  factor: integer,
  pattern: integer,
  shift: integer?
)
```












### gl.LineWidth
---
```lua
function gl.LineWidth(width: number)
```












### gl.PointSize
---
```lua
function gl.PointSize(size: number)
```












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












### gl.Texture
---
```lua
function gl.Texture(
  texNum: integer,
  enable: boolean?
) ->  boolean
```












### gl.Texture
---
```lua
function gl.Texture(enable: boolean) ->  boolean
```












### gl.Texture
---
```lua
function gl.Texture(
  texNum: integer,
  image: string
) ->  boolean
```












### gl.Texture
---
```lua
function gl.Texture(image: string) ->  boolean
```












### gl.CreateTexture
---
```lua
function gl.CreateTexture(
  xsize: integer,
  ysize: integer,
  texture: Texture
) -> texName string?
```












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












### gl.ChangeTextureParams
---
```lua
function gl.ChangeTextureParams(
  texName: string,
  params: Texture
)
```












### gl.DeleteTexture
---
```lua
function gl.DeleteTexture(texName: string) ->  boolean
```












### gl.DeleteTextureFBO
---
```lua
function gl.DeleteTextureFBO(texName: string) ->  boolean
```












### gl.TextureInfo
---
```lua
function gl.TextureInfo(texName: string) -> textureInfo TextureInfo
```












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













### gl.GenerateMipmap
---
```lua
function gl.GenerateMipmap(texName: string)
```












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













### gl.TextEnv
---
```lua
function gl.TextEnv(
  target: GL,
  pname: GL,
  value: number
)
```












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













### gl.TexGen
---
```lua
function gl.TexGen(
  target: GL,
  state: boolean
)
```












### gl.TexGen
---
```lua
function gl.TexGen(
  target: GL,
  pname: GL,
  value: number
)
```












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













### gl.MultiTexGen
---
```lua
function gl.MultiTexGen(
  texNum: integer,
  target: GL,
  state: boolean
)
```












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












### gl.CreateTextureAtlas
---
```lua
function gl.CreateTextureAtlas(
  xsize: integer,
  ysize: integer,
  allocType: integer?
) -> texName string
```












### gl.FinalizeTextureAtlas
---
```lua
function gl.FinalizeTextureAtlas(texName: string) ->  boolean
```












### gl.DeleteTextureAtlas
---
```lua
function gl.DeleteTextureAtlas(texName: string) ->  boolean
```












### gl.AddAtlasTexture
---
```lua
function gl.AddAtlasTexture(
  texName: string,
  subAtlasTexName: string
)
```












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












### gl.GetEngineAtlasTextures
---
```lua
function gl.GetEngineAtlasTextures(atlasName: ("$explosions"|"$groundfx")) -> atlasTextures table<string,float4>
```

@return `atlasTextures` - Table of x1,x2,y1,y2 coordinates by texture name.












### gl.Clear
---
```lua
function gl.Clear(
  bits: GL,
  val: number
)
```
@param `bits` - `GL.DEPTH_BUFFER_BIT` or `GL.STENCIL_BUFFER_BIT`.













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













### gl.SwapBuffers
---
```lua
function gl.SwapBuffers()
```












### gl.Translate
---
```lua
function gl.Translate(
  x: number,
  y: number,
  z: number
)
```












### gl.Scale
---
```lua
function gl.Scale(
  x: number,
  y: number,
  z: number
)
```












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












### gl.Billboard
---
```lua
function gl.Billboard()
```












### gl.Light
---
```lua
function gl.Light(
  light: integer,
  enable: boolean
)
```












### gl.Light
---
```lua
function gl.Light(
  light: integer,
  pname: GL,
  param: GL
)
```












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












### gl.ClipPlane
---
```lua
function gl.ClipPlane(
  plane: integer,
  enable: boolean
)
```












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












### gl.ClipDistance
---
```lua
function gl.ClipDistance(
  clipId: integer,
  enable: boolean
)
```












### gl.MatrixMode
---
```lua
function gl.MatrixMode(mode: GL)
```












### gl.LoadIdentity
---
```lua
function gl.LoadIdentity()
```












### gl.LoadMatrix
---
```lua
function gl.LoadMatrix(matrix: string)
```












### gl.LoadMatrix
---
```lua
function gl.LoadMatrix(matrix: Matrix4x4)
```












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












### gl.MultMatrix
---
```lua
function gl.MultMatrix(matrixName: string)
```












### gl.MultMatrix
---
```lua
function gl.MultMatrix(matrix: Matrix4x4)
```












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












### gl.PushMatrix
---
```lua
function gl.PushMatrix()
```












### gl.PopMatrix
---
```lua
function gl.PopMatrix()
```












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













### gl.PushPopMatrix
---
```lua
function gl.PushPopMatrix(
  func: fun(),
  ...: any
)
```
@param `...` - Arguments to the function.













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








### gl.GetMatrixData
---
```lua
function gl.GetMatrixData(type: GL) -> The Matrix4x4
```
@param `type` - Matrix type (`GL.PROJECTION`, `GL.MODELVIEW`, `GL.TEXTURE`).


@return `The` - matrix.












### gl.GetMatrixData
---
```lua
function gl.GetMatrixData(index: integer) -> The number
```
@param `index` - Matrix index in range `[1, 16]`.


@return `The` - value.












### gl.GetMatrixData
---
```lua
function gl.GetMatrixData(name: MatrixName) -> The Matrix4x4
```
@param `name` - The matrix name.


@return `The` - matrix.












### gl.PushAttrib
---
```lua
function gl.PushAttrib(mask: GL?)
```
@param `mask` - (Default: `GL.ALL_ATTRIB_BITS`)













### gl.PopAttrib
---
```lua
function gl.PopAttrib()
```












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













### gl.CreateList
---
```lua
function gl.CreateList(
  func: fun(),
  ...: any
)
```
@param `...` - Arguments to the function.













### gl.CallList
---
```lua
function gl.CallList(listIndex: integer)
```












### gl.DeleteList
---
```lua
function gl.DeleteList(listIndex: integer)
```












### gl.Flush
---
```lua
function gl.Flush()
```












### gl.Finish
---
```lua
function gl.Finish()
```












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












### gl.CreateQuery
---
```lua
function gl.CreateQuery() -> query any
```












### gl.DeleteQuery
---
```lua
function gl.DeleteQuery(query: any)
```












### gl.RunQuery
---
```lua
function gl.RunQuery(query: any)
```












### gl.GetQuery
---
```lua
function gl.GetQuery(query: any) -> count integer
```












### gl.GetGlobalTexNames
---
```lua
function gl.GetGlobalTexNames() -> List string[]
```

@return `List` - of texture names.












### gl.GetGlobalTexCoords
---
```lua
function gl.GetGlobalTexCoords()
 -> xstart number
 -> ystart number
 -> xend number
 -> yend number

```












### gl.GetShadowMapParams
---
```lua
function gl.GetShadowMapParams()
 -> x number
 -> y number
 -> z number
 -> w number

```












### gl.GetAtmosphere
---
```lua
function gl.GetAtmosphere()
 -> lightDirX number
 -> lightDirY number
 -> lightDirZ number

```












### gl.GetAtmosphere
---
```lua
function gl.GetAtmosphere(param: ("fogStart"|"fogEnd"|"pos"|"fogColor"|"skyColor"|"sunColor"...)) ->  any ...
```












### gl.GetSun
---
```lua
function gl.GetSun()
 -> lightDirX number
 -> lightDirY number
 -> lightDirZ number

```












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













### gl.GetWaterRendering
---
```lua
function gl.GetWaterRendering(key: string) -> value any ...
```












### gl.GetMapRendering
---
```lua
function gl.GetMapRendering(key: string) -> value any ...
```












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








### gl.PopDebugGroup
---
```lua
function gl.PopDebugGroup() ->  nil
```





Pops the most recent GL debug group from the stack (does NOT take the numerical ID from push).
May be unavailable and `nil` if the platform doesn't support the feature.








### gl.GetShaderLog
---
```lua
function gl.GetShaderLog() -> infoLog string
```





Returns the shader compilation error log. This is empty if the shader linking failed, in that case, check your in/out blocks and ensure they match.








### gl.CreateShader
---
```lua
function gl.CreateShader(shaderParams: ShaderParams) -> shaderID integer
```





Create a shader.








### gl.DeleteShader
---
```lua
function gl.DeleteShader(shaderID: integer)
```





Deletes a shader identified by shaderID








### gl.UseShader
---
```lua
function gl.UseShader(shaderID: integer) -> linked boolean
```





Binds a shader program identified by shaderID. Pass 0 to disable the shader. Returns whether the shader was successfully bound.








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








### gl.GetActiveUniforms
---
```lua
function gl.GetActiveUniforms(shaderID: integer) -> activeUniforms ActiveUniform[]
```





Query the active (actually used) uniforms of a shader and identify their
names, types (float, int, uint) and sizes (float, vec4, ...).








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








### gl.GetEngineUniformBufferDef
---
```lua
function gl.GetEngineUniformBufferDef(index: number) -> glslDefinition string
```





Return the GLSL compliant definition of UniformMatricesBuffer(idx=0) or UniformParamsBuffer(idx=1) structure.








### gl.GetEngineModelUniformDataDef
---
```lua
function gl.GetEngineModelUniformDataDef(index: number) -> glslDefinition string
```





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








### gl.CreateFBO
---
```lua
function gl.CreateFBO(fboDesc: FBODescription) -> fbo FBO
```












### gl.DeleteFBO
---
```lua
function gl.DeleteFBO(fbo: FBO)
```





This doesn't delete the attached objects!








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








### gl.RawBindFBO
---
```lua
function gl.RawBindFBO(
  fbo: FBO,
  target: GL?
) -> previouslyBoundRawFboId number
```
@param `target` - (Default: `fbo.target`)













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








### gl.CreateRBO
---
```lua
function gl.CreateRBO(
  xsize: integer,
  ysize: integer,
  data: CreateRBOData
) ->  RBO
```












### gl.DeleteRBO
---
```lua
function gl.DeleteRBO(rbo: RBO)
```












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








### gl.ClearFallbackFonts
---
```lua
function gl.ClearFallbackFonts() ->  nil
```





Clears all fallback fonts.

See the note at 'AddFallbackFont' about the 'FontsChanged' callin,
it also applies when calling this method.








### gl.GetShaderLog
---
```lua
function gl.GetShaderLog() -> infoLog string
```





Returns the shader compilation error log. This is empty if the shader linking failed, in that case, check your in/out blocks and ensure they match.








### gl.CreateShader
---
```lua
function gl.CreateShader(shaderParams: ShaderParams) -> shaderID integer
```





Create a shader.








### gl.DeleteShader
---
```lua
function gl.DeleteShader(shaderID: integer)
```





Deletes a shader identified by shaderID








### gl.UseShader
---
```lua
function gl.UseShader(shaderID: integer) -> linked boolean
```





Binds a shader program identified by shaderID. Pass 0 to disable the shader. Returns whether the shader was successfully bound.








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








### gl.GetActiveUniforms
---
```lua
function gl.GetActiveUniforms(shaderID: integer) -> activeUniforms ActiveUniform[]
```





Query the active (actually used) uniforms of a shader and identify their
names, types (float, int, uint) and sizes (float, vec4, ...).








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








### gl.GetEngineUniformBufferDef
---
```lua
function gl.GetEngineUniformBufferDef(index: number) -> glslDefinition string
```





Return the GLSL compliant definition of UniformMatricesBuffer(idx=0) or UniformParamsBuffer(idx=1) structure.








### gl.GetEngineModelUniformDataDef
---
```lua
function gl.GetEngineModelUniformDataDef(index: number) -> glslDefinition string
```





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








### gl.CreateFBO
---
```lua
function gl.CreateFBO(fboDesc: FBODescription) -> fbo FBO
```












### gl.DeleteFBO
---
```lua
function gl.DeleteFBO(fbo: FBO)
```





This doesn't delete the attached objects!








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








### gl.RawBindFBO
---
```lua
function gl.RawBindFBO(
  fbo: FBO,
  target: GL?
) -> previouslyBoundRawFboId number
```
@param `target` - (Default: `fbo.target`)













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











