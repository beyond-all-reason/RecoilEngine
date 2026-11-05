# Attack movement dispatch validation

Use an isolated data/write directory with development maps and BAR carrying
the AttackCommandMovement gadget-handler entry from this commit. Keep the
Lua movement policy disabled. Never use the production installation.


Install `dispatch_rules.lua` and `dispatch_later.lua` in the isolated game's gadget directory and
`dispatch_gaia.lua` as that game's `LuaGaia/main.lua`. Quicksilver has map-side
LuaGaia entry points; the mod-first loader selects this opt-in fixture. Run
`dispatch.txt` through `run.py`. Expected: `PASS dispatch=6`. The fixture checks
that true stops dispatch across LuaRules/LuaGaia, false/nil reaches Gaia once,
and replacing the command or destroying its target/owner prevents further
dispatch, both to a later LuaRules gadget and to LuaGaia. `dispatch_later.lua`
uses BAR's ascending layer order. Games with their own handler must carry the
invalidation guard from the engine's `gadgetHandler:AttackCommandMovement`;
checking only the literal true return is insufficient.
Remove the fixture from any game intended for actual play.
