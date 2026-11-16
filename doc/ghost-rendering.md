# Lua ghost rendering

`DrawGhost(info)` is an unsynced, opt-in drawing call-in for LuaUI and unsynced
LuaRules. It runs in the engine's alpha-model pass, for the local observer's
remembered building models. Return `true` to replace that ghost's engine draw;
return `false`/`nil` to keep it. No callback or a Lua error leaves engine rendering
available. Only the first client accepting a ghost owns its draw.

`info` contains `unitID`, `transform`, `aimPos`, `teamColor`, `color`, `state`, and
`drawPass`. Position, model, transform and team color are captured when LOS is lost
and retained after unobserved destruction. `unitID` is historical: it is not a
promise that a live unit exists, and must not be used to render/query a replacement
unit after ID reuse. No alive/dead flag is exposed. `transform` is a column-major
16-number model-to-world matrix. The RGBA arrays use [0,1] components. `state` is
one of `ghost`, `ghostHighlight`, `radarGhost`, `radarGhostHighlight`; the radar
color bucket follows the existing LOS_CONTRADAR convention. `drawPass` follows
Game draw modes (normal=1, reflection=3, refraction=4).

Inside the callback, bind a Lua shader and call `gl.GhostRaw()` without arguments.
It renders the current snapshot, not a unit lookup. It cannot be called outside
this callback, including from a different Lua state. Both GL4 and GLSL provide
compatibility shader attributes `gl_Vertex`, `gl_Normal`, `gl_MultiTexCoord0`.
The model-view matrix includes the saved model transform and static piece bind
pose. The projection matrix is the current camera's. Textures 0 and 1 contain
the engine's model diffuse/team mask and shading/alpha mask respectively. The
shader owns shading, alpha masking, tint and opacity; the engine binds no shader.
Use `gl.UniformMatrix(location, "viewinverse")` to recover world coordinates.

The caller starts with the alpha pass's depth/blend state. The engine restores
shader, vertex-array/buffer bindings, fixed-function attributes and model-view /
projection matrices after dispatch. Callbacks must balance their matrix/attribute
stacks and may not change the render target. Side effects such as issuing draws
cannot be undone if a callback subsequently returns false or errors.

Ordinary GL4 ghosts still use the existing batches. Custom draws use the legacy
static geometry interface on both backends, suitable for a small hover set.
`SetGhostHighlight` and `SetGhostColor` continue to work without a Lua renderer;
the default engine effect is a static tint. Animated effects belong to the game.
