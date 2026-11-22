# Lua ghost rendering

`DrawGhost(info)` is an unsynced, opt-in drawing call-in for LuaUI and unsynced
LuaRules. It runs in the engine's alpha-model pass, for the local observer's
remembered building models. Return `true` to replace that ghost's engine draw;
return `false`/`nil` to keep it. No callback or a Lua error leaves engine rendering
available. Only the first client accepting a ghost owns its draw.

`info.ghostID` identifies the local observer's sighting. `info.teamColor`
is its four-component RGBA team color in [0,1], saved on loss of LOS. The
engine uses the stored model and transform for both the native draw and Lua draw.

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
To highlight one sighting, compare the picked ghost ID with `info.ghostID` and
draw it in the desired color. Return false for the other sightings.

## Querying remembered aimpoints

`Spring.GetGhostsInCylinder(x, z, radius[, allyTeamID])` returns an array of
sighting IDs. The horizontal radius includes aimpoints on the boundary. A
radius of zero matches the exact aim XZ. Radar contacts with a remembered
model are included. Full-read gadgets without a specific read allyteam supply
the observer explicitly. LuaUI defaults to its read allyteam.

`Spring.GetGhostPosition(ghostID[, allyTeamID])` returns the frozen aim XYZ.
Its optional fourth return value is the unit ID while the same unit is in
LOS or radar. Foreign or expired IDs return no values. Treat sighting IDs as
opaque. They are distinct across observers and separate from unit and feature
IDs. Seeing an empty site expires its sighting whether the unit moved away
or was destroyed under fog. Submerged sightings use the sonar properties at
loss of LOS and the observer's current sensor coverage. Hidden ownership changes
preserve the sighting and its remembered icon. Several sightings of the same
unit can coexist after detachment.

With ghost picking enabled, `Spring.TraceScreenRay` returns `"ghost", ghostID`
for a remembered model. With radar contact a model hit returns
`"unit", unitID, ghostID`. A plain unit or radar icon hit returns
`"unit", unitID`. Full-read LOS and radar call-ins receive the sighting ID
as an optional fifth argument. The ID remains valid during `UnitEnteredLos`
and expires after that callback.

## Positional command height

`CMD.OPT_KEEP_HEIGHT` (`keepheight`) preserves the supplied Y for positional
Attack and Manual Fire. The basecontent LuaRules gadget
`LuaRules/Gadgets/cmd_keep_height_compat.lua` clears this bit on direct orders
and on the embedded options of `CMD.INSERT`. Unflagged orders pass through.
The gadget reissues flagged orders through synced Lua. Command listeners see
that issuer on the replacement. A custom Lua order timeout is lost on reissue.
Games that validate explicit heights themselves can override this file with
`return false`. Custom handlers that load only game archives must include
the base gadget explicitly if they want the legacy policy.
The option survives queue insertion. Where the native command drawer normally
projects an endpoint onto terrain or raises it for visibility, it uses the stored
XYZ for flagged orders, including custom positional commands. Movement, building
placement and weapon-specific targeting rules still govern command execution.
Synced game code validates the observer's sighting before issuing a positional
attack. The engine accepts the ordinary one-parameter unit and XYZ forms of Attack.
